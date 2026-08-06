# Trạng thái đang thực sự chạy trên board (cập nhật thủ công sau mỗi lần flash)

**Đây là nguồn sự thật duy nhất cho câu hỏi "cái gì đang chạy trên board ngay bây giờ".**
Đọc file này trước khi flash bất cứ thứ gì — đừng suy đoán từ timestamp/tên file.

## Vị trí repo (đổi kể từ 2026-08-06)

Project đã được tách ra khỏi workspace `OPTEE/` lộn xộn (chứa nhiều dự án không liên quan)
thành một git repo độc lập, sạch, sẵn sàng bàn giao tại:

**`/mnt/2d4726e7-046b-47c7-b9a9-d2a9cc0cfc8d/Work/tz-llm-trustzone/`**

Toàn bộ lịch sử git (126k+ file, các commit trước đó) được giữ nguyên qua `mv` — không mất gì.
Xem `CLAUDE.md` (rule/footgun tổng hợp) trước khi bắt đầu bất kỳ build/flash nào.

**`checkpoints/golden-image/`** (backup idbloader-qua-vendor, 3.4GB) **không nằm trong git**
(quá lớn, xem `.gitignore`) — vẫn nằm trên disk cùng thư mục, cần copy thủ công nếu dựng
môi trường mới.

## Cập nhật lần cuối: 2026-08-06 — CẢ NPU (`-s 0`) LẪN CPU (`-s 1`) ĐỀU CHO OUTPUT MẠCH LẠC

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
