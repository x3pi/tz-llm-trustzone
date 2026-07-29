/*
 * Copyright (c) 2021-2023 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include "pin_if.h"
#include "pin_core.h"
#include "hdf_log.h"

#define HDF_LOG_TAG pin_if

DevHandle PinGet(const char *pinName)
{
    return (DevHandle)PinCntlrGetPinDescByName(pinName);
}

void PinPut(DevHandle handle)
{
    if (handle == NULL) {
        HDF_LOGE("PinPut: handle is null!");
        return;
    }
    return PinCntlrPutPin((struct PinDesc *)handle);
}

int32_t PinSetPull(DevHandle handle, enum PinPullType pullType)
{
    struct PinCntlr *cntlr = NULL;

    if (handle == NULL) {
        HDF_LOGE("PinSetPull: handle is null!");
        return HDF_ERR_INVALID_PARAM;
    }

    cntlr = PinCntlrGetByPin((struct PinDesc *)handle);
    return PinCntlrSetPinPull(cntlr, (struct PinDesc *)handle, pullType);
}

int32_t PinGetPull(DevHandle handle, enum PinPullType *pullType)
{
    struct PinCntlr *cntlr = NULL;

    if (handle == NULL || pullType == NULL) {
        HDF_LOGE("PinGetPull: handle or pullType is null!");
        return HDF_ERR_INVALID_PARAM;
    }

    cntlr = PinCntlrGetByPin((struct PinDesc *)handle);
    return PinCntlrGetPinPull(cntlr, (struct PinDesc *)handle, pullType);
}

int32_t PinSetStrength(DevHandle handle, uint32_t strength)
{
    struct PinCntlr *cntlr = NULL;

    if (handle == NULL) {
        HDF_LOGE("PinSetStrength: handle is null!");
        return HDF_ERR_INVALID_PARAM;
    }

    cntlr = PinCntlrGetByPin((struct PinDesc *)handle);
    return PinCntlrSetPinStrength(cntlr, (struct PinDesc *)handle, strength);
}

int32_t PinGetStrength(DevHandle handle, uint32_t *strength)
{
    struct PinCntlr *cntlr = NULL;

    if (handle == NULL || strength == NULL) {
        HDF_LOGE("PinGetStrength: handle or strength is null!");
        return HDF_ERR_INVALID_PARAM;
    }

    cntlr = PinCntlrGetByPin((struct PinDesc *)handle);
    return PinCntlrGetPinStrength(cntlr, (struct PinDesc *)handle, strength);
}

int32_t PinSetFunc(DevHandle handle, const char *funcName)
{
    struct PinCntlr *cntlr = NULL;

    if (handle == NULL || funcName == NULL) {
        HDF_LOGE("PinSetFunc: handle or funcName is null!");
        return HDF_ERR_INVALID_PARAM;
    }

    cntlr = PinCntlrGetByPin((struct PinDesc *)handle);
    return PinCntlrSetPinFunc(cntlr, (struct PinDesc *)handle, funcName);
}

int32_t PinGetFunc(DevHandle handle, const char **funcName)
{
    struct PinCntlr *cntlr = NULL;

    if (handle == NULL || funcName == NULL) {
        HDF_LOGE("PinGetFunc: handle or funcName is null!");
        return HDF_ERR_INVALID_PARAM;
    }

    cntlr = PinCntlrGetByPin((struct PinDesc *)handle);
    return PinCntlrGetPinFunc(cntlr, (struct PinDesc *)handle, funcName);
}