/*
 * clock driver adapter of linux
 *
 * Copyright (c) 2023 Huawei Device Co., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY;without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 */

#include <linux/clk.h>
#include <linux/of.h>
#include <linux/of_platform.h>

#include "device_resource_if.h"
#include "hdf_base.h"
#include "hdf_device_desc.h"
#include "hdf_log.h"
#include "osal_mem.h"
#include "securec.h"
#include "clock_core.h"

#define HDF_LOG_TAG clock_adapter_c

static int32_t ClockStart(struct ClockDevice *device)
{
    struct clk *clk = NULL;
    struct device_node *node = NULL;

    if (device->clk != NULL) {
        HDF_LOGI("device->clk already start\n");
        return HDF_SUCCESS;
    }

    if (IS_ERR_OR_NULL(device->deviceName)) {
        HDF_LOGE("ClockStart: deviceName IS_ERR_OR_NULL\n");
        return HDF_ERR_INVALID_PARAM;
    }

    node = of_find_node_by_path(device->deviceName);
    if (IS_ERR_OR_NULL(node)) {
        HDF_LOGE("ClockStart: can not get node \n");
        return HDF_ERR_INVALID_PARAM;
    }

    clk = of_clk_get_by_name(node, device->clockName);
    if (IS_ERR_OR_NULL(clk)) {
        HDF_LOGE("ClockStart: can not get clk \n");
        return HDF_ERR_INVALID_PARAM;
    }

    device->clk = clk;
    return HDF_SUCCESS;
}

static int32_t ClockLinuxSetRate(struct ClockDevice *device, uint32_t rate)
{
    struct clk *clk = NULL;
    int32_t ret;

    clk = device->clk;
    if (IS_ERR_OR_NULL(clk)) {
        HDF_LOGE("ClockLinuxSetRate: clk IS_ERR_OR_NULL\n");
        return HDF_FAILURE;
    }
    ret = clk_set_rate(clk, rate);
    return ret;
}

static int32_t ClockStop(struct ClockDevice *device)
{
    struct clk *clk = NULL;

    clk = device->clk;
    if (IS_ERR_OR_NULL(clk)) {
        HDF_LOGE("ClockStop: clk IS_ERR_OR_NULL\n");
        return HDF_ERR_INVALID_PARAM;
    }

    if (device->deviceName) {
        clk_put(clk);
    }

    device->clk = NULL;
    return HDF_SUCCESS;
}

static int32_t ClockLinuxGetRate(struct ClockDevice *device, uint32_t *rate)
{
    struct clk *clk = NULL;

    clk = device->clk;
    if (IS_ERR_OR_NULL(clk)) {
        HDF_LOGE("ClockLinuxGetRate: clk IS_ERR_OR_NULL\n");
        return HDF_ERR_INVALID_PARAM;
    }

    *rate = clk_get_rate(clk);
    return HDF_SUCCESS;
}

static int32_t ClockLinuxDisable(struct ClockDevice *device)
{
    struct clk *clk = NULL;

    clk = device->clk;
    if (IS_ERR_OR_NULL(clk)) {
        HDF_LOGE("ClockLinuxDisable: clk IS_ERR_OR_NULL\n");
        return HDF_ERR_INVALID_PARAM;
    }
    clk_disable_unprepare(clk);
    return HDF_SUCCESS;
}

static int32_t ClockLinuxEnable(struct ClockDevice *device)
{
    struct clk *clk = NULL;
    int32_t ret;

    clk = device->clk;
    if (IS_ERR_OR_NULL(clk)) {
        HDF_LOGE("ClockLinuxDisable: clk IS_ERR_OR_NULL\n");
        return HDF_ERR_INVALID_PARAM;
    }

    ret = clk_prepare_enable(clk);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("ClockLinuxDisable  ret = %d \n", ret);
    }

    return ret;
}
static struct ClockDevice *ClockLinuxGetParent(struct ClockDevice *device);


