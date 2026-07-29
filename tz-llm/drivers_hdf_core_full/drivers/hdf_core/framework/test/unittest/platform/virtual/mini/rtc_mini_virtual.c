/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include "hdf_log.h"
#include "rtc_if.h"

int32_t RtcSetTimeZone(int32_t timeZone)
{
    (void)timeZone;
    HDF_LOGI("RtcSetTimeZone: success!");
    return HDF_SUCCESS;
}

int32_t RtcGetTimeZone(int32_t *timeZone)
{
    (void)timeZone;
    HDF_LOGI("RtcGetTimeZone: success!");
    return HDF_SUCCESS;
}
