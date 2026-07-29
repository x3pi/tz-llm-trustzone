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

/**
 * @addtogroup DriverHdi
 * @{
 *
 * @brief Provides APIs for a system ability to obtain hardware device interface (HDI) services,
 * load or unload a device, and listen for service status, and capabilities for the hdi-gen tool to
 * automatically generate code in interface description language (IDL).
 *
 * The HDF and IDL code generated allow the system ability to accesses HDI driver services.
 *
 * @since 1.0
 */

/**
 * @file hdi_smq_meta.h
 *
 * @brief Defines the metadata struct of the shared memory queue (SMQ) and provides APIs for obtaining
 * the data struct members and serializing and deserializing the metadata struct.
 *
 * @since 1.0
 */

#ifndef HDI_SHARED_MEM_QUEUE_META_H
#define HDI_SHARED_MEM_QUEUE_META_H

#include <atomic>
#include <cstddef>
#include <memory>
#include <message_parcel.h>
#include <unistd.h>
#include "securec.h"

#ifndef HDF_LOG_TAG
#define HDF_LOG_TAG smq
#endif

namespace OHOS {
namespace HDI {
namespace Base {
/**
 * @brief Enumerates the SMQ types.
 */
enum SmqType : uint32_t {
    /** SMQ for synchronous communication */
    SYNCED_SMQ = 0x01,
    /** SMQ for asynchronous communication */
    UNSYNC_SMQ = 0x02,
};

/**
 * @brief Defines the <b>MemZone</b> struct.
 */
struct MemZone {
    /** Memory zone size */
    uint32_t size;
    /** Memory zone offset */
    uint32_t offset;
};

/**
 * @brief Defines the metadata template type of the SMQ.
 */
template <typename T>
class SharedMemQueueMeta {
public:
    /**
     * @brief Default constructor used to create a <b>SharedMemQueueMeta</b> object.
     */
    SharedMemQueueMeta() : SharedMemQueueMeta(-1, 0, 0) {}

    /**
     * @brief A constructor used to create a <b>SharedMemQueueMeta</b> object.
     *
     * @param elementCount Indicates the queue size.
     * @param type Indicates the SMQ type.
     */
    SharedMemQueueMeta(size_t elementCount, SmqType type) : SharedMemQueueMeta(-1, elementCount, type) {}

    /**
     * @brief A constructor used to create a <b>SharedMemQueueMeta</b> object.
     *
     * @param fd Indicates the file descriptor (FD) of the shared memory file.
     * @param elementCount Indicates the queue size.
     * @param type Indicates the SMQ type.
     */
    SharedMemQueueMeta(int fd, size_t elementCount, SmqType type);

    /**
     * @brief A constructor used to copy a <b>SharedMemQueueMeta</b> object.
     *
     * @param other Indicates the <b>SharedMemQueueMeta</b> object to copy.
     */
    SharedMemQueueMeta(const SharedMemQueueMeta<T> &other);
    ~SharedMemQueueMeta();

    /**
     * @brief A constructor used to assign values to the <b>SharedMemQueueMeta</b> object.
     *
     * @param other Indicates the <b>SharedMemQueueMeta</b> object to copy.
     */
    SharedMemQueueMeta &operator=(const SharedMemQueueMeta<T> &other);

    /**
     * @brief Sets an FD.
     *
     * @param fd Indicates the FD to set.
     */
    void SetFd(int fd);

    /**
     * @brief Obtains the FD.
     *
     * @return Returns the FD obtained.
     */
    int GetFd() const;

    /**
     * @brief Obtains the shared memory size.
     *
     * @return Returns the shared memory size obtained.
     */
    size_t GetSize();

    /**
     * @brief Obtains the SMQ type.
     *
     * @return Returns the SMQ type obtained.
     */
    uint32_t GetType();

    /**
     * @brief Obtains the number of elements in the SMQ.
     *
     * @return Returns the number of elements in the SMQ obtained.
     */
    size_t GetElementCount();

    /**
     * @brief Obtains the size of a single element in the SMQ.
     *
     * @return Returns the size of a single element obtained.
     */
    size_t GetElemenetSize() const;

    /**
     * @brief Enumerates the shared memory zone types.
     */
    enum MemZoneType : uint32_t {
        /** Read pointer */
        MEMZONE_RPTR,
        /** Write pointer */
        MEMZONE_WPTR,
        /** Syncword */
        MEMZONE_SYNCER,
        /** Data */
        MEMZONE_DATA,
        /** Number of shared memory zones */
        MEMZONE_COUNT,
    };

