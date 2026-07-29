/*
 * Copyright (c) 2021-2023 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include "i3c_test.h"
#include "device_resource_if.h"
#include "hdf_base.h"
#include "hdf_device_desc.h"
#include "hdf_log.h"

#define HDF_LOG_TAG i3c_driver_test_c

static struct I3cTestConfig g_config;

static int32_t I3cTestDispatch(struct HdfDeviceIoClient *client, int cmd, struct HdfSBuf *data, struct HdfSBuf *reply)
{
    HDF_LOGD("I3cTestDispatch: enter!");

    (void)client;
    (void)data;
    if (cmd == 0) {
        if (reply == NULL) {
            HDF_LOGE("I3cTestDispatch: reply is null!");
            return HDF_ERR_INVALID_PARAM;
        }
        if (!HdfSbufWriteBuffer(reply, &g_config, sizeof(g_config))) {
            HDF_LOGE("I3cTestDispatch: write reply fail!");
            return HDF_ERR_IO;
        }
    } else {
        HDF_LOGE("I3cTestDispatch: cmd %d is not support!", cmd);
        return HDF_ERR_NOT_SUPPORT;
    }

    return HDF_SUCCESS;
}

static int32_t I3cTestReadConfig(struct I3cTestConfig *config, const struct DeviceResourceNode *node)
{
    int32_t ret;
    struct DeviceResourceIface *drsOps = NULL;

    drsOps = DeviceResourceGetIfaceInstance(HDF_CONFIG_SOURCE);
    if (drsOps == NULL || drsOps->GetUint16 == NULL) {
        HDF_LOGE("I3cTestReadConfig: invalid drs ops!");
        return HDF_FAILURE;
    }

    ret = drsOps->GetUint16(node, "busId", &config->busId, 0);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("I3cTestReadConfig: read busId fail!");
        return ret;
    }

    ret = drsOps->GetUint16(node, "devAddr", &config->devAddr, 0);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("I3cTestReadConfig: read dev addr fail!");
        return ret;
    }

    ret = drsOps->GetUint16(node, "regLen", &config->regLen, 1);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("I3cTestReadConfig: read reg len fail!");
        return ret;
    }

    ret = drsOps->GetUint16(node, "regAddr", &config->regAddr, 0);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("I3cTestReadConfig: read reg addr fail!");
        return ret;
    }

    ret = drsOps->GetUint16(node, "bufSize", &config->bufSize, 0);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("I3cTestReadConfig: read buf size fail!");
        return ret;
    }
    return HDF_SUCCESS;
}

static int32_t I3cTestBind(struct HdfDeviceObject *device)
{
    int32_t ret;
    static struct IDeviceIoService service;

    if (device == NULL || device->property == NULL) {
        HDF_LOGE("I3cTestBind: device or config is null!");
        return HDF_ERR_IO;
    }

    ret = I3cTestReadConfig(&g_config, device->property);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("I3cTestBind: read config fail!");
        return ret;
    }
    service.Dispatch = I3cTestDispatch;
    device->service = &service;
    HDF_LOGI("I3cTestBind: done!");
    return HDF_SUCCESS;
}

static int32_t I3cTestInit(struct HdfDeviceObject *device)
{
    (void)device;
    HDF_LOGI("I3cTestInit: done!");
    return HDF_SUCCESS;
}

static void I3cTestRelease(struct HdfDeviceObject *device)
{
    if (device != NULL) {
        device->service = NULL;
    }
    HDF_LOGI("I3cTestRelease: done!");
    return;
}

struct HdfDriverEntry g_i3cTestEntry = {
    .moduleVersion = 1,
    .Bind = I3cTestBind,
    .Init = I3cTestInit,
    .Release = I3cTestRelease,
    .moduleName = "PLATFORM_I3C_TEST",
};
HDF_INIT(g_i3cTestEntry);
