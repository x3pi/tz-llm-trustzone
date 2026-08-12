#!/bin/bash

set -ex
echo "zzh: make-ohos.sh started! args: $@"

export PATH=../../../../prebuilts/clang/ohos/linux-x86_64/llvm/bin/:$PATH
export PRODUCT_PATH=vendor/opc/opi5plus
IMAGE_SIZE=64  # 64M
IMAGE_BLOCKS=4096

CPUs=`sed -n "N;/processor/p" /proc/cpuinfo|wc -l`
TOOLCHAIN_ARM64="../../../../prebuilts/gcc/linux-x86/aarch64/gcc-linaro-7.5.0-2019.12-x86_64_aarch64-linux-gnu/bin"
MAKE="make CROSS_COMPILE=${TOOLCHAIN_ARM64}/aarch64-linux-gnu- LLVM=1 LLVM_IAS=1"
BUILD_PATH=boot_linux
EXTLINUX_PATH=${BUILD_PATH}/extlinux
EXTLINUX_CONF=${EXTLINUX_PATH}/extlinux.conf
TOYBRICK_DTB=toybrick.dtb
if [ ${KBUILD_OUTPUT} ]; then
	OBJ_PATH=${KBUILD_OUTPUT}/
fi

ID_MODEL=1
ID_ARCH=2
ID_UART=3
ID_DTB=4
ID_IMAGE=5
ID_CONF=6
model_list=(
	"orangepi_5p          arm64 0xfeb50000 rk3588-orangepi-5-max Image orangepi5plus_oh_defconfig"
)


function help()
{
	echo "Usage: ./make-ohos.sh {BOARD_NAME}"
	echo "e.g."
	for i in "${model_list[@]}"; do
		echo "  ./make-ohos.sh $(echo $i | awk '{print $1}')"
	done
}


function make_extlinux_conf()
{
	dtb_path=$1
	uart=$2
	image=$3
	
	echo "label rockchip-kernel-5.10" > ${EXTLINUX_CONF}
	echo "	kernel /extlinux/${image}" >> ${EXTLINUX_CONF}
	echo "	fdt /extlinux/${TOYBRICK_DTB}" >> ${EXTLINUX_CONF}
	# cmdline="append earlycon=uart8250,mmio32,${uart} console=ttyFIQ0 root=PARTUUID=614e0000-0000 hardware=opi5p default_boot_device=fe2e0000.mmc rw rootwait ohos.required_mount.system=/dev/block/platform/fe2e0000.mmc/by-name/system@/usr@ext4@ro,barrier=1@wait,required ohos.required_mount.vendor=/dev/block/platform/fe2e0000.mmc/by-name/vendor@/vendor@ext4@ro,barrier=1@wait,required ohos.required_mount.misc=/dev/block/platform/fe2e0000.mmc/by-name/misc@none@none@none@wait,required ohos.required_mount.bootctrl=/dev/block/platform/fe2e0000.mmc/by-name/bootctrl@none@none@none@wait,required"
	# echo "  ${cmdline}" >> ${EXTLINUX_CONF}
}

function make_kernel_image()
{
	arch=$1
	conf=$2
	dtb=$3
	
	${MAKE} ARCH=${arch} ${conf}
	if [ $? -ne 0 ]; then
		echo "FAIL: ${MAKE} ARCH=${arch} ${conf}"
		return -1
	fi

	${MAKE} ARCH=${arch} ${dtb}.img -j${CPUs}
	if [ $? -ne 0 ]; then
		echo "FAIL: ${MAKE} ARCH=${arch} ${dtb}.img"
		return -2
	fi

	${MAKE} ARCH=${arch} modules_install

	return 0
}

function make_ext2_image()
{
	blocks=${IMAGE_BLOCKS}
	block_size=$((${IMAGE_SIZE} * 1024 * 1024 / ${blocks}))

	if [ "`uname -m`" == "aarch64" ]; then
		echo y | sudo mke2fs -b ${block_size} -d boot_linux -i 8192 -t ext2 boot_linux.img ${blocks}
	else
		genext2fs -B ${blocks} -b ${block_size} -d boot_linux -i 8192 -U boot_linux.img
	fi

	return $?
}

function make_boot_linux()
{
	arch=${!ID_ARCH}
	uart=${!ID_UART}
	dtb=${!ID_DTB}
	image=${!ID_IMAGE}
	conf=${!ID_CONF}
	if [ ${arch} == "arm" ]; then
		dtb_path=arch/arm/boot/dts
	else
		dtb_path=arch/arm64/boot/dts/rockchip
	fi

	rm -rf ${BUILD_PATH}
	mkdir -p ${EXTLINUX_PATH}

	make_kernel_image ${arch} ${conf} ${dtb}
	if [ $? -ne 0 ]; then
		exit 1
	fi
	make_extlinux_conf ${dtb_path} ${uart} ${image}
	cp -f ${OBJ_PATH}arch/${arch}/boot/${image} ${EXTLINUX_PATH}/
	cp -f ${OBJ_PATH}${dtb_path}/${dtb}.dtb ${EXTLINUX_PATH}/${TOYBRICK_DTB}
	# cp -f logo*.bmp ${BUILD_PATH}/
	# if [ "enable_ramdisk" != "${ramdisk_flag}" ]; then
		make_ext2_image
	# fi
	echo "zzh: make-ohos.sh: make_boot_linux: pwd=`pwd`" # out/kernel/src_tmp/linux-5.10-opi
	./make-fitimage.sh boot_fit.img boot.its ${EXTLINUX_PATH}/${image} ${EXTLINUX_PATH}/${TOYBRICK_DTB} ${EXTLINUX_PATH}/../../../../../opi5plus/packages/phone/images/ramdisk.img
}

ramdisk_flag=$2
found=0
for i in "${model_list[@]}"; do
	if [ "$(echo $i | awk '{print $1}')" == "$1" ]; then
		make_boot_linux $i
		found=1
	fi
done