static int32_t ClockLinuxSetParent(struct ClockDevice *device, struct ClockDevice *parent)
{
    struct clk *clk = NULL;
    struct clk *clkParent = NULL;
    int32_t ret;
    if (device->parent == parent) {
        HDF_LOGI("ClockLinuxSetParent:device parent is not change \n");
        return HDF_SUCCESS;
    }

    clk = device->clk;
    if (IS_ERR_OR_NULL(clk)) {
        HDF_LOGE("ClockLinuxSetParent: clk IS_ERR_OR_NULL\n");
        return HDF_ERR_INVALID_PARAM;
    }

    clkParent = parent->clk;
    if (IS_ERR_OR_NULL(clkParent)) {
        HDF_LOGE("ClockLinuxSetParent: clkParent IS_ERR_OR_NULL\n");
        return HDF_ERR_INVALID_PARAM;
    }

    ret = clk_set_parent(clk, clkParent);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("ClockLinuxSetParent: clk_set_parent fail ret = %d\n", ret);
        return ret;
    }

    if (device->parent && device->parent->deviceName == NULL) {
        ClockDeviceRemove(device->parent);
        OsalMemFree(device->parent);
    }
    device->parent = parent;

    return ret;
}

static const struct ClockMethod g_method = {
    .start = ClockStart,
    .stop = ClockStop,
    .setRate = ClockLinuxSetRate,
    .getRate = ClockLinuxGetRate,
    .disable = ClockLinuxDisable,
    .enable = ClockLinuxEnable,
    .getParent = ClockLinuxGetParent,
    .setParent = ClockLinuxSetParent,
};

static struct ClockDevice *ClockLinuxGetParent(struct ClockDevice *device)
{
    struct clk *clk = NULL;
    struct clk *clkParent = NULL;
    struct ClockDevice *clockDevice = NULL;
    int32_t ret;

    clk = device->clk;
    if (IS_ERR_OR_NULL(clk)) {
        HDF_LOGE("ClockLinuxGetParent: clk IS_ERR_OR_NULL\n");
        return NULL;
    }

    clkParent = clk_get_parent(clk);
    if (IS_ERR_OR_NULL(clkParent)) {
        HDF_LOGE("ClockLinuxGetParent: can not get clkParent \n");
        return NULL;
    }

    if (device->parent != NULL) {
        device->parent->clk = clkParent;
        return device->parent;
    }

    clockDevice = (struct ClockDevice *)OsalMemCalloc(sizeof(*clockDevice));
    if (clockDevice == NULL) {
        HDF_LOGE("ClockLinuxGetParent: can not OsalMemCalloc clockDevice \n");
        return NULL;
    }

    clockDevice->ops = &g_method;
    ret = ClockManagerGetAIdleDeviceId();
    if (ret < 0) {
        HDF_LOGE("ClockLinuxGetParent: add clock device:%d device if full fail!", ret);
        OsalMemFree(clockDevice);
        return NULL;
    }

    clockDevice->deviceIndex = ret;
    clockDevice->clk = clkParent;

    ret = ClockDeviceAdd(clockDevice);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("ClockLinuxGetParent: add clock device:%u fail!", clockDevice->deviceIndex);
        OsalMemFree(clockDevice);
        return NULL;
    }
    device->parent = clockDevice;

    return clockDevice;
}

static int32_t ClockReadDrs(struct ClockDevice *clockDevice, const struct DeviceResourceNode *node)
{
    int32_t ret;
    struct DeviceResourceIface *drsOps = NULL;

    drsOps = DeviceResourceGetIfaceInstance(HDF_CONFIG_SOURCE);
    if (drsOps == NULL || drsOps->GetUint32 == NULL || drsOps->GetString == NULL) {
        HDF_LOGE("ClockReadDrs: invalid drs ops!");
        return HDF_ERR_NOT_SUPPORT;
    }
    ret = drsOps->GetUint32(node, "deviceIndex", &clockDevice->deviceIndex, 0);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("ClockReadDrs: read deviceIndex fail, ret: %d!", ret);
        return ret;
    }

    drsOps->GetString(node, "clockName", &clockDevice->clockName, 0);

    ret = drsOps->GetString(node, "deviceName", &clockDevice->deviceName, 0);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("ClockReadDrs: read deviceName fail, ret: %d!", ret);
        return ret;
    }
    return HDF_SUCCESS;
}

