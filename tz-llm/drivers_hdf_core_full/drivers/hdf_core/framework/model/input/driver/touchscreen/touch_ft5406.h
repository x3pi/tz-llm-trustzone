/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#ifndef TOUCH_FT5406_H
#define TOUCH_FT5406_H

#define FTS_REG_IDX_MAX    3

/* register address */
#define FTS_REG_FW_VER          0xA6    // FW version
#define FTS_REG_FW_MIN_VER      0xB2    // FW min version
#define FTS_REG_FW_SUB_MIN_VER  0xB3    // FW sub min version
#define FTS_REG_STATUS          0x02
#define FTS_REG_X_H             0x03

/* the config for parsing event buffer */
#define HALF_BYTE_MASK     0x0F
#define HALF_BYTE_OFFSET   4
#define SIX_BIT_OFFSET     6
#define ONE_BYTE_OFFSET    8

/* the FT5406 driver ic can support 5 points,
 * use 2 points default.
 */
#define MAX_SUPPORT_POINT 5

/* buffer size of one event */
#define FT_POINT_SIZE     6

/* read data buffer len
 * each point info occupy 6 bytes.
 */
#define POINT_BUFFER_LEN (FT_POINT_SIZE)

/* point data position, base on TD_STATUS */
#define FT_POINT_NUM_POS  2
#define FT_EVENT_POS      0
#define FT_X_H_POS        0
#define FT_X_L_POS        1
#define FT_Y_H_POS        2
#define FT_FINGER_POS     2
#define FT_Y_L_POS        3

#define KEY_CODE_4TH      3

#define POLL_INTERVAL_MS  17 /* 17ms = 60fps */

typedef struct {
    void *data;
    struct timer_list timer;
    struct work_struct work_poll;
} FT5406TouchData;
#endif