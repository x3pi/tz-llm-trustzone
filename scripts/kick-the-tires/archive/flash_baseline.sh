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

echo "Flashing Baseline uboot..."
while true; do
    sudo $RKDEV wl 0x2000 /home/pi/reference_backup/uboot_WORKING_20260722_oppfix_ssdready.img
    sudo rm -f /tmp/verify_uboot.img
    sudo $RKDEV rl 0x2000 131072 /tmp/verify_uboot.img
    cmp /tmp/verify_uboot.img /home/pi/reference_backup/uboot_WORKING_20260722_oppfix_ssdready.img
    if [ $? -eq 0 ]; then
        echo "UBOOT VERIFY OK"
        break
    else
        echo "UBOOT VERIFY FAILED! Retrying..."
        sleep 2
    fi
done

echo "Flashing Baseline boot..."
while true; do
    sudo $RKDEV wl 0x39000 ../../../tz-llm-ae/scripts/kick-the-tires/share_hdf7/images/boot.img
    sudo rm -f /tmp/verify_boot.img
    sudo $RKDEV rl 0x39000 84327 /tmp/verify_boot.img
    cmp /tmp/verify_boot.img ../../../tz-llm-ae/scripts/kick-the-tires/share_hdf7/images/boot.img
    if [ $? -eq 0 ]; then
        echo "BOOT VERIFY OK"
        break
    else
        echo "BOOT VERIFY FAILED! Retrying..."
        sleep 2
    fi
done

echo "Resetting device..."
sudo $RKDEV rd
echo "Hoàn tất! Hệ thống sẽ tự boot."
