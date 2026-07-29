/*
 * Copyright (c) 2020-2022 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#ifndef SPI_CORE_H
#define SPI_CORE_H

#include "hdf_base.h"
#include "hdf_device_desc.h"
#include "hdf_dlist.h"
#include "spi_if.h"
#include "osal_mutex.h"

struct SpiCntlr;
struct SpiCntlrMethod;

struct SpiCntlrMethod {
    int32_t (*GetCfg)(struct SpiCntlr *cntlr, struct SpiCfg *cfg);
    int32_t (*SetCfg)(struct SpiCntlr *cntlr, struct SpiCfg *cfg);
    int32_t (*Transfer)(struct SpiCntlr *cntlr, struct SpiMsg *msg, uint32_t count);
    int32_t (*Open)(struct SpiCntlr *cntlr);
    int32_t (*Close)(struct SpiCntlr *cntlr);
};

struct SpiCntlr {
    struct IDeviceIoService service;
    struct HdfDeviceObject *device;
    uint32_t busNum;
    uint32_t numCs;
    uint32_t curCs;
    struct OsalMutex lock;
    struct SpiCntlrMethod *method;
    struct DListHead list;
    void *priv;
};

struct SpiDev {
    struct SpiCntlr *cntlr;
    struct DListHead list;
    struct SpiCfg cfg;
    uint32_t csNum;
    void *priv;
};

struct SpiCntlr *SpiCntlrCreate(struct HdfDeviceObject *device);
void SpiCntlrDestroy(struct SpiCntlr *cntlr);

/**
 * @brief Turn SpiCntlr to a HdfDeviceObject.
 *
 * @param cntlr Indicates the SPI cntlr device.
 *
 * @return Retrns the pointer of the HdfDeviceObject on success; returns NULL otherwise.
 * @since 1.0
 */
static inline struct HdfDeviceObject *SpiCntlrToDevice(const struct SpiCntlr *cntlr)
{
    return (cntlr == NULL) ? NULL : cntlr->device;
}

/**
 * @brief Turn HdfDeviceObject to an SpiCntlr.
 *
 * @param device Indicates a HdfDeviceObject.
 *
 * @return Retrns the pointer of the SpiCntlr on success; returns NULL otherwise.
 * @since 1.0
 */
static inline struct SpiCntlr *SpiCntlrFromDevice(const struct HdfDeviceObject *device)
{
    return (device == NULL) ? NULL : (struct SpiCntlr *)device->service;
}

int32_t SpiCntlrTransfer(struct SpiCntlr *cntlr, uint32_t csNum, struct SpiMsg *msg, uint32_t count);
int32_t SpiCntlrSetCfg(struct SpiCntlr *cntlr, uint32_t csNum, struct SpiCfg *cfg);
int32_t SpiCntlrGetCfg(struct SpiCntlr *cntlr, uint32_t csNum, struct SpiCfg *cfg);
int32_t SpiCntlrOpen(struct SpiCntlr *cntlr, uint32_t csNum);
int32_t SpiCntlrClose(struct SpiCntlr *cntlr, uint32_t csNum);

#endif /* SPI_CORE_H */
