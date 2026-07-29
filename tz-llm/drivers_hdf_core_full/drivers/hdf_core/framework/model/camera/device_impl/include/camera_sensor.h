/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#ifndef CAMERA_SENSOR_H
#define CAMERA_SENSOR_H

#include <utils/hdf_base.h>
#include <utils/hdf_sbuf.h>
#include  "camera_common_device.h"

int32_t CameraCmdSensorQueryConfig(struct SubDevice subDev);
int32_t CameraCmdSensorGetConfig(struct SubDevice subDev);
int32_t CameraCmdSensorSetConfig(struct SubDevice subDev);
int32_t CameraCmdSensorEnumDevice(struct SubDevice subDev);
struct DeviceConfigOps *GetSensorDeviceOps(void);

#endif  // CAMERA_SENSOR_H