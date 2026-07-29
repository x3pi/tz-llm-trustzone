# Quy trình cấp nguồn (provisioning) — build lại từ đầu, flash một board Orange Pi 5 Max mới

Tài liệu này hướng dẫn từng bước cho hai việc:
1. **Build lại toàn bộ phần mềm từ mã nguồn** (TEE-OS/ChCore, kernel Linux,
   U-Boot/ATF/OP-TEE) chỉ dùng những gì có trong repo `project/` này.
2. **Cấp nguồn (provision) cho một board** — có thể là thẻ SD hoàn toàn
   trắng (thiết bị mới) hoặc cập nhật lại một thẻ đã cấp nguồn từ trước.

Về *lịch sử* mỗi bước được phát hiện/debug ra sao (bug nào, thử nghiệm nào
thất bại, nguyên nhân gốc là gì), xem `STATUS.md`. Tài liệu này chỉ nói về
*quy trình thực hiện* — chạy gì, theo thứ tự nào, và làm sao biết đã thành
công.

## 0. Cần chuẩn bị gì

- **Phần cứng**: một board Orange Pi 5 Max (RK3588), một cáp USB-A sang
  USB-C cắm vào cổng MaskROM/OTG của board, một mạch chuyển USB-to-TTL UART
  nối vào chân UART của board (baud **1500000**, không phải 115200 như
  thường thấy), và một thẻ SD (dung lượng bất kỳ — partition `userdata`
  trong GPT tự động giãn ra để lấp đầy phần còn trống).
- **Máy host**: Linux có `docker`, `rkdeveloptool` (hoặc dùng bản có sẵn
  trong `tools/rkdeveloptool/`), và quyền `sudo` để truy cập thiết bị USB.
- **Repo này**: đã clone/checkout ở một commit đã biết là tốt. Chạy `git log
  --oneline -1` để xác nhận đang ở đâu; `git status` nên sạch trước khi bắt
  đầu (thay đổi chưa commit thường là trạng thái debug còn sót lại, không
  phải chủ ý).

## 1. Build phần mềm từ mã nguồn

Việc build chạy bên trong container Docker (`tzllm_fixed_builder`, image
`vectorxj0553/tz-llm-oh-builder:latest`) đã được tạo sẵn một lần và nên
được tái sử dụng (`docker start`), không nên tạo lại mỗi lần build — build
từ số 0 sẽ mất thời gian lâu hơn nhiều so với build incremental mà container
này cho phép.

```
docker start tzllm_fixed_builder   # nếu chưa chạy
```

**Cái bẫy cần biết trước khi sửa mã nguồn**: các bind mount của container
vẫn đang trỏ vào đường dẫn cũ `tz-llm-ae/tz-llm/...` trên host, không phải
`project/tz-llm/...` (xem `STATUS.md` mục "Container bind-mount trap"). Nếu
bạn sửa bất cứ thứ gì trong `project/tz-llm/{tee_os_kernel,
linux-5.10-opi,tzdriver}/`, bạn **bắt buộc** phải chạy lệnh dưới đây trước,
nếu không container sẽ âm thầm build tiếp nội dung cũ chưa sửa:

```
./scripts/kick-the-tires/sync-to-container.sh
```

Sau đó chạy build thật sự — có thể chạy toàn bộ qua script đã có sẵn. **Lưu
ý cờ `-i` là bắt buộc** — thiếu nó, `docker exec` không forward stdin vào
container, và `bash -s < file` sẽ chạy như một no-op hoàn toàn im lặng
(exit code 0 nhưng không có gì được thực thi cả — đã kiểm chứng thực tế,
không phải suy đoán):

```
docker exec -i tzllm_fixed_builder bash -s < scripts/kick-the-tires/build-oh-docker.sh
```

hoặc nếu chỉ cần build thô (không copy ảnh ra, không patch IRQ NPU, không
build idblock phụ), chạy 3 lệnh bên trong mà script đó thực hiện (dùng
`bash -c` với lệnh truyền trực tiếp thì không cần `-i`, vì không có stdin
nào cần forward):

```
docker exec tzllm_fixed_builder bash -c '
  cd /home/vectorxj/openharmony/
  ./chcore.sh
  ./linux.sh
  ./chcore.sh
'
```

Cách thứ hai (3 lệnh thô) là cách đã thực tế dùng và kiểm chứng trong
project này (ví dụ khi chỉ cần rebuild sau khi đổi `LOG_LEVEL` trong
`tz-llm/tee_os_kernel/kernel/Makefile`). Cách thứ nhất (script wrapper đầy
đủ) đúng về mặt cú pháp nhưng **chưa được chạy thử thực tế trong project
này** — nếu dùng, hãy quan sát kỹ output để chắc chắn nó thực thi đúng.

