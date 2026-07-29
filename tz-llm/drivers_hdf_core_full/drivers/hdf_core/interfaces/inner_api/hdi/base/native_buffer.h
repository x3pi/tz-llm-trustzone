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
 * @file native_buffer.h
 *
 * @brief Provides C++ interfaces for <b>NativeBuffer</b>.
 *
 * <b>NativeBuffer</b> is the wrapper of <b>BufferHandle</b>.
 * It manages the <b>BufferHandle</b> objects and applies to the HDI layer.
 *
 * @since 1.0
 */

#ifndef HDI_NATIVE_BUFFER_H
#define HDI_NATIVE_BUFFER_H

#include <message_parcel.h>
#include "buffer_handle.h"

namespace OHOS {
namespace HDI {
namespace Base {
using OHOS::MessageParcel;
using OHOS::Parcelable;

/**
 * @brief Defines the <b>NativeBuffer</b> class.
 *
 */
class NativeBuffer : public Parcelable {
public:
    NativeBuffer();
    virtual ~NativeBuffer();
    explicit NativeBuffer(const BufferHandle *handle);

    NativeBuffer(const NativeBuffer &other);
    NativeBuffer(NativeBuffer &&other) noexcept;

    NativeBuffer &operator=(const NativeBuffer &other);
    NativeBuffer &operator=(NativeBuffer &&other) noexcept;

    /**
     * @brief Marshals this <b>NativeBuffer</b> object into a <b>MessageParcel</b> object.
     * <b>Marshalling()</b> and <b>Unmarshalling()</b> are used in pairs.
     *
     * @param parcel Indicates the <b>MessageParcel</b> object to which the <b>NativeBuffer</b> object is marshalled.
     * @return Returns <b>true</b> if the operation is successful; returns <b>false</b> otherwise.
     */
    bool Marshalling(Parcel &parcel) const override;

    /**
     * @brief Unmarshals a <b>NativeBuffer</b> object from a <b>MessageParcel</b> object.
     * <b>Marshalling()</b> and <b>Unmarshalling()</b> are used in pairs.
     *
     * @param parcel Indicates the <b>MessageParcel</b> object from
     * which the <b>BufferHandle</b> object is unmarshalled.
     * @return Returns the <b>NativeBuffer</b> object obtained if the operation is successful;
     * returns <b>nullptr</b> otherwise.
     */
    static sptr<NativeBuffer> Unmarshalling(Parcel &parcel);

    /**
     * @brief Clones a <b>BufferHandle</b> object.
     *
     * You can use this API to clone the <b>BufferHandle</b> object held by a <b>NativeBuffer</b> object.
     *
     * @return Returns the pointer to the <b>BufferHandle</b> cloned if the operation is successful;
     * returns <b>nullptr</b> otherwise.
     */
    BufferHandle *Clone();

    /**
     * @brief Moves this <b>BufferHandle</b> object.
     *
     * This API transfers the ownership of the <b>BufferHandle</b> object held by the <b>NativeBuffer</b> object.
     * After the ownership is transferred,
     * you need to use <b>FreeNativeBufferHandle</b> to release the <b>BufferHandle</b> object.
     *
     * @return Returns the pointer to the <b>BufferHandle</b> object after the move if the operation is successful;
     * returns <b>nullptr</b> otherwise.
     */
    BufferHandle *Move() noexcept;

    /**
     * @brief Sets a <b>BufferHandle</b> object.
     *
     * You can use this API to set the owner of a <b>BufferHandle</b> object.
     *
     * @param handle Indicates the <b>BufferHandle</b> object to set.
     * @param isOwner Indicates whether the <b>BufferHandle</b> object is managed by a <b>NativeBuffer</b> object.
     * The value <b>true</b> means the <b>BufferHandle</b> object is managed by the<b>NativeBuffer</b> object;
     * the value <b>false</b> means the opposite. The default value is <b>false</b>.
     * @param destructor Indicates the function used to release the <b>BufferHandle</b> object.
     * By default, <b>FreeNativeBufferHandle</b> is used.
     */
    void SetBufferHandle(BufferHandle *handle, bool isOwner = false,
        std::function<void(BufferHandle *)> destructor = nullptr);

    /**
     * @brief Obtains this <b>BufferHandle</b> object.
     *
     * The <b>NativeBuffer</b> object still hold the <b>BufferHandle</b> object.
     *
     * @return Returns the pointer to the <b>BufferHandle</b> object obtained if the operation is successful;
     * returns <b>nullptr</b> otherwise.
     */
    BufferHandle *GetBufferHandle() noexcept;

    /**
     * @brief Prints dump information.
     *
     * @return Returns dump information about this <b>BufferHandle</b> object.
     */
    std::string Dump() const;
private:
    bool ExtractFromParcel(Parcel &parcel);
    static bool WriteReserveData(MessageParcel &messageParcel, const BufferHandle &handle);
    static bool ReadReserveData(MessageParcel &messageParcel, BufferHandle &handle);
    void DestroyBuffer();
    BufferHandle *handle_;
    bool isOwner_;
    std::function<void(BufferHandle *)> bufferDestructor_;
};
} // namespace Base
} // namespace HDI
} // namespace OHOS

#endif // HDI_NATIVE_BUFFER_H
