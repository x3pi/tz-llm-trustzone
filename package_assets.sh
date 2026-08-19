#!/bin/bash
# Script nén tất cả các file dữ liệu (Assets, ROM images, Models .gguf)
# không có mặt trên Git để tiện bàn giao dự án.

OUTPUT_FILE="tz-llm-untracked-assets.tar.gz"

echo "========================================================="
echo " Đang quét các file dữ liệu không có trên Git..."
echo "========================================================="

# Lấy danh sách toàn bộ các file bị Git ignore hoặc chưa được track
# Lọc lấy các định dạng .img, .gguf, .bin
git ls-files --ignored --exclude-standard --others | grep -E '\.(img|gguf|bin)$' > /tmp/untracked_list.txt

# In ra những gì sẽ được nén
echo "Các file sẽ được nén bao gồm:"
cat /tmp/untracked_list.txt | sed 's/^/ - /'
echo ""

echo "Bắt đầu nén (quá trình này có thể mất nhiều phút do có file hàng GB)..."
# Tạo file nén (sử dụng gzip để giảm dung lượng file ROM)
tar -czvf "$OUTPUT_FILE" -T /tmp/untracked_list.txt

rm -f /tmp/untracked_list.txt

echo "========================================================="
echo " HOÀN TẤT!"
echo " Đã đóng gói toàn bộ dữ liệu vào: $OUTPUT_FILE"
echo " Kích thước file nén:"
du -sh "$OUTPUT_FILE"
echo " Bạn có thể chép file này sang USB hoặc ổ cứng ngoài để bàn giao."
echo "========================================================="
