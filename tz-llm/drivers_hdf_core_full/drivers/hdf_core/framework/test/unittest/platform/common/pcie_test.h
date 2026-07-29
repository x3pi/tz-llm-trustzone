/*
 * Copyright (c) 2021-2023 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#ifndef PCIE_TEST_H
#define PCIE_TEST_H

#include "pcie_if.h"

#ifdef __cplusplus
extern "C" {
#endif

enum PcieTestCmd {
    PCIE_READ_AND_WRITE_01 = 0,
    PCIE_DMA_MAP_AND_UNMAP_01,
    PCIE_REG_AND_UNREG_IRQ_01,
    PCIE_TEST_MAX,
};

struct PcieTestConfig {
    uint32_t busNum;
};

struct PcieTester {
    struct PcieTestConfig config;
    DevHandle handle;
};

int32_t PcieTestExecute(int cmd);

#ifdef __cplusplus
}
#endif

#endif /* PCIE_TEST_H */
