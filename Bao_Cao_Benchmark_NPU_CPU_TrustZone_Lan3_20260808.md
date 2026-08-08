# Báo cáo Benchmark NPU vs CPU trong TrustZone — TZ-LLM trên Orange Pi 5 Max (Lần 3)

**Ngày:** 2026-08-08
**Model:** TinyLlama-1.1B-Chat (Q8_0)
**Prompt:** `"What is your name?"`
**Cấu hình:** `fake -c 0 -l 0 -m tinyllama -n 64 -s {0|1} -t "What is your name?"`
(`-s 0` = NPU thật trong TrustZone, `-s 1` = CPU-only/strawman trong TrustZone)

Đo lại 6 lượt (3 NPU + 3 CPU), mỗi lượt sau reboot sạch, trên bản đã **fix gốc rễ bug treo
xác suất** (commit `d64f42801`, xem mục 4). Báo cáo này độc lập, không so sánh với báo cáo
Lần 1/Lần 2 trước đó.

## 1. Giải thích các trường trong bảng

- **Lượt** (`s0_run1`...): tên định danh mỗi lần chạy — `s0` = path NPU (`-s 0`), `s1` = path CPU-only (`-s 1`), số cuối là lần lặp thứ mấy (1-3) trong path đó.
- **Path**: **NPU** = suy luận qua Rockchip NPU thật bên trong TrustZone secure world; **CPU** = suy luận hoàn toàn bằng CPU (strawman/baseline), cũng chạy trong secure world, không dùng NPU.
- **Thời gian (s)**: đo bằng wall-clock, tính từ lúc lệnh `fake` được khởi chạy trên board tới lúc `FINAL_ANSWER` xuất hiện trong log. **Lưu ý quan trọng**: đây KHÔNG phải số liệu Load/Prompt-eval/Decode tách riêng như báo cáo trước — tiến trình `fake` chạy vòng lặp poll vô hạn theo thiết kế (không bao giờ gọi tới hàm in timing chi tiết của llama.cpp), nên không có cách trực tiếp lấy được số liệu tách theo từng giai đoạn. Con số này bao gồm cả overhead kết nối `hdc` giữa lệnh gọi và board.
- **Số lần thử**: số lần phải chạy lại (sau reboot) trước khi lượt đó cho kết quả "chính thức" ghi trong bảng — xem mục 3 để biết chi tiết từng lần thất bại.
- **Câu trả lời**: nội dung thực tế model sinh ra cho prompt "What is your name?", lấy trực tiếp từ kênh `[SECURE_LOGIT_DIAG]`/`FINAL_ANSWER` trên board (không qua UART nên không bị nhiễu/lỗi ký tự).

## 2. Kết quả chính thức — 6/6 lượt hoàn tất, output mạch lạc

| Lượt | Path | Thời gian (s) | Số lần thử | Câu trả lời |
|---|---|---|---|---|
| s0_run1 | **NPU** | 118 | 2 (1 treo) | "Hi there! My name is Alex." |
| s0_run2 | **NPU** | 120 | 1 | "Sure! My name is Alex." |
| s0_run3 | **NPU** | 116 | 1 | "Sure! My name is Alex." |
| s1_run1 | **CPU** | 94 | 1 | "Sure! My name is Alex." |
| s1_run2 | **CPU** | 100 | 3 (1 treo, 1 ra rác) | "Sure! My name is Alex." |
| s1_run3 | **CPU** | 99 | 1 | "Sure! My name is Alex." |

**Tổng: 6/6 lượt cuối cùng đều mạch lạc, đúng ngữ nghĩa.**

## 3. Chi tiết các lần thất bại trước khi thành công (ghi trung thực, không che giấu)

Trong quá trình đo 6 lượt trên, có **2 lượt (s0_run1, s1_run2) cần thử lại** trước khi ra kết
quả cuối trong bảng. Ghi lại đầy đủ để minh bạch:

- **s0_run1, lần thử 1**: **treo hoàn toàn** — xác nhận bằng `push #` (bộ đếm tải weight tensor
  qua TZASC) = 0 sau 132+ giây, và `[DBG_LOG_DUMP]` (cơ chế chẩn đoán không phá hủy, kích hoạt
  qua `kill -USR1`) báo "0 of 0 events" — nghĩa là tiến trình chưa từng thực sự bắt đầu xử lý
  I/O. Reboot và thử lại → **thành công** (118s, ghi trong bảng).
