# Báo Cáo Tổng Hợp Lỗi & Giải Pháp: Tích Hợp Xapian Vào EVM Trong TEE (TrustZone)

Tài liệu này tổng hợp toàn bộ các lỗi gặp phải trong quá trình biên dịch (Build-time) và thực thi (Runtime) khi tích hợp **Xapian Database Engine** vào **EVM (Ethereum Virtual Machine)** chạy trong môi trường **TrustZone TEE (StarOS / ChCore)**, cùng giải pháp khắc phục chi tiết.

> [!NOTE]
> **Quy tắc bắt buộc cho AI Assistant / Developers:** Mỗi khi phát hiện và sửa chữa bất kỳ lỗi (bug) nào liên quan đến EVM-TA hoặc Xapian trong TEE, **BẮT BUỘC** phải cập nhật lại tài liệu này để ghi lại nguyên nhân gốc rễ và cách xử lý chi tiết.

---

## 1. Nhóm Lỗi Biên Dịch (Build-time: C++20 vs C++17 Toolchain)

### 📌 Bối cảnh:
Mã nguồn EVM gốc sử dụng nhiều tính năng của chuẩn **C++20/C++23**, trong khi trình biên dịch chéo của TEE (`aarch64-linux-musl-gcc 11.4` trong Docker `tz-llm-llama-builder`) chỉ hỗ trợ hoàn chỉnh chuẩn **C++17**.

---

### 1.1. Lỗi thiếu thư viện và cú pháp C++20 trong `intx.hpp`
* **Triệu chứng:**
  * Lỗi `#include <compare>` và `#include <concepts>` không tìm thấy hoặc bị vô hiệu hóa.
  * Cú pháp `std::integral auto` và toán tử `operator<=>` (three-way comparison) không được hỗ trợ.
  * `std::ranges::reverse` không tồn tại.
  * Lỗi `std::tie(...) = addc(...)` trong các hàm `constexpr` (do `std::tuple::operator=` chưa phải là `constexpr` ở C++17).
* **Vị trí file:** `tz-llm/evm-tee/evm-ta/c_mvm/3rdparty/intx/include/intx/intx.hpp`
* **Cách khắc phục:**
  * Bọc `<compare>` trong `#if __has_include(<compare>)`.
  * Thay thế `std::integral auto` bằng template chuẩn C++17 (`template <typename T>`).
  * Thay `std::ranges::reverse` bằng `std::reverse(s.begin(), s.end())`.
  * Thay toàn bộ `std::tie(...)` trong `constexpr add` và `constexpr udivrem_knuth` bằng phép gán trực tiếp các trường struct (`s[i] = res.value; carry = res.carry;`).

---

### 1.2. Lỗi `std::byteswap` và `std::bit_cast` trong `ripemd160.cpp`
* **Triệu chứng:** Trình biên dịch báo lỗi hàm `std::byteswap` (C++23) và `std::bit_cast` (C++20) không tồn tại trong namespace `std`.
* **Vị trí file:** `tz-llm/evm-tee/evm-ta/c_mvm/src/crypto/ripemd160.cpp`
* **Cách khắc phục:**
  * Viết lại hàm đảo byte sử dụng hàm nội tại của GCC: `__builtin_bswap32` và `__builtin_bswap64`.
  * Thay thế `std::bit_cast` bằng `std::memcpy` chuẩn C++17.

---

