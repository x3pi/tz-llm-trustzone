#!/bin/bash

# Kiểm tra xem có đang chạy nhầm trên server không
if [ "$(whoami)" = "abc" ]; then
    echo "[LỖI] Bạn đang chạy script này trên Server!"
    echo "Vui lòng lưu script này lên máy Local (Laptop) của bạn và chạy nó để kéo các file ảnh (images) vừa build về."
    exit 1
fi

echo "Đang tạo thư mục chứa Xapian Images trên Laptop..."
LOCAL_DIR="/home/nhat/Workspace/tz-llm-trustzone/xapian_images"
mkdir -p "$LOCAL_DIR"

echo "Đang kéo file TEE-OS (uboot) và Kernel (boot) mới nhất từ Server về Laptop..."
sshpass -p '1234@abcd' scp abc@192.168.1.234:/home/abc/nhat/tz-llm-trustzone/scripts/kick-the-tires/images/uboot.img "$LOCAL_DIR/"
sshpass -p '1234@abcd' scp abc@192.168.1.234:/home/abc/nhat/tz-llm-trustzone/scripts/kick-the-tires/images/boot.img "$LOCAL_DIR/"
sshpass -p '1234@abcd' scp abc@192.168.1.234:/home/abc/nhat/tz-llm-trustzone/tz-llm/xapian-tee/xapian-ca/xapian-ca "$LOCAL_DIR/"

echo "Hoàn tất! Các file đã được lưu tại: $LOCAL_DIR"
echo "Bây giờ bạn có thể dùng flash.sh hoặc RKDevTool trên Laptop để flash boot.img và uboot.img, sau đó copy xapian-ca vào board."
