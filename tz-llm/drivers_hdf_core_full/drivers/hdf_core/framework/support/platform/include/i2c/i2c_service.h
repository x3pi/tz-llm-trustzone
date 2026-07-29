/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#ifndef I2C_SERVICE_H
#define I2C_SERVICE_H

#include "hdf_base.h"

#ifdef __cplusplus
extern "C" {
#endif

#pragma pack(push, 4)
struct I2cUserMsg {
    /** Address of the I2C device */
    uint16_t addr;
    /** Length of the transferred data */
    uint16_t len;
    /**
     * Transfer Mode Flag | Description
     * ------------| -----------------------
     * I2C_FLAG_READ | Read flag
     * I2C_FLAG_ADDR_10BIT | 10-bit addressing flag
     * I2C_FLAG_READ_NO_ACK | No-ACK read flag
     * I2C_FLAG_IGNORE_NO_ACK | Ignoring no-ACK flag
     * I2C_FLAG_NO_START | No START condition flag
     * I2C_FLAG_STOP | STOP condition flag
     */
    uint16_t flags;
};
#pragma pack(pop)

#ifdef __cplusplus
}
#endif

#endif /* I2C_SERVICE_H */
