# Báo cáo Benchmark NPU vs CPU trong TrustZone — TZ-LLM trên Orange Pi 5 Max

**Ngày:** 2026-08-07
**Model:** TinyLlama-1.1B-Chat (Q8_0)
**Prompt:** `"What is your name?"`
**Cấu hình:** `fake -c 0 -m tinyllama -n 64 -s {0|1} -t "What is your name?"`
(`-s 0` = NPU thật trong TrustZone, `-s 1` = CPU-only/strawman trong TrustZone)

Đo hiệu suất NPU (`-s 0`) và CPU-only (`-s 1`) trong TrustZone, 3 lượt mỗi
path, dùng bản binary/firmware đã commit (`4ce83a425`) tại
`/mnt/2d4726e7-046b-47c7-b9a9-d2a9cc0cfc8d/Work/tz-llm-trustzone/`.

## 1. Giải thích các trường trong bảng

- **Lượt** (`s0_run1`...): tên định danh mỗi lần chạy — `s0` = path NPU (`-s 0`), `s1` = path CPU-only (`-s 1`), số cuối là lần lặp thứ mấy (1-3) trong path đó.
- **Path**: **NPU** = suy luận qua Rockchip NPU thật bên trong TrustZone secure world; **CPU** = suy luận hoàn toàn bằng CPU (strawman/baseline), cũng chạy trong secure world, không dùng NPU.
- **Load (ms)**: thời gian nạp model từ đầu (đọc file GGUF, giải mã, cấp phát bộ nhớ TZASC, dựng cấu trúc weight) tới khi model sẵn sàng suy luận — chưa tính thời gian xử lý prompt hay sinh câu trả lời.
- **Prompt eval (ms / tokens / tok/s)**: giai đoạn "prefill" — model đọc và xử lý toàn bộ câu hỏi đầu vào (ở đây là 27 token, gồm cả token đặc biệt của chat-template) trong một lượt tính toán song song. `ms` = tổng thời gian; `tokens` = số token đầu vào; `tok/s` = tốc độ xử lý (số token đầu vào chia cho thời gian) — chỉ số này càng cao càng tốt, phản ánh tốc độ "đọc hiểu" câu hỏi.
- **Eval/Decode (ms / runs / tok/s)**: giai đoạn sinh câu trả lời — model tạo từng token một, mỗi token là một lượt tính toán riêng (khác prefill, không song song hoá được vì token sau phụ thuộc token trước). `ms` = tổng thời gian sinh; `runs` = số token đã sinh ra (ở đây đều là 8, do model dừng sớm khi gặp token kết thúc câu — EOS); `tok/s` = tốc độ sinh token (số token sinh ra chia cho thời gian) — chỉ số quan trọng nhất cho trải nghiệm "chờ trả lời", càng cao càng tốt.
- **Câu trả lời**: nội dung thực tế model sinh ra cho prompt "What is your name?", lấy trực tiếp từ kênh kết quả trên board (không qua UART nên không bị nhiễu/lỗi ký tự).

## 2. Kết quả — 6/6 lượt hoàn tất thành công

| Lượt | Path | Load (ms) | Prompt eval (ms / tokens / tok/s) | Eval/Decode (ms / runs / tok/s) | Câu trả lời |
|---|---|---|---|---|---|
| s0_run1 | **NPU** | 2948.15 | 2734.52 / 27 / **9.87** | 6599.85 / 8 / **1.21** | "Sure! My name is Alex." |
| s0_run2 | **NPU** | 2959.84 | 2739.71 / 27 / **9.86** | 6587.33 / 8 / **1.21** | "Sure! My name is Alex." |
| s0_run3 | **NPU** | 2982.74 | 2752.64 / 27 / **9.81** | 6560.62 / 8 / **1.22** | "Sure! My name is Alex." |
| s1_run1 | **CPU** | 3440.32 | 3239.50 / 27 / **8.33** | 5694.78 / 8 / **1.40** | "Hi there! My name is Alex." |
| s1_run2 | **CPU** | 3461.99 | 3244.62 / 27 / **8.32** | 5696.84 / 8 / **1.40** | "Sure! My name is Alex." |
| s1_run3 | **CPU** | 3441.60 | 3233.44 / 27 / **8.35** | 5702.92 / 8 / **1.40** | "Sure! My name is John." |

### Trung bình (lần 2)

| Path | Load trung bình (ms) | Prompt eval trung bình (tok/s) | Eval/Decode trung bình (tok/s) |
|---|---|---|---|
| **NPU** (3 lượt) | 2963.58 | 9.85 | 1.21 |
| **CPU** (3 lượt) | 3447.97 | 8.33 | 1.40 |

## 3. Nhận xét

- **Tính đúng đắn (correctness)**: cả NPU và CPU đều cho ra câu trả lời mạch lạc, đúng ngữ nghĩa ("Sure!/Hi there!... My name is Alex/John.") ở tất cả 6 lượt.
- **Độ ổn định (tok/s)**: cả hai path đều rất nhất quán giữa các lượt lặp lại — NPU dao động 1.21-1.22 tok/s (eval), CPU dao động đúng 1.40 tok/s ở cả 3 lượt.
- **Hiệu suất decode (eval tok/s)**: CPU-only (~1.40 tok/s) nhanh hơn NPU (~1.21 tok/s) khoảng 15-16% cho model TinyLlama-1.1B ở benchmark này. Nhiều khả năng do model/prompt quá nhỏ (chỉ 8 decode step thực tế) nên overhead giao tiếp CA↔TA↔NPU qua SMC/TZASC per-token lấn át lợi ích tính toán song song của NPU.
- **Prompt eval (prefill) tok/s**: NPU nhanh hơn CPU rõ rệt (~9.85 vs ~8.33 tok/s, ~+18%) — đúng như kỳ vọng, giai đoạn prefill xử lý nhiều token cùng lúc (27 token), tận dụng tốt hơn khả năng tính toán song song của NPU.
- **Load time**: NPU nạp nhanh hơn CPU (~2964ms vs ~3448ms, ~14%) — hợp lý vì NPU dùng trọng số quantize nhẹ hơn khi restore.
- **Wifi reconnect sau reboot dao động rất lớn** giữa các lượt (từ gần như tức thời tới hơn 3 tiếng ở một lượt) — biến động môi trường mạng, không liên quan tới hiệu suất NPU/CPU đang đo.

## 4. Kết luận

Cả NPU và CPU trong TrustZone đều hoạt động đúng, ổn định, cho output mạch
lạc trên toàn bộ 6/6 lượt. NPU có lợi thế rõ ràng ở giai đoạn prefill (~18%
nhanh hơn) và load time (~14% nhanh hơn), trong khi CPU nhỉnh hơn một chút
ở giai đoạn decode (~15-16%) cho model nhỏ/prompt ngắn này.
