/*
 * Copyright (c) 2021-2023 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include "pcie_if.h"
#include "devsvc_manager_clnt.h"
#include "pcie_core.h"
#include "hdf_base.h"
#include "hdf_log.h"
#include "osal_mem.h"

#define HDF_LOG_TAG pcie_if_c

#define PCIE_SERVICE_NAME_LEN 32

static struct PcieCntlr *PcieCntlrObjGet(uint16_t busNum)
{
    char serviceName[PCIE_SERVICE_NAME_LEN + 1] = {0};
    struct PcieCntlr *obj = NULL;

    if (snprintf_s(serviceName, (PCIE_SERVICE_NAME_LEN + 1),
        PCIE_SERVICE_NAME_LEN, "HDF_PLATFORM_PCIE_%hu", busNum) < 0) {
        HDF_LOGE("PcieCntlrObjGet: get PCIE service name fail.");
        return NULL;
    }
    obj = PcieCntlrGetByBusNum(busNum);
    return obj;
}

DevHandle PcieOpen(uint16_t busNum)
{
    return (DevHandle)PcieCntlrObjGet(busNum);
}

int32_t PcieRead(DevHandle handle, uint32_t mode, uint32_t pos, uint8_t *data, uint32_t len)
{
    return PcieCntlrRead((struct PcieCntlr *)handle, mode, pos, data, len);
}

int32_t PcieWrite(DevHandle handle, uint32_t mode, uint32_t pos, uint8_t *data, uint32_t len)
{
    return PcieCntlrWrite((struct PcieCntlr *)handle, mode, pos, data, len);
}

int32_t PcieDmaMap(DevHandle handle, PcieCallbackFunc cb, uintptr_t addr, uint32_t len, uint8_t dir)
{
    return PcieCntlrDmaMap((struct PcieCntlr *)handle, cb, addr, len, dir);
}

void PcieDmaUnmap(DevHandle handle, uintptr_t addr, uint32_t len, uint8_t dir)
{
    PcieCntlrDmaUnmap((struct PcieCntlr *)handle, addr, len, dir);
}

int32_t PcieRegisterIrq(DevHandle handle, PcieCallbackFunc cb)
{
    return PcieCntlrRegisterIrq((struct PcieCntlr *)handle, cb);
}

void PcieUnregisterIrq(DevHandle handle)
{
    PcieCntlrUnregisterIrq((struct PcieCntlr *)handle);
}

void PcieClose(DevHandle handle)
{
    (void)handle;
}
