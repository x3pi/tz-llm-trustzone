/*
 * Copyright (c) 2020-2022 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#ifndef ILI9881C_BOE_H
#define ILI9881C_BOE_H
#include <drm/drm_mipi_dsi.h>
#include <linux/backlight.h>
#include <linux/of.h>
#include <linux/regulator/consumer.h>
#include "hdf_disp.h"

#define AVDD_GPIO    179
#define AVEE_GPIO    156
#define VGHL_GPIO    155
#define TSRST_GPIO   240
#define RESET_GPIO   178

struct GpioTiming {
    uint16_t level;
    uint32_t delay;
};

struct ResetSeq {
    uint32_t items;
    struct GpioTiming *timing;
};

struct Ili9881cBoeDev {
    struct PanelData panel;
    struct mipi_dsi_device *dsiDev;
    struct regulator *supply;
    uint16_t avddGpio;
    uint16_t aveeGpio;
    uint16_t vghlGpio;
    uint16_t tsrstGpio;
    uint16_t resetGpio;
    struct ResetSeq rstOnSeq;
    struct ResetSeq rstOffSeq;
};

#endif