# Quá trình Fix Bugs để Build EVM trong TEE (TrustZone)

Tài liệu này ghi lại toàn bộ các bước và những thay đổi kỹ thuật (patches) đã được áp dụng để biên dịch thành công hệ thống EVM (lấy từ `c_mvm`) vào trong môi trường TEE (ChCore/StarOS) với trình biên dịch `aarch64-linux-musl-gcc 9.2.0` (chỉ hỗ trợ tối đa **C++17**).

## 1. Vấn đề tương thích chuẩn C++ (C++20 vs C++17)

Mã nguồn gốc của EVM (`intx`, `evmmax`, `bn254`, v.v.) sử dụng nhiều tính năng của chuẩn C++20, trong khi toolchain của TEE chỉ hỗ trợ C++17. Chúng ta đã phải "hạ cấp" (downgrade/polyfill) các đoạn code này:

### 1.1. Loại bỏ `<compare>` và `<concepts>` (trong `intx.hpp`)
- **Lỗi:** Toolchain không tìm thấy các header `<compare>` và `<concepts>`.
- **Cách fix:** 
  - Xóa `#include <compare>` và `#include <concepts>`.
  - Thay thế kiểu trả về `std::strong_ordering` bằng các kiểu nguyên thủy như `int` (trả về `-1`, `0`, `1`).
  - Thay thế toán tử `<=>` bằng các phép so sánh thủ công `<` và `>`.
  - Thay các template ràng buộc bằng concepts (ví dụ `std::integral auto`) thành các template chuẩn của C++17 (`template <typename T>`).

### 1.2. Vấn đề với `std::tie` trong ngữ cảnh `constexpr`
- **Lỗi:** C++17 không cho phép sử dụng `std::tie(...) = ...` bên trong một hàm `constexpr` (lỗi tại `evmmax.hpp` trong các hàm toán học lớn).
- **Cách fix:**
  - Thay thế `std::tie` bằng cách lấy kết quả (thường là `std::pair` hoặc struct) vào một biến trung gian `auto _res = ...;`
  - Sau đó gán tuần tự `c = _res.first; t[j] = _res.second;`.
  - Đảm bảo thêm dấu ngoặc nhọn `{}` cho các vòng lặp `for` có chứa đoạn gán này.

### 1.3. Lỗi `operator==` với từ khóa `= default`
- **Lỗi:** C++17 không hỗ trợ tự động sinh `operator==` bằng cú pháp `= default;` bên trong struct (lỗi ở `field_template.hpp`).
- **Cách fix:**
  - Tự viết tường minh hàm `friend constexpr bool operator==(const Type& a, const Type& b)`.
  - Bổ sung thêm hàm `operator!=` vì C++17 không tự suy luận `!=` từ `==` như C++20.
  - Đối với `ExtFieldElem` chứa `std::array`, vì `std::array::operator==` cũng không phải `constexpr` trong C++17, ta phải dùng vòng lặp `for` thủ công để so sánh từng phần tử.

### 1.4. Loại bỏ `<span`>
- **Lỗi:** Lỗi `span: No such file or directory`. `std::span` là tính năng mới của C++20.
- **Cách fix:**
  - Đã xóa `#include <span>` trong các file `bn254.hpp`, `pairing.cpp`, `my_extension.cpp`, `crypto_handlers.cpp`.
  - Chuyển các tham số hàm từ `std::span<const std::pair<Point, ExtPoint>>` thành `const std::vector<std::pair<Point, ExtPoint>>&`.

### 1.5. `std::ranges::reverse` và `std::bit_cast`
- **Lỗi:** Không có thư viện `ranges` và `bit_cast` trong C++17.
- **Cách fix:**
  - Thay `std::ranges::reverse` bằng `std::reverse`.
  - Tự viết hàm template `bit_cast` dựa trên `std::memcpy` thay vì dùng thư viện chuẩn.

## 2. Loại bỏ các thư viện phụ thuộc bên ngoài (External Dependencies)

TEE Enclave không có sẵn các thư viện tĩnh/động phức tạp như trên Host OS (Linux). Quá trình build thất bại khi liên kết với một số precompiles yêu cầu thư viện mở rộng.

### 2.1. Thư viện MPFR (`mpfr.h`)
- **Vấn đề:** Các hàm tính toán `Math`, `Modexp` do người dùng viết thêm sử dụng thư viện MPFR (Multiple Precision Floating-Point Reliable) để xử lý số nguyên lớn. Tuy nhiên `libmpfr` không có sẵn trong toolchain của ChCore.
- **Cách fix:**
  - Comment out `#include <mpfr.h>` trong `my_extension.h`, `my_extension.cpp`, `utils.h`, `utils.cpp` và `crypto_handlers.cpp`.
  - Bọc các hàm tiện ích như `evm_encode_mpfr`, `hexToSignedInt`, `signedIntToHex` bằng `#if 0 ... #endif` để trình biên dịch bỏ qua chúng.

### 2.2. Thư viện secp256k1 (`secp256k1.h`)
- **Vấn đề:** Lỗi thiếu file header `secp256k1.h` và `secp256k1_recovery.h`. Code đã được mock phần implementation (có dòng ghi chú `// Mocked for TEE RAM EVM to avoid libsecp256k1 dependency`) nhưng các chỉ thị `#include` vẫn còn tồn tại ở đầu file.
- **Cách fix:** Comment out hoặc xóa các `#include <secp256k1...>` tại file `my_extension.cpp` và `crypto_handlers.cpp`.

## 3. Tổng kết

Bằng việc kết hợp các **C++17 polyfills** (code lại tính năng C++20 bằng tay) và **loại bỏ các external dependencies không cần thiết** (mocked precompiles), mã nguồn EVM đã vượt qua quá trình kiểm tra khắt khe của compiler `musl-gcc`.

EVM TA (TrustZone App) hiện tại được biên dịch hoàn toàn tĩnh (static) và có khả năng chạy độc lập bên trong secure RAM (ChCore/StarOS) mà không bị phụ thuộc vào OS Host.
