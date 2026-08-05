#!/bin/bash
RKDEV=../../../rkdeveloptool/rkdeveloptool

echo "Nạp loader..."
sudo $RKDEV db ../../../tz-llm-ae/scripts/kick-the-tires/device_opi5plus_REAL/loader/MiniLoaderAll.bin
sleep 2

sudo $RKDEV cs 2

echo "Flashing GOLDEN SPL to Sector 64..."
sudo $RKDEV wl 0x40 /tmp/golden_idbloader.img

echo "Resetting device..."
sudo $RKDEV rd
