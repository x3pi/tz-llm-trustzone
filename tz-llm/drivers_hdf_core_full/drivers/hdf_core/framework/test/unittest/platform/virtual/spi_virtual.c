/*
 * Copyright (c) 2022-2023 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include "device_resource_if.h"
#include "hdf_base.h"
#include "hdf_log.h"
#include "osal_mem.h"
#include "osal_sem.h"
#include "osal_spinlock.h"
#include "spi/spi_core.h"

#define HDF_LOG_TAG           spi_virtual
#define DEFAULT_BUS_NUM       255
#define FIFO_SIZE_MAX         256
#define WAIT_TIMEOUT          1000 // ms
#define DEFAULT_SPEED         2000000U
#define BYTES_PER_WORD        2U
#define BITS_PER_WORD_MIN     4U
#define BITS_PER_WORD_EIGHT   8U
#define BITS_PER_WORD_MAX     16U

struct VirtualSpi {
    struct SpiCntlr *cntlr;
    struct OsalSem sem;
    OsalSpinlock lock;
    uint32_t busNum;
    uint32_t speed;
    uint32_t fifoSize;
    uint32_t numCs;
    uint16_t waterline;
    uint16_t wp;
    uint16_t rp;
    uint16_t validSize;
    uint16_t *ringBuffer;
    struct SpiCfg cfg;
};

static inline uint16_t NextByte(uint32_t fifoSize, uint16_t curByte)
{
    if (curByte >= fifoSize - 1) {
        return 0;
    } else {
        return ++curByte;
    }
}

static bool RingBufferReadWord(struct VirtualSpi *virtual, uint16_t *word)
{
    uint16_t pos = NextByte(virtual->fifoSize, virtual->rp);

    (void)OsalSpinLock(&virtual->lock);
    if (pos == virtual->wp || virtual->validSize == 0) {
        (void)OsalSpinUnlock(&virtual->lock);
        return false;
    }
    *word = virtual->ringBuffer[pos];
    virtual->rp = pos;
    virtual->validSize--;
    (void)OsalSpinUnlock(&virtual->lock);
    return true;
}

static bool RingBufferWriteWord(struct VirtualSpi *virtual, uint16_t word)
{
    (void)OsalSpinLock(&virtual->lock);
    if (virtual->wp == virtual->rp || virtual->validSize == virtual->fifoSize - 1) {
        (void)OsalSpinUnlock(&virtual->lock);
        return false;
    }
    virtual->ringBuffer[virtual->wp] = word;
    virtual->wp = NextByte(virtual->fifoSize, virtual->wp);
    virtual->validSize++;
    if (virtual->validSize == virtual->waterline) {
        (void)OsalSemPost(&virtual->sem);
    }
    (void)OsalSpinUnlock(&virtual->lock);
    return true;
}

static void RingBufferFlush(struct VirtualSpi *virtual)
{
    (void)OsalSpinLock(&virtual->lock);
    virtual->wp = 0;
    virtual->rp = virtual->fifoSize - 1;
    virtual->validSize = 0;
    (void)OsalSpinUnlock(&virtual->lock);
}

static int32_t RingBufferConfig(struct VirtualSpi *virtual)
{
    if (virtual->fifoSize == 0 || virtual->fifoSize > FIFO_SIZE_MAX) {
        HDF_LOGE("RingBufferConfig, invalid fifoSize!");
        return HDF_ERR_INVALID_PARAM;
    }

    if ((virtual->cfg.mode & SPI_MODE_LOOP) != 0 && virtual->ringBuffer == NULL) {
        virtual->ringBuffer = OsalMemAlloc(virtual->fifoSize * BYTES_PER_WORD);
        if (virtual->ringBuffer == NULL) {
            HDF_LOGE("RingBufferConfig: malloc ringBuffer fail!");
            return HDF_ERR_MALLOC_FAIL;
        }
    } else if ((virtual->cfg.mode & SPI_MODE_LOOP) == 0) {
        OsalMemFree(virtual->ringBuffer);
        virtual->ringBuffer = NULL;
    }
    RingBufferFlush(virtual);
    return HDF_SUCCESS;
}

#define ONE_BYTE 1
#define TWO_BYTE 2

static inline uint8_t VirtualSpiToByteWidth(uint8_t bitsPerWord)
{
    if (bitsPerWord <= BITS_PER_WORD_EIGHT) {
        return ONE_BYTE;
    } else {
        return TWO_BYTE;
    }
}

static bool VirtualSpiWriteFifo(struct VirtualSpi *virtual, const uint8_t *tx, uint32_t count)
{
    uint16_t value;
    uint8_t bytes = VirtualSpiToByteWidth(virtual->cfg.bitsPerWord);

    if ((virtual->cfg.mode & SPI_MODE_LOOP) == 0 || virtual->ringBuffer == NULL) {
        /* open loop mode */
        return true;
    }

    for (value = 0; count >= bytes; count -= bytes) {
        if (tx != NULL) {
            value = (bytes == ONE_BYTE) ? *tx : *((uint16_t *)tx);
            tx += bytes;
        }
        if (!RingBufferWriteWord(virtual, value)) {
            return false;
        }
    }
    return true;
}