**Luôn chạy `chcore.sh` cả trước lẫn sau `linux.sh`.** Nếu chỉ chạy
`chcore.sh` một mình, hoặc bỏ qua lần gọi thứ hai, `uboot.img` tạo ra sẽ có
FIT hash không khớp với kernel/`boot.img` vừa build, và board sẽ từ chối
boot (`FIT: No boot partition` hoặc lỗi kiểm tra hash). Đây không phải bước
tùy chọn, và thông báo lỗi cũng không nói rõ nguyên nhân thật.

Kết quả nằm ở `/home/vectorxj/openharmony/out/uboot/src_tmp/{boot.img,
uboot.img}` bên trong container. Nếu bạn chạy bằng script wrapper, nó đã tự
copy ra `scripts/kick-the-tires/share_build/images/{boot.img, uboot.img}`
trên host rồi (đường dẫn này được bind-mount vào container ở
`/home/vectorxj/share`) — không cần làm gì thêm. Nếu bạn chạy 3 lệnh thô ở
trên, cần copy ra thủ công:

```
docker cp tzllm_fixed_builder:/home/vectorxj/openharmony/out/uboot/src_tmp/boot.img  <dest>/boot.img
docker cp tzllm_fixed_builder:/home/vectorxj/openharmony/out/uboot/src_tmp/uboot.img <dest>/uboot_fresh.img
```

**`uboot.img` thô này không thể tự boot được** — bản U-Boot do pipeline
Docker build ra là bản release/production, không có CLI và dùng sai quy ước
tên partition (`boot` thay vì `boot_linux` mà project này dùng). Cần đóng
gói lại (repack) với một bản U-Boot đã biết là tốt trước khi dùng:

```
bash flash/repack.sh
# mặc định dùng scripts/kick-the-tires/share_build/images/uboot.img -- nếu
# bạn đã copy uboot.img ra thủ công thì truyền đường dẫn tường minh làm $1
# -> ghi ra checkpoints/uboot_repacked.img
```

`repack.sh` chỉ trích xuất phần OP-TEE/TEE-OS (`tee.bin`) từ bản build mới
của bạn, rồi kết hợp với các binary U-Boot/ATF cố định, đã biết là tốt,
được commit sẵn trong `scripts/kick-the-tires/repack/`. Bạn không cần (và
không nên cố) làm cho U-Boot tự build đúng từ pipeline Docker này — phần đó
đã được giải quyết và giữ cố định.

Đến đây bạn đã có 2 file sẵn sàng để flash:
- `checkpoints/uboot_repacked.img`
- `<dest>/boot.img` (copy vào `checkpoints/boot.img` nếu muốn đây là file
  mặc định mà `flash.sh`/`flash-full.sh` sẽ dùng)

## 2. Cấp nguồn cho board

**Trước khi flash, nên xác minh 2 file checkpoint trên đĩa khớp với commit
git bạn nghĩ mình đang ở** — trong quá trình debug, rất dễ vô tình để lại
một bản build thử nghiệm (ví dụ bản bật thêm log debug) đè lên
`checkpoints/uboot_repacked.img`/`checkpoints/boot.img` mà không nhận ra:

```
sha256sum checkpoints/uboot_repacked.img checkpoints/boot.img
git show HEAD:checkpoints/uboot_repacked.img | sha256sum
git show HEAD:checkpoints/boot.img | sha256sum
```

Hai cặp hash phải khớp nhau từng đôi một. Nếu không khớp, hoặc bạn cố ý
đang dùng một bản chưa commit, hãy chắc chắn đó là *chủ ý*, không phải sót
lại từ một lần debug trước.

### 2a. Thẻ SD trắng / mới (chưa từng chạy ảnh của project này)

Đưa board vào chế độ MaskROM: giữ nút MaskROM, cấp nguồn, rồi thả nút ra.
Xác nhận bằng `lsusb | grep 2207` — bạn sẽ thấy thiết bị Rockchip
`2207:350b` **không** có hậu tố `USB-MSC` (xem phần "Những cái bẫy cần biết"
bên dưới nếu thấy hậu tố đó).

```
SUDO_PW=matkhaucuaban bash flash/flash-full.sh \
  checkpoints/uboot_repacked.img checkpoints/boot.img
```

Lệnh này thực hiện theo thứ tự: ghi GPT từ
`assets/full-flash/parameter_custom.txt` → ghi idbloader
(`assets/full-flash/MiniLoaderAll_official.bin`) → ghi uboot → ghi
boot_linux → ghi system → ghi vendor → ghi userdata. uboot/boot_linux được
ghi theo chunk kèm verify bằng đa số phiếu bầu 3 lần đọc lại (nhỏ, được
kiểm tra hash lúc boot, chỉ cần sai 1 byte là hỏng); system/vendor/userdata
chỉ được spot-check đầu/giữa/cuối (ảnh filesystem lớn, verify toàn bộ không
khả thi về mặt thời gian — khối dữ liệu hỏng ở đó sẽ được fsck bắt lúc boot
thật thay vì kiểm tra trước).

