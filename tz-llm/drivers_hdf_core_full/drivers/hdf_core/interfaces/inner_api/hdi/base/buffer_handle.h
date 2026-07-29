/*
 * Copyright (c) 2021-2023 Huawei Device Co., Ltd.
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
 * load or unload a device, and listen for service status, and capabilities for the hdi-gen tool
 * to automatically generate code in interface description language (IDL).
 *
 * The HDF and IDL code generated allow the system ability to accesses HDI driver services.
 *
 * @since 1.0
 */

/**
 * @file buffer_handle.h
 *
 * @brief Defines the common data struct for graphics processing.
 * The <b>BufferHandle</b> must comply with the IDL syntax.
 *
 * You only need to define the struct in IDL for the service module.
 * The HDI module implements serialization and deserialization of this struct.
 *
 * @since 1.0
 */

#ifndef INCLUDE_BUFFER_HANDLE_H
#define INCLUDE_BUFFER_HANDLE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Defines the <b>BufferHandle</b> struct.
 */
typedef struct {
    int32_t fd;           /**< Buffer file descriptor (FD). The value <b>-1</b> indicates a invalid FD. */
    int32_t width;        /**< Width of the image */
    int32_t stride;       /**< Stride of the image */
    int32_t height;       /**< Height of the image */
    int32_t size;         /* < Size of the buffer */
    int32_t format;       /**< Format of the image */
    uint64_t usage;       /**< Usage of the buffer */
    void *virAddr;        /**< Virtual address of the buffer */
    uint64_t phyAddr;     /**< Physical address */
    uint32_t reserveFds;  /**< Number of the reserved FDs */
    uint32_t reserveInts; /**< Number of the reserved integers */
    int32_t reserve[0];   /**< Data */
} BufferHandle;

#ifdef __cplusplus
}
#endif

#endif // INCLUDE_BUFFER_HANDLE_H
