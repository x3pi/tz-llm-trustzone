#!/bin/bash
echo "Đang chờ thiết bị vào chế độ Maskrom..."
while true; do
    ./rkdeveloptool/rkdeveloptool ld 2>/dev/null | grep -q "Maskrom" && break
    sleep 1
done
echo "Đã thấy thiết bị! Bắt đầu nạp lại bản đã fix (share_hdf)..."
./rkdeveloptool/rkdeveloptool db u-boot-orangepi/rk3588_spl_loader_v1.21.114.bin
sleep 2
./rkdeveloptool/rkdeveloptool cs 2
./rkdeveloptool/rkdeveloptool wlx uboot tz-llm-ae/scripts/kick-the-tires/share_hdf/images/uboot.img
./rkdeveloptool/rkdeveloptool wlx boot_linux tz-llm-ae/scripts/kick-the-tires/share_hdf/images/boot.img
./rkdeveloptool/rkdeveloptool rd
echo "Hoàn tất! Hệ thống sẽ tự boot."
