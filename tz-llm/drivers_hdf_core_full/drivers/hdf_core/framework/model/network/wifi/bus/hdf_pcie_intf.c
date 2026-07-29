/*
 * Copyright (c) 2023 Shenzhen Kaihong Digital Industry Development Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include "hdf_base.h"
#include "hdf_dlist.h"
#include "hdf_ibus_intf.h"
#include "hdf_log.h"
#include "osal_mem.h"
#include "pcie_if.h"
#include "securec.h"
#include "wifi_inc.h"
#define HDF_LOG_TAG HDF_WIFI_CORE
struct PcieDeviceBase {
    DevHandle handle;
    IrqHandler *irqHandler;
    MapHandler *mapHandler;
};
#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

static struct DevHandle *HdfGetDevHandle(struct BusDev *dev, const struct HdfConfigWlanBus *busCfg)
{
    struct DevHandle *handle = NULL;
    struct PcieDeviceBase *pcieDevBase = OsalMemCalloc(sizeof(struct PcieDeviceBase));
    if (pcieDevBase == NULL) {
        HDF_LOGE("%s: OsalMemCalloc failed!", __func__);
        return NULL;
    }
    handle = PcieOpen(busCfg->busIdx);
    if (handle == NULL) {
        HDF_LOGE("%s: pcie card detected!", __func__);
        OsalMemFree(pcieDevBase);
        return NULL;
    }
    pcieDevBase->handle = handle;
    dev->devBase = pcieDevBase;
    dev->priData.driverName = NULL; // check driver name
    return handle;
}

static int32_t HdfGetPcieInfo(struct BusDev *dev, struct BusConfig *busCfg)
{
    (void)dev;
    (void)busCfg;
    return HDF_SUCCESS;
}

static int32_t HdfPcieInit(struct BusDev *dev, const struct HdfConfigWlanBus *busCfg)
{
    struct DevHandle *handle = NULL;
    if (dev == NULL || busCfg == NULL) {
        HDF_LOGE("%s: input parameter error!", __func__);
        return HDF_FAILURE;
    }
    handle = HdfGetDevHandle(dev, busCfg);
    if (handle == NULL) {
        HDF_LOGE("%s: get handle error!", __func__);
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

static void HdfPcieReleaseDev(struct BusDev *dev)
{
    struct PcieDeviceBase *devBase = NULL;
    if (dev == NULL || dev->devBase == NULL) {
        HDF_LOGE("%s:input parameter error!", __func__);
        return;
    }
    devBase = dev->devBase;
    if (devBase->handle != NULL) {
        PcieClose(devBase->handle);
        devBase->handle = NULL;
    }
    if (dev->priData.data != NULL) {
        dev->priData.release(dev->priData.data);
        dev->priData.data = NULL;
    }
    OsalMemFree(devBase);
    dev->devBase = NULL;
}

static int32_t HdfPcieDisableFunc(struct BusDev *dev)
{
    (void)dev;
    return HDF_SUCCESS;
}

static int32_t PcieIrqCallBack(DevHandle handle)
{
    struct PcieDeviceBase *devBase = CONTAINER_OF(handle, struct PcieDeviceBase, handle);
    if (devBase->handle == NULL || devBase->irqHandler == NULL) {
        HDF_LOGE("%s:input parameter error!", __func__);
        return HDF_SUCCESS;
    }
    devBase->irqHandler(NULL);
    return HDF_SUCCESS;
}

static int32_t HdfPcieCliamIrq(struct BusDev *dev, IrqHandler *handler, void *data)
{
    (void)data;
    int32_t ret;
    struct PcieDeviceBase *devBase = NULL;
    if (dev == NULL || dev->devBase == NULL) {
        HDF_LOGE("%s:input parameter error!", __func__);
        return HDF_ERR_INVALID_PARAM;
    }
    devBase = dev->devBase;
    if (devBase->handle == NULL) {
        HDF_LOGE("%s:handle is NULL!", __func__);
        return HDF_ERR_INVALID_PARAM;
    }
    // save irq handler
    if (handler != NULL) {
        devBase->irqHandler = handler;
    }

    ret = PcieRegisterIrq(devBase->handle, PcieIrqCallBack);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s:register irq error!", __func__);
    }
    return ret;
}

static int32_t HdfPcieReleaseIrq(struct BusDev *dev)
{
    struct PcieDeviceBase *devBase = NULL;
    if (dev == NULL || dev->devBase == NULL) {
        HDF_LOGE("%s:input parameter error!", __func__);
        return HDF_ERR_INVALID_PARAM;
    }
    devBase = dev->devBase;
    if (devBase->handle == NULL) {
        HDF_LOGE("%s:handle is NULL!", __func__);
        return HDF_ERR_INVALID_PARAM;
    }
    PcieUnregisterIrq(devBase->handle);
    devBase->irqHandler = NULL;
    return HDF_SUCCESS;
}

static void HdfPcieClaimHost(struct BusDev *dev)
{
    (void)dev;
}

static void HdfPcieReleaseHost(struct BusDev *dev)
{
    (void)dev;
}

static int32_t HdfPcieReset(struct BusDev *dev)
{
    (void)dev;
    return HDF_SUCCESS;
}

static int32_t HdfPcieReadN(struct BusDev *dev, uint32_t addr, uint32_t cnt, uint8_t *buf)
{
    // read config
    int32_t ret;
    struct PcieDeviceBase *devBase = NULL;
    if (dev == NULL || dev->devBase == NULL) {
        HDF_LOGE("%s:input parameter error!", __func__);
        return HDF_ERR_INVALID_PARAM;
    }
    devBase = dev->devBase;
    if (devBase->handle == NULL) {
        HDF_LOGE("%s:handle is NULL!", __func__);
        return HDF_ERR_INVALID_PARAM;
    }
    ret = PcieRead(devBase->handle, PCIE_CONFIG, addr, buf, cnt);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: pcie read err", __func__);
    }
    return ret;
}

static int32_t HdfPcieReadFunc0(struct BusDev *dev, uint32_t addr, uint32_t cnt, uint8_t *buf)
{
    (void)dev;
    (void)addr;
    (void)cnt;
    (void)buf;
    return HDF_SUCCESS;
}

static int32_t HdfPcieReadSpcReg(struct BusDev *dev, uint32_t addr, uint32_t cnt, uint8_t *buf, uint32_t sg_len)
{
    (void)dev;
    (void)addr;
    (void)cnt;
    (void)buf;
    (void)sg_len;
    return HDF_SUCCESS;
}

static int32_t HdfPcieWriteN(struct BusDev *dev, uint32_t addr, uint32_t cnt, uint8_t *buf)
{
    // write config
    int32_t ret;
    struct PcieDeviceBase *devBase = NULL;
    if (dev == NULL || dev->devBase == NULL) {
        HDF_LOGE("%s:input parameter error!", __func__);
        return HDF_ERR_INVALID_PARAM;
    }
    devBase = dev->devBase;
    if (devBase->handle == NULL) {
        HDF_LOGE("%s:handle is NULL!", __func__);
        return HDF_ERR_INVALID_PARAM;
    }
    ret = PcieWrite(devBase->handle, PCIE_CONFIG, addr, buf, cnt);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: pcie write err", __func__);
    }
    return ret;
}

static int32_t HdfPcieWriteFunc0(struct BusDev *dev, uint32_t addr, uint32_t cnt, uint8_t *buf)
{
    (void)dev;
    (void)addr;
    (void)cnt;
    (void)buf;
    return HDF_SUCCESS;
}

static int32_t HdfPcieWriteSpcReg(struct BusDev *dev, uint32_t addr, uint32_t cnt, uint8_t *buf, uint32_t sg_len)
{
    (void)dev;
    (void)addr;
    (void)cnt;
    (void)buf;
    (void)sg_len;
    return HDF_SUCCESS;
}

static int32_t PcieMapCallbackFunc(DevHandle handle)
{
    struct PcieDeviceBase *devBase = CONTAINER_OF(handle, struct PcieDeviceBase, handle);
    if (devBase->handle == NULL || devBase->mapHandler == NULL) {
        HDF_LOGE("%s:input parameter error!", __func__);
        return HDF_SUCCESS;
    }
    return devBase->mapHandler(NULL);
}

static int32_t HdfPcieDmaMap(struct BusDev *dev, MapHandler *cb, uintptr_t addr, uint32_t len, uint8_t dir)
{
    int32_t ret;
    struct PcieDeviceBase *devBase = NULL;
    if (dev == NULL || dev->devBase == NULL) {
        HDF_LOGE("%s:input parameter error!", __func__);
        return HDF_ERR_INVALID_PARAM;
    }
    devBase = dev->devBase;
    if (devBase->handle == NULL) {
        HDF_LOGE("%s:handle is NULL!", __func__);
        return HDF_ERR_INVALID_PARAM;
    }
    // save map handler
    devBase->mapHandler = cb;
    ret = PcieDmaMap(devBase->handle, PcieMapCallbackFunc, addr, len, dir);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: pcie dma map err", __func__);
    }
    return ret;
}
static int32_t HdfPcieDmaUnmap(struct BusDev *dev, uintptr_t addr, uint32_t len, uint8_t dir)
{
    struct PcieDeviceBase *devBase = NULL;
    if (dev == NULL || dev->devBase == NULL) {
        HDF_LOGE("%s:input parameter error!", __func__);
        return HDF_ERR_INVALID_PARAM;
    }
    devBase = dev->devBase;
    if (devBase->handle == NULL) {
        HDF_LOGE("%s:handle is NULL!", __func__);
        return HDF_ERR_INVALID_PARAM;
    }
    PcieDmaUnmap(devBase->handle, addr, len, dir);
    devBase->mapHandler = NULL;
    return HDF_SUCCESS;
}

static int32_t HdfPcieIoRead(struct BusDev *dev, uint32_t addr, uint32_t cnt, uint8_t *buf)
{
    int ret;
    struct PcieDeviceBase *devBase = NULL;
    if (dev == NULL || dev->devBase == NULL) {
        HDF_LOGE("%s:input parameter error!", __func__);
        return HDF_ERR_INVALID_PARAM;
    }
    devBase = dev->devBase;
    if (devBase->handle == NULL) {
        HDF_LOGE("%s:handle is NULL!", __func__);
        return HDF_ERR_INVALID_PARAM;
    }
    ret = PcieRead(devBase->handle, PCIE_IO, addr, buf, cnt);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: pcie io read err", __func__);
    }
    return ret;
}
static int32_t HdfPcieIoWrite(struct BusDev *dev, uint32_t addr, uint32_t cnt, uint8_t *buf)
{
    int32_t ret;
    struct PcieDeviceBase *devBase = NULL;
    if (dev == NULL || dev->devBase == NULL) {
        HDF_LOGE("%s:input parameter error!", __func__);
        return HDF_ERR_INVALID_PARAM;
    }
    devBase = dev->devBase;
    if (devBase->handle == NULL) {
        HDF_LOGE("%s:handle is NULL!", __func__);
        return HDF_ERR_INVALID_PARAM;
    }
    ret = PcieWrite(devBase->handle, PCIE_IO, addr, buf, cnt);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: pcie io write err", __func__);
    }
    return ret;
}

static void HdfSetBusOps(struct BusDev *dev)
{
    dev->ops.getBusInfo = HdfGetPcieInfo;
    dev->ops.deInit = HdfPcieReleaseDev;
    dev->ops.init = HdfPcieInit;

    dev->ops.readData = HdfPcieReadN;
    dev->ops.writeData = HdfPcieWriteN;
    dev->ops.bulkRead = HdfPcieReadSpcReg;
    dev->ops.bulkWrite = HdfPcieWriteSpcReg;
    dev->ops.readFunc0 = HdfPcieReadFunc0;
    dev->ops.writeFunc0 = HdfPcieWriteFunc0;

    dev->ops.claimIrq = HdfPcieCliamIrq;
    dev->ops.releaseIrq = HdfPcieReleaseIrq;
    dev->ops.disableBus = HdfPcieDisableFunc;
    dev->ops.reset = HdfPcieReset;

    dev->ops.claimHost = HdfPcieClaimHost;
    dev->ops.releaseHost = HdfPcieReleaseHost;

    // IO /dma
    dev->ops.ioRead = HdfPcieIoRead;
    dev->ops.ioWrite = HdfPcieIoWrite;
    dev->ops.dmaMap = HdfPcieDmaMap;
    dev->ops.dmaUnmap = HdfPcieDmaUnmap;
}
int32_t HdfWlanBusAbsInit(struct BusDev *dev, const struct HdfConfigWlanBus *busConfig)
{
    if (dev == NULL) {
        HDF_LOGE("%s:set pcie device ops failed!", __func__);
        return HDF_FAILURE;
    }
    HdfSetBusOps(dev);
    return HdfPcieInit(dev, busConfig);
}

int32_t HdfWlanConfigBusAbs(uint8_t busId)
{
    (void)busId;
    return HDF_SUCCESS;
}

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif
