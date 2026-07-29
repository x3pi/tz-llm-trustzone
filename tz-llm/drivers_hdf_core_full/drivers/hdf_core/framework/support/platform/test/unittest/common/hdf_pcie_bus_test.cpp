/*
 * Copyright (c) 2023 Shenzhen Kaihong Digital Industry Development Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <fcntl.h>
#include <gtest/gtest.h>
#include <string>
#include <unistd.h>
#include "hdf_io_service_if.h"
#include "hdf_uhdf_test.h"
#include "pcie_bus_test.h"

using namespace testing::ext;
namespace OHOS {
class HdfPcieBusTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp();
    void TearDown();
};

void HdfPcieBusTest::SetUpTestCase()
{
    HdfTestOpenService();
}

void HdfPcieBusTest::TearDownTestCase()
{
    HdfTestCloseService();
}

void HdfPcieBusTest::SetUp()
{}

void HdfPcieBusTest::TearDown()
{}

/**
 * @tc.name: PcieBusGetBusInfo001
 * @tc.desc: test getBusInfo interface.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(HdfPcieBusTest, PcieBusGetBusInfo001, TestSize.Level1)
{
    struct HdfTestMsg msg = {TEST_WIFI_PCIE_BUS_TYPE, CMD_TEST_PCIE_BUS_GET_INFO, -1};
    EXPECT_EQ(HDF_SUCCESS, HdfTestSendMsgToService(&msg));
}

/**
 * @tc.name: PcieBusReadAndWrite001
 * @tc.desc: test readData/WriteData interface.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(HdfPcieBusTest, PcieBusReadAndWrite001, TestSize.Level1)
{
    struct HdfTestMsg msg = {TEST_WIFI_PCIE_BUS_TYPE, CMD_TEST_PCIE_BUS_READ_WRITE_DATA, -1};
    EXPECT_EQ(HDF_SUCCESS, HdfTestSendMsgToService(&msg));
}

/**
 * @tc.name: TestPcieBusBulkReadAndWrite001
 * @tc.desc: test bulkRead/bulkWrite interface.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(HdfPcieBusTest, TestPcieBusBulkReadAndWrite001, TestSize.Level1)
{
    struct HdfTestMsg msg = {TEST_WIFI_PCIE_BUS_TYPE, CMD_TEST_PCIE_BUS_READ_WRITE_BULK, -1};
    EXPECT_EQ(HDF_SUCCESS, HdfTestSendMsgToService(&msg));
}

/**
 * @tc.name: TestPcieBusFunc0ReadAndWrite001
 * @tc.desc: test readFunc0/writeFunc0 interface in kernel status.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(HdfPcieBusTest, TestPcieBusFunc0ReadAndWrite001, TestSize.Level1)
{
    struct HdfTestMsg msg = {TEST_WIFI_PCIE_BUS_TYPE, CMD_TEST_PCIE_BUS_READ_WRITE_FUNC0, -1};
    EXPECT_EQ(HDF_SUCCESS, HdfTestSendMsgToService(&msg));
}

/**
 * @tc.name: TestPcieBusIrqClaimAndRelease001
 * @tc.desc: test claimIrq/releaseIrq interface in kernel status.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(HdfPcieBusTest, TestPcieBusIrqClaimAndRelease001, TestSize.Level1)
{
    struct HdfTestMsg msg = {TEST_WIFI_PCIE_BUS_TYPE, CMD_TEST_PCIE_BUS_CLAIM_RELEASE_IRQ, -1};
    EXPECT_EQ(HDF_SUCCESS, HdfTestSendMsgToService(&msg));
}

/**
 * @tc.name: TestPcieBusDisableAndReset001
 * @tc.desc: test disable/reset interface in kernel status.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(HdfPcieBusTest, TestPcieBusDisableAndReset001, TestSize.Level1)
{
    struct HdfTestMsg msg = {TEST_WIFI_PCIE_BUS_TYPE, CMD_TEST_PCIE_BUS_DISABLE_RESET_BUS, -1};
    EXPECT_EQ(HDF_SUCCESS, HdfTestSendMsgToService(&msg));
}

/**
 * @tc.name: TestPcieBusHostClaimAndRelease001
 * @tc.desc: test claimHost/releaseHost interface in kernel status.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(HdfPcieBusTest, TestPcieBusHostClaimAndRelease001, TestSize.Level1)
{
    struct HdfTestMsg msg = {TEST_WIFI_PCIE_BUS_TYPE, CMD_TEST_PCIE_BUS_CLAIM_RELEASE_HOST, -1};
    EXPECT_EQ(HDF_SUCCESS, HdfTestSendMsgToService(&msg));
}

/**
 * @tc.name: TestPcieBusIoReadAndWrite001
 * @tc.desc: test readIo/writeIo interface in kernel status.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(HdfPcieBusTest, TestPcieBusIoReadAndWrite001, TestSize.Level1)
{
    struct HdfTestMsg msg = {TEST_WIFI_PCIE_BUS_TYPE, CMD_TEST_PCIE_BUS_READ_WRITE_IO, -1};
    EXPECT_EQ(HDF_SUCCESS, HdfTestSendMsgToService(&msg));
}

/**
 * @tc.name: TestPcieBusDmaMapAndUnMap001
 * @tc.desc: test dmaMap/dmaUnmap interface in kernel status.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(HdfPcieBusTest, TestPcieBusDmaMapAndUnMap001, TestSize.Level1)
{
    struct HdfTestMsg msg = {TEST_WIFI_PCIE_BUS_TYPE, CMD_TEST_PCIE_BUS_MAP_UNMAP_DMA, -1};
    EXPECT_EQ(HDF_SUCCESS, HdfTestSendMsgToService(&msg));
}
}  // namespace OHOS