**Nếu bạn đang cấp nguồn lại cho một thẻ đã có dữ liệu người dùng thật**
(model GGUF đã tải về từ trước, v.v.) và không muốn ghi đè nó bằng
`assets/full-flash/userdata.img` (chỉ là một F2FS mẫu rỗng nhỏ, không phải
dữ liệu thật), thêm `SKIP_USERDATA=1`:

```
SUDO_PW=matkhaucuaban SKIP_USERDATA=1 bash flash/flash-full.sh \
  checkpoints/uboot_repacked.img checkpoints/boot.img
```

Phần idbloader (`MiniLoaderAll_official.bin`, loader chính thức của
Rockchip từ RKDevTool) đã mất 5 lần thử thất bại mới tìm ra — xem
`STATUS.md` các attempt #1-#8 nếu sau này cần xem lại. **Đừng thay bằng một
file `MiniLoaderAll.bin` khác** mà không verify lại từ đầu đến cuối — các
file cùng tên nhưng khác nguồn không thể dùng thay cho nhau (file
`device_opi5plus_REAL/loader/MiniLoaderAll.bin` của chính project này là
một file khác, cũ hơn, đã thử và thất bại).

### 2b. Thẻ đã cấp nguồn từ trước (chỉ cập nhật uboot/TEE-OS/kernel)

Nhanh hơn nhiều — chỉ động vào `uboot` và `boot_linux`:

```
SUDO_PW=matkhaucuaban bash flash/flash.sh \
  checkpoints/uboot_repacked.img checkpoints/boot.img
```

## 3. Boot lần đầu / kiểm tra

Sau khi một trong hai script flash chạy xong, nó sẽ tự reset mềm
(`rkdeveloptool rd`), nhưng **lệnh này không đáng tin cậy** — coi nó như
một cú hích, không phải phép test thật. Để test boot thật sự, hãy rút và
cắm lại điện board (**không** giữ nút MaskROM lần này).

Theo dõi UART:

```
tools/uart/uart_cmd.sh "" 30
```

Một lần boot thành công sẽ hiện, theo thứ tự: kiểm tra hash ảnh OP-TEE
thành công, banner khởi tạo ATF/BL31 (`NOTICE: BL31: v2.3()...`), sau đó là
log cold-boot của chính ChCore/TEE-OS (`[ChCore] lock init finished`,
`uart init finished`, `per-CPU info init finished`, `mm init finished`,
... cuối cùng `chanmgr` khởi chạy `llama-cli`).

**Lưu ý**: U-Boot trên ảnh của project này không tự động boot (autoboot) —
nếu cần tự tay nạp và boot kernel từ U-Boot prompt, các lệnh là:

```
mmc dev 0
mmc read 0x10000000 0x39000 0x20000
bootm 0x10000000
```

## 4. Những cái bẫy cần biết trước (đọc trước khi gặp phải)

- **`lsusb` hiện thiết bị Rockchip với hậu tố `USB-MSC` và mọi lệnh
  `rkdeveloptool` bị treo hoặc lỗi.** Thử lại bằng phần mềm không giải
  quyết được — cần rút/cắm điện vật lý thật, không phải lệnh `rd`/reset.
- **Kênh USB/MaskROM thỉnh thoảng làm hỏng dữ liệu** — cả đọc lẫn ghi, tỷ
  lệ hỏng quan sát được khoảng 10-20% mỗi lần đơn lẻ. Đây chính là *lý do*
  `flash-full.sh`/`flash.sh` ghi theo chunk kèm verify bằng đa số phiếu bầu
  3 lần đọc, thay vì tin vào mã thoát (exit code) của `rkdeveloptool`. Đừng
  "đơn giản hóa" phần này đi.
- **Không bao giờ để script tự suy ra LBA của partition uboot/boot_linux
  một cách động** (ví dụ bằng cách parse output của `rkdeveloptool ppt`).
  Một phiên bản trước của `flash.sh` đã làm vậy qua `awk`, một ký tự `\r`
  lạc đã làm sai kết quả so khớp, và nó âm thầm ghi 64MB bắt đầu từ LBA
  0x0 — phá hỏng MBR/GPT và idbloader. Hãy dùng các hằng số cố định từ
  `parameter_custom.txt` (`uboot@0x2000`, `boot_linux@0x88000`) — các giá
  trị này không đổi giữa các thẻ; chỉ có kích thước/điểm kết thúc của
  `userdata` là thay đổi.
