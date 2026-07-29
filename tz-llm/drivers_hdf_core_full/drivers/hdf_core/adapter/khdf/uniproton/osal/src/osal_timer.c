/*
 * Copyright (c) 2022-2023 Huawei Device Co., Ltd. All rights reserved.
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

#include "osal_timer.h"
#include "hdf_log.h"
#include "prt_timer.h"
#include "osal_mem.h"

#define HDF_LOG_TAG osal_timer
#define OSAL_INVALID_TIMER_ID UINT32_MAX

struct OsalLitetimer {
    uintptr_t arg;
    TimerHandle timerID;
    OsalTimerFunc func;
    uint32_t interval;
};

int32_t OsalTimerCreate(OsalTimer *timer, uint32_t interval, OsalTimerFunc func, uintptr_t arg)
{
    struct OsalLitetimer *liteTimer = NULL;

    if (func == NULL || timer == NULL || interval == 0) {
        HDF_LOGE("%s invalid para", __func__);
        return HDF_ERR_INVALID_PARAM;
    }

    timer->realTimer = NULL;

    liteTimer = (struct OsalLitetimer *)OsalMemCalloc(sizeof(*liteTimer));
    if (liteTimer == NULL) {
        HDF_LOGE("%s malloc fail", __func__);
        return HDF_ERR_MALLOC_FAIL;
    }
    liteTimer->timerID = OSAL_INVALID_TIMER_ID;
    liteTimer->arg = arg;
    liteTimer->func = func;
    liteTimer->interval = interval;
    timer->realTimer = (void *)liteTimer;

    return HDF_SUCCESS;
}
static int32_t OsalStartTimer(OsalTimer *timer, U8 mode)
{
    uint32_t ret;
    TimerHandle timerID = 0;
    struct OsalLitetimer *liteTimer = NULL;
    struct TimerCreatePara timerPara = {0};

    if (timer == NULL || timer->realTimer == NULL) {
        HDF_LOGE("%s invalid para %d", __func__, __LINE__);
        return HDF_ERR_INVALID_PARAM;
    }

    liteTimer = (struct OsalLitetimer *)timer->realTimer;
    if (liteTimer->interval == 0 || liteTimer->func == NULL) {
        HDF_LOGE("%s invalid para %d", __func__, __LINE__);
        return HDF_ERR_INVALID_PARAM;
    }

    timerPara.type = OS_TIMER_SOFTWARE;
    timerPara.mode = mode;
    timerPara.interval = liteTimer->interval;
    timerPara.callBackFunc = (TmrProcFunc)liteTimer->func;
    timerPara.arg1 = liteTimer->arg;
    ret = PRT_TimerCreate(&timerPara, &timerID);
    if (ret != OS_OK) {
        HDF_LOGE("%s PRT_TimerCreate fail %u", __func__, ret);
        return HDF_FAILURE;
    }

    ret = PRT_TimerStart(0, timerID);
    if (ret != OS_OK) {
        (void)PRT_TimerDelete(0, timerID);
        HDF_LOGE("%s PRT_TimerStart fail %u", __func__, ret);
        return HDF_FAILURE;
    }

    liteTimer->timerID = timerID;
    return HDF_SUCCESS;
}

int32_t OsalTimerStartLoop(OsalTimer *timer)
{
    return OsalStartTimer(timer, OS_TIMER_LOOP);
}

int32_t OsalTimerStartOnce(OsalTimer *timer)
{
    return OsalStartTimer(timer, OS_TIMER_ONCE);
}

int32_t OsalTimerSetTimeout(OsalTimer *timer, uint32_t interval)
{
    struct OsalLitetimer *liteTimer = NULL;
    uint32_t ret;

    if (timer == NULL || timer->realTimer == NULL || interval == 0) {
        HDF_LOGE("%s invalid para", __func__);
        return HDF_ERR_INVALID_PARAM;
    }

    liteTimer = (struct OsalLitetimer *)timer->realTimer;
    if (liteTimer->timerID == OSAL_INVALID_TIMER_ID) {
        HDF_LOGE("%s timer id invalid %u", __func__, liteTimer->timerID);
        return HDF_FAILURE;
    }

    if (liteTimer->interval == interval) {
        return HDF_SUCCESS;
    }

    liteTimer->interval = interval;
    ret = PRT_TimerDelete(0, liteTimer->timerID);
    if (ret != OS_OK) {
        HDF_LOGE("%s PRT_TimerDelete fail %u", __func__, ret);
        return HDF_FAILURE;
    }

    return OsalTimerStartLoop(timer);
}

int32_t OsalTimerDelete(OsalTimer *timer)
{
    uint32_t ret;
    struct OsalLitetimer *liteTimer = NULL;

    if (timer == NULL || timer->realTimer == NULL) {
        HDF_LOGE("%s invalid para", __func__);
        return HDF_ERR_INVALID_PARAM;
    }

    liteTimer = (struct OsalLitetimer *)timer->realTimer;
    if (liteTimer->timerID == OSAL_INVALID_TIMER_ID) {
        HDF_LOGE("%s timer id invalid %u", __func__, liteTimer->timerID);
        OsalMemFree(timer->realTimer);
        timer->realTimer = NULL;
        return HDF_FAILURE;
    }
    ret = PRT_TimerDelete(0, liteTimer->timerID);
    if (ret == OS_OK) {
        OsalMemFree(timer->realTimer);
        timer->realTimer = NULL;
        return HDF_SUCCESS;
    } else {
        HDF_LOGE("%s PRT_TimerDelete fail %u", __func__, ret);
        return HDF_FAILURE;
    }
}