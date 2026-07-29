/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include "hdf_log.h"
#include "watchdog_if.h"

int32_t WatchdogBark(DevHandle handle)
{
    (void)handle;
    HDF_LOGI("WatchdogBark: success!");
    return HDF_SUCCESS;
}

int32_t WatchdogEnable(DevHandle handle, bool enable)
{
    (void)enable;
    (void)handle;
    HDF_LOGI("WatchdogEnable: success!");
    return HDF_SUCCESS;
}

int32_t WatchdogGetEnable(DevHandle handle, bool *enable)
{
    (void)enable;
    (void)handle;
    HDF_LOGI("WatchdogGetEnable: success!");
    return HDF_SUCCESS;
}
