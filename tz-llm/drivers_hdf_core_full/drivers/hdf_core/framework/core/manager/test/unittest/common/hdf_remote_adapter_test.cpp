/*
 * Copyright (c) 2022-2023 Huawei Device Co., Ltd.
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
#include <hdf_sbuf.h>
#include <hdi_base.h>
#include <iostream>

#include "hdf_remote_adapter.h"
#include "hdf_remote_adapter_if.h"
#include "hdf_sbuf_ipc.h"

namespace OHOS {
using namespace testing::ext;

class HdfRemoteAdapterTest : public testing::Test {
public:
    static void SetUpTestCase() {}
    static void TearDownTestCase() {}

    void SetUp() {};
    void TearDown() {};
};

HWTEST_F(HdfRemoteAdapterTest, HdfRemoteAdapterTest001, TestSize.Level1)
{
    HdfRemoteServiceHolder *holder = new HdfRemoteServiceHolder();
    int ret = holder->SetInterfaceDescriptor(NULL);
    ASSERT_EQ(ret, false);
    const char *desc = "";
    ret = holder->SetInterfaceDescriptor(desc);
    ASSERT_EQ(ret, false);
    ret = HdfRemoteAdapterSetInterfaceDesc(nullptr, nullptr);
    ASSERT_EQ(ret, false);

    HdfRemoteAdapterAddDeathRecipient(NULL, NULL);
    HdfRemoteAdapterAddDeathRecipient(reinterpret_cast<struct HdfRemoteService *>(holder), NULL);
    HdfRemoteAdapterRemoveDeathRecipient(NULL, NULL);
    HdfRemoteAdapterRemoveDeathRecipient(reinterpret_cast<struct HdfRemoteService *>(holder), NULL);

    delete holder;
}

HWTEST_F(HdfRemoteAdapterTest, HdfRemoteAdapterTest002, TestSize.Level1)
{
    int ret = HdfRemoteAdapterAddService(NULL, NULL);
    ASSERT_EQ(ret, HDF_ERR_INVALID_PARAM);
    const char *name = "";
    ret = HdfRemoteAdapterAddService(name, NULL);
    ASSERT_EQ(ret, HDF_ERR_INVALID_PARAM);

    HdfRemoteService *remote = HdfRemoteAdapterGetService(NULL);
    ASSERT_EQ(remote, nullptr);
    remote = HdfRemoteAdapterGetService(name);
    ASSERT_EQ(remote, nullptr);

    ret = HdfRemoteAdapterAddSa(-1, NULL);
    ASSERT_EQ(ret, HDF_ERR_INVALID_PARAM);

    remote = HdfRemoteAdapterGetSa(-1);
    ASSERT_EQ(remote, nullptr);
}

HWTEST_F(HdfRemoteAdapterTest, HdfRemoteAdapterTest003, TestSize.Level1)
{
    bool ret = HdfRemoteAdapterSetInterfaceDesc(NULL, NULL);
    ASSERT_EQ(ret, false);

    HdfRemoteServiceHolder *holder = new HdfRemoteServiceHolder();
    HdfSBuf *sBuf = HdfSbufTypedObtain(SBUF_IPC);
    ret = HdfRemoteAdapterWriteInterfaceToken(reinterpret_cast<struct HdfRemoteService *>(holder), sBuf);
    ASSERT_EQ(ret, false);

    ret = HdfRemoteAdapterWriteInterfaceToken(NULL, NULL);
    ASSERT_EQ(ret, false);
    ret = HdfRemoteAdapterWriteInterfaceToken(reinterpret_cast<struct HdfRemoteService *>(holder), NULL);
    ASSERT_EQ(ret, false);
    ret = HdfRemoteAdapterWriteInterfaceToken(reinterpret_cast<struct HdfRemoteService *>(holder), sBuf);
    ASSERT_EQ(ret, false);

    ret = HdfRemoteAdapterCheckInterfaceToken(NULL, NULL);
    ASSERT_EQ(ret, false);
    ret = HdfRemoteAdapterCheckInterfaceToken(reinterpret_cast<struct HdfRemoteService *>(holder), NULL);
    ASSERT_EQ(ret, false);
    HdfSbufRecycle(sBuf);
    delete holder;
}

HWTEST_F(HdfRemoteAdapterTest, HdfRemoteAdapterTest004, TestSize.Level1)
{
    int ret = HdfRemoteGetCallingPid();
    ASSERT_TRUE(ret > 0);
    ret = HdfRemoteGetCallingUid();
    ASSERT_TRUE(ret >= 0);
}

/*
 * Test method CheckInterfaceTokenIngoreVersion, which can ignore version when checking interface description
 */
