/*
 * Copyright (c) 2020-2022 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <fcntl.h>
#include <string>
#include <unistd.h>
#include <gtest/gtest.h>
#include "gpio_test.h"
#include "hdf_uhdf_test.h"
#include "hdf_io_service_if.h"

using namespace testing::ext;

class HdfGpioTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp();
    void TearDown();
};

void HdfGpioTest::SetUpTestCase()
{
    HdfTestOpenService();
}

void HdfGpioTest::TearDownTestCase()
{
    HdfTestCloseService();
}

void HdfGpioTest::SetUp()
{
}

void HdfGpioTest::TearDown()
{
}

/**
  * @tc.name: GpioTestSetGetDir001
  * @tc.desc: gpio set and get dir test
  * @tc.type: FUNC
  * @tc.require: AR000F868H
  */
HWTEST_F(HdfGpioTest, GpioTestSetGetDir001, TestSize.Level1)
{
    struct HdfTestMsg msg = {TEST_PAL_GPIO_TYPE, GPIO_TEST_SET_GET_DIR, -1};
    EXPECT_EQ(0, HdfTestSendMsgToService(&msg));
    EXPECT_EQ(0, GpioTestExecute(GPIO_TEST_SET_GET_DIR));
}

/**
  * @tc.name: GpioTestWriteRead001
  * @tc.desc: gpio write and read test
  * @tc.type: FUNC
  * @tc.require: AR000F868H
  */
HWTEST_F(HdfGpioTest, GpioTestWriteRead001, TestSize.Level1)
{
    struct HdfTestMsg msg = {TEST_PAL_GPIO_TYPE, GPIO_TEST_WRITE_READ, -1};
    EXPECT_EQ(0, HdfTestSendMsgToService(&msg));
    EXPECT_EQ(0, GpioTestExecute(GPIO_TEST_WRITE_READ));
}

/**
  * @tc.name: GpioTestIrqLevel001
  * @tc.desc: gpio level irq trigger test
  * @tc.type: FUNC
  * @tc.require: AR000F868H
  */
HWTEST_F(HdfGpioTest, GpioTestIrqLevel001, TestSize.Level1)
{
    struct HdfTestMsg msg = {TEST_PAL_GPIO_TYPE, GPIO_TEST_IRQ_LEVEL, -1};
    EXPECT_EQ(0, HdfTestSendMsgToService(&msg));
    EXPECT_EQ(0, GpioTestExecute(GPIO_TEST_IRQ_LEVEL));
}

/**
  * @tc.name: GpioTestIrqEdge001
  * @tc.desc: gpio edge irq trigger test
  * @tc.type: FUNC
  * @tc.require: AR000F868H
  */
HWTEST_F(HdfGpioTest, GpioTestIrqEdge001, TestSize.Level1)
{
    struct HdfTestMsg msg = {TEST_PAL_GPIO_TYPE, GPIO_TEST_IRQ_EDGE, -1};
    EXPECT_EQ(0, HdfTestSendMsgToService(&msg));
    EXPECT_EQ(0, GpioTestExecute(GPIO_TEST_IRQ_EDGE));
}

/**
  * @tc.name: GpioTestIrqThread001
  * @tc.desc: gpio thread irq trigger test
  * @tc.type: FUNC
  * @tc.require: AR000F868H
  */
HWTEST_F(HdfGpioTest, GpioTestIrqThread001, TestSize.Level1)
{
    struct HdfTestMsg msg = {TEST_PAL_GPIO_TYPE, GPIO_TEST_IRQ_THREAD, -1};
    EXPECT_EQ(0, HdfTestSendMsgToService(&msg));
    EXPECT_EQ(0, GpioTestExecute(GPIO_TEST_IRQ_THREAD));
}

/**
  * @tc.name: GpioTestNumberGetByName
  * @tc.desc: get gpio global number test
  * @tc.type: FUNC
  * @tc.require:
  */
HWTEST_F(HdfGpioTest, GpioTestGetNumByName001, TestSize.Level1)
{
    struct HdfTestMsg msg = {TEST_PAL_GPIO_TYPE, GPIO_TEST_GET_NUM_BY_NAME, -1};
    EXPECT_EQ(0, HdfTestSendMsgToService(&msg));
    EXPECT_EQ(0, GpioTestExecute(GPIO_TEST_GET_NUM_BY_NAME));
}

/**
  * @tc.name: GpioTestReliability001
  * @tc.desc: gpio reliability test
  * @tc.type: FUNC
  * @tc.require: AR000F868H
  */
HWTEST_F(HdfGpioTest, GpioTestReliability001, TestSize.Level1)
{
    struct HdfTestMsg msg = {TEST_PAL_GPIO_TYPE, GPIO_TEST_RELIABILITY, -1};
    EXPECT_EQ(0, HdfTestSendMsgToService(&msg));
    EXPECT_EQ(0, GpioTestExecute(GPIO_TEST_RELIABILITY));
}

/**
  * @tc.name: GpioIfPerformanceTest001
  * @tc.desc: gpio user if performance test
  * @tc.type: FUNC
  * @tc.require:
  */
HWTEST_F(HdfGpioTest, GpioIfPerformanceTest001, TestSize.Level1)
{
    EXPECT_EQ(0, GpioTestExecute(GPIO_TEST_PERFORMANCE));
}
