#
# Copyright (c) 2020-2022 Huawei Device Co., Ltd.
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

obj-$(CONFIG_DRIVERS_HDF_NETWORK)  += net_device_adapter.o netbuf_adapter.o

ccflags-$(CONFIG_DRIVERS_HDF_NETWORK) += \
            -I$(srctree)/drivers/hdf/framework/model/network/common/netdevice \
            -I$(srctree)/drivers/hdf/framework/include/net\
            -I$(srctree)/drivers/hdf/khdf/network/include \
            -I$(srctree)/drivers/hdf/khdf/osal/include \
            -I$(srctree)/include/hdf/wifi \
            -I$(srctree)/include/hdf/utils \
            -I$(srctree)/include/hdf/osal \
            -I$(srctree)/drivers/hdf/inner_api/osal/shared \
            -I$(srctree)/drivers/hdf/inner_api/utils \
            -I$(srctree)/drivers/hdf/inner_api/core \
            -I$(srctree)/drivers/hdf/inner_api/host/shared \
            -I$(srctree)/bounds_checking_function/include
