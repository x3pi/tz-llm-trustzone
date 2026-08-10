#!/bin/bash
SHARE_DIR=/home/vectorxj/share
export PATH=/home/tools/clang_linux-x86_64-36cd05-20221030/bin:$PATH
export PATH=/home/tools/ninja:$PATH
export PATH=/home/tools/gn:$PATH
export PATH=/home/tools/node-v14.19.1-linux-x64/bin:$PATH
export PATH=/root/.local/bin:$PATH

pushd /home/vectorxj/openharmony/
./chcore.sh
popd

mkdir -p $SHARE_DIR/images
# uboot.img isn't always produced at this path by a chcore-only build (this
# script only calls chcore.sh, not linux.sh) -- non-fatal here since
# build-llama.sh only needs the chcore/TEE-OS headers+libs this step
# produces for the CA-side build that follows, not this particular image.
cp /home/vectorxj/openharmony/out/uboot/src_tmp/uboot.img $SHARE_DIR/images/ || true
