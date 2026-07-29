#!/bin/bash
# Flash uboot + boot_linux with real per-chunk verification.
#
# The USB/MaskROM channel on this setup is intermittently bit-flipping,
# confirmed repeatedly on 2026-07-29: rkdeveloptool reports "successfully"
# on writes that later fail the on-device FIT hash check. flash_with_retry-
# style scripts that only check rkdeveloptool's exit code do NOT catch this.
# This script writes in 4MiB chunks and reads each one back before moving on;
# a single bad chunk only costs a 4MiB retry, not a 40-90MiB one. Even reads
# are flaky (~10-20% single-shot corruption rate observed), so writes are
# verified with a majority-of-3 vote, not a single readback.
#
# Board must be in MaskROM mode before running this (hold the MaskROM button
# while powering on, or `reboot loader` over an existing shell/UART session).
set -e
cd "$(dirname "$0")/.."
RKDEV=$(command -v rkdeveloptool || echo ./tools/rkdeveloptool/rkdeveloptool)
LOADER=scripts/kick-the-tires/device_opi5plus_REAL/loader/MiniLoaderAll.bin
UBOOT=${1:-checkpoints/uboot_repacked.img}
BOOT=${2:-checkpoints/boot.img}
SUDO_PW=${SUDO_PW:-}
W=$(mktemp -d)
trap 'rm -rf "$W"' EXIT

sudo_run() { if [ -n "$SUDO_PW" ]; then echo "$SUDO_PW" | sudo -S "$@"; else sudo "$@"; fi; }

echo "=== entering loader mode ==="
sudo_run timeout 8s "$RKDEV" db "$LOADER"
sleep 2
sudo_run "$RKDEV" cs 2
sleep 2

echo "=== reading live GPT (sanity check only -- LBAs below are the fixed"
echo "    constants from assets/full-flash/parameter_custom.txt, which never"
echo "    change between cards; only userdata's size/end varies) ==="
sudo_run "$RKDEV" ppt || true

write_verified() {
    local partname=$1 file=$2 base_lba=$3
    local size sectors chunk_sectors=8192 i=0
    size=$(stat -c%s "$file")
    sectors=$(( (size + 511) / 512 ))
    echo "--- $partname: $size bytes, $sectors sectors @ LBA 0x$(printf %x $base_lba) ---"
    split -b $((chunk_sectors*512)) -d -a3 "$file" "$W/${partname}_"
    for f in "$W/${partname}_"*; do
        local lba=$((base_lba + i*chunk_sectors))
        local csize; csize=$(stat -c%s "$f")
        local csect=$(( (csize+511)/512 ))
        local expect; expect=$(sha256sum "$f" | cut -d' ' -f1)
        local attempt
        for attempt in 1 2 3 4 5; do
            sudo_run timeout 20s "$RKDEV" wl "$(printf 0x%x "$lba")" "$f" >/dev/null
            sleep 1
            local ok=0 n
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

# Fixed constants from parameter_custom.txt -- same ones flash-full.sh uses.
# Do NOT switch back to parsing `ppt` output dynamically: rkdeveloptool's
# `ppt` formatting is fragile to parse (a stray '\r' silently broke a
# previous version of this script's awk match on "uboot", which produced an
# EMPTY LBA and wrote 64MB starting at LBA 0x0 -- clobbering the MBR/GPT and
# the idbloader at LBA 0x40). These offsets never change between cards.
UBOOT_LBA=0x2000
BOOT_LBA=0x88000
echo "resolved: uboot @ $UBOOT_LBA, boot_linux @ $BOOT_LBA"

write_verified uboot "$UBOOT" "$UBOOT_LBA"
write_verified boot_linux "$BOOT" "$BOOT_LBA"

echo "=== resetting board ==="
sudo_run "$RKDEV" rd
echo "Done. Board needs a physical power cycle if it does not boot on its own."
