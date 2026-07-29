/*
 * Copyright (c) 2020-2021 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#ifndef HDF_SERVICE_OBSERVER_H
#define HDF_SERVICE_OBSERVER_H

#include "hdf_device.h"
#include "hdf_device_desc.h"
#include "hdf_slist.h"
#include "osal_mutex.h"

struct HdfServiceObserver {
    struct HdfSList services;
    struct OsalMutex observerMutex;
};

bool HdfServiceObserverConstruct(struct HdfServiceObserver *observer);
void HdfServiceObserverDestruct(struct HdfServiceObserver *observer);
int HdfServiceObserverPublishService(struct HdfServiceObserver *observer,
    const char *svcName, devid_t deviceId, uint16_t policy, struct HdfObject *service);
int HdfServiceObserverSubscribeService(struct HdfServiceObserver *observer,
    const char *svcName, devid_t deviceId, struct SubscriberCallback callback);
void HdfServiceObserverRemoveRecord(struct HdfServiceObserver *observer, const char *svcName);

#endif /* HDF_SERVICE_OBSERVER_H */
