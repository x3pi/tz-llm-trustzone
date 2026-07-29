#!/bin/bash
echo "Đang chờ thiết bị vào chế độ Maskrom để wipe phân vùng userdata bị lỗi..."
while true; do
    ../../rkdeveloptool/rkdeveloptool ld 2>/dev/null | grep -q "Maskrom" && break
    sleep 1
done
echo "Đã thấy thiết bị! Bắt đầu nạp file trống để xóa userdata..."
../../rkdeveloptool/rkdeveloptool db ../../u-boot-orangepi/rk3588_spl_loader_v1.21.114.bin
sleep 2
../../rkdeveloptool/rkdeveloptool cs 2

# Tạo file trống 1MB
dd if=/dev/zero of=/tmp/empty_1m.img bs=1M count=1 2>/dev/null
../../rkdeveloptool/rkdeveloptool wlx userdata /tmp/empty_1m.img

../../rkdeveloptool/rkdeveloptool rd
echo "Hoàn tất! Hệ thống sẽ tự boot và tự format lại /data."
