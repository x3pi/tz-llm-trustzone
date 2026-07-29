/*
 * Copyright (c) 2021-2023 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include "dac_if.h"
#include "dac_core.h"
#include "hdf_log.h"
#define HDF_LOG_TAG dac_if_c
#define DAC_SERVICE_NAME "HDF_PLATFORM_DAC_MANAGER"

DevHandle DacOpen(uint32_t number)
{
    int32_t ret;
    struct DacDevice *device = NULL;

    device = DacDeviceGet(number);
    if (device == NULL) {
        HDF_LOGE("DacOpen: get device fail!");
        return NULL;
    }

    ret = DacDeviceStart(device);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("DacOpen: start device fail!");
        return NULL;
    }

    return (DevHandle)device;
}

void DacClose(DevHandle handle)
{
    struct DacDevice *device = (struct DacDevice *)handle;

    if (device == NULL) {
        HDF_LOGE("DacClose: device is null!");
        return;
    }

    (void)DacDeviceStop(device);
    DacDevicePut(device);
}

int32_t DacWrite(DevHandle handle, uint32_t channel, uint32_t val)
{
    if (handle == NULL) {
        HDF_LOGE("DacWrite: invalid handle!");
        return HDF_ERR_INVALID_PARAM;
    }
    return DacDeviceWrite((struct DacDevice *)handle, channel, val);
}