    /**
     * @brief Obtains the memory zone of the specified type.
     *
     * @param type Indicates the type of the shared memory zone.
     * @return Returns the shared memory zone obtained.
     */
    MemZone *GetMemZone(uint32_t type);

    /**
     * @brief Writes the SMQ to a <b>MessageParcel</b> object.
     *
     * @param parcel Indicates the <b>MessageParcel</b> object.
     * @return Returns <b>true</b> if the operation is successful; returns <b>false</b> otherwise.
     */
    bool Marshalling(MessageParcel &parcel);

    /**
     * @brief Reads the SMQ from a <b>MessageParcel</b> object.
     *
     * @param parcel Indicates the <b>MessageParcel</b> object from which the SMQ is to read.
     * @return Returns a <b>SharedMemQueueMeta</b> object, which can be used to create an SQM object.
     */
    static std::shared_ptr<SharedMemQueueMeta<T>> UnMarshalling(MessageParcel &parcel);

    /**
     * @brief Aligns the buffer by 8 bytes.
     *
     * @param num Indicates the number of bytes for aligning the buffer.
     * @return Returns the number of bytes after alignment.
     */
    size_t AlignToWord(size_t num)
    {
        constexpr uint32_t alignByteSize = 8;
        return (num + alignByteSize - 1) & (~(alignByteSize - 1));
    }

private:
    /** FD of the shared memory file */
    int ashmemFd_;

    /** Size of the shared memory */
    size_t size_;

    /** Number of elements in the SMQ */
    size_t elementCount_;

    /** Size of an element in the SMQ */
    size_t elementSize_;

    /** SMQ type */
    SmqType type_;

