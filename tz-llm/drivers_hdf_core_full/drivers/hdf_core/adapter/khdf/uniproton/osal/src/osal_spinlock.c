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

#include "osal_spinlock.h"
#include "hdf_log.h"
#include "prt_sem.h"

#define HDF_LOG_TAG osal_spinlock

#define HDF_INVALID_MUX_ID UINT32_MAX

int32_t OsalSpinInit(OsalSpinlock *spinlock)
{
    (void)spinlock;
    /* On single-core cpu,only use kernel interface to implement Osal
    * interface without other Mechanisms will cause cpu source waste.
    * For this reason,leave unimplementation.
    */
    return HDF_SUCCESS;
}

int32_t OsalSpinDestroy(OsalSpinlock *spinlock)
{
    (void)spinlock;
    /* On single-core cpu,only use kernel interface to implement Osal
    * interface without other Mechanisms will cause cpu source waste.
    * For this reason,leave unimplementation.
    */
    return HDF_SUCCESS;
}

int32_t OsalSpinLock(OsalSpinlock *spinlock)
{
    (void)spinlock;
    /* On single-core cpu,only use kernel interface to implement Osal
    * interface without other Mechanisms will cause cpu source waste.
    * For this reason,leave unimplementation.
    */
    return HDF_SUCCESS;
}

int32_t OsalSpinUnlock(OsalSpinlock *spinlock)
{
    (void)spinlock;
    /* On single-core cpu,only use kernel interface to implement Osal
    * interface without other Mechanisms will cause cpu source waste.
    * For this reason,leave unimplementation.
    */
    return HDF_SUCCESS;
}

int32_t OsalSpinLockIrq(OsalSpinlock *spinlock)
{
    /* On single-core cpu,only use kernel interface to implement Osal
    * interface without other Mechanisms will cause cpu source waste.
    * For this reason,leave unimplementation.
    */
    return OsalSpinLock(spinlock);
}

int32_t OsalSpinUnlockIrq(OsalSpinlock *spinlock)
{
    /* On single-core cpu,only use kernel interface to implement Osal
    * interface without other Mechanisms will cause cpu source waste.
    * For this reason,leave unimplementation.
    */
    return OsalSpinUnlock(spinlock);
}

int32_t OsalSpinLockIrqSave(OsalSpinlock *spinlock, uint32_t *flags)
{
    (void)flags;
    /* On single-core cpu,only use kernel interface to implement Osal
    * interface without other Mechanisms will cause cpu source waste.
    * For this reason,leave unimplementation.
    */
    return OsalSpinLock(spinlock);
}

int32_t OsalSpinUnlockIrqRestore(OsalSpinlock *spinlock, uint32_t *flags)
{
    (void)flags;
    /* On single-core cpu,only use kernel interface to implement Osal
    * interface without other Mechanisms will cause cpu source waste.
    * For this reason,leave unimplementation.
    */
    return OsalSpinUnlock(spinlock);
}