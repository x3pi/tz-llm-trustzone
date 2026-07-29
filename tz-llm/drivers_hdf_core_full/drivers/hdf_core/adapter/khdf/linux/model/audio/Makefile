#
# Copyright (c) 2021-2022 Huawei Device Co., Ltd.
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

KHDF_AUDIO_BASE_ROOT_DIR = ../../../../../../..
KHDF_AUDIO_KHDF_ROOT_DIR = drivers/hdf/khdf
KHDF_FRAMEWORK_ROOT_DIR = drivers/hdf/framework
KHDF_AUDIO_ROOT_DIR = ../../../../../framework/model/audio
KHDF_AUDIO_IMX8MM_DIR = $(KHDF_AUDIO_BASE_ROOT_DIR)/device/board/osware/$(DEVICE_NAME)/drivers/audio
KHDF_AUDIO_IMX8MM_INC_DIR = drivers/hdf/framework/../../../device/board/osware/$(DEVICE_NAME)/drivers/audio
KHDF_AUDIO_A311D_DIR = $(KHDF_AUDIO_BASE_ROOT_DIR)/device/board/unionman/$(DEVICE_NAME)/kernel/hdf/audio
KHDF_AUDIO_A311D_INC_DIR = drivers/hdf/framework/../../../device/board/unionman/$(DEVICE_NAME)/kernel/hdf/audio

obj-$(CONFIG_DRIVERS_HDF_AUDIO) += \
          $(KHDF_AUDIO_ROOT_DIR)/core/src/audio_core.o \
          $(KHDF_AUDIO_ROOT_DIR)/core/src/audio_host.o \
          $(KHDF_AUDIO_ROOT_DIR)/core/src/audio_parse.o \
          $(KHDF_AUDIO_ROOT_DIR)/common/src/audio_codec_base.o \
          $(KHDF_AUDIO_ROOT_DIR)/common/src/audio_dsp_base.o \
          $(KHDF_AUDIO_ROOT_DIR)/common/src/audio_dai_base.o \
          $(KHDF_AUDIO_ROOT_DIR)/common/src/audio_platform_base.o \
          $(KHDF_AUDIO_ROOT_DIR)/common/src/audio_dma_base.o \
          $(KHDF_AUDIO_ROOT_DIR)/sapm/src/audio_sapm.o \
          $(KHDF_AUDIO_ROOT_DIR)/dispatch/src/audio_stream_dispatch.o \
          $(KHDF_AUDIO_ROOT_DIR)/dispatch/src/audio_control_dispatch.o

obj-$(CONFIG_DRIVERS_HDF_AUDIO_HDMI) += \
          $(KHDF_AUDIO_ROOT_DIR)/hdmi/src/audio_hdmi_codec_adapter.o \
          $(KHDF_AUDIO_ROOT_DIR)/hdmi/src/audio_hdmi_codec_linux.o \
          $(KHDF_AUDIO_ROOT_DIR)/hdmi/src/audio_hdmi_codec_ops.o

obj-$(CONFIG_DRIVERS_HDF_AUDIO_USB) += \
          $(KHDF_AUDIO_ROOT_DIR)/usb/src/audio_usb_linux.o \
          $(KHDF_AUDIO_ROOT_DIR)/usb/src/audio_usb_codec_adapter.o \
          $(KHDF_AUDIO_ROOT_DIR)/usb/src/audio_usb_codec_ops.o \
          $(KHDF_AUDIO_ROOT_DIR)/usb/src/audio_usb_dma_adapter.o \
          $(KHDF_AUDIO_ROOT_DIR)/usb/src/audio_usb_dma_ops.o \
          $(KHDF_AUDIO_ROOT_DIR)/usb/src/audio_usb_endpoints.o \
          $(KHDF_AUDIO_ROOT_DIR)/usb/src/audio_usb_parse_interface.o \
          $(KHDF_AUDIO_ROOT_DIR)/usb/src/audio_usb_validate_desc.o \
          $(KHDF_AUDIO_ROOT_DIR)/usb/src/audio_usb_mixer.o

obj-$(CONFIG_DRIVERS_HDF_AUDIO_HI3516CODEC) += \
          $(KHDF_AUDIO_BASE_ROOT_DIR)/device/board/hisilicon/hispark_taurus/audio_drivers/

obj-$(CONFIG_DRIVERS_HDF_AUDIO_RK3568) += \
          $(KHDF_AUDIO_BASE_ROOT_DIR)/device/board/hihope/rk3568/audio_drivers/

obj-$(CONFIG_DRIVERS_HDF_AUDIO_RK3588) += \
          $(KHDF_AUDIO_BASE_ROOT_DIR)/device/board/hihope/dayu210/audio_drivers/

obj-$(CONFIG_DRIVERS_HDF_AUDIO_RK3566) += \
          $(KHDF_AUDIO_BASE_ROOT_DIR)/device/board/kaihong/khdvk_3566b/kernel/audio/

