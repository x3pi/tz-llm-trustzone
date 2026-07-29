/*
 * Copyright (c) 2022-2023 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include "watchdog_test.h"
#include "device_resource_if.h"
#include "hdf_base.h"
#include "hdf_device_desc.h"
#include "hdf_log.h"

#define HDF_LOG_TAG watchdog_driver_test_c

static struct WatchdogTestConfig g_config;

static int32_t WatchdogTestDispatch(struct HdfDeviceIoClient *client, int cmd,
    struct HdfSBuf *data, struct HdfSBuf *reply)
{
    (void)client;
    (void)data;
    if (cmd == 0) {
        if (reply == NULL) {
            HDF_LOGE("WatchdogTestDispatch: reply is null!");
            return HDF_ERR_INVALID_PARAM;
        }
        if (!HdfSbufWriteBuffer(reply, &g_config, sizeof(g_config))) {
            HDF_LOGE("WatchdogTestDispatch: write reply fail!");
            return HDF_ERR_IO;
        }
    } else {
        HDF_LOGE("WatchdogTestDispatch: cmd: %d is not support!", cmd);
        return HDF_ERR_NOT_SUPPORT;
    }

    return HDF_SUCCESS;
}

static int32_t WatchdogTestReadConfig(struct WatchdogTestConfig *config, const struct DeviceResourceNode *node)
{
    int32_t ret;
    struct DeviceResourceIface *drsOps = NULL;
    uint32_t temp;

    drsOps = DeviceResourceGetIfaceInstance(HDF_CONFIG_SOURCE);
    if (drsOps == NULL || drsOps->GetUint32 == NULL) {
        HDF_LOGE("WatchdogTestReadConfig: invalid drs ops!");
        return HDF_FAILURE;
    }

    ret = drsOps->GetUint32(node, "id", &temp, 0);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("WatchdogTestReadConfig: read id fail, ret: %d!", ret);
        return ret;
    }
    config->id = temp;

    ret = drsOps->GetUint32(node, "timeoutSet", &config->timeoutSet, 0);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("WatchdogTestReadConfig: read timeoutSet fail, ret: %d!", ret);
        return ret;
    }

    ret = drsOps->GetUint32(node, "feedTime", &config->feedTime, 0);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("WatchdogTestReadConfig: read feedTime fail, ret: %d!", ret);
        return ret;
    }

    return HDF_SUCCESS;
}

static int32_t WatchdogTestBind(struct HdfDeviceObject *device)
{
    int32_t ret;
    static struct IDeviceIoService service;

    if (device == NULL || device->property == NULL) {
        HDF_LOGE("WatchdogTestBind: device or config is null!");
        return HDF_ERR_IO;
    }
    ret = WatchdogTestReadConfig(&g_config, device->property);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("WatchdogTestBind: read config fail, ret: %d!", ret);
        return ret;
    }
    service.Dispatch = WatchdogTestDispatch;
    device->service = &service;
    return HDF_SUCCESS;
}

static int32_t WatchdogTestInit(struct HdfDeviceObject *device)
{
    (void)device;
    return HDF_SUCCESS;
}

static void WatchdogTestRelease(struct HdfDeviceObject *device)
{
    if (device != NULL) {
        device->service = NULL;
    }
    HDF_LOGI("WatchdogTestRelease: done!");
    return;
}

struct HdfDriverEntry g_watchdogTestEntry = {
    .moduleVersion = 1,
    .Bind = WatchdogTestBind,
    .Init = WatchdogTestInit,
    .Release = WatchdogTestRelease,
    .moduleName = "PLATFORM_WATCHDOG_TEST",
};
HDF_INIT(g_watchdogTestEntry);