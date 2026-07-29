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

#include <ostream>
#include <sstream>
#include <string>

#include <fcntl.h>
#include <sys/stat.h>

#include <gtest/gtest.h>

#include "base/buffer_util.h"

using namespace testing::ext;

namespace OHOS {
class BufferHandleTest : public testing::Test {
public:
    static void SetUpTestCase() {}
    static void TearDownTestCase() {}

    static BufferHandle *CreateBufferHandle(uint32_t reserveFds = 1, uint32_t reserveInts = 0);
    static std::string BufferHandleDump(const BufferHandle *handle);

    void SetUp() {};
    void TearDown() {};
};

BufferHandle *BufferHandleTest::CreateBufferHandle(uint32_t reserveFds, uint32_t reserveInts)
{
    BufferHandle *handle = AllocateNativeBufferHandle(reserveFds, reserveInts);
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

    for (uint32_t i = 0; i < reserveFds; i++) {
        handle->reserve[i] = dup(handle->fd);
    }

    for (uint32_t i = reserveFds; i < reserveInts; i++) {
        handle->reserve[i] = 0;
    }
    return handle;
}

std::string BufferHandleTest::BufferHandleDump(const BufferHandle *handle)
{
    std::stringstream os;
    os << "{";
    if (handle == nullptr) {
        os << "}\n";
        return os.str();
    }

    os << "fd:" << handle->fd << ", ";
    os << "width:" << handle->width << ", ";
    os << "stride:" << handle->stride << ", ";
    os << "height:" << handle->height << ", ";
    os << "size:" << handle->size << ", ";
    os << "format:" << handle->format << ", ";
    os << "reserveFds:" << handle->reserveFds << ", ";
    os << "reserveInts:" << handle->reserveInts << ", ";
    os << "reserve: [";
    if (UINT32_MAX - handle->reserveFds >= handle->reserveInts) {
        uint32_t reserveSize = handle->reserveFds + handle->reserveInts;
        for (uint32_t i = 0; i < reserveSize; ++i) {
            os << handle->reserve[i];
            if (i + 1 < reserveSize) {
                os << ", ";
            }
        }
    }
    os << "]";
    os << "}\n";
    return os.str();
}

// allocate illegal buffer
HWTEST_F(BufferHandleTest, BufferHandleTest001, TestSize.Level1)
{
    BufferHandle *handle = AllocateNativeBufferHandle(UINT32_MAX, UINT32_MAX);
    ASSERT_EQ(handle, nullptr);
    std::cout << "handle:\n" << BufferHandleDump(handle) << std::endl;
    FreeNativeBufferHandle(handle);
}

// allocate zero size buffer
HWTEST_F(BufferHandleTest, BufferHandleTest002, TestSize.Level1)
{
    BufferHandle *handle = AllocateNativeBufferHandle(0, 0);
    ASSERT_NE(handle, nullptr);
    std::cout << "handle:\n" << BufferHandleDump(handle) << std::endl;
    FreeNativeBufferHandle(handle);
}

// test CloneNativeBufferHandle method
HWTEST_F(BufferHandleTest, BufferHandleTest003, TestSize.Level1)
{
    BufferHandle *srcHandle = CreateBufferHandle();
    ASSERT_NE(srcHandle, nullptr);
    std::cout << "srcHandle:\n" << BufferHandleDump(srcHandle) << std::endl;

    BufferHandle *destHandle = CloneNativeBufferHandle(srcHandle);
    ASSERT_NE(destHandle, nullptr);
    std::cout << "destHandle:\n" << BufferHandleDump(destHandle) << std::endl;

    FreeNativeBufferHandle(srcHandle);
    FreeNativeBufferHandle(destHandle);
}

// test available native buffer
HWTEST_F(BufferHandleTest, BufferHandleTest004, TestSize.Level1)
{
    struct HdfSBuf *data = HdfSbufTypedObtain(SBUF_IPC);
    ASSERT_NE(data, nullptr);

    BufferHandle *srcHandle = CreateBufferHandle();
    ASSERT_NE(srcHandle, nullptr);
    std::cout << "srcHandle:\n" << BufferHandleDump(srcHandle) << std::endl;

    bool ret = HdfSbufWriteNativeBufferHandle(data, srcHandle);
    ASSERT_TRUE(ret);

    BufferHandle *destHandle = HdfSbufReadNativeBufferHandle(data);
    ASSERT_NE(destHandle, nullptr);
    std::cout << "destHandle:\n" << BufferHandleDump(destHandle) << std::endl;

    FreeNativeBufferHandle(srcHandle);
    FreeNativeBufferHandle(destHandle);
    HdfSbufRecycle(data);
}

// test write buffer handle with null buf
HWTEST_F(BufferHandleTest, BufferHandleTest005, TestSize.Level1)
{
    struct HdfSBuf *data = nullptr;
    BufferHandle *srcHandle = nullptr;
    bool ret = HdfSbufWriteNativeBufferHandle(data, srcHandle);
    ASSERT_FALSE(ret);
}

// test write buffer handle with null buffer handle
HWTEST_F(BufferHandleTest, BufferHandleTest006, TestSize.Level1)
{
    struct HdfSBuf *data = HdfSbufTypedObtain(SBUF_IPC);
    ASSERT_NE(data, nullptr);
    BufferHandle *srcHandle = nullptr;

    bool ret = HdfSbufWriteNativeBufferHandle(data, srcHandle);
    ASSERT_FALSE(ret);

    HdfSbufRecycle(data);
}

// test read buffer handle with null buf
HWTEST_F(BufferHandleTest, BufferHandleTest007, TestSize.Level1)
{
    struct HdfSBuf *data = nullptr;
    BufferHandle *srcHandle = HdfSbufReadNativeBufferHandle(data);
    ASSERT_EQ(srcHandle, nullptr);
}

// test read buffer handle with empty buf
HWTEST_F(BufferHandleTest, BufferHandleTest008, TestSize.Level1)
{
    struct HdfSBuf *data = HdfSbufTypedObtain(SBUF_IPC);
    ASSERT_NE(data, nullptr);
    BufferHandle *srcHandle = HdfSbufReadNativeBufferHandle(data);
    ASSERT_EQ(srcHandle, nullptr);

    HdfSbufRecycle(data);
}

HWTEST_F(BufferHandleTest, BufferHandleTest009, TestSize.Level1)
{
    struct HdfSBuf *data = HdfSbufTypedObtain(SBUF_IPC);
    ASSERT_NE(data, nullptr);

    bool ret = HdfSbufWriteUint32(data, MAX_RESERVE_FDS);
    ASSERT_TRUE(ret);

    BufferHandle *srcHandle = HdfSbufReadNativeBufferHandle(data);
    ASSERT_EQ(srcHandle, nullptr);

    HdfSbufRecycle(data);
}

HWTEST_F(BufferHandleTest, BufferHandleTest010, TestSize.Level1)
{
    struct HdfSBuf *data = HdfSbufTypedObtain(SBUF_IPC);
    ASSERT_NE(data, nullptr);

    bool ret = HdfSbufWriteUint32(data, MAX_RESERVE_FDS + 1);
    ASSERT_TRUE(ret);
    ret = HdfSbufWriteUint32(data, MAX_RESERVE_INTS + 1);
    ASSERT_TRUE(ret);

    BufferHandle *srcHandle = HdfSbufReadNativeBufferHandle(data);
    ASSERT_EQ(srcHandle, nullptr);
    HdfSbufRecycle(data);
}

HWTEST_F(BufferHandleTest, BufferHandleTest011, TestSize.Level1)
{
    struct HdfSBuf *data = HdfSbufTypedObtain(SBUF_IPC);
    ASSERT_NE(data, nullptr);

    bool ret = HdfSbufWriteUint32(data, 1);
    ASSERT_TRUE(ret);
    ret = HdfSbufWriteUint32(data, 0);
    ASSERT_TRUE(ret);

    BufferHandle *srcHandle = HdfSbufReadNativeBufferHandle(data);
    ASSERT_EQ(srcHandle, nullptr);
    HdfSbufRecycle(data);
}

HWTEST_F(BufferHandleTest, BufferHandleTest012, TestSize.Level1)
{
    struct HdfSBuf *data = HdfSbufTypedObtain(SBUF_IPC);
    ASSERT_NE(data, nullptr);

    // write reserveFds
    bool ret = HdfSbufWriteUint32(data, 1);
    ASSERT_TRUE(ret);
    // write reserveInts
    ret = HdfSbufWriteUint32(data, 0);
    ASSERT_TRUE(ret);
    // write width
    ret = HdfSbufWriteInt32(data, 0);
    ASSERT_TRUE(ret);
    // write stride
    ret = HdfSbufWriteInt32(data, 0);
    ASSERT_TRUE(ret);
    // write height
    ret = HdfSbufWriteInt32(data, 0);
    ASSERT_TRUE(ret);
    // write size
    ret = HdfSbufWriteInt32(data, 0);
    ASSERT_TRUE(ret);
    // write format
    ret = HdfSbufWriteInt32(data, 0);
    ASSERT_TRUE(ret);
    // write usage
    ret = HdfSbufWriteUint64(data, 0);
    ASSERT_TRUE(ret);

    BufferHandle *srcHandle = HdfSbufReadNativeBufferHandle(data);
    ASSERT_EQ(srcHandle, nullptr);
    HdfSbufRecycle(data);
}

HWTEST_F(BufferHandleTest, BufferHandleTest013, TestSize.Level1)
{
    struct HdfSBuf *data = HdfSbufTypedObtain(SBUF_IPC);
    ASSERT_NE(data, nullptr);

    // write reserveFds
    bool ret = HdfSbufWriteUint32(data, 1);
    ASSERT_TRUE(ret);
    // write reserveInts
    ret = HdfSbufWriteUint32(data, 0);
    ASSERT_TRUE(ret);
    // write width
    ret = HdfSbufWriteInt32(data, 0);
    ASSERT_TRUE(ret);
    // write stride
    ret = HdfSbufWriteInt32(data, 0);
    ASSERT_TRUE(ret);
    // write height
    ret = HdfSbufWriteInt32(data, 0);
    ASSERT_TRUE(ret);
    // write size
    ret = HdfSbufWriteInt32(data, 0);
    ASSERT_TRUE(ret);
    // write format
    ret = HdfSbufWriteInt32(data, 0);
    ASSERT_TRUE(ret);
    // write usage
    ret = HdfSbufWriteUint64(data, 0);
    ASSERT_TRUE(ret);
    ret = HdfSbufWriteInt8(data, 0);
    ASSERT_TRUE(ret);

    BufferHandle *srcHandle = HdfSbufReadNativeBufferHandle(data);
    ASSERT_EQ(srcHandle, nullptr);
    HdfSbufRecycle(data);
}

HWTEST_F(BufferHandleTest, BufferHandleTest014, TestSize.Level1)
{
    struct HdfSBuf *data = HdfSbufTypedObtain(SBUF_IPC);
    ASSERT_NE(data, nullptr);

    // write reserveFds
    bool ret = HdfSbufWriteUint32(data, 1);
    ASSERT_TRUE(ret);
    // write reserveInts
    ret = HdfSbufWriteUint32(data, 0);
    ASSERT_TRUE(ret);
    // write width
    ret = HdfSbufWriteInt32(data, 0);
    ASSERT_TRUE(ret);
    // write stride
    ret = HdfSbufWriteInt32(data, 0);
    ASSERT_TRUE(ret);
    // write height
    ret = HdfSbufWriteInt32(data, 0);
    ASSERT_TRUE(ret);
    // write size
    ret = HdfSbufWriteInt32(data, 0);
    ASSERT_TRUE(ret);
    // write format
    ret = HdfSbufWriteInt32(data, 0);
    ASSERT_TRUE(ret);
    // write usage
    ret = HdfSbufWriteUint64(data, 0);
    ASSERT_TRUE(ret);
    ret = HdfSbufWriteInt8(data, 1);
    ASSERT_TRUE(ret);

    BufferHandle *srcHandle = HdfSbufReadNativeBufferHandle(data);
    ASSERT_EQ(srcHandle, nullptr);
    HdfSbufRecycle(data);
}

HWTEST_F(BufferHandleTest, BufferHandleTest015, TestSize.Level1)
{
    struct HdfSBuf *data = HdfSbufTypedObtain(SBUF_IPC);
    ASSERT_NE(data, nullptr);

    // write reserveFds
    bool ret = HdfSbufWriteUint32(data, 1);
    ASSERT_TRUE(ret);
    // write reserveInts
    ret = HdfSbufWriteUint32(data, 0);
    ASSERT_TRUE(ret);
    // write width
    ret = HdfSbufWriteInt32(data, 0);
    ASSERT_TRUE(ret);
    // write stride
    ret = HdfSbufWriteInt32(data, 0);
    ASSERT_TRUE(ret);
    // write height
    ret = HdfSbufWriteInt32(data, 0);
    ASSERT_TRUE(ret);
    // write size
    ret = HdfSbufWriteInt32(data, 0);
    ASSERT_TRUE(ret);
    // write format
    ret = HdfSbufWriteInt32(data, 0);
    ASSERT_TRUE(ret);
    // write usage
    ret = HdfSbufWriteUint64(data, 0);
    ASSERT_TRUE(ret);
    // validFd
    ret = HdfSbufWriteInt8(data, 1);
    ASSERT_TRUE(ret);
    // fd
    int fd = open("/dev/null", O_WRONLY);
    ASSERT_NE(fd, -1);
    ret = HdfSbufWriteFileDescriptor(data, fd);
    ASSERT_TRUE(ret);

    BufferHandle *srcHandle = HdfSbufReadNativeBufferHandle(data);
    ASSERT_EQ(srcHandle, nullptr);
    HdfSbufRecycle(data);
    close(fd);
}

HWTEST_F(BufferHandleTest, BufferHandleTest016, TestSize.Level1)
{
    struct HdfSBuf *data = HdfSbufTypedObtain(SBUF_IPC);
    ASSERT_NE(data, nullptr);

    // write reserveFds
    bool ret = HdfSbufWriteUint32(data, 1);
    ASSERT_TRUE(ret);
    // write reserveInts
    ret = HdfSbufWriteUint32(data, 1);
    ASSERT_TRUE(ret);
    // write width
    ret = HdfSbufWriteInt32(data, 0);
    ASSERT_TRUE(ret);
    // write stride
    ret = HdfSbufWriteInt32(data, 0);
    ASSERT_TRUE(ret);
    // write height
    ret = HdfSbufWriteInt32(data, 0);
    ASSERT_TRUE(ret);
    // write size
    ret = HdfSbufWriteInt32(data, 0);
    ASSERT_TRUE(ret);
    // write format
    ret = HdfSbufWriteInt32(data, 0);
    ASSERT_TRUE(ret);
    // write usage
    ret = HdfSbufWriteUint64(data, 0);
    ASSERT_TRUE(ret);
    // validFd
    ret = HdfSbufWriteInt8(data, 1);
    ASSERT_TRUE(ret);
    // fd
    int fd = open("/dev/null", O_WRONLY);
    ASSERT_NE(fd, -1);
    ret = HdfSbufWriteFileDescriptor(data, fd);
    ASSERT_TRUE(ret);
    int fd2 = dup(fd);
    ASSERT_NE(fd2, -1);
    ret = HdfSbufWriteFileDescriptor(data, fd2);
    ASSERT_TRUE(ret);

    BufferHandle *srcHandle = HdfSbufReadNativeBufferHandle(data);
    ASSERT_EQ(srcHandle, nullptr);
    HdfSbufRecycle(data);
    close(fd);
}

HWTEST_F(BufferHandleTest, BufferHandleTest017, TestSize.Level1)
{
    BufferHandle *srcHandle = CreateBufferHandle();
    ASSERT_NE(srcHandle, nullptr);
    std::cout << "srcHandle:\n" << BufferHandleDump(srcHandle) << std::endl;

    uint32_t realReserveFds = srcHandle->reserveFds;
    uint32_t realReserveInts = srcHandle->reserveInts;

    srcHandle->reserveFds = MAX_RESERVE_FDS + 1;
    srcHandle->reserveInts = MAX_RESERVE_INTS + 1;

    BufferHandle *destHandle = CloneNativeBufferHandle(srcHandle);
    ASSERT_EQ(destHandle, nullptr);

    srcHandle->reserveFds = realReserveFds;
    srcHandle->reserveInts = realReserveInts;
    FreeNativeBufferHandle(srcHandle);
}

HWTEST_F(BufferHandleTest, BufferHandleTest018, TestSize.Level1)
{
    BufferHandle *srcHandle = CreateBufferHandle();
    ASSERT_NE(srcHandle, nullptr);
    std::cout << "srcHandle:\n" << BufferHandleDump(srcHandle) << std::endl;

    close(srcHandle->fd);
    srcHandle->fd = -1;

    for (uint32_t i = 0; i < srcHandle->reserveFds; i++) {
        if (srcHandle->reserve[i] != -1) {
            close(srcHandle->reserve[i]);
            srcHandle->reserve[i] = -1;
        }
    }

    BufferHandle *destHandle = CloneNativeBufferHandle(srcHandle);
    ASSERT_EQ(destHandle, nullptr);
}

HWTEST_F(BufferHandleTest, BufferHandleTest019, TestSize.Level1)
{
    BufferHandle *srcHandle = CreateBufferHandle(1, 1);
    ASSERT_NE(srcHandle, nullptr);
    std::cout << "srcHandle:\n" << BufferHandleDump(srcHandle) << std::endl;

    BufferHandle *destHandle = CloneNativeBufferHandle(srcHandle);
    ASSERT_NE(destHandle, nullptr);
    std::cout << "destHandle:\n" << BufferHandleDump(destHandle) << std::endl;

    FreeNativeBufferHandle(srcHandle);
    FreeNativeBufferHandle(destHandle);
}
} // namespace OHOS