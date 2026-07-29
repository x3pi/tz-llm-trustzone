#!/bin/bash

set -e

export PATH=../../../../prebuilts/clang/ohos/linux-x86_64/llvm/bin/:$PATH
export PRODUCT_PATH=vendor/orangepi/opi5plus
TOOLCHAIN_ARM64="../../../../prebuilts/gcc/linux-x86/aarch64/gcc-linaro-7.5.0-2019.12-x86_64_aarch64-linux-gnu/bin"
MAKE="make CROSS_COMPILE=${TOOLCHAIN_ARM64}/aarch64-linux-gnu- LLVM=1 LLVM_IAS=1"
CPUs=`sed -n "N;/processor/p" /proc/cpuinfo|wc -l`

IMAGE_SIZE=64  # 64M
IMAGE_BLOCKS=4096
BUILD_PATH=boot_linux
EXTLINUX_PATH=${BUILD_PATH}/extlinux
EXTLINUX_CONF=${EXTLINUX_PATH}/extlinux.conf
RK3588S_9TRIPOD_X3588S=toybrick.dtb

if [ ${KBUILD_OUTPUT} ]; then
	OBJ_PATH=${KBUILD_OUTPUT}/
fi

ID_MODEL=1
ID_ARCH=2
ID_UART=3
ID_DTB=4
ID_IMAGE=5
ID_CONF=6
ID_ANDROID=7
ID_PCIE=8
model_list=(
	"orangepi_5p          arm64 0xfeb50000 rk3588-orangepi-5-plus Image orangepi5plus_oh_defconfig"
)

function help()
{
	echo "Usage: ./make-ohos.sh {BOARD_NAME}"
	echo "e.g."
	for i in "${model_list[@]}"; do
		echo "  ./make-ohos.sh $(echo $i | awk '{print $1}')"
	done
}

function make_kernel_image()
{
	arch=$1
	conf=$2
	dtb=$3
	edge=$4
	pcie=$5
	
	${MAKE} ARCH=${arch} ${conf} ${edge}
	if [ $? -ne 0 ]; then
		echo "FAIL: ${MAKE} ARCH=${arch} ${conf} ${edge}"
		return -1
	fi

	# echo ${MAKE} ARCH=${arch} ${dtb}.img -j${CPUs}
	${MAKE} ARCH=${arch} ${dtb}.img -j${CPUs}
	if [ $? -ne 0 ]; then
		echo "FAIL: ${MAKE} ARCH=${arch} ${dtb}.img"
		return -2
	fi

	return 0
}

function make_extlinux_conf()
{
	dtb_path=$1
	uart=$2
	image=$3
	
	echo "label rockchip-kernel-5.10" > ${EXTLINUX_CONF}
	echo "	kernel /extlinux/${image}" >> ${EXTLINUX_CONF}
	echo "	fdt /extlinux/${RK3588S_9TRIPOD_X3588S}" >> ${EXTLINUX_CONF}
	cmdline="append earlycon=uart8250,mmio32,${uart} root=PARTUUID=614e0000-0000-4b53-8000-1d28000054a9 hardware=rk3588s rw rootwait rootfstype=ext4"	
	echo "  ${cmdline}" >> ${EXTLINUX_CONF}
}

function make_ext2_image()
{
	blocks=${IMAGE_BLOCKS}
	block_size=$((${IMAGE_SIZE} * 1024 * 1024 / ${blocks}))

	echo ${blocks}
	echo ${block_size}

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
	dtb=${!ID_DTB}
	image=${!ID_IMAGE}
	conf=${!ID_CONF}
	uart=${!ID_UART}
	android=${!ID_ANDROID}
	pcie=${!ID_PCIE}

	if [ ${arch} == "arm" ]; then
		dtb_path=arch/arm/boot/dts
	else
		dtb_path=arch/arm64/boot/dts/rockchip
	fi
	
	echo ${dtb_path}

	rm -rf ${BUILD_PATH}
	mkdir -p ${EXTLINUX_PATH}

	make_kernel_image ${arch} ${conf} ${dtb} ${android} ${pcie}
	if [ $? -ne 0 ]; then
		exit 1
	fi
	make_extlinux_conf ${dtb_path} ${uart} ${image}

	cp -f ${OBJ_PATH}arch/${arch}/boot/${image} ${EXTLINUX_PATH}/
	cp -f ${OBJ_PATH}${dtb_path}/${dtb}.dtb ${EXTLINUX_PATH}/${RK3588S_9TRIPOD_X3588S}
	#cp -f logo*.bmp ${BUILD_PATH}/
	#if [ "enable_ramdisk" != "${ramdisk_flag}" ]; then
		make_ext2_image
	#fi
}


found=0
for i in "${model_list[@]}"; do
	if [ "$(echo $i | awk '{print $1}')" == "$1" ]; then
		echo "$(echo $i | awk '{print $1}')"
	found=1
	make_boot_linux $i
	fi
done

if [ ${found} == "0" ]; then
	help
fi