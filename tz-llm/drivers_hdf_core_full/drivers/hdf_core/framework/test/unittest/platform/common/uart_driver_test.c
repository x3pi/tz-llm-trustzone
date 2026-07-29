/*
 * Copyright (c) 2022-2023 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */
 
#include "uart_test.h"
#include "device_resource_if.h"
#include "hdf_base.h"
#include "hdf_device_desc.h"
#include "hdf_log.h"
#include "osal_mem.h"

static struct UartTestConfig g_config;

static int32_t UartTestDispatch(struct HdfDeviceIoClient *client, int cmd, struct HdfSBuf *data, struct HdfSBuf *reply)
{
    (void)client;
    (void)data;
    if (cmd == 0) {
        if (reply == NULL) {
            HDF_LOGE("UartTestDispatch: reply is null!");
            return HDF_ERR_INVALID_PARAM;
        }
        if (!HdfSbufWriteUint32(reply, g_config.port)) {
            HDF_LOGE("UartTestDispatch: write port fail!");
            return HDF_ERR_IO;
        }
        if (!HdfSbufWriteUint32(reply, g_config.len)) {
            HDF_LOGE("UartTestDispatch: write len fail!");
            return HDF_ERR_IO;
        }
        if (!HdfSbufWriteBuffer(reply, g_config.wbuf, g_config.len)) {
            HDF_LOGE("UartTestDispatch: write config wbuf fail!");
            return HDF_ERR_IO;
        }
    } else {
        HDF_LOGE("UartTestDispatch: cmd: %d is not support!", cmd);
        return HDF_ERR_NOT_SUPPORT;
    }
    return HDF_SUCCESS;
}

static int32_t UartTestReadConfig(struct UartTestConfig *config, const struct DeviceResourceNode *node)
{
    int32_t ret;
    int32_t i;
    uint32_t *tmp = NULL;
    struct DeviceResourceIface *drsOps = NULL;

    drsOps = DeviceResourceGetIfaceInstance(HDF_CONFIG_SOURCE);
    if (drsOps == NULL || drsOps->GetUint32 == NULL || drsOps->GetUint32Array == NULL) {
        HDF_LOGE("UartTestReadConfig: invalid drs ops fail!");
        return HDF_FAILURE;
    }
    ret = drsOps->GetUint32(node, "port", &config->port, 0);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("UartTestReadConfig: read port fail, ret: %d!", ret);
        return ret;
    }
    ret = drsOps->GetUint32(node, "len", &config->len, 0);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("UartTestReadConfig: read len fail, ret: %d!", ret);
        return ret;
    }
    config->wbuf = (uint8_t *)OsalMemCalloc(config->len);
    if (config->wbuf == NULL) {
        HDF_LOGE("UartTestReadConfig: wbuf OsalMemCalloc error!\n");
        return HDF_ERR_MALLOC_FAIL;
    }
    tmp = (uint32_t *)OsalMemCalloc(config->len * sizeof(uint32_t));
    if (tmp == NULL) {
        HDF_LOGE("UartTestReadConfig: tmp OsalMemCalloc error!\n");
        OsalMemFree(config->wbuf);
        return HDF_ERR_MALLOC_FAIL;
    }
    ret = drsOps->GetUint32Array(node, "wbuf", tmp, config->len, 0);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("UartTestReadConfig: read wbuf fail, ret: %d!\n", ret);
        OsalMemFree(config->wbuf);
        OsalMemFree(tmp);
        return ret;
    }
    for (i = 0; i < config->len; i++) {
        config->wbuf[i] = tmp[i];
    }
    OsalMemFree(tmp);
    return HDF_SUCCESS;
}

static int32_t UartTestBind(struct HdfDeviceObject *device)
{
    int32_t ret;
    static struct IDeviceIoService service;

    if (device == NULL || device->property == NULL) {
        HDF_LOGE("UartTestBind: device or config is null!");
        return HDF_ERR_IO;
    }

    ret = UartTestReadConfig(&g_config, device->property);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("UartTestBind: read config fail, ret: %d!", ret);
        return ret;
    }

    service.Dispatch = UartTestDispatch;
    device->service = &service;

    return HDF_SUCCESS;
}

static int32_t UartTestInit(struct HdfDeviceObject *device)
{
    (void)device;
    return HDF_SUCCESS;
}

static void UartTestRelease(struct HdfDeviceObject *device)
{
    if (device != NULL) {
        device->service = NULL;
    }
    OsalMemFree(g_config.wbuf);
    g_config.wbuf = NULL;
    return;
}

struct HdfDriverEntry g_uartTestEntry = {
    .moduleVersion = 1,
    .Bind = UartTestBind,
    .Init = UartTestInit,
    .Release = UartTestRelease,
    .moduleName = "PLATFORM_UART_TEST",
};
HDF_INIT(g_uartTestEntry);
