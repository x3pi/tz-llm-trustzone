/*
 * Copyright (c) 2022-2023 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include "timer_core.h"
#include "hdf_device_object.h"
#include "hdf_log.h"
#include "osal_mem.h"
#include "osal_time.h"
#include "platform_listener_common.h"
#include "platform_trace.h"
#include "securec.h"

#define HDF_LOG_TAG timer_core

struct TimerManager {
    struct IDeviceIoService service;
    struct HdfDeviceObject *device;
    struct DListHead timerListHead;
    struct OsalMutex lock;
};

static struct TimerManager *g_timerManager = NULL;
#define TIMER_HANDLE_SHIFT          ((uintptr_t)(-1) << 16)
#define TIMER_TRACE_PARAM_NUM       5
#define TIMER_TRACE_BASIC_PARAM_NUM 3
#define TIMER_TRACE_PARAM_STOP_NUM  3

struct TimerCntrl *TimerCntrlOpen(const uint32_t number)
{
    struct TimerCntrl *pos = NULL;
    struct TimerManager *manager = g_timerManager;
    CHECK_NULL_PTR_RETURN_VALUE(manager, NULL);

    if (OsalMutexLock(&manager->lock) != HDF_SUCCESS) {
        HDF_LOGE("TimerCntrlOpen: OsalMutexLock fail!");
        return NULL;
    }

    DLIST_FOR_EACH_ENTRY(pos, &manager->timerListHead, struct TimerCntrl, node) {
        if (number == pos->info.number) {
            (void)OsalMutexUnlock(&manager->lock);
            return pos;
        }
    }

    (void)OsalMutexUnlock(&manager->lock);
    HDF_LOGE("TimerCntrlOpen: open %u fail!", number);
    return NULL;
}

int32_t TimerCntrlClose(struct TimerCntrl *cntrl)
{
    CHECK_NULL_PTR_RETURN_VALUE(cntrl, HDF_ERR_INVALID_OBJECT);
    if (OsalMutexLock(&cntrl->lock) != HDF_SUCCESS) {
        HDF_LOGE("TimerCntrlClose: OsalMutexLock %u fail!", cntrl->info.number);
        return HDF_ERR_DEVICE_BUSY;
    }
    if ((cntrl->ops->Close != NULL) && (cntrl->ops->Close(cntrl) != HDF_SUCCESS)) {
        HDF_LOGE("TimerCntrlClose: close %u fail!", cntrl->info.number);
        (void)OsalMutexUnlock(&cntrl->lock);
        return HDF_FAILURE;
    }
    (void)OsalMutexUnlock(&cntrl->lock);
    return HDF_SUCCESS;
}

int32_t TimerCntrlSet(struct TimerCntrl *cntrl, uint32_t useconds, TimerHandleCb cb)
{
    CHECK_NULL_PTR_RETURN_VALUE(cntrl, HDF_ERR_INVALID_OBJECT);
    CHECK_NULL_PTR_RETURN_VALUE(cb, HDF_ERR_INVALID_OBJECT);
    if (OsalMutexLock(&cntrl->lock) != HDF_SUCCESS) {
        HDF_LOGE("TimerCntrlSet: OsalMutexLock %u fail!", cntrl->info.number);
        return HDF_ERR_DEVICE_BUSY;
    }
    if ((cntrl->ops->Set != NULL) && (cntrl->ops->Set(cntrl, useconds, cb) != HDF_SUCCESS)) {
        HDF_LOGE("TimerCntrlSet: set %u fail!", cntrl->info.number);
        (void)OsalMutexUnlock(&cntrl->lock);
        return HDF_FAILURE;
    }

    (void)OsalMutexUnlock(&cntrl->lock);
    return HDF_SUCCESS;
}

int32_t TimerCntrlSetOnce(struct TimerCntrl *cntrl, uint32_t useconds, TimerHandleCb cb)
{
    CHECK_NULL_PTR_RETURN_VALUE(cntrl, HDF_ERR_INVALID_OBJECT);
    CHECK_NULL_PTR_RETURN_VALUE(cb, HDF_ERR_INVALID_OBJECT);

    if (OsalMutexLock(&cntrl->lock) != HDF_SUCCESS) {
        HDF_LOGE("TimerCntrlSetOnce: OsalMutexLock %u fail!", cntrl->info.number);
        return HDF_ERR_DEVICE_BUSY;
    }
    if ((cntrl->ops->SetOnce != NULL) && (cntrl->ops->SetOnce(cntrl, useconds, cb) != HDF_SUCCESS)) {
        HDF_LOGE("TimerCntrlSetOnce: setOnce %u fail!", cntrl->info.number);
        (void)OsalMutexUnlock(&cntrl->lock);
        return HDF_FAILURE;
    }

    (void)OsalMutexUnlock(&cntrl->lock);
    return HDF_SUCCESS;
}

int32_t TimerCntrlGet(struct TimerCntrl *cntrl, uint32_t *useconds, bool *isPeriod)
{
    CHECK_NULL_PTR_RETURN_VALUE(cntrl, HDF_ERR_INVALID_OBJECT);
    CHECK_NULL_PTR_RETURN_VALUE(useconds, HDF_ERR_INVALID_OBJECT);
    CHECK_NULL_PTR_RETURN_VALUE(isPeriod, HDF_ERR_INVALID_OBJECT);

    if (OsalMutexLock(&cntrl->lock) != HDF_SUCCESS) {
        HDF_LOGE("TimerCntrlGet: OsalMutexLock %u fail!", cntrl->info.number);
        return HDF_ERR_DEVICE_BUSY;
    }
    *useconds = cntrl->info.useconds;
    *isPeriod = cntrl->info.isPeriod;
    (void)OsalMutexUnlock(&cntrl->lock);
    return HDF_SUCCESS;
}

int32_t TimerCntrlStart(struct TimerCntrl *cntrl)
{
    CHECK_NULL_PTR_RETURN_VALUE(cntrl, HDF_ERR_INVALID_OBJECT);

    if (OsalMutexLock(&cntrl->lock) != HDF_SUCCESS) {
        HDF_LOGE("TimerCntrlStart: OsalMutexLock %u fail!", cntrl->info.number);
        return HDF_ERR_DEVICE_BUSY;
    }
    if ((cntrl->ops->Start != NULL) && (cntrl->ops->Start(cntrl) != HDF_SUCCESS)) {
        HDF_LOGE("TimerCntrlStart: start %u fail!", cntrl->info.number);
        (void)OsalMutexUnlock(&cntrl->lock);
        return HDF_FAILURE;
    }

    int32_t ret = PlatformTraceStart();
    if (ret == HDF_SUCCESS) {
        unsigned int infos[TIMER_TRACE_BASIC_PARAM_NUM];
        infos[PLATFORM_TRACE_UINT_PARAM_SIZE_1 - 1] = cntrl->info.number;
        infos[PLATFORM_TRACE_UINT_PARAM_SIZE_2 - 1] = cntrl->info.useconds;
        infos[PLATFORM_TRACE_UINT_PARAM_SIZE_3 - 1] = cntrl->info.isPeriod;
        PlatformTraceAddUintMsg(
            PLATFORM_TRACE_MODULE_TIMER, PLATFORM_TRACE_MODULE_TIMER_FUN_START, infos, TIMER_TRACE_BASIC_PARAM_NUM);
        PlatformTraceStop();
        PlatformTraceInfoDump();
    }

    (void)OsalMutexUnlock(&cntrl->lock);
    return HDF_SUCCESS;
}

int32_t TimerCntrlStop(struct TimerCntrl *cntrl)
{
    CHECK_NULL_PTR_RETURN_VALUE(cntrl, HDF_ERR_INVALID_OBJECT);

    if (OsalMutexLock(&cntrl->lock) != HDF_SUCCESS) {
        HDF_LOGE("TimerCntrlStop: OsalMutexLock %u fail!", cntrl->info.number);
        return HDF_ERR_DEVICE_BUSY;
    }
    if ((cntrl->ops->Stop != NULL) && (cntrl->ops->Stop(cntrl) != HDF_SUCCESS)) {
        HDF_LOGE("TimerCntrlStop: stop %u fail!", cntrl->info.number);
        (void)OsalMutexUnlock(&cntrl->lock);
        return HDF_FAILURE;
    }

    int32_t ret = PlatformTraceStart();
    if (ret == HDF_SUCCESS) {
        unsigned int infos[TIMER_TRACE_PARAM_STOP_NUM];
        infos[0] = cntrl->info.number;
        PlatformTraceAddUintMsg(
            PLATFORM_TRACE_MODULE_TIMER, PLATFORM_TRACE_MODULE_TIMER_FUN_STOP, infos, TIMER_TRACE_PARAM_STOP_NUM);
        PlatformTraceStop();
        PlatformTraceInfoDump();
    }
    (void)OsalMutexUnlock(&cntrl->lock);
    return HDF_SUCCESS;
}

static int32_t TimerIoOpen(struct HdfSBuf *data, struct HdfSBuf *reply)
{
    int16_t number;
    uint32_t handle;

    if ((data == NULL) || (reply == NULL)) {
        HDF_LOGE("TimerIoOpen: param invalid!");
        return HDF_ERR_INVALID_PARAM;
    }
    if (!HdfSbufReadUint16(data, (uint16_t *)&number)) {
        HDF_LOGE("TimerIoOpen: read number fail!");
        return HDF_ERR_IO;
    }

    if (number < 0) {
        HDF_LOGE("TimerIoOpen: info read fail!");
        return HDF_ERR_INVALID_PARAM;
    }

    if (TimerCntrlOpen(number) == NULL) {
        HDF_LOGE("TimerIoOpen: timer cntlr open %d fail!", number);
        return HDF_FAILURE;
    }

    handle = (uint32_t)(number + TIMER_HANDLE_SHIFT);
    if (!HdfSbufWriteUint32(reply, handle)) {
        HDF_LOGE("TimerIoOpen: write handle fail!");
        return HDF_ERR_IO;
    }
    return HDF_SUCCESS;
}

static int32_t TimerIoClose(struct HdfSBuf *data, struct HdfSBuf *reply)
{
    uint32_t handle;
    int16_t number;

    (void)reply;
    if (data == NULL) {
        HDF_LOGE("TimerIoClose: data is null!");
        return HDF_ERR_INVALID_PARAM;
    }

    if (!HdfSbufReadUint32(data, &handle)) {
        HDF_LOGE("TimerIoClose: read handle fail!");
        return HDF_ERR_IO;
    }

    number = (int16_t)(handle - TIMER_HANDLE_SHIFT);
    if (number < 0) {
        HDF_LOGE("TimerIoClose: number[%d] invalid!", number);
        return HDF_ERR_INVALID_PARAM;
    }
    return TimerCntrlClose(TimerCntrlOpen(number));
}

static int32_t TimerIoStart(struct HdfSBuf *data, struct HdfSBuf *reply)
{
    uint32_t handle;
    int16_t number;

    (void)reply;
    if (data == NULL) {
        HDF_LOGE("TimerIoStart: data is null!");
        return HDF_ERR_INVALID_PARAM;
    }
    if (!HdfSbufReadUint32(data, &handle)) {
        HDF_LOGE("TimerIoStart: read handle fail!");
        return HDF_ERR_IO;
    }

    number = (int16_t)(handle - TIMER_HANDLE_SHIFT);
    if (number < 0) {
        HDF_LOGE("TimerIoStart: number[%d] is invalid!", number);
        return HDF_ERR_INVALID_PARAM;
    }
    return TimerCntrlStart(TimerCntrlOpen(number));
}

static int32_t TimerIoStop(struct HdfSBuf *data, struct HdfSBuf *reply)
{
    uint32_t handle;
    int16_t number;

    (void)reply;
    if (data == NULL) {
        HDF_LOGE("TimerIoStop: reply is null!");
        return HDF_ERR_INVALID_PARAM;
    }
    if (!HdfSbufReadUint32(data, &handle)) {
        HDF_LOGE("TimerIoStop: read handle fail!");
        return HDF_ERR_IO;
    }

    number = (int16_t)(handle - TIMER_HANDLE_SHIFT);
    if (number < 0) {
        HDF_LOGE("TimerIoStop: number[%d] is invalid!", number);
        return HDF_ERR_INVALID_PARAM;
    }
    return TimerCntrlStop(TimerCntrlOpen(number));
}

static int32_t TimerIoCb(uint32_t number)
{
    uint32_t handle = number + TIMER_HANDLE_SHIFT;
    int32_t ret;
    struct HdfSBuf *data = NULL;

    data = HdfSbufObtainDefaultSize();
    if (data == NULL) {
        HDF_LOGE("TimerIoCb: fail to obtain data!");
        return HDF_ERR_IO;
    }
    if (!HdfSbufWriteUint32(data, handle)) {
        HDF_LOGE("TimerIoCb: write handle fail!");
        HdfSbufRecycle(data);
        return HDF_ERR_IO;
    }
    ret = HdfDeviceSendEvent(g_timerManager->device, PLATFORM_LISTENER_EVENT_TIMER_NOTIFY, data);
    HdfSbufRecycle(data);
    HDF_LOGD("TimerIoCb: set service info done, ret = %d %d", ret, handle);
    return HDF_SUCCESS;
}

static int32_t TimerIoSet(struct HdfSBuf *data, struct HdfSBuf *reply)
{
    uint32_t len;
    uint32_t handle;
    int16_t number;
    struct TimerConfig *cfg = NULL;

    (void)reply;
    if (data == NULL) {
        HDF_LOGE("TimerIoSet: data is null!");
        return HDF_ERR_INVALID_PARAM;
    }
    if (!HdfSbufReadUint32(data, &handle)) {
        HDF_LOGE("TimerIoSet: read handle fail!");
        return HDF_ERR_IO;
    }

    if (!HdfSbufReadBuffer(data, (const void **)&cfg, &len) || cfg == NULL) {
        HDF_LOGE("TimerIoSet: read buffer fail!");
        return HDF_ERR_IO;
    }

    number = (int16_t)(handle - TIMER_HANDLE_SHIFT);
    if (number < 0) {
        HDF_LOGE("TimerIoSet: number[%d] is invalid!", number);
        return HDF_ERR_INVALID_PARAM;
    }
    return TimerCntrlSet(TimerCntrlOpen(number), cfg->useconds, TimerIoCb);
}

static int32_t TimerIoSetOnce(struct HdfSBuf *data, struct HdfSBuf *reply)
{
    uint32_t len;
    uint32_t handle;
    int16_t number;
    struct TimerConfig *cfg = NULL;

    (void)reply;
    if (data == NULL) {
        HDF_LOGE("TimerIoSetOnce: data is null!");
        return HDF_ERR_INVALID_PARAM;
    }
    if (!HdfSbufReadUint32(data, &handle)) {
        HDF_LOGE("TimerIoSetOnce: read handle fail!");
        return HDF_ERR_IO;
    }

    if (!HdfSbufReadBuffer(data, (const void **)&cfg, &len) || cfg == NULL) {
        HDF_LOGE("TimerIoSetOnce: read buffer fail!");
        return HDF_ERR_IO;
    }

    number = (int16_t)(handle - TIMER_HANDLE_SHIFT);
    if (number < 0) {
        HDF_LOGE("TimerIoSetOnce: number[%d] is invalid!", number);
        return HDF_ERR_INVALID_PARAM;
    }
    return TimerCntrlSetOnce(TimerCntrlOpen(number), cfg->useconds, TimerIoCb);
}

static int32_t TimerIoGet(struct HdfSBuf *data, struct HdfSBuf *reply)
{
    int32_t ret = HDF_SUCCESS;
    struct TimerConfig cfg;
    uint32_t handle;
    int16_t number;

    if ((data == NULL) || (reply == NULL)) {
        HDF_LOGE("TimerIoGet: data or reply is null!");
        return HDF_ERR_INVALID_PARAM;
    }

    if (!HdfSbufReadUint32(data, &handle)) {
        HDF_LOGE("TimerIoGet: read handle fail!");
        return HDF_ERR_IO;
    }

    number = (int16_t)(handle - TIMER_HANDLE_SHIFT);
    if (number < 0) {
        HDF_LOGE("TimerIoGet: number[%d] is invalid!", number);
        return HDF_ERR_INVALID_PARAM;
    }
    cfg.number = number;
    ret = TimerCntrlGet(TimerCntrlOpen(number), &cfg.useconds, &cfg.isPeriod);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("TimerIoGet: get timer cntlr fail!");
        return ret;
    }

    if (!HdfSbufWriteBuffer(reply, &cfg, sizeof(cfg))) {
        HDF_LOGE("TimerIoGet: write buffer fail!");
        return HDF_FAILURE;
    }

    return HDF_SUCCESS;
}

static int32_t TimerIoDispatch(struct HdfDeviceIoClient *client, int cmd, struct HdfSBuf *data, struct HdfSBuf *reply)
{
    int32_t ret;

    (void)client;
    switch (cmd) {
        case TIMER_IO_OPEN:
            ret = TimerIoOpen(data, reply);
            break;
        case TIMER_IO_CLOSE:
            ret = TimerIoClose(data, reply);
            break;
        case TIMER_IO_START:
            ret = TimerIoStart(data, reply);
            break;
        case TIMER_IO_STOP:
            ret = TimerIoStop(data, reply);
            break;
        case TIMER_IO_SET:
            ret = TimerIoSet(data, reply);
            break;
        case TIMER_IO_SETONCE:
            ret = TimerIoSetOnce(data, reply);
            break;
        case TIMER_IO_GET:
            ret = TimerIoGet(data, reply);
            break;
        default:
            ret = HDF_ERR_NOT_SUPPORT;
            HDF_LOGE("TimerIoDispatch: cmd[%d] is not support!", cmd);
            break;
    }
    return ret;
}

int32_t TimerListRemoveAll(void)
{
    struct TimerCntrl *pos = NULL;
    struct TimerCntrl *tmp = NULL;
    struct TimerManager *manager = g_timerManager;

    if (OsalMutexLock(&manager->lock) != HDF_SUCCESS) {
        HDF_LOGE("TimerListRemoveAll: lock regulator manager fail!");
        return HDF_ERR_DEVICE_BUSY;
    }

    DLIST_FOR_EACH_ENTRY_SAFE(pos, tmp, &manager->timerListHead, struct TimerCntrl, node) {
        if ((pos->ops->Remove != NULL) && (pos->ops->Remove(pos) != HDF_SUCCESS)) {
            HDF_LOGE("TimerListRemoveAll: remove %u fail!", pos->info.number);
        }
        DListRemove(&pos->node);
        (void)OsalMutexDestroy(&pos->lock);
        OsalMemFree(pos);
    }

    (void)OsalMutexUnlock(&manager->lock);
    HDF_LOGI("TimerListRemoveAll: remove all regulator success!");
    return HDF_SUCCESS;
}

int32_t TimerCntrlAdd(struct TimerCntrl *cntrl)
{
    CHECK_NULL_PTR_RETURN_VALUE(cntrl, HDF_ERR_INVALID_PARAM);
    CHECK_NULL_PTR_RETURN_VALUE(cntrl->ops, HDF_ERR_INVALID_PARAM);
    struct TimerCntrl *pos = NULL;
    struct TimerCntrl *tmp = NULL;
    struct TimerManager *manager = g_timerManager;
    CHECK_NULL_PTR_RETURN_VALUE(manager, HDF_FAILURE);

    DLIST_FOR_EACH_ENTRY_SAFE(pos, tmp, &manager->timerListHead, struct TimerCntrl, node) {
        if (cntrl->info.number == pos->info.number) {
            HDF_LOGE("TimerCntrlAdd: timer[%u] existed!", cntrl->info.number);
            return HDF_FAILURE;
        }
    }

    // init info
    if (OsalMutexInit(&cntrl->lock) != HDF_SUCCESS) {
        HDF_LOGE("TimerCntrlAdd: OsalMutexInit %u fail!", cntrl->info.number);
        return HDF_FAILURE;
    }

    if (OsalMutexLock(&manager->lock) != HDF_SUCCESS) {
        HDF_LOGE("TimerCntrlAdd: OsalMutexLock %u fail!", cntrl->info.number);
        return HDF_ERR_DEVICE_BUSY;
    }
    DListInsertTail(&cntrl->node, &manager->timerListHead);
    (void)OsalMutexUnlock(&manager->lock);
    HDF_LOGI("TimerCntrlAdd: add timer number[%u] success!", cntrl->info.number);

    return HDF_SUCCESS;
}

int32_t TimerCntrlRemoveByNumber(const uint32_t number)
{
    struct TimerCntrl *pos = NULL;
    struct TimerCntrl *tmp = NULL;
    struct TimerManager *manager = g_timerManager;
    CHECK_NULL_PTR_RETURN_VALUE(manager, HDF_FAILURE);

    if (OsalMutexLock(&manager->lock) != HDF_SUCCESS) {
        HDF_LOGE("TimerCntrlRemoveByNumber: OsalMutexLock fail!");
        return HDF_ERR_DEVICE_BUSY;
    }

    DLIST_FOR_EACH_ENTRY_SAFE(pos, tmp, &manager->timerListHead, struct TimerCntrl, node) {
        if (number == pos->info.number) {
            if ((pos->ops->Remove != NULL) && (pos->ops->Remove(pos) != HDF_SUCCESS)) {
                HDF_LOGE("TimerCntrlRemoveByNumber: remove %u fail!", pos->info.number);
            }
            (void)OsalMutexDestroy(&pos->lock);
            DListRemove(&pos->node);
            OsalMemFree(pos);
            break;
        }
    }

    (void)OsalMutexUnlock(&manager->lock);
    HDF_LOGI("TimerCntrlRemoveByNumber: remove timer %u success!", number);
    return HDF_SUCCESS;
}

static int32_t TimerManagerBind(struct HdfDeviceObject *device)
{
    int32_t ret;
    struct TimerManager *manager = NULL;

    CHECK_NULL_PTR_RETURN_VALUE(device, HDF_ERR_INVALID_OBJECT);

    manager = (struct TimerManager *)OsalMemCalloc(sizeof(*manager));
    if (manager == NULL) {
        HDF_LOGE("TimerManagerBind: malloc manager fail!");
        return HDF_ERR_MALLOC_FAIL;
    }

    ret = OsalMutexInit(&manager->lock);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("TimerManagerBind: mutex init fail, ret: %d", ret);
        OsalMemFree(manager);
        manager = NULL;
        return HDF_FAILURE;
    }

    manager->device = device;
    device->service = &manager->service;
    device->service->Dispatch = TimerIoDispatch;
    DListHeadInit(&manager->timerListHead);
    g_timerManager = manager;

    HDF_LOGI("TimerManagerBind: success!");
    return HDF_SUCCESS;
}

static int32_t TimerManagerInit(struct HdfDeviceObject *device)
{
    HdfDeviceSetClass(device, DEVICE_CLASS_PLAT);
    HDF_LOGI("TimerManagerInit: success!");
    return HDF_SUCCESS;
}

static void TimerManagerRelease(struct HdfDeviceObject *device)
{
    HDF_LOGI("TimerManagerRelease: enter!");
    CHECK_NULL_PTR_RETURN(device);

    if (TimerListRemoveAll() != HDF_SUCCESS) {
        HDF_LOGE("TimerManagerRelease: timer list remove all fail!");
    }

    struct TimerManager *manager = (struct TimerManager *)device->service;
    CHECK_NULL_PTR_RETURN(manager);
    OsalMutexDestroy(&manager->lock);
    OsalMemFree(manager);
    g_timerManager = NULL;
}

struct HdfDriverEntry g_timerManagerEntry = {
    .moduleVersion = 1,
    .Bind = TimerManagerBind,
    .Init = TimerManagerInit,
    .Release = TimerManagerRelease,
    .moduleName = "HDF_PLATFORM_TIMER_MANAGER",
};

HDF_INIT(g_timerManagerEntry);
