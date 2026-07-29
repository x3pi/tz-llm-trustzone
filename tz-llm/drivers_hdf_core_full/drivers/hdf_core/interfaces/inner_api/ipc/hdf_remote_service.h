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
 * @file hdf_remote_service.h
 *
 * @brief Provides IPC interface capabilities in C.
 *
 * @since 1.0
 */
#ifndef HDF_REMOTE_SERVICE_H
#define HDF_REMOTE_SERVICE_H

#include <unistd.h>
#include "hdf_object.h"
#include "hdf_sbuf.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * @brief Defines the <b>HdfRemoteDispatcher</b> struct.
 */
struct HdfRemoteDispatcher {
    /** Interface for synchronous message dispatching */
    int (*Dispatch)(struct HdfRemoteService *, int, struct HdfSBuf *, struct HdfSBuf *);
    /** Interface for asynchronous message dispatching */
    int (*DispatchAsync)(struct HdfRemoteService *, int, struct HdfSBuf *, struct HdfSBuf *);
};

/**
 * @brief Defines the <b>HdfDeathRecipient</b> struct.
 */
struct HdfDeathRecipient {
    /** Interface for listening for the death notification of a service. */
    void (*OnRemoteDied)(struct HdfDeathRecipient *, struct HdfRemoteService *);
};

/**
 * @brief Defines the <b>HdfRemoteService</b> struct.
 */
struct HdfRemoteService {
    /** Remote service object */
    struct HdfObject object;
    /** Target object to which the remote service object points */
    struct HdfObject *target;
    /** Message dispatching interface of the remote service object */
    struct HdfRemoteDispatcher *dispatcher;
    /** Address of the IPC object */
    uint64_t index;
};

/**
 * @brief Obtains a remote service.
 *
 * @param object Indicates the pointer to the service.
 * @param dispatcher Indicates the pointer to the message dispatching interface of the service.
 * @return Returns the remote service obtained if the operation is successful; returns a null pointer otherwise.
 */
struct HdfRemoteService *HdfRemoteServiceObtain(
    struct HdfObject *object, struct HdfRemoteDispatcher *dispatcher);

/**
 * @brief Recycles a remote service.
 *
 * @param object Indicates the pointer to the remote service to recycle.
 */
void HdfRemoteServiceRecycle(struct HdfRemoteService *service);

/**
 * @brief Adds a function for processing the the death notification of a service.
 *
 * @param service Indicates the pointer to the service to be listened for.
 * @param recipient Indicates the pointer to the callback object containing the code to be executed
 * when the death notification is received.
 */
void HdfRemoteServiceAddDeathRecipient(struct HdfRemoteService *service, struct HdfDeathRecipient *recipient);

/**
 * @brief Removes the function for processing the the death notification of a service.
 *
 * @param service Indicates the pointer to the target service.
 * @param recipient Indicates the pointer to the callback object to remove.
 */
void HdfRemoteServiceRemoveDeathRecipient(struct HdfRemoteService *service, struct HdfDeathRecipient *recipient);

/**
 * @brief Registers a service.
 *
 * @param serviceId Indicates the ID of the service to add.
 * @param service Indicates the pointer to the service.
 * @return Returns <b>HDF_SUCCESS</b> if the operation is successful; otherwise, the operation fails.
 */
int HdfRemoteServiceRegister(int32_t serviceId, struct HdfRemoteService *service);

/**
 * @brief Obtains a service.
 *
 * @param serviceId Indicates the ID of the service to obtain.
 * @return Returns the service object obtained if the operation is successful; returns a null pointer otherwise.
 */
struct HdfRemoteService *HdfRemoteServiceGet(int32_t serviceId);

/**
 * @brief Sets an interface descriptor for a service.
 *
 * @param service Indicates the pointer to the target service.
 * @param descriptor Indicates the interface descriptor to set.
 * @return Returns <b>true</b> if the operation is successful; returns <b>false</b> otherwise.
 */
bool HdfRemoteServiceSetInterfaceDesc(struct HdfRemoteService *service, const char *descriptor);

/**
 * @brief Sets an interface token for an <b>HdfSBuf</b> with the interface descriptor of a service.
 *
 * @param service Indicates the pointer to the service.
 * @param data Indicates the pointer to the target<b>HdfSBuf</b> object to be set with an interface token.
 * @return Returns <b>true</b> if the operation is successful; returns <b>false</b> otherwise.
 */
bool HdfRemoteServiceWriteInterfaceToken(struct HdfRemoteService *service, struct HdfSBuf *data);

/**
 * @brief Checks the interface tokens of a service and an <b>HdfSBuf</b> for consistency.
 *
 * @param service Indicates the pointer to the service.
 * @param data Indicates the pointer to the <b>HdfSBuf</b>.
 * @return Returns <b>true</b> if the two interface tokens are the same; returns <b>true</b> otherwise.
 */
bool HdfRemoteServiceCheckInterfaceToken(struct HdfRemoteService *service, struct HdfSBuf *data);

/**
 * @brief Get the caller's process ID.
 *
 * @return The caller's process ID.
 */
pid_t HdfRemoteGetCallingPid(void);

/**
 * @brief Get the caller's user ID.
 *
 * @return The caller's user ID.
 */
pid_t HdfRemoteGetCallingUid(void);

/**
 * @brief Default command distribution for invoking ipc.
 *
 * @param service Indicates the pointer to the service.
 * @param code Indicates the command word to be distributed.
 * @param data Indicates the pointer to the <b>HdfSBuf</b>.
 * @param reply Indicates the pointer to the <b>HdfSBuf</b>.
 * @return Returns <b>HDF_SUCCESS</b> if the operation is successful; otherwise, the operation fails.
 */
int HdfRemoteServiceDefaultDispatch(
    struct HdfRemoteService *service, int code, struct HdfSBuf *data, struct HdfSBuf *reply);

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif /* HDF_REMOTE_SERVICE_H */
