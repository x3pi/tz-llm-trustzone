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

#ifndef OSAL_TEST_TYPE_H
#define OSAL_TEST_TYPE_H

#include <stdio.h>

#undef HDF_LOGE
#define HDF_LOGE(fmt, ...) printf("[ERR]" fmt "\n", ##__VA_ARGS__)
#undef HDF_LOGI
#define HDF_LOGI(fmt, ...) printf("[INFO]" fmt "\n", ##__VA_ARGS__)
#undef HDF_LOGD
#define HDF_LOGD(fmt, ...) printf("[DEBUG]" fmt "\n", ##__VA_ARGS__)

#define BITS_PER_INT 32
#define OSAL_TEST_CASE_CNT (OSAL_TEST_MAX / BITS_PER_INT + 1)

#define OSAL_TEST_CASE_SET(cmd) (g_osalTestCases[(cmd) / BITS_PER_INT] |= (1 << ((cmd) % BITS_PER_INT)))
#define OSAL_TEST_CASE_CHECK(cmd) (g_osalTestCases[(cmd) / BITS_PER_INT] & (1 << ((cmd) % BITS_PER_INT)))

#define UT_TEST_CHECK_RET(val, cmd)                                                        \
    do {                                                                                   \
        if ((val) && (g_testEndFlag == false)) {                                           \
            printf("[OSAL_UT_TEST] %s %d %d OSA_UT_TEST_FAIL\n", __func__, __LINE__, cmd); \
            OSAL_TEST_CASE_SET((cmd));                                                     \
        }                                                                                  \
    } while (0)

#endif /* OSAL_TEST_TYPE_H */