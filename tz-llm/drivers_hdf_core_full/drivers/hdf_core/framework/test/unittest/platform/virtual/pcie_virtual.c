/*
 * Copyright (c) 2020--2023 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include "device_resource_if.h"
#include "hdf_log.h"
#include "osal_mem.h"
#include "pcie_core.h"

#define HDF_LOG_TAG pcie_virtual_c

#define PCIE_VIRTUAL_ADAPTER_ONE_BYTE 1
#define PCIE_VIRTUAL_ADAPTER_TWO_BYTE 2
#define PCIE_VIRTUAL_ADAPTER_FOUR_BYTE 4
#define PCIE_VIRTUAL_ADAPTER_READ_DATA_1 0x95
#define PCIE_VIRTUAL_ADAPTER_READ_DATA_2 0x27
#define PCIE_VIRTUAL_ADAPTER_READ_DATA_3 0x89
#define PCIE_VIRTUAL_DIR_MAX             1

struct PcieVirtualAdapterHost {
    struct PcieCntlr cntlr;
    uintptr_t dmaData;
    uint32_t len;
    uint8_t dir;
    bool irqRegistered;
};

static int32_t PcieVirtualAdapterRead(struct PcieCntlr *cntlr, uint32_t mode,
    uint32_t pos, uint8_t *data, uint32_t len)
{
    (void)mode;
    (void)pos;
    if (cntlr == NULL) {
        HDF_LOGE("PcieVirtualAdapterRead: cntlr is null!");
        return HDF_ERR_INVALID_OBJECT;
    }
    if (len == PCIE_VIRTUAL_ADAPTER_ONE_BYTE) {
        *data = PCIE_VIRTUAL_ADAPTER_READ_DATA_1;
    } else if (len == PCIE_VIRTUAL_ADAPTER_TWO_BYTE) {
        *data = PCIE_VIRTUAL_ADAPTER_READ_DATA_1;
        data++;
        *data = PCIE_VIRTUAL_ADAPTER_READ_DATA_2;
    } else {
        *data = PCIE_VIRTUAL_ADAPTER_READ_DATA_1;
        data++;
        *data = PCIE_VIRTUAL_ADAPTER_READ_DATA_2;
        data++;
        *data = PCIE_VIRTUAL_ADAPTER_READ_DATA_2;
        data++;
        *data = PCIE_VIRTUAL_ADAPTER_READ_DATA_3;
    }
    return HDF_SUCCESS;
}

static int32_t PcieVirtualAdapterWrite(struct PcieCntlr *cntlr, uint32_t mode,
    uint32_t pos, uint8_t *data, uint32_t len)
{
    (void)mode;
    (void)pos;
    (void)data;
    (void)len;
    if (cntlr == NULL) {
        HDF_LOGE("PcieVirtualAdapterWrite: cntlr is null!");
        return HDF_ERR_INVALID_OBJECT;
    }
    return HDF_SUCCESS;
}

static int32_t PcieVirtualAdapterDmaMap(struct PcieCntlr *cntlr, uintptr_t addr,
    uint32_t len, uint8_t dir)
{
    struct PcieVirtualAdapterHost *host = (struct PcieVirtualAdapterHost *)cntlr;

    if (host == NULL) {
        HDF_LOGE("PcieVirtualAdapterDmaMap: host is null!");
        return HDF_ERR_INVALID_OBJECT;
    }
    if (addr == 0 || dir > PCIE_DMA_TO_DEVICE) {
        HDF_LOGE("PcieVirtualAdapterDmaMap: addr or dir is invaild!");
        return HDF_ERR_INVALID_PARAM;
    }
    if (host->dmaData != 0) {
        HDF_LOGE("PcieVirtualAdapterDmaMap: pcie dma has been mapped!");
        return HDF_ERR_DEVICE_BUSY;
    }
    host->dmaData = addr;
    host->len = len;
    host->dir = dir;

    return PcieCntlrDmaCallback(cntlr);
}

static void PcieVirtualAdapterDmaUnmap(struct PcieCntlr *cntlr, uintptr_t addr, uintptr_t len, uint8_t dir)
{
    struct PcieVirtualAdapterHost *host = (struct PcieVirtualAdapterHost *)cntlr;

    if (host == NULL || dir > PCIE_VIRTUAL_DIR_MAX) {
        HDF_LOGE("PcieVirtualAdapterDmaUnmap: host is NULL or dir is invalid!");
        return;
    }
    if (addr != host->dmaData || len != host->len || dir != host->dir) {
        HDF_LOGE("PcieVirtualAdapterDmaUnmap: invalid addr or len or dir!");
        return;
    }
    host->dmaData = 0;
    host->len = 0;
    host->dir = 0;
}

int32_t PcieVirtualRegIrq(struct PcieCntlr *cntlr)
{
    struct PcieVirtualAdapterHost *host = (struct PcieVirtualAdapterHost *)cntlr;

    if (host == NULL) {
        HDF_LOGE("PcieVirtualRegIrq: host is null!");
        return HDF_ERR_INVALID_OBJECT;
    }
    if (host->irqRegistered == true) {
        HDF_LOGE("PcieVirtualRegIrq: irq has been registered!");
        return HDF_ERR_DEVICE_BUSY;
    }
    host->irqRegistered = true;
    return PcieCntlrCallback(cntlr); // test interrupt callback access
}

void PcieVirtualUnregIrq(struct PcieCntlr *cntlr)
{
    struct PcieVirtualAdapterHost *host = (struct PcieVirtualAdapterHost *)cntlr;

    if (host == NULL) {
        HDF_LOGE("PcieVirtualUnregIrq: host is null!");
        return;
    }
    host->irqRegistered = false;
}

static struct PcieCntlrOps g_pcieVirtualAdapterHostOps = {
    .read = PcieVirtualAdapterRead,
    .write = PcieVirtualAdapterWrite,
    .dmaMap = PcieVirtualAdapterDmaMap,
    .dmaUnmap = PcieVirtualAdapterDmaUnmap,
    .registerIrq = PcieVirtualRegIrq,
    .unregisterIrq = PcieVirtualUnregIrq,
};

static int32_t PcieVirtualAdapterBind(struct HdfDeviceObject *obj)
{
    struct PcieVirtualAdapterHost *host = NULL;
    int32_t ret;

    if (obj == NULL) {
        HDF_LOGE("PcieVirtualAdapterBind: fail, device is null!");
        return HDF_ERR_INVALID_OBJECT;
    }

    host = (struct PcieVirtualAdapterHost *)OsalMemCalloc(sizeof(struct PcieVirtualAdapterHost));
    if (host == NULL) {
        HDF_LOGE("PcieVirtualAdapterBind: no mem for PcieAdapterHost!");
        return HDF_ERR_MALLOC_FAIL;
    }
    host->cntlr.ops = &g_pcieVirtualAdapterHostOps;
    host->cntlr.hdfDevObj = obj;
    obj->service = &(host->cntlr.service);

    ret = PcieCntlrParse(&(host->cntlr), obj);
    if (ret != HDF_SUCCESS) {
        goto ERR;
    }

    ret = PcieCntlrAdd(&(host->cntlr));
    if (ret != HDF_SUCCESS) {
        goto ERR;
    }

    HDF_LOGI("PcieVirtualAdapterBind: success.");
    return HDF_SUCCESS;
ERR:
    PcieCntlrRemove(&(host->cntlr));
    OsalMemFree(host);
    HDF_LOGE("PcieVirtualAdapterBind: fail, err = %d.", ret);
    return ret;
}

static int32_t PcieVirtualAdapterInit(struct HdfDeviceObject *obj)
{
    (void)obj;

    HDF_LOGI("PcieVirtualAdapterInit: success.");
    return HDF_SUCCESS;
}

static void PcieVirtualAdapterRelease(struct HdfDeviceObject *obj)
{
    struct PcieCntlr *cntlr = NULL;
    struct PcieVirtualAdapterHost *host = NULL;

    if (obj == NULL) {
        HDF_LOGE("PcieVirtualAdapterRelease: obj is null!");
        return;
    }

    cntlr = (struct PcieCntlr *)obj->service;
    if (cntlr == NULL) {
        HDF_LOGE("PcieVirtualAdapterRelease: cntlr is null!");
        return;
    }
    PcieCntlrRemove(cntlr);
    host = (struct PcieVirtualAdapterHost *)cntlr;
    OsalMemFree(host);
    HDF_LOGI("PcieVirtualAdapterRelease: success.");
}

struct HdfDriverEntry g_pcieVirtualDriverEntry = {
    .moduleVersion = 1,
    .Bind = PcieVirtualAdapterBind,
    .Init = PcieVirtualAdapterInit,
    .Release = PcieVirtualAdapterRelease,
    .moduleName = "PLATFORM_PCIE_VIRTUAL",
};
HDF_INIT(g_pcieVirtualDriverEntry);
