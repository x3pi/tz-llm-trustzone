# TZ-LLM trên Orange Pi 5 Max — porting TrustZone/NPU inference

Port thực nghiệm của bài báo **TZ-LLM: Protecting On-Device Large Language Models with Arm
TrustZone** (EuroSys '26, [arXiv 2511.13717](https://arxiv.org/abs/2511.13717)) lên board
**Orange Pi 5 Max** (RK3588). Mã nguồn gốc (artifact evaluation) lấy từ
[Zenodo 17054270](https://zenodo.org/records/17054270).

Chạy TinyLlama-1.1B (Q8_0) suy luận bên trong TrustZone secure world, cả qua NPU thật
(`-s 0`) lẫn CPU-only/strawman (`-s 1`), cho output mạch lạc trên cả hai đường — xem
`DEPLOYED_STATE.md` để biết trạng thái đã xác nhận gần nhất.

## ⚠️ QUAN TRỌNG — bản backup không nằm trong git, phải copy tay khi bàn giao/chuyển máy

`checkpoints/golden-image/idbloader_through_vendor.img` (3.4GB) là artifact **duy nhất đã kiểm
chứng** để cấp nguồn (flash) một thẻ nhớ **hoàn toàn trống** thành board chạy được (xem
`PROVISIONING.md` mục 2c, Cách A) — phương án thay thế (`assets/full-flash/`, đã nằm trong git)
**chưa đáng tin cậy**, 2 lần thử đều không boot lên được.

File này **cố ý không đưa vào git** (quá lớn — xem `.gitignore`) nên **không tự động đi theo khi
`git clone`/`git push` sang máy khác**. Hiện chỉ tồn tại **một bản duy nhất trên đĩa này**
(`checkpoints/golden-image/`, không có bản sao nào khác được tìm thấy trên máy). **Khi bàn giao
hoặc chuyển sang máy mới, phải copy thủ công cả thư mục `checkpoints/golden-image/` (3.4GB) —
nếu không, sẽ không còn cách nào đã kiểm chứng để dựng board mới từ thẻ trống.**

## Đọc theo thứ tự này

1. **`DEPLOYED_STATE.md`** — *nguồn sự thật duy nhất* cho câu hỏi "cái gì đang thực sự chạy
   trên board ngay bây giờ". Luôn đọc file này trước khi flash bất cứ thứ gì; đừng suy đoán
   từ tên/timestamp file.
2. **`CLAUDE.md`** — checklist rule ngắn gọn, các lỗi/footgun đã từng gặp và cách tránh lặp
   lại. Đọc trước khi sửa bất kỳ code nào trong project.
3. **`PROVISIONING.md`** — quy trình đầy đủ: build lại từ mã nguồn, flash một board mới từ
   đầu, cách triển khai nhất quán cho nhiều board.
4. **`TESTING_GUIDE.md`** — cách test nhanh (WiFi/hdc + UART) trên board đã flash sẵn.
5. **`STATUS.md`** (~2600 dòng) — nhật ký chi tiết theo thời gian của toàn bộ quá trình debug
   nhiều tuần (mọi giả thuyết đã thử, kể cả các giả thuyết sai). Hữu ích khi cần hiểu **tại
   sao** một quyết định thiết kế được đưa ra, nhưng **không phải điểm khởi đầu** — chỉ tra
   cứu khi cần, đừng đọc từ đầu.
6. **`Bao_Cao_Benchmark_NPU_CPU_TrustZone*.md`** — các báo cáo benchmark NPU vs CPU (3 lần đo
   độc lập), bao gồm so sánh trực tiếp với số liệu công bố trong bài báo gốc.

## Cấu trúc thư mục chính

- `tz-llm/` — toàn bộ mã nguồn: TEE OS kernel, driver, llama.cpp đã patch, U-Boot, kernel Linux REE.
- `checkpoints/` — ảnh boot/uboot đã build sẵn, **đại diện cho trạng thái đã xác nhận hoạt động
  đúng gần nhất** (không phải mọi lần build thử nghiệm — các bản debug trung gian đã dọn khỏi
  git, xem lịch sử commit nếu cần tra lại một mốc cũ). `checkpoints/golden-image/` (3.4GB, không
  nằm trong git — xem `.gitignore`) là bản backup phục hồi đầy đủ, copy thủ công khi dựng máy mới.
- `assets/full-flash/` — ảnh partition đầy đủ để flash một board từ trạng thái trống hoàn toàn.
- `flash/`, `scripts/kick-the-tires/` — script build/flash/repack.
- `plots/` — script vẽ lại các biểu đồ của bài báo gốc (từ artifact evaluation), cần thư mục
  `results/` tự sinh (không có sẵn trong repo này) để chạy.
- `rebuild.sh`, `run_test.py`, `run_test2.py` — script tiện ích ở gốc.

## Yêu cầu phần cứng để test

Board Orange Pi 5 Max (RK3588), cáp USB-C vào cổng MaskROM/OTG, mạch USB-to-TTL UART (baud
**1500000**), thẻ SD/eMMC. Chi tiết đầy đủ trong `PROVISIONING.md` mục 0.
