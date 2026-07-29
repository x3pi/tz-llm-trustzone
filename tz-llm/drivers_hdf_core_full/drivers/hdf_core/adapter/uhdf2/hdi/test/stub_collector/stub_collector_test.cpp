/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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
#include "hdi_support.h"
#include "osal_mem.h"
#include "stub_collector.h"

using namespace testing::ext;

namespace OHOS {
    struct TestService {
    };

    struct HdfRemoteService **TestConstruct(void *service)
    {
        return nullptr;
    }

    void TestDestruct(struct HdfRemoteService **remote)
    {
        (void)remote;
    }
    

    const char *ifdesc = "test stub constructor";

    struct StubConstructor testConstructor = {
        .constructor = TestConstruct,
        .destructor = TestDestruct,
    };

class StubCollectorTest : public testing::Test {
public:
    static void SetUpTestCase() {}
    static void TearDownTestCase() {}

    void SetUp() {};
    void TearDown() {};
};

HWTEST_F(StubCollectorTest, StubCollectorTest001, TestSize.Level1)
{
    StubConstructorRegister(nullptr, nullptr);
    StubConstructorRegister(ifdesc, nullptr);
    StubConstructorRegister(ifdesc, &testConstructor);
    StubConstructorRegister(ifdesc, &testConstructor);

    StubConstructorUnregister(ifdesc, &testConstructor);
    StubConstructorUnregister(nullptr, nullptr);
    StubConstructorUnregister(ifdesc, nullptr);

    struct HdfRemoteService **remote = StubCollectorGetOrNewObject(nullptr, nullptr);
    ASSERT_EQ(remote, nullptr);
}

HWTEST_F(StubCollectorTest, StubCollectorTest002, TestSize.Level1)
{
    struct TestService *service = (struct TestService *)OsalMemCalloc(sizeof(struct TestService));
    ASSERT_NE(service, nullptr);
    struct HdfRemoteService **remote = StubCollectorGetOrNewObject(ifdesc, service);
    ASSERT_EQ(remote, nullptr);
    OsalMemFree(service);
}

// constructor is nullptr
HWTEST_F(StubCollectorTest, StubCollectorTest003, TestSize.Level1)
{
    const char *tmpDesc = "tmp desc";
    struct StubConstructor tmpConstructor = {
        .constructor = nullptr,
        .destructor = nullptr,
    };

    struct TestService *service = (struct TestService *)OsalMemCalloc(sizeof(struct TestService));
    ASSERT_NE(service, nullptr);
    StubConstructorRegister(tmpDesc, &tmpConstructor);
    struct HdfRemoteService **remote = StubCollectorGetOrNewObject(tmpDesc, service);
    ASSERT_EQ(remote, nullptr);

    StubConstructorUnregister(tmpDesc, &tmpConstructor);
    OsalMemFree(service);
}

// test calling construct and return nullptr
HWTEST_F(StubCollectorTest, StubCollectorTest004, TestSize.Level1)
{
    struct TestService *service = (struct TestService *)OsalMemCalloc(sizeof(struct TestService));
    ASSERT_NE(service, nullptr);
    StubConstructorRegister(ifdesc, &testConstructor);
    struct HdfRemoteService **remote = StubCollectorGetOrNewObject(ifdesc, service);
    ASSERT_EQ(remote, nullptr);
    StubConstructorUnregister(ifdesc, &testConstructor);
    OsalMemFree(service);
}

HWTEST_F(StubCollectorTest, StubCollectorTest005, TestSize.Level1)
{
    StubCollectorRemoveObject(nullptr, nullptr);
    StubCollectorRemoveObject(ifdesc, nullptr);

    struct TestService *service = (struct TestService *)OsalMemCalloc(sizeof(struct TestService));
    ASSERT_NE(service, nullptr);
    StubCollectorRemoveObject(ifdesc, service);
    OsalMemFree(service);
}

HWTEST_F(StubCollectorTest, StubCollectorTest006, TestSize.Level1)
{
    void *impl = LoadHdiImpl(nullptr, nullptr);
    ASSERT_EQ(impl, nullptr);

    impl = LoadHdiImpl("test", nullptr);
    ASSERT_EQ(impl, nullptr);

    impl = LoadHdiImpl("", "test_service");
    ASSERT_EQ(impl, nullptr);

    impl = LoadHdiImpl("test", "");
    ASSERT_EQ(impl, nullptr);

    impl = LoadHdiImpl("test", "test_service");
    ASSERT_EQ(impl, nullptr);

    impl = LoadHdiImpl("ohos.hdi.test.v1_0.ITest", "test_service");
    ASSERT_EQ(impl, nullptr);
}

HWTEST_F(StubCollectorTest, StubCollectorTest007, TestSize.Level1)
{
    const char *desc = "test";
    struct TestService *service = (struct TestService *)OsalMemCalloc(sizeof(struct TestService));
    ASSERT_NE(service, nullptr);

    UnloadHdiImpl(nullptr, nullptr, nullptr);
    UnloadHdiImpl(desc, nullptr, nullptr);
    UnloadHdiImpl(desc, "test_service", service);

    OsalMemFree(service);
}
} // namespace OHOS