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

#include <fcntl.h>
#include <sys/stat.h>
#include <ostream>
#include <gtest/gtest.h>
#include <message_parcel.h>
#include "base/buffer_util.h"
#include "base/native_buffer.h"
#include "osal_mem.h"

using namespace testing::ext;
using OHOS::MessageParcel;
using OHOS::sptr;
using OHOS::HDI::Base::NativeBuffer;

class NativeBufferTest : public testing::Test {
public:
    static void SetUpTestCase() {}
    static void TearDownTestCase() {}

    static BufferHandle *CreateBufferHandle();

    void SetUp() {};
    void TearDown() {};
};

BufferHandle *NativeBufferTest::CreateBufferHandle()
{
    BufferHandle *handle = AllocateNativeBufferHandle(1, 0);
    if (handle == nullptr) {
        std::cout << "failed to allocate buffer handle" << std::endl;
        return nullptr;
    }

    handle->fd = open("/dev/null", O_WRONLY);
    if (handle->fd == -1) {
        std::cout << "failed to open '/dev/null'" << std::endl;
        FreeNativeBufferHandle(handle);
        return nullptr;
    }
    handle->reserve[0] = dup(handle->fd);
    return handle;
}

// test nullptr native buffer
HWTEST_F(NativeBufferTest, NativeBufferTest001, TestSize.Level1)
{
    MessageParcel data;
    sptr<NativeBuffer> srcBuffer = new NativeBuffer(nullptr);
    std::cout << "srcBuffer:\n" << srcBuffer->Dump() << std::endl;
    bool ret = data.WriteStrongParcelable(srcBuffer);
    ASSERT_TRUE(ret);

    sptr<NativeBuffer> destBuffer = data.ReadStrongParcelable<NativeBuffer>();
    ASSERT_NE(destBuffer, nullptr);
    std::cout << "destBuffer:\n" << destBuffer->Dump() << std::endl;

    BufferHandle *destHandle = destBuffer->Move();
    ASSERT_EQ(destHandle, nullptr);
}

// test available native buffer
HWTEST_F(NativeBufferTest, NativeBufferTest002, TestSize.Level1)
{
    BufferHandle *srcHandle = CreateBufferHandle();
    ASSERT_NE(srcHandle, nullptr);

    MessageParcel data;
    sptr<NativeBuffer> srcBuffer = new NativeBuffer(srcHandle);
    std::cout << "srcBuffer:\n" << srcBuffer->Dump() << std::endl;
    bool ret = data.WriteStrongParcelable(srcBuffer);
    ASSERT_TRUE(ret);

    sptr<NativeBuffer> destBuffer = data.ReadStrongParcelable<NativeBuffer>();
    ASSERT_NE(destBuffer, nullptr);
    std::cout << "destBuffer:\n" << destBuffer->Dump() << std::endl;

    BufferHandle *destHandle = destBuffer->Move();
    ASSERT_NE(destHandle, nullptr);

    FreeNativeBufferHandle(srcHandle);
    FreeNativeBufferHandle(destHandle);
}

// test copy construct
HWTEST_F(NativeBufferTest, NativeBufferTest003, TestSize.Level1)
{
    NativeBuffer srcBuffer;
    NativeBuffer destBuffer(srcBuffer);
    std::cout << "srcBuffer:\n" << srcBuffer.Dump() << std::endl;
    std::cout << "destBuffer:\n" << destBuffer.Dump() << std::endl;
}

// test copy construct
HWTEST_F(NativeBufferTest, NativeBufferTest004, TestSize.Level1)
{
    BufferHandle *srcHandle = CreateBufferHandle();
    ASSERT_NE(srcHandle, nullptr);

    // test copy construct
    NativeBuffer srcBuffer(srcHandle);
    ASSERT_NE(srcBuffer.GetBufferHandle(), nullptr);
    std::cout << "srcBuffer:\n" << srcBuffer.Dump() << std::endl;

    NativeBuffer destBuffer(srcBuffer);
    ASSERT_NE(destBuffer.GetBufferHandle(), nullptr);
    std::cout << "destBuffer:\n" << destBuffer.Dump() << std::endl;

    FreeNativeBufferHandle(srcHandle);
}

