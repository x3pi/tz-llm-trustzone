/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include "clock/clock_core.h"
#include "device_resource_if.h"
#include "hdf_device_desc.h"
#include "hdf_log.h"
#include "osal_mem.h"
#include "securec.h"
#include "hdf_base.h"

#define HDF_LOG_TAG clock_Virtual_driver

int32_t VirtualClockStart(struct ClockDevice *device)
{
    if (device->clk != NULL) {
        HDF_LOGI("device->clk already start\n");
        return HDF_SUCCESS;
    }

    device->clk = (void *)&device->deviceIndex;
    return HDF_SUCCESS;
}

int32_t VirtualClockSetRate(struct ClockDevice *device, uint32_t rate)
{
    if (device->clk == NULL) {
        HDF_LOGE("VirtualClockSetRate: clk IS_ERR_OR_NULL\n");
        return HDF_FAILURE;
    }

    return HDF_SUCCESS;
}

int32_t VirtualClockStop(struct ClockDevice *device)
{
    if (device->clk == NULL) {
        HDF_LOGE("VirtualClockStop: clk IS_ERR_OR_NULL\n");
        return HDF_ERR_INVALID_PARAM;
    }

    device->clk = NULL;
    return HDF_SUCCESS;
}

int32_t VirtualClockGetRate(struct ClockDevice *device, uint32_t *rate)
{
    if (device->clk == NULL) {
        HDF_LOGE("VirtualClockGetRate: clk IS_ERR_OR_NULL\n");
        return HDF_ERR_INVALID_PARAM;
    }

    return HDF_SUCCESS;
}

int32_t VirtualClockDisable(struct ClockDevice *device)
{
    if (device->clk == NULL) {
        HDF_LOGE("VirtualClockDisable: clk IS_ERR_OR_NULL\n");
        return HDF_ERR_INVALID_PARAM;
    }

    return HDF_SUCCESS;
}

int32_t VirtualClockEnable(struct ClockDevice *device)
{
    if (device->clk == NULL) {
        HDF_LOGE("VirtualClockEnable: clk IS_ERR_OR_NULL\n");
        return HDF_ERR_INVALID_PARAM;
    }

    return HDF_SUCCESS;
}

struct ClockDevice *VirtualClockGetParent(struct ClockDevice *device);

int32_t VirtualClockSetParent(struct ClockDevice *device, struct ClockDevice *parent)
{
    if (device->parent == parent) {
        HDF_LOGI("ClockSetParent:device parent is not change \n");
        return HDF_SUCCESS;
    }

    if (device->clk == NULL) {
        HDF_LOGE("ClockSetParent: clk IS_ERR_OR_NULL\n");
        return HDF_ERR_INVALID_PARAM;
    }

    if (parent->clk == NULL) {
        HDF_LOGE("ClockSetParent: clk_parent IS_ERR_OR_NULL\n");
        return HDF_ERR_INVALID_PARAM;
    }

    if (device->parent && device->parent->deviceName == NULL) {
        ClockDeviceRemove(device->parent);
        OsalMemFree(device->parent);
    }
    device->parent = parent;

    return HDF_SUCCESS;
}

static const struct ClockMethod g_method = {
    .start = VirtualClockStart,
    .stop = VirtualClockStop,
    .setRate = VirtualClockSetRate,
    .getRate = VirtualClockGetRate,
    .disable = VirtualClockDisable,
    .enable = VirtualClockEnable,
    .getParent = VirtualClockGetParent,
    .setParent = VirtualClockSetParent,
};

struct ClockDevice *VirtualClockGetParent(struct ClockDevice *device)
{
    int32_t ret;
    struct ClockDevice *clockDevice = NULL;
    if (device->clk == NULL) {
        HDF_LOGE("ClockGetParent: clk IS_ERR_OR_NULL\n");
        return NULL;
    }

    if (device->parent != NULL) {
        device->parent->clk = device->clk;
        return device->parent;
    }

    clockDevice = (struct ClockDevice *)OsalMemCalloc(sizeof(*clockDevice));
    if (clockDevice == NULL) {
        HDF_LOGE("ClockGetParent: can not OsalMemCalloc clockDevice \n");
        return NULL;
    }