### 1.3. Lỗi thiếu Header `<span>` trong `bn254.hpp`
* **Triệu chứng:** Lỗi `#include <span>` (C++20) không tìm thấy khi biên dịch thuật toán kiểm tra cặp điểm đường cong elliptic (`pairing_check`).
* **Vị trí file:** `tz-llm/evm-tee/evm-ta/c_mvm/include/mvm/crypto/bn254.hpp`
* **Cách khắc phục:**
  * Tạo struct `std::span` thu nhỏ tương thích C++17:
    ```cpp
    template <typename T>
    struct span {
        const T* ptr;
        size_t sz;
        span() noexcept : ptr(nullptr), sz(0) {}
        template <typename U>
        span(const std::vector<U>& v) noexcept : ptr(reinterpret_cast<const T*>(v.data())), sz(v.size()) {}
        span(const T* p, size_t s) noexcept : ptr(p), sz(s) {}
        const T* data() const noexcept { return ptr; }
        size_t size() const noexcept { return sz; }
        bool empty() const noexcept { return sz == 0; }
        const T& operator[](size_t i) const noexcept { return ptr[i]; }
        const T* begin() const noexcept { return ptr; }
        const T* end() const noexcept { return ptr + sz; }
    };
    ```

---

### 1.4. Lỗi `std::tie` và `operator==` trong `evmmax.hpp` & `field_template.hpp`
* **Triệu chứng:**
  * Trong `evmmax.hpp`: Hàm nhân Montgomery `constexpr mul(...)` gọi `std::tie(c, t[j]) = addmul(...)` (gán `std::pair` vào `std::tuple` không phải là `constexpr` ở C++17).
  * Trong `field_template.hpp`: Khai báo `operator== ... = default;` trong hàm `constexpr` (gọi `std::array::operator==` chưa phải `constexpr` trong C++17 libstdc++).
* **Vị trí file:**
  * `tz-llm/evm-tee/evm-ta/c_mvm/include/evmmax/evmmax.hpp`
  * `tz-llm/evm-tee/evm-ta/c_mvm/include/mvm/crypto/pairing/field_template.hpp`
* **Cách khắc phục:**
  * Trong `evmmax.hpp`: Gán trực tiếp giá trị `const auto p = addmul(...); c = p.first; t[j] = p.second;`.
  * Trong `field_template.hpp`: Thay `= default;` bằng vòng lặp `for` so sánh từng phần tử `coeffs[i] == other.coeffs[i]`.

---

## 2. Nhóm Lỗi Đồng Bộ Interface & Kiến Trúc Nguồn Độc Lập

### 2.1. Đồng bộ Interface Journaling trong Storage & GlobalState
* **Triệu chứng:** Lớp `mvm::Processor` mới sử dụng `StateJournal` để rollback trạng thái khi sub-call revert. Các class `MyStorage` và `MyGlobalState` ở `evm-ta/linker/` bị thiếu các phương thức pure virtual mới.
* **Cách khắc phục:**
  * Cập nhật `my_storage.cpp/h` và `my_global_state.cpp/h`.
  * Cài đặt đầy đủ các hàm: `snapshot_storage_change`, `clear_storage_change`, `has_cached`, `get_cached`, `set_cached_raw`, `erase_cached`, `snapshot_cached`, `clear_all_cached`.

### 2.2. Gộp toàn bộ mã nguồn EVM về một nơi duy nhất bên trong `evm-ta`
* **Vấn đề ban đầu:** `CMakeLists.txt` của `evm-ta` trỏ chéo sang `/home/abc/nhat/con-chain-v2/...`, tạo ra sự phân mảnh giữa 2 source code.
* **Cách khắc phục:**
  * Sao chép toàn bộ thư mục `c_mvm` vào `tz-llm/evm-tee/evm-ta/c_mvm`.
  * Cập nhật `CMakeLists.txt`: `set(MVM_ROOT ${CMAKE_CURRENT_LIST_DIR}/c_mvm)`.
  * Giờ đây `evm-ta` hoàn toàn độc lập và tự đóng gói trọn vẹn.

---

## 3. Nhóm Lỗi Thực Thi Runtime (Treo tại `incrementShared()`)

---

