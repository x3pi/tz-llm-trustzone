#
# Copyright (c) 2022 Huawei Device Co., Ltd.
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

VIBRATOR_ROOT_DIR = ../../../../../../../hdf_core/framework/model/misc/vibrator/driver
VIBRATOR_ROOT_CHIPSET = ../../../../../../../peripheral/vibrator

obj-$(CONFIG_DRIVERS_HDF_VIBRATOR) += \
               $(VIBRATOR_ROOT_DIR)/src/vibrator_driver.o \
               $(VIBRATOR_ROOT_DIR)/src/vibrator_haptic.o \
               $(VIBRATOR_ROOT_DIR)/src/vibrator_parser.o
obj-$(CONFIG_DRIVERS_HDF_VIBRATOR_LINEAR) += \
              $(VIBRATOR_ROOT_CHIPSET)/chipset/linear/vibrator_linear_driver.o
obj-$(CONFIG_DRIVERS_HDF_VIBRATOR_DRV2605L) += \
              $(VIBRATOR_ROOT_CHIPSET)/chipset/drv2605l/vibrator_drv2605l_driver.o


ccflags-y +=-I$(srctree)/drivers/hdf/framework/model/misc/vibrator/driver/include \
            -I$(srctree)/drivers/hdf/peripheral/vibrator/chipset/linear \
            -I$(srctree)/drivers/hdf/peripheral/vibrator/chipset/drv2605l \
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
            -I$(srctree)/drivers/hdf/khdf/osal/include \
            -I$(srctree)/bounds_checking_function/include
