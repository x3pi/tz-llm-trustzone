#!/bin/bash
# Repack a freshly built uboot.img (from rebuild.sh) with the known-good
# U-Boot binary from scripts/kick-the-tires/repack/ -- the U-Boot produced by
# the Docker pipeline does not know the SD card's GPT partition name
# "boot_linux" (it looks for "boot") and fails with "FIT: No boot partition".
# See ../STATUS.md for the full story and the three mkimage traps.
set -e
cd "$(dirname "$0")/.."

SRC_UBOOT=${1:-scripts/kick-the-tires/share_build/images/uboot.img}
R=scripts/kick-the-tires/repack
W=$(mktemp -d)
trap 'rm -rf "$W"' EXIT

cp $R/u-boot-nodtb.bin $R/bl31_0x00040000.bin $R/bl31_0xff100000.bin $R/bl31_0x000f0000.bin $R/u-boot.dtb "$W/"
sed '/signature {/,/};/d' $R/u-boot.its > "$W/u-boot.its"

tools/bin/dumpimage -i "$SRC_UBOOT" -T flat_dt -p 4 "$W/tee.bin"

# -E: external data (inline data crashes the SPL with a Synchronous Abort).
# -p 0x1000: absolute data-position, matching what the SPL expects (default
# is relative data-offset, which the SPL does not understand).
tools/bin/mkimage-rkbin -f "$W/u-boot.its" -E -p 0x1000 "$W/uboot.itb"
cp "$W/uboot.itb" checkpoints/uboot_repacked.img
truncate -s 67108864 checkpoints/uboot_repacked.img

echo "boot_linux string present: $(strings -a checkpoints/uboot_repacked.img | grep -c boot_linux) (must be 1)"
tools/bin/dumpimage -l checkpoints/uboot_repacked.img | grep -E "^ Image|  Hash value"
echo "-> checkpoints/uboot_repacked.img"