static bool VirtualSpiReadFifo(struct VirtualSpi *virtual, uint8_t *rx, uint32_t count)
{
    uint16_t value;
    uint8_t bytes = VirtualSpiToByteWidth(virtual->cfg.bitsPerWord);

    if ((virtual->cfg.mode & SPI_MODE_LOOP) == 0 || virtual->ringBuffer == NULL) {
        /* open loop mode */
        return true;
    }

    for (value = 0; count >= bytes; count -= bytes) {
        if (!RingBufferReadWord(virtual, &value)) {
            return false;
        }

        if (rx == NULL) {
            continue;
        }
        if (bytes == ONE_BYTE) {
            *rx = (uint8_t)value;
        } else {
            *((uint16_t *)rx) = value;
        }
        rx += bytes;
    }
    return true;
}

static int32_t VirtualSpiTxRx(struct VirtualSpi *virtual, const struct SpiMsg *msg)
{
    int32_t ret;
    uint32_t tmpLen;
    uint32_t len;
    const uint8_t *tx = msg->wbuf;
    uint8_t *rx = msg->rbuf;
    uint32_t burstSize = virtual->fifoSize >> 1; // burstSize = fifoSzie / 2

    if (tx == NULL && rx == NULL) {
        return HDF_ERR_INVALID_PARAM;
    }

    for (tmpLen = 0, len = msg->len; len > 0; len -= tmpLen) {
        tmpLen = (len > burstSize) ? burstSize : len;
        if (!VirtualSpiWriteFifo(virtual, tx, tmpLen)) {
            HDF_LOGE("VirtualSpiTxRx: write fifo fail!");
            return HDF_FAILURE;
        }
        tx = (tx == NULL) ? NULL : (tx + tmpLen);
        if (tmpLen == burstSize) {
            ret = OsalSemWait((struct OsalSem *)(&virtual->sem), WAIT_TIMEOUT);
            if (ret != HDF_SUCCESS) {
                HDF_LOGE("VirtualSpiTxRx: sem wait timeout, ret: %d!", ret);
                return ret;
            }
        }

        if (!VirtualSpiReadFifo(virtual, rx, tmpLen)) {
            HDF_LOGE("VirtualSpiTxRx: read fifo fail!");
            return HDF_FAILURE;
        }
        rx = (rx == NULL) ? NULL : (rx + tmpLen);
    }
    return HDF_SUCCESS;
}

static inline bool VirtualSpiFindDeviceByCsNum(const struct VirtualSpi *virtual, uint32_t cs)
{
    if (virtual == NULL || virtual->numCs <= cs) {
        return false;
    }

    return true;
}

static int32_t VirtualSpiSetCfg(struct SpiCntlr *cntlr, struct SpiCfg *cfg)
{
    struct VirtualSpi *virtual = NULL;

    if (cntlr == NULL || cntlr->priv == NULL || cfg == NULL) {
        HDF_LOGE("VirtualSpiSetCfg: cntlr priv or cfg is null!");
        return HDF_ERR_INVALID_PARAM;
    }
    virtual = (struct VirtualSpi *)cntlr->priv;
    if (!VirtualSpiFindDeviceByCsNum(virtual, cntlr->curCs)) {
        HDF_LOGE("VirtualSpiSetCfg: invalid cs %u!", cntlr->curCs);
        return HDF_ERR_INVALID_PARAM;
    }
    virtual->cfg = *cfg;
    return HDF_SUCCESS;
}

static int32_t VirtualSpiGetCfg(struct SpiCntlr *cntlr, struct SpiCfg *cfg)
{
    struct VirtualSpi *virtual = NULL;

    if (cntlr == NULL || cntlr->priv == NULL || cfg == NULL) {
        HDF_LOGE("VirtualSpiGetCfg: cntlr priv or cfg is null!");
        return HDF_ERR_INVALID_PARAM;
    }
    virtual = (struct VirtualSpi *)cntlr->priv;
    if (!VirtualSpiFindDeviceByCsNum(virtual, cntlr->curCs)) {
        HDF_LOGE("VirtualSpiGetCfg: invalid cs %u!", cntlr->curCs);
        return HDF_ERR_INVALID_PARAM;
    }
    *cfg = virtual->cfg;
    return HDF_SUCCESS;
}

