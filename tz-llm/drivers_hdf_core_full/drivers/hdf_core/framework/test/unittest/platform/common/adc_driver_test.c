/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include "adc_test.h"
#include "device_resource_if.h"
#include "hdf_base.h"
#include "hdf_device_desc.h"
#include "hdf_log.h"

#define HDF_LOG_TAG adc_test_driver_c

static struct AdcTestConfig g_config;

static int32_t AdcTestDispatch(struct HdfDeviceIoClient *client, int cmd, struct HdfSBuf *data, struct HdfSBuf *reply)
{
    (void)data;
    (void)client;

    if (cmd == 0) {
        if (reply == NULL) {
            HDF_LOGE("AdcTestDispatch: reply is null!");
            return HDF_ERR_INVALID_PARAM;
        }
        if (!HdfSbufWriteBuffer(reply, &g_config, sizeof(g_config))) {
            HDF_LOGE("AdcTestDispatch: write reply fail!");
            return HDF_ERR_IO;
        }
    } else {
        HDF_LOGE("AdcTestDispatch: cmd %d is not support!", cmd);
        return HDF_ERR_NOT_SUPPORT;
    }

    return HDF_SUCCESS;
}

static int32_t AdcTestReadConfig(struct AdcTestConfig *config, const struct DeviceResourceNode *node)
{
    int32_t ret;
    struct DeviceResourceIface *drsOps = NULL;

    drsOps = DeviceResourceGetIfaceInstance(HDF_CONFIG_SOURCE);
    if (drsOps == NULL || drsOps->GetUint32 == NULL) {
        HDF_LOGE("AdcTestReadConfig: invalid drs ops!");
        return HDF_FAILURE;
    }

    ret = drsOps->GetUint32(node, "devNum", &config->devNum, 0);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("AdcTestReadConfig: read devNum fail!");
        return ret;
    }

    ret = drsOps->GetUint32(node, "channel", &config->channel, 0);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("AdcTestReadConfig: read channel fail!");
        return ret;
    }

    ret = drsOps->GetUint32(node, "maxChannel", &config->maxChannel, 0);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("AdcTestReadConfig: read maxChannel fail!");
        return ret;
    }

    ret = drsOps->GetUint32(node, "dataWidth", &config->dataWidth, 0);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("AdcTestReadConfig: read dataWidth fail!");
        return ret;
    }

    ret = drsOps->GetUint32(node, "rate", &config->rate, 0);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("AdcTestReadConfig: read rate fail!");
        return ret;
    }

    return HDF_SUCCESS;
}

static int32_t AdcTestBind(struct HdfDeviceObject *device)
{
    int32_t ret;
    static struct IDeviceIoService service;

    if (device == NULL || device->property == NULL) {
        HDF_LOGE("AdcTestBind: device or config is null!");
        return HDF_ERR_IO;
    }

    ret = AdcTestReadConfig(&g_config, device->property);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("AdcTestBind: read config fail!");
        return ret;
    }
    service.Dispatch = AdcTestDispatch;
    device->service = &service;
    HDF_LOGI("AdcTestBind: done!");
    return HDF_SUCCESS;
}

static int32_t AdcTestInit(struct HdfDeviceObject *device)
{
    (void)device;
    HDF_LOGI("AdcTestInit: done!");
    return HDF_SUCCESS;
}

static void AdcTestRelease(struct HdfDeviceObject *device)
{
    if (device != NULL) {
        device->service = NULL;
    }
    HDF_LOGI("AdcTestRelease: done!");
    return;
}

struct HdfDriverEntry g_adcTestEntry = {
    .moduleVersion = 1,
    .Bind = AdcTestBind,
    .Init = AdcTestInit,
    .Release = AdcTestRelease,
    .moduleName = "PLATFORM_ADC_TEST",
};
HDF_INIT(g_adcTestEntry);
