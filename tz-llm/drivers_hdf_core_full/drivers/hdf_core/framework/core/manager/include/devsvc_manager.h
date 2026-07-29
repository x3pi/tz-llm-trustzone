/*
 * Copyright (c) 2020-2022 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#ifndef DEVICE_SERVICE_MANAGER_H
#define DEVICE_SERVICE_MANAGER_H

#include "devsvc_manager_if.h"
#include "hdf_service_observer.h"
#include "hdf_dlist.h"
#include "osal_mutex.h"

struct DevSvcManager {
    struct IDevSvcManager super;
    struct DListHead services;
    struct HdfServiceObserver observer;
    struct DListHead svcstatListeners;
    struct OsalMutex mutex;
};

struct HdfObject *DevSvcManagerCreate(void);
bool DevSvcManagerConstruct(struct DevSvcManager *inst);
void DevSvcManagerRelease(struct IDevSvcManager *inst);
struct IDevSvcManager *DevSvcManagerGetInstance(void);

int DevSvcManagerStartService(void);

int DevSvcManagerAddService(
    struct IDevSvcManager *inst, struct HdfDeviceObject *service, const struct HdfServiceInfo *servInfo);
struct HdfObject *DevSvcManagerGetService(struct IDevSvcManager *inst, const char *svcName);
void DevSvcManagerRemoveService(
    struct IDevSvcManager *inst, const char *svcName, const struct HdfDeviceObject *devObj);
void DevSvcManagerListService(struct HdfSBuf *serviceNameSet, DeviceClass deviceClass);
int DevSvcManagerListServiceByInterfaceDesc(
    struct IDevSvcManager *inst, const char *interfaceDesc, struct HdfSBuf *reply);

int DevSvcManagerClntSubscribeService(const char *svcName, struct SubscriberCallback callback);
int DevSvcManagerClntUnsubscribeService(const char *svcName);

#endif /* DEVICE_SERVICE_MANAGER_H */