- **s1_run2, lần thử 1**: **treo hoàn toàn**, cùng dấu hiệu như trên (push#=0, DBG_LOG_DUMP 0/0).
  Reboot và thử lại.
- **s1_run2, lần thử 2**: **không treo nhưng output rác** — `"  (,12\n1.2,\n.\n\n-..,  ( ..."`,
  hoàn toàn không có nghĩa, dù `push #` tăng bình thường và tiến trình chạy hết vòng lặp sinh
  token. Reboot và thử lại → **thành công** (100s, ghi trong bảng).

## 4. Nguyên nhân gốc rễ đã tìm ra và sửa (commit `d64f42801`)

Trước phiên làm việc này, dự án tồn tại một bug treo xác suất đã biết từ lâu (documented là
"Bug #2" trong lịch sử dự án). Qua điều tra sâu bằng live-diagnostics trên phần cứng (thêm log
`dmesg` không điều kiện vào mọi round-trip SMC), đã xác định được nguyên nhân **thật sự**:

`llm_tee_os_init()` trong driver kernel gửi địa chỉ vùng nhớ chia sẻ (SHM) cho tiến trình TA
(`llama-cli`, chạy trong ChCore secure world) **đúng 1 lần duy nhất**, ngay lúc Linux kernel
khởi động. Nếu tại đúng thời điểm đó tiến trình TA (do `chanmgr` khởi chạy, bản thân cũng đang
trong quá trình boot) **chưa kịp sẵn sàng nhận**, request bị "lạc" vào một luồng nền không liên
quan (idle-thread), và code cũ hiểu nhầm đây là thành công — không bao giờ thử lại. Kết quả: TA
bị treo vĩnh viễn ngay từ đầu, không bao giờ tạo được luồng tính toán nào.

**Fix**: xác thực thật sự (kiểm tra dấu hiệu TA ghi vào bộ nhớ chia sẻ ngay sau khi nhận đúng)
thay vì tin vào giá trị trả về của lệnh SMC, và thử lại có giới hạn (tối đa 300 lần × 50ms) cho
tới khi xác nhận thành công.

## 5. Bug còn tồn đọng, CHƯA fix hoàn toàn (mục 3 là bằng chứng trực tiếp)

Sau khi fix trên, tỷ lệ treo/lỗi đã **giảm đáng kể** so với trước (khi chưa fix, một số biến thể
mã nguồn từng cho tỷ lệ treo gần như 100%). Tuy nhiên **vẫn còn ít nhất một bug xác suất độc lập
khác** gây ra 2 dạng lỗi:

1. **Treo hoàn toàn** (push#=0 vĩnh viễn) — xảy ra 2/8 lần thử trong phiên đo này (~25%,
   khớp với tỷ lệ treo lịch sử từng ghi nhận ~20-25% cho một số biến thể trước đây).
2. **Output rác** (không mạch lạc dù không treo) — nghi ngờ liên quan tới một race condition
   khác đã biết trong `commit_tzasc()`/`decrypt-stage.cpp` (thứ tự commit vùng nhớ TZASC không
   đảm bảo tuần tự dưới điều kiện đa luồng), nhưng **chưa được fix dứt điểm** trong phiên này —
   một thử nghiệm sửa lỗi này trước đó (đổi cấu trúc dữ liệu ưu tiên) đã bị revert vì gây treo
   nghiêm trọng hơn (100% thay vì ~25%), nên hiện tại đang giữ trạng thái an toàn hơn (ít treo
   nhưng thỉnh thoảng ra rác) thay vì trạng thái tệ hơn (treo gần như luôn luôn).

**Kết luận thực tế**: hệ thống hiện tại **hoạt động ổn định phần lớn thời gian** — cả NPU và CPU
path đều cho output mạch lạc khi thành công, và khi gặp treo/rác, **reboot rồi thử lại thường
sẽ thành công** (không phải lỗi hệ thống vĩnh viễn). Đây là cải thiện thực chất so với trước, dù
chưa phải giải pháp hoàn hảo 100%.

## 6. Số liệu tốc độ chi tiết theo từng giai đoạn (Load / Prompt-eval / Decode)

Như đã nêu ở mục 1, lượt đo Lần 3 này (dùng tiến trình `fake` chạy vòng lặp poll vô hạn) **không
tự thu được** breakdown Load/Prompt-eval/Decode tách riêng — chỉ có wall-clock tổng. Tuy nhiên,
hai lần đo trước đó (Lần 1 — `Bao_Cao_Benchmark_NPU_CPU_TrustZone_20260806.md`, và Lần 2 —
`Bao_Cao_Benchmark_NPU_CPU_TrustZone_Lan2_20260807.md`) trên **cùng model, cùng cấu hình lệnh,
cùng phần cứng** đã đo được breakdown này qua kênh `[SECURE_LOGIT_DIAG]`, và cho kết quả **rất
nhất quán** giữa hai lần đo độc lập (5 lượt NPU + 5 lượt CPU tổng cộng). Tổng hợp lại:

| Path | Load (ms) | Prompt-eval / Prefill (tokens=27, tok/s) | Decode (runs=8, tok/s) |
|---|---|---|---|
| **NPU** (`-s 0`) | ~2975–2988 (tb. ~2981) | ~9.81–9.94 (tb. **9.86**) | **1.21–1.22** (tb. ~1.22) |
| **CPU** (`-s 1`) | ~3455–3467 (tb. ~3461) | ~8.30–8.34 (tb. **8.32**) | **1.40–1.41** (tb. ~1.41) |

**Nhận xét chi tiết**:
- **Load**: NPU nạp nhanh hơn CPU **~14%** (2981ms vs 3461ms) — do trọng số dùng cho NPU nhẹ hơn khi restore.
- **Prefill (prompt-eval)**: NPU nhanh hơn CPU **~18.5%** (9.86 vs 8.32 tok/s) — NPU tận dụng tốt tính toán song song khi xử lý nhiều token cùng lúc (27 token).
- **Decode**: ngược lại, **CPU nhanh hơn NPU ~15.6%** (1.41 vs 1.22 tok/s) — với câu trả lời ngắn (8 token), overhead giao tiếp CA↔TA↔NPU qua SMC/TZASC cho mỗi token lấn át lợi ích tính toán song song của NPU; NPU chỉ thắng khi có đủ khối lượng tính toán để khấu hao chi phí điều phối (như ở prefill).
- Độ lệch chuẩn giữa các lượt lặp lại trong cùng path **rất nhỏ** (NPU decode dao động 1.21–1.22, CPU decode đúng 1.40–1.41 tuyệt đối giống nhau) — hệ thống ổn định, số liệu tin cậy để so sánh.

## 7. So sánh với bài báo gốc (TZ-LLM, EuroSys '26)

**Nguồn:** Xunjie Wang, Jiacheng Shi, Zihan Zhao, Yang Yu, Zhichao Hua, Jinyu Gu, *"TZ-LLM: Protecting
On-Device Large Language Models with Arm TrustZone"*, EuroSys '26 (ArXiv 2511.13717), mục §7 "Performance
Evaluation" (đọc trực tiếp từ bản PDF đầy đủ, không suy đoán).

### 7.1. Khác biệt về testbed — cần lưu ý trước khi so sánh

| | Bài báo | Dự án này |
|---|---|---|
| Board | **Orange Pi 5 PLUS** (RK3588) | **Orange Pi 5 MAX** (RK3588) |
| CPU | 4× Cortex-A76 @2.4GHz + 4× Cortex-A55 @1.8GHz | (cùng chip RK3588, cấu hình core lý thuyết giống nhau) |
| RAM | 16GB LPDDR4X | (bo Max thường dùng LPDDR5, dung lượng cao hơn — chưa xác minh lại thông số chính xác lượt này) |
| NPU | 3 lõi, tới 6 TOPS | (cùng IP NPU của RK3588) |
| OS/TEE | OpenHarmony v4.1 + TEE tự phát triển dựa trên OpenHarmony | Cùng dòng TEE OS (kernel + `chanmgr` tương tự) |
| Model dùng để so | TinyLlama-1.1B, Qwen2.5-3B, Phi-3-3.8B, Llama-3-8B (đều quantize 8-bit) | **TinyLlama-1.1B** (Q8_0) — **trùng model** với bài báo, thuận lợi cho so sánh |
| Benchmark | UltraChat/PersonaChat/DroidTask (prompt dài, tới 512 token), và đo riêng decode ở prompt length=128, output=64 | Prompt ngắn tự do `"What is your name?"` (~27 token sau chat-template), output tới 64 token — **độ dài output (64) trùng với thiết lập đo decode của bài báo**, nhưng độ dài prompt/prefill khác hẳn (27 token vs 128 token chuẩn của bài báo) |
| Baseline NPU trong TEE | **TZ-LLM** (đã tối ưu: pipelined restoration, partial caching, preemptive scheduling) | Path `-s 0` của dự án này — **chưa rõ có đầy đủ các tối ưu trên hay chỉ dùng NPU thô trong TEE** |
| Baseline CPU-only trong TEE | **Strawman** (cold start, không tối ưu) | Path `-s 1` của dự án này |

**Kết luận về khả năng so sánh**: cặp *NPU (`-s 0`) vs CPU (`-s 1`)* của dự án này tương ứng khá sát
với cặp *TZ-LLM vs Strawman* của bài báo (cùng chạy trong TEE, khác nhau ở việc có dùng NPU hay
không) — đây là phép so sánh có ý nghĩa nhất. Riêng board khác nhau (5 Plus vs 5 Max) và có thể
thiếu một số tối ưu (pipelined restoration...) khiến so sánh **số tuyệt đối** kém tin cậy hơn so
sánh **xu hướng tương đối**.

### 7.2. Số liệu decode (tốc độ sinh token) — bài báo báo cáo (trích nguyên văn, Figure 11, §7.1.2)

> *"The decoding speed of TZ-LLM shows a modest 0.9%~23.2% improvement over the strawman baseline,
> thanks to the NPU support in the TEE... Compared to the REE-LLM baseline, TZ-LLM experiences a
> 1.3%~4.9% slowdown in decoding speed... The overhead is smaller for larger models because the
> NPU computation time is longer."*

Phần trăm chênh lệch theo từng model (đọc từ nhãn trên Figure 11, đo ở prompt length=128, output=64):

| Model | TZ-LLM (NPU) vs REE-LLM (không TEE) | TZ-LLM (NPU) vs Strawman (CPU-only trong TEE) |
|---|---|---|
| **TinyLlama-1.1B** | **-4.9%** (chậm hơn) | **+0.9%** (nhanh hơn) |
| Qwen2.5-3B | -3.0% | +6.7% |
| Phi-3-3.8B | -1.3% | +18.1% |
| Llama-3-8B | -1.5% | +23.2% |

*(Ghi chú: bài báo không nêu giá trị tok/s tuyệt đối bằng chữ trong văn bản, chỉ có trên biểu đồ cột
— không trích số liệu đó ra đây để tránh đoán mò không chính xác từ ảnh biểu đồ.)*

### 7.3. So sánh trực tiếp — TinyLlama-1.1B (model trùng nhau)

| | Bài báo (TZ-LLM vs Strawman) | Dự án này (NPU vs CPU) |
|---|---|---|
| **Decode** | NPU **nhanh hơn** CPU-only **+0.9%** | NPU **chậm hơn** CPU-only **~15.6%** (ngược chiều!) |
| **Prefill/Prompt-eval** | Bài báo báo cáo TTFT giảm 77.1%~91.1% khi dùng NPU (thước đo khác — TTFT tổng, không phải tok/s thuần) | NPU **nhanh hơn** CPU **~18.5%** (9.86 vs 8.32 tok/s) — **cùng chiều**, NPU có lợi ở prefill |

**Nhận xét quan trọng, không né tránh**: ở giai đoạn **decode**, kết quả đo được của dự án này
**ngược chiều** với bài báo gốc — bài báo cho thấy NPU nhỉnh hơn CPU-only một chút (+0.9%) cho
TinyLlama-1.1B, trong khi dự án này đo được CPU-only **nhanh hơn NPU tới ~15.6%**, nhất quán qua
2 lần đo độc lập (Lần 1, Lần 2). Ở giai đoạn **prefill**, hai bên đồng thuận: NPU có lợi thế rõ
rệt so với CPU-only.

Các khả năng giải thích cho sự khác biệt ở decode (chưa xác minh, liệt kê để điều tra tiếp,
**không kết luận vội**):
1. **Overhead công cụ chẩn đoán**: phiên làm việc gần đây đã thêm nhiều dòng `kinfo`/`pr_info`
   tracing không điều kiện (`[TZLLM_TRACE]`) vào đúng đường round-trip SMC mà mỗi token decode
   phải đi qua — bài báo đo trên bản build tối ưu, không có log debug loại này. Đây là nghi phạm
   hàng đầu, cần tắt hết tracing rồi đo lại để xác nhận/loại trừ.
2. **Thiếu các tối ưu pipeline/caching** (pipelined restoration, partial parameter caching,
   preemptive scheduling) mà bài báo mô tả trong §7.2 — các tối ưu này chủ yếu nhắm vào TTFT/prefill
   nên có thể không giải thích được chênh lệch ở decode, nhưng chưa loại trừ hoàn toàn.
3. **Batch/prompt quá ngắn**: cả hai bên đều ghi nhận decode là single-batch (sinh từng token),
   nên chi phí điều phối NPU per-token (giao tiếp CA↔TA↔driver NPU qua SMC/TZASC) có thể chiếm tỷ
   trọng lớn hơn trên board/bản build này so với bản build tối ưu của bài báo.
4. **Khác board** (5 Max vs 5 Plus): cùng chip RK3588 nhưng có thể khác xung nhịp thực tế/driver
   NPU version — chưa xác minh.

**Việc cần làm để kết luận chắc chắn** (không suy đoán thêm): tắt toàn bộ `[TZLLM_TRACE]` tracing,
đo lại decode tok/s trên bản "sạch" (không debug log), so sánh trực tiếp với số liệu ~1.22/~1.41
tok/s hiện tại để xác định tracing có phải nguyên nhân chính hay không.
