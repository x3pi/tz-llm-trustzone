/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include "hdf_log.h"
#include "uart_if.h"

int32_t UartBlockWrite(DevHandle handle, uint8_t *data, uint32_t size)
{
    (void)handle;
    (void)data;
    (void)size;
    HDF_LOGI("UartBlockWrite: success!");
    return HDF_SUCCESS;
}
