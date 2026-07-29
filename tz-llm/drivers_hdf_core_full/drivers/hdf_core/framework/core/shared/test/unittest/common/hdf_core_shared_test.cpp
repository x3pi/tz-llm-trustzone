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
#include <hdf_sbuf.h>
#include <hdi_base.h>

#include "hdf_device_info.h"
#include "hdf_object_manager.h"
#include "hdf_service_status_inner.h"
#include "osal_mem.h"

namespace OHOS {
using namespace testing::ext;

class HdfCoreSharedTest : public testing::Test {
public:
    static void SetUpTestCase() {}
    static void TearDownTestCase() {}

    void SetUp() {};
    void TearDown() {};
};

HWTEST_F(HdfCoreSharedTest, HdfObjectManagerGetObjectTest, TestSize.Level1)
{
    HdfObject *object = HdfObjectManagerGetObject(-1);
    ASSERT_EQ(object, nullptr);
}

HWTEST_F(HdfCoreSharedTest, HdfObjectManagerFreeObjectTest, TestSize.Level1)
{
    HdfObjectManagerFreeObject(nullptr);
    HdfObject *object = (struct HdfObject *)OsalMemCalloc(sizeof(struct HdfObject));
    ASSERT_NE(object, nullptr);
    object->objectId = -1;
    HdfObjectManagerFreeObject(object);
    OsalMemFree(object);
}

HWTEST_F(HdfCoreSharedTest, HdfDeviceInfoFreeInstanceTest, TestSize.Level1)
{
    HdfDeviceInfoConstruct(nullptr);

    HdfDeviceInfoFreeInstance(nullptr);
    HdfDeviceInfo *info = HdfDeviceInfoNewInstance();
    ASSERT_NE(info, nullptr);
    HdfDeviceInfoFreeInstance(info);

    HdfDeviceInfoDelete(nullptr);
}

HWTEST_F(HdfCoreSharedTest, ServiceStatusMarshallingTest, TestSize.Level1)
{
    int ret = ServiceStatusMarshalling(nullptr, nullptr);
    ASSERT_EQ(ret, HDF_ERR_INVALID_PARAM);
    ServiceStatus *status = (struct ServiceStatus *)OsalMemCalloc(sizeof(struct ServiceStatus));
    ASSERT_NE(status, nullptr);
    ret = ServiceStatusMarshalling(status, nullptr);
    ASSERT_EQ(ret, HDF_ERR_INVALID_PARAM);
    OsalMemFree(status);
}

HWTEST_F(HdfCoreSharedTest, ServiceStatusUnMarshallingTest, TestSize.Level1)
{
    int ret = ServiceStatusUnMarshalling(nullptr, nullptr);
    ASSERT_EQ(ret, HDF_ERR_INVALID_PARAM);
    ServiceStatus *status = (struct ServiceStatus *)OsalMemCalloc(sizeof(struct ServiceStatus));
    ASSERT_NE(status, nullptr);
    HdfSBuf *sBuf = HdfSbufTypedObtain(SBUF_IPC);
    ASSERT_NE(sBuf, nullptr);
    ret = ServiceStatusUnMarshalling(status, sBuf);
    ASSERT_EQ(ret, HDF_FAILURE);
    const char *serviceName = "testServiceName";
    HdfSbufWriteString(sBuf, serviceName);
    ret = ServiceStatusUnMarshalling(status, sBuf);
    ASSERT_EQ(ret, HDF_FAILURE);
    OsalMemFree(status);
    HdfSbufRecycle(sBuf);
}
} // namespace OHOS
