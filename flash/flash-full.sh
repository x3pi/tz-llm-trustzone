#!/bin/bash
# Full initialization of a BLANK SD card: GPT + uboot + boot_linux + system +
# vendor + userdata. Use this only for a card that has never had this
# project's OpenHarmony/TZ-LLM image on it. For a card that already has one
# (i.e. you're just updating after a TEE-OS/kernel rebuild), use flash.sh
# instead -- it only touches uboot+boot_linux and is much faster.
#
# Loader: this uses rk3588_spl_loader_v1.21.114.bin (NOT
# device_opi5plus_REAL/loader/MiniLoaderAll.bin, which is what flash.sh and
# repack.sh use) -- this is the loader combination that produced the
# reference "ALL_PERFECT" full flash historically. Do not mix the two.
#
# uboot.img and boot_linux (small, hash-checked by the SPL/U-Boot at boot,
# so a single bad byte hard-fails boot) get the same chunked write + 3x
# majority-vote readback verification as flash.sh, given the confirmed
# intermittently-corrupting USB/MaskROM channel (see STATUS.md). system /
# vendor / userdata (large filesystem images, 268MB-1.6GB) only get a light
# spot-check (first/middle/last 4MiB) -- full verification of ~2.9GB over
# this channel is not practical time-wise. A corrupt block in a filesystem
# image is also less likely to be fatal than in uboot/boot_linux; f2fs/ext4
# fsck at first real boot is the actual end-to-end check for those.
set -e
cd "$(dirname "$0")/.."
RKDEV=$(command -v rkdeveloptool || echo ./tools/rkdeveloptool/rkdeveloptool)
LOADER=assets/full-flash/rk3588_spl_loader_v1.21.114.bin
PARAM=assets/full-flash/parameter_custom.txt
UBOOT=${1:-checkpoints/uboot_repacked.img}
BOOT=${2:-checkpoints/boot.img}
SYSTEM=assets/full-flash/system_real.img
VENDOR=assets/full-flash/vendor_real.img
USERDATA=assets/full-flash/userdata.img
# Sectors 64-8191 (up to but excluding the "uboot" GPT partition at LBA
# 0x2000) extracted whole from a card that has run this exact project's
# GPT/uboot layout successfully since the beginning -- NOT hand-built. See
# STATUS.md "Blank SD card: idbloader" for the full story: a hand-built
# idblock.bin (mkimage -T rksd from rkbin's generic RK3588MINIALL.ini
# FlashBoot/FlashData) and even device_opi5plus_REAL/loader/MiniLoaderAll.bin
# written directly both leave the board silently hung after power-on (no
# UART output at all, not even the ROM's own DDR-init banner, so this is a
# very early hang, not a clean BootROM rejection) -- both are believed to be
# "USB-download-mode-only" loaders, not real cold-boot-capable ones. This
# extracted region is UNTESTED as of this writing (was pulled from the
# working card right as we switched back to it, not yet flashed+verified on
# a blank card) -- confirm this actually boots before trusting it blindly.
IDBLOADER=assets/full-flash/idblock_from_working_card.bin
SUDO_PW=${SUDO_PW:-}
W=$(mktemp -d)
trap 'rm -rf "$W"' EXIT

sudo_run() { if [ -n "$SUDO_PW" ]; then echo "$SUDO_PW" | sudo -S "$@"; else sudo "$@"; fi; }

echo "=== entering loader mode (rk3588_spl_loader_v1.21.114.bin) ==="
sudo_run timeout 8s "$RKDEV" db "$LOADER"
sleep 2
sudo_run "$RKDEV" cs 2
sleep 3

echo "=== writing GPT from $PARAM ==="
sudo_run "$RKDEV" gpt "$PARAM"
sleep 2

write_verified() {
    local partname=$1 file=$2 base_lba=$3
    local size sectors chunk_sectors=8192 i=0
    size=$(stat -c%s "$file")
    sectors=$(( (size + 511) / 512 ))
    echo "--- $partname (verified): $size bytes, $sectors sectors @ LBA 0x$(printf %x $base_lba) ---"
    split -b $((chunk_sectors*512)) -d -a3 "$file" "$W/${partname}_"
    for f in "$W/${partname}_"*; do
        local lba=$((base_lba + i*chunk_sectors))
        local csize; csize=$(stat -c%s "$f")
        local csect=$(( (csize+511)/512 ))
        local expect; expect=$(sha256sum "$f" | cut -d' ' -f1)
        local ok=0 attempt
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
            [ "$ok" -ge 2 ] && { echo "  chunk $i: OK ($ok/3) attempt $attempt"; break; }
            echo "  chunk $i: FAILED verify ($ok/3), retrying write (attempt $attempt)"
        done
        [ "$ok" -ge 2 ] || { echo "*** chunk $i unrecoverable after 5 attempts ***"; exit 1; }
        i=$((i+1))
    done
}

write_spotcheck() {
    local partname=$1 file=$2 base_lba=$3
    local size sectors
    size=$(stat -c%s "$file")
    sectors=$(( (size + 511) / 512 ))
    echo "--- $partname (spot-check only): $size bytes, $sectors sectors @ LBA 0x$(printf %x $base_lba) ---"
    sudo_run timeout 600s "$RKDEV" wl "$(printf 0x%x "$base_lba")" "$file"
    sleep 2
    local spots=("0:head" "$(( (sectors/2)*512 )):mid" "$(( (sectors-8192)*512 )):tail")
    local spot label off_bytes off_lba
    for spot in "${spots[@]}"; do
        off_bytes=${spot%%:*}; label=${spot##*:}
        off_lba=$((base_lba + off_bytes/512))
        local expect; expect=$(dd if="$file" bs=512 skip=$((off_bytes/512)) count=8192 2>/dev/null | sha256sum | cut -d' ' -f1)
        sudo_run timeout 20s "$RKDEV" rl "$(printf 0x%x "$off_lba")" 8192 "$W/spot.bin" >/dev/null
        local got; got=$(sha256sum "$W/spot.bin" | cut -d' ' -f1)
        [ "$got" = "$expect" ] && echo "  $label: OK" || echo "  $label: *** MISMATCH -- consider re-running this partition ***"
    done
}

IDBLOADER_LBA=0x40
UBOOT_LBA=0x2000
BOOT_LBA=0x88000
SYSTEM_LBA=0xBA000
VENDOR_LBA=0x4BA000
USERDATA_LBA=0x1308000

write_verified idbloader "$IDBLOADER" "$IDBLOADER_LBA"
write_verified uboot "$UBOOT" "$UBOOT_LBA"
write_verified boot_linux "$BOOT" "$BOOT_LBA"
write_spotcheck system "$SYSTEM" "$SYSTEM_LBA"
write_spotcheck vendor "$VENDOR" "$VENDOR_LBA"
write_spotcheck userdata "$USERDATA" "$USERDATA_LBA"

echo "=== resetting board ==="
sudo_run "$RKDEV" rd
echo "Done. Board needs a physical power cycle if it does not boot on its own."
