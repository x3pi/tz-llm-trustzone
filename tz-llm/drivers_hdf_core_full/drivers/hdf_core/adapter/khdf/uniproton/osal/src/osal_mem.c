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

#include "osal_mem.h"
#include "hdf_log.h"
#include "prt_mem.h"
#include "securec.h"

#define HDF_LOG_TAG osal_mem

#define MEM_IS_POW_TWO(value)    ((((uintptr_t)(value)) & ((uintptr_t)(value) - 1)) == 0)

#define MEM_IS_ALIGNED(a, b)     (!(((uintptr_t)(a)) & (((uintptr_t)(b)) - 1)))

void *OsalMemAlloc(size_t size)
{
    if (size == 0) {
        HDF_LOGE("%s invalid param", __func__);
        return NULL;
    }

    return PRT_MemAlloc(OS_MEM_DEFAULT_PT0, OS_MEM_DEFAULT_FSC_PT, size);
}

void *OsalMemCalloc(size_t size)
{
    void *buf;

    if (size == 0) {
        HDF_LOGE("%s invalid param", __func__);
        return NULL;
    }

    buf = OsalMemAlloc(size);
    if (buf != NULL) {
        (void)memset_s(buf, size, 0, size);
    }

    return buf;
}

static int OsalMemLog2(int alignment)
{
    int x = 0;
    int temp = alignment;

    if ((alignment == 0) || !MEM_IS_POW_TWO(alignment) || !MEM_IS_ALIGNED(alignment, sizeof(void *))) {
        return -1;
    }

    while (temp > 1) {
        temp >>= 1;
        x++;
    }
    return x;
}

void *OsalMemAllocAlign(size_t alignment, size_t size)
{
    if (size == 0) {
        HDF_LOGE("%s invalid param", __func__);
        return NULL;
    }

    return PRT_MemAllocAlign(OS_MEM_DEFAULT_PT0, OS_MEM_DEFAULT_FSC_PT, size, OsalMemLog2(alignment));
}

void OsalMemFree(void *mem)
{
    if (mem != NULL) {
        PRT_MemFree((U32)OS_MID_TSK, mem);
    }
}