- **`sudo` cần mật khẩu thật** để `rkdeveloptool` truy cập USB thô. Truyền
  qua biến `SUDO_PW=...`, đừng bao giờ hardcode mật khẩu vào một script có
  thể bị commit.
- **Board có thể rơi về MaskROM dù nội dung đã ghi và verify hash khớp
  100%.** Đã gặp thực tế: cùng một bộ file (`checkpoints/uboot_repacked.img`
  + `checkpoints/boot.img` + `MiniLoaderAll_official.bin`, đã verify hash
  khớp từng byte) — có lần boot thành công sâu vào tới ChCore init, có lần
  rơi thẳng về MaskROM ngay ở lần cắm điện tiếp theo, dù không hề đổi file
  nào. Rất có thể là chập chờn phần cứng/kênh USB-MaskROM, không phải lỗi
  nội dung. **Đừng vội kết luận "file sai" hay bắt đầu sửa lại code/script
  chỉ vì một lần rơi MaskROM** — trước tiên hãy: (1) xác minh lại hash trên
  đĩa vẫn khớp git HEAD (mục 2 ở trên), (2) rút/cắm điện thật vài lần nữa,
  (3) nếu vẫn thất bại liên tục ≥3-4 lần với đúng file đã biết là tốt, khi
  đó mới nghi ngờ có gì thay đổi thật (xem `STATUS.md` để so sánh với các
  lần thất bại đã ghi nhận).

## 5. Tái sử dụng cho nhiều board Orange Pi 5 Max tiếp theo

Mọi thứ ở trên nên hoạt động không cần sửa đổi cho một board Orange Pi 5
**Max** khác (không phải Plus — nhiều fix trong project này là đặc thù cho
5-Max, ví dụ số IRQ của NPU trong patch `set-npu-irq.sh` của
`scripts/kick-the-tires/build-oh-docker.sh`, 27/28/29 cho Max so với
29/30/31 cho Plus). Những thứ cần kiểm tra lại lần đầu khi cấp nguồn cho
một **board vật lý mới**:

- **Số IRQ của NPU** — đã xác nhận đúng cho toàn bộ dòng 5-Max qua
  `/proc/interrupts` (`fdab0000.npu` ở IRQ 27/28/29) trên đúng 1 board đã
  test suốt lịch sử project này. Nếu một board mới báo số IRQ khác, patch
  `set-npu-irq.sh` trong `build-oh-docker.sh` cần được cập nhật lại.
- **Idbloader (`MiniLoaderAll_official.bin`) mới chỉ được chứng minh boot
  được trên đúng 1 tổ hợp board+thẻ vật lý cho tới nay.** Đây là loader
  *chính thức* của Rockchip (không trích xuất từ một thẻ cụ thể nào), nên
  về lý thuyết không có lý do gì để nó gắn với một thẻ/board cụ thể, nhưng
  điều này chưa được kiểm chứng qua nhiều board vật lý khác nhau. Nếu một
  board mới rơi về MaskROM với đúng file này, đó là thông tin mới — xem
  các attempt #1-#8 trong `STATUS.md` để có quy trình chẩn đoán (treo im
  lặng và bị từ chối "sạch" về MaskROM là hai kiểu lỗi khác nhau, nguyên
  nhân khác nhau).
- **Dung lượng thẻ SD** — partition `userdata` trong `parameter_custom.txt`
  là kiểu `grow` (lấp đầy phần còn trống), nên các dung lượng thẻ khác nhau
  không cần sửa gì. Thẻ nhỏ hơn khoảng 4GB sẽ không đủ chỗ cho các
  partition cố định (`uboot` 256M + `boot_linux` 96M + `system` 2G +
  `vendor` 1G + vài partition quản trị nhỏ) — không phải mối lo thực tế với
  bất kỳ thẻ nào đáng dùng ở đây.
- **Backup trước khi cấp nguồn, nếu thẻ nguồn có dữ liệu thật**: một thẻ có
  thể đọc/ghi trực tiếp như một block device Linux thông thường qua đầu
  đọc thẻ USB (bỏ qua hoàn toàn `rkdeveloptool`/MaskROM, nhanh và đáng tin
  cậy hơn nhiều):
  ```
  sudo dd if=/dev/sdX of=backup.img bs=1M count=3445   # idbloader..vendor, ~3.6GB
  ```
  (`3445` MiB phủ từ sector 0 đến `0x6BA000`, tức mọi thứ trước `sys-prod`
  — đủ để khôi phục một hệ thống boot được, không phải bản sao toàn bộ ổ
  đĩa. Hãy xác minh đúng thiết bị trước — kiểm tra qua `lsblk`/dung lượng —
  trước khi tin vào `/dev/sdX`, vì đầu đọc thẻ USB có thể nhận ký tự ổ đĩa
  khác nhau mỗi lần cắm.)
