/*
 * Copyright (c) 2021-2023 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include "mipi_csi_core.h"
#include "hdf_log.h"

#define HDF_LOG_TAG mipi_csi_core

struct MipiCsiHandle {
    struct MipiCsiCntlr *cntlr;
    struct OsalMutex lock;
    void *priv;
};

static struct MipiCsiHandle g_mipiCsihandle[MAX_CNTLR_CNT];

int32_t MipiCsiRegisterCntlr(struct MipiCsiCntlr *cntlr, struct HdfDeviceObject *device)
{
    HDF_LOGI("MipiCsiRegisterCntlr: enter!");
    if (cntlr == NULL) {
        HDF_LOGE("MipiCsiRegisterCntlr: cntlr is null!");
        return HDF_ERR_INVALID_OBJECT;
    }
    if (cntlr->devNo >= MAX_CNTLR_CNT) {
        HDF_LOGE("MipiCsiRegisterCntlr: cntlr->devNo is error!");
        return HDF_ERR_INVALID_PARAM;
    }
    if (device == NULL) {
        HDF_LOGE("MipiCsiRegisterCntlr: device is null!");
        return HDF_ERR_INVALID_OBJECT;
    }

    if (g_mipiCsihandle[cntlr->devNo].cntlr == NULL) {
        (void)OsalMutexInit(&g_mipiCsihandle[cntlr->devNo].lock);
        (void)OsalMutexInit(&(cntlr->lock));

        g_mipiCsihandle[cntlr->devNo].cntlr = cntlr;
        g_mipiCsihandle[cntlr->devNo].priv = NULL;
        cntlr->device = device;
        device->service = &(cntlr->service);
        cntlr->priv = NULL;
        HDF_LOGI("MipiCsiRegisterCntlr: success!");

        return HDF_SUCCESS;
    }

    HDF_LOGE("MipiCsiRegisterCntlr: cntlr already exists!");
    return HDF_FAILURE;
}

void MipiCsiUnregisterCntlr(struct MipiCsiCntlr *cntlr)
{
    if (cntlr == NULL) {
        HDF_LOGE("MipiCsiUnregisterCntlr: cntlr is null!");
        return;
    }

    (void)OsalMutexDestroy(&(cntlr->lock));
    (void)OsalMutexDestroy(&(g_mipiCsihandle[cntlr->devNo].lock));

    HDF_LOGI("MipiCsiUnregisterCntlr: success!");
    return;
}

struct MipiCsiCntlr *MipiCsiCntlrFromDevice(const struct HdfDeviceObject *device)
{
    return (device == NULL) ? NULL : (struct MipiCsiCntlr *)device->service;
}

struct MipiCsiCntlr *MipiCsiCntlrGet(uint8_t number)
{
    struct MipiCsiCntlr *cntlr = NULL;
    HDF_LOGI("MipiCsiCntlrGet: enter!");

    if (number >= MAX_CNTLR_CNT) {
        HDF_LOGE("MipiCsiCntlrGet: invalid number!");
        return NULL;
    }
    if (g_mipiCsihandle[number].cntlr == NULL) {
        HDF_LOGE("MipiCsiCntlrGet: g_mipiCsihandle[number].cntlr is null!");
        return NULL;
    }

    (void)OsalMutexLock(&(g_mipiCsihandle[number].lock));
    g_mipiCsihandle[number].cntlr->devNo = number;
    cntlr = g_mipiCsihandle[number].cntlr;
    (void)OsalMutexUnlock(&(g_mipiCsihandle[number].lock));

    return cntlr;
}

void MipiCsiCntlrPut(const struct MipiCsiCntlr *cntlr)
{
    uint8_t number;

    if (cntlr == NULL) {
        HDF_LOGE("MipiCsiCntlrPut: cntlr is null!");
        return;
    }

    number = cntlr->devNo;
    if (number >= MAX_CNTLR_CNT) {
        HDF_LOGE("MipiCsiCntlrPut: invalid number!");
        return;
    }

    HDF_LOGI("MipiCsiCntlrPut: success!");
}

int32_t MipiCsiCntlrSetComboDevAttr(struct MipiCsiCntlr *cntlr, ComboDevAttr *pAttr)
{
    int32_t ret;
    HDF_LOGI("MipiCsiCntlrSetComboDevAttr: enter!");

    if ((cntlr == NULL) || (cntlr->ops == NULL)) {
        HDF_LOGE("MipiCsiCntlrSetComboDevAttr: cntlr or ops is null!");
        return HDF_ERR_INVALID_OBJECT;
    }
    if (pAttr == NULL) {
        HDF_LOGE("MipiCsiCntlrSetComboDevAttr: pAttr is null!");
        return HDF_ERR_INVALID_OBJECT;
    }
    if (cntlr->ops->setComboDevAttr == NULL) {
        HDF_LOGE("MipiCsiCntlrSetComboDevAttr: setComboDevAttr is null!");
        return HDF_ERR_NOT_SUPPORT;
    }

    (void)OsalMutexLock(&(cntlr->lock));
    ret = cntlr->ops->setComboDevAttr(cntlr, pAttr);
    (void)OsalMutexUnlock(&(cntlr->lock));

    if (ret == HDF_SUCCESS) {
        HDF_LOGI("MipiCsiCntlrSetComboDevAttr: success!");
    } else {
        HDF_LOGE("MipiCsiCntlrSetComboDevAttr: fail!");
    }

    return ret;
}

int32_t MipiCsiCntlrSetPhyCmvmode(struct MipiCsiCntlr *cntlr, uint8_t devno, PhyCmvMode cmvMode)
{
    int32_t ret;
    HDF_LOGI("MipiCsiCntlrSetPhyCmvmode: enter!");

    if ((cntlr == NULL) || (cntlr->ops == NULL)) {
        HDF_LOGE("MipiCsiCntlrSetPhyCmvmode: cntlr or ops is null!");
        return HDF_ERR_INVALID_OBJECT;
    }
    if (cntlr->ops->setPhyCmvmode == NULL) {
        HDF_LOGE("MipiCsiCntlrSetPhyCmvmode: setPhyCmvmode is null!");
        return HDF_ERR_NOT_SUPPORT;
    }

    (void)OsalMutexLock(&(cntlr->lock));
    ret = cntlr->ops->setPhyCmvmode(cntlr, devno, cmvMode);
    (void)OsalMutexUnlock(&(cntlr->lock));

    if (ret == HDF_SUCCESS) {
        HDF_LOGI("MipiCsiCntlrSetPhyCmvmode: success!");
    } else {
        HDF_LOGE("MipiCsiCntlrSetPhyCmvmode: fail!");
    }

    return ret;
}

int32_t MipiCsiCntlrSetExtDataType(struct MipiCsiCntlr *cntlr, ExtDataType* dataType)
{
    int32_t ret;
    HDF_LOGI("MipiCsiCntlrSetExtDataType: enter!");

    if ((cntlr == NULL) || (cntlr->ops == NULL)) {
        HDF_LOGE("MipiCsiCntlrSetExtDataType: cntlr or ops is null!");
        return HDF_ERR_INVALID_OBJECT;
    }
    if (dataType == NULL) {
        HDF_LOGE("MipiCsiCntlrSetExtDataType: dataType is null!");
        return HDF_FAILURE;
    }
    if (cntlr->ops->setExtDataType == NULL) {
        HDF_LOGE("MipiCsiCntlrSetExtDataType: setExtDataType is null!");
        return HDF_ERR_NOT_SUPPORT;
    }

    (void)OsalMutexLock(&(cntlr->lock));
    ret = cntlr->ops->setExtDataType(cntlr, dataType);
    (void)OsalMutexUnlock(&(cntlr->lock));

    if (ret == HDF_SUCCESS) {
        HDF_LOGI("MipiCsiCntlrSetExtDataType: success!");
    } else {
        HDF_LOGE("MipiCsiCntlrSetExtDataType: fail!");
    }

    return ret;
}

int32_t MipiCsiCntlrSetHsMode(struct MipiCsiCntlr *cntlr, LaneDivideMode laneDivideMode)
{
    int32_t ret;
    HDF_LOGI("MipiCsiCntlrSetHsMode: enter!");

    if ((cntlr == NULL) || (cntlr->ops == NULL)) {
        HDF_LOGE("MipiCsiCntlrSetHsMode: cntlr or ops is null!");
        return HDF_ERR_INVALID_OBJECT;
    }
    if (cntlr->ops->setHsMode == NULL) {
        HDF_LOGE("MipiCsiCntlrSetHsMode: setHsMode is null!");
        return HDF_ERR_NOT_SUPPORT;
    }

    (void)OsalMutexLock(&(cntlr->lock));
    ret = cntlr->ops->setHsMode(cntlr, laneDivideMode);
    (void)OsalMutexUnlock(&(cntlr->lock));

    if (ret == HDF_SUCCESS) {
        HDF_LOGI("MipiCsiCntlrSetHsMode: success!");
    } else {
        HDF_LOGE("MipiCsiCntlrSetHsMode: fail!");
    }

    return ret;
}

int32_t MipiCsiCntlrEnableClock(struct MipiCsiCntlr *cntlr, uint8_t comboDev)
{
    int32_t ret;
    HDF_LOGI("MipiCsiCntlrEnableClock: enter!");

    if ((cntlr == NULL) || (cntlr->ops == NULL)) {
        HDF_LOGE("MipiCsiCntlrEnableClock: cntlr or ops is null!");
        return HDF_ERR_INVALID_OBJECT;
    }
    if (cntlr->ops->enableClock == NULL) {
        HDF_LOGE("MipiCsiCntlrEnableClock: enableClock is null!");
        return HDF_ERR_NOT_SUPPORT;
    }

    (void)OsalMutexLock(&(cntlr->lock));
    ret = cntlr->ops->enableClock(cntlr, comboDev);
    (void)OsalMutexUnlock(&(cntlr->lock));

    if (ret == HDF_SUCCESS) {
        HDF_LOGI("MipiCsiCntlrEnableClock: success!");
    } else {
        HDF_LOGE("MipiCsiCntlrEnableClock: fail!");
    }

    return ret;
}

int32_t MipiCsiCntlrDisableClock(struct MipiCsiCntlr *cntlr, uint8_t comboDev)
{
    int32_t ret;
    HDF_LOGI("MipiCsiCntlrDisableClock: enter!");

    if ((cntlr == NULL) || (cntlr->ops == NULL)) {
        HDF_LOGE("MipiCsiCntlrDisableClock: cntlr or ops is null!");
        return HDF_ERR_INVALID_OBJECT;
    }
    if (cntlr->ops->disableClock == NULL) {
        HDF_LOGE("MipiCsiCntlrDisableClock: disableClock is null!");
        return HDF_ERR_NOT_SUPPORT;
    }

    (void)OsalMutexLock(&(cntlr->lock));
    ret = cntlr->ops->disableClock(cntlr, comboDev);
    (void)OsalMutexUnlock(&(cntlr->lock));

    if (ret == HDF_SUCCESS) {
        HDF_LOGI("MipiCsiCntlrDisableClock: success!");
    } else {
        HDF_LOGE("MipiCsiCntlrDisableClock: fail!");
    }

    return ret;
}

int32_t MipiCsiCntlrResetRx(struct MipiCsiCntlr *cntlr, uint8_t comboDev)
{
    int32_t ret;
    HDF_LOGI("MipiCsiCntlrResetRx: enter!");

    if ((cntlr == NULL) || (cntlr->ops == NULL)) {
        HDF_LOGE("MipiCsiCntlrResetRx: cntlr or ops is null!");
        return HDF_ERR_INVALID_OBJECT;
    }
    if (cntlr->ops->resetRx == NULL) {
        HDF_LOGE("MipiCsiCntlrResetRx: resetRx is null!");
        return HDF_ERR_NOT_SUPPORT;
    }

    (void)OsalMutexLock(&(cntlr->lock));
    ret = cntlr->ops->resetRx(cntlr, comboDev);
    (void)OsalMutexUnlock(&(cntlr->lock));

    if (ret == HDF_SUCCESS) {
        HDF_LOGI("MipiCsiCntlrResetRx: success!");
    } else {
        HDF_LOGE("MipiCsiCntlrResetRx: fail!");
    }

    return ret;
}

int32_t MipiCsiCntlrUnresetRx(struct MipiCsiCntlr *cntlr, uint8_t comboDev)
{
    int32_t ret;
    HDF_LOGI("MipiCsiCntlrUnresetRx: enter!");

    if ((cntlr == NULL) || (cntlr->ops == NULL)) {
        HDF_LOGE("MipiCsiCntlrUnresetRx: cntlr or ops is null!");
        return HDF_ERR_INVALID_OBJECT;
    }
    if (cntlr->ops->unresetRx == NULL) {
        HDF_LOGE("MipiCsiCntlrUnresetRx: unresetRx is null!");
        return HDF_ERR_NOT_SUPPORT;
    }

    (void)OsalMutexLock(&(cntlr->lock));
    ret = cntlr->ops->unresetRx(cntlr, comboDev);
    (void)OsalMutexUnlock(&(cntlr->lock));

    if (ret == HDF_SUCCESS) {
        HDF_LOGI("MipiCsiCntlrUnresetRx: success!");
    } else {
        HDF_LOGE("MipiCsiCntlrUnresetRx: fail!");
    }

    return ret;
}

int32_t MipiCsiCntlrEnableSensorClock(struct MipiCsiCntlr *cntlr, uint8_t snsClkSource)
{
    int32_t ret;
    HDF_LOGI("MipiCsiCntlrEnableSensorClock: enter!");

    if ((cntlr == NULL) || (cntlr->ops == NULL)) {
        HDF_LOGE("MipiCsiCntlrEnableSensorClock: cntlr or ops is null!");
        return HDF_ERR_INVALID_OBJECT;
    }
    if (cntlr->ops->enableSensorClock == NULL) {
        HDF_LOGE("MipiCsiCntlrEnableSensorClock: enableSensorClock is null!");
        return HDF_ERR_NOT_SUPPORT;
    }

    (void)OsalMutexLock(&(cntlr->lock));
    ret = cntlr->ops->enableSensorClock(cntlr, snsClkSource);
    (void)OsalMutexUnlock(&(cntlr->lock));

    if (ret == HDF_SUCCESS) {
        HDF_LOGI("MipiCsiCntlrEnableSensorClock: success!");
    } else {
        HDF_LOGE("MipiCsiCntlrEnableSensorClock: fail!");
    }

    return ret;
}

int32_t MipiCsiCntlrDisableSensorClock(struct MipiCsiCntlr *cntlr, uint8_t snsClkSource)
{
    int32_t ret;
    HDF_LOGI("MipiCsiCntlrDisableSensorClock: enter!");

    if ((cntlr == NULL) || (cntlr->ops == NULL)) {
        HDF_LOGE("MipiCsiCntlrDisableSensorClock: cntlr or ops is null!");
        return HDF_ERR_INVALID_OBJECT;
    }
    if (cntlr->ops->disableSensorClock == NULL) {
        HDF_LOGE("MipiCsiCntlrDisableSensorClock: disableSensorClock is null!");
        return HDF_ERR_NOT_SUPPORT;
    }

    (void)OsalMutexLock(&(cntlr->lock));
    ret = cntlr->ops->disableSensorClock(cntlr, snsClkSource);
    (void)OsalMutexUnlock(&(cntlr->lock));

    if (ret == HDF_SUCCESS) {
        HDF_LOGI("MipiCsiCntlrDisableSensorClock: success!");
    } else {
        HDF_LOGE("MipiCsiCntlrDisableSensorClock: fail!");
    }

    return ret;
}

int32_t MipiCsiCntlrResetSensor(struct MipiCsiCntlr *cntlr, uint8_t snsResetSource)
{
    int32_t ret;
    HDF_LOGI("MipiCsiCntlrResetSensor: enter!");

    if ((cntlr == NULL) || (cntlr->ops == NULL)) {
        HDF_LOGE("MipiCsiCntlrResetSensor: cntlr or ops is null!");
        return HDF_ERR_INVALID_OBJECT;
    }
    if (cntlr->ops->resetSensor == NULL) {
        HDF_LOGE("MipiCsiCntlrResetSensor: resetSensor is null!");
        return HDF_ERR_NOT_SUPPORT;
    }

    (void)OsalMutexLock(&(cntlr->lock));
    ret = cntlr->ops->resetSensor(cntlr, snsResetSource);
    (void)OsalMutexUnlock(&(cntlr->lock));

    if (ret == HDF_SUCCESS) {
        HDF_LOGI("MipiCsiCntlrResetSensor: success!");
    } else {
        HDF_LOGE("MipiCsiCntlrResetSensor: fail!");
    }

    return ret;
}

int32_t MipiCsiCntlrUnresetSensor(struct MipiCsiCntlr *cntlr, uint8_t snsResetSource)
{
    int32_t ret;
    HDF_LOGI("MipiCsiCntlrUnresetSensor: enter!");

    if ((cntlr == NULL) || (cntlr->ops == NULL)) {
        HDF_LOGE("MipiCsiCntlrUnresetSensor: cntlr or ops is null!");
        return HDF_ERR_INVALID_OBJECT;
    }
    if (cntlr->ops->unresetSensor == NULL) {
        HDF_LOGE("MipiCsiCntlrUnresetSensor: unresetSensor is null!");
        return HDF_ERR_NOT_SUPPORT;
    }

    (void)OsalMutexLock(&(cntlr->lock));
    ret = cntlr->ops->unresetSensor(cntlr, snsResetSource);
    (void)OsalMutexUnlock(&(cntlr->lock));

    if (ret == HDF_SUCCESS) {
        HDF_LOGI("MipiCsiCntlrUnresetSensor: success!");
    } else {
        HDF_LOGE("MipiCsiCntlrUnresetSensor: fail!");
    }

    return ret;
}

int32_t MipiCsiCntlrSetDrvData(struct MipiCsiCntlr *cntlr, void *drvData)
{
    int32_t ret;

    if ((cntlr == NULL) || (cntlr->ops == NULL)) {
        HDF_LOGE("MipiCsiCntlrSetDrvData: cntlr or ops is null!");
        return HDF_ERR_INVALID_OBJECT;
    }

    if (cntlr->ops->setDrvData == NULL) {
        HDF_LOGE("MipiCsiCntlrSetDrvData: setDrvData is null!");
        return HDF_ERR_NOT_SUPPORT;
    }

    (void)OsalMutexLock(&(cntlr->lock));
    ret = cntlr->ops->setDrvData(cntlr, drvData);
    (void)OsalMutexUnlock(&(cntlr->lock));

    if (ret == HDF_SUCCESS) {
        HDF_LOGI("MipiCsiCntlrSetDrvData: success!");
    } else {
        HDF_LOGE("MipiCsiCntlrSetDrvData: fail!");
    }

    return ret;
}

int32_t MipiCsiDebugGetMipiDevCtx(struct MipiCsiCntlr *cntlr, MipiDevCtx *ctx)
{
    if (cntlr == NULL) {
        HDF_LOGE("MipiCsiDebugGetMipiDevCtx: cntlr is null!");
        return HDF_ERR_INVALID_OBJECT;
    }
    if (cntlr->debugs == NULL) {
        HDF_LOGE("MipiCsiDebugGetMipiDevCtx: debugs is null!");
        return HDF_ERR_NOT_SUPPORT;
    }
    if (cntlr->debugs->getMipiDevCtx == NULL) {
        HDF_LOGE("MipiCsiDebugGetMipiDevCtx: getMipiDevCtx is null!");
        return HDF_ERR_NOT_SUPPORT;
    }
    if (ctx == NULL) {
        HDF_LOGE("MipiCsiDebugGetMipiDevCtx: ctx is null!");
        return HDF_ERR_INVALID_OBJECT;
    }

    (void)OsalMutexLock(&(cntlr->lock));
    cntlr->debugs->getMipiDevCtx(cntlr, ctx);
    (void)OsalMutexUnlock(&(cntlr->lock));
    HDF_LOGI("MipiCsiDebugGetMipiDevCtx: success!");

    return HDF_SUCCESS;
}

int32_t MipiCsiDebugGetPhyErrIntCnt(struct MipiCsiCntlr *cntlr, unsigned int phyId, PhyErrIntCnt *errInfo)
{
    if (cntlr == NULL) {
        HDF_LOGE("MipiCsiDebugGetPhyErrIntCnt: cntlr is null!");
        return HDF_ERR_INVALID_OBJECT;
    }
    if (cntlr->debugs == NULL) {
        HDF_LOGE("MipiCsiDebugGetPhyErrIntCnt: debugs is null!");
        return HDF_ERR_NOT_SUPPORT;
    }
    if (cntlr->debugs->getPhyErrIntCnt == NULL) {
        HDF_LOGE("MipiCsiDebugGetPhyErrIntCnt: getPhyErrIntCnt is null!");
        return HDF_ERR_NOT_SUPPORT;
    }
    if (errInfo == NULL) {
        HDF_LOGE("MipiCsiDebugGetPhyErrIntCnt: errInfo is null!");
        return HDF_ERR_INVALID_OBJECT;
    }

    (void)OsalMutexLock(&(cntlr->lock));
    cntlr->debugs->getPhyErrIntCnt(cntlr, phyId, errInfo);
    (void)OsalMutexUnlock(&(cntlr->lock));
    HDF_LOGI("MipiCsiDebugGetPhyErrIntCnt: success!");

    return HDF_SUCCESS;
}

int32_t MipiCsiDebugGetMipiErrInt(struct MipiCsiCntlr *cntlr, unsigned int phyId, MipiErrIntCnt *errInfo)
{
    if (cntlr == NULL) {
        HDF_LOGE("MipiCsiDebugGetMipiErrInt: cntlr is null!");
        return HDF_ERR_INVALID_OBJECT;
    }
    if (cntlr->debugs == NULL) {
        HDF_LOGE("MipiCsiDebugGetMipiErrInt: debugs is null!");
        return HDF_ERR_NOT_SUPPORT;
    }
    if (cntlr->debugs->getMipiErrInt == NULL) {
        HDF_LOGE("MipiCsiDebugGetMipiErrInt: getMipiErrInt is null!");
        return HDF_ERR_NOT_SUPPORT;
    }
    if (errInfo == NULL) {
        HDF_LOGE("MipiCsiDebugGetMipiErrInt: errInfo is null!");
        return HDF_ERR_INVALID_OBJECT;
    }

    (void)OsalMutexLock(&(cntlr->lock));
    cntlr->debugs->getMipiErrInt(cntlr, phyId, errInfo);
    (void)OsalMutexUnlock(&(cntlr->lock));
    HDF_LOGI("MipiCsiDebugGetMipiErrInt: success!");

    return HDF_SUCCESS;
}

int32_t MipiCsiDebugGetLvdsErrIntCnt(struct MipiCsiCntlr *cntlr, unsigned int phyId, LvdsErrIntCnt *errInfo)
{
    if (cntlr == NULL) {
        HDF_LOGE("MipiCsiDebugGetLvdsErrIntCnt: cntlr is null!");
        return HDF_ERR_INVALID_OBJECT;
    }
    if (cntlr->debugs == NULL) {
        HDF_LOGE("MipiCsiDebugGetLvdsErrIntCnt: debugs is null!");
        return HDF_ERR_NOT_SUPPORT;
    }
    if (cntlr->debugs->getLvdsErrIntCnt == NULL) {
        HDF_LOGE("MipiCsiDebugGetLvdsErrIntCnt: getLvdsErrIntCnt is null!");
        return HDF_ERR_NOT_SUPPORT;
    }
    if (errInfo == NULL) {
        HDF_LOGE("MipiCsiDebugGetLvdsErrIntCnt: errInfo is null!");
        return HDF_ERR_INVALID_OBJECT;
    }

    (void)OsalMutexLock(&(cntlr->lock));
    cntlr->debugs->getLvdsErrIntCnt(cntlr, phyId, errInfo);
    (void)OsalMutexUnlock(&(cntlr->lock));
    HDF_LOGI("MipiCsiDebugGetLvdsErrIntCnt: success!");

    return HDF_SUCCESS;
}

int32_t MipiCsiDebugGetAlignErrIntCnt(struct MipiCsiCntlr *cntlr, unsigned int phyId, AlignErrIntCnt *errInfo)
{
    if (cntlr == NULL) {
        HDF_LOGE("MipiCsiDebugGetAlignErrIntCnt: cntlr is null!");
        return HDF_ERR_INVALID_OBJECT;
    }
    if (cntlr->debugs == NULL) {
        HDF_LOGE("MipiCsiDebugGetAlignErrIntCnt: debugs is null!");
        return HDF_ERR_NOT_SUPPORT;
    }
    if (cntlr->debugs->getAlignErrIntCnt == NULL) {
        HDF_LOGE("MipiCsiDebugGetAlignErrIntCnt: getAlignErrIntCnt is null!");
        return HDF_ERR_NOT_SUPPORT;
    }
    if (errInfo == NULL) {
        HDF_LOGE("MipiCsiDebugGetAlignErrIntCnt: errInfo is null!");
        return HDF_ERR_INVALID_OBJECT;
    }

    (void)OsalMutexLock(&(cntlr->lock));
    cntlr->debugs->getAlignErrIntCnt(cntlr, phyId, errInfo);
    (void)OsalMutexUnlock(&(cntlr->lock));
    HDF_LOGI("MipiCsiDebugGetAlignErrIntCnt: success!");

    return HDF_SUCCESS;
}

int32_t MipiCsiDebugGetPhyData(struct MipiCsiCntlr *cntlr, int phyId, int laneId, unsigned int *laneData)
{
    if (cntlr == NULL) {
        HDF_LOGE("MipiCsiDebugGetPhyData: cntlr is null!");
        return HDF_ERR_INVALID_OBJECT;
    }
    if (cntlr->debugs == NULL) {
        HDF_LOGE("MipiCsiDebugGetPhyData: debugs is null!");
        return HDF_ERR_NOT_SUPPORT;
    }
    if (cntlr->debugs->getPhyData == NULL) {
        HDF_LOGE("MipiCsiDebugGetPhyData: getPhyData is null!");
        return HDF_ERR_NOT_SUPPORT;
    }
    if (laneData == NULL) {
        HDF_LOGE("MipiCsiDebugGetPhyData: laneData is null!");
        return HDF_ERR_INVALID_OBJECT;
    }

    (void)OsalMutexLock(&(cntlr->lock));
    cntlr->debugs->getPhyData(cntlr, phyId, laneId, laneData);
    (void)OsalMutexUnlock(&(cntlr->lock));
    HDF_LOGI("MipiCsiDebugGetPhyData: success!");

    return HDF_SUCCESS;
}

int32_t MipiCsiDebugGetPhyMipiLinkData(struct MipiCsiCntlr *cntlr, int phyId, int laneId, unsigned int *laneData)
{
    if (cntlr == NULL) {
        HDF_LOGE("MipiCsiDebugGetPhyMipiLinkData: cntlr is null!");
        return HDF_ERR_INVALID_OBJECT;
    }
    if (cntlr->debugs == NULL) {
        HDF_LOGE("MipiCsiDebugGetPhyMipiLinkData: debugs is null!");
        return HDF_ERR_NOT_SUPPORT;
    }
    if (cntlr->debugs->getPhyMipiLinkData == NULL) {
        HDF_LOGE("MipiCsiDebugGetPhyMipiLinkData: getPhyMipiLinkData is null!");
        return HDF_ERR_NOT_SUPPORT;
    }
    if (laneData == NULL) {
        HDF_LOGE("MipiCsiDebugGetPhyMipiLinkData: laneData is null!");
        return HDF_ERR_INVALID_OBJECT;
    }

    (void)OsalMutexLock(&(cntlr->lock));
    cntlr->debugs->getPhyMipiLinkData(cntlr, phyId, laneId, laneData);
    (void)OsalMutexUnlock(&(cntlr->lock));
    HDF_LOGI("MipiCsiDebugGetPhyMipiLinkData: success!");

    return HDF_SUCCESS;
}

int32_t MipiCsiDebugGetPhyLvdsLinkData(struct MipiCsiCntlr *cntlr, int phyId, int laneId, unsigned int *laneData)
{
    if (cntlr == NULL) {
        HDF_LOGE("MipiCsiDebugGetPhyLvdsLinkData: cntlr is null!");
        return HDF_ERR_INVALID_OBJECT;
    }
    if (cntlr->debugs == NULL) {
        HDF_LOGE("MipiCsiDebugGetPhyLvdsLinkData: debugs is null!");
        return HDF_ERR_NOT_SUPPORT;
    }
    if (cntlr->debugs->getPhyLvdsLinkData == NULL) {
        HDF_LOGE("MipiCsiDebugGetPhyLvdsLinkData: getPhyLvdsLinkData is null!");
        return HDF_ERR_NOT_SUPPORT;
    }
    if (laneData == NULL) {
        HDF_LOGE("MipiCsiDebugGetPhyLvdsLinkData: laneData is null!");
        return HDF_ERR_INVALID_OBJECT;
    }

    (void)OsalMutexLock(&(cntlr->lock));
    cntlr->debugs->getPhyLvdsLinkData(cntlr, phyId, laneId, laneData);
    (void)OsalMutexUnlock(&(cntlr->lock));
    HDF_LOGI("MipiCsiDebugGetPhyLvdsLinkData: success!");

    return HDF_SUCCESS;
}

int32_t MipiCsiDebugGetMipiImgsizeStatis(struct MipiCsiCntlr *cntlr, uint8_t devno, short vc, ImgSize *pSize)
{
    if (cntlr == NULL) {
        HDF_LOGE("MipiCsiDebugGetMipiImgsizeStatis: cntlr is null!");
        return HDF_ERR_INVALID_OBJECT;
    }
    if (cntlr->debugs == NULL) {
        HDF_LOGE("MipiCsiDebugGetMipiImgsizeStatis: debugs is null!");
        return HDF_ERR_NOT_SUPPORT;
    }
    if (cntlr->debugs->getMipiImgsizeStatis == NULL) {
        HDF_LOGE("MipiCsiDebugGetMipiImgsizeStatis: getMipiImgsizeStatis is null!");
        return HDF_ERR_NOT_SUPPORT;
    }
    if (pSize == NULL) {
        HDF_LOGE("MipiCsiDebugGetMipiImgsizeStatis: pSize is null!");
        return HDF_ERR_INVALID_OBJECT;
    }

    (void)OsalMutexLock(&(cntlr->lock));
    cntlr->debugs->getMipiImgsizeStatis(cntlr, devno, vc, pSize);
    (void)OsalMutexUnlock(&(cntlr->lock));
    HDF_LOGI("MipiCsiDebugGetMipiImgsizeStatis: success!");

    return HDF_SUCCESS;
}

int32_t MipiCsiDebugGetLvdsImgsizeStatis(struct MipiCsiCntlr *cntlr, uint8_t devno, short vc, ImgSize *pSize)
{
    if (cntlr == NULL) {
        HDF_LOGE("MipiCsiDebugGetLvdsImgsizeStatis: cntlr is null!");
        return HDF_ERR_INVALID_OBJECT;
    }
    if (cntlr->debugs == NULL) {
        HDF_LOGE("MipiCsiDebugGetLvdsImgsizeStatis: debugs is null!");
        return HDF_ERR_NOT_SUPPORT;
    }
    if (cntlr->debugs->getLvdsImgsizeStatis == NULL) {
        HDF_LOGE("MipiCsiDebugGetLvdsImgsizeStatis: getLvdsImgsizeStatis is null!");
        return HDF_ERR_NOT_SUPPORT;
    }
    if (pSize == NULL) {
        HDF_LOGE("MipiCsiDebugGetLvdsImgsizeStatis: pSize is null!");
        return HDF_ERR_INVALID_OBJECT;
    }

    (void)OsalMutexLock(&(cntlr->lock));
    cntlr->debugs->getLvdsImgsizeStatis(cntlr, devno, vc, pSize);
    (void)OsalMutexUnlock(&(cntlr->lock));
    HDF_LOGI("MipiCsiDebugGetLvdsImgsizeStatis: success!");

    return HDF_SUCCESS;
}

int32_t MipiCsiDebugGetLvdsLaneImgsizeStatis(struct MipiCsiCntlr *cntlr, uint8_t devno, short lane, ImgSize *pSize)
{
    if (cntlr == NULL) {
        HDF_LOGE("MipiCsiDebugGetLvdsLaneImgsizeStatis: cntlr is null!");
        return HDF_ERR_INVALID_OBJECT;
    }
    if (cntlr->debugs == NULL) {
        HDF_LOGE("MipiCsiDebugGetLvdsLaneImgsizeStatis: debugs is null!");
        return HDF_ERR_NOT_SUPPORT;
    }
    if (cntlr->debugs->getLvdsLaneImgsizeStatis == NULL) {
        HDF_LOGE("MipiCsiDebugGetLvdsLaneImgsizeStatis: getLvdsLaneImgsizeStatis is null!");
        return HDF_ERR_NOT_SUPPORT;
    }
    if (pSize == NULL) {
        HDF_LOGE("MipiCsiDebugGetLvdsLaneImgsizeStatis: pSize is null!");
        return HDF_ERR_INVALID_OBJECT;
    }

    (void)OsalMutexLock(&(cntlr->lock));
    cntlr->debugs->getLvdsLaneImgsizeStatis(cntlr, devno, lane, pSize);
    (void)OsalMutexUnlock(&(cntlr->lock));
    HDF_LOGI("MipiCsiDebugGetLvdsLaneImgsizeStatis: success!");

    return HDF_SUCCESS;
}