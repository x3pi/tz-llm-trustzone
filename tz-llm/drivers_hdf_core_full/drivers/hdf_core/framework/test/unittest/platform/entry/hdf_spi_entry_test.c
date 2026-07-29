/*
 * Copyright (c) 2020-2023 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include "hdf_spi_entry_test.h"
#include "hdf_log.h"
#include "spi_test.h"

#define HDF_LOG_TAG hdf_spi_entry_test

int32_t HdfSpiUnitTestEntry(HdfTestMsg *msg)
{
    if (msg == NULL) {
        HDF_LOGE("HdfSpiUnitTestEntry: msg is null!");
        return HDF_FAILURE;
    }

    msg->result = SpiTestExecute(msg->subCmd);
    return msg->result;
}
