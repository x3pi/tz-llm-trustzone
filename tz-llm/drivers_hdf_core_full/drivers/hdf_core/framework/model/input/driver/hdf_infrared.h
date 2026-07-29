/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */
#ifndef HDF_INFRARED_H
#define HDF_INFRARED_H

#include <securec.h>
#include "osal_time.h"
#include "input_config.h"
#include "hdf_input_device_manager.h"

typedef struct InfraredDriverInfo {
    struct HdfDeviceObject *hdfInfraredDev;
    uint8_t devType;
    InfraredCfg *infraredCfg;
    InputDevice *inputDev;
} InfraredDriver;

struct InfraredKey {
    uint8_t value;
    int32_t infraredCode;
};

#endif