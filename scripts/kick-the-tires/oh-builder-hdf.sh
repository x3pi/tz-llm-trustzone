#!/bin/bash
# Wrapper around oh-builder.sh that ALSO bind-mounts our bugfixed
# drivers/hdf_core (CreateDeviceNode() mouse/keyboard crash fix, official
# OpenHarmony Gitee PR !2035) over the Docker image's stock baked-in copy.
#
# The pipeline's own device/board/opc/opi5plus/patches/hdf_patch.sh already
# wires drivers/hdf_core into the kernel build automatically (patch + symlinks)
# every time build_kernel.sh runs — it just needs drivers/hdf_core to be OUR
# fixed copy instead of the image's stock one. This is the only difference
# from oh-builder.sh.
set -e

CURRENT_DIR=$(realpath $(dirname $0))
ROOT_DIR=$(realpath $CURRENT_DIR/../..)
TZ_LLM_DIR=$ROOT_DIR/tz-llm

pushd $1
SHARE_DIR=$(pwd)
popd
shift

docker run --rm \
    -v $TZ_LLM_DIR/tee_os_kernel:/home/vectorxj/openharmony/base/tee/tee_os_kernel \
    -v $TZ_LLM_DIR/linux-5.10-opi:/home/vectorxj/openharmony/kernel/linux/linux-5.10-opi \
    -v $TZ_LLM_DIR/tzdriver:/home/vectorxj/openharmony/kernel/linux/common_modules/tzdriver \
    -v $TZ_LLM_DIR/llama.cpp:/home/vectorxj/chcore/opentrustee_llm/llama.cpp \
    -v $CURRENT_DIR/device_opi5plus_REAL:/home/vectorxj/openharmony/device/board/opc/opi5plus \
    -v $CURRENT_DIR/ramdisk_5plus_original.img:/home/vectorxj/openharmony/out/opi5plus/packages/phone/images/ramdisk.img \
    -v $TZ_LLM_DIR/drivers_hdf_core_full/drivers/hdf_core:/home/vectorxj/openharmony/drivers/hdf_core \
    -v $TZ_LLM_DIR/vendor_opi5plus_full/vendor/opc/opi5plus:/home/vectorxj/openharmony/vendor/opc/opi5plus \
    -v $CURRENT_DIR/chcore-extracted.sh:/home/vectorxj/openharmony/chcore.sh \
    -v $SHARE_DIR:/home/vectorxj/share \
    -w /home/vectorxj/share \
    vectorxj0553/tz-llm-oh-builder:latest \
    $@
