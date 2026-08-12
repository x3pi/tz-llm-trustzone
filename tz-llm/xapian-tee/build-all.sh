#!/bin/bash
set -e

echo "==================================================="
echo "🚀 BẮT ĐẦU QUÁ TRÌNH BUILD TOÀN BỘ XAPIAN TEE 🚀"
echo "==================================================="

# 1. Chuyển về thư mục gốc của project
cd "$(dirname "$0")/../../"
PROJECT_ROOT=$(pwd)

echo "--- [1/4] Đang biên dịch Xapian CA và TA ---"
./scripts/kick-the-tires/xapian-builder.sh bash -x build-ca-ta.sh

echo "--- [2/4] Sao chép TA vào thư mục Build của Firmware ---"
cp tz-llm/xapian-tee/xapian-ta/build/xapian-ta scripts/kick-the-tires/xapian-ta

echo "--- [3/4] Biên dịch TEE OS (Firmware) ---"
cd scripts/kick-the-tires
./oh-builder.sh . bash -x /home/vectorxj/share/build-oh-docker.sh
cd ../../

echo "--- [4/4] Đóng gói U-Boot Repacked Image ---"
./flash/repack.sh scripts/kick-the-tires/images/uboot.img
cp scripts/kick-the-tires/images/boot.img checkpoints/boot.img

echo "==================================================="
echo "✅ BUILD THÀNH CÔNG!"
echo "Các file kết quả được lưu tại:"
echo "- xapian-ca (Linux): $PROJECT_ROOT/tz-llm/xapian-tee/checkpoints/xapian-ca"
echo "- uboot_repacked.img: $PROJECT_ROOT/checkpoints/uboot_repacked.img"
echo "- boot.img: $PROJECT_ROOT/checkpoints/boot.img"
echo "==================================================="
