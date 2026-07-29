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
 * @file buffer_util.h
 *
 * @brief Provides the APIs for allocating, releasing, serializing, and deserializing the <b>BufferHandle</b> object.
 *
 * @since 1.0
 */

#ifndef HDI_BUFFER_UTIL_H
#define HDI_BUFFER_UTIL_H

#include "buffer_handle.h"
#include "hdf_sbuf.h"

/**
 * @brief Defines the maximum value of the <b>reserveFds</b> in <b>BufferHandle</b>.
 */
#define MAX_RESERVE_FDS 1024

/**
 * @brief Defines the maximum value of the <b>reserveInts</b> in <b>BufferHandle</b>.
 */
#define MAX_RESERVE_INTS 1024

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * @brief Allocates a <b>BufferHandle</b> object.
 *
 * You can use this API to create the default <b>BufferHandle</b> object
 * based on the specified <b>reserveFds</b> and <b>reserveInts</b>.
 *
 * @param reserveFds Indicates the file descriptor (FD) count (<b>0</b> to <b>MAX_RESERVE_FDS</b>) in <b>reserve[0]</b>.
 * @param reserveInts Indicates the integer count (<b>0</b> to <b>MAX_RESERVE_FDS</b>) in <b>reserve[reserveFds]</b>.
 * @return Returns the pointer to the <b>BufferHandle</b> allocated if the operation is successful;
 * returns <b>NULL</b> otherwise.
 */
BufferHandle *AllocateNativeBufferHandle(uint32_t reserveFds, uint32_t reserveInts);

/**
 * @brief Clones a <b>BufferHandle</b> object.
 *
 * You can use this API to create a <b>BufferHandle</b> object from a given <b>BufferHandle</b> object.
 *
 * @param other Indicates the pointer to the <b>BufferHandle</b> object to clone.
 * @return Returns the pointer to the <b>BufferHandle</b> created if the operation is successful;
 * returns <b>NULL</b> otherwise.
 */
BufferHandle *CloneNativeBufferHandle(const BufferHandle *other);

/**
 * @brief Releases a <b>BufferHandle</b> object.
 *
 * @param handle Indicates the pointer to the <b>BufferHandle</b> object to release.
 */
void FreeNativeBufferHandle(BufferHandle *handle);

/**
 * @brief Serializes a <b>BufferHandle</b> object.
 *
 * You can use this API to write a <b>BufferHandle</b> object to a <b>HdfSBuf</b> object.
 * <b>HdfSbufWriteNativeBufferHandle</b> and <b>HdfSbufReadNativeBufferHandle</b> are used in pairs.
 *
 * @param data Indicates the pointer to the <b>HdfSBuf</b> object.
 * @param handle Indicates the pointer to the <b>BufferHandle</b> object to serialize.
 */
bool HdfSbufWriteNativeBufferHandle(struct HdfSBuf *data, const BufferHandle *handle);

/**
 * @brief Deserializes a <b>BufferHandle</b> object.
 *
 * You can use this API to read a <b>BufferHandle</b> object from a <b>HdfSBuf</b> object
 * according to the serialization format and create a new <b>BufferHandle</b> object.
 * <b>HdfSbufWriteNativeBufferHandle</b> and <b>HdfSbufReadNativeBufferHandle</b> are used in pairs.
 *
 * @param data Indicates the pointer to the <b>HdfSBuf</b> object.
 * @return Returns the pointer to the <b>BufferHandle</b> obtained if the operation is successful;
 * returns <b>NULL</b> otherwise.
 */
BufferHandle *HdfSbufReadNativeBufferHandle(struct HdfSBuf *data);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* HDI_BUFFER_UTIL_H */