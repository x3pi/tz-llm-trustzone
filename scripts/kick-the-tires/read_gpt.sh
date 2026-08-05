#!/bin/bash
RKDEV=../../../rkdeveloptool/rkdeveloptool
sudo $RKDEV ld 2>/dev/null | grep -q "Maskrom"
sudo $RKDEV db ../../../tz-llm-ae/scripts/kick-the-tires/device_opi5plus_REAL/loader/MiniLoaderAll.bin
sleep 2
sudo $RKDEV cs 2
sudo $RKDEV ppt
