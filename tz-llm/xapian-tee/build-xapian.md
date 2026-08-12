# Xapian TrustZone - Version 2.0 Release Notes & Build Process

## 🌟 Tính năng mới trong Version 2.0
Để kiểm chứng việc cập nhật code và build lại thành công, hệ thống đã được nâng cấp lên **Version 2.0** với các thay đổi sau:
1. **Sửa lỗi Infinite Loop (Lag máy):** Đã bọc luồng xử lý Search vào vòng lặp `while(1)`, cho phép TEE liên tục lắng nghe và phục vụ nhiều câu hỏi liên tiếp mà không bị kẹt cứng.
2. **Version Identifier:** Bổ sung chữ `(Version 2.0 - Service Loop Enabled)` vào dòng log khởi động của TA để dễ dàng nhận biết.
3. **Thêm tài liệu mới vào Database:** Bổ sung một tài liệu mật mới vào CSDL in-memory của TEE:
   > *"Welcome to Xapian-TA Version 2.0! The infinite loop bug is fixed."*
   > (Bạn có thể test bằng cách dùng `xapian-ca` tìm từ khóa `"infinite"` hoặc `"bug"` để thấy kết quả này trả về).

---

## 🏗 Quá trình Tự động Build (Automated Build Process)
Hệ thống đã tự động chạy chuỗi lệnh sau để đóng gói Version 2.0 vào Firmware:

```bash
# Bước 1: Biên dịch lại TA & CA bằng môi trường Docker chuyên dụng
./scripts/kick-the-tires/xapian-builder.sh bash -x tz-llm/xapian-tee/build-ca-ta.sh

# Bước 2: Bơm file nhị phân TA vào thư mục mã nguồn của TEE OS
cp tz-llm/xapian-tee/xapian-ta/build/xapian-ta scripts/kick-the-tires/xapian-ta

# Bước 3: Biên dịch lại toàn bộ hệ điều hành TEE OS (ChCore) 
cd scripts/kick-the-tires
./oh-builder.sh . bash -x /home/vectorxj/share/build-oh-docker.sh

# Bước 4: Đóng gói hệ điều hành TEE OS vào phân vùng U-Boot
cd ../../
./flash/repack.sh scripts/kick-the-tires/images/uboot.img
```

**Kết quả:** Quá trình này sẽ tạo ra 2 file Firmware mới là `uboot_repacked.img` và `boot.img` lưu ở thư mục `checkpoints/`.

---

## 🚀 Hướng dẫn Flash và Test Version 2.0

Khi quá trình Build chạy ngầm hoàn tất, bạn chỉ việc cắm bo mạch và làm theo đúng 3 bước:

1. Đưa bo mạch về chế độ **Loader/Maskrom**.
2. Flash Firmware mới bằng lệnh:
   ```bash
   sudo ./flash/flash.sh checkpoints/uboot_repacked.img scripts/kick-the-tires/images/boot.img
   ```
3. Sau khi bo mạch khởi động xong, kết nối lại HDC và gọi lệnh test thử để xem thành quả:
   ```bash
   hdc shell "/data/local/tmp/xapian-ca 'infinite'"
   ```
   (Kết quả sẽ trả ra tài liệu mới của Version 2.0!)

---

## 🛠 Tự động hóa quá trình Build (build-all.sh)

Thay vì gõ thủ công 4-5 lệnh rườm rà ở trên, bạn có thể tự động hóa toàn bộ quá trình biên dịch (bao gồm cả TEE Firmware, Xapian TA và Xapian CA) thông qua 1 câu lệnh duy nhất:

```bash
cd tz-llm/xapian-tee
./build-all.sh
```

Tiến trình sẽ chạy ngầm và tự động đóng gói TA vào Firmware. Các file kết quả cuối cùng sẽ tự động xuất hiện tại thư mục `checkpoints/`:
- `xapian-ca` (Ứng dụng phía Linux)
- `uboot_repacked.img` (Firmware U-Boot)
- `boot.img` (Firmware Boot)
