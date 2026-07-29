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

#include "osal_sem.h"
#include "prt_sem.h"
#include "prt_config.h"
#include "hdf_log.h"

#define HDF_LOG_TAG osal_sem
#define HDF_INVALID_SEM_ID UINT32_MAX

int32_t OsalSemInit(struct OsalSem *sem, uint32_t value)
{
    SemHandle semId;
    uint32_t ret;

    if (sem == NULL) {
        HDF_LOGE("%s invalid param", __func__);
        return HDF_ERR_INVALID_PARAM;
    }

    ret = PRT_SemCreate(value, (SemHandle *)&semId);
    if (ret == OS_OK) {
        sem->realSemaphore = (void *)(uintptr_t)semId;
        return HDF_SUCCESS;
    } else {
        sem->realSemaphore = (void *)(uintptr_t)HDF_INVALID_SEM_ID;
        HDF_LOGE("%s create fail %u", __func__, ret);
        return HDF_FAILURE;
    }
}

int32_t OsalSemWait(struct OsalSem *sem, uint32_t ms)
{
    uint32_t ret;
    uint32_t ticks = (uint32_t)(((uint64_t)ms * OS_TICK_PER_SECOND) / 1000);

    if (sem == NULL || sem->realSemaphore == (void *)(uintptr_t)HDF_INVALID_SEM_ID) {
        HDF_LOGE("%s invalid param", __func__);
        return HDF_ERR_INVALID_PARAM;
    }

    ret = PRT_SemPend((uint32_t)(uintptr_t)sem->realSemaphore, ticks);
    if (ret == OS_OK) {
        return HDF_SUCCESS;
    } else {
        if (ret == OS_ERRNO_SEM_TIMEOUT) {
            return HDF_ERR_TIMEOUT;
        }
        HDF_LOGE("%s LOS_SemPend fail %u", __func__, ret);
        return HDF_FAILURE;
    }
}

int32_t OsalSemPost(struct OsalSem *sem)
{
    uint32_t ret;

    if (sem == NULL || sem->realSemaphore == (void *)(uintptr_t)HDF_INVALID_SEM_ID) {
        HDF_LOGE("%s invalid param", __func__);
        return HDF_ERR_INVALID_PARAM;
    }

    ret = PRT_SemPost((uint32_t)(uintptr_t)sem->realSemaphore);
    if (ret == OS_OK) {
        return HDF_SUCCESS;
    } else {
        HDF_LOGE("%s LOS_SemPost fail %u", __func__, ret);
        return HDF_FAILURE;
    }
}

int32_t OsalSemDestroy(struct OsalSem *sem)
{
    uint32_t ret;

    if (sem == NULL || sem->realSemaphore == (void *)(uintptr_t)HDF_INVALID_SEM_ID) {
        HDF_LOGE("%s invalid param", __func__);
        return HDF_ERR_INVALID_PARAM;
    }

    ret = PRT_SemDelete((uint32_t)(uintptr_t)sem->realSemaphore);
    if (ret != OS_OK) {
        HDF_LOGE("%s LOS_SemDelete fail %u", __func__, ret);
        return HDF_FAILURE;
    }
    sem->realSemaphore = (void *)(uintptr_t)HDF_INVALID_SEM_ID;
    return HDF_SUCCESS;
}