### 3.1. Nguyên nhân 1: Xapian In-Memory DB không tương thích với Database Pool
* **Bản chất:**
  * Ban đầu, `XapianManager` sử dụng Pool (`search_pool` và `simple_read_pool`) được thiết kế cho Database lưu trên ổ cứng (Disk).
  * Trong TEE, ta dùng **`Xapian::InMemory::open()` (chạy thuần RAM)**. Khi khởi tạo Pool, lệnh `new Xapian::Database(this->db)` tạo ra một **bản sao Snapshot bị đóng băng tại thời điểm DB còn rỗng**.
  * Với In-Memory DB, hàm `reopen()` **hoàn toàn vô hiệu**. Do đó, khi `initializeDoc()` ghi document vào `this->db`, các kết nối trong Pool không thấy dữ liệu mới.
* **Cách khắc phục:**
  * Trong `tz-llm/evm-tee/evm-ta/linker/src/xapian/xapian_manager.cpp`: Chuyển `acquireSearchDb()` và `acquireSimpleReadDb()` **trả về trực tiếp con trỏ `&this->db`**.
  * Kết hợp với khóa đọc/ghi song song `std::shared_mutex changes_mutex`, toàn bộ các luồng đọc vẫn chạy đồng thời song song 100% trên live database trong RAM.

---

### 3.2. Nguyên nhân 2: Mất trạng thái Storage Slot giữa các Transaction (Lỗi `State Singleton`)
* **Bản chất:**
  * Trong `SharedUpdate.sol`, `initializeDoc()` lưu ID tài liệu vào biến trạng thái hợp đồng:
    ```solidity
    function initializeDoc() external {
        sharedDocId = fullDB.newDocument(DB_NAME, abi.encode(uint256(0)));
    }
    ```
    Lệnh này thực hiện `SSTORE(slot 0, docId)`.
  * Sau khi transaction `initializeDoc()` chạy xong, vòng lặp lưu storage trong `main.cpp` ban đầu có điều kiện:
    ```cpp
    gs.iterate_storage_changes([&](const mvm::Address &a, const uint256_t &k, const uint256_t &v) {
        if (State::instanceExists(a)) { // ❌ Lúc này State cho contract chưa được tạo -> Bị bỏ qua!
            State::getInstance(a)->insertOrUpdate(...);
        }
    });
    ```
  * Do `State::instanceExists(a)` là `false`, slot 0 (`sharedDocId`) **không hề được lưu vào bộ nhớ RAM lâu dài (`State`)**.
  * Khi bước sang transaction `incrementShared()`:
    ```solidity
    function incrementShared() external {
        bytes memory data = fullDB.getDataDocument(DB_NAME, sharedDocId); // sharedDocId đọc ra = 0!
        ...
    }
    ```
    EVM đọc `SLOAD(slot 0)` $\to$ `MyStorage::load` không tìm thấy trong `State` $\to$ trả về `0`.
  * Hợp đồng gọi `getDataDocument(DB_NAME, 0)` $\to$ Xapian không có Doc ID `0` $\to$ trả về rỗng $\to$ Solidity `abi.decode` gặp dữ liệu rỗng $\to$ kích hoạt **`REVERT`**!
* **Cách khắc phục:**
  * Trong `tz-llm/evm-tee/evm-ta/main.cpp`:
    1. Khi `DEPLOY` hợp đồng: Khởi tạo và lưu bytecode, số dư, nonce vào `State::getInstance(callee)`.
    2. Sau mỗi lần thực thi (`DEPLOY` và `CALL`): Bỏ điều kiện `State::instanceExists(a)`, luôn gọi trực tiếp `State::getInstance(a)->insertOrUpdate(...)` để đảm bảo 100% các biến trạng thái (storage slot) của hợp đồng được lưu giữ xuyên suốt giữa các transaction.

---

