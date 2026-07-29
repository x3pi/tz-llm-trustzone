/*
 * Copyright (c) 2021-2022 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#ifndef HDF_CHIP_CONFIG_H
#define HDF_CHIP_CONFIG_H

#include "device_resource_if.h"
#include "hdf_base.h"
#include "hdf_log.h"
#include "osal_mem.h"
#include "securec.h"

#define HDF_CHIP_MAX_POWER_SUPPORTED 2

#define BUS_FUNC_MAX 1
#define MAX_POWER_COUNT_LEN 32

enum PowerType {
    POWER_TYPE_ALWAYS_ON = 0,
    POWER_TYPE_GPIO
};

struct HdfConfigGpioBasedSwitch {
    uint16_t gpioId;
    uint8_t activeLevel;
};

struct HdfPowerConfig {
    uint8_t powerSeqDelay;
    uint8_t type;
    union {
        struct HdfConfigGpioBasedSwitch gpio;
    };
};

struct HdfPowersConfig {
    uint8_t powerCount;
    struct HdfPowerConfig power[0];
};

enum ResetType {
    RESET_TYPE_NOT_MANAGEABLE = 0,
    RESET_TYPE_GPIO
};

struct HdfResetConfig {
    union {
        struct HdfConfigGpioBasedSwitch gpio;
    };
    uint8_t resetType;
    uint8_t resetHoldTime;
};

struct HdfChipConfig {
    const char *name;
    struct HdfPowersConfig *powers;
    struct HdfResetConfig reset;
    uint8_t bootUpTimeOut;
};

void ClearChipConfig(struct HdfChipConfig *config);
int32_t ParseChipConfig(const struct DeviceResourceNode *node, struct HdfChipConfig *config);

#endif