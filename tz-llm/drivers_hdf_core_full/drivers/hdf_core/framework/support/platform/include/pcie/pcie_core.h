/*
 * Copyright (c) 2021-2023 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#ifndef PCIE_CORE_H
#define PCIE_CORE_H

#include "hdf_base.h"
#include "hdf_device_desc.h"
#include "osal_mutex.h"
#include "pcie_if.h"
#include "platform_core.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* __cplusplus */

struct PcieCntlr;

struct PcieCntlrOps {
    int32_t (*read)(struct PcieCntlr *cntlr, uint32_t mode, uint32_t pos, uint8_t *data, uint32_t len);
    int32_t (*write)(struct PcieCntlr *cntlr, uint32_t mode, uint32_t pos, uint8_t *data, uint32_t len);
    int32_t (*dmaMap)(struct PcieCntlr *cntlr, uintptr_t addr, uint32_t len, uint8_t dir);
    void (*dmaUnmap)(struct PcieCntlr *cntlr, uintptr_t addr, uint32_t len, uint8_t dir);
    int32_t (*registerIrq)(struct PcieCntlr *cntlr);
    void (*unregisterIrq)(struct PcieCntlr *cntlr);
};

struct PcieDevCfgInfo {
    uint16_t busNum;
    uint32_t vendorId;
    uint32_t devId;
};

struct PcieCntlr {
    struct IDeviceIoService service;
    struct HdfDeviceObject *hdfDevObj;
    struct PlatformDevice device;
    OsalSpinlock spin;
    struct PcieCntlrOps *ops;
    struct PcieDevCfgInfo devInfo;
    PcieCallbackFunc cb;
    PcieCallbackFunc dmaCb;
    uintptr_t dmaData;
    uint32_t len;
    uint8_t dir;
    void *priv;
};

static inline void PcieCntlrLock(struct PcieCntlr *cntlr)
{
    if (cntlr != NULL) {
        (void)OsalSpinLock(&cntlr->spin);
    }
}

static inline void PcieCntlrUnlock(struct PcieCntlr *cntlr)
{
    if (cntlr != NULL) {
        (void)OsalSpinUnlock(&cntlr->spin);
    }
}

static inline struct PcieCntlr *PcieCntlrGetByBusNum(uint16_t num)
{
    struct PlatformDevice *device = PlatformManagerGetDeviceByNumber(PlatformManagerGet(PLATFORM_MODULE_PCIE), num);

    if (device == NULL) {
        return NULL;
    }
    return CONTAINER_OF(device, struct PcieCntlr, device);
}

int32_t PcieCntlrParse(struct PcieCntlr *cntlr, struct HdfDeviceObject *obj);
int32_t PcieCntlrAdd(struct PcieCntlr *cntlr);
void PcieCntlrRemove(struct PcieCntlr *cntlr);
int32_t PcieCntlrCallback(struct PcieCntlr *cntlr);
int32_t PcieCntlrDmaCallback(struct PcieCntlr *cntlr);

int32_t PcieCntlrRead(struct PcieCntlr *cntlr, uint32_t mode, uint32_t pos, uint8_t *data, uint32_t len);
int32_t PcieCntlrWrite(struct PcieCntlr *cntlr, uint32_t mode, uint32_t pos, uint8_t *data, uint32_t len);
int32_t PcieCntlrRegisterIrq(struct PcieCntlr *cntlr, PcieCallbackFunc cb);
void PcieCntlrUnregisterIrq(struct PcieCntlr *cntlr);
int32_t PcieCntlrDmaMap(struct PcieCntlr *cntlr, PcieCallbackFunc cb, uintptr_t addr, uint32_t len, uint8_t dir);
void PcieCntlrDmaUnmap(struct PcieCntlr *cntlr, uintptr_t addr, uint32_t len, uint8_t dir);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif /* PCIE_CORE_H */
