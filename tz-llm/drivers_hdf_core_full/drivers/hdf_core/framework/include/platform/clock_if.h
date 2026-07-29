/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#ifndef CLOCK_IF_H
#define CLOCK_IF_H

#include "platform_if.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* __cplusplus */
#define MAX_PATH_LEN 120

DevHandle ClockOpen(uint32_t number);

int32_t ClockClose(DevHandle handle);

int32_t ClockEnable(DevHandle handle);

int32_t ClockDisable(DevHandle handle);

int32_t ClockSetRate(DevHandle handle, uint32_t rate);

int32_t ClockGetRate(DevHandle handle, uint32_t *rate);

int32_t ClockSetParent(DevHandle child, DevHandle parent);

DevHandle ClockGetParent(DevHandle handle);

/**
 * @brief 枚举 CLOCK I/O 命令.
 *
 * @since 1.0
 */
enum ClockIoCmd {
    CLOCK_IO_OPEN = 0,
    CLOCK_IO_CLOSE,
    CLOCK_IO_ENABLE,
    CLOCK_IO_DISABLE,
    CLOCK_IO_SET_RATE,
    CLOCK_IO_GET_RATE,
    CLOCK_IO_SET_PARENT,
    CLOCK_IO_GET_PARENT,
};

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif /* CLOCK_IF_H */
