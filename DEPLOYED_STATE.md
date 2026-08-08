# Trạng thái đang thực sự chạy trên board (cập nhật thủ công sau mỗi lần flash)

**Đây là nguồn sự thật duy nhất cho câu hỏi "cái gì đang chạy trên board ngay bây giờ".**
Đọc file này trước khi flash bất cứ thứ gì — đừng suy đoán từ timestamp/tên file.

## ĐÃ GIẢI QUYẾT (2026-08-08): tìm ra và fix gốc rễ bug treo xác suất, đã commit `d64f42801`

**Trạng thái: ĐÃ XÁC NHẬN ỔN ĐỊNH.** Sau một phiên điều tra rất dài (tối 08-07 → sáng 08-08,
xem phần "CẢNH BÁO 2026-08-07" bên dưới để biết toàn bộ quá trình bisect/loại trừ dẫn tới phát
hiện này), đã tìm ra và fix đúng nguyên nhân gốc rễ của bug treo xác suất đã tồn tại từ lâu
(lịch sử ghi trong "Bug #2"/STATUS.md, tưởng là do NPU nhưng thực ra ảnh hưởng cả `-s 1`).

**Nguyên nhân thật sự**: `llm_tee_os_init()` trong `tc_client_driver.c` gửi địa chỉ SHM
(`CMD_QUEUE_SHM`) cho luồng main của TA (`llama-cli`) **đúng 1 lần duy nhất**, ngay lúc Linux
kernel `module_init` chạy. Nếu tại đúng thời điểm đó, luồng main của TA (do `chanmgr` khởi chạy,
bản thân cũng đang trong quá trình boot userspace của ChCore) **chưa kịp park** ở
`usys_tee_wait_switch_req()` để nhận request này, `handle_yield_smc()` phía secure-world không có
gì để đánh thức, quay sang chạy nhầm luồng idle của `chanmgr` (luôn trả lời `SMC_EXIT_NORMAL`).
Vì `SMC_EXIT_NORMAL != SMC_EXIT_PREEMPTED`, code cũ coi đây là "xong việc" và **không bao giờ
retry** — request bị mất vĩnh viễn, TA's main-thread park mãi mãi, không bao giờ tạo được luồng
compute nào. Xác nhận trực tiếp bằng dmesg: hàng triệu round-trip SMC chỉ toàn `SMC_EXIT_NORMAL`
từ idle-thread, `push #` = 0 tuyệt đối suốt cả lượt treo.

**Fix**: `llm_tee_os_init()` giờ xác thực THẬT (kiểm tra marker `"msg from tee"` mà TA ghi vào
chính SHM ngay sau khi nhận đúng) thay vì tin mù vào giá trị SMC trả về, và retry có giới hạn
(tối đa 300 lần × 50ms ≈ 15s) cho tới khi xác nhận thành công. Đồng thời **revert** một thử
nghiệm không liên quan cùng ngày (chuyển `smc_percpu_structs` trong `smc.c` sang mảng per-CPU) —
thử nghiệm đó vô tình phá vỡ CHÍNH handshake này theo cách khác: `llm_tee_os_init()` luôn chạy
trên CPU boot của Linux, nhưng luồng main của TA có thể park trên core VẬT LÝ khác (ChCore tự
quyết định) — với mảng per-CPU, 2 core không khớp nhau thì không bao giờ thấy nhau, dù retry bao
nhiêu lần cũng vô ích (xác nhận thực nghiệm: 300/300 lần retry đều thất bại khi dùng per-CPU
array). Bản gốc (`smc_percpu_structs` là 1 struct dùng chung, không phân biệt core) là đúng cho
cơ chế NÀY, vì đồng bộ hoá thật sự dựa vào `smc_struct_lock`, không phải core affinity.

**Đã xác nhận trên phần cứng** (nhiều lượt liên tiếp sau reboot sạch): `llm_tee_os_init` báo
"marker confirmed after 1 attempt" (không cần retry trong thực tế), và cả NPU (`-s 0`) lẫn CPU
(`-s 1`) đều cho output mạch lạc — "Hi there! My name is Alex.", "Sure! My name is Alex.",
"Sure! My name is John." — 3/3 lượt liên tiếp thành công (2 NPU + 1 CPU) trên bản đã commit.

