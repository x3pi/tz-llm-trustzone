/*
 * Copyright (c) 2021-2023 Huawei Device Co., Ltd.
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
#include "pcie_test.h"

using namespace testing::ext;

class HdfPcieTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp();
    void TearDown();
};

void HdfPcieTest::SetUpTestCase()
{
    HdfTestOpenService();
}

void HdfPcieTest::TearDownTestCase()
{
    HdfTestCloseService();
}

void HdfPcieTest::SetUp()
{
}

void HdfPcieTest::TearDown()
{
}

/**
  * @tc.name: PcieReadAndWrite001
  * @tc.desc: test PcieRead/PcieWrite interface.
  * @tc.type: FUNC
  * @tc.require:
  */
HWTEST_F(HdfPcieTest, PcieReadAndWrite001, TestSize.Level1)
{
    struct HdfTestMsg msg = { TEST_PAL_PCIE_TYPE, PCIE_READ_AND_WRITE_01, -1 };
    EXPECT_EQ(0, HdfTestSendMsgToService(&msg));
    EXPECT_EQ(0, PcieTestExecute(PCIE_READ_AND_WRITE_01));
}

/**
  * @tc.name: TestPcieDmaMapAndUnmap001
  * @tc.desc: test PcieDmaMap/PcieDmaUnmap interface.
  * @tc.type: FUNC
  * @tc.require:
  */
HWTEST_F(HdfPcieTest, TestPcieDmaMapAndUnmap001, TestSize.Level1)
{
    struct HdfTestMsg msg = { TEST_PAL_PCIE_TYPE, PCIE_DMA_MAP_AND_UNMAP_01, -1 };
    EXPECT_EQ(0, HdfTestSendMsgToService(&msg));
    EXPECT_EQ(0, PcieTestExecute(PCIE_DMA_MAP_AND_UNMAP_01));
}

/**
  * @tc.name: TestPcieRegAndUnregIrq001
  * @tc.desc: test PcieRegisterIrq/PcieUnregisterIrq interface in kernel status.
  * @tc.type: FUNC
  * @tc.require:
  */
HWTEST_F(HdfPcieTest, TestPcieRegAndUnregIrq001, TestSize.Level1)
{
    struct HdfTestMsg msg = { TEST_PAL_PCIE_TYPE, PCIE_REG_AND_UNREG_IRQ_01, -1 };
    EXPECT_EQ(0, HdfTestSendMsgToService(&msg));
    EXPECT_EQ(0, PcieTestExecute(PCIE_REG_AND_UNREG_IRQ_01));
}
