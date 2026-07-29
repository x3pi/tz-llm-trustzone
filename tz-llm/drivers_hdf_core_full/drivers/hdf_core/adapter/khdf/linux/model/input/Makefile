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

INPUT_ROOT_DIR = ../../../framework/model/input/driver

# NOTE: input_i2c_ops.o, hdf_touch.o and hdf_key.o are for I2C-attached
# touchscreens and GPIO-wired physical keys (they call GpioSetDir/
# GpioRead/I2cTransfer etc. from CONFIG_DRIVERS_HDF_PLATFORM_GPIO/_I2C,
# which this board config does not enable). Not needed for USB HID
# (mouse/keyboard), which only goes through event_hub.o + hdf_hid_adapter.o.
obj-$(CONFIG_DRIVERS_HDF_INPUT) += \
               $(INPUT_ROOT_DIR)/hdf_input_device_manager.o \
               $(INPUT_ROOT_DIR)/input_config_parser.o \
               $(INPUT_ROOT_DIR)/event_hub.o \
               $(INPUT_ROOT_DIR)/hdf_hid_adapter.o

obj-$(CONFIG_DRIVERS_HDF_TP_5P5_GT911) += \
              $(INPUT_ROOT_DIR)/touchscreen/touch_gt911.o
obj-$(CONFIG_ARCH_NXP_TOUCH) += \
              $(INPUT_ROOT_DIR)/touchscreen/touch_ft5x06.o
obj-$(CONFIG_DRIVERS_HDF_TP_2P35_FT6236) += \
              $(INPUT_ROOT_DIR)/touchscreen/touch_ft6336.o
obj-$(CONFIG_DRIVERS_HDF_INPUT_INFRARED) += \
              $(INPUT_ROOT_DIR)/hdf_infrared.o
obj-$(CONFIG_DRIVERS_HDF_TP_5P43_FT5406) += \
              $(INPUT_ROOT_DIR)/touchscreen/touch_ft5406.o

ccflags-y +=-I$(srctree)/drivers/hdf/framework/model/input/driver \
            -I$(srctree)/drivers/hdf/framework/model/input/driver/input_bus_ops \
            -I$(srctree)/drivers/hdf/framework/include/core \
            -I$(srctree)/drivers/hdf/framework/core/common/include/host \
            -I$(srctree)/drivers/hdf/framework/include/utils \
            -I$(srctree)/drivers/hdf/framework/include/osal \
            -I$(srctree)/drivers/hdf/framework/include/platform \
            -I$(srctree)/drivers/hdf/framework/include/config \
            -I$(srctree)/drivers/hdf/framework/core/host/include \
            -I$(srctree)/drivers/hdf/framework/core/shared/include \
            -I$(srctree)/drivers/hdf/framework/utils/include \
            -I$(srctree)/drivers/hdf/inner_api/osal/shared \
            -I$(srctree)/drivers/hdf/inner_api/utils \
            -I$(srctree)/drivers/hdf/inner_api/core \
            -I$(srctree)/drivers/hdf/inner_api/host/shared \
            -I$(srctree)/drivers/hdf/khdf/osal/include
ccflags-y +=-I$(srctree)/bounds_checking_function/include \
            -I$(srctree)/drivers/hdf/evdev
