/*
 * Copyright (c) 2020-2022 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#ifndef HDF_SERVICE_RECORD_H
#define HDF_SERVICE_RECORD_H

#include "hdf_device.h"
#include "hdf_dlist.h"

struct DevSvcRecord {
    struct DListHead entry;
    uint32_t key;
    struct HdfDeviceObject *value;
    const char *servName;
    const char *servInfo;
    uint16_t devClass;
    devid_t devId;
    const char *interfaceDesc;
};

struct DevSvcRecord *DevSvcRecordNewInstance(void);
void DevSvcRecordFreeInstance(struct DevSvcRecord *inst);

#endif /* HDF_SERVICE_RECORD_H */

