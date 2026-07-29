#!/bin/bash
CURRENT_DIR=$(realpath $(dirname $0))
ROOT_DIR=$(realpath $CURRENT_DIR/../..)
FLASH_DIR=$ROOT_DIR/flash-proxy/
SHARE_DIR=$CURRENT_DIR/share

echo "Đang nạp lại boot.img và uboot.img..."
cd $FLASH_DIR
sudo ./upgrade_tool di -b $SHARE_DIR/images/boot.img
sudo ./upgrade_tool di -u $SHARE_DIR/images/uboot.img
echo "Khởi động lại mạch..."
sudo ./upgrade_tool rd
echo "Hoàn tất!"
