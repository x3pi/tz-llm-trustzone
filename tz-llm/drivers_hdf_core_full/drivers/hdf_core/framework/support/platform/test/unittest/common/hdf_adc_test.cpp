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
#include "hdf_uhdf_test.h"
#include "adc_test.h"
#include "hdf_io_service_if.h"

using namespace testing::ext;

class HdfAdcTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp();
    void TearDown();
};

void HdfAdcTest::SetUpTestCase()
{
    HdfTestOpenService();
}

void HdfAdcTest::TearDownTestCase()
{
    HdfTestCloseService();
}

void HdfAdcTest::SetUp()
{
}

void HdfAdcTest::TearDown()
{
}

/**
  * @tc.name: AdcTestRead001
  * @tc.desc: adc read test
  * @tc.type: FUNC
  * @tc.require: NA
  */
HWTEST_F(HdfAdcTest, AdcTestRead001, TestSize.Level1)
{
    struct HdfTestMsg msg = {TEST_PAL_ADC_TYPE, ADC_TEST_CMD_READ, -1};
    EXPECT_EQ(0, HdfTestSendMsgToService(&msg));
    EXPECT_EQ(0, AdcTestExecute(ADC_TEST_CMD_READ));
}

/**
  * @tc.name: AdcTestMultiThread001
  * @tc.desc: adc multi thread test
  * @tc.type: FUNC
  * @tc.require: NA
  */
HWTEST_F(HdfAdcTest, AdcTestMultiThread001, TestSize.Level1)
{
    struct HdfTestMsg msg = {TEST_PAL_ADC_TYPE, ADC_TEST_CMD_MULTI_THREAD, -1};
    EXPECT_EQ(0, HdfTestSendMsgToService(&msg));
    EXPECT_EQ(0, AdcTestExecute(ADC_TEST_CMD_MULTI_THREAD));
}

/**
  * @tc.name: AdcTestReliability001
  * @tc.desc: adc reliability test
  * @tc.type: FUNC
  * @tc.require: NA
  */
HWTEST_F(HdfAdcTest, AdcTestReliability001, TestSize.Level1)
{
    struct HdfTestMsg msg = {TEST_PAL_ADC_TYPE, ADC_TEST_CMD_RELIABILITY, -1};
    EXPECT_EQ(0, HdfTestSendMsgToService(&msg));
    EXPECT_EQ(0, AdcTestExecute(ADC_TEST_CMD_RELIABILITY));
}

/**
  * @tc.name: AdcIfPerformanceTest001
  * @tc.desc: adc user if performance test
  * @tc.type: FUNC
  * @tc.require:
  */
HWTEST_F(HdfAdcTest, AdcIfPerformanceTest001, TestSize.Level1)
{
    EXPECT_EQ(0, AdcTestExecute(ADC_IF_PERFORMANCE_TEST));
}
