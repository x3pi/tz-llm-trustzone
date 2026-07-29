/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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
 * It provides APIs for registering a service object, registering a function for processing the death
 * notification of a service object, and implementing the dump mechanism.
 *
 * @since 1.0
 */

/**
 * @file hdf_dump_reg.h
 *
 * @brief Provides the dump feature in C based on the IPC dump over C++.
 *
 * @since 1.0
 */

#ifndef HDF_DUMP_REG_H
#define HDF_DUMP_REG_H

#include "hdf_sbuf.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * @brief Implements IPC dump.
 *
 * @param data Indicates the pointer to the input parameter, which is obtained from the command line.
 * @param reply Indicates the pointer to the output parameter,
 * which is returned by the service module to the command line for display.
 * @return Returns <b>HDF_SUCCESS</b> if the operation is successful; otherwise, the operation fails.
 */
typedef int32_t (*DevHostDumpFunc)(struct HdfSBuf *data, struct HdfSBuf *reply);

/**
 * @brief Registers the dump function.
 *
 * @param dump Indicates the dump function to register.
 */
void HdfRegisterDumpFunc(DevHostDumpFunc dump);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif // HDF_DUMP_REG_H
