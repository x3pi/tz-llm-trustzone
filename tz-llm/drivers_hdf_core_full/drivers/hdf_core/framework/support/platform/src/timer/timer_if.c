/*
 * Copyright (c) 2022-2023 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */
#include "timer_if.h"
#include "hdf_log.h"
#include "timer_core.h"

DevHandle HwTimerOpen(const uint32_t number)
{
    return (DevHandle)TimerCntrlOpen(number);
}

void HwTimerClose(DevHandle handle)
{
    struct TimerCntrl *cntrl = (struct TimerCntrl *)handle;

    if (cntrl == NULL) {
        HDF_LOGE("HwTimerClose: cntrl is null!");
        return;
    }

    if (TimerCntrlClose(cntrl) != HDF_SUCCESS) {
        HDF_LOGE("HwTimerClose: timer cnltr close fail!");
        return;
    }
}

int32_t HwTimerSet(DevHandle handle, uint32_t useconds, TimerHandleCb cb)
{
    struct TimerCntrl *cntrl = (struct TimerCntrl *)handle;

    if (cntrl == NULL || cb == NULL) {
        HDF_LOGE("HwTimerSet: cntrl or cb is null!");
        return HDF_FAILURE;
    }

    if (TimerCntrlSet(cntrl, useconds, cb) != HDF_SUCCESS) {
        HDF_LOGE("HwTimerSet: timer cnltr set fail!");
        return HDF_FAILURE;
    }

    return HDF_SUCCESS;
}

int32_t HwTimerSetOnce(DevHandle handle, uint32_t useconds, TimerHandleCb cb)
{
    struct TimerCntrl *cntrl = (struct TimerCntrl *)handle;

    if (cntrl == NULL || cb == NULL) {
        HDF_LOGE("HwTimerSetOnce: cntrl or cb is null!");
        return HDF_FAILURE;
    }

    if (TimerCntrlSetOnce(cntrl, useconds, cb) != HDF_SUCCESS) {
        HDF_LOGE("HwTimerSetOnce: timer cntlr set once fail!");
        return HDF_FAILURE;
    }

    return HDF_SUCCESS;
}

int32_t HwTimerGet(DevHandle handle, uint32_t *useconds, bool *isPeriod)
{
    struct TimerCntrl *cntrl = (struct TimerCntrl *)handle;

    if (cntrl == NULL || useconds == NULL || isPeriod == NULL) {
        HDF_LOGE("HwTimerGet: cntlr or useconds or isPeriod is null!");
        return HDF_FAILURE;
    }

    if (TimerCntrlGet(cntrl, useconds, isPeriod) != HDF_SUCCESS) {
        HDF_LOGE("HwTimerGet: timer cntlr get fail!");
        return HDF_FAILURE;
    }

    return HDF_SUCCESS;
}

int32_t HwTimerStart(DevHandle handle)
{
    struct TimerCntrl *cntrl = (struct TimerCntrl *)handle;

    if (cntrl == NULL) {
        HDF_LOGE("HwTimerStart: cntrl is null!");
        return HDF_FAILURE;
    }

    if (TimerCntrlStart(cntrl) != HDF_SUCCESS) {
        HDF_LOGE("HwTimerStart: timer cntlr start fail!");
        return HDF_FAILURE;
    }

    return HDF_SUCCESS;
}

int32_t HwTimerStop(DevHandle handle)
{
    struct TimerCntrl *cntrl = (struct TimerCntrl *)handle;

    if (cntrl == NULL) {
        HDF_LOGE("HwTimerStop: cntrl is null!");
        return HDF_FAILURE;
    }

    if (TimerCntrlStop(cntrl) != HDF_SUCCESS) {
        HDF_LOGE("HwTimerStop: timer cntlr stop fail!");
        return HDF_FAILURE;
    }

    return HDF_SUCCESS;
}
