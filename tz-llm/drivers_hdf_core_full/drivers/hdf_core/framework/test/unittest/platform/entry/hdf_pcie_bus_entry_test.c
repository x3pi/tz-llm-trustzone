/*
 * Copyright (c) 2023 Shenzhen Kaihong Digital Industry Development Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include "hdf_log.h"
#include "hdf_pcie_entry_test.h"
#include "pcie_bus_test.h"

#define HDF_LOG_TAG hdf_pcie_bus_entry_test

int32_t HdfPcieBusUnitTestEntry(HdfTestMsg *msg)
{
    if (msg == NULL) {
        return HDF_FAILURE;
    }

    msg->result = PcieBusTestExecute(msg->subCmd);

    return HDF_SUCCESS;
}