    /** Number of shared memory zones */
    MemZone memzone_[MEMZONE_COUNT];
};

/**
 * @brief A constructor used to assign values to the <b>SharedMemQueueMeta</b> object.
 *
 * @param other Indicates the <b>SharedMemQueueMeta</b> object to copy.
 */
template <typename T>
SharedMemQueueMeta<T> &SharedMemQueueMeta<T>::operator=(const SharedMemQueueMeta<T> &other)
{
    if (this != &other) {
        if (ashmemFd_ >= 0) {
            close(ashmemFd_);
        }
        ashmemFd_ = dup(other.ashmemFd_);
        size_ = other.size_;
        elementCount_ = other.elementCount_;
        elementSize_ = other.elementSize_;
        type_ = other.type_;
        if (memcpy_s(memzone_, sizeof(memzone_), other.memzone_, sizeof(other.memzone_)) != EOK) {
            HDF_LOGW("failed to memcpy_s overload memzone_");
        }
    }

    return *this;
}

template <typename T>
SharedMemQueueMeta<T>::SharedMemQueueMeta(int fd, size_t elementCount, SmqType type)
    : ashmemFd_(fd), size_(0), elementCount_(elementCount), elementSize_(sizeof(T)), type_(type)
{
    // max size UIN32_MAX byte
    if (elementCount_ > UINT32_MAX / elementSize_) {
        return;
    }

    size_t dataSize = elementCount_ * elementSize_;
    size_t memZoneSize[] = {
        sizeof(uint64_t), // read ptr
        sizeof(uint64_t), // write ptr
        sizeof(uint32_t), // sync word
        dataSize,
    };

    size_t offset = 0;
    for (size_t i = 0; i < MEMZONE_COUNT; offset += memZoneSize[i++]) {
        memzone_[i].offset = AlignToWord(offset);
        memzone_[i].size = memZoneSize[i];
    }

    size_ = memzone_[MEMZONE_DATA].offset + memzone_[MEMZONE_DATA].size;
}

template <typename T>
SharedMemQueueMeta<T>::SharedMemQueueMeta(const SharedMemQueueMeta<T> &other)
{
    ashmemFd_ = dup(other.ashmemFd_);
    if (ashmemFd_ < 0) {
        HDF_LOGW("failed to dup ashmem fd for smq");
    }
    elementCount_ = other.elementCount_;
    elementSize_ = other.elementSize_;
    size_ = other.size_;
    type_ = other.type_;
    if (memcpy_s(memzone_, sizeof(memzone_), other.memzone_, sizeof(other.memzone_)) != EOK) {
        HDF_LOGW("failed to memcpy_s memzone_");
    }
}

template <typename T>
SharedMemQueueMeta<T>::~SharedMemQueueMeta()
{
    if (ashmemFd_ >= 0) {
        close(ashmemFd_);
    }
    ashmemFd_ = -1;
}

/**
 * @brief Sets an FD.
 *
 * @param fd Indicates the FD to set.
 */
template <typename T>
void SharedMemQueueMeta<T>::SetFd(int fd)
{
    if (ashmemFd_ >= 0) {
        close(ashmemFd_);
    }
    ashmemFd_ = fd;
}

/**
 * @brief Obtains the FD.
 *
 * @return Returns the FD obtained.
 */
template <typename T>
int SharedMemQueueMeta<T>::GetFd() const
{
    return ashmemFd_;
}

/**
 * @brief Obtains the shared memory size.
 *
 * @return Returns the shared memory size obtained.
 */
template <typename T>
size_t SharedMemQueueMeta<T>::GetSize()
{
    return size_;
}

/**
 * @brief Obtains the SMQ type.
 *
 * @return Returns the SMQ type obtained.
 */
template <typename T>
uint32_t SharedMemQueueMeta<T>::GetType()
{
    return type_;
}

/**
 * @brief Obtains the number of elements in the SMQ.
 *
 * @return Returns the number of elements in the SMQ obtained.
 */
template <typename T>
size_t SharedMemQueueMeta<T>::GetElementCount()
{
    return elementCount_;
}

/**
 * @brief Obtains the size of a single element in the SMQ.
 *
 * @return Returns the size of a single element obtained.
 */
template <typename T>
size_t SharedMemQueueMeta<T>::GetElemenetSize() const
{
    return elementSize_;
}

/**
 * @brief Obtains the memory zone of the specified type.
 *
 * @param type Indicates the type of the shared memory zone.
 * @return Returns the shared memory zone obtained.
 */
template <typename T>
MemZone *SharedMemQueueMeta<T>::GetMemZone(uint32_t type)
{
    if (type >= MEMZONE_COUNT) {
        return nullptr;
    }

    return &memzone_[type];
}

/**
 * @brief Marshals the SMQ into a <b>MessageParcel</b> object.
 *
 * @param parcel Indicates the <b>MessageParcel</b> object after marshalling.
 * @return Returns <b>true</b> if the operation is successful; returns <b>false</b> otherwise.
 */
template <typename T>
bool SharedMemQueueMeta<T>::Marshalling(MessageParcel &parcel)
{
    if (!parcel.WriteFileDescriptor(ashmemFd_)) {
        HDF_LOGE("%{public}s: failed to write ashmemFd_", __func__);
        return false;
    }

    if (!parcel.WriteUint64(static_cast<uint64_t>(elementCount_))) {
        HDF_LOGE("%{public}s: failed to write elementCount_", __func__);
        return false;
    }

    if (!parcel.WriteUint32(static_cast<uint32_t>(type_))) {
        HDF_LOGE("%{public}s: failed to write type_", __func__);
        return false;
    }

    return true;
}

/**
 * @brief Unmarshals the SMQ from a <b>MessageParcel</b> object.
 *
 * @param parcel Indicates the <b>MessageParcel</b> object to unmarshal.
 * @return Returns a <b>SharedMemQueueMeta</b> object, which can be used to create an SQM object.
 */
template <typename T>
std::shared_ptr<SharedMemQueueMeta<T>> SharedMemQueueMeta<T>::UnMarshalling(MessageParcel &parcel)
{
    int fd = parcel.ReadFileDescriptor();
    if (fd < 0) {
        HDF_LOGE("read invalid fd of smq");
        return nullptr;
    }

    uint64_t readElementCount = 0;
    if (!parcel.ReadUint64(readElementCount)) {
        HDF_LOGE("read invalid elementCount of smq");
        close(fd);
        return nullptr;
    }
    size_t elementCount = static_cast<size_t>(readElementCount);

    uint32_t typecode = 0;
    if (!parcel.ReadUint32(typecode)) {
        HDF_LOGE("read invalid typecode of smq");
        close(fd);
        return nullptr;
    }
    SmqType type = static_cast<SmqType>(typecode);
    return std::make_shared<SharedMemQueueMeta<T>>(fd, elementCount, type);
}
} // namespace Base
} // namespace HDI
} // namespace OHOS

#ifdef HDF_LOG_TAG
#undef HDF_LOG_TAG
#endif

#endif /* HDI_SHARED_MEM_QUEUE_META_H */
