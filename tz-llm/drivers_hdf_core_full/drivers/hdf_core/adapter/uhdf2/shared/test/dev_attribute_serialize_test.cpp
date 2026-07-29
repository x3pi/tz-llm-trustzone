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
#include "dev_attribute_serialize.h"
#include "hdf_device_desc.h"

using namespace testing::ext;

namespace OHOS {
class DevAttributeSerializeTest : public testing::Test {
public:
    static void SetUpTestCase() {}
    static void TearDownTestCase() {}

    void SetUp() {};
    void TearDown() {};
};

HWTEST_F(DevAttributeSerializeTest, DevAttributeSerializeTest001, TestSize.Level1)
{
    bool ret = DeviceAttributeSerialize(nullptr, nullptr);
    ASSERT_FALSE(ret);
}

HWTEST_F(DevAttributeSerializeTest, DevAttributeSerializeTest002, TestSize.Level1)
{
    struct HdfDeviceInfo *attribute = HdfDeviceInfoNewInstance();
    ASSERT_NE(attribute, nullptr);
    bool ret = DeviceAttributeSerialize(attribute, nullptr);
    ASSERT_FALSE(ret);
    HdfDeviceInfoFreeInstance(attribute);
    attribute = nullptr;
}

HWTEST_F(DevAttributeSerializeTest, DevAttributeSerializeTest003, TestSize.Level1)
{
    struct HdfSBuf *buf = HdfSbufTypedObtain(SBUF_IPC);
    ASSERT_NE(buf, nullptr);

    bool ret = DeviceAttributeSerialize(nullptr, buf);
    ASSERT_FALSE(ret);
    HdfSbufRecycle(buf);
    buf = nullptr;
}

HWTEST_F(DevAttributeSerializeTest, DevAttributeSerializeTest004, TestSize.Level1)
{
    struct HdfDeviceInfo *attribute = HdfDeviceInfoNewInstance();
    ASSERT_NE(attribute, nullptr);
    struct HdfSBuf *buf = HdfSbufTypedObtain(SBUF_IPC);
    ASSERT_NE(buf, nullptr);

    bool ret = DeviceAttributeSerialize(attribute, buf);
    ASSERT_FALSE(ret);
    HdfDeviceInfoFreeInstance(attribute);
    attribute = nullptr;
    HdfSbufRecycle(buf);
    buf = nullptr;
}

HWTEST_F(DevAttributeSerializeTest, DevAttributeSerializeTest005, TestSize.Level1)
{
    struct HdfDeviceInfo *attribute = HdfDeviceInfoNewInstance();
    ASSERT_NE(attribute, nullptr);

    attribute->moduleName = strdup("test_module");
    attribute->svcName = strdup("test_service");
    attribute->deviceName = strdup("test_device");

    struct HdfSBuf *buf = HdfSbufTypedObtain(SBUF_IPC);
    ASSERT_NE(buf, nullptr);

    bool ret = DeviceAttributeSerialize(attribute, buf);
    ASSERT_TRUE(ret);

    HdfDeviceInfoFreeInstance(attribute);
    attribute = nullptr;
    HdfSbufRecycle(buf);
    buf = nullptr;
}

HWTEST_F(DevAttributeSerializeTest, DevAttributeSerializeTest006, TestSize.Level1)
{
    struct HdfDeviceInfo *attribute = DeviceAttributeDeserialize(nullptr);
    ASSERT_EQ(attribute, nullptr);
}

HWTEST_F(DevAttributeSerializeTest, DevAttributeSerializeTest007, TestSize.Level1)
{
    struct HdfSBuf *buf = HdfSbufTypedObtain(SBUF_IPC);
    ASSERT_NE(buf, nullptr);

    // read deviceId failed
    struct HdfDeviceInfo *attribute = DeviceAttributeDeserialize(buf);
    ASSERT_EQ(attribute, nullptr);

    HdfSbufRecycle(buf);
    buf = nullptr;
}

HWTEST_F(DevAttributeSerializeTest, DevAttributeSerializeTest008, TestSize.Level1)
{
    struct HdfSBuf *buf = HdfSbufTypedObtain(SBUF_IPC);
    ASSERT_NE(buf, nullptr);

    // write deviceId
    bool ret = HdfSbufWriteUint32(buf, 0);
    ASSERT_TRUE(ret);

    // read policy failed
    struct HdfDeviceInfo *attribute = DeviceAttributeDeserialize(buf);
    ASSERT_EQ(attribute, nullptr);

    HdfSbufRecycle(buf);
    buf = nullptr;
}

HWTEST_F(DevAttributeSerializeTest, DevAttributeSerializeTest009, TestSize.Level1)
{
    struct HdfSBuf *buf = HdfSbufTypedObtain(SBUF_IPC);
    ASSERT_NE(buf, nullptr);

    // write deviceId
    bool ret = HdfSbufWriteUint32(buf, 0);
    ASSERT_TRUE(ret);
    // write policy
    ret = HdfSbufWriteUint16(buf, SERVICE_POLICY_INVALID);
    ASSERT_TRUE(ret);

    // read svcName failed
    struct HdfDeviceInfo *attribute = DeviceAttributeDeserialize(buf);
    ASSERT_EQ(attribute, nullptr);

    HdfSbufRecycle(buf);
    buf = nullptr;
}

HWTEST_F(DevAttributeSerializeTest, DevAttributeSerializeTest010, TestSize.Level1)
{
    struct HdfSBuf *buf = HdfSbufTypedObtain(SBUF_IPC);
    ASSERT_NE(buf, nullptr);

    // write deviceId
    bool ret = HdfSbufWriteUint32(buf, 0);
    ASSERT_TRUE(ret);
    // write policy
    ret = HdfSbufWriteUint16(buf, SERVICE_POLICY_INVALID);
    ASSERT_TRUE(ret);
    // write svcName
    ret = HdfSbufWriteString(buf, "test_service");
    ASSERT_TRUE(ret);

    // read moduleName failed
    struct HdfDeviceInfo *attribute = DeviceAttributeDeserialize(buf);
    ASSERT_EQ(attribute, nullptr);

    HdfSbufRecycle(buf);
    buf = nullptr;
}

HWTEST_F(DevAttributeSerializeTest, DevAttributeSerializeTest011, TestSize.Level1)
{
    struct HdfSBuf *buf = HdfSbufTypedObtain(SBUF_IPC);
    ASSERT_NE(buf, nullptr);

    // write deviceId
    bool ret = HdfSbufWriteUint32(buf, 0);
    ASSERT_TRUE(ret);
    // write policy
    ret = HdfSbufWriteUint16(buf, SERVICE_POLICY_INVALID);
    ASSERT_TRUE(ret);
    // write svcName
    ret = HdfSbufWriteString(buf, "test_service");
    ASSERT_TRUE(ret);
    // write moduleName
    ret = HdfSbufWriteString(buf, "test_module");
    ASSERT_TRUE(ret);

    // read deviceName failed
    struct HdfDeviceInfo *attribute = DeviceAttributeDeserialize(buf);
    ASSERT_EQ(attribute, nullptr);

    HdfSbufRecycle(buf);
    buf = nullptr;
}

HWTEST_F(DevAttributeSerializeTest, DevAttributeSerializeTest012, TestSize.Level1)
{
    struct HdfSBuf *buf = HdfSbufTypedObtain(SBUF_IPC);
    ASSERT_NE(buf, nullptr);

    // write deviceId
    bool ret = HdfSbufWriteUint32(buf, 0);
    ASSERT_TRUE(ret);
    // write policy
    ret = HdfSbufWriteUint16(buf, SERVICE_POLICY_INVALID);
    ASSERT_TRUE(ret);
    // write svcName
    ret = HdfSbufWriteString(buf, "test_service");
    ASSERT_TRUE(ret);
    // write moduleName
    ret = HdfSbufWriteString(buf, "test_module");
    ASSERT_TRUE(ret);
    // write deviceName
    ret = HdfSbufWriteString(buf, "test_device");
    ASSERT_TRUE(ret);

    // read length failed
    struct HdfDeviceInfo *attribute = DeviceAttributeDeserialize(buf);
    ASSERT_EQ(attribute, nullptr);

    HdfSbufRecycle(buf);
    buf = nullptr;
}

HWTEST_F(DevAttributeSerializeTest, DevAttributeSerializeTest013, TestSize.Level1)
{
    struct HdfSBuf *buf = HdfSbufTypedObtain(SBUF_IPC);
    ASSERT_NE(buf, nullptr);

    // write deviceId
    bool ret = HdfSbufWriteUint32(buf, 0);
    ASSERT_TRUE(ret);
    // write policy
    ret = HdfSbufWriteUint16(buf, SERVICE_POLICY_INVALID);
    ASSERT_TRUE(ret);
    // write svcName
    ret = HdfSbufWriteString(buf, "test_service");
    ASSERT_TRUE(ret);
    // write moduleName
    ret = HdfSbufWriteString(buf, "test_module");
    ASSERT_TRUE(ret);
    // write deviceName
    ret = HdfSbufWriteString(buf, "test_device");
    ASSERT_TRUE(ret);
    // write length
    ret = HdfSbufWriteUint32(buf, 1);
    ASSERT_TRUE(ret);

    // read deviceMatchAttr failed
    struct HdfDeviceInfo *attribute = DeviceAttributeDeserialize(buf);
    ASSERT_EQ(attribute, nullptr);

    HdfSbufRecycle(buf);
    buf = nullptr;
}

HWTEST_F(DevAttributeSerializeTest, DevAttributeSerializeTest014, TestSize.Level1)
{
    struct HdfSBuf *buf = HdfSbufTypedObtain(SBUF_IPC);
    ASSERT_NE(buf, nullptr);

    // write deviceId
    bool ret = HdfSbufWriteUint32(buf, 0);
    ASSERT_TRUE(ret);
    // write policy
    ret = HdfSbufWriteUint16(buf, SERVICE_POLICY_INVALID);
    ASSERT_TRUE(ret);
    // write svcName
    ret = HdfSbufWriteString(buf, "test_service");
    ASSERT_TRUE(ret);
    // write moduleName
    ret = HdfSbufWriteString(buf, "test_module");
    ASSERT_TRUE(ret);
    // write deviceName
    ret = HdfSbufWriteString(buf, "test_device");
    ASSERT_TRUE(ret);
    // write invalid length
    ret = HdfSbufWriteUint32(buf, 0);
    ASSERT_TRUE(ret);

    // read success
    struct HdfDeviceInfo *attribute = DeviceAttributeDeserialize(buf);
    ASSERT_NE(attribute, nullptr);

    HdfDeviceInfoFreeInstance(attribute);
    HdfSbufRecycle(buf);
    buf = nullptr;
}

HWTEST_F(DevAttributeSerializeTest, DevAttributeSerializeTest015, TestSize.Level1)
{
    DeviceSerializedAttributeRelease(nullptr);
    struct HdfDeviceInfo *attribute = HdfDeviceInfoNewInstance();
    ASSERT_NE(attribute, nullptr);
    DeviceSerializedAttributeRelease(attribute);
}
} // namespace OHOS