HWTEST_F(HdfRemoteAdapterTest, HdfRemoteAdapterTest005, TestSize.Level1)
{
    struct HdfRemoteService *service = HdfRemoteAdapterObtain();
    ASSERT_NE(service, nullptr);
    bool status = HdfRemoteServiceSetInterfaceDesc(service, "ohos.hdi.foo.v1_1.ifoo");
    ASSERT_TRUE(status);
    HdfSBuf *sBuf = HdfSbufTypedObtain(SBUF_IPC);
    ASSERT_NE(sBuf, nullptr);
    OHOS::MessageParcel *parcel = nullptr;
    int ret = SbufToParcel(sBuf, &parcel);
    ASSERT_EQ(ret, HDF_SUCCESS);
    status = parcel->WriteInterfaceToken(u"ohos.hdi.foo.v1_0.ifoo");
    ASSERT_TRUE(status);
    status = HdfRemoteServiceCheckInterfaceToken(service, sBuf);
    ASSERT_TRUE(status);
    HdfRemoteServiceRecycle(service);
}

HWTEST_F(HdfRemoteAdapterTest, HdfRemoteAdapterTest006, TestSize.Level1)
{
    struct HdfRemoteService *service = HdfRemoteAdapterObtain();
    ASSERT_NE(service, nullptr);
    int status = HdfRemoteServiceSetInterfaceDesc(service, "ohos.hdi.foo.v1_1.ifoo");
    ASSERT_TRUE(status);
    HdfSBuf *sBuf = HdfSbufTypedObtain(SBUF_IPC);
    ASSERT_NE(sBuf, nullptr);
    OHOS::MessageParcel *parcel = nullptr;
    int ret = SbufToParcel(sBuf, &parcel);
    ASSERT_EQ(ret, HDF_SUCCESS);
    status = parcel->WriteInterfaceToken(u"ohos.hdi.foo.v1_0");
    ASSERT_TRUE(status);
    status = HdfRemoteServiceCheckInterfaceToken(service, sBuf);
    ASSERT_FALSE(status);
    HdfRemoteServiceRecycle(service);
}

HWTEST_F(HdfRemoteAdapterTest, HdfRemoteAdapterTest007, TestSize.Level1)
{
    struct HdfRemoteService *service = HdfRemoteAdapterObtain();
    ASSERT_NE(service, nullptr);
    int status = HdfRemoteServiceSetInterfaceDesc(service, "ohos.hdi.foo.v1_1.ifoo");
    ASSERT_TRUE(status);
    HdfSBuf *sBuf = HdfSbufTypedObtain(SBUF_IPC);
    ASSERT_NE(sBuf, nullptr);
    OHOS::MessageParcel *parcel = nullptr;
    int ret = SbufToParcel(sBuf, &parcel);
    ASSERT_EQ(ret, HDF_SUCCESS);
    status = parcel->WriteInterfaceToken(u"ohos.hdi.foo.v1_0.ifoo_");
    ASSERT_TRUE(status);
    status = HdfRemoteServiceCheckInterfaceToken(service, sBuf);
    ASSERT_FALSE(status);
    HdfRemoteServiceRecycle(service);
}
} // namespace OHOS
