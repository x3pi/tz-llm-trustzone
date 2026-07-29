/*
 * Copyright (c) 2023 Shenzhen Kaihong Digital Industry Development Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */
#ifndef PCIE_BUS_TEST_H
#define PCIE_BUS_TEST_H
#include <inttypes.h>
#include "hdf_ibus_intf.h"
#include "hdf_io_service_if.h"

enum PcieBusCmd {
    CMD_TEST_PCIE_BUS_GET_INFO,
    CMD_TEST_PCIE_BUS_READ_WRITE_DATA,
    CMD_TEST_PCIE_BUS_READ_WRITE_BULK,
    CMD_TEST_PCIE_BUS_READ_WRITE_FUNC0,
    CMD_TEST_PCIE_BUS_CLAIM_RELEASE_IRQ,
    CMD_TEST_PCIE_BUS_DISABLE_RESET_BUS,
    CMD_TEST_PCIE_BUS_CLAIM_RELEASE_HOST,
    CMD_TEST_PCIE_BUS_READ_WRITE_IO,
    CMD_TEST_PCIE_BUS_MAP_UNMAP_DMA,
    CMD_TEST_PCIE_MAX,
};

struct PcieBusTestConfig {
    uint8_t busNum;
};

struct PcieBusTester {
    struct PcieBusTestConfig config;
    struct BusDev busDev;
};
int32_t PcieBusTestExecute(int32_t cmd);
#endif