static int32_t VirtualSpiTransferOneMessage(struct VirtualSpi *virtual, struct SpiMsg *msg)
{
    int32_t ret;

    if (msg == NULL || (msg->rbuf == NULL && msg->wbuf == NULL)) {
        HDF_LOGE("VirtualSpiTransferOneMessage: invalid parameter!");
        return HDF_ERR_INVALID_PARAM;
    }
    virtual->speed = (msg->speed) == 0 ? DEFAULT_SPEED : msg->speed;

    ret = RingBufferConfig(virtual);
    if (ret != HDF_SUCCESS) {
        return ret;
    }

    return VirtualSpiTxRx(virtual, msg);
}

static int32_t VirtualSpiTransfer(struct SpiCntlr *cntlr, struct SpiMsg *msg, uint32_t count)
{
    int32_t ret;
    uint32_t i;
    struct VirtualSpi *virtual = NULL;

    if (cntlr == NULL || cntlr->priv == NULL) {
        HDF_LOGE("VirtualSpiTransfer: invalid controller!");
        return HDF_ERR_INVALID_OBJECT;
    }
    if (msg == NULL || count == 0) {
        HDF_LOGE("VirtualSpiTransfer: invalid parameter!");
        return HDF_ERR_INVALID_PARAM;
    }

    virtual = (struct VirtualSpi *)cntlr->priv;
    if (!VirtualSpiFindDeviceByCsNum(virtual, cntlr->curCs)) {
        HDF_LOGE("VirtualSpiTransfer: invalid cs %u!", cntlr->curCs);
        return HDF_FAILURE;
    }
    for (i = 0; i < count; i++) {
        ret = VirtualSpiTransferOneMessage(virtual, &(msg[i]));
        if (ret != 0) {
            HDF_LOGE("VirtualSpiTransfer: transfer error, ret: %d!", ret);
            return ret;
        }
    }
    return HDF_SUCCESS;
}

int32_t VirtualSpiOpen(struct SpiCntlr *cntlr)
{
    (void)cntlr;
    return HDF_SUCCESS;
}

int32_t VirtualSpiClose(struct SpiCntlr *cntlr)
{
    (void)cntlr;
    return HDF_SUCCESS;
}

static struct SpiCntlrMethod g_method = {
    .Transfer = VirtualSpiTransfer,
    .SetCfg = VirtualSpiSetCfg,
    .GetCfg = VirtualSpiGetCfg,
    .Open = VirtualSpiOpen,
    .Close = VirtualSpiClose,
};

