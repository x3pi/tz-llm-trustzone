/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#ifndef CAMERA_UVC_H
#define CAMERA_UVC_H

#include <utils/hdf_base.h>
#include <utils/hdf_sbuf.h>
#include  "camera_common_device.h"

int32_t CameraCmdUvcQueryConfig(struct SubDevice subDev);
int32_t CameraCmdUvcGetConfig(struct SubDevice subDev);
int32_t CameraCmdUvcSetConfig(struct SubDevice subDev);
int32_t CameraCmdUvcEnumDevice(struct SubDevice subDev);
struct DeviceConfigOps *GetUvcDeviceOps(void);

#endif  // CAMERA_UVC_H