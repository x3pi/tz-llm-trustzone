#!/bin/bash
RKDEV=../../../rkdeveloptool/rkdeveloptool

echo "Đang chờ MaskROM..."
while true; do
    sudo $RKDEV ld 2>/dev/null | grep -q "Maskrom" && break
    sleep 1
done
echo "Đã thấy thiết bị! Nạp loader..."

while true; do
    sudo $RKDEV db ../../../tz-llm-ae/scripts/kick-the-tires/device_opi5plus_REAL/loader/MiniLoaderAll.bin
    if [ $? -eq 0 ]; then
        echo "Downloading bootloader succeeded."
        break
    else
        echo "Downloading bootloader failed. Retrying in 2 seconds..."
        sleep 2
    fi
done

sleep 2

sudo $RKDEV cs 2

echo "Flashing idblock.bin to 0x40 (SD Card)..."
while true; do
    sudo $RKDEV wl 0x40 ../../../tz-llm-ae/scripts/kick-the-tires/share/images/idblock.bin
    if [ $? -eq 0 ]; then
        echo "IDBLOCK VERIFY OK"
        break
    else
        echo "IDBLOCK FLASH FAILED! Retrying..."
        sleep 2
    fi
done

echo "Resetting device..."
sudo $RKDEV rd
echo "Hoàn tất! Hệ thống sẽ tự boot."
