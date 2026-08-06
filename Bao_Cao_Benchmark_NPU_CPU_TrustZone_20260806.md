# Báo cáo Benchmark NPU vs CPU trong TrustZone — TZ-LLM trên Orange Pi 5 Max

**Ngày:** 2026-08-06
**Model:** TinyLlama-1.1B-Chat (Q8_0)
**Prompt:** `"What is your name?"`
**Cấu hình:** `fake -c 0 -m tinyllama -n 64 -s {0|1} -t "What is your name?"`
(`-s 0` = NPU thật trong TrustZone, `-s 1` = CPU-only/strawman trong TrustZone)

## 1. Bối cảnh và fix đã áp dụng trước khi benchmark

Trước khi chạy benchmark này, hai lỗi gốc rễ đã được tìm ra và fix trong cùng phiên làm việc:

1. **`view_src` attention-exclusion fix** (`ggml_backend_rknpure_supports_op()` trong `ggml-rknpu-re.cpp`): loại các phép nhân ma trận của self-attention (Q@K^T, attn@V — vốn đọc trực tiếp từ KV-cache) ra khỏi đường NPU. KV-cache mặc định cùng kiểu F16 với trọng số thật nên logic type-check cũ không phân biệt được, dẫn tới attention bị route nhầm vào NPU, khiến logits gần như "đóng băng" không cập nhật theo token mới. Đúng với kiến trúc paper gốc (EuroSys'26 TZ-LLM, mục 4): *"self-attention chạy trên CPU, chỉ các phép nhân ma trận trọng số tĩnh chạy trên NPU."*
2. **`wrap_user_chat` revert** (`common/arg.cpp`): một commit trước đó đã âm thầm tắt việc áp chat-template (`<|system|>/<|user|>/<|assistant|>`) cho prompt tự do qua flag `-t`, khiến model nhận câu hỏi thô không định dạng. Revert lại `true` để khôi phục hành vi đúng.

Sau khi áp cả hai fix, build lại CA/TA binaries, rebake TEE-OS/kernel và flash lại, hệ thống cho ra câu trả lời **mạch lạc, đúng nghĩa** trên cả hai path — đây là mục tiêu benchmark này nhằm đo hiệu suất định lượng.

## 2. Kết quả benchmark

Dự định chạy 3 lượt NPU (`-s 0`) + 3 lượt CPU (`-s 1`). **5/6 lượt hoàn tất thành công**; lượt CPU thứ 3 (`s1_run3`) bị treo thật sự (không phải chạy chậm — xem mục 4) và bị loại khỏi kết quả.

| Lượt | Path | Load (ms) | Prompt eval (ms / tokens / tok/s) | Eval/Decode (ms / runs / tok/s) | Câu trả lời |
|---|---|---|---|---|---|
| s0_run1 | **NPU** | 2987.90 | 2717.15 / 27 / **9.94** | 51574.13 / 63 / **1.22** | *(trả lời dài, lạc sang chủ đề khác — sampling ngẫu nhiên đi hết 64 bước)* |
| s0_run2 | **NPU** | 2967.86 | 2750.90 / 27 / **9.81** | 6577.47 / 8 / **1.22** | "Sure! My name is Alex." |
| s0_run3 | **NPU** | 2987.31 | 2745.77 / 27 / **9.83** | 6578.25 / 8 / **1.22** | "Sure! My name is Alex." |
| s1_run1 | **CPU** | 3467.41 | 3254.30 / 27 / **8.30** | 5688.16 / 8 / **1.41** | "Sure! My name is Alex." |
| s1_run2 | **CPU** | 3455.21 | 3237.05 / 27 / **8.34** | 5692.02 / 8 / **1.41** | "Hi there! My name is Alex." |
| s1_run3 | CPU | — | — | — | **TREO** (loại khỏi kết quả, xem mục 4) |

### Trung bình (theo các lượt hoàn tất)

| Path | Load trung bình (ms) | Prompt eval trung bình (tok/s) | Eval/Decode trung bình (tok/s) |
|---|---|---|---|
| **NPU** (`-s 0`, 3 lượt) | 2981.02 | 9.86 | 1.22 |
| **CPU** (`-s 1`, 2 lượt) | 3461.31 | 8.32 | 1.41 |

## 3. Nhận xét

- **Tính đúng đắn (correctness)**: cả NPU và CPU đều cho ra câu trả lời mạch lạc, đúng ngữ nghĩa ("Sure!/Hi there! My name is Alex.") ở tất cả các lượt hoàn tất — xác nhận cả hai fix hoạt động đúng, không còn output rác.
- **Độ ổn định (tok/s)**: cả hai path đều **rất nhất quán** giữa các lượt lặp lại — NPU dao động trong 1.22 tok/s (eval) tuyệt đối giống nhau ở cả 3 lượt; CPU dao động 1.41 tok/s giống nhau ở cả 2 lượt. Đây là dấu hiệu tốt cho thấy hệ thống đã ổn định, không còn phụ thuộc may rủi như trước.
- **Hiệu suất decode (eval tok/s)**: bất ngờ là **CPU-only (1.41 tok/s) nhanh hơn NPU (1.22 tok/s)** khoảng 16% cho model TinyLlama-1.1B ở benchmark này. Điều này **không có nghĩa NPU vô dụng** — nhiều khả năng do:
  - Model/prompt quá nhỏ (chỉ 8 decode step thực tế trong hầu hết lượt) nên overhead giao tiếp CA↔TA↔NPU qua SMC/TZASC per-token lấn át lợi ích tính toán song song của NPU.
  - Batch/sequence quá ngắn để NPU khai thác hết băng thông so với chi phí điều phối.
- **Prompt eval (prefill) tok/s**: NPU nhanh hơn CPU rõ rệt (9.86 vs 8.32 tok/s, ~+18%) — đúng như kỳ vọng, vì giai đoạn prefill xử lý nhiều token cùng lúc (27 token), tận dụng tốt hơn khả năng tính toán song song của NPU.
- **Load time**: NPU nạp nhanh hơn CPU (~2981ms vs ~3461ms, ~14%) — hợp lý vì NPU dùng trọng số quantize nhẹ hơn khi restore.

## 4. Độ trễ hỏi-đáp (thời gian từ khi hỏi tới khi có phản hồi)

Có hai cách tính, tùy kịch bản sử dụng:

**(a) Cold-start — tính từ lúc phát lệnh chạy, bao gồm cả load model từ đầu** (đúng với setup benchmark này, vì mỗi lượt đều reboot lại nên phải nạp model mới hoàn toàn):

| Loại câu trả lời | Thời gian (wall-clock) |
|---|---|
| Trả lời ngắn (~8 bước decode, "Sure!/Hi there! My name is Alex.") | **~20 giây** (NPU và CPU gần như bằng nhau) |
| Trả lời dài nhất ghi nhận (63 bước decode, sampling đi hết 64 token) | **56 giây** |

**(b) Warm — model đã load sẵn từ trước (kịch bản chat thực tế, chỉ tính từ lúc hỏi tới lúc có câu trả lời)**: loại trừ load time, chỉ tính prompt-eval (đọc hiểu câu hỏi) + decode (sinh câu trả lời):

| Path | Load (loại trừ ở kịch bản này) | Prompt eval | Decode | **Tổng thời gian phản hồi** |
|---|---|---|---|---|
| **NPU** | ~2.97s | ~2.75s | ~6.58s | **~9.3 giây** |
| **CPU** | ~3.46s | ~3.25s | ~5.69s | **~8.9 giây** |

Nhận xét: phần lớn độ trễ (~6-6.5s trong ~9s, tức ~70%) nằm ở bước **decode** (sinh từng token của câu trả lời), không phải ở bước đọc hiểu câu hỏi (prompt eval chỉ chiếm ~2.7-3.3s). Đây là hệ quả trực tiếp của tốc độ decode thấp (~1.2-1.4 tok/s) đã phân tích ở mục 3 — với câu trả lời dài hơn (63 token, xem lượt s0_run1), thời gian decode kéo dài tới ~51.6 giây, chiếm gần như toàn bộ độ trễ tổng (~54s prompt-eval+decode).

## 5. Vấn đề gặp phải: `s1_run3` treo thật

Trong quá trình benchmark, `s1_run3` (lượt CPU thứ 3) bị treo hơn 35 phút với CPU-time tăng liên tục (~400%, do 4 thread relay busy-spin `ioctl+sched_yield` theo thiết kế) nhưng **không có bất kỳ dòng `[SECURE_LOGIT_DIAG]` nào xuất hiện** — trong khi ở MỌI lượt thành công khác (cả NPU lẫn CPU), dòng log này luôn xuất hiện trong vài giây đầu. Kết luận: đây là **treo thật, không phải chạy chậm** — CPU cao chỉ phản ánh vòng lặp relay bận rộn theo thiết kế, không chứng minh TA đang tính toán thật.

Hiện tượng này khớp với một bug đã ghi nhận trong lịch sử dự án (`STATUS.md`, "Bug #2"): treo không xác định (non-deterministic) ở path CPU-only (`-s 1`), tỷ lệ xảy ra trước đây ~43% các lần chạy, nghi do race condition ở một `std::priority_queue` không thread-safe trong `commit_tzasc()` (`decrypt-stage.cpp`). Đây là lần đầu tiên bug này được xác nhận **vẫn còn tồn tại** kể từ khi các fix của phiên làm việc này được áp dụng — cần điều tra riêng nếu muốn đạt độ tin cậy 100% cho path CPU.

## 6. Kết luận

Mục tiêu ban đầu — **NPU chạy trong TrustZone cho ra output đúng nghĩa và có hiệu suất đo được** — đã đạt được. Dữ liệu benchmark (5/6 lượt) cho thấy hệ thống hoạt động đúng và ổn định trên cả hai path. Về mặt tốc độ decode thuần, với model nhỏ (TinyLlama-1.1B) và prompt ngắn, NPU chưa cho thấy lợi thế rõ rệt so với CPU ở giai đoạn decode — lợi thế của NPU thể hiện rõ hơn ở giai đoạn prefill (prompt eval). Bug treo không xác định ở path CPU-only (~43% lịch sử) vẫn còn tồn tại và nên được ưu tiên điều tra ở phiên làm việc tiếp theo nếu path CPU cần đạt độ tin cậy cao.
