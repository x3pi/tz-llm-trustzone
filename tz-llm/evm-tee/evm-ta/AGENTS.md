# QUY TẮC BẮT BUỘC CHO AI AGENT TRONG MODULE EVM-TA (TEE)

## 📌 QUY TẮC CỐT LÕI (MANDATORY RULE):
Bất kỳ khi nào AI Agent phát hiện lỗi (bug), điều chỉnh logic, sửa mã nguồn hoặc giải quyết một vấn đề trong `evm-ta`, **BẮT BUỘC** phải ghi lại chi tiết vào file tài liệu:
👉 **`/home/abc/nhat/tz-llm-trustzone/tz-llm/evm-tee/evm-ta/error-build-xpain-evm.md`**

---

### 📝 Cấu Trúc Bắt Buộc Khi Thêm Mục Mới Vào `error-build-xpain-evm.md`:
Mỗi lỗi được sửa phải ghi rõ các phần:
1. **Tiêu đề lỗi:** Tên ngắn gọn của lỗi và phân loại (Build-time / Runtime / Logic).
2. **Triệu chứng (Symptoms):** Lệnh nào bị lỗi, thông báo lỗi trên UART hoặc terminal là gì.
3. **Vị trí file:** Đường dẫn tuyệt đối của các file liên quan.
4. **Bản chất nguyên nhân gốc rễ (Root Cause):** Phân tích chi tiết tại sao lỗi xảy ra, cơ chế luồng dữ liệu bị sai như thế nào.
5. **Cách khắc phục (Fix Applied):** Đoạn mã trước và sau khi fix, lý do tại sao cách fix này triệt để và an toàn.

---

### ⚙️ Các Lưu Ý Quan Trọng Về Môi Trường EVM-TA Trong TEE:
1. **Toolchain C++17:** Môi trường ChCore/Musl chỉ hỗ trợ C++17. Không sử dụng các tính năng C++20/C++23 (`<compare>`, `<concepts>`, `std::byteswap`, `std::bit_cast`, `std::span` chuẩn).
2. **Xapian In-Memory:** Database trên RAM không hỗ trợ `reopen()`. Mọi thao tác đọc/ghi phải trỏ vào cùng 1 live DB `&this->db` và đồng bộ qua `std::shared_mutex`.
3. **Exception Handling:** Không bao giờ để `pop64()` hoặc bất kỳ thao tác nào throw Exception thứ hai bên trong khối `catch`/`eh`/`he` vì sẽ gọi `std::terminate()` $\to$ `tkill` (syscall 130 không hỗ trợ làm crash TEE).
4. **State Persistence:** Sau mỗi giao dịch `DEPLOY` hoặc `CALL`, mọi thay đổi storage slot của hợp đồng phải được lưu vào `State::getInstance(address)->insertOrUpdate(...)` để đảm bảo không bị mất trạng thái giữa các giao dịch.
