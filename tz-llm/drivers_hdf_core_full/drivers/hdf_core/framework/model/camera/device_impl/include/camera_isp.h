/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#ifndef CAMERA_ISP_H
#define CAMERA_ISP_H

#include <utils/hdf_base.h>
#include <utils/hdf_sbuf.h>
#include  "camera_common_device.h"

int32_t CameraCmdIspQueryConfig(struct SubDevice subDev);
int32_t CameraCmdIspGetConfig(struct SubDevice subDev);
int32_t CameraCmdIspSetConfig(struct SubDevice subDev);
int32_t CameraCmdIspEnumDevice(struct SubDevice subDev);
struct DeviceConfigOps *GetIspDeviceOps(void);

#endif  // CAMERA_HDF_ISP_H