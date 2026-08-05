#!/bin/bash
RKDEV=../../../rkdeveloptool/rkdeveloptool

echo "Đang chờ MaskROM..."
while true; do
    sudo $RKDEV ld 2>/dev/null | grep -q "Maskrom" && break
    sleep 1
done
echo "Đã thấy thiết bị! Nạp loader..."

sudo $RKDEV db ../../../tz-llm-ae/scripts/kick-the-tires/device_opi5plus_REAL/loader/MiniLoaderAll.bin
sleep 2

sudo $RKDEV cs 2

echo "Flashing Debug U-Boot..."
sudo $RKDEV wl 0x2000 share/images/uboot.img

echo "Resetting device..."
sudo $RKDEV rd
echo "Hoàn tất! Hệ thống sẽ tự boot."
