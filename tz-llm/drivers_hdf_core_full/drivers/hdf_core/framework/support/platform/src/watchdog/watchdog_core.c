/*
 * Copyright (c) 2020-2023 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include "watchdog_core.h"
#include "hdf_log.h"
#include "watchdog_if.h"
#include "platform_trace.h"

#define WATCHDOG_TRACE_BASIC_PARAM_NUM  1
#define WATCHDOG_TRACE_PARAM_STOP_NUM   1
#define HDF_LOG_TAG watchdog_core

static int32_t WatchdogIoDispatch(struct HdfDeviceIoClient *client, int cmd,
    struct HdfSBuf *data, struct HdfSBuf *reply);
int32_t WatchdogCntlrAdd(struct WatchdogCntlr *cntlr)
{
    int32_t ret;

    if (cntlr == NULL) {
        return HDF_ERR_INVALID_OBJECT;
    }

    if (cntlr->device == NULL) {
        HDF_LOGE("WatchdogCntlrAdd: no device associated!");
        return HDF_ERR_INVALID_OBJECT;
    }

    if (cntlr->ops == NULL) {
        HDF_LOGE("WatchdogCntlrAdd: no ops supplied!");
        return HDF_ERR_INVALID_OBJECT;
    }

    ret = OsalSpinInit(&cntlr->lock);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("WatchdogCntlrAdd: spinlock init fail!");
        return ret;
    }

    cntlr->device->service = &cntlr->service;
    cntlr->device->service->Dispatch = WatchdogIoDispatch;
    return HDF_SUCCESS;
}

void WatchdogCntlrRemove(struct WatchdogCntlr *cntlr)
{
    if (cntlr == NULL) {
        HDF_LOGE("WatchdogCntlrRemove: cntlr is null!");
        return;
    }

    if (cntlr->device == NULL) {
        HDF_LOGE("WatchdogCntlrRemove: no device associated!");
        return;
    }

    cntlr->device->service = NULL;
    (void)OsalSpinDestroy(&cntlr->lock);
}

int32_t WatchdogGetPrivData(struct WatchdogCntlr *cntlr)
{
    if (cntlr == NULL || cntlr->ops == NULL) {
        HDF_LOGE("WatchdogGetPrivData: cntlr or ops is null!");
        return HDF_ERR_INVALID_OBJECT;
    }
    if (cntlr->ops->getPriv != NULL) {
        return cntlr->ops->getPriv(cntlr);
    }
    return HDF_SUCCESS;
}

int32_t WatchdogReleasePriv(struct WatchdogCntlr *cntlr)
{
    if (cntlr == NULL || cntlr->ops == NULL) {
        HDF_LOGE("WatchdogReleasePriv: cntlr or ops is null!");
        return HDF_SUCCESS;
    }
    if (cntlr->ops->releasePriv != NULL) {
        cntlr->ops->releasePriv(cntlr);
    }
    return HDF_SUCCESS;
}

int32_t WatchdogCntlrGetStatus(struct WatchdogCntlr *cntlr, int32_t *status)
{
    int32_t ret;
    uint32_t flags;

    if (cntlr == NULL) {
        HDF_LOGE("WatchdogCntlrGetStatus: cntlr is null!");
        return HDF_ERR_INVALID_OBJECT;
    }
    if (cntlr->ops == NULL || cntlr->ops->getStatus == NULL) {
        HDF_LOGE("WatchdogCntlrGetStatus: ops or getStatus is null!");
        return HDF_ERR_NOT_SUPPORT;
    }
    if (status == NULL) {
        HDF_LOGE("WatchdogCntlrGetStatus: status is null!");
        return HDF_ERR_INVALID_PARAM;
    }

    if (OsalSpinLockIrqSave(&cntlr->lock, &flags) != HDF_SUCCESS) {
        HDF_LOGE("WatchdogCntlrGetStatus: osal spin lock irq save fail!");
        return HDF_ERR_DEVICE_BUSY;
    }
    ret = cntlr->ops->getStatus(cntlr, status);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("WatchdogCntlrGetStatus: getStatus fail!");
        return ret;
    }
    (void)OsalSpinUnlockIrqRestore(&cntlr->lock, &flags);
    return ret;
}

int32_t WatchdogCntlrStart(struct WatchdogCntlr *cntlr)
{
    int32_t ret;
    uint32_t flags;

    if (cntlr == NULL) {
        HDF_LOGE("WatchdogCntlrStart: cntlr is null!");
        return HDF_ERR_INVALID_OBJECT;
    }
    if (cntlr->ops == NULL || cntlr->ops->start == NULL) {
        HDF_LOGE("WatchdogCntlrStart: ops or start is null!");
        return HDF_ERR_NOT_SUPPORT;
    }

    if (OsalSpinLockIrqSave(&cntlr->lock, &flags) != HDF_SUCCESS) {
        HDF_LOGE("WatchdogCntlrStart: osal spin lock irq save fail!");
        return HDF_ERR_DEVICE_BUSY;
    }
    ret = cntlr->ops->start(cntlr);
    if (PlatformTraceStart() == HDF_SUCCESS) {
        unsigned int infos[WATCHDOG_TRACE_BASIC_PARAM_NUM];
        infos[PLATFORM_TRACE_UINT_PARAM_SIZE_1 - 1] = cntlr->wdtId;
        PlatformTraceAddUintMsg(PLATFORM_TRACE_MODULE_WATCHDOG, PLATFORM_TRACE_MODULE_WATCHDOG_FUN_START,
            infos, WATCHDOG_TRACE_BASIC_PARAM_NUM);
        PlatformTraceStop();
    }
    (void)OsalSpinUnlockIrqRestore(&cntlr->lock, &flags);
    return ret;
}

int32_t WatchdogCntlrStop(struct WatchdogCntlr *cntlr)
{
    int32_t ret;
    uint32_t flags;

    if (cntlr == NULL) {
        HDF_LOGE("WatchdogCntlrStop: cntlr is null!");
        return HDF_ERR_INVALID_OBJECT;
    }
    if (cntlr->ops == NULL || cntlr->ops->stop == NULL) {
        HDF_LOGE("WatchdogCntlrStop: ops or stop is null!");
        return HDF_ERR_NOT_SUPPORT;
    }

    if (OsalSpinLockIrqSave(&cntlr->lock, &flags) != HDF_SUCCESS) {
        HDF_LOGE("WatchdogCntlrStop: osal spin lock irq save fail!");
        return HDF_ERR_DEVICE_BUSY;
    }
    ret = cntlr->ops->stop(cntlr);
    if (PlatformTraceStart() == HDF_SUCCESS) {
        unsigned int infos[WATCHDOG_TRACE_PARAM_STOP_NUM];
        infos[PLATFORM_TRACE_UINT_PARAM_SIZE_1 - 1] = cntlr->wdtId;
        PlatformTraceAddUintMsg(PLATFORM_TRACE_MODULE_WATCHDOG, PLATFORM_TRACE_MODULE_WATCHDOG_FUN_STOP,
            infos, WATCHDOG_TRACE_PARAM_STOP_NUM);
        PlatformTraceStop();
        PlatformTraceInfoDump();
    }
    (void)OsalSpinUnlockIrqRestore(&cntlr->lock, &flags);
    return ret;
}

int32_t WatchdogCntlrSetTimeout(struct WatchdogCntlr *cntlr, uint32_t seconds)
{
    int32_t ret;
    uint32_t flags;

    if (cntlr == NULL) {
        HDF_LOGE("WatchdogCntlrSetTimeout: cntlr is null!");
        return HDF_ERR_INVALID_OBJECT;
    }
    if (cntlr->ops == NULL || cntlr->ops->setTimeout == NULL) {
        HDF_LOGE("WatchdogCntlrSetTimeout: ops or setTimeout is null!");
        return HDF_ERR_NOT_SUPPORT;
    }

    if (OsalSpinLockIrqSave(&cntlr->lock, &flags) != HDF_SUCCESS) {
        HDF_LOGE("WatchdogCntlrSetTimeout: osal spin lock irq save fail!");
        return HDF_ERR_DEVICE_BUSY;
    }
    ret = cntlr->ops->setTimeout(cntlr, seconds);
    (void)OsalSpinUnlockIrqRestore(&cntlr->lock, &flags);
    return ret;
}

int32_t WatchdogCntlrGetTimeout(struct WatchdogCntlr *cntlr, uint32_t *seconds)
{
    int32_t ret;
    uint32_t flags;

    if (cntlr == NULL) {
        HDF_LOGE("WatchdogCntlrGetTimeout: cntlr is null!");
        return HDF_ERR_INVALID_OBJECT;
    }
    if (cntlr->ops == NULL || cntlr->ops->getTimeout == NULL) {
        HDF_LOGE("WatchdogCntlrGetTimeout: ops or getTimeout is null!");
        return HDF_ERR_NOT_SUPPORT;
    }
    if (seconds == NULL) {
        HDF_LOGE("WatchdogCntlrGetTimeout: seconds is null!");
        return HDF_ERR_INVALID_PARAM;
    }

    if (OsalSpinLockIrqSave(&cntlr->lock, &flags) != HDF_SUCCESS) {
        HDF_LOGE("WatchdogCntlrGetTimeout: osal spin lock irq save fail!");
        return HDF_ERR_DEVICE_BUSY;
    }
    ret = cntlr->ops->getTimeout(cntlr, seconds);
    (void)OsalSpinUnlockIrqRestore(&cntlr->lock, &flags);
    return ret;
}

int32_t WatchdogCntlrFeed(struct WatchdogCntlr *cntlr)
{
    int32_t ret;
    uint32_t flags;

    if (cntlr == NULL) {
        HDF_LOGE("WatchdogCntlrFeed: cntlr is null!");
        return HDF_ERR_INVALID_OBJECT;
    }
    if (cntlr->ops == NULL || cntlr->ops->feed == NULL) {
        HDF_LOGE("WatchdogCntlrFeed: ops or feed is null!");
        return HDF_ERR_NOT_SUPPORT;
    }

    if (OsalSpinLockIrqSave(&cntlr->lock, &flags) != HDF_SUCCESS) {
        HDF_LOGE("OsalSpinLockIrqSave: osal spin lock irq save fail!");
        return HDF_ERR_DEVICE_BUSY;
    }
    ret = cntlr->ops->feed(cntlr);
    (void)OsalSpinUnlockIrqRestore(&cntlr->lock, &flags);
    return ret;
}

static int32_t WatchdogUserGetPrivData(struct WatchdogCntlr *cntlr, struct HdfSBuf *reply)
{
    int32_t ret;

    if (reply == NULL) {
        HDF_LOGE("WatchdogUserGetPrivData: reply is null!");
        return HDF_ERR_INVALID_OBJECT;
    }
    ret = WatchdogGetPrivData(cntlr);
    if (!HdfSbufWriteInt32(reply, ret)) {
        HDF_LOGE("WatchdogUserGetPrivData: sbuf write buffer fail!");
        return HDF_ERR_IO;
    }
    return HDF_SUCCESS;
}

static int32_t WatchdogUserGetStatus(struct WatchdogCntlr *cntlr, struct HdfSBuf *reply)
{
    int32_t ret;
    int32_t status;

    if (reply == NULL) {
        HDF_LOGE("WatchdogUserGetStatus:reply is null!");
        return HDF_ERR_INVALID_OBJECT;
    }
    ret = WatchdogCntlrGetStatus(cntlr, &status);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("WatchdogUserGetStatus: watchdog cntlr get status fail, ret: %d!", ret);
        return ret;
    }
    if (!HdfSbufWriteInt32(reply, status)) {
        HDF_LOGE("WatchdogUserGetStatus: sbuf write status fail!");
        return HDF_ERR_IO;
    }
    return HDF_SUCCESS;
}

static int32_t WatchdogUserSetTimeout(struct WatchdogCntlr *cntlr, struct HdfSBuf *data)
{
    uint32_t seconds;

    if (data == NULL) {
        HDF_LOGE("WatchdogUserSetTimeout: data is null!");
        return HDF_ERR_INVALID_OBJECT;
    }

    if (!HdfSbufReadUint32(data, &seconds)) {
        HDF_LOGE("WatchdogUserSetTimeout: sbuf read seconds fail!");
        return HDF_ERR_IO;
    }

    return WatchdogCntlrSetTimeout(cntlr, seconds);
}

static int32_t WatchdogUserGetTimeout(struct WatchdogCntlr *cntlr, struct HdfSBuf *reply)
{
    int32_t ret;
    uint32_t seconds;

    if (reply == NULL) {
        HDF_LOGE("WatchdogUserGetTimeout: reply is null!");
        return HDF_ERR_INVALID_OBJECT;
    }
    ret = WatchdogCntlrGetTimeout(cntlr, &seconds);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("WatchdogUserGetTimeout: watchdog cntlr get timeout fail, ret: %d!", ret);
        return ret;
    }
    
    if (!HdfSbufWriteUint32(reply, seconds)) {
        HDF_LOGE("WatchdogUserGetTimeout: sbuf write buffer fail!");
        return HDF_ERR_IO;
    }
    return HDF_SUCCESS;
}

static int32_t WatchdogIoDispatch(struct HdfDeviceIoClient *client, int cmd,
    struct HdfSBuf *data, struct HdfSBuf *reply)
{
    struct WatchdogCntlr *cntlr = NULL;

    if (client == NULL) {
        HDF_LOGE("WatchdogIoDispatch: client is null!");
        return HDF_ERR_INVALID_OBJECT;
    }
    if (client->device == NULL) {
        HDF_LOGE("WatchdogIoDispatch: client->device is null!");
        return HDF_ERR_INVALID_OBJECT;
    }
    if (client->device->service == NULL) {
        HDF_LOGE("WatchdogIoDispatch: client->device->service is null!");
        return HDF_ERR_INVALID_OBJECT;
    }

    cntlr = (struct WatchdogCntlr *)client->device->service;
    switch (cmd) {
        case WATCHDOG_IO_GET_PRIV:
            return WatchdogUserGetPrivData(cntlr, reply);
        case WATCHDOG_IO_RELEASE_PRIV:
            return WatchdogReleasePriv(cntlr);
        case WATCHDOG_IO_GET_STATUS:
            return WatchdogUserGetStatus(cntlr, reply);
        case WATCHDOG_IO_START:
            return WatchdogCntlrStart(cntlr);
        case WATCHDOG_IO_STOP:
            return WatchdogCntlrStop(cntlr);
        case WATCHDOG_IO_SET_TIMEOUT:
            return WatchdogUserSetTimeout(cntlr, data);
        case WATCHDOG_IO_GET_TIMEOUT:
            return WatchdogUserGetTimeout(cntlr, reply);
        case WATCHDOG_IO_FEED:
            return WatchdogCntlrFeed(cntlr);
        default:
            HDF_LOGE("WatchdogIoDispatch: cmd %d is not support!", cmd);
            return HDF_ERR_NOT_SUPPORT;
    }
}
