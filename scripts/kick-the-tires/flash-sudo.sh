#!/bin/bash
set -e

CURRENT_DIR=$(realpath $(dirname $0))
ROOT_DIR=$(realpath $CURRENT_DIR/../..)
FLASH_DIR=$ROOT_DIR/flash-proxy/
SHARE_DIR=$CURRENT_DIR/share

hdc shell reboot loader
sleep 5

sudo ./upgrade_tool di -b $SHARE_DIR/images/boot.img
sudo ./upgrade_tool di -u $SHARE_DIR/images/uboot.img
sudo ./upgrade_tool rd
