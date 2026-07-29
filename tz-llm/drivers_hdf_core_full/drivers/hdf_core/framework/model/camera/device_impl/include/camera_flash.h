/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#ifndef CAMERA_FLASH_H
#define CAMERA_FLASH_H

#include <utils/hdf_base.h>
#include <utils/hdf_sbuf.h>
#include  "camera_common_device.h"

int32_t CameraCmdFlashQueryConfig(struct SubDevice subDev);
int32_t CameraCmdFlashGetConfig(struct SubDevice subDev);
int32_t CameraCmdFlashSetConfig(struct SubDevice subDev);
int32_t CameraCmdFlashEnumDevice(struct SubDevice subDev);
struct DeviceConfigOps *GetFlashDeviceOps(void);

#endif  // CAMERA_FLASH_H