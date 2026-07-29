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

#include "base/native_buffer.h"
#include <sstream>
#include "base/buffer_util.h"
#include "hdf_log.h"

#define HDF_LOG_TAG native_buffer

namespace OHOS {
namespace HDI {
namespace Base {
NativeBuffer::NativeBuffer() : handle_(nullptr), isOwner_(true), bufferDestructor_(nullptr) {}

NativeBuffer::~NativeBuffer()
{
    DestroyBuffer();
}

NativeBuffer::NativeBuffer(const BufferHandle *handle) : NativeBuffer()
{
    handle_ = CloneNativeBufferHandle(handle);
}

NativeBuffer::NativeBuffer(const NativeBuffer &other) : NativeBuffer()
{
    if (other.handle_ == nullptr) {
        return;
    }
    handle_ = CloneNativeBufferHandle(other.handle_);
}

NativeBuffer::NativeBuffer(NativeBuffer &&other) noexcept : NativeBuffer()
{
    handle_ = other.handle_;
    isOwner_ = other.isOwner_;
    bufferDestructor_ = other.bufferDestructor_;
    other.handle_ = nullptr;
}

NativeBuffer &NativeBuffer::operator=(const NativeBuffer &other)
{
    if (this != &other) {
        DestroyBuffer();
        handle_ = CloneNativeBufferHandle(other.handle_);
        isOwner_ = true;
    }
    return *this;
}

NativeBuffer &NativeBuffer::operator=(NativeBuffer &&other) noexcept
{
    if (this != &other) {
        DestroyBuffer();
        handle_ = other.handle_;
        isOwner_ = other.isOwner_;
        bufferDestructor_ = other.bufferDestructor_;
        other.handle_ = nullptr;
    }
    return *this;
}

bool NativeBuffer::Marshalling(Parcel &parcel) const
{
    MessageParcel &messageParcel = static_cast<MessageParcel &>(parcel);
    bool isValid = handle_ != nullptr ? true : false;
    if (!messageParcel.WriteBool(isValid)) {
        HDF_LOGI("%{public}s: failed to write valid flag of buffer handle", __func__);
        return false;
    }

    if (!isValid) {
        HDF_LOGD("%{public}s: write valid buffer handle", __func__);
        return true;
    }

    if (!messageParcel.WriteUint32(handle_->reserveFds) || !messageParcel.WriteUint32(handle_->reserveInts) ||
        !messageParcel.WriteInt32(handle_->width) || !messageParcel.WriteInt32(handle_->stride) ||
        !messageParcel.WriteInt32(handle_->height) || !messageParcel.WriteInt32(handle_->size) ||
        !messageParcel.WriteInt32(handle_->format) || !messageParcel.WriteUint64(handle_->usage)) {
        HDF_LOGE("%{public}s: a lot failed", __func__);
        return false;
    }

    bool validFd = (handle_->fd >= 0);
    if (!messageParcel.WriteBool(validFd)) {
        HDF_LOGE("%{public}s: failed to write valid flag of fd", __func__);
        return false;
    }
    if (validFd && !messageParcel.WriteFileDescriptor(handle_->fd)) {
        HDF_LOGE("%{public}s: failed to write fd", __func__);
        return false;
    }

    if (!WriteReserveData(messageParcel, *handle_)) {
        return false;
    }

    return true;
}

sptr<NativeBuffer> NativeBuffer::Unmarshalling(Parcel &parcel)
{
    sptr<NativeBuffer> newParcelable = new NativeBuffer();
    if (!newParcelable->ExtractFromParcel(parcel)) {
        return nullptr;
    }
    return newParcelable;
}

BufferHandle *NativeBuffer::Clone()
{
    return CloneNativeBufferHandle(handle_);
}

BufferHandle *NativeBuffer::Move() noexcept
{
    if (isOwner_ == false) {
        HDF_LOGE("%{public}s@%{public}d: isOwner_ is false, Cannot be moved", __func__, __LINE__);
        return nullptr;
    }
    BufferHandle *handlePtr = handle_;
    handle_ = nullptr;
    return handlePtr;
}

void NativeBuffer::SetBufferHandle(BufferHandle *handle, bool isOwner, std::function<void(BufferHandle *)> destructor)
{
    DestroyBuffer();
    isOwner_ = isOwner;
    handle_ = handle;
    bufferDestructor_ = destructor;
}

void NativeBuffer::DestroyBuffer()
{
    if (handle_ != nullptr && isOwner_ == true) {
        if (bufferDestructor_ == nullptr) {
            FreeNativeBufferHandle(handle_);
        } else {
            bufferDestructor_(handle_);
        }
        handle_ = nullptr;
    }
}

BufferHandle *NativeBuffer::GetBufferHandle() noexcept
{
    return handle_;
}

std::string NativeBuffer::Dump() const
{
    std::stringstream os;
    os << "{";
    if (handle_ == nullptr) {
        os << "}\n";
        return os.str();
    }

    os << "fd:" << handle_->fd << ", ";
    os << "width:" << handle_->width << ", ";
    os << "stride:" << handle_->stride << ", ";
    os << "height:" << handle_->height << ", ";
    os << "size:" << handle_->size << ", ";
    os << "format:" << handle_->format << ", ";
    os << "reserveFds:" << handle_->reserveFds << ", ";
    os << "reserveInts:" << handle_->reserveInts << ", ";
    os << "reserve: [";

    if (UINT32_MAX - handle_->reserveFds >= handle_->reserveInts) {
        uint32_t reserveSize = handle_->reserveFds + handle_->reserveInts;
        for (uint32_t i = 0; i < reserveSize; ++i) {
            os << handle_->reserve[i];
            if (i + 1 < reserveSize) {
                os << ", ";
            }
        }
    }
    os << "]";
    os << "}\n";
    return os.str();
}

bool NativeBuffer::ExtractFromParcel(Parcel &parcel)
{
    MessageParcel &messageParcel = static_cast<MessageParcel &>(parcel);
    if (!messageParcel.ReadBool()) {
        HDF_LOGD("%{public}s: read invalid buffer handle", __func__);
        return true;
    }
    uint32_t reserveFds = 0;
    uint32_t reserveInts = 0;
    if (!messageParcel.ReadUint32(reserveFds) || !messageParcel.ReadUint32(reserveInts)) {
        HDF_LOGE("%{public}s: failed to read reserveFds or reserveInts", __func__);
        return false;
    }
    if ((handle_ = AllocateNativeBufferHandle(reserveFds, reserveInts)) == nullptr) {
        HDF_LOGE("%{public}s: failed to malloc BufferHandle", __func__);
        return false;
    }
    bool validFd = false;
    if (!messageParcel.ReadInt32(handle_->width) || !messageParcel.ReadInt32(handle_->stride) ||
        !messageParcel.ReadInt32(handle_->height) || !messageParcel.ReadInt32(handle_->size) ||
        !messageParcel.ReadInt32(handle_->format) || !messageParcel.ReadUint64(handle_->usage) ||
        !messageParcel.ReadBool(validFd)) {
        HDF_LOGE("%{public}s: failed to parcel read", __func__);
        return false;
    }
    if (validFd) {
        handle_->fd = messageParcel.ReadFileDescriptor();
        if (handle_->fd == -1) {
            DestroyBuffer();
            HDF_LOGE("%{public}s: failed to read fd", __func__);
            return false;
        }
    }

    if (!ReadReserveData(messageParcel, *handle_)) {
        DestroyBuffer();
        return false;
    }
    return true;
}

bool NativeBuffer::WriteReserveData(MessageParcel &messageParcel, const BufferHandle &handle)
{
    for (uint32_t i = 0; i < handle.reserveFds; i++) {
        if (!messageParcel.WriteFileDescriptor(handle.reserve[i])) {
            HDF_LOGE("%{public}s: failed to write reserved fd value", __func__);
            return false;
        }
    }
    for (uint32_t j = 0; j < handle.reserveInts; j++) {
        if (!messageParcel.WriteInt32(handle.reserve[handle.reserveFds + j])) {
            HDF_LOGE("%{public}s: failed to write reserved integer value", __func__);
            return false;
        }
    }
    return true;
}

bool NativeBuffer::ReadReserveData(MessageParcel &messageParcel, BufferHandle &handle)
{
    for (uint32_t i = 0; i < handle.reserveFds; i++) {
        handle.reserve[i] = messageParcel.ReadFileDescriptor();
        if (handle.reserve[i] == -1) {
            HDF_LOGE("%{public}s: failed to read reserved fd value", __func__);
            return false;
        }
    }
    for (uint32_t j = 0; j < handle.reserveInts; j++) {
        if (!messageParcel.ReadInt32(handle.reserve[handle.reserveFds + j])) {
            HDF_LOGE("%{public}s: failed to read reserved integer value", __func__);
            return false;
        }
    }
    return true;
}
} // namespace Base
} // namespace HDI
} // namespace OHOS
