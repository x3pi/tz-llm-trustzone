# Quy trình cấp nguồn (provisioning) — build lại từ đầu, flash một board Orange Pi 5 Max mới

Tài liệu này hướng dẫn: build lại toàn bộ phần mềm từ mã nguồn, và cấp
nguồn (provision) cho một board — bao gồm cách triển khai nhất quán cho
**nhiều board khác nhau**.

Về *lịch sử* mỗi bước được phát hiện/debug ra sao, xem `STATUS.md`. Về
*checklist rule/footgun* ngắn gọn, xem `CLAUDE.md`. Tài liệu này nói về
*quy trình thực hiện đầy đủ* — chạy gì, theo thứ tự nào, cái nào build
được cái nào không, và làm sao biết đã thành công.

## 0. Cần chuẩn bị gì

- **Phần cứng**: một board Orange Pi 5 Max (RK3588), một cáp USB-A sang
  USB-C cắm vào cổng MaskROM/OTG của board, một mạch chuyển USB-to-TTL UART
  nối vào chân UART của board (baud **1500000**, không phải 115200 như
  thường thấy), và một thẻ SD/eMMC.
- **Máy host**: Linux có `docker`, `rkdeveloptool` (hoặc dùng bản có sẵn
  trong `tools/rkdeveloptool/`), và quyền `sudo` để truy cập thiết bị USB.
- **Docker images** (kéo về sẵn từ Docker Hub, **không phải build bởi repo
  này** — xem mục 1 bên dưới để biết tên chính xác từng image).
- **Repo này**: đã clone/checkout ở một commit đã biết là tốt. Chạy `git log
  --oneline -1` để xác nhận đang ở đâu; `git status` nên sạch trước khi bắt
  đầu.

## 1. Kiến trúc: cái gì build từ code, cái gì là asset cố định

Đây là câu hỏi quan trọng nhất khi triển khai lên máy/board mới — nhầm lẫn
giữa hai loại này là nguồn gốc phần lớn các sự cố trong lịch sử project.

### 1a. Build được từ source trong repo này (tái tạo lại bất cứ lúc nào)

| Artifact | Build bằng | Nguồn code |
|---|---|---|
| `checkpoints/uboot_repacked.img` (chứa TEE-OS/ChCore, component `optee`) | `./rebuild.sh` rồi `./flash/repack.sh` | `tz-llm/tee_os_kernel/`, `tz-llm/llama.cpp/` (phần TA/chcore-API) |
| `checkpoints/boot.img` (kernel Linux + ramdisk + tzdriver.ko) | `./rebuild.sh` (copy thủ công từ `scripts/kick-the-tires/share_build/images/boot.img`) | `tz-llm/linux-5.10-opi/`, `tz-llm/tzdriver/` |
| CA binaries (`fake`, `libggml.so`, `libllama.so`, `libremoting_backend.so`, `llama-cli` trong `scripts/kick-the-tires/share/build-rknpure/`) | `./scripts/kick-the-tires/llama-builder.sh scripts/kick-the-tires/share bash -c ./build-llama-docker.sh` | `tz-llm/llama.cpp/` (phần CA/rknpure) |

**Cả 2 loại binary (TA nhúng trong `uboot_repacked.img` và CA trên
`scripts/kick-the-tires/share/build-rknpure/`) đều biên dịch từ CÙNG một
thư mục source `tz-llm/llama.cpp/`** — chỉ khác cấu hình CMake
(`LLAMA_USE_CHCORE_API=ON` cho TA vs `OFF` cho CA). Sửa file trong
`tz-llm/llama.cpp/` luôn cần build lại **cả hai**, không chỉ một.

### 1b. Asset cố định — KHÔNG build được, phải lấy sẵn/copy nguyên trạng

