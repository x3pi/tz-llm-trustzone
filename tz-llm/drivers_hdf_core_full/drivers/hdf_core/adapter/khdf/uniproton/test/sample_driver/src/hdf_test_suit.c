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

#include <stdio.h>
#include "hdf_log.h"
#include "hdf_platform_entry_test.h"
#include "platform_device_test.h"
#include "platform_manager_test.h"
#include "hdf_test_suit.h"

void HdfPlatformEntryTest(void)
{
    HDF_LOGI("entering HdfPlatformEntryTest.\r\n");
    HdfTestMsg msg = {TEST_PAL_DEVICE_TYPE, PLAT_DEVICE_TEST_GET_DEVICE, -1};
    (void)HdfPlatformDeviceTestEntry(&msg);
    HdfTestMsg msg1 = {TEST_PAL_MANAGER_TYPE, PLAT_MANAGER_TEST_GET_DEVICE, -1};
    (void)HdfPlatformManagerTestEntry(&msg1);
    HDF_LOGI("HdfPlatformEntryTest done.\r\n");
}

void HdfPlatformDeviceTest(void)
{
    HDF_LOGI("entering HdfPlatformDeviceTest.\r\n");
    PlatformDeviceTestExecuteAll();
    HDF_LOGI("HdfPlatformDeviceTest done.\r\n");
}

void HdfPlatformDriverTest(void)
{
    HDF_LOGI("HdfPlatformDriverTest done.\r\n");
}

void HdfPlatformManagerTest(void)
{
    HDF_LOGI("entering HdfPlatformManagerTest.\r\n");
    PlatformManagerTestExecuteAll();
    HDF_LOGI("HdfPlatformManagerTest done.\r\n");
}