/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include "emmc_if.h"
#include "hdf_log.h"

int32_t EmmcGetCardState(DevHandle handle, uint8_t *state, uint32_t size)
{
    (void)handle;
    (void)state;
    (void)size;
    HDF_LOGI("EmmcGetCardState: success!");
    return HDF_SUCCESS;
}

int32_t EmmcGetCardCsd(DevHandle handle, uint8_t *csd, uint32_t size)
{
    (void)handle;
    (void)csd;
    (void)size;
    HDF_LOGI("EmmcGetCardCsd: success!");
    return HDF_SUCCESS;
}

int32_t EmmcGetCardInfo(DevHandle handle, uint8_t *cardInfo, uint32_t size)
{
    (void)handle;
    (void)cardInfo;
    (void)size;
    HDF_LOGI("EmmcGetCardInfo: success!");
    return HDF_SUCCESS;
}
