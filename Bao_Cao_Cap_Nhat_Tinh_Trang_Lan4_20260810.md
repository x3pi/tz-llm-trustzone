# Báo cáo Cập nhật Tình trạng Hệ thống và Sửa Lỗi IDLE BACKOFF (Lần 4)

**Ngày:** 2026-08-10
**Vấn đề chính:** Khắc phục tình trạng "treo" board (Starvation, RCU stall, soft-lockup) khi hệ thống rảnh rỗi chờ request.

Báo cáo này tổng hợp những nguyên nhân và thay đổi kỹ thuật (IDLE BACKOFF) đã được thực hiện để giải quyết dứt điểm các lỗi làm sập hệ thống (bao gồm cả hiện tượng `usb_host` crash loop, mất kết nối WiFi và UART bị flood) khi daemon `fake` chạy vòng lặp chờ HTTP request.

## 1. Phân tích nguyên nhân gốc rễ (Root Cause)

Với kiến trúc mới (HTTP daemon đa request), TA (chạy trong TrustZone) không thoát ngay sau một câu hỏi mà lặp lại việc chờ (`usys_tee_wait_switch_req()`). Tuy nhiên, cơ chế kết nối từ Normal World (CA) sang Secure World (TA) không được thiết kế cho việc "nằm chờ" này:

1. **SMC Spinning ở Kernel (tc_client_driver.c):** Khi TA park (ngủ) chờ request, `smc_call_cpu_resume` liên tục nhận về `SMC_EXIT_PREEMPTED` và lặp lại việc gọi `do_smc_transport()` ở tốc độ tối đa mà không hề sleep.
2. **User-space Spinning (fake_ca.cpp):** Các luồng `ca_thread` gọi ioctl `LLM_CLIENT_IOCTL_RUN` hàng triệu lần mỗi giây. Do một lỗi truyền tham số `ioctl()` (thừa biến `fd`), cờ trạng thái `out_cmd` không bao giờ được ghi về userspace, khiến `ca_thread` không biết TA đang rảnh rỗi và liên tục busy-spin.
3. **UART Overhead (layer-sched.cpp):** Log `[TZLLM_TRACE]` in ra khi hàng đợi trống (mỗi khi rảnh rỗi) gây I/O blocking nặng nề trên UART, góp phần trực tiếp gây ra RCU stall.
4. **Active-Compute Spinning (prefetch.cpp):** Khi chờ I/O / Decrypt tensor, các luồng tính toán liên tục poll mà không chịu `yield`, vắt kiệt CPU.

Tất cả các yếu tố trên cộng dồn tạo ra tải giả ~400% CPU, cắt đứt tài nguyên của toàn bộ hệ thống (gây rớt WiFi, `usb_host` sập, và lockup kernel).

## 2. Các thay đổi và khắc phục (IDLE BACKOFF)

Toàn bộ các bản vá đã được áp dụng vào source code để giải quyết từng vấn đề:

### 2.1. Kernel Driver (`tc_client_driver.c`)
- **IDLE BACKOFF part 1:** Thêm biến đếm `consecutive_preempts`. Nếu TA liên tục trả về `SMC_EXIT_PREEMPTED` mà không có tiến triển thực sự (>3000 vòng), driver sẽ chủ động gọi `usleep_range(500, 2000)` thay vì gọi SMC ở tốc độ tối đa.
- **IDLE BACKOFF part 2:** Trả về mã cờ `SMC_LOOP_EXIT_FINISH` một cách tường minh cho Userspace qua `copy_to_user` khi TA đang thực sự ngủ, giúp `ca_thread` nhận biết trạng thái idle.

### 2.2. CA Relay Threads (`fake_ca.cpp`)
- **Sửa lỗi ioctl:** Đổi `ioctl(fd, LLM_CLIENT_IOCTL_RUN, fd, &out_cmd)` thành `ioctl(fd, LLM_CLIENT_IOCTL_RUN, &out_cmd)`. Lỗi cũ khiến cờ trả về bị rơi vào thinh không.
- **Idle Sleep:** Đọc cờ `SMC_LOOP_EXIT_FINISH` từ Kernel. Nếu nhận cờ này liên tục, luồng CA sẽ `usleep(1000)` để giải phóng CPU.
- **Watchdog cho Warmup:** Bổ sung watchdog 300 giây (`watchdog_wait_for_answer`) cho quá trình warmup. Nếu TA chết giữa chừng lúc load model, CA sẽ tự động reboot thay vì treo vĩnh viễn.

### 2.3. Hàng đợi và Scheduler (`layer-sched.cpp` & `prefetch.cpp`)
- **Ring Buffer (Lock-free) cho Log:** Thay thế lệnh `printf` (gây nghẽn UART) bằng cấu trúc ring buffer nội bộ (`dbg_log_push_idle`). Người dùng chỉ cần gửi tín hiệu `SIGUSR1` để dump log ra khi cần (`dbg_log_idle_dump`).
- **Thêm `usys_yield()`:** Bổ sung `usys_yield()` vào vòng lặp chờ load tensor (trong `prefetch.cpp`), giúp nhường CPU khi đang đợi I/O bất đồng bộ.

## 3. Tình trạng kiểm chứng
- Các file source đã được sửa đổi và build thành image mới (`boot.img`, `uboot_repacked.img`).
- Đã cập nhật file `DEPLOYED_STATE.md` để lưu lại những nỗ lực debug này.
- **Hiện tại:** Đang chờ người dùng reset lại cáp UART/Board để boot và test thực tế sức chịu đựng của các bản vá này trên phần cứng.
