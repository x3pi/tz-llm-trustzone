#!/bin/bash
# Like flash.sh (uboot + boot_linux only, existing GPT untouched), but also
# swaps the idbloader at LBA 0x40 first. For testing an idbloader variant
# (e.g. a from-source SPL build) without doing a full flash-full.sh wipe of
# GPT/system/vendor/userdata. Board must already have a valid GPT from a
# prior flash-full.sh run -- this does not write one.
set -e
cd "$(dirname "$0")/.."
RKDEV=$(command -v rkdeveloptool || echo ./tools/rkdeveloptool/rkdeveloptool)
LOADER=scripts/kick-the-tires/device_opi5plus_REAL/loader/MiniLoaderAll.bin
IDBLOADER=${1:?usage: flash-with-idbloader.sh <idbloader.bin> [uboot_repacked.img] [boot.img]}
UBOOT=${2:-checkpoints/uboot_repacked.img}
BOOT=${3:-checkpoints/boot.img}
SUDO_PW=${SUDO_PW:-}
W=$(mktemp -d)
trap 'rm -rf "$W"' EXIT

sudo_run() { if [ -n "$SUDO_PW" ]; then echo "$SUDO_PW" | sudo -S "$@"; else sudo "$@"; fi; }

echo "=== entering loader mode ==="
sudo_run timeout 8s "$RKDEV" db "$LOADER"
sleep 2
sudo_run "$RKDEV" cs 2
sleep 2

write_verified() {
    local partname=$1 file=$2 base_lba=$3
    local size sectors chunk_sectors=1024 i=0
    size=$(stat -c%s "$file")
    sectors=$(( (size + 511) / 512 ))
    echo "--- $partname: $size bytes, $sectors sectors @ LBA 0x$(printf %x $base_lba) ---"
    split -b $((chunk_sectors*512)) -d -a3 "$file" "$W/${partname}_"
    for f in "$W/${partname}_"*; do
        local lba=$((base_lba + i*chunk_sectors))
        local csize; csize=$(stat -c%s "$f")
        local csect=$(( (csize+511)/512 ))
        local expect; expect=$(sha256sum "$f" | cut -d' ' -f1)
        local attempt ok=0
        for attempt in 1 2 3 4 5; do
            sudo_run timeout 20s "$RKDEV" wl "$(printf 0x%x "$lba")" "$f" >/dev/null
            sleep 1
            ok=0
            local n
            for n in 1 2 3; do
                sleep 1
                sudo_run timeout 20s "$RKDEV" rl "$(printf 0x%x "$lba")" "$csect" "$W/verify.bin" >/dev/null
                local got; got=$(head -c "$csize" "$W/verify.bin" | sha256sum | cut -d' ' -f1)
                [ "$got" = "$expect" ] && ok=$((ok+1))
            done
            if [ "$ok" -ge 2 ]; then
                echo "  chunk $i: OK ($ok/3) attempt $attempt"
                break
            fi
            echo "  chunk $i: FAILED verify ($ok/3), retrying write (attempt $attempt)"
        done
        [ "$ok" -ge 2 ] || { echo "*** chunk $i unrecoverable after 5 attempts ***"; exit 1; }
        i=$((i+1))
    done
}

IDBLOADER_LBA=0x40
UBOOT_LBA=0x2000
BOOT_LBA=0x88000

write_verified idbloader "$IDBLOADER" "$IDBLOADER_LBA"
write_verified uboot "$UBOOT" "$UBOOT_LBA"
write_verified boot_linux "$BOOT" "$BOOT_LBA"

echo "=== resetting board ==="
sudo_run "$RKDEV" rd
echo "Done. Board needs a physical power cycle if it does not boot on its own."