obj-$(CONFIG_DRIVERS_HDF_AUDIO_A311D) += \
          $(KHDF_AUDIO_A311D_DIR)/codec/soc_codec/a311d_codec_adapter.o \
          $(KHDF_AUDIO_A311D_DIR)/codec/soc_codec/a311d_codec_ops.o \
          $(KHDF_AUDIO_A311D_DIR)/dsp/a311d_dsp_adapter.o \
          $(KHDF_AUDIO_A311D_DIR)/dsp/a311d_dsp_ops.o \
          $(KHDF_AUDIO_A311D_DIR)/dai/a311d_dai_adapter.o \
          $(KHDF_AUDIO_A311D_DIR)/dai/a311d_dai_ops.o \
          $(KHDF_AUDIO_A311D_DIR)/soc/a311d_dma_adapter.o \
          $(KHDF_AUDIO_A311D_DIR)/soc/a311d_dma_ops.o \
          $(KHDF_AUDIO_A311D_DIR)/linux_drv/src/axg_fifo.o \
          $(KHDF_AUDIO_A311D_DIR)/linux_drv/src/axg_fifo_drv.o \
          $(KHDF_AUDIO_A311D_DIR)/linux_drv/src/axg_tdmin.o \
          $(KHDF_AUDIO_A311D_DIR)/linux_drv/src/axg_tdmout.o \
          $(KHDF_AUDIO_A311D_DIR)/linux_drv/src/axg_tdm_formatter.o \
          $(KHDF_AUDIO_A311D_DIR)/linux_drv/src/axg_tdm_interface.o \
          $(KHDF_AUDIO_A311D_DIR)/linux_drv/src/g12a_toacodec.o \
          $(KHDF_AUDIO_A311D_DIR)/linux_drv/src/g12a_tohdmitx.o \
          $(KHDF_AUDIO_A311D_DIR)/linux_drv/src/meson_t9015.o \
          $(KHDF_AUDIO_A311D_DIR)/linux_drv/src/axg_snd_card.o \
          $(KHDF_AUDIO_A311D_DIR)/linux_drv/src/nau8540.o

obj-$(CONFIG_DRIVERS_HDF_AUDIO_IMX8MM) += \
          $(KHDF_AUDIO_IMX8MM_DIR)/soc/src/sai_driver.o \
          $(KHDF_AUDIO_IMX8MM_DIR)/soc/src/dma_driver.o \
          $(KHDF_AUDIO_IMX8MM_DIR)/soc/src/imx8mm_platform_ops.o \
          $(KHDF_AUDIO_IMX8MM_DIR)/soc/src/imx8mm_dai_adapter.o \
          $(KHDF_AUDIO_IMX8MM_DIR)/soc/src/imx8mm_platform_adapter.o \
          $(KHDF_AUDIO_IMX8MM_DIR)/dsp/src/dsp_adapter.o \
          $(KHDF_AUDIO_IMX8MM_DIR)/dsp/src/dsp_ops.o \
          $(KHDF_AUDIO_IMX8MM_DIR)/codec/src/wm8904.o \
          $(KHDF_AUDIO_IMX8MM_DIR)/codec/src/wm8904_impl.o

ccflags-$(CONFIG_DRIVERS_HDF_AUDIO) += -lm -lc -lgcc -std=gnu99 -Werror\
             -I$(srctree)/$(KHDF_AUDIO_KHDF_ROOT_DIR)/osal/include \
             -I$(srctree)/$(KHDF_FRAMEWORK_ROOT_DIR)/include/core \
             -I$(srctree)/$(KHDF_FRAMEWORK_ROOT_DIR)/include/utils \
             -I$(srctree)/$(KHDF_FRAMEWORK_ROOT_DIR)/include/osal \
             -I$(srctree)/drivers/hdf/inner_api/osal/shared \
             -I$(srctree)/drivers/hdf/inner_api/utils \
             -I$(srctree)/drivers/hdf/inner_api/core \
             -I$(srctree)/drivers/hdf/inner_api/host/shared \
             -I$(srctree)/$(KHDF_FRAMEWORK_ROOT_DIR)/include/platform \
             -I$(srctree)/$(KHDF_FRAMEWORK_ROOT_DIR)/include/config \
             -I$(srctree)/$(KHDF_FRAMEWORK_ROOT_DIR)/include/audio \
             -I$(srctree)/$(KHDF_FRAMEWORK_ROOT_DIR)/ability/sbuf/include \
             -I$(srctree)/$(KHDF_FRAMEWORK_ROOT_DIR)/core/common/include/host \
             -I$(srctree)/$(KHDF_FRAMEWORK_ROOT_DIR)/core/host/include \
             -I$(srctree)/$(KHDF_FRAMEWORK_ROOT_DIR)/core/shared/include \
             -I$(srctree)/$(KHDF_FRAMEWORK_ROOT_DIR)/core/manager/include \
             -I$(srctree)/$(KHDF_FRAMEWORK_ROOT_DIR)/utils/include \
             -I$(srctree)/$(KHDF_FRAMEWORK_ROOT_DIR)/model/audio/core/include \
             -I$(srctree)/$(KHDF_FRAMEWORK_ROOT_DIR)/model/audio/sapm/include \
             -I$(srctree)/$(KHDF_FRAMEWORK_ROOT_DIR)/model/audio/dispatch/include \
             -I$(srctree)/$(KHDF_FRAMEWORK_ROOT_DIR)/model/audio/common/include \
             -I$(srctree)/$(KHDF_FRAMEWORK_ROOT_DIR)/model/audio/hdmi/include \
             -I$(srctree)/$(KHDF_FRAMEWORK_ROOT_DIR)/model/audio/usb/include \
             -I$(srctree)/bounds_checking_function/include

ccflags-$(CONFIG_DRIVERS_HDF_AUDIO_A311D) += \
             -I$(srctree)/$(KHDF_AUDIO_A311D_INC_DIR)/linux_drv/include

ccflags-$(CONFIG_DRIVERS_HDF_AUDIO_IMX8MM) += \
             -I$(srctree)/$(KHDF_AUDIO_IMX8MM_INC_DIR)/codec/include \
             -I$(srctree)/$(KHDF_AUDIO_IMX8MM_INC_DIR)/soc/include \
             -I$(srctree)/$(KHDF_AUDIO_IMX8MM_INC_DIR)/dsp/include \
             -I$(srctree)/$(KHDF_AUDIO_IMX8MM_INC_DIR)/include
