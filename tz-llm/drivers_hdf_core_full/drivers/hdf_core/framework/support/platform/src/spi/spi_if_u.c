/*
 * Copyright (c) 2022-2023 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include "hdf_io_service_if.h"
#include "hdf_log.h"
#include "osal_mem.h"
#include "securec.h"
#include "spi_if.h"
#include "spi_service.h"

#define HDF_LOG_TAG   spi_if_u
#define HOST_NAME_LEN 32

struct SpiClient {
    struct SpiCntlr *cntlr;
    uint32_t csNum;
};

static struct HdfIoService *SpiGetCntlrByBusNum(uint32_t num)
{
    int ret;
    char name[HOST_NAME_LEN + 1] = {0};
    struct HdfIoService *service = NULL;

    ret = snprintf_s(name, HOST_NAME_LEN + 1, HOST_NAME_LEN, "HDF_PLATFORM_SPI_%u", num);
    if (ret < 0) {
        HDF_LOGE("SpiGetCntlrByBusNum: snprintf_s fail!");
        return NULL;
    }
    service = HdfIoServiceBind(name);

    return service;
}

static int32_t SpiMsgSetDataHead(struct HdfSBuf *data, uint32_t csNum, uint32_t count)
{
    if (!HdfSbufWriteUint32(data, csNum)) {
        HDF_LOGE("SpiMsgSetDataHead: write csNum fail!");
        return HDF_ERR_IO;
    }
    if (!HdfSbufWriteUint32(data, count)) {
        HDF_LOGE("SpiMsgSetDataHead: write count fail!");
        return HDF_ERR_IO;
    }

    return HDF_SUCCESS;
}

// data format:csNum -- count -- count data records:SpiUserMsg( if write data has write buffer data) -- rbufLen
static int32_t SpiMsgWriteArray(struct HdfSBuf *data, struct SpiMsg *msgs, uint32_t count, struct HdfSBuf **reply)
{
    uint32_t i;
    uint32_t replyLen = 0;
    uint32_t rbufLen = 0;
    struct SpiUserMsg userMsg = {0};

    for (i = 0; i < count; i++) {
        if (msgs[i].rbuf != NULL) {
            userMsg.rwFlag |= SPI_USER_MSG_READ;
            replyLen += msgs[i].len + sizeof(uint64_t);
            rbufLen += msgs[i].len;
        }
        if (msgs[i].wbuf != NULL) {
            userMsg.rwFlag |= SPI_USER_MSG_WRITE;
        }
        userMsg.len = msgs[i].len;
        userMsg.speed = msgs[i].speed;
        userMsg.delayUs = msgs[i].delayUs;
        userMsg.keepCs = msgs[i].keepCs;
        if (!HdfSbufWriteBuffer(data, &userMsg, sizeof(struct SpiUserMsg))) {
            HDF_LOGE("SpiMsgWriteArray: write userMsgs[%u] buf fail!", i);
            return HDF_ERR_IO;
        }
        (void)memset_s(&userMsg, sizeof(struct SpiUserMsg), 0, sizeof(struct SpiUserMsg));

        if (msgs[i].wbuf != NULL) {
            if (!HdfSbufWriteBuffer(data, (uint8_t *)msgs[i].wbuf, msgs[i].len)) {
                HDF_LOGE("SpiMsgWriteArray: write msg[%u] buf fail!", i);
                return HDF_ERR_IO;
            }
        }
    }

    if (!HdfSbufWriteUint32(data, rbufLen)) {
        HDF_LOGE("SpiMsgWriteArray: write count fail!");
        return HDF_ERR_IO;
    }

    *reply = (replyLen == 0) ? HdfSbufObtainDefaultSize() : HdfSbufObtain(replyLen);
    if (*reply == NULL) {
        HDF_LOGE("SpiMsgWriteArray: fail to obtain reply!");
        return HDF_ERR_IO;
    }

    return HDF_SUCCESS;
}

static int32_t SpiMsgReadBack(struct HdfSBuf *data, struct SpiMsg *msg)
{
    uint32_t rLen;
    const void *rBuf = NULL;

    if (!HdfSbufReadBuffer(data, &rBuf, &rLen)) {
        HDF_LOGE("SpiMsgReadBack: read rBuf fail!");
        return HDF_ERR_IO;
    }
    if (msg->len != rLen) {
        HDF_LOGW("SpiMsgReadBack: err len:%u, rLen:%u!", msg->len, rLen);
        if (rLen > msg->len) {
            rLen = msg->len;
        }
    }
    if (memcpy_s(msg->rbuf, msg->len, rBuf, rLen) != EOK) {
        HDF_LOGE("SpiMsgReadBack: memcpy rBuf fail!");
        return HDF_ERR_IO;
    }

    return HDF_SUCCESS;
}

static int32_t SpiMsgReadArray(struct HdfSBuf *reply, struct SpiMsg *msgs, uint32_t count)
{
    uint32_t i;
    int32_t ret;

    for (i = 0; i < count; i++) {
        if (msgs[i].rbuf == NULL) {
            continue;
        }
        ret = SpiMsgReadBack(reply, &msgs[i]);
        if (ret != HDF_SUCCESS) {
            HDF_LOGE("SpiMsgReadArray: spi msg read back fail!");
            return ret;
        }
    }
    return HDF_SUCCESS;
}

int32_t SpiTransfer(DevHandle handle, struct SpiMsg *msgs, uint32_t count)
{
    int32_t ret;
    struct HdfSBuf *data = NULL;
    struct HdfSBuf *reply = NULL;
    struct HdfIoService *service = NULL;
    struct SpiClient *client = NULL;

    if (handle == NULL || msgs == NULL || count == 0) {
        HDF_LOGE("SpiTransfer: invalid handle or msgs or count!");
        return HDF_ERR_INVALID_OBJECT;
    }
    client = (struct SpiClient *)handle;

    data = HdfSbufObtainDefaultSize();
    if (data == NULL) {
        HDF_LOGE("SpiTransfer: fail to obtain data!");
        return HDF_ERR_MALLOC_FAIL;
    }
    ret = SpiMsgSetDataHead(data, client->csNum, count);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("SpiTransfer: spi msg set data head fail!");
        goto EXIT;
    }

    ret = SpiMsgWriteArray(data, msgs, count, &reply);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("SpiTransfer: fail to write msgs!");
        goto EXIT;
    }

    service = (struct HdfIoService *)client->cntlr;
    if (service == NULL || service->dispatcher == NULL || service->dispatcher->Dispatch == NULL) {
        HDF_LOGE("SpiTransfer: service is invalid!");
        ret = HDF_FAILURE;
        goto EXIT;
    }
    ret = service->dispatcher->Dispatch(&service->object, SPI_IO_TRANSFER, data, reply);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("SpiTransfer: failed to send service call ret:%d!", ret);
        goto EXIT;
    }

    ret = SpiMsgReadArray(reply, msgs, count);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("SpiTransfer: spi msg read array fail, ret:%d!", ret);
        goto EXIT;
    }

    ret = HDF_SUCCESS;
EXIT:
    HdfSbufRecycle(data);
    HdfSbufRecycle(reply);
    return ret;
}

int32_t SpiRead(DevHandle handle, uint8_t *buf, uint32_t len)
{
    struct SpiMsg msg = {0};

    msg.wbuf = NULL;
    msg.rbuf = buf;
    msg.len = len;
    return SpiTransfer(handle, &msg, 1);
}

int32_t SpiWrite(DevHandle handle, uint8_t *buf, uint32_t len)
{
    struct SpiMsg msg = {0};

    msg.wbuf = buf;
    msg.rbuf = NULL;
    msg.len = len;
    return SpiTransfer(handle, &msg, 1);
}

int32_t SpiSetCfg(DevHandle handle, struct SpiCfg *cfg)
{
    int32_t ret;
    struct SpiClient *client = NULL;
    struct HdfSBuf *data = NULL;
    struct HdfIoService *service = NULL;

    if (handle == NULL || cfg == NULL) {
        HDF_LOGE("SpiSetCfg: handle or cfg is null!");
        return HDF_ERR_INVALID_OBJECT;
    }
    client = (struct SpiClient *)handle;
    data = HdfSbufObtainDefaultSize();
    if (data == NULL) {
        HDF_LOGE("SpiSetCfg: fail to obtain data!");
        return HDF_ERR_MALLOC_FAIL;
    }

    if (!HdfSbufWriteUint32(data, client->csNum)) {
        HDF_LOGE("SpiSetCfg: write csNum fail!");
        HdfSbufRecycle(data);
        return HDF_FAILURE;
    }

    if (!HdfSbufWriteBuffer(data, cfg, sizeof(*cfg))) {
        HDF_LOGE("SpiSetCfg: write cfg fail!");
        HdfSbufRecycle(data);
        return HDF_FAILURE;
    }

    service = (struct HdfIoService *)client->cntlr;
    if (service == NULL || service->dispatcher == NULL || service->dispatcher->Dispatch == NULL) {
        HDF_LOGE("SpiSetCfg: service is invalid!");
        HdfSbufRecycle(data);
        return HDF_FAILURE;
    }
    ret = service->dispatcher->Dispatch(&service->object, SPI_IO_SET_CONFIG, data, NULL);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("SpiSetCfg: fail, ret is %d!", ret);
        HdfSbufRecycle(data);
        return HDF_ERR_IO;
    }
    HdfSbufRecycle(data);

    return HDF_SUCCESS;
}

int32_t SpiGetCfg(DevHandle handle, struct SpiCfg *cfg)
{
    int32_t ret;
    uint32_t len;
    const void *rBuf = NULL;
    struct HdfSBuf *data = NULL;
    struct HdfSBuf *reply = NULL;
    struct SpiClient *client = NULL;
    struct HdfIoService *service = NULL;

    if (handle == NULL || cfg == NULL) {
        HDF_LOGE("SpiGetCfg: handle or cfg is null!");
        return HDF_ERR_INVALID_OBJECT;
    }
    client = (struct SpiClient *)handle;

    data = HdfSbufObtainDefaultSize();
    if (data == NULL) {
        HDF_LOGE("SpiGetCfg: fail to obtain data!");
        return HDF_ERR_MALLOC_FAIL;
    }
    reply = HdfSbufObtainDefaultSize();
    if (reply == NULL) {
        HDF_LOGE("SpiGetCfg: fail to obtain reply!");
        HdfSbufRecycle(data);
        return HDF_ERR_MALLOC_FAIL;
    }
    if (!HdfSbufWriteUint32(data, client->csNum)) {
        HDF_LOGE("SpiGetCfg: write csNum fail!");
        ret = HDF_ERR_IO;
        goto EXIT;
    }
    service = (struct HdfIoService *)client->cntlr;
    if (service == NULL || service->dispatcher == NULL || service->dispatcher->Dispatch == NULL) {
        HDF_LOGE("SpiGetCfg: service is invalid!");
        ret = HDF_ERR_MALLOC_FAIL;
        goto EXIT;
    }
    ret = service->dispatcher->Dispatch(&service->object, SPI_IO_GET_CONFIG, data, reply);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("SpiGetCfg: failed, ret is %d!", ret);
        goto EXIT;
    }
    if (!HdfSbufReadBuffer(reply, &rBuf, &len) || rBuf == NULL) {
        HDF_LOGE("SpiGetCfg: read buffer fail!");
        goto EXIT;
    }
    if (memcpy_s(cfg, sizeof(struct SpiCfg), rBuf, len) != EOK) {
        HDF_LOGE("SpiGetCfg: memcpy rBuf fail!");
        ret = HDF_ERR_IO;
        goto EXIT;
    }
EXIT:
    HdfSbufRecycle(data);
    HdfSbufRecycle(reply);
    return ret;
}

void SpiClose(DevHandle handle)
{
    int32_t ret;
    struct SpiClient *client = NULL;
    struct HdfSBuf *data = NULL;
    struct HdfIoService *service = NULL;

    if (handle == NULL) {
        HDF_LOGE("SpiClose: handle is null!");
        return;
    }
    client = (struct SpiClient *)handle;

    data = HdfSbufObtainDefaultSize();
    if (data == NULL) {
        HDF_LOGE("SpiClose: fail to obtain data!");
        goto EXIT;
    }
    if (!HdfSbufWriteUint32(data, client->csNum)) {
        HDF_LOGE("SpiClose: write csNum fail!");
        goto EXIT;
    }

    service = (struct HdfIoService *)client->cntlr;
    if (service == NULL || service->dispatcher == NULL || service->dispatcher->Dispatch == NULL) {
        HDF_LOGE("SpiClose: service is invalid!");
        goto EXIT;
    }
    ret = service->dispatcher->Dispatch(&service->object, SPI_IO_CLOSE, data, NULL);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("SpiClose: failed, ret is %d!", ret);
    }
EXIT:
    HdfSbufRecycle(data);
    HdfIoServiceRecycle(service);
    OsalMemFree(client);
}

DevHandle SpiOpen(const struct SpiDevInfo *info)
{
    int32_t ret;
    struct SpiClient *client = NULL;
    struct HdfSBuf *data = NULL;
    struct HdfIoService *service = NULL;

    if (info == NULL) {
        HDF_LOGE("SpiOpen: error, info is null!");
        return NULL;
    }
    service = SpiGetCntlrByBusNum(info->busNum);
    if (service == NULL) {
        HDF_LOGE("SpiOpen: service is null");
        return NULL;
    }
    data = HdfSbufObtainDefaultSize();
    if (data == NULL) {
        HDF_LOGE("SpiOpen: fail to obtain data!");
        HdfIoServiceRecycle(service);
        return NULL;
    }
    if (!HdfSbufWriteUint32(data, info->csNum)) {
        HDF_LOGE("SpiOpen: write csNum fail!");
        HdfSbufRecycle(data);
        HdfIoServiceRecycle(service);
        return NULL;
    }

    if (service == NULL || service->dispatcher == NULL || service->dispatcher->Dispatch == NULL) {
        HDF_LOGE("SpiOpen: service is invalid!");
        HdfSbufRecycle(data);
        HdfIoServiceRecycle(service);
        return NULL;
    }
    ret = service->dispatcher->Dispatch(&service->object, SPI_IO_OPEN, data, NULL);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("SpiOpen: fail, ret is %d!", ret);
        HdfSbufRecycle(data);
        HdfIoServiceRecycle(service);
        return NULL;
    }

    client = (struct SpiClient *)OsalMemCalloc(sizeof(*client));
    if (client == NULL) {
        HDF_LOGE("SpiOpen: client malloc fail!");
        HdfSbufRecycle(data);
        HdfIoServiceRecycle(service);
        return NULL;
    }
    client->cntlr = (struct SpiCntlr *)service;
    client->csNum = info->csNum;
    HdfSbufRecycle(data);
    return (DevHandle)client;
}
