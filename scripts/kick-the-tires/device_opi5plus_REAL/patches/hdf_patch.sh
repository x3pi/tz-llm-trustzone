#!/bin/bash
#
# Copyright (c) 2020-2021 Huawei Device Co., Ltd.
#
# This software is licensed under the terms of the GNU General Public
# License version 2, as published by the Free Software Foundation, and
# may be copied, distributed, and modified under those terms.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
#

set -e

OHOS_SOURCE_ROOT=$1
KERNEL_BUILD_ROOT=$2
HDF_PATCH_FILE=$3

ln_list=(
    drivers/hdf_core/adapter/khdf/linux    drivers/hdf/khdf
    drivers/hdf_core/framework             drivers/hdf/framework
    drivers/hdf_core/interfaces/inner_api  drivers/hdf/inner_api
    drivers/hdf_core/framework/include     include/hdf
)

cp_list=(
    $OHOS_SOURCE_ROOT/third_party/bounds_checking_function  ./
#    $OHOS_SOURCE_ROOT/device/soc/hisilicon/common/platform/wifi         drivers/hdf/
    $OHOS_SOURCE_ROOT/third_party/FreeBSD/sys/dev/evdev     drivers/hdf/
)

function copy_external_compents()
{
    for ((i=0; i<${#cp_list[*]}; i+=2))
    do
        dst_dir=${cp_list[$(expr $i + 1)]}/${cp_list[$i]##*/}
        mkdir -p $dst_dir
        [ -d "${cp_list[$i]}"/ ] && cp -arfL ${cp_list[$i]}/* $dst_dir/
    done
}

function ln_hdf_repos()
{
    # drivers/hdf_core's own Makefiles use hardcoded '../'-escaping obj-y
    # paths (e.g. drivers/hdf/khdf/utils/Makefile references
    # ../../../../framework/..., manager/Makefile goes up 6 levels for
    # third_party/bounds_checking_function) whose depth assumes hdf_core
    # sits directly at $OHOS_SOURCE_ROOT/drivers/hdf_core. A plain
    # directory symlink at drivers/hdf/khdf (the upstream approach) makes
    # real file-open()s resolve fine (the OS follows the symlink), but
    # kbuild's thin archives (`ar cDPrST`, used unconditionally by this
    # kernel's scripts/Makefile.lib) store/re-resolve the literal
    # '../'-relative path string when composing nested built-in.a files,
    # and that bookkeeping does NOT consistently account for a directory
    # symlink jump -- it silently breaks ("No such file" from ar, or "No
    # rule to make target" from make), and empirically *where* it breaks
    # shifts depending on how many directory-symlink hops are involved.
    #
    # Fix: never symlink a DIRECTORY. Instead mirror each source tree as
    # a "symlink farm" (`cp -rs`): real directories all the way down,
    # with only the leaf regular files replaced by symlinks back to
    # $OHOS_SOURCE_ROOT. '../' traversal during kbuild's archive
    # composition then only ever walks real directories, so there is no
    # symlink-vs-naive-path divergence for it to get confused by. And
    # since leaf files are absolute symlinks straight to
    # $OHOS_SOURCE_ROOT/drivers/hdf_core/..., $(realpath) on e.g.
    # drivers/hdf/khdf/Makefile still resolves to its true depth under
    # $OHOS_SOURCE_ROOT, so upstream Makefiles' own '../'-escapes (e.g.
    # khdf/Makefile's HCS_DIR, manager/Makefile's OHOS_SOURCE_ROOT) keep
    # resolving correctly too -- no extra depth-mismatch patching needed.
    for ((i=0; i<${#ln_list[*]}; i+=2))
    do
        rm -rf ${ln_list[$(expr $i + 1)]}
        mkdir -p $(dirname ${ln_list[$(expr $i + 1)]})
        cp -rs ${OHOS_SOURCE_ROOT}/${ln_list[$i]} ${ln_list[$(expr $i + 1)]}
    done
}

function fix_naive_path_shortfall()
{
    # Various drivers/hdf_core Makefiles reference paths like
    # $OHOS_SOURCE_ROOT/drivers/hdf_core/framework/tools/hc-gen or
    # $OHOS_SOURCE_ROOT/vendor/opc/opi5plus/hdf_config/khdf using
    # hardcoded '../'-escape counts, assuming the kernel tree sits at
    # $OHOS_SOURCE_ROOT/kernel/linux/<version>/ (2 levels below
    # $OHOS_SOURCE_ROOT). This project's kernel instead builds under
    # $OHOS_SOURCE_ROOT/out/kernel/src_tmp/<version>/ (4 levels below
    # $OHOS_SOURCE_ROOT). Some of these references go through
    # $(realpath ...) (which follows the leaf-file symlinks set up by
    # ln_hdf_repos and correctly resolves to the true depth), but others
    # are plain string concatenation done by kbuild's own Makefile.build
    # recursion against $(obj) (which tracks the shallow, real
    # drivers/hdf/khdf directory) -- those consistently land exactly 2
    # levels short, always at $OHOS_SOURCE_ROOT/out/kernel/<rest-of-path>
    # instead of $OHOS_SOURCE_ROOT/<rest-of-path>. Rather than patch each
    # occurrence individually as they're discovered, just make that whole
    # class of reference resolve: symlink the two subtrees that get
    # referenced this way (drivers/, vendor/) into
    # $OHOS_SOURCE_ROOT/out/kernel/.
    mkdir -p ${OHOS_SOURCE_ROOT}/out/kernel
    ln -sfn ../../drivers ${OHOS_SOURCE_ROOT}/out/kernel/drivers
    ln -sfn ../../vendor  ${OHOS_SOURCE_ROOT}/out/kernel/vendor
}

function main()
{
    cd $KERNEL_BUILD_ROOT
    patch -p1 < $HDF_PATCH_FILE
    ln_hdf_repos
    copy_external_compents
    fix_naive_path_shortfall
    cd -
}

main
