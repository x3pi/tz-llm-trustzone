/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#ifndef CLOCK_TEST_H
#define CLOCK_TEST_H

#include "clock_if.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* __cplusplus */

struct ClockTestConfig {
    uint32_t deviceIndex;
};

struct ClockTester {
    struct ClockTestConfig config;
    DevHandle handle;
};

enum ClockTestCmd {
    CLOCK_TEST_CMD_ENABLE = 0,
    CLOCK_TEST_CMD_DISABLE,
    CLOCK_TEST_CMD_SET_RATE,
    CLOCK_TEST_CMD_GET_RATE,
    CLOCK_TEST_CMD_GET_PARENT,
    CLOCK_TEST_CMD_SET_PARENT,
    CLOCK_TEST_CMD_MULTI_THREAD,
    CLOCK_TEST_CMD_RELIABILITY,
    CLOCK_IF_PERFORMANCE_TEST,
    CLOCK_TEST_CMD_MAX,
};

int32_t ClockTestExecute(int cmd);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif /* CLOCK_TEST_H */
