/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include "clock_test.h"
#include "device_resource_if.h"
#include "hdf_base.h"
#include "hdf_device_desc.h"
#include "hdf_log.h"

#define HDF_LOG_TAG clock_test_driver_c

static struct ClockTestConfig g_config;

static int32_t ClockTestDispatch(struct HdfDeviceIoClient *client, int cmd, struct HdfSBuf *data, struct HdfSBuf *reply)
{
    (void)data;
    (void)client;

    if (cmd == 0) {
        if (reply == NULL) {
            HDF_LOGE("ClockTestDispatch: reply is null!");
            return HDF_ERR_INVALID_PARAM;
        }
        if (!HdfSbufWriteBuffer(reply, &g_config, sizeof(g_config))) {
            HDF_LOGE("ClockTestDispatch: write reply fail!");
            return HDF_ERR_IO;
        }
    } else {
        HDF_LOGE("ClockTestDispatch: cmd %d is not support!", cmd);
        return HDF_ERR_NOT_SUPPORT;
    }

    return HDF_SUCCESS;
}

static int32_t ClockTestReadConfig(struct ClockTestConfig *config, const struct DeviceResourceNode *node)
{
    int32_t ret;
    struct DeviceResourceIface *drsOps = NULL;

    drsOps = DeviceResourceGetIfaceInstance(HDF_CONFIG_SOURCE);
    if (drsOps == NULL || drsOps->GetUint32 == NULL) {
        HDF_LOGE("ClockTestReadConfig: invalid drs ops!");
        return HDF_FAILURE;
    }

    ret = drsOps->GetUint32(node, "deviceIndex", &config->deviceIndex, 0);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("ClockTestReadConfig: read deviceIndex fail!");
        return ret;
    }

    return HDF_SUCCESS;
}

static int32_t ClockTestBind(struct HdfDeviceObject *device)
{
    int32_t ret;
    static struct IDeviceIoService service;

    if (device == NULL || device->property == NULL) {
        HDF_LOGE("ClockTestBind: device or config is null!");
        return HDF_ERR_IO;
    }

    ret = ClockTestReadConfig(&g_config, device->property);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("ClockTestBind: read config fail!");
        return ret;
    }

    service.Dispatch = ClockTestDispatch;
    device->service = &service;
    HDF_LOGI("ClockTestBind: done!");
    return HDF_SUCCESS;
}

static int32_t ClockTestInit(struct HdfDeviceObject *device)
{
    (void)device;
    HDF_LOGI("ClockTestInit: done!");
    return HDF_SUCCESS;
}

static void ClockTestRelease(struct HdfDeviceObject *device)
{
    if (device != NULL) {
        device->service = NULL;
    }
    HDF_LOGI("ClockTestRelease: done!");
    return;
}

struct HdfDriverEntry g_clockTestEntry = {
    .moduleVersion = 1,
    .Bind = ClockTestBind,
    .Init = ClockTestInit,
    .Release = ClockTestRelease,
    .moduleName = "PLATFORM_CLOCK_TEST",
};
HDF_INIT(g_clockTestEntry);