static int32_t SpiGetCfgFromHcs(struct VirtualSpi *virtual, const struct DeviceResourceNode *node)
{
    struct DeviceResourceIface *iface = DeviceResourceGetIfaceInstance(HDF_CONFIG_SOURCE);

    if (iface == NULL || iface->GetUint32 == NULL || iface->GetUint16 == NULL || iface->GetUint8 == NULL) {
        HDF_LOGE("SpiGetCfgFromHcs: face is invalid!");
        return HDF_FAILURE;
    }
    if (iface->GetUint32(node, "busNum", &virtual->busNum, DEFAULT_BUS_NUM) != HDF_SUCCESS) {
        HDF_LOGE("SpiGetCfgFromHcs: read busNum fail!");
        return HDF_FAILURE;
    }
    if (iface->GetUint32(node, "numCs", &virtual->numCs, 0) != HDF_SUCCESS) {
        HDF_LOGE("SpiGetCfgFromHcs: read numCs fail!");
        return HDF_FAILURE;
    }
    if (iface->GetUint32(node, "speed", &virtual->speed, 0) != HDF_SUCCESS) {
        HDF_LOGE("SpiGetCfgFromHcs: read speed fail!");
        return HDF_FAILURE;
    }
    if (iface->GetUint32(node, "fifoSize", &virtual->fifoSize, 0) != HDF_SUCCESS) {
        HDF_LOGE("SpiGetCfgFromHcs: read fifoSize fail!");
        return HDF_FAILURE;
    }
    if (iface->GetUint16(node, "mode", &virtual->cfg.mode, 0) != HDF_SUCCESS) {
        HDF_LOGE("SpiGetCfgFromHcs: read mode fail!");
        return HDF_FAILURE;
    }
    if (iface->GetUint8(node, "bitsPerWord", &virtual->cfg.bitsPerWord, 0) != HDF_SUCCESS) {
        HDF_LOGE("SpiGetCfgFromHcs: read bitsPerWord fail!");
        return HDF_FAILURE;
    }
    if (iface->GetUint8(node, "transferMode", &virtual->cfg.transferMode, 0) != HDF_SUCCESS) {
        HDF_LOGE("SpiGetCfgFromHcs: read transferMode fail!");
        return HDF_FAILURE;
    }
    if (iface->GetUint32(node, "maxSpeedHz", &virtual->cfg.maxSpeedHz, 0) != HDF_SUCCESS) {
        HDF_LOGE("SpiGetCfgFromHcs: read maxSpeedHz fail!");
        return HDF_FAILURE;
    }
    if (iface->GetUint16(node, "waterline", &virtual->waterline, 0) != HDF_SUCCESS) {
        HDF_LOGE("SpiGetCfgFromHcs: read waterline fail!");
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

static int32_t VirtualSpiInit(struct SpiCntlr *cntlr, const struct HdfDeviceObject *device)
{
    int32_t ret;
    struct VirtualSpi *virtual = NULL;

    if (device->property == NULL) {
        HDF_LOGE("VirtualSpiInit: property is null!");
        return HDF_ERR_INVALID_PARAM;
    }

    virtual = (struct VirtualSpi *)OsalMemCalloc(sizeof(*virtual));
    if (virtual == NULL) {
        HDF_LOGE("VirtualSpiInit: OsalMemCalloc error!");
        return HDF_FAILURE;
    }
    ret = SpiGetCfgFromHcs(virtual, device->property);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("VirtualSpiInit: SpiGetCfgFromHcs error, ret: %d!", ret);
        OsalMemFree(virtual);
        return ret;
    }

    virtual->cntlr = cntlr;
    cntlr->priv = (void *)virtual;
    cntlr->method = &g_method;
    ret = OsalSemInit(&virtual->sem, 0);
    if (ret != HDF_SUCCESS) {
        return ret;
    }
    if (OsalSpinInit(&virtual->lock) != HDF_SUCCESS) {
        HDF_LOGE("VirtualSpiInit: init lock fail!");
        return HDF_FAILURE;
    }

    return HDF_SUCCESS;
}

static int32_t VirtualSpiDeviceBind(struct HdfDeviceObject *device)
{
    HDF_LOGI("VirtualSpiDeviceBind: entry!");
    if (device == NULL) {
        return HDF_ERR_INVALID_OBJECT;
    }
    return (SpiCntlrCreate(device) == NULL) ? HDF_FAILURE : HDF_SUCCESS;
}

static int32_t VirtualSpiDeviceInit(struct HdfDeviceObject *device)
{
    int32_t ret;
    struct SpiCntlr *cntlr = NULL;

    if (device == NULL) {
        HDF_LOGE("VirtualSpiDeviceInit: device is null!");
        return HDF_ERR_INVALID_OBJECT;
    }
    cntlr = SpiCntlrFromDevice(device);
    if (cntlr == NULL) {
        HDF_LOGE("VirtualSpiDeviceInit: cntlr is null!");
        return HDF_FAILURE;
    }

    ret = VirtualSpiInit(cntlr, device);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("VirtualSpiDeviceInit: error init, ret: %d!", ret);
        return ret;
    }
    ret = RingBufferConfig((struct VirtualSpi *)cntlr->priv);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("VirtualSpiDeviceInit: RingBufferConfig error!");
    } else {
        HDF_LOGI("VirtualSpiDeviceInit: spi device init success!");
    }

    return ret;
}

static void VirtualSpiDeviceRelease(struct HdfDeviceObject *device)
{
    struct SpiCntlr *cntlr = NULL;
    struct VirtualSpi *virtual = NULL;

    HDF_LOGI("VirtualSpiDeviceRelease: entry!");
    if (device == NULL) {
        HDF_LOGE("VirtualSpiDeviceRelease: device is null!");
        return;
    }
    cntlr = SpiCntlrFromDevice(device);
    if (cntlr == NULL || cntlr->priv == NULL) {
        HDF_LOGE("VirtualSpiDeviceRelease: cntlr or priv is null!");
        return;
    }
    SpiCntlrDestroy(cntlr);
    virtual = (struct VirtualSpi *)cntlr->priv;
    (void)OsalSpinDestroy(&(virtual->lock));
    (void)OsalSemDestroy(&virtual->sem);
    OsalMemFree(virtual->ringBuffer);
    OsalMemFree(virtual);
}

static const struct HdfDriverEntry g_virtualSpiDevice = {
    .moduleVersion = 1,
    .moduleName = "virtual_spi_driver",
    .Bind = VirtualSpiDeviceBind,
    .Init = VirtualSpiDeviceInit,
    .Release = VirtualSpiDeviceRelease,
};

HDF_INIT(g_virtualSpiDevice);
