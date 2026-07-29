/*
 * Copyright (c) 2022-2023 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include <math.h>
#include "adc/adc_core.h"
#include "device_resource_if.h"
#include "hdf_device_desc.h"
#include "hdf_log.h"
#include "osal_mem.h"

#define HDF_LOG_TAG adc_virtual_driver
#define MATCH_ATTR  adc_virtual
#define BASE_NUMBER         2
#define ADC_MAX_DATA_WIDTH  24

struct VirtualAdcDevice {
    struct AdcDevice device;
    uint32_t devNum;
    uint32_t dataWidth;
    uint32_t channels;
    uint32_t maxValue;
};

static int32_t VirtualAdcStart(struct AdcDevice *device)
{
    struct VirtualAdcDevice *virtual = NULL;

    if (device == NULL) {
        HDF_LOGE("VirtualAdcStart: device is null!");
        return HDF_ERR_INVALID_OBJECT;
    }

    virtual = (struct VirtualAdcDevice *)device;
    virtual->maxValue = pow(BASE_NUMBER, virtual->dataWidth) - 1;

    return HDF_SUCCESS;
}

static int32_t VirtualAdcRead(struct AdcDevice *device, uint32_t channel, uint32_t *val)
{
    struct VirtualAdcDevice *virtual = NULL;

    if (device == NULL) {
        HDF_LOGE("VirtualAdcRead: device is null!");
        return HDF_ERR_INVALID_OBJECT;
    }

    virtual = (struct VirtualAdcDevice *)device;
    if (channel >= virtual->channels || val == NULL) {
        HDF_LOGE("VirtualAdcRead: invalid channel or val is null!");
        return HDF_ERR_INVALID_PARAM;
    }

    *val = virtual->maxValue;
    return HDF_SUCCESS;
}

static int32_t VirtualAdcStop(struct AdcDevice *device)
{
    (void)device;
    return HDF_SUCCESS;
}

static const struct AdcMethod g_method = {
    .read = VirtualAdcRead,
    .stop = VirtualAdcStop,
    .start = VirtualAdcStart,
};

static int32_t VirtualAdcReadDrs(struct VirtualAdcDevice *virtual, const struct DeviceResourceNode *node)
{
    int32_t ret;
    struct DeviceResourceIface *drsOps = NULL;

    drsOps = DeviceResourceGetIfaceInstance(HDF_CONFIG_SOURCE);
    if (drsOps == NULL || drsOps->GetUint32 == NULL) {
        HDF_LOGE("VirtualAdcReadDrs: invalid drs ops!");
        return HDF_ERR_NOT_SUPPORT;
    }

    ret = drsOps->GetUint32(node, "devNum", &virtual->devNum, 0);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("VirtualAdcReadDrs: read devNum fail, ret: %d!", ret);
        return ret;
    }

    ret = drsOps->GetUint32(node, "dataWidth", &virtual->dataWidth, 0);
    if (ret != HDF_SUCCESS || virtual->dataWidth == 0 || virtual->dataWidth >= ADC_MAX_DATA_WIDTH) {
        HDF_LOGE("VirtualAdcReadDrs: read dataWidth fail, ret: %d!", ret);
        return ret;
    }

    ret = drsOps->GetUint32(node, "channels", &virtual->channels, 0);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("VirtualAdcReadDrs: read channels fail, ret: %d!", ret);
        return ret;
    }
    HDF_LOGD("VirtualAdcReadDrs: success, devNum=%u, dataWidth=%u, channels=%u!",
        virtual->devNum, virtual->dataWidth, virtual->channels);

    return HDF_SUCCESS;
}

static int32_t VirtualAdcInit(struct HdfDeviceObject *device)
{
    int32_t ret;
    struct VirtualAdcDevice *virtual = NULL;

    if (device == NULL || device->property == NULL) {
        HDF_LOGE("VirtualAdcInit: device or property is null!");
        return HDF_ERR_INVALID_OBJECT;
    }

    virtual = (struct VirtualAdcDevice *)OsalMemCalloc(sizeof(*virtual));
    if (virtual == NULL) {
        HDF_LOGE("VirtualAdcInit: alloc virtual fail!");
        return HDF_ERR_MALLOC_FAIL;
    }

    ret = VirtualAdcReadDrs(virtual, device->property);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("VirtualAdcInit: read drs fail, ret: %d!", ret);
        OsalMemFree(virtual);
        return ret;
    }

    virtual->device.priv = (void *)device->property;
    virtual->device.devNum = virtual->devNum;
    virtual->device.ops = &g_method;
    ret = AdcDeviceAdd(&virtual->device);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("VirtualAdcInit: add adc virtual device:%u fail, ret: %d!", virtual->devNum, ret);
        OsalMemFree(virtual);
        return ret;
    }

    HDF_LOGI("VirtualAdcInit: adc virtual driver init success!");
    return ret;
}

static void VirtualAdcRelease(struct HdfDeviceObject *device)
{
    int32_t ret;
    uint32_t devNum;
    struct AdcDevice *dev = NULL;
    struct DeviceResourceIface *drsOps = NULL;

    HDF_LOGI("VirtualAdcRelease: enter!");
    if (device == NULL || device->property == NULL) {
        HDF_LOGE("VirtualAdcRelease: device or property is null!");
        return;
    }

    drsOps = DeviceResourceGetIfaceInstance(HDF_CONFIG_SOURCE);
    if (drsOps == NULL || drsOps->GetUint32 == NULL) {
        HDF_LOGE("VirtualAdcRelease: invalid drs ops!");
        return;
    }

    ret = drsOps->GetUint32(device->property, "devNum", (uint32_t *)&devNum, 0);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("VirtualAdcRelease: read devNum fail, ret: %d!", ret);
        return;
    }

    dev = AdcDeviceGet(devNum);
    AdcDevicePut(dev);
    if (dev != NULL && dev->priv == device->property) {
        AdcDeviceRemove(dev);
        OsalMemFree(dev);
    }
}

static struct HdfDriverEntry g_virtualAdcDriverEntry = {
    .moduleVersion = 1,
    .Init = VirtualAdcInit,
    .Release = VirtualAdcRelease,
    .moduleName = "virtual_adc_driver",
};
HDF_INIT(g_virtualAdcDriverEntry);
