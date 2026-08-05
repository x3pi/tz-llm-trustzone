#!/bin/bash
echo "Đang chờ thiết bị vào chế độ Maskrom để nạp bản fix lỗi std::stoi (share_fixed_build)..."
while true; do
    ../../rkdeveloptool/rkdeveloptool ld 2>/dev/null | grep -q "Maskrom" && break
    sleep 1
done
echo "Đã thấy thiết bị! Bắt đầu nạp..."
../../rkdeveloptool/rkdeveloptool db ../../u-boot-orangepi/rk3588_spl_loader_v1.21.114.bin
sleep 2
../../rkdeveloptool/rkdeveloptool cs 2
../../rkdeveloptool/rkdeveloptool wlx uboot share_fixed_build/images/uboot.img
../../rkdeveloptool/rkdeveloptool rd
echo "Hoàn tất! Hệ thống sẽ tự boot."
