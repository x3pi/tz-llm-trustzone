/*
 * adc driver adapter of linux
 *
 * Copyright (c) 2022-2023 Huawei Device Co., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 */

#include <linux/fs.h>
#include <linux/kernel.h>
#include "device_resource_if.h"
#include "hdf_base.h"
#include "hdf_device_desc.h"
#include "hdf_log.h"
#include "osal_mem.h"
#include "securec.h"
#include "adc_core.h"

#define ADC_STRING_VALUE_LEN 15
#define ADC_MAX_CHANNEL_NUM 128
#define ADC_CHANNEL_NAME_LEN 30
#define INT_MAX_VALUE 2147483647
#define FILE_MODE 0600
#define DECIMAL_SHIFT_LEFT 10

struct AdcIioDevice {
    struct AdcDevice device;
    uint32_t channelNum;
    volatile unsigned char *regBase;
    volatile unsigned char *pinCtrlBase;
    uint32_t regBasePhy;
    uint32_t regSize;
    uint32_t deviceNum;
    uint32_t dataWidth;
    uint32_t validChannel;
    uint32_t scanMode;
    uint32_t delta;
    uint32_t deglitch;
    uint32_t glitchSample;
    uint32_t rate;
    const char *driverPathname[ADC_MAX_CHANNEL_NUM];
    struct file *fp[ADC_MAX_CHANNEL_NUM];
};

static int32_t AdcIioRead(struct AdcDevice *device, uint32_t channel, uint32_t *val)
{
    int ret;
    loff_t pos = 0;
    unsigned char strValue[ADC_STRING_VALUE_LEN] = {0};
    struct AdcIioDevice *adcDevice = NULL;

    if (device == NULL) {
        HDF_LOGE("AdcIioRead: device is null!");
        return HDF_ERR_INVALID_OBJECT;
    }
    if (val == NULL) {
        HDF_LOGE("AdcIioRead: val is null!");
        return HDF_ERR_INVALID_PARAM;
    }
    adcDevice = (struct AdcIioDevice *)device;
    if (channel >= adcDevice->channelNum || adcDevice->fp[channel] == NULL) {
        HDF_LOGE("AdcIioRead: invalid channel:%u!", channel);
        return HDF_ERR_INVALID_PARAM;
    }
    ret = kernel_read(adcDevice->fp[channel], strValue, ADC_STRING_VALUE_LEN, &pos);
    if (ret < 0) {
        HDF_LOGE("AdcIioRead: kernel_read fail, ret: %d!", ret);
        return HDF_PLT_ERR_OS_API;
    }
    *val = simple_strtoul(strValue, NULL, 0);
    return HDF_SUCCESS;
}

static int32_t AdcIioStop(struct AdcDevice *device)
{
    int ret;
    uint32_t i;
    struct AdcIioDevice *adcDevice = NULL;

    if (device == NULL) {
        HDF_LOGE("AdcIioStop: device is null!");
        return HDF_ERR_INVALID_OBJECT;
    }
    adcDevice = (struct AdcIioDevice *)device;
    for (i = 0; i < adcDevice->channelNum; i++) {
        if (adcDevice->fp[i] != NULL) {
            ret = filp_close(adcDevice->fp[i], NULL);
            if (ret != 0) {
                HDF_LOGE("AdcIioStop: filp_close fail!");
                adcDevice->fp[i] = NULL;
                return HDF_FAILURE;
            }
            adcDevice->fp[i] = NULL;
        }
    }
    return HDF_SUCCESS;
}

static int32_t AdcIioStart(struct AdcDevice *device)
{
    uint32_t i;
    int32_t ret;
    struct AdcIioDevice *adcDevice = NULL;

    if (device == NULL) {
        HDF_LOGE("AdcIioStart: device is null!");
        return HDF_ERR_INVALID_OBJECT;
    }
    adcDevice = (struct AdcIioDevice *)device;
    for (i = 0; i < adcDevice->channelNum; i++) {
        if (adcDevice->fp[i] != NULL) {
            continue;
        }
        adcDevice->fp[i] = filp_open(adcDevice->driverPathname[i], O_RDWR | O_NOCTTY | O_NDELAY, FILE_MODE);
        if (IS_ERR(adcDevice->fp[i])) {
            adcDevice->fp[i] = NULL;
            ret = AdcIioStop(device);
            if (ret != HDF_SUCCESS) {
                return ret;
            }
            HDF_LOGE("AdcIioStart: filp open fail!");
            return HDF_PLT_ERR_OS_API;
        }
    }

    return HDF_SUCCESS;
}
static const struct AdcMethod g_method = {
    .read = AdcIioRead,
    .stop = AdcIioStop,
    .start = AdcIioStart,
};

