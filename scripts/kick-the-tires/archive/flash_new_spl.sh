#!/bin/bash
RKDEV=../../../rkdeveloptool/rkdeveloptool

echo "Nạp loader..."
sudo $RKDEV db ../../../tz-llm-ae/scripts/kick-the-tires/device_opi5plus_REAL/loader/MiniLoaderAll.bin
sleep 2

sudo $RKDEV cs 2

echo "Flashing NEW SPL to Sector 64..."
sudo $RKDEV wl 0x40 ../../../u-boot-orangepi/rk3588_spl_loader_v1.21.114.bin

echo "Resetting device..."
sudo $RKDEV rd
