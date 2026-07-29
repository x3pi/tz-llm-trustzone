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

#include "devhost_dump.h"
#include "devhost_dump_reg.h"
#include "hdf_base.h"
#include "hdf_cstring.h"
#include "hdf_dlist.h"
#include "hdf_log.h"
#include "osal_mem.h"
#include "osal_mutex.h"

#define HDF_LOG_TAG devhost_dump

struct DumpServiceNode {
    DevHostDumpFunc dumpService;
    char *servName;
    struct DListHead node;
};

struct DumpHostNode {
    struct DListHead list;
    struct OsalMutex mutex;
    DevHostDumpFunc dumpHost;
};

static struct DumpHostNode g_dumpHostNode;

void DevHostDumpInit(void)
{
    OsalMutexInit(&g_dumpHostNode.mutex);
    DListHeadInit(&g_dumpHostNode.list);
    g_dumpHostNode.dumpHost = NULL;
}

void DevHostDumpDeInit(void)
{
    struct DumpServiceNode *pos = NULL;
    struct DumpServiceNode *tmp = NULL;

    OsalMutexLock(&g_dumpHostNode.mutex);
    DLIST_FOR_EACH_ENTRY_SAFE(pos, tmp, &g_dumpHostNode.list, struct DumpServiceNode, node) {
        DListRemove(&pos->node);
        OsalMemFree(pos->servName);
        OsalMemFree(pos);
    }
    OsalMutexUnlock(&g_dumpHostNode.mutex);

    OsalMutexDestroy(&g_dumpHostNode.mutex);
    DListHeadInit(&g_dumpHostNode.list);
    g_dumpHostNode.dumpHost = NULL;
}

static bool DevHostCheckDumpExist(const char *servName)
{
    struct DumpServiceNode *pos = NULL;
    bool findFlag = false;
    OsalMutexLock(&g_dumpHostNode.mutex);
    DLIST_FOR_EACH_ENTRY(pos, &g_dumpHostNode.list, struct DumpServiceNode, node) {
        if (strcmp(pos->servName, servName) == 0) {
            findFlag = true;
            break;
        }
    }
    OsalMutexUnlock(&g_dumpHostNode.mutex);
    return findFlag;
}

int32_t DevHostRegisterDumpService(const char *servName, DevHostDumpFunc dump)
{
    if (dump == NULL || servName == NULL) {
        return HDF_FAILURE;
    }

    if (DevHostCheckDumpExist(servName)) {
        HDF_LOGE("%{public}s service %{public}s dump function exist", __func__, servName);
        return HDF_FAILURE;
    }

    struct DumpServiceNode *node = (struct DumpServiceNode *)OsalMemCalloc(sizeof(*node));
    if (node == NULL) {
        return HDF_FAILURE;
    }

    HDF_LOGI("%{public}s enter", __func__);
    node->dumpService = dump;
    node->servName = HdfStringCopy(servName);
    if (node->servName == NULL) {
        OsalMemFree(node);
        return HDF_FAILURE;
    }
    OsalMutexLock(&g_dumpHostNode.mutex);
    DListInsertTail(&node->node, &g_dumpHostNode.list);
    OsalMutexUnlock(&g_dumpHostNode.mutex);

    return HDF_SUCCESS;
}

int32_t DevHostRegisterDumpHost(DevHostDumpFunc dump)
{
    if (dump == NULL) {
        return HDF_FAILURE;
    }

    g_dumpHostNode.dumpHost = dump;

    return HDF_SUCCESS;
}

void DevHostDump(struct HdfSBuf *data, struct HdfSBuf *reply)
{
    HDF_LOGI("%{public}s enter", __func__);
    if (data == NULL || reply == NULL) {
        return;
    }

    const char *option = HdfSbufReadString(data);
    if (option == NULL) {
        return;
    }

    if (strcmp(option, "dumpHost") == 0) {
        if (g_dumpHostNode.dumpHost == NULL) {
            HDF_LOGE("%{public}s no dumpHost function", __func__);
            (void)HdfSbufWriteString(reply, "The host does not register dump function\n");
            return;
        }
        g_dumpHostNode.dumpHost(data, reply);
    } else if (strcmp(option, "dumpService") == 0) {
        const char *servName = HdfSbufReadString(data);
        struct DumpServiceNode *pos = NULL;
        bool dumpFlag = false;
        OsalMutexLock(&g_dumpHostNode.mutex);
        DLIST_FOR_EACH_ENTRY(pos, &g_dumpHostNode.list, struct DumpServiceNode, node) {
            if (strcmp(pos->servName, servName) != 0) {
                continue;
            }
            if (pos->dumpService) {
                pos->dumpService(data, reply);
                dumpFlag = true;
                break;
            }
        }
        OsalMutexUnlock(&g_dumpHostNode.mutex);
        if (!dumpFlag) {
            (void)HdfSbufWriteString(reply, "The service does not register dump function\n");
        }
    } else {
        HDF_LOGE("%{public}s invalid parameter %{public}s", __func__, option);
    }

    return;
}