| Asset | Là gì | Lấy từ đâu |
|---|---|---|
| `assets/full-flash/rk3588_spl_loader_v1.21.114.bin` | SPL loader của Rockchip cho chế độ full-flash | Vendor Rockchip (đã có sẵn trong repo, đã commit) |
| `assets/full-flash/MiniLoaderAll_official.bin` | idbloader chính thức từ gói RKDevTool của Rockchip | Vendor Rockchip (đã có sẵn trong repo) — **đừng thay bằng file khác cùng tên**, xem mục 4 |
| `assets/full-flash/parameter_custom.txt` | Bảng định nghĩa GPT partition | Hand-authored (config, không phải code build) |
| `assets/full-flash/system_real.img`, `vendor_real.img` | Ảnh partition OpenHarmony hệ thống | **Trích xuất/dump từ một board đã chạy tốt trước đó** — repo này không build OpenHarmony userland từ source |
| `assets/full-flash/userdata.img` | Template F2FS trống cho partition `userdata` | Tạo sẵn — **⚠️ xem cảnh báo quan trọng ở mục 2c, chưa được xác nhận tự boot lên được một cách đáng tin cậy** |
| `assets/full-flash/secure_storage.img` | Ảnh partition secure storage | Trích xuất/dump sẵn |
| `checkpoints/golden-image/idbloader_through_vendor.img` (3.4GB, **không nằm trong git**) | Bản backup thô nguyên khối (LBA 0 → hết vendor) từ một thẻ SD đã biết chạy tốt | Snapshot chụp lại bằng `dd`/`rkdeveloptool rl`, không phải build. **Đây là artifact quan trọng nhất để triển khai board mới — xem mục 2c** |
| GGUF model files (`tinyllama-1.1b-chat-v1.0.Q8_0.gguf`, v.v., trên `/data/ssd/`) | Trọng số model đã huấn luyện sẵn | Tải về từ nguồn công khai (HuggingFace...), không phải build |
| `tools/bin/mkimage-rkbin`, các binary trong `scripts/kick-the-tires/repack/` | U-Boot/ATF/BL31 đã biết là tốt, dùng để repack | Đã build/lấy sẵn 1 lần, giữ cố định — xem `flash/repack.sh`'s comment |
| Docker images `vectorxj0553/tz-llm-oh-builder:latest`, `vectorxj0553/tz-llm-llama-builder:latest` | Toolchain build (cross-compiler, SDK OpenHarmony...) | Kéo về từ Docker Hub, không build từ Dockerfile trong repo này |

## 2. Quy trình build + flash

### 2a. Build từ source

```bash
# 1. Build CA/TA binaries từ tz-llm/llama.cpp/ (LUÔN làm trước nếu sửa gì trong llama.cpp)
docker run --rm \
    -v $(pwd)/tz-llm/tee_os_kernel:/home/vectorxj/openharmony/base/tee/tee_os_kernel \
    -v $(pwd)/tz-llm/llama.cpp:/home/vectorxj/chcore/opentrustee_llm/llama.cpp \
    -v $(pwd)/scripts/kick-the-tires/share:/home/vectorxj/share \
    -w /home/vectorxj/share \
    vectorxj0553/tz-llm-llama-builder:latest \
    bash -c ./build-llama-docker.sh
# (lưu ý: KHÔNG dùng `-it` nếu chạy không tương tác/qua script tự động —
# cần TTY thật, sẽ hang/lỗi nếu không có. `docker run --rm` không cần
# container thường trực, không cần bước "sync-to-container" nào cả.)

# 2. Rebake TEE-OS/kernel (tự động lấy CA/TA binaries mới nhất từ bước 1)
./rebuild.sh

# 3. Repack uboot với TEE-OS mới
./flash/repack.sh

# 4. Copy boot.img mới (repack.sh KHÔNG tự làm bước này)
cp scripts/kick-the-tires/share_build/images/boot.img checkpoints/boot.img
```

**Xác minh trước khi flash**: `checkpoints/uboot_repacked.img`/`boot.img`
nên khớp git HEAD nếu bạn không cố ý build bản mới:
```bash
sha256sum checkpoints/uboot_repacked.img checkpoints/boot.img
git show HEAD:checkpoints/uboot_repacked.img | sha256sum
git show HEAD:checkpoints/boot.img | sha256sum
```

### 2b. Board đã có sẵn OpenHarmony của project này (chỉ cập nhật uboot/TEE-OS/kernel)

Đây là đường **nhanh và đã kiểm chứng nhiều lần** trong project này:

