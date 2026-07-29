/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include "pcie_test.h"
#include "device_resource_if.h"
#include "hdf_base.h"
#include "hdf_device_desc.h"
#include "hdf_log.h"

#define HDF_LOG_TAG pcie_test_driver_c

static struct PcieTestConfig g_config;

static int32_t PcieTestDispatch(struct HdfDeviceIoClient *client, int cmd, struct HdfSBuf *data, struct HdfSBuf *reply)
{
    (void)data;
    (void)client;

    if (cmd == 0) {
        if (reply == NULL) {
            HDF_LOGE("PcieTestDispatch: reply is null!");
            return HDF_ERR_INVALID_PARAM;
        }
        if (!HdfSbufWriteUint32(reply, g_config.busNum)) {
            HDF_LOGE("PcieTestDispatch: write busNum fail!");
            return HDF_ERR_IO;
        }
    } else {
        HDF_LOGE("PcieTestDispatch: cmd %d is not support!", cmd);
        return HDF_ERR_NOT_SUPPORT;
    }

    return HDF_SUCCESS;
}

static int32_t PcieTestReadConfig(const struct DeviceResourceNode *node)
{
    int32_t ret;
    struct DeviceResourceIface *drsOps = NULL;

    drsOps = DeviceResourceGetIfaceInstance(HDF_CONFIG_SOURCE);
    if (drsOps == NULL || drsOps->GetUint32 == NULL) {
        HDF_LOGE("PcieTestReadConfig: invalid drs ops!");
        return HDF_FAILURE;
    }

    ret = drsOps->GetUint32(node, "busNum", &(g_config.busNum), 0);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("PcieTestReadConfig: read bus num fail!");
        return ret;
    }

    return HDF_SUCCESS;
}

static int32_t PcieTestBind(struct HdfDeviceObject *device)
{
    static struct IDeviceIoService service;

    if (device == NULL || device->property == NULL) {
        HDF_LOGE("PcieTestBind: device or config is null!");
        return HDF_ERR_IO;
    }

    service.Dispatch = PcieTestDispatch;
    device->service = &service;
    HDF_LOGI("PcieTestBind: done!");
    return HDF_SUCCESS;
}

static int32_t PcieTestInit(struct HdfDeviceObject *device)
{
    int32_t ret = PcieTestReadConfig(device->property);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("PcieTestInit: read config fail!");
        return ret;
    }
    HDF_LOGI("PcieTestInit: done!");
    return HDF_SUCCESS;
}

static void PcieTestRelease(struct HdfDeviceObject *device)
{
    if (device != NULL) {
        device->service = NULL;
    }
    HDF_LOGI("PcieTestRelease: done!");
    return;
}

struct HdfDriverEntry g_pcieTestEntry = {
    .moduleVersion = 1,
    .Bind = PcieTestBind,
    .Init = PcieTestInit,
    .Release = PcieTestRelease,
    .moduleName = "PLATFORM_PCIE_TEST",
};
HDF_INIT(g_pcieTestEntry);
