/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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
#include "hdf_uhdf_test.h"
#include "clock_test.h"
#include "hdf_io_service_if.h"

using namespace testing::ext;

class HdfClockTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp();
    void TearDown();
};

void HdfClockTest::SetUpTestCase()
{
    HdfTestOpenService();
}

void HdfClockTest::TearDownTestCase()
{
    HdfTestCloseService();
}

void HdfClockTest::SetUp()
{
}

void HdfClockTest::TearDown()
{
}

/**
 * @tc.name: ClockTestEnable001
 * @tc.desc: clock enable test
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(HdfClockTest, ClockTestEnable001, TestSize.Level1)
{
    struct HdfTestMsg msg = {TEST_PAL_CLOCK_TYPE, CLOCK_TEST_CMD_ENABLE, -1};
    EXPECT_EQ(0, HdfTestSendMsgToService(&msg));
    EXPECT_EQ(0, ClockTestExecute(CLOCK_TEST_CMD_ENABLE));
}

/**
 * @tc.name: ClockTestDisable001
 * @tc.desc: clock disable test
 * @tc.type: FUNC
 * @tc.require: NA
 */

HWTEST_F(HdfClockTest, ClockTestDisable001, TestSize.Level1)
{
    struct HdfTestMsg msg = {TEST_PAL_CLOCK_TYPE, CLOCK_TEST_CMD_DISABLE, -1};
    EXPECT_EQ(0, HdfTestSendMsgToService(&msg));
    EXPECT_EQ(0, ClockTestExecute(CLOCK_TEST_CMD_DISABLE));
}

/**
 * @tc.name: ClockTestSetrate001
 * @tc.desc: clock setrate test
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(HdfClockTest, ClockTestSetrate001, TestSize.Level1)
{
    struct HdfTestMsg msg = {TEST_PAL_CLOCK_TYPE, CLOCK_TEST_CMD_SET_RATE, -1};
    EXPECT_EQ(0, HdfTestSendMsgToService(&msg));
    EXPECT_EQ(0, ClockTestExecute(CLOCK_TEST_CMD_SET_RATE));
}

/**
 * @tc.name: ClockTestGetRate001
 * @tc.desc: clock getrate test
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(HdfClockTest, ClockTestGetRate001, TestSize.Level1)
{
    struct HdfTestMsg msg = {TEST_PAL_CLOCK_TYPE, CLOCK_TEST_CMD_GET_RATE, -1};
    EXPECT_EQ(0, HdfTestSendMsgToService(&msg));
    EXPECT_EQ(0, ClockTestExecute(CLOCK_TEST_CMD_GET_RATE));
}

/**
 * @tc.name: ClockTestGetparent001
 * @tc.desc: clock getparent test
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(HdfClockTest, ClockTestGetparent001, TestSize.Level1)
{
    struct HdfTestMsg msg = {TEST_PAL_CLOCK_TYPE, CLOCK_TEST_CMD_GET_PARENT, -1};
    EXPECT_EQ(0, HdfTestSendMsgToService(&msg));
    EXPECT_EQ(0, ClockTestExecute(CLOCK_TEST_CMD_GET_PARENT));
}

/**
 * @tc.name: ClockTestSetparent001
 * @tc.desc: clock setparent test
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(HdfClockTest, ClockTestSetparent001, TestSize.Level1)
{
    struct HdfTestMsg msg = {TEST_PAL_CLOCK_TYPE, CLOCK_TEST_CMD_SET_PARENT, -1};
    EXPECT_EQ(0, HdfTestSendMsgToService(&msg));
    EXPECT_EQ(0, ClockTestExecute(CLOCK_TEST_CMD_SET_PARENT));
}

/**
 * @tc.name: ClockTestMultiThread001
 * @tc.desc: clock multi thread test
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(HdfClockTest, ClockTestMultiThread001, TestSize.Level1)
{
    struct HdfTestMsg msg = {TEST_PAL_CLOCK_TYPE, CLOCK_TEST_CMD_MULTI_THREAD, -1};
    EXPECT_EQ(0, HdfTestSendMsgToService(&msg));
    EXPECT_EQ(0, ClockTestExecute(CLOCK_TEST_CMD_MULTI_THREAD));
}

/**
 * @tc.name: CLOCKTestReliability001
 * @tc.desc: clock reliability test
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(HdfClockTest, CLOCKTestReliability001, TestSize.Level1)
{
    struct HdfTestMsg msg = {TEST_PAL_CLOCK_TYPE, CLOCK_TEST_CMD_RELIABILITY, -1};
    EXPECT_EQ(0, HdfTestSendMsgToService(&msg));
    EXPECT_EQ(0, ClockTestExecute(CLOCK_TEST_CMD_RELIABILITY));
}

/**
 * @tc.name: ClockIfPerformanceTest001
 * @tc.desc: clock user if performance test
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(HdfClockTest, ClockIfPerformanceTest001, TestSize.Level1)
{
    struct HdfTestMsg msg = {TEST_PAL_CLOCK_TYPE, CLOCK_IF_PERFORMANCE_TEST, -1};
    EXPECT_EQ(0, HdfTestSendMsgToService(&msg));
    EXPECT_EQ(0, ClockTestExecute(CLOCK_IF_PERFORMANCE_TEST));
}
