/*
 * Copyright (c) 2021-2023 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include "device_resource_if.h"
#include "hdf_base.h"
#include "hdf_device_desc.h"
#include "hdf_log.h"
#include "pin_test.h"
#include "securec.h"
#include "string.h"

#define HDF_LOG_TAG pin_driver_test_c

static struct PinTestConfig g_config;

static int32_t PinTestDispatch(struct HdfDeviceIoClient *client, int cmd, struct HdfSBuf *data, struct HdfSBuf *reply)
{
    (void)client;
    (void)data;
    if (cmd == 0) {
        if (reply == NULL) {
            HDF_LOGE("PinTestDispatch: reply is null!");
            return HDF_ERR_INVALID_PARAM;
        }
        if (!HdfSbufWriteBuffer(reply, &g_config, sizeof(g_config))) {
            HDF_LOGE("PinTestDispatch: write config fail!");
            return HDF_ERR_IO;
        }
    } else {
        HDF_LOGE("PinTestDispatch: cmd %d is not support!", cmd);
        return HDF_ERR_NOT_SUPPORT;
    }

    return HDF_SUCCESS;
}
static int32_t PinTestReadConfig(struct PinTestConfig *config, const struct DeviceResourceNode *node)
{
    int32_t ret;
    const char *funcName = NULL;
    struct DeviceResourceIface *drsOps = NULL;

    drsOps = DeviceResourceGetIfaceInstance(HDF_CONFIG_SOURCE);
    if (drsOps == NULL || drsOps->GetUint32 == NULL || drsOps->GetString == NULL) {
        HDF_LOGE("PinTestReadConfig: invalid drs ops!");
        return HDF_FAILURE;
    }
    ret = drsOps->GetString(node, "pinName", &config->pinName, 0);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("PinTestReadConfig: read pinName fail!");
        return ret;
    }

    ret = drsOps->GetUint32(node, "strengthNum", &config->strengthNum, 0);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("PinTestReadConfig: read StrengthNum fail!");
        return ret;
    }

    ret = drsOps->GetUint32(node, "PullTypeNum", &config->PullTypeNum, 0);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("PinTestReadConfig: read PullTypeNum fail!");
        return ret;
    }

    ret = drsOps->GetString(node, "funcName", &funcName, 0);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("PinTestReadConfig: read funcName fail!");
        return ret;
    }
    if (strcpy_s(config->funcNameBuf, NAME_SIZE_MAX, funcName) != EOK) {
        HDF_LOGE("PinTestReadConfig: copy funcNameBuf fail!");
        return HDF_FAILURE;
    }

    if (strcpy_s(config->pinNameBuf, NAME_SIZE_MAX, config->pinName) != EOK) {
        HDF_LOGE("PinTestReadConfig: copy pinNameBuf fail!");
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

static int32_t PinTestBind(struct HdfDeviceObject *device)
{
    int32_t ret;
    static struct IDeviceIoService service;

    if (device == NULL || device->property == NULL) {
        HDF_LOGE("PinTestBind: device or config is null!");
        return HDF_ERR_IO;
    }
    ret = PinTestReadConfig(&g_config, device->property);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("PinTestBind: read config fail!");
        return ret;
    }
    service.Dispatch = PinTestDispatch;
    device->service = &service;
    return HDF_SUCCESS;
}

static int32_t PinTestInit(struct HdfDeviceObject *device)
{
    (void)device;
    return HDF_SUCCESS;
}

static void PinTestRelease(struct HdfDeviceObject *device)
{
    if (device != NULL) {
        device->service = NULL;
    }
    HDF_LOGI("PinTestRelease: done!");
    return;
}

struct HdfDriverEntry g_pinTestEntry = {
    .moduleVersion = 1,
    .Bind = PinTestBind,
    .Init = PinTestInit,
    .Release = PinTestRelease,
    .moduleName = "PLATFORM_PIN_TEST",
};
HDF_INIT(g_pinTestEntry);