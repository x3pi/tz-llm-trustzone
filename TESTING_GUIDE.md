# Hướng dẫn test TZ-LLM (WiFi/hdc + UART)

IP board mặc định dùng trong hướng dẫn này: `192.168.1.224` (đổi lại nếu router cấp IP khác — kiểm tra bằng `ifconfig wlan0` qua UART).

## Lưu ý quan trọng
1. **TA (secure world) chỉ khởi chạy đúng 1 lần sau mỗi lần board boot.** Muốn test một câu hỏi mới, phải **reboot board trước**, rồi mới chạy lại từ Bước 1.
2. **Câu trả lời thật (`GENERATED_ANSWER`) chỉ in ra UART, KHÔNG có trong log phía CA** (`/data/test_answer.log` chỉ có vài dòng khởi động + `shm_open` lỗi, không có câu trả lời). Bắt buộc phải mở UART để xem kết quả — hdc/WiFi chỉ dùng để mount SSD, gửi file, chạy lệnh, không xem được câu trả lời.

---

## Bước 0: Mở UART (bắt buộc — dùng để xem kết quả và thiết lập hdc)

```bash
picocom -b 1500000 /dev/ttyUSB0
```
Nếu báo lỗi "cannot lock /dev/ttyUSB0": có phiên picocom cũ chưa đóng — tìm và `kill -9` tiến trình đó trước (`ps aux | grep picocom`).

## Bước 1: Reboot board (nếu vừa test xong 1 lần)

Cách đáng tin cậy nhất — qua hdc (nếu đang có kết nối từ lần trước):
```bash
hdc shell "reboot"
```
Nếu không có hdc, gõ `reboot` trực tiếp trong picocom (đôi khi cần gõ Enter trước, rồi mới gõ `reboot`, và chờ vài giây — lệnh không phải lúc nào cũng nhận ngay, có thể cần thử lại). Nếu cả 2 cách đều không thấy board thực sự khởi động lại (uptime trong UART không reset về gần 0), **dùng nút nguồn vật lý trên board** — cách này luôn đáng tin cậy nhất.

Chờ tới khi thấy dòng `All boot events are fired, boot complete now` trên UART (khoảng 55-60 giây từ lúc thấy `U-Boot SPL board init`).

## Bước 2: Thiết lập hdc qua WiFi (làm lại mỗi khi board vừa reboot)

Trong cửa sổ picocom, gõ lần lượt:
```bash
param set persist.hdc.port 8710
param set ohos.ctl.stop hdcd
/system/bin/hdcd -t &
mount -t ext4 /dev/block/nvme0n1p1 /data/ssd
```

## Bước 3: Kết nối từ máy tính (qua WiFi, mở terminal khác)

```bash
hdc tconn 192.168.1.224:8710
```
Nếu báo lỗi kết nối: WiFi có thể chưa lấy được IP — kiểm tra bằng `ifconfig wlan0` trong picocom, đợi thêm hoặc kết nối WiFi thủ công qua giao diện board.

## Bước 4: Chạy test với câu hỏi tự nhập

```bash
hdc -t 192.168.1.224:8710 shell "cd /data/ssd/rknpu && export LD_LIBRARY_PATH=/data/ssd/rknpu && nohup ./ld-linux-aarch64.so.1 ./fake -c 0 -m tinyllama -n 64 -s 1 -t 'What is your name?' > /data/test_answer.log 2>&1 &"
```
(Lưu ý: **bắt buộc có `nohup`** trước `./ld-linux-aarch64.so.1` — đây là cách đã kiểm chứng chạy ổn định qua hdc; thiếu `nohup` chưa được test, có rủi ro tiến trình bị kill khi phiên hdc kết thúc.)

Đổi `'What is your name?'` thành câu hỏi khác tùy ý. Muốn dùng prompt mẫu có sẵn (dài hơn) thay vì tự nhập, bỏ `-t '...'` và thêm `-l 0`.

## Bước 5: Xem kết quả — **qua cửa sổ picocom (UART), KHÔNG qua hdc**

Đợi khoảng 20-60 giây (tùy độ dài câu hỏi), quan sát trực tiếp trong picocom tới khi thấy:
```
===GENERATED_ANSWER_START===
<câu trả lời model sinh ra>
===GENERATED_ANSWER_END===
```
Ngay sau đó là thống kê thời gian thật (`llama_perf_context_print`) — nếu `eval time` hiện `0.00 ms / 1 runs` và câu trả lời trống, đó là dấu hiệu lỗi (không phải bình thường).

### Cách kiểm tra tiến độ mà không cần nhìn UART (tùy chọn, qua hdc)
```bash
hdc -t 192.168.1.224:8710 shell "dmesg | grep -c 'push #'"
```
Chạy lệnh này 2 lần cách nhau ~10 giây — nếu số không tăng trong nhiều phút, có thể đã kẹt (dù hiếm gặp sau các fix gần đây).

---

## Giải thích tham số `fake`

| Tham số | Ý nghĩa |
|---|---|
| `-m tinyllama` | Model dùng test (khác: `gemma`, `qwen`, `phi`, `llama`) |
| `-n 64` | Số token tối đa sinh ra |
| `-s 1` | **Strawman = CPU-only, đường an toàn** — cho output mạch lạc, đã xác nhận hoạt động |
| `-s 0` | Dùng NPU thật — **hiện đang lỗi**, ra ký tự rác (bug tính toán NPU chưa fix) |
| `-c 0` | Cache proportion (giữ 0) |
| `-t "câu hỏi"` | Prompt tự do — thay cho bảng prompt cố định mặc định |
| `-l 0` | Dùng prompt mẫu cố định số 0 trong bảng có sẵn (bỏ qua nếu dùng `-t`) |

## Nếu gõ lệnh UART mà không có phản hồi (chỉ echo lại, không chạy)

Khả năng cao do nhiều tiến trình đọc UART cùng lúc phía máy tính gây nhiễu dữ liệu — kiểm tra `ps aux | grep ttyUSB0`, đóng bớt phiên cũ trước khi mở mới.
