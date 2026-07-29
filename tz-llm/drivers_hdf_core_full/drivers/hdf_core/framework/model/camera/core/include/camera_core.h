/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#ifndef CAMERA_CORE_H
#define CAMERA_CORE_H

#include <utils/hdf_base.h>

int32_t HdfCameraDeinitDevice(const char *driverName);
bool HdfCameraGetDeviceInitStatus(void);

#endif  // CAMERA_CORE_H