```bash
# Board vào MaskROM: giữ nút MaskROM, cấp nguồn, thả nút. Xác nhận:
lsusb | grep 2207   # KHÔNG được có hậu tố "USB-MSC"

SUDO_PW=matkhaucuaban ./flash/flash.sh
```

Chỉ ghi `uboot`+`boot_linux`, verify chunk-by-chunk đầy đủ, nhanh (vài phút).

### 2c. Board hoàn toàn mới / thẻ SD trắng — ⚠️ ĐỌC KỸ TRƯỚC KHI LÀM

**Có 2 cách, và chỉ 1 cách hiện đã được kiểm chứng đáng tin cậy:**

**Cách A — Clone golden-image (ĐÃ KIỂM CHỨNG, khuyến nghị dùng để nhân
bản board mới)**: ghi thẳng `checkpoints/golden-image/
idbloader_through_vendor.img` (3.4GB, idbloader+GPT+system+vendor từ một
board đã biết chạy tốt) vào thẻ SD/eMMC mới, sau đó flash uboot/boot_linux
đã fix lên trên:

```bash
# Board vào MaskROM
./flash/recover-golden-image.sh
```

Script này (ban đầu viết để khôi phục sau sự cố, nhưng dùng được y hệt để
cấp nguồn board mới) tự làm 2 việc: ghi golden-image raw vào LBA 0, rồi
gọi `flash.sh` để đảm bảo uboot/boot_linux là bản đã fix mới nhất. **Đã
xác nhận 2 lần trên phần cứng thật boot lên đúng và chạy NPU+CPU thành
công** (2026-08-06, 2026-08-07).

Nhược điểm: dùng chung `userdata` từ golden-image (đã có account/wifi
config của lần chụp gốc) — không phải "trắng hoàn toàn" theo đúng nghĩa,
cần đổi wifi credentials / account nếu deploy cho môi trường khác.

**Cách B — `flash-full.sh` từ asset thật sự trắng — ⚠️ CHƯA ĐÁNG TIN CẬY,
KHÔNG khuyến nghị cho tới khi điều tra thêm**:

```bash
SUDO_PW=matkhaucuaban ./flash/flash-full.sh
```

Mặc định (từ 2026-08-07) **không ghi đè `userdata`** — an toàn cho board
đã có dữ liệu thật. Nhưng nếu bạn **chủ động** thêm `FORCE_USERDATA=1` để
ghi `assets/full-flash/userdata.img` (dùng cho trường hợp muốn thật sự
trắng hoàn toàn): **đã thử nghiệm 2 lần (2026-08-07) và cả 2 lần board
không boot lên OS được** (lần 1: UART im lặng + USB liên tục
re-enumerate, giống reset-loop; lần 2: UART im lặng ~35+ phút dù USB ổn
định — cả 2 lần đều phải khôi phục bằng Cách A). **Chưa xác định được
nguyên nhân gốc** (có thể `userdata.img` thiếu feature `encrypt` cần
thiết cho StorageDaemon — xem lịch sử fix tương tự hồi 2026-07-20 ở
memory/`STATUS.md`, hoặc không tương thích với `system_real.img`/
`vendor_real.img` hiện tại). **Việc cần làm nếu muốn dùng Cách B**: điều
tra tại sao `userdata.img` không tự boot được, có thể cần build lại nó
bằng đúng công thức `mke2fs -O encrypt`/mkfs.f2fs đã dùng để fix lockscreen
trước đây, rồi test lại độc lập trước khi tin dùng cho nhiều board.

**Tóm lại: dùng Cách A (`recover-golden-image.sh`) để triển khai board
mới cho tới khi Cách B được điều tra và xác nhận lại.**

## 3. Boot lần đầu / kiểm tra

Sau khi flash xong, rút và cắm lại điện board (**không** giữ nút MaskROM
lần này — `rkdeveloptool rd` không đáng tin cậy để tự boot).

```bash
tools/uart/uart_cmd.sh "" 30
```

Boot thành công sẽ hiện: kiểm tra hash OP-TEE thành công, banner ATF/BL31,
log cold-boot ChCore/TEE-OS, cuối cùng `chanmgr` khởi chạy `llama-cli`.

