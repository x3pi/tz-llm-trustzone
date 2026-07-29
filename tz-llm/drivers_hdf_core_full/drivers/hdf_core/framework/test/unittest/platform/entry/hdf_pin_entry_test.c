/*
 * Copyright (c) 2021-2023 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include "hdf_pin_entry_test.h"
#include "hdf_log.h"
#include "pin_test.h"

#define HDF_LOG_TAG hdf_pin_entry_test

int32_t HdfPinTestEntry(HdfTestMsg *msg)
{
    HDF_LOGD("HdfPinTestEntry: enter!");

    if (msg == NULL) {
        HDF_LOGE("HdfPinTestEntry: msg is null!");
        return HDF_FAILURE;
    }

    msg->result = PinTestExecute(msg->subCmd);

    return HDF_SUCCESS;
}