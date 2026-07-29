#!/bin/bash
# Rebuild TEE-OS + Linux kernel via the tz-llm-oh-builder Docker image.
# Output lands in scripts/kick-the-tires/share_build/images/{uboot.img,boot.img,vendor.img}.
set -e
cd "$(dirname "$0")"
SHARE_DIR=$(pwd)/scripts/kick-the-tires/share_build
mkdir -p "$SHARE_DIR"
cp scripts/kick-the-tires/build-oh-docker.sh "$SHARE_DIR/"
./scripts/kick-the-tires/oh-builder-hdf.sh "$SHARE_DIR" bash -c ./build-oh-docker.sh 2>&1 | tee "$SHARE_DIR/build.log"
echo "Build output: $SHARE_DIR/images/"