Sau đó (mỗi lần boot, không tự động):
```bash
tools/uart/uart_cmd.sh "param set persist.hdc.port 8710" 4
tools/uart/uart_cmd.sh "param set ohos.ctl.stop hdcd" 4
tools/uart/uart_cmd.sh "/system/bin/hdcd -t &" 4
hdc tconn <ip-board>:8710
hdc -t <ip-board>:8710 shell "mkdir -p /data/ssd && mount -t ext4 /dev/block/nvme0n1p1 /data/ssd"
```

Rồi push CA binaries mới nhất (nếu chưa có/không khớp) và chạy test xác
nhận NPU+CPU — xem `TESTING_GUIDE.md`.

## 4. Những cái bẫy cần biết trước (đọc trước khi gặp phải)

- **`lsusb` hiện thiết bị Rockchip với hậu tố `USB-MSC`**: mọi lệnh
  `rkdeveloptool` sẽ treo (đặc biệt lệnh `cs 2`, không có timeout). Rút/cắm
  điện vật lý thật để vào lại đúng MaskROM, không có cách sửa bằng phần mềm.
- **Kênh USB/MaskROM thỉnh thoảng làm hỏng dữ liệu** (10-20% mỗi lần đơn
  lẻ) — đây là lý do `flash.sh`/`flash-full.sh` ghi theo chunk kèm verify
  đa số phiếu 3 lần đọc. Đừng đơn giản hóa phần này.
- **Không để script tự suy ra LBA động** (parsing `rkdeveloptool ppt`) —
  dùng hằng số cố định từ `parameter_custom.txt`.
- **`flash-full.sh` mặc định (từ 2026-08-07) không ghi userdata** — phải
  chủ động `FORCE_USERDATA=1` mới ghi đè, và xem cảnh báo mục 2c trước khi
  làm vậy.
- **Nếu board im lặng/reset-loop sau bất kỳ lần flash nào**: chạy
  `./flash/recover-golden-image.sh` (cần MaskROM) — không cần tự ghép lại
  quy trình thủ công.
- **`sudo` cần mật khẩu thật** cho `rkdeveloptool` — truyền qua
  `SUDO_PW=...`, đừng hardcode vào script có thể bị commit.
- **Board có thể rơi về MaskROM dù nội dung đã ghi/verify hash khớp
  100%** — nghi chập chờn phần cứng/kênh USB, không phải lỗi nội dung.
  Đừng vội sửa code chỉ vì 1 lần rơi MaskROM; verify lại hash, thử rút/cắm
  điện vài lần, chỉ nghi ngờ thật khi thất bại liên tục ≥3-4 lần.

## 5. Tái sử dụng cho nhiều board Orange Pi 5 Max khác

- **Cách triển khai khuyến nghị**: Cách A (mục 2c) — `recover-golden-image.sh`
  rồi push CA binaries. Đã kiểm chứng lặp lại được, không phụ thuộc vào
  `assets/full-flash/userdata.img` (chưa đáng tin cậy).
- **Số IRQ của NPU** — đã xác nhận đúng cho dòng 5-Max (`fdab0000.npu` ở
  IRQ 27/28/29) trên 1 board test suốt lịch sử project. Board mới báo số
  khác thì cần cập nhật `set-npu-irq.sh` trong `build-oh-docker.sh`.
- **Idbloader** — `MiniLoaderAll_official.bin` (dùng trong `flash-full.sh`)
  mới chỉ chứng minh boot được trên đúng 1 tổ hợp board+thẻ. Golden-image
  (Cách A) tránh vấn đề này hoàn toàn vì nó dùng idbloader đã build sẵn
  trong chính bản backup, không cần tách riêng.
- **Dung lượng thẻ SD** — partition `userdata` kiểu `grow`, không cần sửa
  gì cho thẻ khác dung lượng (miễn ≥4GB).
- **Backup trước khi cấp nguồn cho thẻ có dữ liệu thật**:
  ```bash
  sudo dd if=/dev/sdX of=backup.img bs=1M count=3445   # idbloader..vendor, ~3.6GB
  ```
  (xác minh đúng thiết bị qua `lsblk` trước khi tin `/dev/sdX`.)