### 3.3. Nguyên nhân 3: Stack Underflow trong Exception Handler (`eh` & `he`) gây Crash TEE OS
* **Bản chất:**
  * Khi Solidity hoặc Precompile sub-call `REVERT`, EVM ném ngoại lệ `mvm::Exception`.
  * Trong `processor.cpp`, exception handlers `eh` (top-level) và `he` (sub-calls/precompiles) cố gắng lấy thông tin revert bằng `ctxt->s.pop64()`.
  * Nếu stack `ctxt->s` có ít hơn 2 phần tử, `pop64()` ném tiếp một ngoại lệ `Exception(ET::outOfBounds)` **ngay bên trong khối `catch`**.
  * Theo chuẩn C++, ném exception trong lúc đang xử lý exception sẽ gọi `std::terminate()` $\to$ `abort()` $\to$ gọi `tkill` (Syscall 130). Do ChCore TEE OS không hỗ trợ syscall 130, kernel lập tức kill tiến trình `evm-ta`, khiến `evm-ca` phía Linux bị treo vĩnh viễn ở `ioctl`.
* **Cách khắc phục:**
  * Trong `processor.cpp`:
    1. Bọc an toàn `if (ctxt && ctxt->s.size() >= 2)` trước khi gọi `pop64()` ở cả 2 handler `eh` và `he`.
    2. Bọc toàn bộ khối handler bằng `try-catch (...)`.
  * Trong `main.cpp`: Bổ sung khối bắt ngoại lệ toàn diện `catch (const mvm::Exception& e)`, `catch (const std::exception& e)`, và `catch (...)`.

---

### 3.4. Nguyên nhân 4: Nhận diện sai `UintTy` và Lỗi Mở Rộng Dấu (`signExtension`) trong ABI Encoder
* **Bản chất:**
  * Trong `abi_utilities.hpp`, hàm `getType` kiểm tra `type.rfind("int")` trước, khiến kiểu `uint256` bị nhận diện nhầm thành `IntTy` (số nguyên có dấu).
  * Trong `abi_encode.hpp`, hàm `encodeInt` kiểm tra bit cao nhất `(originBytes[0] & 0x80) ? 0xFF : 0x00` (sign extension).
  * Đối với `docId` (số nguyên không dấu `uint256` có byte đầu $\ge 0x80$), các byte đầu bị điền toàn bộ `0xFF...` thay vì `0x00...`.
  * Khi Solidity nhận lại `newDocID`, giá trị bị sai lệch hoàn toàn so với chuỗi Hex lưu trong Xapian $\to$ Không tìm thấy document khi đọc.
* **Cách khắc phục:**
  * Trong `abi_utilities.hpp`: Thêm kiểm tra `type.rfind("uint")` trước để trả về đúng kiểu `UintTy`.
  * Trong `abi_encode.hpp`: Viết hàm `encodeUint` chuẩn với cơ chế Zero-padding (`0x00`) từ bên trái cho mọi số không dấu `uint256`.

---

### 3.5. Nguyên nhân 5: Phân biệt hoa/thường (Case-sensitivity) của Doc ID trong Term Indexing
* **Bản chất:**
  * Hàm `new_document` sinh ra mã hash không có tiền tố `0x`, trong khi các hàm đọc `get_data` nhận mã hash có tiền tố `0x` và có thể có ký tự hoa/thường khác nhau.
  * Xapian tìm kiếm Term `Q<docId>` phân biệt chính xác từng byte ký tự, dẫn đến `resolveVirtualDocId` không tìm thấy Postlist.
* **Cách khắc phục:**
  * Chuẩn hóa toàn bộ `clean_id` về chữ thường (`tolower`) và loại bỏ tiền tố `0x` / `0X` ở cả hai hàm `resolveVirtualDocId` (`xapian_manager_resolve.cpp`) và `replay_log` (`xapian_manager.cpp`).

---

### 3.6. Bổ sung Logging Debug Toàn Diện
* **Đã thêm:**
  * In toàn bộ câu lệnh và calldata nhận được: `[EVM-TA] Received command: ...`
  * In kết quả thực thi chi tiết của `DEPLOY` và `CALL`: `res.er`, `gas_used`, `exmsg`.
  * In thông tin chi tiết khi tạo tài liệu: `[DEBUG] XAPIAN_V1_NEW_DOCUMENT: dbname=... newDocID=...`
  * In thông tin chi tiết khi đọc tài liệu: `[DEBUG] XAPIAN_V1_GET_DATA_DOCUMENT: dbname=... docId_hex=... docInfo_len=...`

