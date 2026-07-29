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
 * @addtogroup HDF_IPC_ADAPTER
 * @{
 *
 * @brief Provides capabilities for the user-mode driver to use inter-process communication (IPC).
 *
 * The driver is implemented in C, while IPC is implemented in C++.
 * This module implements IPC APIs in C based on the IPC over C++.
 * It provides APIs for registering a service object, registering a function for processing
 * the death notification of a service, and implementing the dump mechanism.
 *
 * @since 1.0
 */

/**
 * @file hdf_sbuf_ipc.h
 *
 * @brief Provides APIs for converting between the <b>MessageParcel</b> objects (in C++)
 * and <b>HdfSBuf</b> objects (in C).
 *
 * @since 1.0
 */

#ifndef HDF_SBUF_IPC_IMPL_H
#define HDF_SBUF_IPC_IMPL_H

#include <message_parcel.h>
#include "hdf_sbuf.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * @brief Converts a <b>MessageParcel</b> object into an <b>HdfSBuf</b> object.
 *
 * @param parcel Indicates the pointer to the <b>MessageParcel</b> object to convert.
 * @return Returns the <b>HdfSBuf</b> object obtained.
 */
struct HdfSBuf *ParcelToSbuf(OHOS::MessageParcel *parcel);

/**
 * @brief Converts an <b>HdfSBuf</b> object into a <b>MessageParcel</b> object.
 *
 * @param sbuf Indicates the pointer to the <b>HdfSBuf</b> object to convert.
 * @param parcel Indicates the pointer to the <b>MessageParcel</b> object obtained.
 * @return Returns <b>HDF_SUCCESS</b> if the operation is successful; otherwise, the operation fails.
 */
int32_t SbufToParcel(struct HdfSBuf *sbuf, OHOS::MessageParcel **parcel);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif // HDF_SBUF_IPC_IMPL_H
