/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include "hdf_log.h"
#include "i2c_if.h"

int32_t I2cRead(DevHandle handle, uint8_t *buf, uint16_t len)
{
    (void)handle;
    (void)buf;
    (void)len;
    HDF_LOGI("I2cRead: success!");
    return HDF_SUCCESS;
}

int32_t I2cWrite(DevHandle handle, uint8_t *buf, uint16_t len)
{
    (void)handle;
    (void)buf;
    (void)len;
    HDF_LOGI("I2cWrite: success!");
    return HDF_SUCCESS;
}
