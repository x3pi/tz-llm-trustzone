/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#ifndef HDF_SERVICE_INFO_H
#define HDF_SERVICE_INFO_H

#include "hdf_device_node.h"

struct HdfServiceInfo {
    const char *servName;
    const char *servInfo;
    uint16_t devClass;
    devid_t devId;
    const char *interfaceDesc;
};

static inline void HdfServiceInfoInit(struct HdfServiceInfo *info, const struct HdfDeviceNode *devNode)
{
    info->servName = devNode->servName;
    info->servInfo = devNode->servInfo;
    info->devClass = devNode->deviceObject.deviceClass;
    info->devId = devNode->devId;
    info->interfaceDesc = devNode->interfaceDesc;
}

#endif // HDF_SERVICE_INFO_H