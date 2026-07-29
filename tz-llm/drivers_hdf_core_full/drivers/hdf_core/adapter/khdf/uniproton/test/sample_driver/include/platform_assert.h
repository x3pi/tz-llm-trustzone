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

#ifndef PLATFORM_ASSERT_H
#define PLATFORM_ASSERT_H

#include "platform_log.h"

#define ERROR_AND_LOG(expr)                                             \
    do {                                                                \
        PLAT_LOGE(__FILE__ "(line:%d): " #expr " is false!", __LINE__); \
    } while (0)

#define CHECK(expr)              \
    do {                         \
        if (!(expr)) {           \
            ERROR_AND_LOG(expr); \
        }                        \
        (expr);                  \
    } while (0)

#define CHECK_EQ(a, b)      CHECK((a) == (b))
#define CHECK_NE(a, b)      CHECK((a) != (b))
#define CHECK_GE(a, b)      CHECK((a) >= (b))
#define CHECK_GT(a, b)      CHECK((a) > (b))
#define CHECK_LE(a, b)      CHECK((a) <= (b))
#define CHECK_LT(a, b)      CHECK((a) < (b))
#define CHECK_NULL(ptr)     CHECK((ptr) == NULL)
#define CHECK_NOT_NULL(ptr) CHECK((ptr) != NULL)

#define ERROR_AND_RETURN(expr, ret)                                     \
    do {                                                                \
        PLAT_LOGE(__FILE__ "(line:%d): " #expr " is false!", __LINE__); \
        return ret;                                                     \
    } while (0)

#define CHECK_AND_RETURN(expr, ret)        \
    do {                                   \
        if (!(expr)) {                     \
            ERROR_AND_RETURN(expr, (ret)); \
        }                                  \
    } while (0)

#define CHECK_EQ_RETURN(a, b, ret)      CHECK_AND_RETURN((a) == (b), ret)
#define CHECK_NE_RETURN(a, b, ret)      CHECK_AND_RETURN((a) != (b), ret)
#define CHECK_GE_RETURN(a, b, ret)      CHECK_AND_RETURN((a) >= (b), ret)
#define CHECK_GT_RETURN(a, b, ret)      CHECK_AND_RETURN((a) > (b), ret)
#define CHECK_LE_RETURN(a, b, ret)      CHECK_AND_RETURN((a) <= (b), ret)
#define CHECK_LT_RETURN(a, b, ret)      CHECK_AND_RETURN((a) < (b), ret)
#define CHECK_NULL_RETURN(ptr, ret)     CHECK_AND_RETURN((ptr) == NULL, ret)
#define CHECK_NOT_NULL_RETURN(ptr, ret) CHECK_AND_RETURN((ptr) != NULL, ret)

#define LONGS_EQUAL_RETURN(x, y)                                                              \
    do {                                                                                      \
        int xx = (x);                                                                         \
        int yy = (y);                                                                         \
        if (xx != yy) {                                                                       \
            HDF_LOGE(__FILE__ "(line:%d): expect:%d->" #x ", got:%d->" #y, __LINE__, xx, yy); \
            return HDF_FAILURE;                                                               \
        }                                                                                     \
    } while (0)

#define CHECK_TRUE_RETURN(cond)                                        \
    do {                                                               \
        if (cond) {                                                    \
            break;                                                     \
        }                                                              \
        HDF_LOGE(__FILE__ "(line:%d): " #cond " is false!", __LINE__); \
        return HDF_FAILURE;                                            \
    } while (0)

#define CHECK_FALSE_RETURN(cond)                                      \
    do {                                                              \
        if (!(cond)) {                                                \
            break;                                                    \
        }                                                             \
        HDF_LOGE(__FILE__ "(line:%d): " #cond " is true!", __LINE__); \
        return HDF_FAILURE;                                           \
    } while (0)

#endif /* PLATFORM_ASSERT_H */
