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

MODULE_NAME := hdf_vendor_wifi_bus

HDF_DIR_PREFIX := ../../../../../../../

include drivers/hdf/khdf/model/network/wifi/hdfwifi.mk

IBUS_PATH := bus

obj-$(CONFIG_DRIVERS_HDF_WIFI_BUS) += $(MODULE_NAME).o
$(MODULE_NAME)-objs := $(HDF_WIFI_FRAMEWORKS_ROOT)/$(IBUS_PATH)/hdf_ibus_intf.o \

ifeq ($(CONFIG_DRIVERS_HDF_WLAN_SDIO),y)
$(MODULE_NAME)-$(CONFIG_DRIVERS_HDF_WLAN_SDIO) += $(HDF_WIFI_FRAMEWORKS_ROOT)/$(IBUS_PATH)/hdf_sdio_intf.o
else ifeq ($(CONFIG_DRIVERS_HDF_WLAN_USB),y)
$(MODULE_NAME)-$(CONFIG_DRIVERS_HDF_WLAN_USB) += $(HDF_WIFI_FRAMEWORKS_ROOT)/$(IBUS_PATH)/hdf_usb_intf.o
endif

ccflags-$(CONFIG_DRIVERS_HDF_WIFI_BUS) += \
    $(HDF_FRAMEWORKS_INC) \
    $(HDF_WIFI_FRAMEWORKS_INC) \
    $(HDF_WIFI_ADAPTER_INC) \
    $(SECURE_LIB_INC)

HDF_WIFI_BUS_FLAGS +=-D_PRE_OS_VERSION_LINUX=1
HDF_WIFI_BUS_FLAGS +=-D_PRE_OS_VERSION_LITEOS=2
HDF_WIFI_BUS_FLAGS +=-D_PRE_OS_VERSION=_PRE_OS_VERSION_LINUX

ccflags-$(CONFIG_DRIVERS_HDF_WIFI_BUS) += $(HDF_WIFI_BUS_FLAGS)
