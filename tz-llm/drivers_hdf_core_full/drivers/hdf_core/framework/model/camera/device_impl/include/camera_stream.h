/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#ifndef CAMERA_STREAM_H
#define CAMERA_STREAM_H

#include <utils/hdf_base.h>
#include <utils/hdf_sbuf.h>
#include "camera_device_manager.h"

int32_t CameraGetFormat(struct CommonDevice *comDev);
int32_t CameraSetFormat(struct CommonDevice *comDev);
int32_t CameraGetCrop(struct CommonDevice *comDev);
int32_t CameraSetCrop(struct CommonDevice *comDev);
int32_t CameraGetFPS(struct CommonDevice *comDev);
int32_t CameraSetFPS(struct CommonDevice *comDev);
int32_t CameraEnumFmt(struct CommonDevice *comDev);
int32_t CameraStreamGetAbility(struct CommonDevice *comDev);
int32_t CameraStreamEnumDevice(struct CommonDevice *comDev);
int32_t CameraQueueInit(struct CommonDevice *comDev);
int32_t CameraReqMemory(struct CommonDevice *comDev);
int32_t CameraQueryMemory(struct CommonDevice *comDev);
int32_t CameraMmapMemory(struct CommonDevice *comDev);
int32_t CameraStreamQueue(struct CommonDevice *comDev);
int32_t CameraStreamDeQueue(struct CommonDevice *comDev);
int32_t CameraStreamOn(struct CommonDevice *comDev);
int32_t CameraStreamOff(struct CommonDevice *comDev);

#endif  // CAMERA_STREAM_H