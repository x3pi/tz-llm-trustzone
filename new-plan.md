# Kế hoạch triển khai & Xác nhận quy trình Build Xapian trên TEE

Tài liệu này tổng hợp lại toàn bộ quy trình chuẩn để build và chạy Xapian Search Engine bên trong TrustZone (TEE) trên board Orange Pi 5 Max. Quy trình này **đã được kiểm chứng là hoạt động thành công 100%** dựa trên log thực tế.

## 1. Xác nhận quá trình Build (Đã chuẩn xác)

Quá trình build hiện tại trong `README.md` là hoàn toàn chính xác và hợp lý với kiến trúc Bare-metal/ChCore TEE. Dưới đây là luồng hoạt động:

1. **Cross-compile thư viện lõi (Xapian Core & Zlib):**
   - Chạy qua Docker (`xapian-builder.sh`) để đảm bảo đúng toolchain AArch64 của TEE.
   - Script: `build-deps.sh` và `build-xapian.sh`.
2. **Biên dịch CA (Normal World) và TA (Secure World):**
   - Script: `build-ca-ta.sh`.
   - Kết quả sinh ra file thực thi `xapian-ca` (dành cho Linux) và file nhị phân `xapian-ta` (dành cho TEE).
3. **Đóng gói TA vào thẳng TEE OS (Firmware):**
   - Vì cấu trúc TEE ở đây không nạp `.ta` linh hoạt từ File System của Linux, file `xapian-ta` được copy thẳng vào source code của OS và build lại nhân OS (`oh-builder.sh`).
4. **Repack U-boot:**
   - Đóng gói TEE OS mới vào `uboot_repacked.img` để tương thích với phân vùng `boot_linux` của thẻ nhớ.

> [!TIP]
> **Đánh giá kiến trúc:** Việc nhúng thẳng `xapian-ta` vào Firmware (U-boot/Boot) là lý do tại sao vừa rồi bạn chỉ cần gửi mỗi file `xapian-ca` vào bo mạch là chạy được ngay! TA đã nằm sẵn trong nhân bảo mật từ lúc bạn flash `golden-image` hoặc `uboot_repacked.img`.

---

## 2. Kế hoạch triển khai (Deployment Plan) chuẩn cho các lần sau

Nếu sau này bạn có chỉnh sửa code C++ của Xapian (ví dụ thêm thuật toán tìm kiếm mới), bạn chỉ cần làm đúng theo checklist sau:

### Giai đoạn 1: Build trên máy chủ (Server/Laptop)
- [ ] Chạy `build-ca-ta.sh` để biên dịch lại code mới.
- [ ] Chép `xapian-ta` vào thư mục `images` của TEE OS.
- [ ] Chạy `oh-builder.sh` để build lại TEE OS mới.
- [ ] Chạy `repack.sh` để tạo ra file `uboot_repacked.img`.

### Giai đoạn 2: Flash Firmware xuống bo mạch
- [ ] Đưa bo mạch về chế độ Maskrom/Loader.
- [ ] Chạy lệnh flash: `sudo ./flash/flash.sh checkpoints/uboot_repacked.img checkpoints/boot.img`.
- [ ] Khởi động lại board và đợi boot hoàn tất (`boot complete`).

### Giai đoạn 3: Bật HDC và truyền file Client (CA)
- [ ] Cắm cáp UART, vào Picocom và mở cổng HDC:
  ```bash
  mkdir -p /data/local/tmp
  chmod 777 /data/local/tmp
  param set persist.hdc.mode tcp
  param set persist.hdc.port 8710
  killall -9 hdcd
  hdcd &
  ```
- [ ] Trên Laptop, gửi file `xapian-ca` mới nhất vào board:
  ```bash
  hdc tconn 192.168.1.200:8710
  hdc file send checkpoints/xapian-ca /data/local/tmp/
  ```

### Giai đoạn 4: Chạy test và nghiệm thu
- [ ] Vào shell của board và cấp quyền thực thi:
  ```bash
  hdc shell
  cd /data/local/tmp
  chmod +x xapian-ca
  ```
- [ ] Gửi truy vấn (Query) để kiểm tra:
  ```bash
  ./xapian-ca "Từ-khóa-cần-tìm"
  ```
- [ ] Quan sát kết quả trả về từ TEE. Nếu in ra `Found X matches` và nội dung đúng, quá trình triển khai thành công!

> [!NOTE]
> **Về vấn đề lưu trữ (Storage):** Hiện tại hệ thống đang chạy `InMemory` (RAM). Nếu tương lai dự án yêu cầu lưu Database lượng lớn, hãy lên kế hoạch lập trình thêm một chức năng O-Call (SMC ngược) để TA băm nhỏ dữ liệu đã mã hóa và gửi ra cho CA ghi xuống file `/data/ssd/xapian.db`.
