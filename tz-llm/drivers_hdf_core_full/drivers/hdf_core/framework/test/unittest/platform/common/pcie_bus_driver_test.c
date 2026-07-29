/*
 * Copyright (c) 2023 Shenzhen Kaihong Digital Industry Development Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include "device_resource_if.h"
#include "hdf_base.h"
#include "hdf_device_desc.h"
#include "hdf_ibus_intf.h"
#include "hdf_log.h"
#include "pcie_bus_test.h"

static struct PcieBusTestConfig g_config;

static int32_t PcieBusTestDispatch(struct HdfDeviceIoClient *client, int cmd, struct HdfSBuf *data,
                                   struct HdfSBuf *reply)
{
    HDF_LOGD("%s: enter!", __func__);

    (void)client;
    (void)data;
    if (cmd == 0) {
        if (reply == NULL) {
            HDF_LOGE("%s: reply is null!", __func__);
            return HDF_ERR_INVALID_PARAM;
        }
        if (!HdfSbufWriteBuffer(reply, &g_config, sizeof(g_config))) {
            HDF_LOGE("%s: write config failed", __func__);
            return HDF_ERR_IO;
        }
        return HDF_SUCCESS;
    } else {
        return HDF_ERR_NOT_SUPPORT;
    }
}

static int32_t PcieBusTestReadConfig(const struct DeviceResourceNode *node)
{
    HDF_LOGI("PcieBusTestReadConfig enter.");
    int32_t ret;
    uint32_t busNum = 0;
    struct DeviceResourceIface *drsOps = NULL;

    drsOps = DeviceResourceGetIfaceInstance(HDF_CONFIG_SOURCE);
    if (drsOps == NULL || drsOps->GetUint32 == NULL) {
        HDF_LOGE("PcieBusTestReadConfig: invalid drs ops");
        return HDF_FAILURE;
    }
    ret = drsOps->GetUint32(node, "busNum", &busNum, 0);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("PcieBusTestReadConfig: read bus num failed");
        return ret;
    }
    g_config.busNum = (uint8_t)busNum;
    return HDF_SUCCESS;
}

static int32_t PcieBusTestBind(struct HdfDeviceObject *device)
{
    HDF_LOGI("%{public}s enter.", __func__);
    static struct IDeviceIoService service;

    if (device == NULL) {
        HDF_LOGE("%s: device or config is null!", __func__);
        return HDF_ERR_IO;
    }
    service.Dispatch = PcieBusTestDispatch;
    device->service = &service;

    return HDF_SUCCESS;
}

static int32_t PcieBusTestInit(struct HdfDeviceObject *device)
{
    int32_t ret;
    if (device->property == NULL) {
        HDF_LOGW("PcieBusTestInit property is NULL.");
        return HDF_SUCCESS;
    }

    ret = PcieBusTestReadConfig(device->property);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: read config failed", __func__);
    }
    return ret;
}

static void PcieBusTestRelease(struct HdfDeviceObject *device)
{
    if (device != NULL) {
        device->service = NULL;
    }
    return;
}

struct HdfDriverEntry g_pcieBusTestEntry = {
    .moduleVersion = 1,
    .Bind = PcieBusTestBind,
    .Init = PcieBusTestInit,
    .Release = PcieBusTestRelease,
    .moduleName = "PLATFORM_PCIE_BUS_TEST",
};
HDF_INIT(g_pcieBusTestEntry);
