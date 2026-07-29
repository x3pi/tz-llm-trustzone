/*
 * Copyright (c) 2021-2023 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include "hdf_adc_entry_test.h"
#include "adc_test.h"
#include "hdf_log.h"

#define HDF_LOG_TAG hdf_adc_entry_test

int32_t HdfAdcTestEntry(HdfTestMsg *msg)
{
    if (msg == NULL) {
        HDF_LOGE("HdfAdcTestEntry: msg is null!");
        return HDF_FAILURE;
    }

    msg->result = AdcTestExecute(msg->subCmd);

    return HDF_SUCCESS;
}
