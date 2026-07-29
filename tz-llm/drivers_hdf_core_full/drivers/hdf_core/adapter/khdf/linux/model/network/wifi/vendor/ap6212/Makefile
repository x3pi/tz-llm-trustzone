MODULE_NAME := ap6212

HDF_VENDOR_PREFIX := ../../../../../../../../../../

include drivers/hdf/khdf/model/network/wifi/hdfwifi.mk

INC_TOP_PATH := $(PROJECT_ROOT)
VENDOR_WIFI_PATH := device/board/osware/imx8mm/drivers/wifi
WIFI_DRIVER_DIR := ap6212

HDF_WIFI_VENDOR_ROOT = $(HDF_VENDOR_PREFIX)/device/board/osware/imx8mm/drivers/wifi
obj-$(CONFIG_DRIVERS_AP6212) += $(MODULE_NAME).o
$(MODULE_NAME)-objs := $(HDF_WIFI_VENDOR_ROOT)/adapter/hdf_wlan_sdio_adapt.o

ccflags-$(CONFIG_DRIVERS_HDF_WIFI) += \
    $(HDF_FRAMEWORKS_INC) \
    $(HDF_WIFI_FRAMEWORKS_INC) \
    $(HDF_WIFI_ADAPTER_INC) \
    $(HDF_WIFI_VENDOR_INC) \
   -I$(srctree)/$(INC_TOP_PATH)/$(WIFI_DRIVER_DIR)/linuxkpi/include \
    $(SECURE_LIB_INC)

obj-y += $(HDF_VENDOR_PREFIX)/device/board/osware/imx8mm/drivers/ethernet/
ccflags-y += -Idrivers/net/ethernet/freescale/
obj-y += $(HDF_VENDOR_PREFIX)/device/board/osware/imx8mm/drivers/wifi/ap6212/