/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#ifndef ILI9881_ST_5P5_H
#define ILI9881_ST_5P5_H

#include <drm/drm_mipi_dsi.h>
#include <uapi/drm/drm_mode.h>
#include <drm/drm_modes.h>
#include <linux/backlight.h>
#include <linux/of.h>
#include <linux/regulator/consumer.h>
#include <linux/gpio/consumer.h>
#include "hdf_disp.h"

struct panel_hw_delay {
    uint32_t prepare_delay;
    uint32_t hpd_absent_delay;
    uint32_t enable_delay;
    uint32_t disable_delay;
    uint32_t unprepare_delay;
    uint32_t reset_delay;
    uint32_t init_delay;
};

struct panel_ili9881_dev {
    bool power_invert;
    struct PanelData panel;
    struct mipi_dsi_device *dsiDev;
    struct regulator *supply;
    struct gpio_desc *enable_gpio;
    struct gpio_desc *reset_gpio;
    struct gpio_desc *hpd_gpio;
    struct panel_hw_delay hw_delay;
};

#endif