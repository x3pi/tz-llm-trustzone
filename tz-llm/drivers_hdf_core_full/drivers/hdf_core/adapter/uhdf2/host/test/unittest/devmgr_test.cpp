/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


#include <gtest/gtest.h>

#include <devmgr_hdi.h>
#include <osal_time.h>
#include <servmgr_hdi.h>

#define HDF_LOG_TAG   driver_manager

namespace OHOS {
using namespace testing::ext;

static constexpr const char *TEST_SERVICE_NAME = "sample_driver_service";

class DevMgrTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp();
    void TearDown();
    static const uint32_t waitTime = 30;
    static const uint32_t timeout = 200;
    static struct HDIServiceManager *servmgr;
    static struct HDIDeviceManager *devmgr;
};

struct HDIServiceManager *DevMgrTest::servmgr = nullptr;
struct HDIDeviceManager *DevMgrTest::devmgr = nullptr;

void DevMgrTest::SetUpTestCase()
{
    servmgr = HDIServiceManagerGet();
    devmgr = HDIDeviceManagerGet();
}

void DevMgrTest::TearDownTestCase()
{
}

void DevMgrTest::SetUp()
{
}

void DevMgrTest::TearDown()
{
}

/*
* @tc.name: DriverLoaderTest
* @tc.desc: driver load test
* @tc.type: FUNC
* @tc.require: AR000DT1TK
*/
HWTEST_F(DevMgrTest, DriverLoaderTest, TestSize.Level1)
{
    ASSERT_TRUE(servmgr != nullptr);
    ASSERT_TRUE(devmgr != nullptr);

    int ret = devmgr->LoadDevice(devmgr, TEST_SERVICE_NAME);
    ASSERT_EQ(ret, HDF_SUCCESS);

    struct HdfRemoteService *sampleService = servmgr->GetService(servmgr, TEST_SERVICE_NAME);
    uint32_t cnt = 0;
    while (sampleService == nullptr && cnt < timeout) {
        OsalMSleep(waitTime);
        sampleService = servmgr->GetService(servmgr, TEST_SERVICE_NAME);
        cnt++;
    }

    ASSERT_TRUE(sampleService != nullptr);
}
/*
* @tc.name: DriverUnLoaderTest
* @tc.desc: driver unload test
* @tc.type: FUNC
* @tc.require: AR000DT1TK
*/
HWTEST_F(DevMgrTest, DriverUnLoaderTest, TestSize.Level1)
{
    ASSERT_TRUE(servmgr != nullptr);
    ASSERT_TRUE(devmgr != nullptr);

    int ret = devmgr->UnloadDevice(devmgr, TEST_SERVICE_NAME);
    ASSERT_EQ(ret, HDF_SUCCESS);

    uint32_t cnt = 0;
    struct HdfRemoteService *sampleService = servmgr->GetService(servmgr, TEST_SERVICE_NAME);
    while (sampleService != nullptr && cnt < timeout) {
        OsalMSleep(waitTime);
        sampleService = servmgr->GetService(servmgr, TEST_SERVICE_NAME);
        cnt++;
    }

    ASSERT_TRUE(sampleService == nullptr);
}

HWTEST_F(DevMgrTest, DriverTest, TestSize.Level1)
{
    ASSERT_TRUE(servmgr != nullptr);
    ASSERT_TRUE(devmgr != nullptr);
    int ret;
    constexpr int loop = 100;

    for (int i = 0; i < loop; i++) {
        ret = devmgr->LoadDevice(devmgr, TEST_SERVICE_NAME);
        ASSERT_EQ(ret, HDF_SUCCESS);
        uint32_t cnt = 0;
        struct HdfRemoteService *sampleService = servmgr->GetService(servmgr, TEST_SERVICE_NAME);
        while (sampleService == nullptr && cnt < timeout) {
            OsalMSleep(waitTime);
            sampleService = servmgr->GetService(servmgr, TEST_SERVICE_NAME);
            cnt++;
        }
        ASSERT_TRUE(sampleService != nullptr);

        ret = devmgr->UnloadDevice(devmgr, TEST_SERVICE_NAME);
        ASSERT_EQ(ret, HDF_SUCCESS);
        cnt = 0;
        sampleService = servmgr->GetService(servmgr, TEST_SERVICE_NAME);
        while (sampleService != nullptr && cnt < timeout) {
            OsalMSleep(waitTime);
            sampleService = servmgr->GetService(servmgr, TEST_SERVICE_NAME);
            cnt++;
        }
        ASSERT_TRUE(sampleService == nullptr);
    }
}
} // namespace OHOS
