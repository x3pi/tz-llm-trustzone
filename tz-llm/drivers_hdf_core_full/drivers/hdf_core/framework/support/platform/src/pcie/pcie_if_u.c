/*
 * Copyright (c) 2021-2023 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include "pcie_if.h"
#include "hdf_base.h"
#include "hdf_io_service_if.h"
#include "hdf_log.h"
#include "osal_mem.h"
#include "platform_listener_u.h"
#include "securec.h"

#define HDF_LOG_TAG pcie_if_u_c

#define PCIE_SERVICE_NAME_LEN 32
#define IRQ_CB_NUM            0
#define DMA_CB_NUM            1

enum PcieIoCmd {
    PCIE_CMD_READ = 0,
    PCIE_CMD_WRITE,
    PCIE_CMD_DMA_MAP,
    PCIE_CMD_DMA_UNMAP,
    PCIE_CMD_REG_IRQ,
    PCIE_CMD_UNREG_IRQ,
    PCIE_CMD_BUTT,
};

struct DmaData {
    uintptr_t addr;
    uint32_t len;
    uint8_t dir;
};

DevHandle PcieOpen(uint16_t busNum)
{
    struct HdfIoService *service = NULL;
    char serviceName[PCIE_SERVICE_NAME_LEN + 1];

    if (snprintf_s(serviceName, (PCIE_SERVICE_NAME_LEN + 1),
        PCIE_SERVICE_NAME_LEN, "HDF_PLATFORM_PCIE_%hu", busNum) < 0) {
        HDF_LOGE("PcieOpen: get PCIE service name fail.");
        return NULL;
    }

    service = HdfIoServiceBind(serviceName);
    if (service->priv == NULL) {
        struct PlatformUserListenerManager *manager = PlatformUserListenerManagerGet(PLATFORM_MODULE_PCIE);
        if (manager == NULL) {
            HDF_LOGE("PcieOpen: PlatformUserListenerManagerGet fail!");
            HdfIoServiceRecycle(service);
            return NULL;
        }
        service->priv = manager;
        manager->service = service;
    }
    return (DevHandle)service;
}

static int32_t PcieGetDataFromReply(struct HdfSBuf *reply, uint8_t *data, uint32_t size)
{
    uint32_t rLen;
    const void *rBuf = NULL;

    if (HdfSbufReadBuffer(reply, &rBuf, &rLen) == false) {
        HDF_LOGE("PcieGetDataFromReply: read rBuf fail!");
        return HDF_ERR_IO;
    }
    if (size != rLen) {
        HDF_LOGE("PcieGetDataFromReply: err len:%u, rLen:%u!", size, rLen);
        if (rLen > size) {
            rLen = size;
        }
    }

    if (memcpy_s(data, size, rBuf, rLen) != EOK) {
        HDF_LOGE("PcieGetDataFromReply: memcpy rBuf fail!");
        return HDF_ERR_IO;
    }
    return HDF_SUCCESS;
}

int32_t PcieRead(DevHandle handle, uint32_t mode, uint32_t pos, uint8_t *data, uint32_t len)
{
    int32_t ret;
    struct HdfSBuf *reply = NULL;
    struct HdfIoService *service = (struct HdfIoService *)handle;

    if (service == NULL || service->dispatcher == NULL || service->dispatcher->Dispatch == NULL) {
        HDF_LOGE("PcieRead: service is invalid");
        return HDF_ERR_INVALID_OBJECT;
    }
    if (data == NULL || mode > PCIE_IO || len == 0) {
        HDF_LOGE("PcieRead: invalid parameter");
        return HDF_ERR_INVALID_PARAM;
    }

    struct HdfSBuf *buf = HdfSbufObtainDefaultSize();
    if (buf == NULL) {
        HDF_LOGE("PcieRead: fail to obtain buf!");
        return HDF_ERR_MALLOC_FAIL;
    }
    do {
        if (!HdfSbufWriteUint32(buf, mode)) {
            HDF_LOGE("PcieRead: sbuf write uint32 fail!");
            ret = HDF_ERR_IO;
            break;
        }
        if (!HdfSbufWriteUint32(buf, len)) {
            HDF_LOGE("PcieRead: sbuf write uint32 fail!");
            ret = HDF_ERR_IO;
            break;
        }
        if (!HdfSbufWriteUint32(buf, pos)) {
            HDF_LOGE("PcieRead: sbuf write uint64 fail!");
            ret = HDF_ERR_IO;
            break;
        }

        reply = HdfSbufObtainDefaultSize();
        if (reply == NULL) {
            HDF_LOGE("PcieRead: fail to obtain reply!");
            ret = HDF_ERR_MALLOC_FAIL;
            break;
        }

        ret = service->dispatcher->Dispatch(&service->object, PCIE_CMD_READ, buf, reply);
        if (ret != HDF_SUCCESS) {
            HDF_LOGE("PcieRead: fail to write, ret: %d!", ret);
        } else {
            ret = PcieGetDataFromReply(reply, data, len);
        }
    } while (0);

    HdfSbufRecycle(reply);
    HdfSbufRecycle(buf);
    return ret;
}

int32_t PcieWrite(DevHandle handle, uint32_t mode, uint32_t pos, uint8_t *data, uint32_t len)
{
    int32_t ret;
    struct HdfSBuf *buf = NULL;
    struct HdfIoService *service = (struct HdfIoService *)handle;

    if (service == NULL || service->dispatcher == NULL || service->dispatcher->Dispatch == NULL) {
        HDF_LOGE("PcieWrite: service is invalid");
        return HDF_ERR_INVALID_OBJECT;
    }
    if (data == NULL || mode > PCIE_IO || len == 0) {
        HDF_LOGE("PcieWrite: invalid parameter");
        return HDF_ERR_INVALID_PARAM;
    }
    buf = HdfSbufObtainDefaultSize();
    if (buf == NULL) {
        HDF_LOGE("PcieWrite: fail to obtain buf!");
        return HDF_ERR_MALLOC_FAIL;
    }
    if (!HdfSbufWriteUint32(buf, mode)) {
        HDF_LOGE("PcieWrite: sbuf write mode fail!");
        HdfSbufRecycle(buf);
        return HDF_ERR_IO;
    }
    if (!HdfSbufWriteUint32(buf, len)) {
        HDF_LOGE("PcieWrite: sbuf write len fail!");
        HdfSbufRecycle(buf);
        return HDF_ERR_IO;
    }
    if (!HdfSbufWriteUint32(buf, pos)) {
        HDF_LOGE("PcieWrite: sbuf write pos fail!");
        HdfSbufRecycle(buf);
        return HDF_ERR_IO;
    }
    if (!HdfSbufWriteBuffer(buf, data, len)) {
        HDF_LOGE("PcieWrite: sbuf write buffer fail!");
        HdfSbufRecycle(buf);
        return HDF_ERR_IO;
    }

    ret = service->dispatcher->Dispatch(&service->object, PCIE_CMD_WRITE, buf, NULL);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("PcieWrite: fail to write, ret: %d!", ret);
    }

    HdfSbufRecycle(buf);
    return ret;
}

static int32_t PcieRegListener(
    struct HdfIoService *service, PcieCallbackFunc cb, uint32_t num, const struct DmaData *dma)
{
    struct PlatformUserListenerPcieParam *param = NULL;

    param = OsalMemCalloc(sizeof(struct PlatformUserListenerPcieParam));
    if (param == NULL) {
        HDF_LOGE("PcieRegListener: OsalMemCalloc param fail!");
        return HDF_ERR_IO;
    }
    param->handle = (DevHandle)service;
    param->func = cb;
    if (num == DMA_CB_NUM) {
        param->dmaData = dma->addr;
        param->len = dma->len;
        param->dir = dma->dir;
    }

    if (PlatformUserListenerReg((struct PlatformUserListenerManager *)service->priv, num, (void *)param,
        PcieOnDevEventReceive) != HDF_SUCCESS) {
        HDF_LOGE("PcieRegListener: PlatformUserListenerReg fail!");
        OsalMemFree(param);
        return HDF_ERR_IO;
    }

    return HDF_SUCCESS;
}

static int32_t DmaToDevice(
    DevHandle handle, PcieCallbackFunc cb, struct HdfSBuf *data, uintptr_t addr, uint32_t len)
{
    if (!HdfSbufWriteUint8(data, PCIE_DMA_TO_DEVICE)) {
        HDF_LOGE("DmaToDevice: sbuf write dir fail!");
        return HDF_ERR_IO;
    }
    if (!HdfSbufWriteUint32(data, len)) {
        HDF_LOGE("DmaToDevice: sbuf write len fail!");
        return HDF_ERR_IO;
    }
    if (!HdfSbufWriteBuffer(data, (const void *)addr, len)) {
        HDF_LOGE("DmaToDevice: sbuf write buffer fail!");
        return HDF_ERR_IO;
    }

    const struct DmaData dma = {
        .addr = addr,
        .len = len,
        .dir = PCIE_DMA_TO_DEVICE,
    };
    return PcieRegListener((struct HdfIoService *)handle, cb, DMA_CB_NUM, &dma);
}

static int32_t DeviceToDma(
    DevHandle handle, PcieCallbackFunc cb, struct HdfSBuf *data, uintptr_t addr, uint32_t len)
{
    if (!HdfSbufWriteUint8(data, PCIE_DMA_FROM_DEVICE)) {
        HDF_LOGE("DeviceToDma: sbuf write dir fail!");
        return HDF_ERR_IO;
    }
    if (!HdfSbufWriteUint32(data, len)) {
        HDF_LOGE("DeviceToDma: sbuf write len fail!");
        return HDF_ERR_IO;
    }

    const struct DmaData dma = {
        .addr = addr,
        .len = len,
        .dir = PCIE_DMA_FROM_DEVICE,
    };
    return PcieRegListener((struct HdfIoService *)handle, cb, DMA_CB_NUM, &dma);
}

int32_t PcieDmaMap(DevHandle handle, PcieCallbackFunc cb, uintptr_t addr, uint32_t len, uint8_t dir)
{
    int32_t ret;
    struct HdfSBuf *data = NULL;
    struct HdfIoService *service = (struct HdfIoService *)handle;

    if (service == NULL || service->dispatcher == NULL || service->dispatcher->Dispatch == NULL) {
        HDF_LOGE("PcieDmaMap: service is invalid!");
        return HDF_ERR_INVALID_OBJECT;
    }

    if (addr == 0) {
        HDF_LOGE("PcieDmaMap: invalid addr!");
        return HDF_ERR_INVALID_PARAM;
    }

    data = HdfSbufObtainDefaultSize();
    if (data == NULL) {
        HDF_LOGE("PcieDmaMap: fail to obtain data!");
        return HDF_ERR_MALLOC_FAIL;
    }
    if (dir == PCIE_DMA_FROM_DEVICE) {
        ret = DeviceToDma(handle, cb, data, addr, len);
    } else if (dir == PCIE_DMA_TO_DEVICE) {
        ret = DmaToDevice(handle, cb, data, addr, len);
    } else {
        HDF_LOGE("PcieDmaMap: invalid dir");
        HdfSbufRecycle(data);
        return HDF_ERR_INVALID_PARAM;
    }
    if (ret != HDF_SUCCESS) {
        HdfSbufRecycle(data);
        HDF_LOGE("PcieDmaMap: failed to map DMA, ret: %d!", ret);
        return ret;
    }

    ret = service->dispatcher->Dispatch(&service->object, PCIE_CMD_DMA_MAP, data, NULL);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("PcieDmaMap: fail, ret: %d!", ret);
        PlatformUserListenerDestory(service->priv, DMA_CB_NUM);
    }

    HdfSbufRecycle(data);
    return ret;
}

void PcieDmaUnmap(DevHandle handle, uintptr_t addr, uint32_t len, uint8_t dir)
{
    int32_t ret;
    struct HdfIoService *service = (struct HdfIoService *)handle;
    struct HdfSBuf *data = NULL;

    (void)addr;
    (void)len;
    if (service == NULL || service->dispatcher == NULL || service->dispatcher->Dispatch == NULL) {
        HDF_LOGE("PcieDmaUnmap: service is invalid");
        return;
    }

    PlatformUserListenerDestory(service->priv, DMA_CB_NUM);
    data = HdfSbufObtainDefaultSize();
    if (data == NULL) {
        HDF_LOGE("PcieDmaUnmap: fail to obtain data!");
        return;
    }
    if (!HdfSbufWriteUint8(data, dir)) {
        HDF_LOGE("PcieDmaUnmap: sbuf write dir fail!");
        HdfSbufRecycle(data);
        return;
    }

    ret = service->dispatcher->Dispatch(&service->object, PCIE_CMD_DMA_UNMAP, data, NULL);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("PcieDmaUnmap: fail, ret: %d!", ret);
    }
    HdfSbufRecycle(data);
    return;
}

int32_t PcieRegisterIrq(DevHandle handle, PcieCallbackFunc cb)
{
    int32_t ret;
    struct HdfIoService *service = (struct HdfIoService *)handle;

    if (service == NULL || service->dispatcher == NULL || service->dispatcher->Dispatch == NULL) {
        HDF_LOGE("PcieRegisterIrq: service is invalid");
        return HDF_ERR_INVALID_OBJECT;
    }

    ret = PcieRegListener(service, cb, IRQ_CB_NUM, NULL);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("PcieRegisterIrq: register listener fail, ret: %d!", ret);
        return ret;
    }
    ret = service->dispatcher->Dispatch(&service->object, PCIE_CMD_REG_IRQ, NULL, NULL);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("PcieRegisterIrq: fail, ret: %d!", ret);
    }

    return ret;
}

void PcieUnregisterIrq(DevHandle handle)
{
    int32_t ret;
    struct HdfIoService *service = (struct HdfIoService *)handle;

    if (service == NULL || service->dispatcher == NULL || service->dispatcher->Dispatch == NULL) {
        HDF_LOGE("PcieUnregisterIrq: service is invalid");
        return;
    }

    PlatformUserListenerDestory(service->priv, IRQ_CB_NUM);
    ret = service->dispatcher->Dispatch(&service->object, PCIE_CMD_UNREG_IRQ, NULL, NULL);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("PcieUnregisterIrq: fail, ret: %d!", ret);
    }

    return;
}

void PcieClose(DevHandle handle)
{
    struct HdfIoService *service = (struct HdfIoService *)handle;

    if (service != NULL) {
        if (service->priv != NULL) {
            PlatformUserListenerManagerDestory((struct PlatformUserListenerManager *)service->priv);
        }
        HdfIoServiceRecycle(service);
    }
}
