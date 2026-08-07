#!/bin/bash
# Emergency recovery: restore idbloader+GPT+system+vendor from the known-good
# golden-image backup, then reflash the current (fixed) uboot/boot_linux on
# top. Use this when the board won't boot / sits silent on UART / USB keeps
# re-enumerating after a bad flash -- most commonly after running
# flash-full.sh with FORCE_USERDATA=1 against a board that had real working
# userdata (see DEPLOYED_STATE.md / CLAUDE.md, incident 2026-08-07).
#
# This does NOT touch the userdata partition at all (the golden-image backup
# stops right after vendor) -- if userdata itself is what's corrupted/wiped,
# this script alone won't fix that; you'll get a board that boots but starts
# from a blank/first-boot userdata state (no `/data/ssd` dir yet -- mkdir -p
# it before mounting the NVMe SSD, and re-push CA binaries after boot).
#
# Board must be in MaskROM mode before running this.
set -e
cd "$(dirname "$0")/.."
RKDEV=$(command -v rkdeveloptool || echo ./tools/rkdeveloptool/rkdeveloptool)
LOADER=assets/full-flash/rk3588_spl_loader_v1.21.114.bin
IMG=checkpoints/golden-image/idbloader_through_vendor.img
SUDO_PW=${SUDO_PW:-}

sudo_run() {
    if "$@" 2>/dev/null; then return 0; fi
    if [ -n "$SUDO_PW" ]; then echo "$SUDO_PW" | sudo -S "$@"; else sudo "$@"; fi
}

if [ ! -f "$IMG" ]; then
    echo "*** $IMG not found -- this recovery path requires the golden-image"
    echo "    backup on disk (not in git, too large -- see DEPLOYED_STATE.md"
    echo "    for where it should be). Cannot proceed. ***"
    exit 1
fi

echo "=== entering loader mode ($LOADER) ==="
sudo_run timeout 15s "$RKDEV" db "$LOADER"
sleep 2
sudo_run "$RKDEV" cs 2
sleep 3

echo "=== writing golden image ($(stat -c%s "$IMG") bytes) raw to LBA 0 ==="
echo "    (idbloader+GPT+system+vendor -- does NOT touch userdata)"
sudo_run timeout 1200s "$RKDEV" wl 0x0 "$IMG"
echo "golden-image write done."

echo ""
echo "=== reflashing fixed uboot/boot_linux on top ==="
"$(dirname "$0")/flash.sh"

echo ""
echo "=== Recovery flash complete. ==="
echo "Power-cycle the board (not MaskROM) normally. If userdata was wiped"
echo "(e.g. a prior FORCE_USERDATA=1 flash-full.sh run), first boot will be a"
echo "fresh/blank userdata: /data/ssd needs 'mkdir -p' before mounting the"
echo "NVMe SSD, and CA binaries under /data/ssd/rknpu/ will need re-pushing"
echo "from scripts/kick-the-tires/share/build-rknpure/ (they live on the"
echo "separate NVMe SSD though, so if that wasn't wiped, they should still"
echo "be there once mounted)."
