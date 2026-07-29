/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include <gtest/gtest.h>
#include "emmc_if.h"
#include "hdf_uhdf_test.h"

using namespace testing::ext;

class HdfEmmcMiniTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp();
    void TearDown();
};

void HdfEmmcMiniTest::SetUpTestCase()
{
}

void HdfEmmcMiniTest::TearDownTestCase()
{
}

void HdfEmmcMiniTest::SetUp()
{
}

void HdfEmmcMiniTest::TearDown()
{
}

static int32_t EmmcMiniGetCardStateTest(void)
{
    DevHandle handle = nullptr;
    int32_t ret;
    uint8_t state;
    uint32_t busNum = 0;

    handle = EmmcOpen(busNum);
    if (handle == nullptr) {
        printf("EmmcMiniGetCardStateTest: emmc open fail, handle is nullptr!\n");
        return HDF_ERR_INVALID_OBJECT;
    }

    ret = EmmcGetCardState(handle, &state, sizeof(state));
    if (ret != HDF_SUCCESS) {
        printf("EmmcMiniGetCardStateTest: emmc get card state fail, ret: %d!\n", ret);
        EmmcClose(handle);
        return ret;
    }
    EmmcClose(handle);
    printf("EmmcMiniGetCardStateTest: all test done!\n");
    return HDF_SUCCESS;
}

static int32_t EmmcMiniGetCardCsdTest(void)
{
    DevHandle handle = nullptr;
    int32_t ret;
    uint8_t csd;
    uint32_t busNum = 0;

    handle = EmmcOpen(busNum);
    if (handle == nullptr) {
        printf("EmmcMiniGetCardCsdTest: emmc open fail, handle is nullptr!\n");
        return HDF_ERR_INVALID_OBJECT;
    }
    ret = EmmcGetCardCsd(handle, &csd, sizeof(csd));
    if (ret != HDF_SUCCESS) {
        printf("EmmcMiniGetCardCsdTest: emmc get card state fail, ret: %d!\n", ret);
        EmmcClose(handle);
        return ret;
    }
    EmmcClose(handle);
    printf("EmmcMiniGetCardCsdTest: all test done!\n");
    return HDF_SUCCESS;
}

static int32_t EmmcMiniGetCardInfoTest(void)
{
    DevHandle handle = nullptr;
    int32_t ret;
    uint8_t cardInfo;
    uint32_t busNum = 0;

    handle = EmmcOpen(busNum);
    if (handle == nullptr) {
        printf("EmmcMiniGetCardInfoTest: emmc open fail, handle is nullptr!\n");
        return HDF_ERR_INVALID_OBJECT;
    }
    ret = EmmcGetCardInfo(handle, &cardInfo, sizeof(cardInfo));
    if (ret != HDF_SUCCESS) {
        printf("EmmcMiniGetCardInfoTest: emmc get card info fail, ret: %d!\n", ret);
        EmmcClose(handle);
        return ret;
    }
    EmmcClose(handle);
    printf("EmmcMiniGetCardInfoTest: all test done!\n");
    return HDF_SUCCESS;
}

/**
  * @tc.name: EmmcMiniTestGetCardState001
  * @tc.desc: emmc mini get card state test.
  * @tc.type: FUNC
  * @tc.require:
  */
HWTEST_F(HdfEmmcMiniTest, EmmcMiniTestGetCardState001, TestSize.Level1)
{
    int32_t ret = EmmcMiniGetCardStateTest();
    ASSERT_EQ(HDF_SUCCESS, ret);
}

/**
  * @tc.name: EmmcMiniTestGetCardCsd001
  * @tc.desc: emmc mini get card csd test.
  * @tc.type: FUNC
  * @tc.require:
  */
HWTEST_F(HdfEmmcMiniTest, EmmcMiniTestGetCardCsd001, TestSize.Level1)
{
    int32_t ret = EmmcMiniGetCardCsdTest();
    ASSERT_EQ(HDF_SUCCESS, ret);
}

/**
  * @tc.name: EmmcMiniTestGetCardInfo001
  * @tc.desc: emmc mini get card info test.
  * @tc.type: FUNC
  * @tc.require:
  */
HWTEST_F(HdfEmmcMiniTest, EmmcMiniTestGetCardInfo001, TestSize.Level1)
{
    int32_t ret = EmmcMiniGetCardInfoTest();
    ASSERT_EQ(HDF_SUCCESS, ret);
}
