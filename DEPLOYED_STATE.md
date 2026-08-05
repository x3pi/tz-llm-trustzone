# Trạng thái đang thực sự chạy trên board (cập nhật thủ công sau mỗi lần flash)

**Đây là nguồn sự thật duy nhất cho câu hỏi "cái gì đang chạy trên board ngay bây giờ".**
Đọc file này trước khi flash bất cứ thứ gì — đừng suy đoán từ timestamp/tên file.

## Cập nhật lần cuối: 2026-08-05 (phiên phục hồi brick + fix NPU)

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

## Vấn đề đang mở (2026-08-05)
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