---

### 3.7. Nguyên nhân 6: Thiếu Nhánh Decode cho `UintTy` gây Exception `type_error.302` trong `nlohmann::json`
* **Triệu chứng:** Khi chạy `incrementShared()`, TEE OS lập tức crash với lỗi:
  ```text
  terminate called after throwing an instance of 'nlohmann::detail::type_error'
    what(): [json.exception.type_error.302] type must be string, but is object
  Unsupported syscall 130, bye.
  ```
* **Vị trí file:**
  * `tz-llm/evm-tee/evm-ta/linker/include/abi_decode.hpp`
  * `tz-llm/evm-tee/evm-ta/linker/include/abi_parser.hpp`
* **Bản chất:**
  * Khi ta thêm `UintTy` vào hàm phân loại kiểu `getType()` trong `abi_utilities.hpp`, trong hàm `decodeElement()` của `abi_decode.hpp` chỉ có nhánh `if (t == IntTy) { result = decodeInt(...); }` mà thiếu kiểm tra `if (t == UintTy)`.
  * Khi hàm `GET_DATA_DOCUMENT` giải mã calldata ABI chứa `{"internalType": "uint256", "name": "docId", "type": "uint256"}`, hàm `decodeElement()` không khớp nhánh nào, dẫn tới `result` vẫn là `json::object()` rỗng (`{}`) thay vì trả về chuỗi Hex `hexString`.
  * Khi `xapian_handlers.cpp` thực hiện `input_argument["docId"].get<std::string>()`, thư viện `nlohmann::json` phát hiện node JSON đang là `object` chứ không phải `string` $\to$ ném ngoại lệ `json.exception.type_error.302` $\to$ Gây crash TEE OS!
* **Cách khắc phục:**
  * Cập nhật `decodeElement()` trong cả `abi_decode.hpp` và `abi_parser.hpp`:
    ```cpp
    if (t == IntTy || t == UintTy) {
        result = decodeInt(bytes, i, abi, totalLength);
    }
    ```

---

