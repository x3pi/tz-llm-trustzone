/*
 * Copyright (c) 2021-2023 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include "mipi_dsi_core.h"
#include "hdf_log.h"
#include "osal_mem.h"
#include "osal_time.h"

#define HDF_LOG_TAG mipi_dsi_core

struct MipiDsiHandle {
    struct MipiDsiCntlr *cntlr;
    struct OsalMutex lock;
    void *priv;
};

static struct MipiDsiHandle g_mipiDsihandle[MAX_CNTLR_CNT];

int32_t MipiDsiRegisterCntlr(struct MipiDsiCntlr *cntlr, struct HdfDeviceObject *device)
{
    if (cntlr == NULL) {
        HDF_LOGE("MipiDsiRegisterCntlr: cntlr is null!");
        return HDF_ERR_INVALID_OBJECT;
    }
    if (cntlr->devNo >= MAX_CNTLR_CNT) {
        HDF_LOGE("MipiDsiRegisterCntlr: cntlr->devNo is error!");
        return HDF_ERR_INVALID_PARAM;
    }

    if (device == NULL) {
        HDF_LOGE("MipiDsiRegisterCntlr: device is null!");
        return HDF_ERR_INVALID_OBJECT;
    }

    if (g_mipiDsihandle[cntlr->devNo].cntlr == NULL) {
        (void)OsalMutexInit(&g_mipiDsihandle[cntlr->devNo].lock);
        (void)OsalMutexInit(&(cntlr->lock));

        g_mipiDsihandle[cntlr->devNo].cntlr = cntlr;
        g_mipiDsihandle[cntlr->devNo].priv = NULL;
        cntlr->device = device;
        device->service = &(cntlr->service);
        cntlr->priv = NULL;
        HDF_LOGI("MipiDsiRegisterCntlr: success!");

        return HDF_SUCCESS;
    }

    HDF_LOGE("MipiDsiRegisterCntlr: cntlr already exists!");
    return HDF_FAILURE;
}

void MipiDsiUnregisterCntlr(struct MipiDsiCntlr *cntlr)
{
    if (cntlr == NULL) {
        HDF_LOGE("MipiDsiUnregisterCntlr: cntlr is null!");
        return;
    }

    (void)OsalMutexDestroy(&(cntlr->lock));
    (void)OsalMutexDestroy(&(g_mipiDsihandle[cntlr->devNo].lock));

    HDF_LOGI("MipiDsiUnregisterCntlr: success!");
    return;
}

struct MipiDsiCntlr *MipiDsiCntlrFromDevice(const struct HdfDeviceObject *device)
{
    return (device == NULL) ? NULL : (struct MipiDsiCntlr *)device->service;
}

struct MipiDsiCntlr *MipiDsiCntlrOpen(uint8_t number)
{
    struct MipiDsiCntlr *cntlr = NULL;

    if (number >= MAX_CNTLR_CNT) {
        HDF_LOGE("MipiDsiCntlrOpen: invalid number!");
        return NULL;
    }

    if (g_mipiDsihandle[number].cntlr == NULL) {
        HDF_LOGE("MipiDsiCntlrOpen: g_mipiDsihandle[number].cntlr is null!");
        return NULL;
    }

    (void)OsalMutexLock(&(g_mipiDsihandle[number].lock));
    g_mipiDsihandle[number].cntlr->devNo = number;
    cntlr = g_mipiDsihandle[number].cntlr;
    (void)OsalMutexUnlock(&(g_mipiDsihandle[number].lock));

    return cntlr;
}

void MipiDsiCntlrClose(struct MipiDsiCntlr *cntlr)
{
    uint8_t number;

    if (cntlr == NULL) {
        HDF_LOGE("MipiDsiCntlrClose: cntlr is null!");
        return;
    }

    number = cntlr->devNo;
    if (number >= MAX_CNTLR_CNT) {
        HDF_LOGE("MipiDsiCntlrClose: invalid number!");
        return;
    }

    HDF_LOGI("MipiDsiCntlrClose: success!");
}

int32_t MipiDsiCntlrSetCfg(struct MipiDsiCntlr *cntlr, const struct MipiCfg *cfg)
{
    int32_t ret;

    if ((cntlr == NULL) || (cntlr->ops == NULL)) {
        HDF_LOGE("MipiDsiCntlrSetCfg: cntlr or ops is null!");
        return HDF_ERR_INVALID_OBJECT;
    }

    if (cfg == NULL) {
        HDF_LOGE("MipiDsiCntlrSetCfg: cfg is null!");
        return HDF_ERR_INVALID_OBJECT;
    }

    if (cntlr->ops->setCntlrCfg == NULL) {
        HDF_LOGE("MipiDsiCntlrSetCfg: setCntlrCfg is null!");
        return HDF_ERR_NOT_SUPPORT;
    }

    (void)OsalMutexLock(&(cntlr->lock));
    cntlr->cfg = *cfg;
    ret = cntlr->ops->setCntlrCfg(cntlr);
    (void)OsalMutexUnlock(&(cntlr->lock));

    if (ret != HDF_SUCCESS) {
        HDF_LOGE("MipiDsiCntlrSetCfg: fail!");
    }

    return ret;
}

int32_t MipiDsiCntlrGetCfg(struct MipiDsiCntlr *cntlr, struct MipiCfg *cfg)
{
    if ((cntlr == NULL) || (cntlr->ops == NULL)) {
        HDF_LOGE("MipiDsiCntlrGetCfg: cntlr or ops is null!");
        return HDF_ERR_INVALID_OBJECT;
    }
    if (cfg == NULL) {
        HDF_LOGE("MipiDsiCntlrGetCfg: cfg is null!");
        return HDF_ERR_INVALID_OBJECT;
    }

    (void)OsalMutexLock(&(cntlr->lock));
    *cfg = cntlr->cfg;
    (void)OsalMutexUnlock(&(cntlr->lock));

    return HDF_SUCCESS;
}

void MipiDsiCntlrSetLpMode(struct MipiDsiCntlr *cntlr)
{
    if ((cntlr == NULL) || (cntlr->ops == NULL)) {
        HDF_LOGE("MipiDsiCntlrSetLpMode: cntlr or ops is null!");
        return;
    }

    if (cntlr->ops->toLp == NULL) {
        HDF_LOGE("MipiDsiCntlrSetLpMode: toLp is null!");
        return;
    }

    (void)OsalMutexLock(&(cntlr->lock));
    cntlr->ops->toLp(cntlr);
    (void)OsalMutexUnlock(&(cntlr->lock));
}

void MipiDsiCntlrSetHsMode(struct MipiDsiCntlr *cntlr)
{
    if ((cntlr == NULL) || (cntlr->ops == NULL)) {
        HDF_LOGE("MipiDsiCntlrSetHsMode: cntlr or ops is null!");
        return;
    }

    if (cntlr->ops->toHs == NULL) {
        HDF_LOGE("MipiDsiCntlrSetHsMode: toHs is null!");
        return;
    }

    (void)OsalMutexLock(&(cntlr->lock));
    cntlr->ops->toHs(cntlr);
    (void)OsalMutexUnlock(&(cntlr->lock));
}

void MipiDsiCntlrEnterUlps(struct MipiDsiCntlr *cntlr)
{
    if ((cntlr == NULL) || (cntlr->ops == NULL)) {
        HDF_LOGE("MipiDsiCntlrEnterUlps: cntlr or ops is null!");
        return;
    }

    if (cntlr->ops->enterUlps == NULL) {
        HDF_LOGE("MipiDsiCntlrEnterUlps: enterUlps is null!");
        return;
    }

    (void)OsalMutexLock(&(cntlr->lock));
    cntlr->ops->enterUlps(cntlr);
    (void)OsalMutexUnlock(&(cntlr->lock));
}

void MipiDsiCntlrExitUlps(struct MipiDsiCntlr *cntlr)
{
    if ((cntlr == NULL) || (cntlr->ops == NULL)) {
        HDF_LOGE("MipiDsiCntlrExitUlps: cntlr, ops or exitUlps is null!");
        return;
    }

    if (cntlr->ops->exitUlps == NULL) {
        HDF_LOGE("MipiDsiCntlrExitUlps: exitUlps is null!");
        return;
    }

    (void)OsalMutexLock(&(cntlr->lock));
    cntlr->ops->exitUlps(cntlr);
    (void)OsalMutexUnlock(&(cntlr->lock));
}

int32_t MipiDsiCntlrTx(struct MipiDsiCntlr *cntlr, struct DsiCmdDesc *cmd)
{
    int32_t ret;

    if ((cntlr == NULL) || (cntlr->ops == NULL)) {
        HDF_LOGE("MipiDsiCntlrTx: cntlr or ops is null!");
        return HDF_ERR_INVALID_OBJECT;
    }
    if (cmd == NULL) {
        HDF_LOGE("MipiDsiCntlrTx: cmd is null!");
        return HDF_ERR_INVALID_OBJECT;
    }

    if (cntlr->ops->setCmd == NULL) {
        HDF_LOGE("MipiDsiCntlrTx: setCmd is null!");
        return HDF_ERR_NOT_SUPPORT;
    }

    (void)OsalMutexLock(&(cntlr->lock));
    ret = cntlr->ops->setCmd(cntlr, cmd);
    if (cmd->delay > 0) {
        OsalMSleep(cmd->delay);
    }
    (void)OsalMutexUnlock(&(cntlr->lock));

    if (ret != HDF_SUCCESS) {
        HDF_LOGE("MipiDsiCntlrTx: fail!");
    }

    return ret;
}

int32_t MipiDsiCntlrRx(struct MipiDsiCntlr *cntlr, struct DsiCmdDesc *cmd, int32_t readLen, uint8_t *out)
{
    int32_t ret;

    if ((cntlr == NULL) || (cntlr->ops == NULL)) {
        HDF_LOGE("MipiDsiCntlrRx: cntlr or ops is null!");
        return HDF_ERR_INVALID_OBJECT;
    }
    if ((cmd == NULL) || (out == NULL)) {
        HDF_LOGE("MipiDsiCntlrRx: cmd or out is null!");
        return HDF_ERR_INVALID_OBJECT;
    }

    if (cntlr->ops->getCmd == NULL) {
        HDF_LOGE("MipiDsiCntlrRx: getCmd is null!");
        return HDF_ERR_NOT_SUPPORT;
    }

    (void)OsalMutexLock(&(cntlr->lock));
    ret = cntlr->ops->getCmd(cntlr, cmd, readLen, out);
    (void)OsalMutexUnlock(&(cntlr->lock));

    if (ret != HDF_SUCCESS) {
        HDF_LOGE("MipiDsiCntlrRx: fail!");
    }

    return ret;
}

int32_t MipiDsiCntlrPowerControl(struct MipiDsiCntlr *cntlr, uint8_t enable)
{
    int32_t ret;

    if ((cntlr == NULL) || (cntlr->ops == NULL)) {
        HDF_LOGE("MipiDsiCntlrPowerControl: cntlr or ops is null!");
        return HDF_ERR_INVALID_OBJECT;
    }

    if (cntlr->ops->powerControl == NULL) {
        HDF_LOGE("MipiDsiCntlrPowerControl: powerControl is null!");
        return HDF_ERR_NOT_SUPPORT;
    }

    (void)OsalMutexLock(&(cntlr->lock));
    ret = cntlr->ops->powerControl(cntlr, enable);
    (void)OsalMutexUnlock(&(cntlr->lock));

    if (ret != HDF_SUCCESS) {
        HDF_LOGE("MipiDsiCntlrPowerControl: fail!");
    }

    return ret;
}

int32_t MipiDsiCntlrAttach(struct MipiDsiCntlr *cntlr, uint8_t *name)
{
    int32_t ret;

    if ((cntlr == NULL) || (cntlr->ops == NULL)) {
        HDF_LOGE("MipiDsiCntlrAttach: cntlr or ops is null!");
        return HDF_ERR_INVALID_OBJECT;
    }

    if (cntlr->ops->attach == NULL) {
        HDF_LOGE("MipiDsiCntlrAttach: attach is null!");
        return HDF_ERR_NOT_SUPPORT;
    }

    (void)OsalMutexLock(&(cntlr->lock));
    ret = cntlr->ops->attach(cntlr, name);
    (void)OsalMutexUnlock(&(cntlr->lock));

    if (ret != HDF_SUCCESS) {
        HDF_LOGE("MipiDsiCntlrAttach: fail!");
    }

    return ret;
}

int32_t MipiDsiCntlrSetDrvData(struct MipiDsiCntlr *cntlr, void *panelData)
{
    int32_t ret;

    if ((cntlr == NULL) || (cntlr->ops == NULL)) {
        HDF_LOGE("MipiDsiCntlrSetDrvData: cntlr or ops is null!");
        return HDF_ERR_INVALID_OBJECT;
    }

    if (cntlr->ops->setDrvData == NULL) {
        HDF_LOGE("MipiDsiCntlrSetDrvData: setDrvData is null!");
        return HDF_ERR_NOT_SUPPORT;
    }

    (void)OsalMutexLock(&(cntlr->lock));
    ret = cntlr->ops->setDrvData(cntlr, panelData);
    (void)OsalMutexUnlock(&(cntlr->lock));

    if (ret != HDF_SUCCESS) {
        HDF_LOGE("MipiDsiCntlrSetDrvData: fail!");
    }

    return ret;
}
