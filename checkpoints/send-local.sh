#!/bin/bash

# Kiểm tra xem có đang chạy nhầm trên server không
if [ "$(whoami)" = "abc" ]; then
    echo "[LỖI] Bạn đang chạy script này trên Server!"
    echo "Vui lòng copy nội dung script này về lưu thành file trên Laptop của bạn (hoặc copy lệnh dán vào terminal của laptop) để kéo file về nhé."
    exit 1
fi

echo "Đang tạo thư mục trên Laptop..."
mkdir -p /home/nhat/Workspace/tz-llm-trustzone/checkpoints

echo "Đang kéo file TEE-OS và Kernel từ Server về Laptop..."
sshpass -p '1234@abcd' scp abc@192.168.1.234:/home/abc/nhat/tz-llm-trustzone/checkpoints/uboot_repacked.img /home/nhat/Workspace/tz-llm-trustzone/checkpoints/
sshpass -p '1234@abcd' scp abc@192.168.1.234:/home/abc/nhat/tz-llm-trustzone/checkpoints/boot.img /home/nhat/Workspace/tz-llm-trustzone/checkpoints/
echo "Đang kéo file evm-ca từ Server về Laptop..."
sshpass -p '1234@abcd' scp abc@192.168.1.234:/home/abc/nhat/tz-llm-trustzone/checkpoints/evm-ca /home/nhat/Workspace/tz-llm-trustzone/checkpoints/

echo "Hoàn tất! Bây giờ bạn có thể flash board được rồi."
