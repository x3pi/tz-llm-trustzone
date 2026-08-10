#!/bin/bash
# Flash vendor.img ONLY, with the exact same per-chunk write+verify
# mechanism as flash.sh (see that file's comment for why: the USB/MaskROM
# channel intermittently bit-flips). Deliberately separate from flash.sh's
# routine uboot+boot_linux flow -- vendor is a partition we rarely touch,
# this is a one-off, explicit action (2026-08-09: baking in the
# llm_daemon auto-start service added to init.opi5p.usb.cfg).
#
# Board must be in MaskROM mode before running this.
set -e
cd "$(dirname "$0")/.."
RKDEV=$(command -v rkdeveloptool || echo ./tools/rkdeveloptool/rkdeveloptool)
LOADER=scripts/kick-the-tires/device_opi5plus_REAL/loader/MiniLoaderAll.bin
VENDOR=${1:-scripts/kick-the-tires/share_build/images/vendor.img}
SUDO_PW=${SUDO_PW:-}
W=$(mktemp -d)
trap 'rm -rf "$W"' EXIT

sudo_run() {
    if "$@" 2>/dev/null; then return 0; fi
    if [ -n "$SUDO_PW" ]; then echo "$SUDO_PW" | sudo -S "$@"; else sudo "$@"; fi
}

echo "=== entering loader mode ==="
sudo_run timeout 30s "$RKDEV" db "$LOADER" || true
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

# Fixed constant from assets/full-flash/parameter_custom.txt:
# 0x00200000@0x004BA000(vendor) -- never changes between cards.
VENDOR_LBA=0x4BA000
echo "resolved: vendor @ $VENDOR_LBA"

write_verified vendor "$VENDOR" "$VENDOR_LBA"

echo "=== resetting board ==="
sudo_run "$RKDEV" rd
echo "Done. Board needs a physical power cycle if it does not boot on its own."