static int32_t AdcIioReadDrs(struct AdcIioDevice *adcDevice, const struct DeviceResourceNode *node)
{
    int32_t ret;
    const char *drName = NULL;
    char channelName[ADC_CHANNEL_NAME_LEN] = {0};
    struct DeviceResourceIface *drsOps = NULL;
    uint32_t i;

    drsOps = DeviceResourceGetIfaceInstance(HDF_CONFIG_SOURCE);
    if (drsOps == NULL || drsOps->GetUint32 == NULL || drsOps->GetString == NULL) {
        HDF_LOGE("AdcIioReadDrs: invalid drs ops!");
        return HDF_ERR_NOT_SUPPORT;
    }
    ret = drsOps->GetUint32(node, "deviceNum", &adcDevice->deviceNum, 0);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("AdcIioReadDrs: read deviceNum fail, ret: %d!", ret);
        return ret;
    }
    ret = drsOps->GetUint32(node, "channelNum", &adcDevice->channelNum, 1);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("AdcIioReadDrs: read channelNum fail, ret: %d!", ret);
        return ret;
    }
    if (adcDevice->channelNum > ADC_MAX_CHANNEL_NUM) {
        HDF_LOGE("AdcIioReadDrs: channelNum is illegal!");
        return HDF_FAILURE;
    }
    for (i = 0; i < adcDevice->channelNum; i++) {
        if (sprintf_s(channelName, ADC_CHANNEL_NAME_LEN - 1, "driver_channel%d_name", i) < 0) {
            return HDF_FAILURE;
        }
        ret = drsOps->GetString(node, channelName, &drName, NULL);
        if (ret != HDF_SUCCESS) {
            HDF_LOGE("AdcIioReadDrs: read driver_name fail, ret: %d!", ret);
            return ret;
        }
        adcDevice->driverPathname[i] = drName;
        adcDevice->fp[i] = NULL;
        drName = NULL;
    }

    ret = drsOps->GetUint32(node, "scanMode", &adcDevice->scanMode, 0);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("AdcIioReadDrs: read scanMode fail, ret: %d!", ret);
        return ret;
    }

    ret = drsOps->GetUint32(node, "rate", &adcDevice->rate, 0);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("AdcIioReadDrs: read rate fail, ret: %d!", ret);
        return ret;
    }
    
    return HDF_SUCCESS;
}

static int32_t AdcIioParseAndDeviceAdd(struct HdfDeviceObject *device, struct DeviceResourceNode *node)
{
    int32_t ret;
    struct AdcIioDevice *adcDevice = NULL;

    (void)device;
    adcDevice = (struct AdcIioDevice *)OsalMemCalloc(sizeof(*adcDevice));
    if (adcDevice == NULL) {
        HDF_LOGE("AdcIioParseAndDeviceAdd: alloc adcDevice fail!");
        return HDF_ERR_MALLOC_FAIL;
    }
    ret = AdcIioReadDrs(adcDevice, node);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("AdcIioParseAndDeviceAdd: read drs fail, ret: %d!", ret);
        OsalMemFree(adcDevice);
        return ret;
    }
    adcDevice->device.priv = (void *)node;
    adcDevice->device.devNum = adcDevice->deviceNum;
    adcDevice->device.ops = &g_method;

    ret = AdcDeviceAdd(&adcDevice->device);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("AdcIioParseAndDeviceAdd: add adc device:%u fail!", adcDevice->deviceNum);
        OsalMemFree(adcDevice);
        return ret;
    }
    return HDF_SUCCESS;
}

static int32_t LinuxAdcInit(struct HdfDeviceObject *device)
{
    int32_t ret = HDF_SUCCESS;
    struct DeviceResourceNode *childNode = NULL;

    if (device == NULL || device->property == NULL) {
        HDF_LOGE("LinuxAdcInit: device or property is null");
        return HDF_ERR_INVALID_OBJECT;
    }

    DEV_RES_NODE_FOR_EACH_CHILD_NODE(device->property, childNode) {
        ret = AdcIioParseAndDeviceAdd(device, childNode);
        if (ret != HDF_SUCCESS) {
            return ret;
        }
    }
    HDF_LOGI("LinuxAdcInit: adc iio init success!");

    return HDF_SUCCESS;
}

static void AdcIioRemoveByNode(const struct DeviceResourceNode *node)
{
    int32_t ret;
    int32_t deviceNum;
    struct AdcDevice *device = NULL;
    struct DeviceResourceIface *drsOps = NULL;

    drsOps = DeviceResourceGetIfaceInstance(HDF_CONFIG_SOURCE);
    if (drsOps == NULL || drsOps->GetUint32 == NULL) {
        HDF_LOGE("AdcIioRemoveByNode: invalid drs ops!");
        return;
    }

    ret = drsOps->GetUint32(node, "deviceNum", (uint32_t *)&deviceNum, 0);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("AdcIioRemoveByNode: read deviceNum fail, ret: %d!", ret);
        return;
    }

    device = AdcDeviceGet(deviceNum);
    if (device != NULL && device->priv == node) {
        ret = AdcIioStop(device);
        if (ret != HDF_SUCCESS) {
            HDF_LOGE("AdcIioRemoveByNode: close fail, ret: %d!", ret);
        }
        AdcDevicePut(device);
        AdcDeviceRemove(device);
        OsalMemFree(device);
    }
}

static void LinuxAdcRelease(struct HdfDeviceObject *device)
{
    const struct DeviceResourceNode *childNode = NULL;
    if (device == NULL || device->property == NULL) {
        HDF_LOGE("LinuxAdcRelease: device or property is null!");
        return;
    }
    DEV_RES_NODE_FOR_EACH_CHILD_NODE(device->property, childNode) {
        AdcIioRemoveByNode(childNode);
    }
}

struct HdfDriverEntry g_adcLinuxDriverEntry = {
    .moduleVersion = 1,
    .Bind = NULL,
    .Init = LinuxAdcInit,
    .Release = LinuxAdcRelease,
    .moduleName = "linux_adc_adapter",
};
HDF_INIT(g_adcLinuxDriverEntry);
