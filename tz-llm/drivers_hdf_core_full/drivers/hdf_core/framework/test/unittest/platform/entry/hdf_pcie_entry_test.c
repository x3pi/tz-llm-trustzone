/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include "hdf_pcie_entry_test.h"
#include "hdf_log.h"
#include "pcie_test.h"

#define HDF_LOG_TAG hdf_pcie_entry_test

int32_t HdfPcieUnitTestEntry(HdfTestMsg *msg)
{
    if (msg == NULL) {
        HDF_LOGE("HdfPcieUnitTestEntry: msg is null!");
        return HDF_FAILURE;
    }

    msg->result = PcieTestExecute(msg->subCmd);

    return HDF_SUCCESS;
}
