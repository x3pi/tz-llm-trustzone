#!/bin/bash

SHARE_DIR=/home/vectorxj/share

export PATH=/home/tools/clang_linux-x86_64-36cd05-20221030/bin:$PATH
export PATH=/home/tools/ninja:$PATH
export PATH=/home/tools/gn:$PATH
export PATH=/home/tools/node-v14.19.1-linux-x64/bin:$PATH
export PATH=/root/.local/bin:$PATH

# chcore.sh must bracket linux.sh on both sides, otherwise the FIT hash of
# uboot.img does not match the freshly built boot.img and the board refuses
# to boot. Running linux.sh alone also means TEE-OS/TA changes never reach
# uboot.img at all.
pushd /home/vectorxj/openharmony/
./chcore.sh
./linux.sh
set -e
./chcore.sh
popd

ls -lhat /home/vectorxj/openharmony/out/uboot/src_tmp/

mkdir -p $SHARE_DIR/images
cp /home/vectorxj/openharmony/out/uboot/src_tmp/boot.img $SHARE_DIR/images/
cp /home/vectorxj/openharmony/out/uboot/src_tmp/uboot.img $SHARE_DIR/images/
cp /home/vectorxj/openharmony/out/opi5plus/packages/phone/images/vendor.img $SHARE_DIR/images/ || true
mkdir -p $SHARE_DIR/rknpu/
cp /home/vectorxj/rknpu/* $SHARE_DIR/rknpu/

# The image ships set-npu-irq.sh for the Orange Pi 5 Plus, where the NPU sits
# on IRQ 29/30/31. This board is a 5 Max: /proc/interrupts shows fdab0000.npu
# on 27/28/29, so the stock script only ever moves one of the three. Rewritten
# wholesale rather than sed'd, because renumbering 31->29 and 29->27 in place
# aliases the two lines onto each other.
cat > $SHARE_DIR/rknpu/set-npu-irq.sh <<'EOF'
# Orange Pi 5 Max: fdab0000.npu is on IRQ 27/28/29 (not 29/30/31 as on 5 Plus).
# Pin them to CPU 4/5/6 so the Secure World cores can take them.
echo 10 > /proc/irq/27/smp_affinity
echo 20 > /proc/irq/28/smp_affinity
echo 40 > /proc/irq/29/smp_affinity
EOF
echo "--- patched set-npu-irq.sh ---"
cat $SHARE_DIR/rknpu/set-npu-irq.sh

mkdir -p $SHARE_DIR/driver_modules
find /home/vectorxj/openharmony/out -name 'bcmdhd.ko' -exec cp {} $SHARE_DIR/driver_modules/ \; 2>/dev/null || true
find /home/vectorxj/openharmony/out -name 'dhd_static_buf.ko' -exec cp {} $SHARE_DIR/driver_modules/ \; 2>/dev/null || true

# Build a raw, wl-writable idbloader (mkimage -T rksd) for blank-SD-card
# initialization. This runs AFTER boot.img/uboot.img/vendor.img are already
# safely copied out above, using absolute paths (not make.sh's own
# pack_idblock/`ul`, both found broken/dangerous -- see STATUS.md) so a
# failure here can never take down the proven-good outputs already produced.
# PLAT and the rk3588_spl_v1.13.bin/rk3588_ddr_...bin paths are read from
# this exact build's own already-configured U-Boot tree, not a generic
# rk3588-evb default (which is missing files in this image).
(
  set +e
  UBOOT_TMP=/home/vectorxj/openharmony/out/uboot/src_tmp
  cd $UBOOT_TMP
  COMMON_H=$(grep "_common.h:" include/autoconf.mk.dep | awk -F"/" '{ printf $3 }')
  PLAT=${COMMON_H%_*}
  echo "idblock: resolved PLAT=$PLAT"
  DDR_BIN=rkbin/bin/rk35/rk3588_ddr_lp4_2112MHz_lp5_2400MHz_v1.18.bin
  SPL_BIN=rkbin/bin/rk35/rk3588_spl_v1.13.bin
  ls -l $DDR_BIN $SPL_BIN
  ./tools/mkimage -n "$PLAT" -T rksd -d ${DDR_BIN}:${SPL_BIN} idblock.bin
  if [ -f idblock.bin ]; then
    cp idblock.bin $SHARE_DIR/images/idblock.bin
    echo "idblock: wrote $SHARE_DIR/images/idblock.bin ($(stat -c%s idblock.bin) bytes)"
  else
    echo "idblock: mkimage did not produce idblock.bin"
  fi
)
