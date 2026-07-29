/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#ifndef CAMERA_DISPATCHER_H
#define CAMERA_DISPATCHER_H

#include <utils/hdf_base.h>
#include <utils/hdf_sbuf.h>
#include <core/hdf_device_desc.h>
#include <camera/camera_product.h>
#include "hdf_types.h"

struct CameraCmdHandle {
    enum CameraMethodCmd cmd;
    int32_t (*func)(struct HdfDeviceIoClient *client, struct HdfSBuf *data, struct HdfSBuf *reply);
};

int32_t HdfCameraDispatch(struct HdfDeviceIoClient *client, int cmdId,
    struct HdfSBuf *reqData, struct HdfSBuf *rspData);

#endif  // CAMERA_DISPATCHER_H