// test move construct
HWTEST_F(NativeBufferTest, NativeBufferTest005, TestSize.Level1)
{
    BufferHandle *srcHandle = CreateBufferHandle();
    ASSERT_NE(srcHandle, nullptr);

    NativeBuffer srcBuffer(srcHandle);
    ASSERT_NE(srcBuffer.GetBufferHandle(), nullptr);
    std::cout << "srcBuffer:\n" << srcBuffer.Dump() << std::endl;

    NativeBuffer destBuffer(std::move(srcBuffer));
    ASSERT_NE(destBuffer.GetBufferHandle(), nullptr);
    std::cout << "destBuffer:\n" << destBuffer.Dump() << std::endl;

    FreeNativeBufferHandle(srcHandle);
}

// test copy assgin
HWTEST_F(NativeBufferTest, NativeBufferTest006, TestSize.Level1)
{
    BufferHandle *srcHandle = CreateBufferHandle();
    ASSERT_NE(srcHandle, nullptr);

    NativeBuffer srcBuffer(srcHandle);
    ASSERT_NE(srcBuffer.GetBufferHandle(), nullptr);
    std::cout << "srcBuffer:\n" << srcBuffer.Dump() << std::endl;

    NativeBuffer destBuffer(srcHandle);
    ASSERT_NE(destBuffer.GetBufferHandle(), nullptr);
    std::cout << "destBuffer:\n" << destBuffer.Dump() << std::endl;

    // copy assign
    destBuffer = srcBuffer;
    ASSERT_NE(destBuffer.GetBufferHandle(), nullptr);
    std::cout << "destBuffer:\n" << destBuffer.Dump() << std::endl;

    FreeNativeBufferHandle(srcHandle);
}

// test move assgin
HWTEST_F(NativeBufferTest, NativeBufferTest007, TestSize.Level1)
{
    BufferHandle *srcHandle = CreateBufferHandle();
    ASSERT_NE(srcHandle, nullptr);

    NativeBuffer srcBuffer(srcHandle);
    ASSERT_NE(srcBuffer.GetBufferHandle(), nullptr);
    std::cout << "srcBuffer:\n" << srcBuffer.Dump() << std::endl;

    NativeBuffer destBuffer(srcHandle);
    ASSERT_NE(destBuffer.GetBufferHandle(), nullptr);
    std::cout << "destBuffer:\n" << destBuffer.Dump() << std::endl;

    // copy assign
    destBuffer = std::move(srcBuffer);
    ASSERT_NE(destBuffer.GetBufferHandle(), nullptr);
    std::cout << "destBuffer:\n" << destBuffer.Dump() << std::endl;

    FreeNativeBufferHandle(srcHandle);
}

// set destory function
HWTEST_F(NativeBufferTest, NativeBufferTest008, TestSize.Level1)
{
    BufferHandle *srcHandle = CreateBufferHandle();
    ASSERT_NE(srcHandle, nullptr);

    NativeBuffer srcBuffer;
    srcBuffer.SetBufferHandle(srcHandle, true, [](BufferHandle *handle) {
        if (handle == nullptr) {
            return;
        }

        if (handle->fd != -1) {
            close(handle->fd);
            handle->fd = -1;
        }

        for (uint32_t i = 0; i < handle->reserveFds; i++) {
            if (handle->reserve[i] != -1) {
                close(handle->reserve[i]);
                handle->reserve[i] = -1;
            }
        }
        OsalMemFree(handle);
    });
}

// test move
HWTEST_F(NativeBufferTest, NativeBufferTest009, TestSize.Level1)
{
    BufferHandle *srcHandle = CreateBufferHandle();
    ASSERT_NE(srcHandle, nullptr);

    NativeBuffer srcBuffer;
    srcBuffer.SetBufferHandle(srcHandle, false);

    BufferHandle *handle = srcBuffer.Move();
    ASSERT_EQ(handle, nullptr);

    FreeNativeBufferHandle(srcHandle);
}

// test clone
HWTEST_F(NativeBufferTest, NativeBufferTest010, TestSize.Level1)
{
    BufferHandle *srcHandle = CreateBufferHandle();
    ASSERT_NE(srcHandle, nullptr);

    NativeBuffer srcBuffer(srcHandle);
    ASSERT_NE(srcBuffer.GetBufferHandle(), nullptr);

    BufferHandle *destHandle = srcBuffer.Clone();
    ASSERT_NE(destHandle, nullptr);

    FreeNativeBufferHandle(srcHandle);
    FreeNativeBufferHandle(destHandle);
}

// test unmarshalling
HWTEST_F(NativeBufferTest, NativeBufferTest011, TestSize.Level1)
{
    MessageParcel parcel;
    bool ret = parcel.WriteBool(true);
    ASSERT_TRUE(ret);

    sptr<NativeBuffer> destBuffer = NativeBuffer::Unmarshalling(parcel);
    ASSERT_EQ(destBuffer, nullptr);
}

