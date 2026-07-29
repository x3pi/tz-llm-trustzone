/*
 * Copyright (c) 2022-2023 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include "securec.h"
#include "hdf_device_desc.h"
#include "device_resource_if.h"
#include "osal_mem.h"
#include "hdf_base.h"
#include "hdf_log.h"
#include "spi_test.h"

#define HDF_LOG_TAG spi_test_driver_c

static struct SpiTestConfig g_config;

static int32_t SpiTestDispatch(struct HdfDeviceIoClient *client, int cmd, struct HdfSBuf *data, struct HdfSBuf *reply)
{
    (void)client;
    (void)data;
    HDF_LOGD("SpiTestDispatch: enter!");
    if (cmd != 0) {
        HDF_LOGE("SpiTestDispatch: cmd: %d is not support!", cmd);
        return HDF_ERR_NOT_SUPPORT;
    }

    if (reply == NULL) {
        HDF_LOGE("SpiTestDispatch: reply is null!");
        return HDF_ERR_INVALID_PARAM;
    }
    HDF_LOGD("SpiTestDispatch: sizeof(g_config): %d, len: %d", sizeof(g_config), g_config.len);
    if (!HdfSbufWriteBuffer(reply, &g_config, sizeof(g_config))) {
        HDF_LOGE("SpiTestDispatch: write config fail!");
        return HDF_ERR_IO;
    }
    if (!HdfSbufWriteBuffer(reply, g_config.wbuf, g_config.len)) {
        HDF_LOGE("SpiTestDispatch: write config fail!");
        return HDF_ERR_IO;
    }

    return HDF_SUCCESS;
}

static int32_t SpiTestInitFromHcs(struct SpiTestConfig *config, const struct DeviceResourceNode *node)
{
    int32_t ret;
    struct DeviceResourceIface *face = NULL;

    face = DeviceResourceGetIfaceInstance(HDF_CONFIG_SOURCE);
    if (face == NULL) {
        HDF_LOGE("SpiTestInitFromHcs: face is null!");
        return HDF_FAILURE;
    }
    if (face->GetUint32 == NULL || face->GetUint8Array == NULL) {
        HDF_LOGE("SpiTestInitFromHcs: GetUint32 or GetUint32Array not support!");
        return HDF_ERR_NOT_SUPPORT;
    }
    ret = face->GetUint32(node, "bus", &config->bus, 0);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("SpiTestInitFromHcs: read bus fail, ret: %d", ret);
        return ret;
    }
    ret = face->GetUint32(node, "cs", &config->cs, 0);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("SpiTestInitFromHcs: read cs fail, ret: %d", ret);
        return ret;
    }
    ret = face->GetUint32(node, "len", &config->len, 0);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("SpiTestInitFromHcs: read len fail, ret: %d", ret);
        return ret;
    }
    config->wbuf = (uint8_t *)OsalMemCalloc(config->len);
    if (config->wbuf == NULL) {
        HDF_LOGE("SpiTestInitFromHcs: wbuf OsalMemCalloc error!\n");
        return HDF_ERR_MALLOC_FAIL;
    }

    ret = face->GetUint8Array(node, "wbuf", g_config.wbuf, config->len, 0);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("SpiTestInitFromHcs: read wbuf fail, ret: %d", ret);
        OsalMemFree(config->wbuf);
        return ret;
    }

    return HDF_SUCCESS;
}

static int32_t SpiTestBind(struct HdfDeviceObject *device)
{
    int32_t ret;
    struct IDeviceIoService *service = NULL;

    service = (struct IDeviceIoService *)OsalMemCalloc(sizeof(*service));
    if (service == NULL) {
        HDF_LOGE("SpiTestBind: malloc service fail!");
        return HDF_ERR_MALLOC_FAIL;
    }

    if (device == NULL || device->property == NULL) {
        HDF_LOGE("SpiTestBind: device or config is null!");
        return HDF_ERR_IO;
    }

    ret = SpiTestInitFromHcs(&g_config, device->property);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("SpiTestBind: read config fail, ret: %d", ret);
        return ret;
    }

    service->Dispatch = SpiTestDispatch;
    device->service = service;

    return HDF_SUCCESS;
}

static int32_t SpiTestInit(struct HdfDeviceObject *device)
{
    (void)device;
    return HDF_SUCCESS;
}

static void SpiTestRelease(struct HdfDeviceObject *device)
{
    if (device != NULL) {
        OsalMemFree(device->service);
        device->service = NULL;
    }
    OsalMemFree(g_config.wbuf);
    g_config.wbuf = NULL;
    return;
}

struct HdfDriverEntry g_spiTestEntry = {
    .moduleVersion = 1,
    .Bind = SpiTestBind,
    .Init = SpiTestInit,
    .Release = SpiTestRelease,
    .moduleName = "PLATFORM_SPI_TEST",
};
HDF_INIT(g_spiTestEntry);