**File đã sửa** (commit `d64f42801`): `tz-llm/tzdriver/core/tc_client_driver.c` (retry-verify
handshake + diagnostic trace), `tz-llm/llama.cpp/examples/main/main.cpp` (marker ghi vào SHM sau
lần wake thứ 2, dùng để chẩn đoán — không ảnh hưởng logic chạy thật).
`tz-llm/tee_os_kernel/kernel/arch/aarch64/trustzone/spd/opteed/smc.c` đã revert về nguyên bản
(không còn thay đổi nào so với git history trước đó).

## CẢNH BÁO (2026-08-07, phiên tối) — LỊCH SỬ, đã giải quyết ở trên

**Trạng thái: ĐÃ GIẢI QUYẾT (xem mục "ĐÃ GIẢI QUYẾT (2026-08-08)" ở trên) — phần dưới đây giữ
nguyên văn để tham khảo lịch sử quá trình bisect, không xóa.**

3 lượt test độc lập liên tiếp (mỗi lượt sau reboot sạch, TA mới) trong tối 2026-08-07 đều **treo
hoàn toàn giống hệt bug 08-05** ("Vấn đề đang mở (2026-08-05)" bên dưới, từng được đánh dấu "ĐÃ
VƯỢT QUA" sau 08-06): `push #`/`npu_submit`/`npu_done` = 0 tuyệt đối (chưa từng đạt
`SMC_EXIT_SHADOW` lần nào), `[DBG_LOG_DUMP] last 0 of 0 events`, cả 4 luồng `ca_thread` ở trạng
thái Linux `R` với `wchan=0` (đơ ngay bên trong lệnh SMC đầu tiên, không bao giờ trả về) — burn
~400% CPU vô thời hạn, không panic, không dmesg error.

- Lượt 1+2: build có 2 fix `smc.c` cùng ngày (assign-before-use + mảng per-CPU
  `smc_percpu_structs[PLAT_CPU_NUM]`) — treo cả 2/2.
- Lượt 3: **đã revert fix per-CPU**, chỉ giữ fix assign-before-use (bản từng cho "ít nhất 1 lượt
  thành công" theo ghi nhận trước đó trong phiên) — **vẫn treo giống hệt** (hash optee
  `487473f9f76a0bc30c28dc06e70bee100186f9513d20c7fa87816201f6cb6141`).

**Đã loại trừ**: `tzasc_cma[3] is NULL` (nghi vấn hàng đầu của bug 08-05) — dmesg xác nhận cả 4
vùng `tzasc_cma[0-3]` đều "reserved 768MiB OK" bình thường lần này, không phải nguyên nhân lần
này (hoặc ít nhất không biểu hiện giống hệt).

**Đã xác nhận KHÔNG phải nguyên nhân**: file ngoài `smc.c`/`decrypt-stage.cpp` bị đổi ngoài ý
muốn — `git status`/`git diff --stat` xác nhận chỉ 2 file này (+ checkpoint binaries) khác HEAD,
không có thay đổi lạ nào khác lọt vào build hôm nay.

