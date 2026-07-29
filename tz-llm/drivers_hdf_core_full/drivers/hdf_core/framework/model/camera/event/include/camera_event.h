/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#ifndef CAMERA_EVENT_H
#define CAMERA_EVENT_H

#include <utils/hdf_base.h>
#include <utils/hdf_sbuf.h>
#include <core/hdf_device_desc.h>
#include "camera_buffer_manager.h"

int32_t HdfCameraSendEvent(uint32_t eventId, const struct HdfSBuf *data);
int32_t HdfCameraAddDevice(struct HdfDeviceObject *device);
void HdfCameraDelDevice(void);

#endif  // CAMERA_EVENT_H