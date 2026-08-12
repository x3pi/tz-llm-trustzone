mkdir -p /dev/shm

set -e
LD_LIBRARY_PATH=/data/local/tmp/rknpu/ taskset -a 2 /data/local/tmp/rknpu/ld-linux-aarch64.so.1 /data/local/tmp/rknpu/ca

sleep 1

LD_LIBRARY_PATH=/data/local/tmp/rknpu/ /data/local/tmp/rknpu/ld-linux-aarch64.so.1 /data/local/tmp/rknpu/llama-cli -m /data/local/tmp/Qwen2-1.5B-Instruct.fp16.gguf --no-warmup --file /data/local/tmp/gsm8k-question.txt -n 2 -s 1 -ngl 100 -t 4 --no-mmap
