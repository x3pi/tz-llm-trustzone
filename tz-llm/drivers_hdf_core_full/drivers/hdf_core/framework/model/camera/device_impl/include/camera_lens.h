/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#ifndef CAMERA_LENS_H
#define CAMERA_LENS_H

#include <utils/hdf_base.h>
#include <utils/hdf_sbuf.h>
#include  "camera_common_device.h"

int32_t CameraCmdLensQueryConfig(struct SubDevice subDev);
int32_t CameraCmdLensGetConfig(struct SubDevice subDev);
int32_t CameraCmdLensSetConfig(struct SubDevice subDev);
int32_t CameraCmdLensEnumDevice(struct SubDevice subDev);
struct DeviceConfigOps *GetLensDeviceOps(void);

#endif  // CAMERA_LENS_H