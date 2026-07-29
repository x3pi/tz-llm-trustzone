#!/bin/bash
# Copyright (c) 2023 Diemit <598757652@qq.com>
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

set -e

PROJECT_ROOT=$(cd $(dirname $0);cd ../../../../../; pwd)
PRODUCT_PATH=vendor/opc/opi5plus
KERNEL_VERSION=linux-5.10-opi
DEFCONFIG_FILE=orangepi5plus_oh_defconfig
OUT_PKG_DIR=${PROJECT_ROOT}/out/opi5plus/packages/phone/images

OUT_DIR=${PROJECT_ROOT}/out
KERNEL_SOURCE=${PROJECT_ROOT}/kernel/linux/${KERNEL_VERSION}
KERNEL_SRC_TMP_PATH=${OUT_DIR}/kernel/src_tmp/${KERNEL_VERSION}
KERNEL_OBJ_TMP_PATH=${OUT_DIR}/kernel/OBJ/${KERNEL_VERSION}
DEVICE_DIR=${PROJECT_ROOT}/device/board/opc/opi5plus
KERNEL_CONFIG_FILE=${DEVICE_DIR}/kernel/configs/${DEFCONFIG_FILE}
PATCHES_PATH=${DEVICE_DIR}/patches
HDF_PATCH_FILE=${PATCHES_PATH}/hdf.patch
TZDRIVER_PATCH_FILE=${PROJECT_ROOT}/kernel/linux/common_modules/tzdriver/apply_tzdriver.sh

rm -rf ${KERNEL_SRC_TMP_PATH}
mkdir -p ${KERNEL_SRC_TMP_PATH}

rm -rf ${KERNEL_OBJ_TMP_PATH}
# mkdir -p ${KERNEL_OBJ_TMP_PATH}

export KBUILD_OUTPUT=${KERNEL_SRC_TMP_PATH}
export INSTALL_MOD_PATH=${OUT_PKG_DIR}/../../../driver_modules

echo "cp kernel source"
cp -arf ${KERNEL_SOURCE}/* ${KERNEL_SRC_TMP_PATH}/

cd ${KERNEL_SRC_TMP_PATH}

#hdf patch 打入HDF补丁
bash ${PATCHES_PATH}/hdf_patch.sh ${PROJECT_ROOT} ${KERNEL_SRC_TMP_PATH} ${HDF_PATCH_FILE}

#tzdriver
if [ -f $TZDRIVER_PATCH_FILE ]; then
    bash $TZDRIVER_PATCH_FILE ${PROJECT_ROOT} ${KERNEL_SRC_TMP_PATH} orangepi5plus ${KERNEL_VERSION}
fi

patch -p1 < ${PATCHES_PATH}/tzdriver.patch

cp -rf ${DEVICE_DIR}/kernel/make*.sh ${KERNEL_SRC_TMP_PATH}/
#config
cp -rf ${KERNEL_CONFIG_FILE} ${KERNEL_SRC_TMP_PATH}/arch/arm64/configs/${DEFCONFIG_FILE}

./make-ohos.sh orangepi_5p enable_ramdisk

mkdir -p ${OUT_PKG_DIR}

cp ${KERNEL_SRC_TMP_PATH}/boot_linux.img ${OUT_PKG_DIR}
cp ${KBUILD_OUTPUT}/resource.img ${OUT_PKG_DIR}/resource.img
cp ${DEVICE_DIR}/loader/parameter.txt ${OUT_PKG_DIR}/parameter.txt
#cp ${DEVICE_DIR}/loader/MiniLoaderAll.bin ${OUT_PKG_DIR}/MiniLoaderAll.bin
#cp ${DEVICE_DIR}/loader/uboot.img ${OUT_PKG_DIR}/uboot.img
#cp ${DEVICE_DIR}/loader/config.cfg ${OUT_PKG_DIR}/config.cfg
