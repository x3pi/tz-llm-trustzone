/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#ifndef CAMERA_VCM_H
#define CAMERA_VCM_H

#include <utils/hdf_base.h>
#include <utils/hdf_sbuf.h>
#include  "camera_common_device.h"

int32_t CameraCmdVcmQueryConfig(struct SubDevice subDev);
int32_t CameraCmdVcmGetConfig(struct SubDevice subDev);
int32_t CameraCmdVcmSetConfig(struct SubDevice subDev);
int32_t CameraCmdVcmEnumDevice(struct SubDevice subDev);
struct DeviceConfigOps *GetVcmDeviceOps(void);

#endif  // CAMERA_VCM_H