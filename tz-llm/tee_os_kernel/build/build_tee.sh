# Copyright (c) 2023 Institute of Parallel And Distributed Systems (IPADS), Shanghai Jiao Tong University (SJTU)
# Licensed under the Mulan PSL v2.
# You can use this software according to the terms and conditions of the Mulan PSL v2.
# You may obtain a copy of Mulan PSL v2 at:
#     http://license.coscl.org.cn/MulanPSL2
# THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR FIT FOR A PARTICULAR
# PURPOSE.
# See the Mulan PSL v2 for more details.
set -e

# paths setting
OH_TOP_DIR=$1
CHCORE_DIR=$2
OH_TEE_FRAMEWORK_DIR=$3
COMPILER_DIR=$4
COMPILER_VER=$5
if [[ -z "$OH_TOP_DIR" ]]; then
    OH_TOP_DIR=$(pwd)/../../../..
fi
if [[ -z "$CHCORE_DIR" ]]; then
    CHCORE_DIR=$(pwd)/..
fi
if [[ -z "$OH_TEE_FRAMEWORK_DIR" ]]; then
    OH_TEE_FRAMEWORK_DIR=$(pwd)/../../tee_os_framework
fi

cd ${CHCORE_DIR}
make clean
mkdir -p ramdisk-dir
cp oh_tee/apps/* ramdisk-dir
make -j$(nproc) OH_DIR=${OH_TOP_DIR}