### 3.8. Nguyên nhân 7: Khởi tạo nhầm `isOffChain = true` trong `MyExtension` khiến bỏ qua ghi Xapian DB
* **Triệu chứng:** Khi chạy `incrementShared()`, `getDataDocument` đọc ra dữ liệu rỗng (`docInfo_len = 0`), smart contract kích hoạt opcode `REVERT` do không tìm thấy dữ liệu (`Data not found in Xapian`), gây `mvm::Exception` và crash TEE OS.
* **Vị trí file:** `tz-llm/evm-tee/evm-ta/main.cpp` (dòng 98)
* **Bản chất:**
  * Constructor của `MyExtension` có chữ ký: `MyExtension(unsigned char *id, bool offChain = false, const uint256_t *hash = nullptr)`.
  * Trong `main.cpp`, biến `extension` bị khởi tạo: `MyExtension extension(nullptr, true, nullptr);` (`isOffChain = true`).
  * Trong `xapian_handlers.cpp`, khi `isOffChain == true`, toàn bộ các hàm ghi (`NEW_DOCUMENT`, `SET_DATA`,...) bị coi là mô phỏng `eth_call`, lập tức trả về `1` giả định mà hoàn toàn không ghi tài liệu vào Xapian InMemory DB.
  * Khi `initializeDoc()` (TX #4) chạy, `newDocument` trả về `1` giả định $\to$ gán `sharedDocId = 1` nhưng DB rỗng $\to$ TX #5 gọi `getDataDocument(..., 1)` trả về rỗng $\to$ Solidity revert.
* **Cách khắc phục:**
  * Sửa lại `main.cpp`: Khởi tạo `MyExtension extension(nullptr, false, nullptr);` (chế độ On-Chain thực thi ghi trạng thái thật).

---

## 4. Nhóm Cải Tiến Kiến Trúc Ví Chuẩn Ethereum (Single-Wallet EOA)

* **Vấn đề ban đầu:** Hệ thống sử dụng địa chỉ người gọi tĩnh (`caller = 0`) và địa chỉ contract cố định (`callee = 1`), không tuân theo quy chuẩn của Ethereum.
* **Đã nâng cấp:**
  1. **Khởi tạo EOA Wallet chuẩn:** Sử dụng ví mặc định `0xf39Fd6e51aad88F6F4ce6aB8827279cffFb92266` với số dư ban đầu `1,000,000 ETH` (`10^24 Wei`) và `nonce = 0`.
  2. **Tính toán địa chỉ Contract tự động (`generate_address`):** Khi `DEPLOY`, địa chỉ smart contract được sinh bằng công thức chuẩn:
     $$\text{ContractAddress} = \text{keccak256}(\text{RLP}([\text{sender}, \text{nonce}]))[12:]$$
  3. **Tự động quản lý Nonce:** Mỗi giao dịch (Deploy hoặc Call) đều tăng `nonce` của ví lên 1 đơn vị, mô phỏng chính xác 100% hoạt động của ví Web3 trên thực tế.

---

## 5. Quy Trình Chạy Test Hoàn Chỉnh Sau Khi Fix

```text
=======================================================
🦊 [EVM-CA] TEE EVM Wallet Test Suite (Standard EVM EOA)
=======================================================
[WALLET] WALLET: 0xf39fd6e51aad88f6f4ce6ab8827279cfffb92266 BALANCE: 0xd3c21bcecceda1000000 NONCE: 0

[TX #1] 🚀 Deploying Basic 42 Contract from Wallet...
  ↳ Result: SUCCESS - Contract deployed at address 0x5fbdb2315678afecb367f032d93f642f64180aa3 by 0xf39fd6e51aad88f6f4ce6ab8827279cfffb92266 (Nonce: 0). Gas used: 54

[TX #2] 📞 Calling Basic Contract at 0x5fbdb2315678afecb367f032d93f642f64180aa3...
  ↳ Result: SUCCESS - Output: 000000000000000000000000000000000000000000000000000000000000002a Gas: 18 Nonce: 1
  ↳ Output Verification: 42 (0x2a) ✅ PASSED

[TX #3] 🚀 Deploying SharedUpdate Contract (Xapian Precompile 107)...
  ↳ Result: SUCCESS - Contract deployed at address 0x9fe46736679d2d9a65f0992f2272de9f3c7fa6e0 by 0xf39fd6e51aad88f6f4ce6ab8827279cfffb92266 (Nonce: 2). Gas used: 8521

[TX #4] 📝 Calling initializeDoc() on 0x9fe46736679d2d9a65f0992f2272de9f3c7fa6e0...
  ↳ Result: SUCCESS - Output:  Gas: 21802 Nonce: 3

[TX #5] ➕ Calling incrementShared() on 0x9fe46736679d2d9a65f0992f2272de9f3c7fa6e0...
  ↳ Result: SUCCESS - Output:  Gas: 15420 Nonce: 4

[TX #6] 🔍 Calling getSharedDataFromDB() on 0x9fe46736679d2d9a65f0992f2272de9f3c7fa6e0...
  ↳ Result: SUCCESS - Output: 0000000000000000000000000000000000000000000000000000000000000001 Gas: 8930 Nonce: 5

=======================================================
🎉 ✅ ALL TESTS PASSED! FULL WALLET & XAPIAN WORKFLOW VERIFIED.
=======================================================
[WALLET FINAL STATE] WALLET: 0xf39fd6e51aad88f6f4ce6ab8827279cfffb92266 BALANCE: 0xd3c21bcecceda1000000 NONCE: 6
```
