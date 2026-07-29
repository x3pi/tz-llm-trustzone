/*
 * Copyright (c) 2020-2023 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include "spi_core.h"
#include "hdf_log.h"
#include "osal_mem.h"
#include "spi_if.h"
#include "spi_service.h"
#include "platform_trace.h"

#define SPI_TRACE_BASIC_PARAM_NUM  3
#define SPI_TRACE_PARAM_GET_NUM    3
#define HDF_LOG_TAG spi_core

int32_t SpiCntlrOpen(struct SpiCntlr *cntlr, uint32_t csNum)
{
    int32_t ret;

    if (cntlr == NULL) {
        HDF_LOGE("SpiCntlrOpen: cntlr is null!");
        return HDF_ERR_INVALID_PARAM;
    }
    if (cntlr->method == NULL || cntlr->method->Open == NULL) {
        HDF_LOGE("SpiCntlrOpen: method or Open is null!");
        return HDF_ERR_NOT_SUPPORT;
    }
    (void)OsalMutexLock(&(cntlr->lock));
    cntlr->curCs = csNum;
    ret = cntlr->method->Open(cntlr);
    (void)OsalMutexUnlock(&(cntlr->lock));
    return ret;
}

int32_t SpiCntlrClose(struct SpiCntlr *cntlr, uint32_t csNum)
{
    int32_t ret;

    if (cntlr == NULL) {
        HDF_LOGE("SpiCntlrClose: cntlr is null!");
        return HDF_ERR_INVALID_PARAM;
    }
    if (cntlr->method == NULL || cntlr->method->Close == NULL) {
        HDF_LOGE("SpiCntlrClose: method or Close is null!");
        return HDF_ERR_NOT_SUPPORT;
    }
    (void)OsalMutexLock(&(cntlr->lock));
    cntlr->curCs = csNum;
    ret = cntlr->method->Close(cntlr);
    (void)OsalMutexUnlock(&(cntlr->lock));
    return ret;
}

int32_t SpiCntlrTransfer(struct SpiCntlr *cntlr, uint32_t csNum, struct SpiMsg *msg, uint32_t count)
{
    int32_t ret;

    if (cntlr == NULL) {
        HDF_LOGE("SpiCntlrTransfer: cntlr is null!");
        return HDF_ERR_INVALID_PARAM;
    }
    if (cntlr->method == NULL || cntlr->method->Transfer == NULL) {
        HDF_LOGE("SpiCntlrTransfer: method or Transfer is null!");
        return HDF_ERR_NOT_SUPPORT;
    }

    (void)OsalMutexLock(&(cntlr->lock));
    cntlr->curCs = csNum;
    ret = cntlr->method->Transfer(cntlr, msg, count);
    (void)OsalMutexUnlock(&(cntlr->lock));
    return ret;
}

int32_t SpiCntlrSetCfg(struct SpiCntlr *cntlr, uint32_t csNum, struct SpiCfg *cfg)
{
    int32_t ret;

    if (cntlr == NULL) {
        HDF_LOGE("SpiCntlrSetCfg: cntlr is null!");
        return HDF_ERR_INVALID_PARAM;
    }

    if (cntlr->method == NULL || cntlr->method->SetCfg == NULL) {
        HDF_LOGE("SpiCntlrSetCfg: method or SetCfg is null!");
        return HDF_ERR_NOT_SUPPORT;
    }

    (void)OsalMutexLock(&(cntlr->lock));
    cntlr->curCs = csNum;
    ret = cntlr->method->SetCfg(cntlr, cfg);
    if (PlatformTraceStart() == HDF_SUCCESS) {
        unsigned int infos[SPI_TRACE_BASIC_PARAM_NUM];
        infos[PLATFORM_TRACE_UINT_PARAM_SIZE_1 - 1] = cntlr->busNum;
        infos[PLATFORM_TRACE_UINT_PARAM_SIZE_2 - 1] = cntlr->numCs;
        infos[PLATFORM_TRACE_UINT_PARAM_SIZE_3 - 1] = cntlr->curCs;
        PlatformTraceAddUintMsg(PLATFORM_TRACE_MODULE_SPI, PLATFORM_TRACE_MODULE_SPI_FUN_SET,
            infos, SPI_TRACE_BASIC_PARAM_NUM);
        PlatformTraceStop();
    }
    (void)OsalMutexUnlock(&(cntlr->lock));
    return ret;
}

int32_t SpiCntlrGetCfg(struct SpiCntlr *cntlr, uint32_t csNum, struct SpiCfg *cfg)
{
    int32_t ret;

    if (cntlr == NULL) {
        HDF_LOGE("SpiCntlrGetCfg: cntlr is null!");
        return HDF_ERR_INVALID_PARAM;
    }

    if (cntlr->method == NULL || cntlr->method->GetCfg == NULL) {
        HDF_LOGE("SpiCntlrGetCfg: method or GetCfg is null!");
        return HDF_ERR_NOT_SUPPORT;
    }

    (void)OsalMutexLock(&(cntlr->lock));
    cntlr->curCs = csNum;
    ret = cntlr->method->GetCfg(cntlr, cfg);
    if (PlatformTraceStart() == HDF_SUCCESS) {
        unsigned int infos[SPI_TRACE_PARAM_GET_NUM];
        infos[PLATFORM_TRACE_UINT_PARAM_SIZE_1 - 1] = cntlr->busNum;
        infos[PLATFORM_TRACE_UINT_PARAM_SIZE_2 - 1] = cntlr->numCs;
        infos[PLATFORM_TRACE_UINT_PARAM_SIZE_3 - 1] = cntlr->curCs;
        PlatformTraceAddUintMsg(PLATFORM_TRACE_MODULE_SPI, PLATFORM_TRACE_MODULE_SPI_FUN_GET,
            infos, SPI_TRACE_PARAM_GET_NUM);
        PlatformTraceStop();
        PlatformTraceInfoDump();
    }
    (void)OsalMutexUnlock(&(cntlr->lock));
    return ret;
}

static int32_t SpiMsgsRwProcess(
    struct HdfSBuf *data, uint32_t count, uint8_t *tmpFlag, struct SpiMsg *msgs, uint32_t *lenReply)
{
    int32_t i;
    uint32_t len;
    uint8_t *buf = NULL;
    uint32_t rbufLen;
    struct SpiUserMsg *userMsg = NULL;

    for (i = 0; i < count; i++) {
        if ((!HdfSbufReadBuffer(data, (const void **)&userMsg, &len)) || (userMsg == NULL) ||
            (len != sizeof(struct SpiUserMsg))) {
            HDF_LOGE("SpiMsgsRwProcess: read msg[%d] userMsg fail!", i);
            return HDF_ERR_IO;
        }

        msgs[i].len = userMsg->len;
        msgs[i].speed = userMsg->speed;
        msgs[i].delayUs = userMsg->delayUs;
        msgs[i].keepCs = userMsg->keepCs;
        msgs[i].rbuf = NULL;
        msgs[i].wbuf = NULL;
        if ((userMsg->rwFlag & SPI_USER_MSG_READ) == SPI_USER_MSG_READ) {
            (*lenReply) += msgs[i].len;
            msgs[i].rbuf = tmpFlag; // tmpFlag is not mainpulated, only to mark rbuf not NULL
        }
        if ((userMsg->rwFlag & SPI_USER_MSG_WRITE) == SPI_USER_MSG_WRITE) {
            if ((!HdfSbufReadBuffer(data, (const void **)&buf, &len)) || (buf == NULL) || (len != msgs[i].len)) {
                HDF_LOGE("SpiMsgsRwProcess: read msg[%d] wbuf fail, len[%u], msgs[%d].len[%u]!", i,
                    len, i, msgs[i].len);
            } else {
                msgs[i].wbuf = buf;
            }
        }
    }

    if ((!HdfSbufReadUint32(data, &rbufLen)) || (rbufLen != *lenReply)) {
        HDF_LOGE("SpiMsgsRwProcess: read rbufLen failed %u != %u!", rbufLen, *lenReply);
        return HDF_ERR_IO;
    }

    return HDF_SUCCESS;
}

// data format:csNum -- count -- count data records:SpiUserMsg( if write data has write buffer data) -- rbufLen
static int32_t SpiTransferRebuildMsgs(struct HdfSBuf *data, struct SpiMsg **ppmsgs, uint32_t *pcount, uint8_t **ppbuf)
{
    uint32_t count;
    int32_t i;
    uint32_t lenReply = 0;
    uint8_t *buf = NULL;
    uint8_t tmpFlag = 0;
    uint8_t *bufReply = NULL;
    struct SpiMsg *msgs = NULL;

    if (!HdfSbufReadUint32(data, &count) || (count == 0)) {
        HDF_LOGE("SpiTransferRebuildMsgs: read count fail!");
        return HDF_ERR_IO;
    }

    msgs = OsalMemCalloc(sizeof(struct SpiMsg) * count);
    if (msgs == NULL) {
        HDF_LOGE("SpiTransferRebuildMsgs: memcalloc fail!");
        return HDF_ERR_MALLOC_FAIL;
    }

    if (SpiMsgsRwProcess(data, count, &tmpFlag, msgs, &lenReply) != HDF_SUCCESS) {
        HDF_LOGE("SpiTransferRebuildMsgs: SpiMsgsRwProcess fail!");
        return HDF_FAILURE;
    }

    if (lenReply > 0) {
        bufReply = OsalMemCalloc(lenReply);
        if (bufReply == NULL) {
            HDF_LOGE("SpiTransferRebuildMsgs: memcalloc fail!");
            return HDF_ERR_MALLOC_FAIL;
        }
        for (i = 0, buf = bufReply; i < count && buf < (bufReply + lenReply); i++) {
            if (msgs[i].rbuf == &tmpFlag) {
                msgs[i].rbuf = buf;
                buf += msgs[i].len;
            }
        }
    }

    *ppmsgs = msgs;
    *pcount = count;
    *ppbuf = bufReply;

    return HDF_SUCCESS;
}

static int32_t SpiTransferWriteBackMsgs(struct HdfSBuf *reply, struct SpiMsg *msgs, uint32_t count)
{
    uint32_t i;

    for (i = 0; i < count; i++) {
        if (msgs[i].rbuf == NULL) {
            continue;
        }

        if (!HdfSbufWriteBuffer(reply, msgs[i].rbuf, msgs[i].len)) {
            HDF_LOGE("SpiTransferWriteBackMsgs: write msg[%u] reply fail!", i);
            return HDF_ERR_IO;
        }
    }

    return HDF_SUCCESS;
}

static int32_t SpiIoTransfer(struct SpiCntlr *cntlr, uint32_t csNum, struct HdfSBuf *data, struct HdfSBuf *reply)
{
    int32_t ret;
    uint32_t count;
    struct SpiMsg *msgs = NULL;
    uint8_t *bufReply = NULL;

    if (data == NULL || reply == NULL) {
        HDF_LOGE("SpiIoTransfer: data or reply is null!");
        return HDF_ERR_INVALID_PARAM;
    }

    ret = SpiTransferRebuildMsgs(data, &msgs, &count, &bufReply);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("SpiIoTransfer: rebuild msgs fail, ret: %d!", ret);
        goto EXIT;
    }

    ret = SpiCntlrTransfer(cntlr, csNum, msgs, count);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("SpiIoTransfer: do transfer fail, ret: %d!", ret);
        goto EXIT;
    }

    ret = SpiTransferWriteBackMsgs(reply, msgs, count);

EXIT:
    OsalMemFree(bufReply);
    OsalMemFree(msgs);
    return ret;
}

static inline int32_t SpiIoOpen(struct SpiCntlr *cntlr, uint32_t csNum)
{
    return SpiCntlrOpen(cntlr, csNum);
}

static inline int32_t SpiIoClose(struct SpiCntlr *cntlr, uint32_t csNum)
{
    return SpiCntlrClose(cntlr, csNum);
}

static int32_t SpiIoSetConfig(struct SpiCntlr *cntlr, uint32_t csNum, struct HdfSBuf *data)
{
    uint32_t len;
    struct SpiCfg *cfg = NULL;

    if (!HdfSbufReadBuffer(data, (const void **)&cfg, &len) || sizeof(*cfg) != len) {
        HDF_LOGE("SpiIoSetConfig: read buffer fail!");
        return HDF_ERR_IO;
    }
    return SpiCntlrSetCfg(cntlr, csNum, cfg);
}

static int32_t SpiIoGetConfig(struct SpiCntlr *cntlr, uint32_t csNum, struct HdfSBuf *reply)
{
    int32_t ret;
    struct SpiCfg cfg;

    ret = SpiCntlrGetCfg(cntlr, csNum, &cfg);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("SpiIoGetConfig: get cfg fail!");
        return ret;
    }

    if (!HdfSbufWriteBuffer(reply, &cfg, sizeof(cfg))) {
        HDF_LOGE("SpiIoGetConfig: write buffer fail!");
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

static int32_t SpiIoDispatch(struct HdfDeviceIoClient *client, int cmd, struct HdfSBuf *data, struct HdfSBuf *reply)
{
    int32_t ret;
    uint32_t csNum;
    struct SpiCntlr *cntlr = NULL;

    if (client == NULL || client->device == NULL || client->device->service == NULL) {
        HDF_LOGE("SpiIoDispatch: invalid client!");
        return HDF_ERR_INVALID_OBJECT;
    }

    cntlr = (struct SpiCntlr *)client->device->service;
    if (data == NULL) {
        HDF_LOGE("SpiIoDispatch: data is null!");
        return HDF_ERR_INVALID_PARAM;
    }

    if (!HdfSbufReadUint32(data, &csNum)) {
        HDF_LOGE("SpiIoDispatch: read csNum fail!");
        return HDF_ERR_IO;
    }

    switch (cmd) {
        case SPI_IO_OPEN:
            return SpiIoOpen(cntlr, csNum);
        case SPI_IO_CLOSE:
            return SpiIoClose(cntlr, csNum);
        case SPI_IO_SET_CONFIG:
            return SpiIoSetConfig(cntlr, csNum, data);
        case SPI_IO_GET_CONFIG:
            return SpiIoGetConfig(cntlr, csNum, reply);
        case SPI_IO_TRANSFER:
            return SpiIoTransfer(cntlr, csNum, data, reply);
        default:
            HDF_LOGE("SpiIoDispatch: cmd %d is not support!", cmd);
            ret = HDF_ERR_NOT_SUPPORT;
            break;
    }
    return ret;
}

void SpiCntlrDestroy(struct SpiCntlr *cntlr)
{
    if (cntlr == NULL) {
        HDF_LOGE("SpiCntlrDestroy: device is null!");
        return;
    }
    (void)OsalMutexDestroy(&(cntlr->lock));
    OsalMemFree(cntlr);
}

struct SpiCntlr *SpiCntlrCreate(struct HdfDeviceObject *device)
{
    struct SpiCntlr *cntlr = NULL;

    if (device == NULL) {
        HDF_LOGE("SpiCntlrCreate: device is null!");
        return NULL;
    }

    cntlr = (struct SpiCntlr *)OsalMemCalloc(sizeof(*cntlr));
    if (cntlr == NULL) {
        HDF_LOGE("SpiCntlrCreate: OsalMemCalloc error!");
        return NULL;
    }
    cntlr->device = device;
    device->service = &(cntlr->service);
    device->service->Dispatch = SpiIoDispatch;
    (void)OsalMutexInit(&cntlr->lock);
    DListHeadInit(&cntlr->list);
    cntlr->priv = NULL;
    return cntlr;
}
