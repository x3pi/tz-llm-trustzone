/*
 * Copyright (c) 2022 Beijing OSWare Technology Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#ifndef TOUCH_FT5x06_H
#define TOUCH_FT5x06_H

#define WORK_REGISTER_REPORT_RATE    0x08

#define TOUCH_EVENT_DOWN             0x00
#define TOUCH_EVENT_UP               0x01
#define TOUCH_EVENT_ON               0x02
#define TOUCH_EVENT_RESERVED         0x03

#define EDT_NAME_LEN                 23

#define EDT_M06        0
#define EDT_M09        1
#define EDT_M12        2
#define EV_FT          3
#define GENERIC_FT     4

#define POINT_BUFFER_LEN_M06     26
#define POINT_BUFFER_LEN_M09     33

#define MAX_POINT 5

#define GT_FINGER_NUM_MASK    0x0F

#define NUM_0 (0)
#define NUM_1 (1)
#define NUM_2 (2)
#define NUM_3 (3)
#define NUM_4 (4)
#define NUM_5 (5)
#define NUM_6 (6)
#define NUM_7 (7)
#define EP0350M09 (0x35)
#define EP0430M09 (0x43)
#define EP0500M09 (0x50)
#define EP0570M09 (0x57)
#define EP0700M09 (0x70)
#define EP1010ML0 (0xa1)
#define EPDISPLAY (0x59)
#define EDT_M06_CON (0xf9)


#endif