static int32_t ClockParseAndDeviceAdd(struct HdfDeviceObject *device, struct DeviceResourceNode *node)
{
    int32_t ret;
    struct ClockDevice *clockDevice = NULL;

    (void)device;
    clockDevice = (struct ClockDevice *)OsalMemCalloc(sizeof(*clockDevice));
    if (clockDevice == NULL) {
        HDF_LOGE("ClockParseAndDeviceAdd: alloc clockDevice fail!");
        return HDF_ERR_MALLOC_FAIL;
    }
    ret = ClockReadDrs(clockDevice, node);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("ClockParseAndDeviceAdd: read drs fail, ret: %d!", ret);
        OsalMemFree(clockDevice);
        return ret;
    }

    clockDevice->priv = (void *)node;
    clockDevice->ops = &g_method;

    ret = ClockDeviceAdd(clockDevice);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("ClockParseAndDeviceAdd: add clock device:%u fail!", clockDevice->deviceIndex);
        OsalMemFree(clockDevice);
        return ret;
    }

    return HDF_SUCCESS;
}

static int32_t LinuxClockInit(struct HdfDeviceObject *device)
{
    int32_t ret = HDF_SUCCESS;
    struct DeviceResourceNode *childNode = NULL;

    if (device == NULL || device->property == NULL) {
        HDF_LOGE("LinuxClockInit: device or property is null");
        return HDF_ERR_INVALID_OBJECT;
    }

    DEV_RES_NODE_FOR_EACH_CHILD_NODE(device->property, childNode) {
        ret = ClockParseAndDeviceAdd(device, childNode);
        if (ret != HDF_SUCCESS) {
            HDF_LOGE("LinuxClockInit: clock init fail!");
            return ret;
        }
    }
    HDF_LOGE("LinuxClockInit: clock init success!");

    return HDF_SUCCESS;
}

static void ClockRemoveByNode(const struct DeviceResourceNode *node)
{
    int32_t ret;
    int32_t deviceIndex;
    struct ClockDevice *device = NULL;
    struct DeviceResourceIface *drsOps = NULL;

    drsOps = DeviceResourceGetIfaceInstance(HDF_CONFIG_SOURCE);
    if (drsOps == NULL || drsOps->GetUint32 == NULL) {
        HDF_LOGE("ClockRemoveByNode: invalid drs ops!");
        return;
    }

    ret = drsOps->GetUint32(node, "deviceIndex", (uint32_t *)&deviceIndex, 0);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("ClockRemoveByNode: read deviceIndex fail, ret: %d!", ret);
        return;
    }

    device = ClockDeviceGet(deviceIndex);
    if (device != NULL && device->priv == node) {
        ret = ClockStop(device);
        if (ret != HDF_SUCCESS) {
            HDF_LOGE("ClockRemoveByNode: close fail, ret: %d!", ret);
        }
        if (device->parent  && device->parent->deviceName == NULL) {
            ClockDeviceRemove(device->parent);
            OsalMemFree(device->parent);
        }
        ClockDeviceRemove(device);
        OsalMemFree(device);
    }
}

static void LinuxClockRelease(struct HdfDeviceObject *device)
{
    const struct DeviceResourceNode *childNode = NULL;
    if (device == NULL || device->property == NULL) {
        HDF_LOGE("LinuxClockRelease: device or property is null!");
        return;
    }
    DEV_RES_NODE_FOR_EACH_CHILD_NODE(device->property, childNode) {
        ClockRemoveByNode(childNode);
    }
}

struct HdfDriverEntry g_clockLinuxDriverEntry = {
    .moduleVersion = 1,
    .Bind = NULL,
    .Init = LinuxClockInit,
    .Release = LinuxClockRelease,
    .moduleName = "linux_clock_adapter",
};
HDF_INIT(g_clockLinuxDriverEntry);
