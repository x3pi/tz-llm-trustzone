#!/bin/bash
# Diagnostic test: does re-entering loader mode (fresh db+cs handshake)
# partway through the uboot write avoid the chronic chunk-62 failure?
# If a per-session cumulative-transfer limit (~32MB) in rkdeveloptool or the
# MaskROM loader is the real cause, restarting the session before chunk 62
# should let it write cleanly since the new session starts its own byte count
# back at zero.
set -e
cd "$(dirname "$0")/.."
RKDEV=$(command -v rkdeveloptool || echo ./tools/rkdeveloptool/rkdeveloptool)
LOADER=scripts/kick-the-tires/device_opi5plus_REAL/loader/MiniLoaderAll.bin
IDBLOADER=${1:?usage: flash-split-session-test.sh <idbloader.bin> <uboot_repacked.img> <boot.img>}
UBOOT=${2:?}
BOOT=${3:?}
SUDO_PW=${SUDO_PW:-}
W=$(mktemp -d)
trap 'rm -rf "$W"' EXIT

sudo_run() { if [ -n "$SUDO_PW" ]; then echo "$SUDO_PW" | sudo -S "$@"; else sudo "$@"; fi; }

enter_loader() {
    echo "=== (re)entering loader mode ==="
    sudo_run timeout 8s "$RKDEV" db "$LOADER"
    sleep 2
    sudo_run "$RKDEV" cs 2
    sleep 2
}

write_verified_range() {
    # write_verified_range partname file base_lba chunk_sectors start_chunk end_chunk
    local partname=$1 file=$2 base_lba=$3 chunk_sectors=$4 start_chunk=$5 end_chunk=$6
    split -b $((chunk_sectors*512)) -d -a3 "$file" "$W/${partname}_"
    local i=0
    for f in "$W/${partname}_"*; do
        if [ "$i" -lt "$start_chunk" ] || { [ -n "$end_chunk" ] && [ "$i" -ge "$end_chunk" ]; }; then
            i=$((i+1)); continue
        fi
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
CHUNK_SECTORS=1024

echo "### Session 1: idbloader + uboot chunks 0-49 ###"
enter_loader
write_verified_range idbloader "$IDBLOADER" "$IDBLOADER_LBA" "$CHUNK_SECTORS" 0 ""
write_verified_range uboot "$UBOOT" "$UBOOT_LBA" "$CHUNK_SECTORS" 0 50

echo "### Session 2 (FRESH handshake): uboot chunks 50 onward (includes the chronic chunk 62) ###"
enter_loader
write_verified_range uboot "$UBOOT" "$UBOOT_LBA" "$CHUNK_SECTORS" 50 ""

echo "### Session 3 (FRESH handshake): boot_linux ###"
enter_loader
write_verified_range boot_linux "$BOOT" "$BOOT_LBA" "$CHUNK_SECTORS" 0 ""

echo "=== resetting board ==="
sudo_run "$RKDEV" rd
echo "Done."
