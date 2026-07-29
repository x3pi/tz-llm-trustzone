/*
 * Copyright (c) 2022-2023 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include "hdf_base.h"
#include "hdf_dlist.h"
#include "hdf_io_service_if.h"
#include "osal_mem.h"
#include "pin_if.h"
#include "platform_core.h"
#include "securec.h"

#define HDF_LOG_TAG   pin_if_u_c
#define PIN_NAME_LEN  128
#define FUNC_NAME_LEN 30

#define PIN_SERVICE_NAME "HDF_PLATFORM_PIN_MANAGER"

static struct DListHead g_listHead;
static OsalSpinlock g_listLock;

struct PinInfo {
    char pinName[PIN_NAME_LEN];
    char funcName[FUNC_NAME_LEN];
    struct DListHead node;
};

static const char *AddNode(const char *pinName)
{
    struct PinInfo *pin = NULL;
    static struct DListHead *head = NULL;

    if (head == NULL) {
        head = &g_listHead;
        DListHeadInit(head);
        OsalSpinInit(&g_listLock);
    }
    pin = OsalMemCalloc(sizeof(struct PinInfo));
    if (pin == NULL) {
        HDF_LOGE("AddNode: malloc pin fail!");
        return NULL;
    }
    DListHeadInit(&pin->node);
    if (strcpy_s(pin->pinName, PIN_NAME_LEN, pinName) != EOK) {
        HDF_LOGE("AddNode: copy pinName fail!");
        OsalMemFree(pin);
        return NULL;
    }
    (void)OsalSpinLock(&g_listLock);
    DListInsertTail(&pin->node, head);
    (void)OsalSpinUnlock(&g_listLock);
    return pin->pinName;
}

static void RemoveNode(const char *pinName)
{
    struct DListHead *head = &g_listHead;
    struct PinInfo *pos = NULL;
    struct PinInfo *tmp = NULL;

    (void)OsalSpinLock(&g_listLock);
    DLIST_FOR_EACH_ENTRY_SAFE(pos, tmp, head, struct PinInfo, node) {
        if (strcmp(pos->pinName, pinName) == 0) {
            DListRemove(&pos->node);
            OsalMemFree(pos);
        }
    }
    (void)OsalSpinUnlock(&g_listLock);
}

static struct HdfIoService *PinManagerServiceGet(void)
{
    static struct HdfIoService *service = NULL;

    if (service != NULL) {
        return service;
    }
    service = HdfIoServiceBind(PIN_SERVICE_NAME);
    if (service == NULL) {
        HDF_LOGE("PinServiceGetService: fail to get pin service!");
    }
    return service;
}

DevHandle PinGet(const char *pinName)
{
    int32_t ret;
    const char *copyName = NULL;
    struct HdfIoService *service = NULL;
    struct HdfSBuf *data = NULL;

    if (pinName == NULL) {
        HDF_LOGE("PinGet: pinName is null!");
        return NULL;
    }

    service = PinManagerServiceGet();
    if (service == NULL || service->dispatcher == NULL || service->dispatcher->Dispatch == NULL) {
        HDF_LOGE("PinGet: service is invalid!");
        return NULL;
    }

    data = HdfSbufObtainDefaultSize();
    if (data == NULL) {
        HDF_LOGE("PinGet: fail to obtain data!");
        return NULL;
    }

    if (!HdfSbufWriteString(data, pinName)) {
        HDF_LOGE("PinGet: write dec fail!");
        HdfSbufRecycle(data);
        return NULL;
    }

    ret = service->dispatcher->Dispatch(&service->object, PIN_IO_GET, data, NULL);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("PinGet: PIN_IO_GET service process fail, ret: %d!", ret);
        HdfSbufRecycle(data);
        return NULL;
    }

    copyName = AddNode(pinName);
    if (copyName == NULL) {
        HDF_LOGE("PinGet: copyName is null!");
        HdfSbufRecycle(data);
        return NULL;
    }

    HdfSbufRecycle(data);
    return (void *)copyName;
}

void PinPut(DevHandle handle)
{
    int32_t ret;
    struct HdfIoService *service = NULL;
    struct HdfSBuf *data = NULL;

    if (handle == NULL) {
        HDF_LOGE("PinPut: handle is null!");
        return;
    }

    service = PinManagerServiceGet();
    if (service == NULL || service->dispatcher == NULL || service->dispatcher->Dispatch == NULL) {
        HDF_LOGE("PinPut: service is invalid!");
        RemoveNode((const char *)handle);
        return;
    }

    data = HdfSbufObtainDefaultSize();
    if (data == NULL) {
        HDF_LOGE("PinPut: fail to obtain data!");
        RemoveNode((const char *)handle);
        return;
    }
    if (!HdfSbufWriteString(data, handle)) {
        HDF_LOGE("PinPut: write handle fail!");
        RemoveNode((const char *)handle);
        HdfSbufRecycle(data);
        return;
    }

    ret = service->dispatcher->Dispatch(&service->object, PIN_IO_PUT, data, NULL);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("PinPut: PIN_IO_PUT service process fail, ret: %d!", ret);
    }
    RemoveNode((const char *)handle);
    HdfSbufRecycle(data);
}

int32_t PinSetPull(DevHandle handle, enum PinPullType pullType)
{
    int32_t ret;
    struct HdfIoService *service = NULL;
    struct HdfSBuf *data = NULL;

    if (handle == NULL) {
        HDF_LOGE("PinSetPull: handle is null!");
        return HDF_ERR_INVALID_PARAM;
    }

    service = PinManagerServiceGet();
    if (service == NULL || service->dispatcher == NULL || service->dispatcher->Dispatch == NULL) {
        HDF_LOGE("PinSetPull: service is invalid!");
        return HDF_PLT_ERR_DEV_GET;
    }

    data = HdfSbufObtainDefaultSize();
    if (data == NULL) {
        HDF_LOGE("PinSetPull: fail to obtain data!");
        return HDF_ERR_MALLOC_FAIL;
    }

    if (!HdfSbufWriteString(data, handle)) {
        HDF_LOGE("PinSetPull: write handle fail!");
        HdfSbufRecycle(data);
        return HDF_ERR_IO;
    }

    if (!HdfSbufWriteUint32(data, (uint32_t)pullType)) {
        HDF_LOGE("PinSetPull: write pulltype fail!");
        HdfSbufRecycle(data);
        return HDF_ERR_IO;
    }

    ret = service->dispatcher->Dispatch(&service->object, PIN_IO_SET_PULL, data, NULL);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("PinSetPull: PIN_IO_SET_PULL service process fail, ret: %d!", ret);
    }
    HdfSbufRecycle(data);
    return ret;
}

int32_t PinGetPull(DevHandle handle, enum PinPullType *pullType)
{
    int32_t ret;
    struct HdfIoService *service = NULL;
    struct HdfSBuf *reply = NULL;
    struct HdfSBuf *data = NULL;

    if (handle == NULL || pullType == NULL) {
        HDF_LOGE("PinGetPull: handle or pullType is null!");
        return HDF_ERR_INVALID_PARAM;
    }

    service = PinManagerServiceGet();
    if (service == NULL || service->dispatcher == NULL || service->dispatcher->Dispatch == NULL) {
        HDF_LOGE("PinGetPull: service is invalid!");
        return HDF_PLT_ERR_DEV_GET;
    }

    data = HdfSbufObtainDefaultSize();
    if (data == NULL) {
        HDF_LOGE("PinGetPull: fail to obtain data!");
        return HDF_ERR_MALLOC_FAIL;
    }

    reply = HdfSbufObtainDefaultSize();
    if (reply == NULL) {
        HDF_LOGE("PinGetPull: fail to obtain reply!");
        HdfSbufRecycle(data);
        return HDF_ERR_MALLOC_FAIL;
    }

    if (!HdfSbufWriteString(data, handle)) {
        HDF_LOGE("PinGetPull: write handle fail!");
        HdfSbufRecycle(reply);
        HdfSbufRecycle(data);
        return HDF_ERR_IO;
    }

    ret = service->dispatcher->Dispatch(&service->object, PIN_IO_GET_PULL, data, reply);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("PinGetPull: PIN_IO_GET_PULL service process fail, ret: %d!", ret);
        HdfSbufRecycle(reply);
        HdfSbufRecycle(data);
        return ret;
    }

    if (!HdfSbufReadUint32(reply, pullType)) {
        HDF_LOGE("PinGetPull: read pulltype fail!");
        ret = HDF_ERR_IO;
    }

    HdfSbufRecycle(reply);
    HdfSbufRecycle(data);
    return ret;
}

int32_t PinSetStrength(DevHandle handle, uint32_t strength)
{
    int32_t ret;
    struct HdfIoService *service = NULL;
    struct HdfSBuf *data = NULL;

    if (handle == NULL) {
        HDF_LOGE("PinSetStrength: handle is null!");
        return HDF_ERR_INVALID_PARAM;
    }

    service = PinManagerServiceGet();
    if (service == NULL || service->dispatcher == NULL || service->dispatcher->Dispatch == NULL) {
        HDF_LOGE("PinSetStrength: service is invalid!");
        return HDF_PLT_ERR_DEV_GET;
    }

    data = HdfSbufObtainDefaultSize();
    if (data == NULL) {
        HDF_LOGE("PinSetStrength: fail to obtain data!");
        return HDF_ERR_MALLOC_FAIL;
    }

    if (!HdfSbufWriteString(data, handle)) {
        HDF_LOGE("PinSetStrength: write handle fail!");
        HdfSbufRecycle(data);
        return HDF_ERR_IO;
    }

    if (!HdfSbufWriteUint32(data, strength)) {
        HDF_LOGE("PinSetStrength: write strength fail!");
        HdfSbufRecycle(data);
        return HDF_ERR_IO;
    }

    ret = service->dispatcher->Dispatch(&service->object, PIN_IO_SET_STRENGTH, data, NULL);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("PinSetStrength: PIN_IO_SET_STRENGTH service process fail, ret: %d!", ret);
    }
    HdfSbufRecycle(data);
    return ret;
}

int32_t PinGetStrength(DevHandle handle, uint32_t *strength)
{
    int32_t ret;
    struct HdfIoService *service = NULL;
    struct HdfSBuf *reply = NULL;
    struct HdfSBuf *data = NULL;

    if (handle == NULL || strength == NULL) {
        HDF_LOGE("PinGetStrength: handle or strength is null!");
        return HDF_ERR_INVALID_PARAM;
    }

    service = PinManagerServiceGet();
    if (service == NULL || service->dispatcher == NULL || service->dispatcher->Dispatch == NULL) {
        HDF_LOGE("PinGetStrength: service is invalid!");
        return HDF_PLT_ERR_DEV_GET;
    }

    data = HdfSbufObtainDefaultSize();
    if (data == NULL) {
        HDF_LOGE("PinGetStrength: fail to obtain data!");
        return HDF_ERR_MALLOC_FAIL;
    }

    reply = HdfSbufObtainDefaultSize();
    if (reply == NULL) {
        HDF_LOGE("PinGetStrength: fail to obtain reply!");
        HdfSbufRecycle(data);
        return HDF_ERR_MALLOC_FAIL;
    }

    if (!HdfSbufWriteString(data, handle)) {
        HDF_LOGE("PinGetStrength: write handle fail!");
        HdfSbufRecycle(reply);
        HdfSbufRecycle(data);
        return HDF_ERR_IO;
    }

    ret = service->dispatcher->Dispatch(&service->object, PIN_IO_GET_STRENGTH, data, reply);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("PinGetStrength: PIN_IO_GET_STRENGTH service process fail, ret: %d!", ret);
        HdfSbufRecycle(reply);
        HdfSbufRecycle(data);
        return ret;
    }

    if (!HdfSbufReadUint32(reply, strength)) {
        HDF_LOGE("PinGetStrength: read strength fail!");
        ret = HDF_ERR_IO;
    }

    HdfSbufRecycle(data);
    HdfSbufRecycle(reply);
    return ret;
}

int32_t PinSetFunc(DevHandle handle, const char *funcName)
{
    int32_t ret;
    struct HdfIoService *service = NULL;
    struct HdfSBuf *data = NULL;

    if (handle == NULL || funcName == NULL) {
        HDF_LOGE("PinSetFunc: handle or funcName is null!");
        return HDF_ERR_INVALID_PARAM;
    }

    service = PinManagerServiceGet();
    if (service == NULL || service->dispatcher == NULL || service->dispatcher->Dispatch == NULL) {
        HDF_LOGE("PinSetFunc: service is invalid!");
        return HDF_PLT_ERR_DEV_GET;
    }

    data = HdfSbufObtainDefaultSize();
    if (data == NULL) {
        HDF_LOGE("PinSetFunc: fail to obtain data!");
        return HDF_ERR_MALLOC_FAIL;
    }

    if (!HdfSbufWriteString(data, handle)) {
        HDF_LOGE("PinSetFunc: write handle fail!");
        HdfSbufRecycle(data);
        return HDF_ERR_IO;
    }

    if (!HdfSbufWriteString(data, funcName)) {
        HDF_LOGE("PinSetFunc: write funcName fail!");
        HdfSbufRecycle(data);
        return HDF_ERR_IO;
    }

    ret = service->dispatcher->Dispatch(&service->object, PIN_IO_SET_FUNC, data, NULL);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("PinSetFunc: PIN_IO_SET_FUNC service process fail, ret: %d!", ret);
    }
    HdfSbufRecycle(data);
    return ret;
}

static int32_t CopyFuncName(const char *pinName, const char *tempName, const char **funcName)
{
    struct DListHead *head = &g_listHead;
    struct PinInfo *pos = NULL;
    struct PinInfo *tmp = NULL;

    (void)OsalSpinLock(&g_listLock);
    DLIST_FOR_EACH_ENTRY_SAFE(pos, tmp, head, struct PinInfo, node) {
        if (strcmp(pos->pinName, pinName) == 0) {
            if (strcpy_s(pos->funcName, FUNC_NAME_LEN, tempName) != EOK) {
                HDF_LOGE("CopyFuncName: copy tempName fail!");
                (void)OsalSpinUnlock(&g_listLock);
                return HDF_FAILURE;
            }
            *funcName = (const char *)&pos->funcName;
            (void)OsalSpinUnlock(&g_listLock);
            return HDF_SUCCESS;
        }
    }
    (void)OsalSpinUnlock(&g_listLock);
    return HDF_FAILURE;
}

int32_t PinGetFunc(DevHandle handle, const char **funcName)
{
    int32_t ret;
    struct HdfIoService *service = NULL;
    struct HdfSBuf *reply = NULL;
    struct HdfSBuf *data = NULL;
    const char *tempName = NULL;

    if (handle == NULL || funcName == NULL) {
        HDF_LOGE("PinGetFunc: handle or funcName is null!");
        return HDF_ERR_INVALID_PARAM;
    }

    service = PinManagerServiceGet();
    if (service == NULL || service->dispatcher == NULL || service->dispatcher->Dispatch == NULL) {
        HDF_LOGE("PinGetFunc: service is invalid!");
        return HDF_PLT_ERR_DEV_GET;
    }

    data = HdfSbufObtainDefaultSize();
    if (data == NULL) {
        HDF_LOGE("PinGetFunc: fail to obtain data!");
        return HDF_ERR_MALLOC_FAIL;
    }

    reply = HdfSbufObtainDefaultSize();
    if (reply == NULL) {
        HDF_LOGE("PinGetFunc: fail to obtain reply!");
        HdfSbufRecycle(data);
        return HDF_ERR_MALLOC_FAIL;
    }

    if (!HdfSbufWriteString(data, handle)) {
        HDF_LOGE("PinGetFunc: write handle fail!");
        HdfSbufRecycle(reply);
        HdfSbufRecycle(data);
        return HDF_ERR_IO;
    }

    ret = service->dispatcher->Dispatch(&service->object, PIN_IO_GET_FUNC, data, reply);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("PinGetFunc: PIN_IO_GET_FUNC service process fail, ret: %d!", ret);
        HdfSbufRecycle(data);
        HdfSbufRecycle(reply);
        return ret;
    }

    tempName = HdfSbufReadString(reply);
    if (tempName == NULL) {
        HDF_LOGE("PinGetFunc: tempName is null!");
        return HDF_ERR_IO;
    }
    ret = CopyFuncName((const char *)handle, tempName, funcName);
    HdfSbufRecycle(data);
    HdfSbufRecycle(reply);
    return ret;
}
