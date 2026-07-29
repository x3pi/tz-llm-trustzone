/*
 * Copyright (c) 2020-2021 Huawei Device Co., Ltd.
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
#include "hdf_uhdf_test.h"
#include "hdf_io_service_if.h"
#include "uart_test.h"

using namespace testing::ext;

class HdfUartTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp();
    void TearDown();
};

void HdfUartTest::SetUpTestCase()
{
    HdfTestOpenService();
}

void HdfUartTest::TearDownTestCase()
{
    HdfTestCloseService();
}

void HdfUartTest::SetUp()
{
}

void HdfUartTest::TearDown()
{
}

/**
 * @tc.name: UartSetTransModeTest001
 * @tc.desc: uart function test
 * @tc.type: FUNC
 * @tc.require: AR000F8689
 */
HWTEST_F(HdfUartTest, UartSetTransModeTest001, TestSize.Level1)
{
    struct HdfTestMsg msg = {TEST_PAL_UART_TYPE, UART_TEST_CMD_SET_TRANSMODE, -1};
    EXPECT_EQ(0, HdfTestSendMsgToService(&msg));
    EXPECT_EQ(0, UartTestExecute(UART_TEST_CMD_SET_TRANSMODE));
}

/**
  * @tc.name: UartWriteTest001
  * @tc.desc: uart function test
  * @tc.type: FUNC
  * @tc.require: AR000F8689
  */
HWTEST_F(HdfUartTest, UartWriteTest001, TestSize.Level1)
{
    struct HdfTestMsg msg = {TEST_PAL_UART_TYPE, UART_TEST_CMD_WRITE, -1};
    EXPECT_EQ(0, HdfTestSendMsgToService(&msg));
    EXPECT_EQ(0, UartTestExecute(UART_TEST_CMD_WRITE));
}

/**
  * @tc.name: UartReadTest001
  * @tc.desc: uart function test
  * @tc.type: FUNC
  * @tc.require: AR000F8689
  */
HWTEST_F(HdfUartTest, UartReadTest001, TestSize.Level1)
{
    struct HdfTestMsg msg = { TEST_PAL_UART_TYPE, UART_TEST_CMD_READ, -1};
    EXPECT_EQ(0, HdfTestSendMsgToService(&msg));
    EXPECT_EQ(0, UartTestExecute(UART_TEST_CMD_READ));
}

/**
  * @tc.name: UartSetBaudTest001
  * @tc.desc: uart function test
  * @tc.type: FUNC
  * @tc.require: AR000F8689
  */
HWTEST_F(HdfUartTest, UartSetBaudTest001, TestSize.Level1)
{
    struct HdfTestMsg msg = {TEST_PAL_UART_TYPE, UART_TEST_CMD_SET_BAUD, -1};
    EXPECT_EQ(0, HdfTestSendMsgToService(&msg));
    EXPECT_EQ(0, UartTestExecute(UART_TEST_CMD_SET_BAUD));
}

/**
  * @tc.name: UartGetBaudTest001
  * @tc.desc: uart function test
  * @tc.type: FUNC
  * @tc.require: AR000F8689
  */
HWTEST_F(HdfUartTest, UartGetBaudTest001, TestSize.Level1)
{
    struct HdfTestMsg msg = {TEST_PAL_UART_TYPE, UART_TEST_CMD_GET_BAUD, -1};
    EXPECT_EQ(0, HdfTestSendMsgToService(&msg));
    EXPECT_EQ(0, UartTestExecute(UART_TEST_CMD_GET_BAUD));
}

/**
  * @tc.name: UartSetAttributeTest001
  * @tc.desc: uart function test
  * @tc.type: FUNC
  * @tc.require: AR000F8689
  */
HWTEST_F(HdfUartTest, UartSetAttributeTest001, TestSize.Level1)
{
    struct HdfTestMsg msg = {TEST_PAL_UART_TYPE, UART_TEST_CMD_SET_ATTRIBUTE, -1};
    EXPECT_EQ(0, HdfTestSendMsgToService(&msg));
    EXPECT_EQ(0, UartTestExecute(UART_TEST_CMD_SET_ATTRIBUTE));
}

/**
  * @tc.name: UartGetAttributeTest001
  * @tc.desc: uart function test
  * @tc.type: FUNC
  * @tc.require: AR000F8689
  */
HWTEST_F(HdfUartTest, UartGetAttributeTest001, TestSize.Level1)
{
    struct HdfTestMsg msg = {TEST_PAL_UART_TYPE, UART_TEST_CMD_GET_ATTRIBUTE, -1};
    EXPECT_EQ(0, HdfTestSendMsgToService(&msg));
    EXPECT_EQ(0, UartTestExecute(UART_TEST_CMD_GET_ATTRIBUTE));
}

/**
  * @tc.name: UartReliabilityTest001
  * @tc.desc: uart function test
  * @tc.type: FUNC
  * @tc.require: AR000F8689
  */
HWTEST_F(HdfUartTest, UartReliabilityTest001, TestSize.Level1)
{
    struct HdfTestMsg msg = {TEST_PAL_UART_TYPE, UART_TEST_CMD_RELIABILITY, -1};
    EXPECT_EQ(0, HdfTestSendMsgToService(&msg));
    EXPECT_EQ(0, UartTestExecute(UART_TEST_CMD_RELIABILITY));
}

/**
  * @tc.name: UartIfPerformanceTest001
  * @tc.desc: uart user if performance test
  * @tc.type: FUNC
  * @tc.require:
  */
HWTEST_F(HdfUartTest, UartIfPerformanceTest001, TestSize.Level1)
{
    EXPECT_EQ(0, UartTestExecute(UART_TEST_CMD_PERFORMANCE));
}

/**
  * @tc.name: UartMiniBlockWriteTest001
  * @tc.desc: uart mini block write test only for the mini platform
  * @tc.type: FUNC
  * @tc.require:
  */
HWTEST_F(HdfUartTest, UartMiniBlockWriteTest001, TestSize.Level1)
{
    struct HdfTestMsg msg = {TEST_PAL_UART_TYPE, UART_MINI_BLOCK_WRITE_TEST, -1};
    EXPECT_EQ(0, HdfTestSendMsgToService(&msg));
}
