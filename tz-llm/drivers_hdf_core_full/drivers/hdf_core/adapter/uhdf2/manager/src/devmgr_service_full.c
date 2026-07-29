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

#include "devmgr_service_full.h"
#include "devhost_service_clnt.h"
#include "devhost_service_proxy.h"
#include "device_token_clnt.h"
#include "hdf_driver_installer.h"
#include "hdf_log.h"
#include "hdf_message_looper.h"
#include "osal_message.h"

#define HDF_LOG_TAG devmgr_service_full
#define INVALID_PID (-1)

static void CleanupDiedHostResources(struct DevHostServiceClnt *hostClnt, struct HdfRemoteService *service)
{
    OsalMutexLock(&hostClnt->hostLock);
    struct DevHostServiceProxy *hostProxy = (struct DevHostServiceProxy *)hostClnt->hostService;
    if (hostProxy != NULL) {
        if ((hostProxy->remote != NULL) && ((uintptr_t)hostProxy->remote == (uintptr_t)service)) {
            HDF_LOGI("%{public}s hostId: %{public}u remove current hostService", __func__, hostClnt->hostId);
            hostClnt->hostPid = INVALID_PID;
            DevHostServiceProxyRecycle(hostProxy);
            hostClnt->hostService = NULL;
            HdfSListFlush(&hostClnt->devices, DeviceTokenClntDelete);
        } else {
            hostProxy = (struct DevHostServiceProxy *)service->target;
            HDF_LOGI("%{public}s hostId: %{public}u remove old hostService", __func__, hostClnt->hostId);
            DevHostServiceProxyRecycle(hostProxy);
        }
    } else {
        hostProxy = (struct DevHostServiceProxy *)service->target;
        HDF_LOGI("%{public}s hostId: %{public}u remove old hostService, and current hostService is null",
            __func__, hostClnt->hostId);
        DevHostServiceProxyRecycle(hostProxy);
    }

    OsalMutexUnlock(&hostClnt->hostLock);
}

static int32_t DevmgrServiceFullHandleDeviceHostDied(struct DevHostServiceClnt *hostClnt,
    struct HdfRemoteService *service)
{
    // the host will be restart by init module if there are default loaded devices in it.
    // the on-demand loaded device needs to be loaded by calling 'LoadDevice' interface.
    CleanupDiedHostResources(hostClnt, service);
    return 0;
}

static void DevmgrServiceFullOnDeviceHostDied(struct DevmgrServiceFull *inst, uint32_t hostId,
    struct HdfRemoteService *service)
{
    struct DevHostServiceClnt *hostClnt = NULL;
    struct DevHostServiceClnt *hostClntTmp = NULL;
    if (inst == NULL) {
        return;
    }
    OsalMutexLock(&inst->super.devMgrMutex);
    DLIST_FOR_EACH_ENTRY_SAFE(hostClnt, hostClntTmp, &inst->super.hosts, struct DevHostServiceClnt, node) {
        if (hostClnt->hostId == hostId) {
            int32_t ret = DevmgrServiceFullHandleDeviceHostDied(hostClnt, service);
            if (ret == INVALID_PID) {
                HDF_LOGE("%{public}s: failed to respawn host %{public}s", __func__, hostClnt->hostName);
            }
            break;
        }
    }
    OsalMutexUnlock(&inst->super.devMgrMutex);
}

int32_t DevmgrServiceFullDispatchMessage(struct HdfMessageTask *task, struct HdfMessage *msg)
{
    (void)task;
    struct DevmgrServiceFull *fullService = (struct DevmgrServiceFull *)DevmgrServiceGetInstance();
    if (msg == NULL) {
        HDF_LOGE("Input msg is null");
        return HDF_ERR_INVALID_PARAM;
    }

    if (msg->messageId == DEVMGR_MESSAGE_DEVHOST_DIED) {
        int hostId = (int)(uintptr_t)msg->data[0];
        struct HdfRemoteService *service = (struct HdfRemoteService *)msg->data[1];
        DevmgrServiceFullOnDeviceHostDied(fullService, hostId, service);
    } else {
        HDF_LOGE("wrong message(%{public}u)", msg->messageId);
    }

    return HDF_SUCCESS;
}

struct HdfMessageTask *DevmgrServiceFullGetMessageTask(void)
{
    struct DevmgrServiceFull *fullService = (struct DevmgrServiceFull *)DevmgrServiceGetInstance();
    if (fullService != NULL) {
        return &fullService->task;
    }
    HDF_LOGE("Get message task failed, fullService is null");
    return NULL;
}

void DevmgrServiceFullConstruct(struct DevmgrServiceFull *inst)
{
    static struct IHdfMessageHandler handler = {.Dispatch = DevmgrServiceFullDispatchMessage};
    if (inst != NULL) {
        HdfMessageLooperConstruct(&inst->looper);
        DevmgrServiceConstruct(&inst->super);
        HdfMessageTaskConstruct(&inst->task, &inst->looper, &handler);
    }
}

struct HdfObject *DevmgrServiceFullCreate(void)
{
    static struct DevmgrServiceFull *instance = NULL;
    if (instance == NULL) {
        static struct DevmgrServiceFull fullInstance;
        DevmgrServiceFullConstruct(&fullInstance);
        instance = &fullInstance;
    }
    return (struct HdfObject *)instance;
}

int DeviceManagerIsQuickLoad(void)
{
    return false;
}