// test unmarshalling
HWTEST_F(NativeBufferTest, NativeBufferTest012, TestSize.Level1)
{
    MessageParcel parcel;
    bool ret = parcel.WriteBool(true);
    ASSERT_TRUE(ret);

    ret = parcel.WriteUint32(MAX_RESERVE_FDS);
    ASSERT_TRUE(ret);

    sptr<NativeBuffer> destBuffer = NativeBuffer::Unmarshalling(parcel);
    ASSERT_EQ(destBuffer, nullptr);
}

// test unmarshalling
HWTEST_F(NativeBufferTest, NativeBufferTest013, TestSize.Level1)
{
    MessageParcel parcel;
    bool ret = parcel.WriteBool(true);
    ASSERT_TRUE(ret);

    ret = parcel.WriteUint32(MAX_RESERVE_FDS + 1);
    ASSERT_TRUE(ret);
    ret = parcel.WriteUint32(MAX_RESERVE_INTS + 1);
    ASSERT_TRUE(ret);

    sptr<NativeBuffer> destBuffer = NativeBuffer::Unmarshalling(parcel);
    ASSERT_EQ(destBuffer, nullptr);
}

// test unmarshalling
HWTEST_F(NativeBufferTest, NativeBufferTest014, TestSize.Level1)
{
    MessageParcel parcel;
    bool ret = parcel.WriteBool(true);
    ASSERT_TRUE(ret);

    ret = parcel.WriteUint32(1);
    ASSERT_TRUE(ret);
    ret = parcel.WriteUint32(0);
    ASSERT_TRUE(ret);

    sptr<NativeBuffer> destBuffer = NativeBuffer::Unmarshalling(parcel);
    ASSERT_EQ(destBuffer, nullptr);
}

// test unmarshalling
HWTEST_F(NativeBufferTest, NativeBufferTest015, TestSize.Level1)
{
    MessageParcel parcel;
    bool ret = parcel.WriteBool(true);
    ASSERT_TRUE(ret);

    ret = parcel.WriteUint32(1);  // reserveFds
    ASSERT_TRUE(ret);
    ret = parcel.WriteUint32(0);  // reserveInts
    ASSERT_TRUE(ret);
    ret = parcel.WriteInt32(0);  // width
    ASSERT_TRUE(ret);
    ret = parcel.WriteInt32(0);  // stride
    ASSERT_TRUE(ret);
    ret = parcel.WriteInt32(0);  // height
    ASSERT_TRUE(ret);
    ret = parcel.WriteInt32(0);  // size
    ASSERT_TRUE(ret);
    ret = parcel.WriteInt32(0);  // format
    ASSERT_TRUE(ret);
    ret = parcel.WriteUint64(0);  // usage
    ASSERT_TRUE(ret);
    ret = parcel.WriteBool(true);  // validFd
    ASSERT_TRUE(ret);

    sptr<NativeBuffer> destBuffer = NativeBuffer::Unmarshalling(parcel);
    ASSERT_EQ(destBuffer, nullptr);
}

// test unmarshalling
HWTEST_F(NativeBufferTest, NativeBufferTest016, TestSize.Level1)
{
    MessageParcel parcel;
    bool ret = parcel.WriteBool(true);
    ASSERT_TRUE(ret);

    ret = parcel.WriteUint32(1);  // reserveFds
    ASSERT_TRUE(ret);
    ret = parcel.WriteUint32(0);  // reserveInts
    ASSERT_TRUE(ret);
    ret = parcel.WriteInt32(0);  // width
    ASSERT_TRUE(ret);
    ret = parcel.WriteInt32(0);  // stride
    ASSERT_TRUE(ret);
    ret = parcel.WriteInt32(0);  // height
    ASSERT_TRUE(ret);
    ret = parcel.WriteInt32(0);  // size
    ASSERT_TRUE(ret);
    ret = parcel.WriteInt32(0);  // format
    ASSERT_TRUE(ret);
    ret = parcel.WriteInt64(0);  // usage
    ASSERT_TRUE(ret);
    ret = parcel.WriteBool(true);  // validFd
    ASSERT_TRUE(ret);
    int fd = open("/dev/null", O_WRONLY);
    ASSERT_NE(fd, -1);
    ret = parcel.WriteFileDescriptor(fd);
    ASSERT_TRUE(ret);

    sptr<NativeBuffer> destBuffer = NativeBuffer::Unmarshalling(parcel);
    ASSERT_EQ(destBuffer, nullptr);
    close(fd);
}