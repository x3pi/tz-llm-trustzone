/*
 * Copyright (c) 2021-2023 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include "pcie_dispatch.h"
#include "hdf_log.h"
#include "osal_mem.h"
#include "pcie_core.h"
#include "pcie_if.h"
#include "platform_listener_common.h"

#define HDF_LOG_TAG pcie_dispatch_c
#define IRQ_CB_NUM      0
#define DMA_CB_NUM      1
#define USER_LEM_MAX    4096
#define DMA_ALIGN_SIZE  256

enum PcieIoCmd {
    PCIE_CMD_READ = 0,
    PCIE_CMD_WRITE,
    PCIE_CMD_DMA_MAP,
    PCIE_CMD_DMA_UNMAP,
    PCIE_CMD_REG_IRQ,
    PCIE_CMD_UNREG_IRQ,
    PCIE_CMD_BUTT,
};

static int32_t PcieCmdRead(struct PcieCntlr *cntlr, struct HdfSBuf *data, struct HdfSBuf *reply)
{
    uint32_t mode;
    uint32_t len;
    uint32_t pos;
    uint8_t *buf = NULL;
    int32_t ret;

    if (!HdfSbufReadUint32(data, &mode)) {
        HDF_LOGE("PcieCmdRead: read mode fail");
        return HDF_ERR_IO;
    }
    if (!HdfSbufReadUint32(data, &len)) {
        HDF_LOGE("PcieCmdRead: read len fail");
        return HDF_ERR_IO;
    }
    if (len == 0 ||  len > USER_LEM_MAX) {
        HDF_LOGE("PcieCmdRead: invalid len");
        return HDF_ERR_INVALID_PARAM;
    }
    if (!HdfSbufReadUint32(data, &pos)) {
        HDF_LOGE("PcieCmdRead: read pos fail");
        return HDF_ERR_IO;
    }

    buf = (uint8_t *)OsalMemCalloc(sizeof(*buf) * len);
    if (buf == NULL) {
        HDF_LOGE("PcieCmdRead: OsalMemCalloc error");
        return HDF_ERR_MALLOC_FAIL;
    }

    ret = PcieCntlrRead(cntlr, mode, pos, buf, len);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("PcieCmdRead: error, ret is %d", ret);
        OsalMemFree(buf);
        return ret;
    }
    if (!HdfSbufWriteBuffer(reply, buf, len)) {
        HDF_LOGE("PcieCmdRead: sbuf write buffer fail!");
        OsalMemFree(buf);
        return HDF_ERR_IO;
    }

    OsalMemFree(buf);
    return HDF_SUCCESS;
}

static int32_t PcieCmdWrite(struct PcieCntlr *cntlr, struct HdfSBuf *data)
{
    uint32_t pos;
    uint32_t len;
    uint32_t size;
    uint32_t mode;
    uint8_t *buf = NULL;

    if (!HdfSbufReadUint32(data, &mode)) {
        HDF_LOGE("PcieCmdWrite: read pos fail");
        return HDF_ERR_IO;
    }
    if (!HdfSbufReadUint32(data, &len)) {
        HDF_LOGE("PcieCmdWrite: read pos fail");
        return HDF_ERR_IO;
    }
    if (!HdfSbufReadUint32(data, &pos)) {
        HDF_LOGE("PcieCmdWrite: read pos fail");
        return HDF_ERR_IO;
    }
    if (!HdfSbufReadBuffer(data, (const void **)&buf, &size)) {
        HDF_LOGE("PcieCmdWrite: sbuf read buffer fail!");
        return HDF_ERR_IO;
    }
    if (len == 0 || len != size) {
        HDF_LOGE("PcieCmdWrite: read sbuf error");
        return HDF_ERR_IO;
    }
    return PcieCntlrWrite(cntlr, mode, pos, buf, len);
}

static int32_t PcieIoDmaCb(DevHandle handle)
{
    struct HdfSBuf *data = NULL;
    struct PcieCntlr *cntlr = (struct PcieCntlr *)handle;
    int32_t ret;

    if (cntlr == NULL) {
        HDF_LOGE("PcieIoDmaCb: cntlr is null!");
        return HDF_ERR_INVALID_PARAM;
    }

    data = HdfSbufObtainDefaultSize();
    if (data == NULL) {
        HDF_LOGE("PcieIoDmaCb: fail to obtain data!");
        return HDF_ERR_MALLOC_FAIL;
    }
    if (!HdfSbufWriteUint32(data, DMA_CB_NUM)) {
        HDF_LOGE("PcieIoDmaCb: sbuf write num fail!");
        HdfSbufRecycle(data);
        return HDF_ERR_IO;
    }
    if (cntlr->dir == PCIE_DMA_FROM_DEVICE) {
        if (!HdfSbufWriteBuffer(data, (const void *)cntlr->dmaData, cntlr->len)) {
            HDF_LOGE("PcieIoDmaCb: sbuf write buffer fail!");
            HdfSbufRecycle(data);
            return HDF_ERR_IO;
        }
    }
    ret = HdfDeviceSendEvent(cntlr->hdfDevObj, PLATFORM_LISTENER_EVENT_PCIE_NOTIFY, data);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("PcieIoDmaCb: send event fail");
    }

    HdfSbufRecycle(data);
    return ret;
}

static int32_t DmaToDevice(struct PcieCntlr *cntlr, struct HdfSBuf *data, uint32_t len)
{
    uint8_t *buf = NULL;
    uint32_t size;

    if (!HdfSbufReadBuffer(data, (const void **)&buf, &size) || size != len) {
        HDF_LOGE("DmaToDevice: sbuf read buffer fail!");
        return HDF_ERR_IO;
    }
    cntlr->dmaData = (uintptr_t)buf;
    cntlr->len = len;
    cntlr->dir = PCIE_DMA_TO_DEVICE;

    return PcieCntlrDmaMap(cntlr, PcieIoDmaCb, (uintptr_t)buf, len, PCIE_DMA_TO_DEVICE);
}

static int32_t DeviceToDma(struct PcieCntlr *cntlr, uint32_t len)
{
    uint8_t *addr = NULL;

    addr = OsalMemAllocAlign(DMA_ALIGN_SIZE, len);
    if (addr == NULL) {
        HDF_LOGE("DeviceToDma: malloc fail");
        return HDF_ERR_MALLOC_FAIL;
    }
    cntlr->dmaData = (uintptr_t)addr;
    cntlr->len = len;
    cntlr->dir = PCIE_DMA_FROM_DEVICE;

    return PcieCntlrDmaMap(cntlr, PcieIoDmaCb, (uintptr_t)addr, len, PCIE_DMA_FROM_DEVICE);
}

static int32_t PcieCmdDmaMap(struct PcieCntlr *cntlr, struct HdfSBuf *data)
{
    int32_t ret;
    uint32_t len;
    uint8_t dir;

    if (!HdfSbufReadUint8(data, &dir)) {
        HDF_LOGE("PcieCmdDmaMap: read dir fail");
        return HDF_ERR_IO;
    }
    if (!HdfSbufReadUint32(data, &len)) {
        HDF_LOGE("PcieCmdDmaMap: read len fail");
        return HDF_ERR_IO;
    }
    if (len == 0 || len > USER_LEM_MAX) {
        HDF_LOGE("PcieCmdDmaMap: invalid len");
        return HDF_ERR_INVALID_PARAM;
    }
    if (cntlr->dmaData != 0 || cntlr->len != 0) {
        return HDF_ERR_DEVICE_BUSY;
    }
    if (dir == PCIE_DMA_FROM_DEVICE) {
        ret = DeviceToDma(cntlr, len);
    } else if (dir == PCIE_DMA_TO_DEVICE) {
        ret = DmaToDevice(cntlr, data, len);
    } else {
        HDF_LOGE("PcieCmdDmaMap: invalid dir");
        ret = HDF_ERR_INVALID_PARAM;
    }

    return ret;
}

static int32_t PcieCmdDmaUnmap(struct PcieCntlr *cntlr, struct HdfSBuf *data)
{
    uint8_t dir;

    if (!HdfSbufReadUint8(data, &dir) || dir != cntlr->dir) {
        HDF_LOGE("PcieCmdDmaUnmap: read dir fail");
        return HDF_ERR_IO;
    }

    PcieCntlrDmaUnmap(cntlr, cntlr->dmaData, cntlr->len, dir);
    OsalMemFree((void *)cntlr->dmaData);
    cntlr->dmaData = 0;
    cntlr->len = 0;

    return HDF_SUCCESS;
}

static int32_t PcieIoCallback(DevHandle handle)
{
    struct HdfSBuf *data = NULL;
    struct PcieCntlr *cntlr = (struct PcieCntlr *)handle;
    int32_t ret;

    if (cntlr == NULL) {
        HDF_LOGE("PcieIoCallback: cntlr is null!");
        return HDF_ERR_INVALID_PARAM;
    }

    data = HdfSbufObtainDefaultSize();
    if (data == NULL) {
        HDF_LOGE("PcieIoCallback: fail to obtain data!");
        return HDF_ERR_MALLOC_FAIL;
    }
    if (!HdfSbufWriteUint32(data, IRQ_CB_NUM)) {
        HDF_LOGE("PcieIoCallback: sbuf write num fail!");
        HdfSbufRecycle(data);
        return HDF_ERR_IO;
    }

    ret = HdfDeviceSendEvent(cntlr->hdfDevObj, PLATFORM_LISTENER_EVENT_PCIE_NOTIFY, data);
    HdfSbufRecycle(data);
    return ret;
}

static int32_t PcieCmdRegisterIrq(struct PcieCntlr *cntlr)
{
    return PcieCntlrRegisterIrq(cntlr, PcieIoCallback);
}

static int32_t PcieCmdUnregisterIrq(struct PcieCntlr *cntlr)
{
    PcieCntlrUnregisterIrq(cntlr);
    return HDF_SUCCESS;
}

int32_t PcieIoDispatch(struct HdfDeviceIoClient *client, int32_t cmd, struct HdfSBuf *data, struct HdfSBuf *reply)
{
    struct PcieCntlr *cntlr = NULL;

    if (client == NULL || client->device == NULL) {
        HDF_LOGE("PcieIoDispatch: client or hdf dev obj is null!");
        return HDF_ERR_INVALID_OBJECT;
    }

    cntlr = (struct PcieCntlr *)client->device->service;
    if (cntlr == NULL) {
        HDF_LOGE("PcieIoDispatch: service is null!");
        return HDF_ERR_INVALID_OBJECT;
    }

    switch (cmd) {
        case PCIE_CMD_READ:
            return PcieCmdRead(cntlr, data, reply);
        case PCIE_CMD_WRITE:
            return PcieCmdWrite(cntlr, data);
        case PCIE_CMD_DMA_MAP:
            return PcieCmdDmaMap(cntlr, data);
        case PCIE_CMD_DMA_UNMAP:
            return PcieCmdDmaUnmap(cntlr, data);
        case PCIE_CMD_REG_IRQ:
            return PcieCmdRegisterIrq(cntlr);
        case PCIE_CMD_UNREG_IRQ:
            return PcieCmdUnregisterIrq(cntlr);
        default:
            HDF_LOGE("PcieIoDispatch: cmd %d is not support", cmd);
            return HDF_ERR_NOT_SUPPORT;
    }

    return HDF_SUCCESS;
}
