/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include <gtest/gtest.h>

#include <devmgr_hdi.h>
#include <osal_time.h>
#include <servmgr_hdi.h>

using namespace testing::ext;
static constexpr const char *TEST_SERVICE_NAME = "sample_driver_service";
static constexpr const char *TEST_DEV_NODE = "/sys/devices/virtual/hdf/hdf_uevent_ut/uevent";
static constexpr const char *ADD_EVENT_CMD = "echo \"add\" > /sys/devices/virtual/hdf/hdf_uevent_ut/uevent";
static constexpr const char *REMOVE_EVENT_CMD = "echo \"remove\" > /sys/devices/virtual/hdf/hdf_uevent_ut/uevent";

class DevmgrUeventTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp();
    void TearDown();

    static const uint32_t waitTime = 30;
    static const uint32_t timeout = 200;
    static const uint32_t stressTime = 10;
    static struct HDIServiceManager *servmgr;
    static struct HDIDeviceManager *devmgr;
    static bool haveDevNode;
};

struct HDIServiceManager *DevmgrUeventTest::servmgr = nullptr;
struct HDIDeviceManager *DevmgrUeventTest::devmgr = nullptr;
bool DevmgrUeventTest::haveDevNode = false;

void DevmgrUeventTest::SetUpTestCase()
{
    // Confirm whether there is a kernel node of HDF, testing is supported only when there is a kernel node
    if (access(TEST_DEV_NODE, F_OK) == 0) {
        haveDevNode = true;
        servmgr = HDIServiceManagerGet();
        devmgr = HDIDeviceManagerGet();
    }
}

void DevmgrUeventTest::TearDownTestCase() {}

void DevmgrUeventTest::SetUp() {}

void DevmgrUeventTest::TearDown() {}

/**
 * @tc.name: DevmgrUeventTestAdd
 * @tc.desc: trigger add uevent
 * @tc.type: FUNC
 * @tc.require: SR000H0E0E
 */
HWTEST_F(DevmgrUeventTest, DevmgrUeventTestAdd, TestSize.Level3)
{
    if (!haveDevNode) {
        ASSERT_TRUE(true);
        return;
    }

    ASSERT_TRUE(servmgr != nullptr);
    ASSERT_TRUE(devmgr != nullptr);

    // prepare:ensure that the service is not loaded
    int ret = devmgr->UnloadDevice(devmgr, TEST_SERVICE_NAME);
    ASSERT_EQ(ret, HDF_SUCCESS);

    // prepare:waiting for service offline
    uint32_t cnt = 0;
    struct HdfRemoteService *sampleService = servmgr->GetService(servmgr, TEST_SERVICE_NAME);
    while (sampleService != nullptr && cnt < timeout) {
        OsalMSleep(waitTime);
        sampleService = servmgr->GetService(servmgr, TEST_SERVICE_NAME);
        cnt++;
    }

    // prepare:confirm that the service is unavailable
    ASSERT_TRUE(sampleService == nullptr);

    // trigger add uevent
    system(ADD_EVENT_CMD);
    cnt = 0;
    sampleService = servmgr->GetService(servmgr, TEST_SERVICE_NAME);
    while (sampleService == nullptr && cnt < timeout) {
        OsalMSleep(waitTime);
        sampleService = servmgr->GetService(servmgr, TEST_SERVICE_NAME);
        cnt++;
    }

    // expect:confirm that the service is available
    ASSERT_TRUE(sampleService != nullptr);
}

/**
 * @tc.name: DevmgrUeventTestRemove
 * @tc.desc: trigger remove uevent
 * @tc.type: FUNC
 * @tc.require: SR000H0E0E
 */
HWTEST_F(DevmgrUeventTest, DevmgrUeventTestRemove, TestSize.Level3)
{
    if (!haveDevNode) {
        ASSERT_TRUE(true);
        return;
    }

    ASSERT_TRUE(servmgr != nullptr);
    ASSERT_TRUE(devmgr != nullptr);

    // prepare:ensure that the service is loaded
    int ret = devmgr->LoadDevice(devmgr, TEST_SERVICE_NAME);
    ASSERT_EQ(ret, HDF_SUCCESS);

    // prepare:waiting for service online
    struct HdfRemoteService *sampleService = servmgr->GetService(servmgr, TEST_SERVICE_NAME);
    uint32_t cnt = 0;
    while (sampleService == nullptr && cnt < timeout) {
        OsalMSleep(waitTime);
        sampleService = servmgr->GetService(servmgr, TEST_SERVICE_NAME);
        cnt++;
    }

    // prepare:confirm that the service is available
    ASSERT_TRUE(sampleService != nullptr);

    // trigger remove uevent
    system(REMOVE_EVENT_CMD);
    cnt = 0;
    sampleService = servmgr->GetService(servmgr, TEST_SERVICE_NAME);
    while (sampleService != nullptr && cnt < timeout) {
        OsalMSleep(waitTime);
        sampleService = servmgr->GetService(servmgr, TEST_SERVICE_NAME);
        cnt++;
    }

    // expect:confirm that the service is unavailable
    ASSERT_TRUE(sampleService == nullptr);
}

/**
 * @tc.name: DevmgrUeventStressTest
 * @tc.desc: Stress Test
 * @tc.type: FUNC
 * @tc.require: SR000H0E0E
 */
HWTEST_F(DevmgrUeventTest, DevmgrUeventStressTest, TestSize.Level3)
{
    if (!haveDevNode) {
        ASSERT_TRUE(true);
        return;
    }

    ASSERT_TRUE(servmgr != nullptr);
    ASSERT_TRUE(devmgr != nullptr);

    // prepare:ensure that the service is not loaded
    int ret = devmgr->UnloadDevice(devmgr, TEST_SERVICE_NAME);
    ASSERT_EQ(ret, HDF_SUCCESS);

    // prepare:waiting for service offline
    uint32_t cnt = 0;
    struct HdfRemoteService *sampleService = servmgr->GetService(servmgr, TEST_SERVICE_NAME);
    while (sampleService != nullptr && cnt < timeout) {
        OsalMSleep(waitTime);
        sampleService = servmgr->GetService(servmgr, TEST_SERVICE_NAME);
        cnt++;
    }

    // prepare:confirm that the service is unavailable
    ASSERT_TRUE(sampleService == nullptr);

    for (uint32_t i = 0; i < stressTime; i++) {
        // trigger add uevent
        system(ADD_EVENT_CMD);
        cnt = 0;
        sampleService = servmgr->GetService(servmgr, TEST_SERVICE_NAME);
        while (sampleService == nullptr && cnt < timeout) {
            OsalMSleep(waitTime);
            sampleService = servmgr->GetService(servmgr, TEST_SERVICE_NAME);
            cnt++;
        }

        // expect:confirm that the service is available
        ASSERT_TRUE(sampleService != nullptr);

        // trigger remove uevent
        system(REMOVE_EVENT_CMD);
        cnt = 0;
        sampleService = servmgr->GetService(servmgr, TEST_SERVICE_NAME);
        while (sampleService != nullptr && cnt < timeout) {
            OsalMSleep(waitTime);
            sampleService = servmgr->GetService(servmgr, TEST_SERVICE_NAME);
            cnt++;
        }

        // expect:confirm that the service is unavailable
        ASSERT_TRUE(sampleService == nullptr);
    }
}
