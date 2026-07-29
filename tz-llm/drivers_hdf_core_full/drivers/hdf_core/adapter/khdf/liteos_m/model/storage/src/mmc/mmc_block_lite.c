/*
 * Copyright (c) 2022 Huawei Device Co., Ltd. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this list of
 *    conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice, this list
 *    of conditions and the following disclaimer in the documentation and/or other materials
 *    provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its contributors may be used
 *    to endorse or promote products derived from this software without specific prior written
 *    permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "mmc_block.h"
#include "mmc_block_lite.h"

struct DiskioMmcHandler g_diskDrv = { { 0 }, { 0 }, { 0 }, { 0 } };

static DSTATUS DiskMmcInit(BYTE pd)
{
    if (pd >= FF_VOLUMES) {
        return RES_PARERR;
    }
    if (g_diskDrv.drv[pd] == NULL) {
        return RES_PARERR;
    }
    g_diskDrv.initialized[pd] = 1;
    return RES_OK;
}

static DSTATUS DiskMmcStatus(BYTE pd)
{
    bool present = false;
    struct MmcBlock *mb = NULL;

    if (pd >= FF_VOLUMES) {
        return RES_PARERR;
    }
    if (g_diskDrv.drv[pd] == NULL) {
        return RES_ERROR;
    }

    mb = (struct MmcBlock *)g_diskDrv.priv[pd];
    if (mb == NULL || mb->mmc == NULL) {
        return RES_ERROR;
    }

    present = (MmcDeviceIsPresent(mb->mmc)) ? true : false;
    if (!present) {
        return RES_ERROR;
    }
    return RES_OK;
}

static DRESULT DisckMmcRead(BYTE pd, BYTE *buffer, DWORD startSector, UINT nSectors)
{
    struct MmcBlock *mb = NULL;
    size_t max;
    ssize_t ret;

    if (pd >= FF_VOLUMES) {
        return RES_PARERR;
    }

    max = (size_t)(-1);
    if (startSector >= (DWORD)max || nSectors >= (UINT)max) {
        return RES_PARERR;
    }

    mb = (struct MmcBlock *)g_diskDrv.priv[pd];
    if (mb == NULL || mb->mmc == NULL) {
        return RES_ERROR;
    }

    ret = MmcDeviceRead(mb->mmc, (uint8_t *)buffer, (size_t)startSector, (size_t)nSectors);
    if (ret != (ssize_t)nSectors) {
        return RES_ERROR;
    }
    return RES_OK;
}

static DRESULT DiskMmcWrite(BYTE pd, const BYTE *buffer, DWORD startSector, UINT nSectors)
{
    struct MmcBlock *mb = NULL;
    size_t max;
    ssize_t ret;

    if (pd >= FF_VOLUMES) {
        return RES_PARERR;
    }

    max = (size_t)(-1);
    if (startSector >= (DWORD)max || nSectors >= (UINT)max) {
        return RES_PARERR;
    }

    mb = (struct MmcBlock *)g_diskDrv.priv[pd];
    if (mb == NULL || mb->mmc == NULL) {
        return RES_ERROR;
    }

    ret = MmcDeviceWrite(mb->mmc, (uint8_t *)buffer, (size_t)startSector, (size_t)nSectors);
    if (ret != (ssize_t)nSectors) {
        return RES_ERROR;
    }
    return RES_OK;
}

static DRESULT DiskMmcIoctl(BYTE pd, BYTE cmd, void *buffer)
{
    struct MmcBlock *mb = NULL;
    struct MmcDevice *dev = NULL;

    if (pd >= FF_VOLUMES) {
        return RES_PARERR;
    }
    if (g_diskDrv.drv[pd] == NULL) {
        return RES_ERROR;
    }

    mb = (struct MmcBlock *)g_diskDrv.priv[pd];
    if (mb == NULL || mb->mmc == NULL) {
        return RES_ERROR;
    }

    dev = mb->mmc;
    switch (cmd) {
        case GET_SECTOR_COUNT:
            *(DWORD *)buffer = (DWORD)dev->capacity;
            break;
        case GET_SECTOR_SIZE:
            *(WORD *)buffer = (WORD)dev->secSize;
            break;
        case GET_BLOCK_SIZE:
            *(WORD *)buffer = (WORD)dev->eraseSize;
            break;
        default:
            break;
    }

    return RES_OK;
}

static struct DiskioMmcOps g_diskioMmcOps = {
    .disk_initialize = DiskMmcInit,
    .disk_status = DiskMmcStatus,
    .disk_read = DisckMmcRead,
    .disk_write = DiskMmcWrite,
    .disk_ioctl = DiskMmcIoctl,
};

static int32_t MmcBlockAdd(struct MmcDevice *mmcDevice, struct MmcBlock *mb)
{
    uint16_t index;

    if (mmcDevice->cntlr == NULL) {
        return HDF_ERR_INVALID_OBJECT;
    }

    index = mmcDevice->cntlr->index;
    if (index >= FF_VOLUMES) {
        return HDF_ERR_INVALID_PARAM;
    }

    g_diskDrv.drv[index] = &g_diskioMmcOps;
    g_diskDrv.lun[index] = (uint8_t)index;
    g_diskDrv.priv[index] = (void *)mb;
    VolToPart[index].di = (uint8_t)index;
    VolToPart[index].pd = (uint8_t)index;
    return HDF_SUCCESS;
}

int32_t MmcBlockOsInit(struct MmcDevice *mmcDevice)
{
    struct MmcBlock *mb = NULL;
    int32_t ret;

    if (mmcDevice == NULL || mmcDevice->mb == NULL) {
        return HDF_ERR_INVALID_OBJECT;
    }

    mb = mmcDevice->mb;
    ret = MmcBlockAdd(mmcDevice, mb);
    if (ret != HDF_SUCCESS) {
        return ret;
    }
    return HDF_SUCCESS;
}

static void MmcBlockRemove(struct MmcDevice *mmcDevice)
{
    uint16_t index;

    if (mmcDevice->cntlr == NULL) {
        return;
    }

    index = mmcDevice->cntlr->index;
    if (index >= FF_VOLUMES) {
        return;
    }
    g_diskDrv.drv[index] = NULL;
    g_diskDrv.priv[index] = NULL;
    g_diskDrv.initialized[index] = 0;
    g_diskDrv.lun[index] = 0;
}

void MmcBlockOsUninit(struct MmcDevice *mmcDevice)
{
    if (mmcDevice == NULL) {
        return;
    }
    MmcBlockRemove(mmcDevice);
}
