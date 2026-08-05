#!/bin/bash
# Single entry point that rebuilds AND deploys everything from the CURRENT
# source tree in one atomic pass: TEE-OS+kernel (rebuild.sh), the CA-side
# userspace binaries (build-llama.sh), repacks uboot_repacked.img, flashes
# uboot+boot_linux (flash.sh), and pushes the freshly-built CA binaries over
# hdc -- so it's never possible to end up with a TEE-OS build and a CA build
# from two different points in the source tree's history (the exact
# confusion a 2026-08-05 session lost significant time chasing as a
# suspected NPU bug before realizing it was a version-skew question).
#
# Does NOT touch idbloader/GPT/vendor/system/userdata -- use
# flash/flash-full.sh directly (and read DEPLOYED_STATE.md's warning about
# MiniLoaderAll_official.bin first) if those need to change too.
#
# Usage: SUDO_PW=... BOARD_IP=192.168.1.224 ./flash/deploy-all.sh
set -e
cd "$(dirname "$0")/.."
BOARD_IP=${BOARD_IP:-192.168.1.224}
BOARD_HDC_PORT=${BOARD_HDC_PORT:-8710}

echo "=== [1/5] rebuild.sh: kernel + TEE-OS ==="
./rebuild.sh

echo "=== [2/5] repack.sh: checkpoints/uboot_repacked.img ==="
./flash/repack.sh

# repack.sh only handles uboot.img (the FIT with optee/TEE-OS embedded).
# boot.img (Linux kernel + tzdriver.ko + ramdisk) is a plain copy with no
# repacking step -- rebuild.sh's own output at
# scripts/kick-the-tires/share_build/images/boot.img was NEVER copied into
# checkpoints/boot.img automatically. A 2026-08-05 session lost real time
# to this: flash/flash.sh kept flashing an Aug-4 boot.img while believing a
# same-day kernel/tzdriver source fix (TZASC_TOTAL_MEM_SIZE) was live.
echo "=== [2b/5] copying freshly-built boot.img into checkpoints/ ==="
cp scripts/kick-the-tires/share_build/images/boot.img checkpoints/boot.img

echo "=== [3/5] build-llama.sh: CA-side userspace binaries (same source tree) ==="
./scripts/kick-the-tires/build-llama.sh

echo "=== [4/5] flash.sh: uboot@0x2000 + boot_linux@0x88000 (board must be in MaskROM now) ==="
./flash/flash.sh

echo "=== Board needs a physical power cycle now (not MaskROM) before step 5 ==="
read -p "Press Enter once the board has finished booting and hdc is reachable... "

echo "=== [5/5] pushing freshly-built CA binaries over hdc (avoids the version-skew bug) ==="
hdc tconn "$BOARD_IP:$BOARD_HDC_PORT" || true
for f in fake libggml.so libllama.so libremoting_backend.so llama-cli; do
    hdc -t "$BOARD_IP:$BOARD_HDC_PORT" file send "scripts/kick-the-tires/share/build-rknpure/$f" "/data/ssd/rknpu/$f"
done

echo "=== Done. Update DEPLOYED_STATE.md by hand with today's date + hashes. ==="
