/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#ifndef CAMERA_COMMON_DEVICE_H
#define CAMERA_COMMON_DEVICE_H

#include <utils/hdf_base.h>
#include <utils/hdf_sbuf.h>
#include "camera_device_manager.h"

int32_t CommonDevicePowerOperation(struct CommonDevice *comDev, enum DevicePowerState power);
int32_t CommonDeviceQueryConfig(struct CommonDevice *comDev);
int32_t CommonDeviceEnumDevice(struct CommonDevice *comDev);
int32_t CommonDeviceGetConfig(struct CommonDevice *comDev);
int32_t CommonDeviceSetConfig(struct CommonDevice *comDev);

struct SubDevice {
    struct CameraDevice *camDev;
    int32_t devId;
    uint32_t ctrlId;
    struct HdfSBuf *reqData;
    struct HdfSBuf *rspData;
    struct CameraDeviceConfig *cameraHcsConfig;
    struct DeviceConfigOps *subDevOps;
};

struct DeviceConfigOps {
    int32_t (*queryConfig)(struct SubDevice subDev);
    int32_t (*getConfig)(struct SubDevice subDev);
    int32_t (*setConfig)(struct SubDevice subDev);
    int32_t (*enumDevice)(struct SubDevice subDev);
};

#endif  // CAMERA_COMMON_DEVICE_H