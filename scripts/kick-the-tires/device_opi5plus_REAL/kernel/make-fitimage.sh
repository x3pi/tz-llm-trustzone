#!/bin/bash

#export RK_ARCH=arm64
#export RK_KERNEL_DTS=rk3588-orangepi-5-plus

set -e

#SCRIPT_DIR=$(dirname $(realpath $BASH_SOURCE))
#TOP_DIR=$(realpath $SCRIPT_DIR/../../../../..) # OpenHarmony path
TOP_DIR=../../../../
echo "zzh: TOP_DIR=$TOP_DIR"
#cd $TOP_DIR

#source $TOP_DIR/device/rockchip/.BoardConfig.mk
#ROCKDEV=$TOP_DIR/rockdev

fdt=0
kernel=0
ramdisk=0
resource=0
OUTPUT_TARGET_IMAGE="$1"
src_its_file="$2"
ramdisk_file_path="$5"
kernel_dtb_file="$4"
kernel_image="$3"
target_its_file=".tmp_its"

#if [ -z $kernel_image ]; then
#	if [ -z $RK_KERNEL_ZIMG ]; then
#		kernel_image=$TOP_DIR/out/kernel/src_tmp/linux-5.10-opi/arch/arm64/boot/Image #$RK_KERNEL_IMG # change kernel/arch/arm64/boot/Image
#	else
#		kernel_image=$TOP_DIR/$RK_KERNEL_ZIMG
#	fi
#fi

if [ ! -f $src_its_file ]; then
	echo "Not Fount $src_its_file ..."
	exit -1
fi

if [ ! -f $ramdisk_file_path ]; then
	echo "Not Fount $ramdisk_file_path ..."
	exit -1
fi

if [ ! -f $kernel_dtb_file ]; then
	echo "Not Fount $kernel_dtb_file ..."
	exit -1
fi

if [ ! -f $kernel_image ]; then
	echo "Not Fount $kernel_image ..."
	exit -1
fi

MKIMAGE="mkimage"

#if [ "$RK_ARCH" == "arm" ]; then
#	kernel_dtb_file="kernel/arch/arm/boot/dts/$RK_KERNEL_DTS.dtb"
#else
#	kernel_dtb_file="out/kernel/src_tmp/linux-5.10-opi/arch/arm64/boot/dts/rockchip/$RK_KERNEL_DTS.dtb" # change
#fi

rm -f $target_its_file
mkdir -p "`dirname $target_its_file`"

while read line
do
	############################# generate fdt path
	if [ $fdt -eq 1 ];then
		echo "data = /incbin/(\"$kernel_dtb_file\");" >> $target_its_file
		fdt=0
		continue
	fi
	if echo $line | grep -w "^fdt" |grep -v ";"; then
		fdt=1
		echo "$line" >> $target_its_file
		continue
	fi

	############################# generate kernel image path
	if [ $kernel -eq 1 ];then
		echo "data = /incbin/(\"$kernel_image\");" >> $target_its_file
		kernel=0
		continue
	fi
	if echo $line | grep -w "^kernel" |grep -v ";"; then
		kernel=1
		echo "$line" >> $target_its_file
		continue
	fi

	############################# generate ramdisk path
	if [ -f $ramdisk_file_path ]; then
		if [ $ramdisk -eq 1 ];then
			echo "data = /incbin/(\"$ramdisk_file_path\");" >> $target_its_file
			ramdisk=0
			continue
		fi
		if echo $line | grep -w "^ramdisk" |grep -v ";"; then
			ramdisk=1
			echo "$line" >> $target_its_file
			continue
		fi
	fi

	############################# generate resource path
	if [ $resource -eq 1 ];then
		echo "data = /incbin/(\"boot_linux.img\");" >> $target_its_file
		resource=0
		continue
	fi
	if echo $line | grep -w "^resource" |grep -v ";"; then
		resource=1
		echo "$line" >> $target_its_file
		continue
	fi

	if [ "$RK_RAMDISK_SECURITY_BOOTUP" = "true" ];then
		if echo $line | grep -wq "uboot-ignore"; then
			echo "Enable Security boot, Skip uboot-ignore ..."
			continue
		fi
	fi

	echo "$line" >> $target_its_file
done < $src_its_file

#$TOP_DIR/uboot/rk3588/rkbin/tools/mkimage -f $target_its_file  -E -p 0x800 $OUTPUT_TARGET_IMAGE # todo: if you don't build uboot, you can't find rkbin!
RK_MKIMAGE="$TOP_DIR/out/uboot/src_tmp/tools/mkimage"
if [ ! -f "$RK_MKIMAGE" ]; then
    echo "Using host mkimage as fallback"
    RK_MKIMAGE="mkimage"
fi
$RK_MKIMAGE -f $target_its_file -E -p 0x800 $OUTPUT_TARGET_IMAGE
rm -f $target_its_file
