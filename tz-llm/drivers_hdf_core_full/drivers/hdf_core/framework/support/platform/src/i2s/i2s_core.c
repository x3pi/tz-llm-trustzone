/*
 * Copyright (c) 2021-2023 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include "i2s_core.h"
#include "hdf_log.h"
#include "osal_mem.h"
#include "platform_trace.h"

#define HDF_LOG_TAG i2s_core
#define I2S_TRACE_BASIC_PARAM_NUM  2
#define I2S_TRACE_PARAM_WRITE_NUM  2

int32_t I2sCntlrOpen(struct I2sCntlr *cntlr)
{
    int32_t ret;

    if (cntlr == NULL) {
        HDF_LOGE("I2sCntlrOpen: cntlr is null!");
        return HDF_ERR_INVALID_PARAM;
    }
    if (cntlr->method == NULL || cntlr->method->Open == NULL) {
        HDF_LOGE("I2sCntlrOpen: method or Open is null!");
        return HDF_ERR_NOT_SUPPORT;
    }
    (void)OsalMutexLock(&(cntlr->lock));
    ret = cntlr->method->Open(cntlr);
    (void)OsalMutexUnlock(&(cntlr->lock));
    return ret;
}

int32_t I2sCntlrClose(struct I2sCntlr *cntlr)
{
    int32_t ret;

    if (cntlr == NULL) {
        HDF_LOGE("I2sCntlrClose: cntlr is null!");
        return HDF_ERR_INVALID_PARAM;
    }
    if (cntlr->method == NULL || cntlr->method->Close == NULL) {
        HDF_LOGE("I2sCntlrClose: method or Close is null!");
        return HDF_ERR_NOT_SUPPORT;
    }
    (void)OsalMutexLock(&(cntlr->lock));
    ret = cntlr->method->Close(cntlr);
    (void)OsalMutexUnlock(&(cntlr->lock));
    return ret;
}

int32_t I2sCntlrEnable(struct I2sCntlr *cntlr)
{
    int32_t ret;

    if (cntlr == NULL) {
        HDF_LOGE("I2sCntlrEnable: cntlr is null!");
        return HDF_ERR_INVALID_PARAM;
    }
    if (cntlr->method == NULL || cntlr->method->Enable == NULL) {
        HDF_LOGE("I2sCntlrEnable: method or Enable is null!");
        return HDF_ERR_NOT_SUPPORT;
    }
    (void)OsalMutexLock(&(cntlr->lock));
    ret = cntlr->method->Enable(cntlr);
    (void)OsalMutexUnlock(&(cntlr->lock));
    return ret;
}

int32_t I2sCntlrDisable(struct I2sCntlr *cntlr)
{
    int32_t ret;

    if (cntlr == NULL) {
        HDF_LOGE("I2sCntlrDisable: cntlr is null!");
        return HDF_ERR_INVALID_PARAM;
    }
    if (cntlr->method == NULL || cntlr->method->Disable == NULL) {
        HDF_LOGE("I2sCntlrDisable: method or Disable is null!");
        return HDF_ERR_NOT_SUPPORT;
    }
    (void)OsalMutexLock(&(cntlr->lock));
    ret = cntlr->method->Disable(cntlr);
    (void)OsalMutexUnlock(&(cntlr->lock));
    return ret;
}

int32_t I2sCntlrStartRead(struct I2sCntlr *cntlr)
{
    int32_t ret;

    if (cntlr == NULL) {
        HDF_LOGE("I2sCntlrStartRead: cntlr is null!");
        return HDF_ERR_INVALID_PARAM;
    }
    if (cntlr->method == NULL || cntlr->method->StartRead == NULL) {
        HDF_LOGE("I2sCntlrStartRead: method or StartRead is null!");
        return HDF_ERR_NOT_SUPPORT;
    }

    (void)OsalMutexLock(&(cntlr->lock));
    ret = cntlr->method->StartRead(cntlr);
    if (PlatformTraceStart() == HDF_SUCCESS) {
        unsigned int infos[I2S_TRACE_BASIC_PARAM_NUM];
        infos[PLATFORM_TRACE_UINT_PARAM_SIZE_1 - 1] = cntlr->busNum;
        infos[PLATFORM_TRACE_UINT_PARAM_SIZE_2 - 1] = cntlr->irqNum;
        PlatformTraceAddUintMsg(PLATFORM_TRACE_MODULE_I2S, PLATFORM_TRACE_MODULE_I2S_READ_DATA,
            infos, I2S_TRACE_BASIC_PARAM_NUM);
        PlatformTraceStop();
    }
    (void)OsalMutexUnlock(&(cntlr->lock));
    return ret;
}

int32_t I2sCntlrStopRead(struct I2sCntlr *cntlr)
{
    int32_t ret;

    if (cntlr == NULL) {
        HDF_LOGE("I2sCntlrStopRead: cntlr is null!");
        return HDF_ERR_INVALID_PARAM;
    }
    if (cntlr->method == NULL || cntlr->method->StopRead == NULL) {
        HDF_LOGE("I2sCntlrStopRead: method or StopRead is null!");
        return HDF_ERR_NOT_SUPPORT;
    }
    (void)OsalMutexLock(&(cntlr->lock));
    ret = cntlr->method->StopRead(cntlr);
    (void)OsalMutexUnlock(&(cntlr->lock));
    return ret;
}

int32_t I2sCntlrStartWrite(struct I2sCntlr *cntlr)
{
    int32_t ret;

    if (cntlr == NULL) {
        HDF_LOGE("I2sCntlrStartWrite: cntlr is null!");
        return HDF_ERR_INVALID_PARAM;
    }
    if (cntlr->method == NULL || cntlr->method->StartWrite == NULL) {
        HDF_LOGE("I2sCntlrStartWrite: method or StartWrite is null!");
        return HDF_ERR_NOT_SUPPORT;
    }
    (void)OsalMutexLock(&(cntlr->lock));
    ret = cntlr->method->StartWrite(cntlr);
    if (PlatformTraceStart() == HDF_SUCCESS) {
        unsigned int infos[I2S_TRACE_PARAM_WRITE_NUM];
        infos[PLATFORM_TRACE_UINT_PARAM_SIZE_1 - 1] = cntlr->busNum;
        infos[PLATFORM_TRACE_UINT_PARAM_SIZE_2 - 1] = cntlr->irqNum;
        PlatformTraceAddUintMsg(PLATFORM_TRACE_MODULE_I2S, PLATFORM_TRACE_MODULE_I2S_WRITE_DATA,
            infos, I2S_TRACE_PARAM_WRITE_NUM);
        PlatformTraceStop();
        PlatformTraceInfoDump();
    }
    (void)OsalMutexUnlock(&(cntlr->lock));
    return ret;
}

int32_t I2sCntlrStopWrite(struct I2sCntlr *cntlr)
{
    int32_t ret;

    if (cntlr == NULL) {
        HDF_LOGE("I2sCntlrStopWrite: cntlr is null!");
        return HDF_ERR_INVALID_PARAM;
    }
    if (cntlr->method == NULL || cntlr->method->StopWrite == NULL) {
        HDF_LOGE("I2sCntlrStopWrite: method or StopWrite is null!");
        return HDF_ERR_NOT_SUPPORT;
    }
    (void)OsalMutexLock(&(cntlr->lock));
    ret = cntlr->method->StopWrite(cntlr);
    (void)OsalMutexUnlock(&(cntlr->lock));
    return ret;
}

int32_t I2sCntlrSetCfg(struct I2sCntlr *cntlr, struct I2sCfg *cfg)
{
    int32_t ret;

    if (cntlr == NULL) {
        HDF_LOGE("I2sCntlrSetCfg: cntlr is null!");
        return HDF_ERR_INVALID_PARAM;
    }

    if (cntlr->method == NULL || cntlr->method->SetCfg == NULL) {
        HDF_LOGE("I2sCntlrSetCfg: method or SetCfg is null!");
        return HDF_ERR_NOT_SUPPORT;
    }

    (void)OsalMutexLock(&(cntlr->lock));
    ret = cntlr->method->SetCfg(cntlr, cfg);
    (void)OsalMutexUnlock(&(cntlr->lock));
    return ret;
}

int32_t I2sCntlrGetCfg(struct I2sCntlr *cntlr, struct I2sCfg *cfg)
{
    int32_t ret;

    if (cntlr == NULL) {
        HDF_LOGE("I2sCntlrGetCfg: cntlr is null!");
        return HDF_ERR_INVALID_PARAM;
    }

    if (cntlr->method == NULL || cntlr->method->GetCfg == NULL) {
        HDF_LOGE("I2sCntlrGetCfg: method or GetCfg is null!");
        return HDF_ERR_NOT_SUPPORT;
    }

    (void)OsalMutexLock(&(cntlr->lock));
    ret = cntlr->method->GetCfg(cntlr, cfg);
    (void)OsalMutexUnlock(&(cntlr->lock));
    return ret;
}

int32_t I2sCntlrTransfer(struct I2sCntlr *cntlr, struct I2sMsg *msg)
{
    int32_t ret;

    if (cntlr == NULL || msg == NULL) {
        HDF_LOGE("I2sCntlrTransfer: cntlr or msg is null!");
        return HDF_ERR_INVALID_PARAM;
    }
    if (cntlr->method == NULL || cntlr->method->Transfer == NULL) {
        HDF_LOGE("I2sCntlrTransfer: method or Transfer is null!");
        return HDF_ERR_NOT_SUPPORT;
    }

    (void)OsalMutexLock(&(cntlr->lock));
    ret = cntlr->method->Transfer(cntlr, msg);
    (void)OsalMutexUnlock(&(cntlr->lock));
    return ret;
}

struct I2sCntlr *I2sCntlrCreate(struct HdfDeviceObject *device)
{
    struct I2sCntlr *cntlr = NULL;

    if (device == NULL) {
        HDF_LOGE("I2sCntlrCreate: device is null!");
        return NULL;
    }

    cntlr = (struct I2sCntlr *)OsalMemCalloc(sizeof(*cntlr));
    if (cntlr == NULL) {
        HDF_LOGE("I2sCntlrCreate: memcalloc cntlr fail!");
        return NULL;
    }
    cntlr->device = device;
    device->service = &(cntlr->service);
    (void)OsalMutexInit(&cntlr->lock);
    cntlr->priv = NULL;
    cntlr->method = NULL;
    return cntlr;
}

void I2sCntlrDestroy(struct I2sCntlr *cntlr)
{
    if (cntlr == NULL) {
        HDF_LOGE("I2sCntlrDestroy: cntlr is null!");
        return;
    }

    (void)OsalMutexDestroy(&(cntlr->lock));
    cntlr->device = NULL;
    cntlr->method = NULL;
    cntlr->priv = NULL;
    OsalMemFree(cntlr);
}
