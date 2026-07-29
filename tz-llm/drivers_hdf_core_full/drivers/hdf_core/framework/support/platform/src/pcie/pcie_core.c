/*
 * Copyright (c) 2021-2023 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include "pcie_core.h"
#include "device_resource_if.h"
#include "hdf_log.h"
#include "osal_mem.h"
#include "pcie_dispatch.h"
#include "securec.h"

#define HDF_LOG_TAG pcie_core_c

static int32_t PcieCntlrInit(struct PcieCntlr *cntlr)
{
    int32_t ret;

    if (cntlr == NULL || cntlr->hdfDevObj == NULL) {
        HDF_LOGE("PcieCntlrInit: cntlr or hdfDevObj is null!");
        return HDF_ERR_INVALID_OBJECT;
    }

    ret = OsalSpinInit(&cntlr->spin);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("PcieCntlrInit: spin init fail!");
        return ret;
    }

    cntlr->service.Dispatch = PcieIoDispatch;
    cntlr->hdfDevObj->service = &(cntlr->service);
    cntlr->device.number = (int32_t)cntlr->devInfo.busNum;
    cntlr->device.hdfDev = cntlr->hdfDevObj;
    return HDF_SUCCESS;
}

static inline void PcieCntlrUninit(struct PcieCntlr *cntlr)
{
    if (cntlr != NULL) {
        (void)OsalSpinDestroy(&cntlr->spin);
    }
}

int32_t PcieCntlrAdd(struct PcieCntlr *cntlr)
{
    int32_t ret;

    if (cntlr == NULL) {
        HDF_LOGE("PcieCntlrAdd: invalid param!");
        return HDF_ERR_INVALID_OBJECT;
    }

    ret = PcieCntlrInit(cntlr);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("PcieCntlrAdd: pcie cntlr init fail!");
        return ret;
    }

    cntlr->device.manager = PlatformManagerGet(PLATFORM_MODULE_PCIE);
    ret = PlatformDeviceAdd(&cntlr->device);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("PcieCntlrAdd: device add fail!");
        PcieCntlrUninit(cntlr);
        return ret;
    }
    return HDF_SUCCESS;
}

void PcieCntlrRemove(struct PcieCntlr *cntlr)
{
    if (cntlr != NULL) {
        PlatformDeviceDel(&cntlr->device);
        PcieCntlrUninit(cntlr);
    }
}

int32_t PcieCntlrParse(struct PcieCntlr *cntlr, struct HdfDeviceObject *obj)
{
    const struct DeviceResourceNode *node = NULL;
    struct DeviceResourceIface *drsOps = NULL;
    int32_t ret;

    if (obj == NULL || cntlr == NULL) {
        HDF_LOGE("PcieCntlrParse: input param is null!");
        return HDF_FAILURE;
    }

    node = obj->property;
    if (node == NULL) {
        HDF_LOGE("PcieCntlrParse: drs node is null!");
        return HDF_FAILURE;
    }
    drsOps = DeviceResourceGetIfaceInstance(HDF_CONFIG_SOURCE);
    if (drsOps == NULL || drsOps->GetUint32 == NULL) {
        HDF_LOGE("PcieCntlrParse: invalid drs ops fail.");
        return HDF_FAILURE;
    }

    ret = drsOps->GetUint16(node, "busNum", &(cntlr->devInfo.busNum), 0);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("PcieCntlrParse: read busNum fail!");
        return ret;
    }

    ret = drsOps->GetUint32(node, "vendorId", &(cntlr->devInfo.vendorId), 0);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("PcieCntlrParse: read vendorId fail!");
        return HDF_FAILURE;
    }

    ret = drsOps->GetUint32(node, "devId", &(cntlr->devInfo.devId), 0);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("PcieCntlrParse: read devId fail!");
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

int32_t PcieCntlrRead(struct PcieCntlr *cntlr, uint32_t mode, uint32_t pos, uint8_t *data, uint32_t len)
{
    int32_t ret;

    if (cntlr == NULL || cntlr->ops == NULL || cntlr->ops->read == NULL) {
        HDF_LOGE("PcieCntlrRead: invalid cntlr");
        return HDF_ERR_INVALID_OBJECT;
    }
    if (data == NULL || len == 0 || mode > PCIE_IO) {
        HDF_LOGE("PcieCntlrRead: invalid param");
        return HDF_ERR_INVALID_PARAM;
    }

    PcieCntlrLock(cntlr);
    ret = cntlr->ops->read(cntlr, mode, pos, data, len);
    PcieCntlrUnlock(cntlr);
    return ret;
}

int32_t PcieCntlrWrite(struct PcieCntlr *cntlr, uint32_t mode, uint32_t pos, uint8_t *data, uint32_t len)
{
    int32_t ret;
    if (cntlr == NULL || cntlr->ops == NULL || cntlr->ops->write == NULL) {
        HDF_LOGE("PcieCntlrWrite: invalid cntlr");
        return HDF_ERR_INVALID_OBJECT;
    }
    if (data == NULL || len == 0 || mode > PCIE_IO) {
        HDF_LOGE("PcieCntlrWrite: invalid cb");
        return HDF_ERR_INVALID_PARAM;
    }

    PcieCntlrLock(cntlr);
    ret = cntlr->ops->write(cntlr, mode, pos, data, len);
    PcieCntlrUnlock(cntlr);
    return ret;
}

int32_t PcieCntlrDmaMap(struct PcieCntlr *cntlr, PcieCallbackFunc cb, uintptr_t addr, uint32_t len, uint8_t dir)
{
    int32_t ret;

    if (cntlr == NULL || cntlr->ops == NULL || cntlr->ops->write == NULL) {
        HDF_LOGE("PcieCntlrDmaMap: invalid param");
        return HDF_ERR_INVALID_OBJECT;
    }
    if (addr == 0 || cb == NULL || len == 0) {
        HDF_LOGE("PcieCntlrDmaMap: invalid param");
        return HDF_ERR_INVALID_PARAM;
    }

    PcieCntlrLock(cntlr);
    if (cntlr->dmaCb != NULL) {
        HDF_LOGE("PcieCntlrDmaMap: irq has been registered");
        PcieCntlrUnlock(cntlr);
        return HDF_FAILURE;
    }
    cntlr->dmaCb = cb;
    ret = cntlr->ops->dmaMap(cntlr, addr, len, dir);
    PcieCntlrUnlock(cntlr);
    return ret;
}

void PcieCntlrDmaUnmap(struct PcieCntlr *cntlr, uintptr_t addr, uint32_t len, uint8_t dir)
{
    if (cntlr == NULL) {
        HDF_LOGE("PcieCntlrDmaUnmap: invalid param");
        return;
    }
    PcieCntlrLock(cntlr);
    cntlr->ops->dmaUnmap(cntlr, addr, len, dir);
    cntlr->dmaCb = NULL;
    PcieCntlrUnlock(cntlr);
}

int32_t PcieCntlrRegisterIrq(struct PcieCntlr *cntlr, PcieCallbackFunc cb)
{
    if (cntlr == NULL || cntlr->ops == NULL || cb == NULL) {
        HDF_LOGE("PcieCntlrRegisterIrq: invalid param");
        return HDF_ERR_INVALID_OBJECT;
    }

    PcieCntlrLock(cntlr);
    if (cntlr->cb != NULL) {
        HDF_LOGE("PcieCntlrRegisterIrq: irq has been registered");
        PcieCntlrUnlock(cntlr);
        return HDF_FAILURE;
    }
    cntlr->cb = cb;
    if (cntlr->ops->registerIrq != NULL) {
        cntlr->ops->registerIrq(cntlr);
    }
    PcieCntlrUnlock(cntlr);
    return HDF_SUCCESS;
}

void PcieCntlrUnregisterIrq(struct PcieCntlr *cntlr)
{
    if (cntlr == NULL || cntlr->ops == NULL) {
        HDF_LOGE("PcieCntlrUnregisterIrq: invalid param");
        return;
    }
    PcieCntlrLock(cntlr);
    if (cntlr->ops->unregisterIrq != NULL) {
        cntlr->ops->unregisterIrq(cntlr);
    }

    cntlr->cb = NULL;
    PcieCntlrUnlock(cntlr);
}

int32_t PcieCntlrCallback(struct PcieCntlr *cntlr)
{
    if (cntlr == NULL) {
        HDF_LOGE("PcieCntlrCallback: cntlr is null!");
        return HDF_ERR_INVALID_OBJECT;
    }
    if (cntlr->cb != NULL) {
        return cntlr->cb((DevHandle)cntlr);
    }
    return HDF_SUCCESS;
}

int32_t PcieCntlrDmaCallback(struct PcieCntlr *cntlr)
{
    if (cntlr == NULL) {
        HDF_LOGE("PcieCntlrDmaCallback: cntlr is null!");
        return HDF_ERR_INVALID_OBJECT;
    }
    if (cntlr->dmaCb != NULL) {
        return cntlr->dmaCb((DevHandle)cntlr);
    }
    return HDF_SUCCESS;
}