**Nghi vấn chưa kiểm chứng, việc cần làm tiếp theo**: vì CẢ 2 biến thể `smc.c` (per-CPU array và
bản gốc-đã-sửa-assign-before-use) đều treo **giống hệt nhau và tệ hơn** baseline 08-06 (từng có
~43% tỷ lệ treo nhưng KHÔNG PHẢI 100%, và không phải "treo tuyệt đối trước push đầu tiên" mà là
treo giữa chừng) — nghi ngờ smc.c KHÔNG PHẢI nguyên nhân chính của lần treo này, mà là một
regression khác, có thể do hao mòn card/NVMe sau nhiều chu kỳ flash liên tục trong phiên (đã flash
lại thẻ SD full-chain nhiều lần hôm nay: fix#2, revert), hoặc một biến số môi trường khác chưa xác
định. Bước tiếp theo hợp lý: `git checkout -- tz-llm/tee_os_kernel/kernel/arch/aarch64/trustzone/spd/opteed/smc.c`
(quay về bản smc.c gốc, KHÔNG có bất kỳ fix nào của 08-07) rồi build+flash+test lại — nếu bản gốc
CŨNG treo giống hệt, xác nhận smc.c không liên quan, cần điều tra hướng khác (CA-side NVMe binary
build 08-06 có thể lệch ABI với TEE-OS build 08-07 mới, dù trên lý thuyết interface.h không đổi).
Tiến trình treo (PID 1962, lượt 3) **cố tình không kill** để giữ nguyên trạng thái phục vụ điều
tra sâu hơn nếu cần.

### CẬP NHẬT: Lượt 4 (smc.c HOÀN TOÀN NGUYÊN BẢN, `git checkout --`) — VẪN TREO GIỐNG HỆT

Đã build+flash+test bản `smc.c` quay về đúng HEAD (không còn bất kỳ fix nào của 08-07, kể cả fix
assign-before-use), hash optee `be8ac8d7ca7085d24f819fcf64022f9142b19a46101a49100eb92eefa5de2b71`.
**Kết quả: TREO GIỐNG HỆT 3 lượt trước** (push#=0 tuyệt đối, `[DBG_LOG_DUMP] last 0 of 0 events`).

**=> KẾT LUẬN DỨT KHOÁT: `smc.c` KHÔNG PHẢI nguyên nhân của lần treo tối 08-07.** Đã test đủ 3
biến thể (per-CPU array, assign-before-use only, hoàn toàn nguyên bản) — cả 3 đều treo 100% giống
hệt nhau (4/4 lượt độc lập sau reboot sạch). Đã revert `smc.c` về nguyên bản qua `git checkout --`
(KHÔNG áp dụng lại fix assign-before-use — cần re-áp dụng nếu sau này xác định nguyên nhân thật và
quay lại làm việc này).

**Đã loại trừ thêm**: binary CA trên NVMe SSD (`fake`, `libggml.so`) — hash trên board khớp
CHÍNH XÁC với build tương ứng trên host (`scripts/kick-the-tires/share/build-rknpure/`), không bị
corrupt/lệch ABI.

**Nghi vấn còn lại, chưa kiểm chứng**: (1) hao mòn/suy giảm độ tin cậy thẻ SD sau rất nhiều chu kỳ
flash liên tục trong cùng 1 phiên (hôm nay đã flash lại uboot+boot_linux ít nhất 4 lần); (2)
`vendor`/`system` partition có thể đang lệch phiên bản so với kernel/TA build hôm nay (cần so
sánh ngày build qua UART, chưa làm); (3) đơn giản là vẫn cùng 1 bug xác suất đã biết (~40-57%
tỷ lệ treo lịch sử) và 4 lần liên tiếp chỉ là xui rủi (xác suất thấp nhưng không phải bằng 0).
**Tạm dừng bisect `smc.c` tại đây** — cần hướng điều tra khác trước khi tiếp tục flash thêm.

### CẬP NHẬT: Lượt 5 — TÌM RA THỦ PHẠM THẬT SỰ, THÀNH CÔNG TRỞ LẠI

Nhờ người dùng cắm thẻ SD cũ (bản 08-06 từng thành công) vào USB để so sánh trực tiếp: hash
`uboot`/`atf-1/2/3`/`fdt` của thẻ cũ **khớp 100%** với mọi bản build hôm nay, nhưng hash `optee`
(`e8040d4a3768595bc336f6e7976b68572d9f53d7e21e79c472e4ab1ae67fafd9`, "Created: Thu Aug 6 12:17:56
2026") **khác hoàn toàn** mọi bản build hôm nay (đã thử 4 biến thể). Khác biệt logic duy nhất còn
lại giữa git HEAD và trạng thái đã test: fix `decrypt-stage.cpp` (đổi `pending_addr[4]` từ
max-heap mặc định sang min-heap `std::greater`, xem lịch sử fix ở trên).

Đã `git checkout --` **CẢ HAI** file (`smc.c` VÀ `decrypt-stage.cpp`) về đúng git HEAD (bản
hoàn toàn pristine, không còn fix nào của 08-07), build lại (hash optee mới `4d737636...`, không
khớp tuyệt đối `e8040d4a...` của thẻ cũ nhưng đây là bình thường — project đã di chuyển thư mục
`OPTEE/project/` → `tz-llm-trustzone/` từ 08-06 nên đường dẫn tuyệt đối nhúng trong binary qua
debug info/`__FILE__` đổi, không phải khác biệt logic thật), flash, test.

**Kết quả: THÀNH CÔNG** — `push #`=22, có `[SECURE_LOGIT_DIAG]` tiến triển liên tục
(`n_past` từ 0 → 36), và **có FINAL_ANSWER mạch lạc thật sự**:
```
===FINAL_ANSWER_START===
Sure, my name is John.
===FINAL_ANSWER_END===
```

**=> KẾT LUẬN**: So sánh 5 lượt liên tiếp (mỗi lượt sau reboot sạch):
| Lượt | smc.c | decrypt-stage.cpp | Kết quả |
|---|---|---|---|
| 1, 2 | fix#2 (per-CPU array) | fix (min-heap) | Treo |
| 3 | fix#1 (assign-before-use) | fix (min-heap) | Treo |
| 4 | pristine (git HEAD) | fix (min-heap) | Treo |
| 5 | pristine (git HEAD) | **pristine (git HEAD)** | **Thành công** |

**`smc.c` KHÔNG PHẢI nguyên nhân** (đã loại trừ dứt khoát qua lượt 3+4). Biến duy nhất đổi giữa
lượt 4 (treo) và lượt 5 (thành công) là `decrypt-stage.cpp`'s fix min-heap. **Nghi vấn hàng đầu
hiện tại**: chính fix "min-heap cho `pending_addr[4]`" (tưởng là sửa đúng theo lý thuyết — xem
comment gốc của fix) lại là nguyên nhân gây treo trong thực tế, có thể do thay đổi thứ tự xử lý
làm lộ ra một race condition/thứ tự phụ thuộc khác chưa được hiểu đúng, hoặc lý thuyết ban đầu về
việc "cần min-heap" là sai. **CHỈ MỚI CÓ 1 LƯỢT THÀNH CÔNG** — theo thông lệ đã thiết lập trong dự
án (cần nhiều lượt liên tiếp mới kết luận chắc chắn, vì có bug xác suất ~40-57% lịch sử), cần test
thêm 2-3 lượt nữa với cấu hình pristine này trước khi khẳng định dứt điểm. Nếu xác nhận, bước tiếp
theo là hiểu TẠI SAO min-heap fix gây treo (không chỉ đơn giản revert nó đi mà bỏ qua) trước khi
quyết định có cần một fix khác đúng đắn hơn cho vấn đề `commit_tzasc()` gốc hay không.

### CẬP NHẬT: Lượt 6 (xác nhận lượt 5) — KHÔNG TREO nhưng OUTPUT RÁC, làm rõ lại bức tranh

Reboot sạch, chạy lại bản pristine y hệt lượt 5. **Không treo** (`push #`=22,
`[SECURE_LOGIT_DIAG]` tiến triển đều `n_past` 0→91) — nhưng **FINAL_ANSWER lần này là rác, không
mạch lạc** (`" (,12\n1.2,\n.\n\n-.0. 3.\n..."`) thay vì "Sure, my name is John." như lượt 5.

**Bức tranh đầy đủ sau 6 lượt**:
| Cấu hình | Treo? | Chất lượng output khi không treo |
|---|---|---|
| decrypt-stage.cpp có fix min-heap (lượt 1-4) | **4/4 treo** | (chưa từng chạy xong) |
| decrypt-stage.cpp pristine (lượt 5-6) | **0/2 treo** | 1/2 mạch lạc, 1/2 rác |

**Diễn giải lại**: fix min-heap KHÔNG liên quan tới bug "rác output" gốc (đúng lý thuyết ban đầu
của fix — `commit_tzasc()` với max-heap mặc định có thể bỏ sót/trễ mở rộng biên TZASC = output
rác, bug này CÓ THẬT, vẫn quan sát được ở lượt 6 với code pristine). Nhưng bản thân fix min-heap
lại **gây ra một bug KHÁC, nghiêm trọng hơn: treo hoàn toàn** (100% trong 4 lần test) — cơ chế cụ
thể vì sao đổi thứ tự heap lại gây treo (thay vì chỉ sửa lỗi thứ tự commit) **chưa được hiểu rõ**.
Tạm thời: **giữ nguyên trạng thái pristine (đã revert) là lựa chọn an toàn hơn** — chấp nhận rủi ro
output rác không xác định (đã biết từ trước) thay vì treo 100%. Không nên áp dụng lại fix min-heap
cho tới khi hiểu được cơ chế gây treo của nó.

## Vị trí repo (đổi kể từ 2026-08-06)

Project đã được tách ra khỏi workspace `OPTEE/` lộn xộn (chứa nhiều dự án không liên quan)
thành một git repo độc lập, sạch, sẵn sàng bàn giao tại:

**`/mnt/2d4726e7-046b-47c7-b9a9-d2a9cc0cfc8d/Work/tz-llm-trustzone/`**

Toàn bộ lịch sử git (126k+ file, các commit trước đó) được giữ nguyên qua `mv` — không mất gì.
Xem `CLAUDE.md` (rule/footgun tổng hợp) trước khi bắt đầu bất kỳ build/flash nào.

**`checkpoints/golden-image/`** (backup idbloader-qua-vendor, 3.4GB) **không nằm trong git**
(quá lớn, xem `.gitignore`) — vẫn nằm trên disk cùng thư mục, cần copy thủ công nếu dựng
môi trường mới.

## Cập nhật lần cuối: 2026-08-07 — SỰ CỐ `flash-full.sh` GHI ĐÈ USERDATA + ĐÃ SỬA DEFAULT + KHÔI PHỤC THÀNH CÔNG

**Sự cố (2026-08-07)**: chạy thử `flash-full.sh` (default cũ: ghi cả userdata trừ khi tự set
`SKIP_USERDATA=1`) trên board đang hoạt động tốt để kiểm chứng full-flash dùng đúng bản đã fix
— script ghi `assets/full-flash/userdata.img` (template F2FS trống) đè lên userdata thật đang
chạy tốt (đã có account activated, wifi config...). Sau khi flash xong và power-cycle, board
**im lặng hoàn toàn trên UART** trong nhiều phút và USB liên tục re-enumerate — nghi ngờ
reset-loop do userdata trắng không tương thích với combo system/vendor cụ thể của board.

**Đã khắc phục 2 việc**:
1. **Khôi phục board**: chạy `flash/recover-golden-image.sh` (script mới, tự động hoá đúng quy
   trình đã dùng để cứu board lần này) — ghi lại `checkpoints/golden-image/
   idbloader_through_vendor.img` raw vào LBA 0 (không đụng userdata) rồi tự gọi `flash/flash.sh`
   để đảm bảo uboot/boot_linux vẫn là bản đã fix. Board boot lại bình thường (dù userdata vẫn là
   bản trắng mới từ full-flash — `/data/ssd` cần `mkdir -p` lại trước khi mount, không tồn tại
   sẵn như trước). **Xác nhận lại NPU và CPU đều chạy đúng** ("Sure! My name is Alex." cả 2 path).
2. **Sửa `flash-full.sh` an toàn hơn**: đã **đảo ngược default** — giờ userdata **mặc định được
   bỏ qua** (không ghi đè), phải chủ động đặt `FORCE_USERDATA=1` mới thực sự ghi đè userdata.
   `UBOOT`/`BOOT` args mặc định vẫn đúng trỏ tới `checkpoints/uboot_repacked.img`/`boot.img`
   (bản đã fix) — phần đó không có vấn đề gì, chỉ userdata là nguồn gốc sự cố.

**Nếu gặp lại tình trạng board im lặng/reset-loop sau flash**: chạy ngay
`./flash/recover-golden-image.sh` (cần board ở MaskROM) — không cần tự tay ghép lại quy trình.

## Lịch sử: 2026-08-06 — CẢ NPU (`-s 0`) LẪN CPU (`-s 1`) ĐỀU CHO OUTPUT MẠCH LẠC

**Cả hai bug gốc rễ của "output rác" đã được tìm ra và fix, xác nhận trên phần cứng:**
1. `ggml_backend_rknpure_supports_op()` (`ggml-rknpu-re.cpp`): loại view tensor của KV-cache
   (`view_src != NULL`) khỏi NPU routing — trước đó self-attention bị route nhầm vào NPU.
2. `wrap_user_chat` (`common/arg.cpp`): khôi phục lại `true` (đã bị tắt âm thầm ở một commit
   trước, không giải thích) — khôi phục chat-template cho prompt tự do `-t`.

**Kết quả xác nhận trên phần cứng**: cả `-s 0` và `-s 1` đều trả lời "Sure! My name is Alex."
cho prompt "What is your name?" — lần đầu tiên trong lịch sử dự án có câu trả lời mạch lạc
thật sự từ path NPU/secure. Đã build lại + reflash + benchmark hiệu suất (5/6 lượt thành công,
xem `Bao_Cao_Benchmark_NPU_CPU_TrustZone_20260806.md`): NPU thắng ở prefill (~9.86 vs 8.32
tok/s) và load time; CPU thắng nhẹ ở decode (~1.41 vs 1.22 tok/s, chưa rõ nguyên nhân, không
phải bug). Cả 2 fix đã được commit (`4ce83a425`).

**Bug còn tồn đọng, chưa fix**: path CPU-only (`-s 1`) có bug treo không xác định (kế thừa từ
`STATUS.md`'s "Bug #2" lịch sử, ~43% tỷ lệ) — tái xác nhận vẫn còn trong lượt benchmark thứ 6.
Nghi do race condition ở `std::priority_queue` trong `decrypt-stage.cpp`'s `commit_tzasc()`.

## Lịch sử trước đó (2026-08-05, phiên phục hồi brick + fix NPU)

### Thẻ SD (idbloader + GPT + uboot + boot_linux + system + vendor + userdata)
- **Nguồn**: `checkpoints/golden-image/idbloader_through_vendor.img` (chụp 2026-07-29), khôi phục
  toàn bộ (LBA 0 → hết vendor) sau khi 2 lần thử idbloader khác (`MiniLoaderAll_official.bin`,
  `device_opi5plus_REAL/loader/MiniLoaderAll.bin`) đều gây rơi MaskROM liên tục.
- **idbloader hash** (LBA 0x40, 941 sectors): `093a1bfe09c8b0bbbc45a69a274c63cf72687e2219ed2916920e44acbbd83628`
  — **khác cả 2 file trên**, chưa rõ tên gốc, chỉ tồn tại trong golden-image này.
- **GPT**: scheme đầy đủ của `assets/full-flash/parameter_custom.txt`
  (`uboot@0x2000, boot_linux@0x88000, system@0xba000, vendor@0x4ba000, ...`).
- **uboot_repacked.img đang chạy trên board**: build hôm nay (2026-08-05), optee hash
  `adbfa46bc94e488c67c793515973c3db5f769b40372d92082c99fd9b6b6dc5bc` — chứa:
  - fix OOM-print (`tz-llm/tee_os_kernel/kernel/mm/buddy.c`, comment out `[OOM]` kinfo — vẫn UNCOMMITTED)
  - fix `ggml_backend_rknpure_supports_op` (xóa `return true` sớm, giữ tắt check `buft RKNPURE`
    đã deprecated — `tz-llm/llama.cpp/ggml/src/ggml-rknpu-re.cpp` — vẫn UNCOMMITTED)
- **boot_linux (kernel Linux + ramdisk)**: build hôm nay, cùng lần với uboot ở trên, flash qua
  `flash/flash.sh` (uboot@0x2000 + boot_linux@0x88000).
- **vendor/system**: **VẪN LÀ BẢN 2026-07-29** (từ golden-image) — **CHƯA reflash lại từ build hôm
  nay**. Đây là nghi vấn đang treo (xem mục "Vấn đề đang mở" bên dưới).

### NVMe /data/ssd/rknpu (binary CA — KHÔNG bị ảnh hưởng bởi bất kỳ thao tác flash thẻ SD nào)
- `fake`, `libggml.so`, `libllama.so`, `libremoting_backend.so`, `llama-cli`: build
  **2026-08-03 20:57** (sau "attn_fix", theo `scripts/kick-the-tires/share/bake_attn_fix.log`),
  hash `fake`=`f9b332f838ed498517905e764274a018`, `libggml.so`=`2fbbb0b9588e81cd02c54ad42c8598a6`.
  Khớp 100% với `scripts/kick-the-tires/share/build-rknpure/` trên máy host — **chưa bị mất, chưa
  cần rebuild lại**, trừ khi nghi ngờ ABI lệch với TEE-OS mới (xem bên dưới).

## Vấn đề đang mở (2026-08-05) — ĐÃ VƯỢT QUA (xem bản cập nhật 2026-08-06 ở trên)

Vấn đề dưới đây không còn tái hiện trong phiên 2026-08-06 (cả `-s 0` và `-s 1` chạy xong bình
thường, nhiều lượt liên tiếp) — giữ lại nguyên văn để tham khảo lịch sử, không xóa.
Cả `-s 0` (NPU) lẫn `-s 1` (CPU thuần) đều **treo giống hệt nhau** ngay từ đầu (TA lặp
`SMC_EXIT_PREEMPTED` vô hạn, chưa từng đạt `SMC_EXIT_SHADOW`, chưa từng gọi `push`/`npu_submit`) —
xảy ra **trước cả bước cần I/O relay**, nên **không liên quan tới NPU/fix `supports_op`** (đã
chứng minh bằng thực nghiệm). Dmesg cho thấy `tzasc_cma[3] is NULL, skipping` lúc boot (vùng CMA
dành cho NPU scratch, index `TZASC_NR_NPU_SCRATCH = TZASC_NR-1`, không cấp phát được đủ vùng nhớ
liên tục) — nghi vấn hàng đầu nhưng **chưa xác nhận** đây là nguyên nhân trực tiếp gây treo (vì
`-s 1` treo ngay cả trước khi cần dùng vùng này). Nghi vấn thứ 2 chưa loại trừ: `vendor`/`system`
đang là bản 29/07, có thể lệch phiên bản so với runtime thực sự chạy lúc build CA 03/08 thành công.

**Việc cần làm tiếp**: xác nhận `tzasc_cma[3]` NULL có phải do device-tree/kernel command line
build hôm nay thiếu carve-out vùng nhớ đúng kích thước hay không (so với build 03/08); cân nhắc
reflash `vendor.img` build hôm nay để loại trừ lệch phiên bản.

## Cách flash lại đúng, nhất quán (đọc trước khi tự chạy tay từng bước)
1. Sửa source trong `tz-llm/` như bình thường.
2. `./rebuild.sh` (kernel + TEE-OS, ~10-15 phút) rồi `./flash/repack.sh` (đóng gói
   `checkpoints/uboot_repacked.img`).
3. Nếu có sửa code phía CA (`tz-llm/llama.cpp/src/*, examples/main/*`), PHẢI build lại binary CA
   cùng lúc: `./scripts/kick-the-tires/build-llama.sh` — đừng chỉ rebuild TEE-OS một mình rồi tin
   binary CA cũ vẫn khớp `interface.h` mới.
4. Board vào MaskROM → `SUDO_PW=... ./flash/flash.sh` (nhanh, chỉ uboot+boot_linux — dùng khi GPT/
   idbloader/vendor/system không đổi) hoặc `./flash/flash-full.sh` (chậm hơn nhiều, ghi lại toàn
   bộ GPT+idbloader+system+vendor+userdata — chỉ dùng khi thật sự cần, và nhớ dùng
   `assets/full-flash/MiniLoaderAll_official.bin` — **file này đã 2 lần gây rơi MaskROM liên tục
   trên board vật lý hiện tại, cân nhắc kỹ trước khi dùng lại**).
5. Cập nhật lại chính file này (`DEPLOYED_STATE.md`) ngay sau khi xác nhận boot ổn định.
