#!/bin/bash
echo "Đang chờ thiết bị vào chế độ Maskrom để khôi phục Baseline..."
while true; do
    ./rkdeveloptool/rkdeveloptool ld 2>/dev/null | grep -q "Maskrom" && break
    sleep 1
done
echo "Đã thấy thiết bị! Bắt đầu nạp lại Baseline known-good..."
./rkdeveloptool/rkdeveloptool db u-boot-orangepi/rk3588_spl_loader_v1.21.114.bin
sleep 2
./rkdeveloptool/rkdeveloptool cs 2
# KHÔNG flash uboot nữa để tránh rủi ro, nhưng theo tài liệu baseline thì có thể flash cả 2
./rkdeveloptool/rkdeveloptool wlx uboot /home/pi/reference_backup/uboot_WORKING_20260722_oppfix_ssdready.img
./rkdeveloptool/rkdeveloptool wlx boot_linux /home/pi/reference_backup/boot_linux_5max_WORKING_20260722_oppfix_ssdready.img
./rkdeveloptool/rkdeveloptool rd
echo "Hoàn tất! Hệ thống sẽ tự boot."
