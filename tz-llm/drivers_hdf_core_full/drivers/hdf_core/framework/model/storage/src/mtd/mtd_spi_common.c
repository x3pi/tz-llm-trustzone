/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include "mtd_spi_common.h"
#include "hdf_log.h"

static void SpiFlashDumpDefualt(struct MtdDevice *mtdDevice)
{
    struct MtdSpiConfig *cfg = NULL;
    struct SpiFlash *spi = NULL;

    if (mtdDevice == NULL) {
        HDF_LOGE("SpiFlashDumpDefualt: mtdDevice is null!");
        return;
    }
    MTD_DEVICE_DUMP(mtdDevice);

    spi = CONTAINER_OF(mtdDevice, struct SpiFlash, mtd);
    HDF_LOGD("SpiFlashDumpDefualt: cs = %u, addrCycle = %u!", spi->cs, spi->addrCycle);

    cfg = &spi->readCfg;
    HDF_LOGD("SpiFlashDumpDefualt: readCfg -> ifType:%u, cmd:0x%x, dummy:%u, size:%u, clock:%u!",
        cfg->ifType, cfg->cmd, cfg->dummy, cfg->size, cfg->clock);
    cfg = &spi->writeCfg;
    HDF_LOGD("SpiFlashDumpDefualt: writeCfg -> ifType:%u, cmd:0x%x, dummy:%u, size:%u, clock:%u!",
        cfg->ifType, cfg->cmd, cfg->dummy, cfg->size, cfg->clock);
    cfg = &spi->eraseCfg;
    HDF_LOGD("SpiFlashDumpDefualt: eraseCfg -> ifType:%u, cmd:0x%x, dummy:%u, size:%u, clock:%u!",
        cfg->ifType, cfg->cmd, cfg->dummy, cfg->size, cfg->clock);
}

int32_t SpiFlashAdd(struct SpiFlash *spi)
{
    if (spi == NULL || spi->mtd.ops == NULL) {
        HDF_LOGE("SpiFlashAdd: spi or mtd.ops is null!");
        return HDF_ERR_INVALID_OBJECT;
    }
    if (spi->mtd.ops->dump == NULL) {
        spi->mtd.ops->dump = SpiFlashDumpDefualt;
    }
    return MtdDeviceAdd(&spi->mtd);
}

void SpiFlashDel(struct SpiFlash *spi)
{
    if (spi != NULL) {
        MtdDeviceDel(&spi->mtd);
    }
}

int32_t SpiFlashWaitReady(struct SpiFlash *spi)
{
    int32_t ret;

    if (spi == NULL) {
        HDF_LOGE("SpiFlashWaitReady: spi is null!");
        return HDF_ERR_INVALID_OBJECT;
    }
    if (spi->spiOps.waitReady == NULL) {
        HDF_LOGE("SpiFlashWaitReady: waitReady is null!");
        return HDF_ERR_NOT_SUPPORT;
    }
    ret = (spi->spiOps.waitReady(spi));
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("SpiFlashWaitReady: wait dev ready fail, ret: %d!", ret);
    }
    return ret;
}

int32_t SpiFlashWriteEnable(struct SpiFlash *spi)
{
    int32_t ret;

    if (spi == NULL) {
        HDF_LOGE("SpiFlashWriteEnable: spi is null!");
        return HDF_ERR_INVALID_OBJECT;
    }
    if (spi->spiOps.writeEnable == NULL) {
        HDF_LOGE("SpiFlashWriteEnable: writeEnable is null!");
        return HDF_ERR_NOT_SUPPORT;
    }
    ret = (spi->spiOps.writeEnable(spi));
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("SpiFlashWriteEnable: dev write enable fail, ret: %d!", ret);
    }
    return ret;
}

int32_t SpiFlashQeEnable(struct SpiFlash *spi)
{
    int32_t ret;

    if (spi == NULL) {
        HDF_LOGE("SpiFlashQeEnable: spi is null!");
        return HDF_ERR_INVALID_OBJECT;
    }
    if (spi->spiOps.qeEnable == NULL) {
        HDF_LOGE("SpiFlashQeEnable: qeEnable is null!");
        return HDF_ERR_NOT_SUPPORT;
    }
    ret = (spi->spiOps.qeEnable(spi));
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("SpiFlashQeEnable: dev qe enable fail, ret: %d!", ret);
    }
    return ret;
}

int32_t SpiFlashEntry4Addr(struct SpiFlash *spi, int enable)
{
    int32_t ret;

    if (spi == NULL) {
        HDF_LOGE("SpiFlashEntry4Addr: spi is null!");
        return HDF_ERR_INVALID_OBJECT;
    }
    if (spi->spiOps.entry4Addr == NULL) {
        HDF_LOGE("SpiFlashEntry4Addr: entry4Addr is null!");
        return HDF_ERR_NOT_SUPPORT;
    }
    ret = (spi->spiOps.entry4Addr(spi, enable));
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("SpiFlashEntry4Addr: dev set 4addr fail, enabl: %d, ret: %d!", enable, ret);
    }
    return ret;
}