    clockDevice->ops = &g_method;
    ret = ClockManagerGetAIdleDeviceId();
    if (ret < 0) {
        HDF_LOGE("ClockGetParent: add clock device:%d device if full fail!", ret);
        OsalMemFree(clockDevice);
        return NULL;
    }

    clockDevice->deviceIndex = ret;
    clockDevice->clk = device->clk;

    ret = ClockDeviceAdd(clockDevice);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("ClockGetParent: add clock device:%u fail!", clockDevice->deviceIndex);
        OsalMemFree(clockDevice);
        return NULL;
    }
    device->parent = clockDevice;

    return clockDevice;
}

struct VirtualClockDevice {
    struct ClockDevice device;
    uint32_t deviceIndex;
    const char *clockName;
    const char *deviceName;
};

static int32_t VirtualClockReadDrs(struct VirtualClockDevice *virtual, const struct DeviceResourceNode *node)
{
    int32_t ret;
    struct DeviceResourceIface *drsOps = NULL;

    drsOps = DeviceResourceGetIfaceInstance(HDF_CONFIG_SOURCE);
    if (drsOps == NULL || drsOps->GetUint32 == NULL || drsOps->GetString == NULL) {
        HDF_LOGE("VirtualClockReadDrs: invalid drs ops!");
        return HDF_ERR_NOT_SUPPORT;
    }
    ret = drsOps->GetUint32(node, "deviceIndex", &virtual->deviceIndex, 0);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("VirtualClockReadDrs: read deviceIndex fail, ret: %d!", ret);
        return ret;
    }

    return HDF_SUCCESS;
}

static int32_t VirtualClockInit(struct HdfDeviceObject *device)
{
    int32_t ret;
    struct VirtualClockDevice *virtual = NULL;

    if (device == NULL || device->property == NULL) {
        HDF_LOGE("VirtualClockInit: device or property is null!");
        return HDF_ERR_INVALID_OBJECT;
    }

    virtual = (struct VirtualClockDevice *)OsalMemCalloc(sizeof(*virtual));
    if (virtual == NULL) {
        HDF_LOGE("VirtualClockInit: alloc virtual fail!");
        return HDF_ERR_MALLOC_FAIL;
    }

    ret = VirtualClockReadDrs(virtual, device->property);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("VirtualClockInit: read drs fail, ret: %d!", ret);
        OsalMemFree(virtual);
        return ret;
    }

    virtual->device.priv = (void *)device->property;
    virtual->device.deviceIndex = virtual->deviceIndex;
    virtual->device.ops = &g_method;
    ret = ClockDeviceAdd(&virtual->device);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("VirtualClockInit: clock clock virtual device:%u fail, ret: %d!", virtual->deviceIndex, ret);
        OsalMemFree(virtual);
        return ret;
    }

    HDF_LOGI("VirtualClockInit: clock virtual driver init success!");
    return ret;
}

static void VirtualClockRelease(struct HdfDeviceObject *device)
{
    int32_t ret;
    uint32_t deviceIndex;
    struct ClockDevice *dev = NULL;
    struct DeviceResourceIface *drsOps = NULL;

    HDF_LOGI("VirtualClockRelease: enter!");
    if (device == NULL || device->property == NULL) {
        HDF_LOGE("VirtualClockRelease: device or property is null!");
        return;
    }

    drsOps = DeviceResourceGetIfaceInstance(HDF_CONFIG_SOURCE);
    if (drsOps == NULL || drsOps->GetUint32 == NULL) {
        HDF_LOGE("VirtualClockRelease: invalid drs ops!");
        return;
    }

    ret = drsOps->GetUint32(device->property, "deviceIndex", (uint32_t *)&deviceIndex, 0);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("VirtualClockRelease: read devNum fail, ret: %d!", ret);
        return;
    }

    dev = ClockDeviceGet(deviceIndex);
    if (dev != NULL && dev->priv == device->property) {
        ret = VirtualClockStop(dev);
        if (ret != HDF_SUCCESS) {
            HDF_LOGE("ClockRemoveByNode: close fail, ret: %d!", ret);
        }
        ClockDeviceRemove(dev);
        OsalMemFree(dev);
    }
}

static struct HdfDriverEntry g_VirtualClockDriverEntry = {
    .moduleVersion = 1,
    .Init = VirtualClockInit,
    .Release = VirtualClockRelease,
    .moduleName = "virtual_clock_driver",
};
HDF_INIT(g_VirtualClockDriverEntry);
