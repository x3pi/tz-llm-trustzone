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

#include "hdf_base.h"
#include "hdf_device_desc.h"
#include "platform_device_test.h"
#include "platform_log.h"
#include "platform_manager_test.h"

static void DoAllPlatformTest(void)
{
    PlatformManagerTestExecuteAll();
    PlatformDeviceTestExecuteAll();
}
static int32_t PlatformTestBind(struct HdfDeviceObject *device)
{
    HDF_LOGI("TEST_DRIVER:PlatformTestBind.\r\n");
    static struct IDeviceIoService service;

    if (device == NULL) {
        PLAT_LOGE("%s: device is null!", __func__);
        return HDF_ERR_IO;
    }
    device->service = &service;
    PLAT_LOGE("PlatformTestBind: done!");
    return HDF_SUCCESS;
}
static int32_t PlatformTestInit(struct HdfDeviceObject *device)
{
    HDF_LOGI("TEST_DRIVER:PlatformTestInit.\r\n");
    (void)device;
    DoAllPlatformTest();
    return HDF_SUCCESS;
}

static void PlatformTestRelease(struct HdfDeviceObject *device)
{
    if (device != NULL) {
        device->service = NULL;
    }
    return;
}

struct HdfDriverEntry g_platformTestEntry = {
    .moduleVersion = 1,
    .Bind = PlatformTestBind,
    .Init = PlatformTestInit,
    .Release = PlatformTestRelease,
    .moduleName = "PLATFORM_TEST_DRIVER",
};
HDF_INIT(g_platformTestEntry);
