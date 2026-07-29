/*
 * Copyright (c) 2021-2023 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include "hdf_i2s_entry_test.h"
#include "hdf_log.h"
#include "i2s_test.h"

#define HDF_LOG_TAG hdf_i2s_entry_test

int32_t HdfI2sUnitTestEntry(HdfTestMsg *msg)
{
    struct I2sTest *test = NULL;

    HDF_LOGE("HdfI2sUnitTestEntry: enter!\n");

    if (msg == NULL) {
        HDF_LOGE("HdfI2sUnitTestEntry: msg is null!");
        return HDF_FAILURE;
    }
    test = GetI2sTest();
    if (test == NULL || test->TestEntry == NULL) {
        HDF_LOGE("HdfI2sUnitTestEntry: tester or TestEntry is null!\n");
        msg->result = HDF_FAILURE;
        return HDF_FAILURE;
    }
    msg->result = test->TestEntry(test, msg->subCmd);
    return msg->result;
}
