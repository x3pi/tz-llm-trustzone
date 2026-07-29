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
#include <gtest/gtest.h>
#include <string>
#include <unistd.h>
#include "hdf_uhdf_test.h"
#include "pwm_test.h"
#include "hdf_io_service_if.h"

using namespace testing::ext;

class HdfPwmTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp();
    void TearDown();
};

void HdfPwmTest::SetUpTestCase()
{
    HdfTestOpenService();
}

void HdfPwmTest::TearDownTestCase()
{
    HdfTestCloseService();
}

void HdfPwmTest::SetUp()
{
}

void HdfPwmTest::TearDown()
{
}

/**
  * @tc.name: PwmSetPeriodTest001
  * @tc.desc: pwm function test
  * @tc.type: FUNC
  * @tc.require: AR000F868C
  */
HWTEST_F(HdfPwmTest, PwmSetPeriodTest001, TestSize.Level1)
{
    struct HdfTestMsg msg = {TEST_PAL_PWM_TYPE, PWM_SET_PERIOD_TEST, -1};
    EXPECT_EQ(0, HdfTestSendMsgToService(&msg));
    EXPECT_EQ(0, PwmTestExecute(PWM_SET_PERIOD_TEST));
}

/**
  * @tc.name: PwmSetDutyTest001
  * @tc.desc: pwm function test
  * @tc.type: FUNC
  * @tc.require: AR000F868C
  */
HWTEST_F(HdfPwmTest, PwmSetDutyTest001, TestSize.Level1)
{
    struct HdfTestMsg msg = {TEST_PAL_PWM_TYPE, PWM_SET_DUTY_TEST, -1};
    EXPECT_EQ(0, HdfTestSendMsgToService(&msg));
    EXPECT_EQ(0, PwmTestExecute(PWM_SET_DUTY_TEST));
}

/**
  * @tc.name: PwmSetPolarityTest001
  * @tc.desc: pwm function test
  * @tc.type: FUNC
  * @tc.require: AR000F868C
  */
HWTEST_F(HdfPwmTest, PwmSetPolarityTest001, TestSize.Level1)
{
    struct HdfTestMsg msg = { TEST_PAL_PWM_TYPE, PWM_SET_POLARITY_TEST, -1};
    EXPECT_EQ(0, HdfTestSendMsgToService(&msg));
    EXPECT_EQ(0, PwmTestExecute(PWM_SET_POLARITY_TEST));
}

/**
  * @tc.name: PwmSetGetConfigTest001
  * @tc.desc: pwm function test
  * @tc.type: FUNC
  * @tc.require: AR000F868D
  */
HWTEST_F(HdfPwmTest, PwmSetGetConfigTest001, TestSize.Level1)
{
    struct HdfTestMsg msg = {TEST_PAL_PWM_TYPE, PWM_SET_GET_CONFIG_TEST, -1};
    EXPECT_EQ(0, HdfTestSendMsgToService(&msg));
    EXPECT_EQ(0, PwmTestExecute(PWM_SET_GET_CONFIG_TEST));
}

/**
  * @tc.name: PwmEnableTest001
  * @tc.desc: pwm function test
  * @tc.type: FUNC
  * @tc.require: AR000F868D
  */
HWTEST_F(HdfPwmTest, PwmEnableTest001, TestSize.Level1)
{
    struct HdfTestMsg msg = {TEST_PAL_PWM_TYPE, PWM_ENABLE_TEST, -1};
    EXPECT_EQ(0, HdfTestSendMsgToService(&msg));
    EXPECT_EQ(0, PwmTestExecute(PWM_ENABLE_TEST));
}

/**
  * @tc.name: PwmDisableTest001
  * @tc.desc: pwm function test
  * @tc.type: FUNC
  * @tc.require: AR000F868D
  */
HWTEST_F(HdfPwmTest, PwmDisableTest001, TestSize.Level1)
{
    struct HdfTestMsg msg = {TEST_PAL_PWM_TYPE, PWM_DISABLE_TEST, -1};
    EXPECT_EQ(0, HdfTestSendMsgToService(&msg));
    EXPECT_EQ(0, PwmTestExecute(PWM_DISABLE_TEST));
}

/**
  * @tc.name: PwmIfPerformanceTest001
  * @tc.desc: pwm user if performance test
  * @tc.type: FUNC
  * @tc.require:
  */
HWTEST_F(HdfPwmTest, PwmIfPerformanceTest001, TestSize.Level1)
{
    EXPECT_EQ(0, PwmTestExecute(PWM_IF_PERFORMANCE_TEST));
}
