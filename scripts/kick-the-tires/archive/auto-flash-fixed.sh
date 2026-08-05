#!/bin/bash
RKDEV=../../../rkdeveloptool/rkdeveloptool

echo "Đang chờ thiết bị vào chế độ Maskrom..."
while true; do
    sudo $RKDEV ld 2>/dev/null | grep -q "Maskrom" && break
    sleep 1
done
echo "Đã thấy thiết bị! Bắt đầu nạp lại bản đã fix (share)..."

sudo $RKDEV db ../../../tz-llm-ae/scripts/kick-the-tires/device_opi5plus_REAL/loader/MiniLoaderAll.bin
sleep 2

sudo $RKDEV cs 2

echo "Flashing U-Boot..."
sudo $RKDEV wl 0x2000 share/images/uboot.img

echo "Flashing Boot Linux..."
sudo $RKDEV wl 0x39000 share/images/boot.img

echo "Resetting device..."
sudo $RKDEV rd
echo "Hoàn tất! Hệ thống sẽ tự boot."
