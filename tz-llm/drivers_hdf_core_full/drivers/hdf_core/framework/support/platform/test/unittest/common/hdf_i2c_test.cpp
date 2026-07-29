/*
 * Copyright (c) 2020-2023 Huawei Device Co., Ltd.
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
#include "i2c_test.h"
#include "hdf_io_service_if.h"

using namespace testing::ext;

class HdfI2cTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp();
    void TearDown();
};

void HdfI2cTest::SetUpTestCase()
{
    int32_t ret;
    struct HdfTestMsg msg = {TEST_PAL_I2C_TYPE, I2C_TEST_CMD_SETUP_ALL, -1};
    HdfTestOpenService();
    HdfTestSendMsgToService(&msg);

    ret = I2cTestExecute(I2C_TEST_CMD_SETUP_ALL);
    if (ret != 0) {
        printf("SetUpTestCase: User SetUp FAIL:%d\n\r", ret);
    }
    printf("SetUpTestCase: exit!\n");
}

void HdfI2cTest::TearDownTestCase()
{
    int32_t ret;
    struct HdfTestMsg msg = {TEST_PAL_I2C_TYPE, I2C_TEST_CMD_TEARDOWN_ALL, -1};
    HdfTestSendMsgToService(&msg);
    HdfTestCloseService();

    ret = I2cTestExecute(I2C_TEST_CMD_TEARDOWN_ALL);
    if (ret != 0) {
        printf("TearDownTestCase: User TearDown FAIL:%d\n\r", ret);
    }
    printf("TearDownTestCase: exit!\n");
}

void HdfI2cTest::SetUp()
{
}

void HdfI2cTest::TearDown()
{
}

/**
  * @tc.name: HdfI2cTestTransfer001
  * @tc.desc: i2c transfer test
  * @tc.type: FUNC
  * @tc.require: AR000F8688
  */
HWTEST_F(HdfI2cTest, HdfI2cTestTransfer001, TestSize.Level1)
{
    struct HdfTestMsg msg = {TEST_PAL_I2C_TYPE, I2C_TEST_CMD_TRANSFER, -1};
    EXPECT_EQ(0, HdfTestSendMsgToService(&msg));
    EXPECT_EQ(0, I2cTestExecute(I2C_TEST_CMD_TRANSFER));
}

/**
  * @tc.name: HdfI2cTestMultiThread001
  * @tc.desc: i2c multithread test
  * @tc.type: FUNC
  * @tc.require: AR000F8688
  */
HWTEST_F(HdfI2cTest, HdfI2cTestMultiThread001, TestSize.Level1)
{
    struct HdfTestMsg msg = {TEST_PAL_I2C_TYPE, I2C_TEST_CMD_MULTI_THREAD, -1};
    EXPECT_EQ(0, HdfTestSendMsgToService(&msg));
    EXPECT_EQ(0, I2cTestExecute(I2C_TEST_CMD_MULTI_THREAD));
}

/**
  * @tc.name: HdfI2cTestReliability001
  * @tc.desc: i2c reliability test
  * @tc.type: FUNC
  * @tc.require: AR000F8688
  */
HWTEST_F(HdfI2cTest, HdfI2cTestReliability001, TestSize.Level1)
{
    struct HdfTestMsg msg = {TEST_PAL_I2C_TYPE, I2C_TEST_CMD_RELIABILITY, -1};
    EXPECT_EQ(0, HdfTestSendMsgToService(&msg));
    EXPECT_EQ(0, I2cTestExecute(I2C_TEST_CMD_RELIABILITY));
}

/**
  * @tc.name: HdfI2cTestPeformance001
  * @tc.desc: i2c reliability test
  * @tc.type: FUNC
  * @tc.require: AR000F8688
  */
HWTEST_F(HdfI2cTest, HdfI2cTestPeformance001, TestSize.Level1)
{
    EXPECT_EQ(0, I2cTestExecute(I2C_TEST_CMD_PERFORMANCE));
}

/**
  * @tc.name: HdfI2cMiniWriteReadTest001
  * @tc.desc: i2c write and read test only for the mini platform
  * @tc.type: FUNC
  * @tc.require:
  */
HWTEST_F(HdfI2cTest, HdfI2cMiniWriteReadTest001, TestSize.Level1)
{
    struct HdfTestMsg msg = {TEST_PAL_I2C_TYPE, I2C_MINI_WRITE_READ_TEST, -1};
    EXPECT_EQ(0, HdfTestSendMsgToService(&msg));
}
