# Báo Cáo Kỹ Thuật & Hướng Dẫn Kế Tiếp (U-Boot & TA NPU)

> [!TIP]
> Tài liệu này lưu trữ lại toàn bộ quá trình debug lỗi U-Boot và hướng dẫn cụ thể các bước bạn cần làm khi quay lại để nạp bản build hoàn hảo nhất.

## 1. Các Vấn Đề Đã Phân Tích & Giải Quyết Trót Lọt

Trong phiên làm việc vừa rồi, chúng ta đã gặp một loạt lỗi khi cố gắng nạp file `uboot.img` chứa TrustZone App (TA) NPU 55MB. Dưới đây là nguyên nhân và cách giải quyết dứt điểm:

1. **Lỗi `Downloading bootloader failed!`**
   - **Nguyên nhân**: Script nạp bị thiếu lệnh `cs 2` để chuyển đổi giao tiếp sang eMMC (mặc định Maskrom dùng SPI/SD).
   - **Giải pháp**: Đã cập nhật vào script `flash_final_uboot.sh` chạy `cs 2` trước khi nạp `uboot.img`.
2. **Lỗi `Synchronous Abort` trong bộ nạp SPL (Treo ngay đầu)**
   - **Nguyên nhân**: Khi dùng lệnh `mkimage` tiêu chuẩn của Ubuntu để đóng gói FIT image, cấu trúc Device Tree bị hỏng. Hơn nữa, nó không sử dụng định dạng External Data (`-E`) chuẩn của Rockchip.
   - **Giải pháp**: Phải sử dụng script build `make.sh` và `mkimage` chính chủ của Rockchip.
3. **Lỗi `Failed to mount ext2 filesystem` (U-Boot chạy được nhưng không load được OS)**
   - **Nguyên nhân**: Khi dùng script hãng, ban đầu tôi cấu hình `orangepi_5_max_defconfig`. Đây là cấu hình cho Linux truyền thống, nó sẽ tìm file `/extlinux/extlinux.conf` để boot. Nhưng OpenHarmony sử dụng luồng boot tuỳ chỉnh (boot trực tiếp từ phân vùng thô `boot_linux`).
   - **Giải pháp**: Chuyển sang build bằng cấu hình `rk3588-edge` (chuẩn OpenHarmony).
4. **Lỗi `max limit: 2097152 bytes` (Build thất bại vì file quá lớn)**
   - **Nguyên nhân**: Cấu hình `rk3588-edge` giới hạn kích thước phân vùng U-Boot tối đa là 2MB. Do file TA của chúng ta lên tới 55MB, trình biên dịch từ chối đóng gói.
   - **Giải pháp dứt điểm**: Tôi đã sửa mã nguồn U-Boot (`CONFIG_SPL_FIT_IMAGE_KB=131072`), nới rộng bộ nhớ đệm lên **128MB**. Sau đó dùng Docker chính chủ OpenHarmony (`vectorxj0553/tz-llm-oh-builder`) để build lại từ đầu.

**KẾT QUẢ:** Chúng ta hiện đang có một file `uboot.img` hoàn hảo, kích thước 128MB, chứa file TA 55MB, được định dạng chuẩn OpenHarmony và ký số (signed) đầy đủ. File này đã được đặt sẵn tại:
`/mnt/2d4726e7-046b-47c7-b9a9-d2a9cc0cfc8d/Work/OPTEE/tz-llm-ae/scripts/kick-the-tires/share/images/uboot.img`

---

## 2. Các Bước Cần Làm Khi Bạn Quay Lại

Khi bạn đã nghỉ ngơi xong, chỉ cần làm theo ĐÚNG 3 bước sau:

### Bước 1: Đưa mạch vào chế độ Maskrom
- Rút nguồn Orange Pi 5 Max.
- Nhấn giữ nút Maskrom trên bo mạch.
- Cắm nguồn lại, chờ 2-3 giây rồi nhả nút.
- (Chạy thử lệnh `lsusb | grep Rockchip` để xác nhận mạch đang ở chế độ Maskrom).

### Bước 2: Chạy lệnh nạp U-Boot mới
Mở terminal tại thư mục `/mnt/2d4726e7-046b-47c7-b9a9-d2a9cc0cfc8d/Work/OPTEE` và chạy lệnh sau (chỉ nạp đúng `uboot.img` vào eMMC):
```bash
bash flash_final_uboot.sh
```

### Bước 3: Xem Log và Test NPU
1. Bật ngay màn hình theo dõi UART để xem mạch khởi động:
```bash
stty -F /dev/ttyUSB0 1500000 && cat /dev/ttyUSB0
```
2. Ngay khi hệ điều hành OpenHarmony boot xong lên màn hình Lockscreen. Vuốt mở màn hình bằng `uinput` nếu cần.
3. Truy cập qua `hdc shell` và chạy lại ứng dụng test NPU (kịch bản fake tinyllama).
4. Kiểm tra xem lỗi crash `std::stoi` đã hết chưa, và hệ thống có còn bị hang (block thread ở 400% CPU) khi NPU bắn ngắt (IRQ 27, 28, 29) hay không.

Chúc bạn nghỉ ngơi vui vẻ! Mọi thứ phức tạp nhất về quá trình Build & Pack U-Boot đều đã được giải quyết trọn vẹn!
