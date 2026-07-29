/*
 * Copyright (c) 2020-2022 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include "devsvc_manager.h"
#include "devmgr_service.h"
#include "hdf_base.h"
#include "hdf_cstring.h"
#include "hdf_device_node.h"
#include "hdf_log.h"
#include "hdf_object_manager.h"
#include "hdf_service_record.h"
#include "osal_mem.h"

#define HDF_LOG_TAG devsvc_manager
#define SERVICE_LIST_MAX 16

static struct DevSvcRecord *DevSvcManagerSearchServiceLocked(struct IDevSvcManager *inst, uint32_t serviceKey)
{
    struct DevSvcRecord *record = NULL;
    struct DevSvcRecord *searchResult = NULL;
    struct DevSvcManager *devSvcManager = (struct DevSvcManager *)inst;
    if (devSvcManager == NULL) {
        HDF_LOGE("failed to search service, devSvcManager is null");
        return NULL;
    }

    DLIST_FOR_EACH_ENTRY(record, &devSvcManager->services, struct DevSvcRecord, entry) {
        if (record->key == serviceKey) {
            searchResult = record;
            break;
        }
    }
    return searchResult;
}

static void NotifyServiceStatusLocked(
    struct DevSvcManager *devSvcManager, const struct DevSvcRecord *record, uint32_t status)
{
    struct ServStatListenerHolder *holder = NULL;
    struct ServStatListenerHolder *tmp = NULL;
    struct ServiceStatus svcstat = {
        .deviceClass = record->devClass,
        .serviceName = record->servName,
        .status = status,
        .info = record->servInfo,
    };
    DLIST_FOR_EACH_ENTRY_SAFE(holder, tmp, &devSvcManager->svcstatListeners, struct ServStatListenerHolder, node) {
        if ((holder->listenClass & record->devClass) && holder->NotifyStatus != NULL) {
            if (holder->NotifyStatus(holder, &svcstat) == HDF_FAILURE) {
                DListRemove(&holder->node);
                if (holder->Recycle != NULL) {
                    holder->Recycle(holder);
                }
            }
        }
    }
}

static void NotifyServiceStatusOnRegisterLocked(
    const struct DevSvcManager *devSvcManager, struct ServStatListenerHolder *listenerHolder)
{
    struct DevSvcRecord *record = NULL;
    DLIST_FOR_EACH_ENTRY(record, &devSvcManager->services, struct DevSvcRecord, entry) {
        if ((listenerHolder->listenClass & record->devClass) == 0) {
            continue;
        }
        struct ServiceStatus svcstat = {
            .deviceClass = record->devClass,
            .serviceName = record->servName,
            .status = SERVIE_STATUS_START,
            .info = record->servInfo,
        };
        if (listenerHolder->NotifyStatus != NULL) {
            listenerHolder->NotifyStatus(listenerHolder, &svcstat);
        }
    }
}

int DevSvcManagerAddService(struct IDevSvcManager *inst,
    struct HdfDeviceObject *service, const struct HdfServiceInfo *servInfo)
{
    struct DevSvcManager *devSvcManager = (struct DevSvcManager *)inst;
    struct DevSvcRecord *record = NULL;
    if (devSvcManager == NULL || service == NULL || servInfo == NULL || servInfo->servName == NULL) {
        HDF_LOGE("failed to add service, input param is null");
        return HDF_FAILURE;
    }
    OsalMutexLock(&devSvcManager->mutex);
    record = DevSvcManagerSearchServiceLocked(inst, HdfStringMakeHashKey(servInfo->servName, 0));
    if (record != NULL) {
        // on service died will release old service object
        record->value = service;
        OsalMutexUnlock(&devSvcManager->mutex);
        HDF_LOGI("%{public}s:add service %{public}s exist, only update value", __func__, servInfo->servName);
        return HDF_SUCCESS;
    }
    OsalMutexUnlock(&devSvcManager->mutex);
    record = DevSvcRecordNewInstance();
    if (record == NULL) {
        HDF_LOGE("failed to add service , record is null");
        return HDF_FAILURE;
    }

    record->key = HdfStringMakeHashKey(servInfo->servName, 0);
    record->value = service;
    record->devClass = servInfo->devClass;
    record->devId = servInfo->devId;
    record->servName = HdfStringCopy(servInfo->servName);
    record->servInfo = HdfStringCopy(servInfo->servInfo);

    if (servInfo->interfaceDesc != NULL && strcmp(servInfo->interfaceDesc, "") != 0) {
        record->interfaceDesc = HdfStringCopy(servInfo->interfaceDesc);
    }
    if (record->servName == NULL) {
        DevSvcRecordFreeInstance(record);
        return HDF_ERR_MALLOC_FAIL;
    }
    OsalMutexLock(&devSvcManager->mutex);
    DListInsertTail(&record->entry, &devSvcManager->services);
    NotifyServiceStatusLocked(devSvcManager, record, SERVIE_STATUS_START);
    OsalMutexUnlock(&devSvcManager->mutex);
    return HDF_SUCCESS;
}

int DevSvcManagerUpdateService(struct IDevSvcManager *inst,
    struct HdfDeviceObject *service, const struct HdfServiceInfo *servInfo)
{
    struct DevSvcManager *devSvcManager = (struct DevSvcManager *)inst;
    struct DevSvcRecord *record = NULL;
    char *servInfoStr = NULL;
    if (devSvcManager == NULL || service == NULL || servInfo == NULL || servInfo->servName == NULL) {
        HDF_LOGE("failed to update service, invalid param");
        return HDF_FAILURE;
    }
    OsalMutexLock(&devSvcManager->mutex);
    record = DevSvcManagerSearchServiceLocked(inst, HdfStringMakeHashKey(servInfo->servName, 0));
    if (record == NULL) {
        OsalMutexUnlock(&devSvcManager->mutex);
        return HDF_DEV_ERR_NO_DEVICE;
    }

    if (servInfo->servInfo != NULL) {
        servInfoStr = HdfStringCopy(servInfo->servInfo);
        if (servInfoStr == NULL) {
            OsalMutexUnlock(&devSvcManager->mutex);
            return HDF_ERR_MALLOC_FAIL;
        }
        OsalMemFree((char *)record->servInfo);
        record->servInfo = servInfoStr;
    }

    record->value = service;
    record->devClass = servInfo->devClass;
    record->devId = servInfo->devId;
    NotifyServiceStatusLocked(devSvcManager, record, SERVIE_STATUS_CHANGE);
    OsalMutexUnlock(&devSvcManager->mutex);
    return HDF_SUCCESS;
}

int DevSvcManagerSubscribeService(struct IDevSvcManager *inst, const char *svcName, struct SubscriberCallback callBack)
{
    struct DevmgrService *devMgrSvc = (struct DevmgrService *)DevmgrServiceGetInstance();
    struct HdfObject *deviceService = NULL;
    if (inst == NULL || svcName == NULL || devMgrSvc == NULL) {
        return HDF_FAILURE;
    }

    deviceService = DevSvcManagerGetService(inst, svcName);
    if (deviceService != NULL) {
        if (callBack.OnServiceConnected != NULL) {
            callBack.OnServiceConnected(callBack.deviceObject, deviceService);
        }
        return HDF_SUCCESS;
    }

    return devMgrSvc->super.LoadDevice(&devMgrSvc->super, svcName);
}

void DevSvcManagerRemoveService(struct IDevSvcManager *inst, const char *svcName, const struct HdfDeviceObject *devObj)
{
    struct DevSvcManager *devSvcManager = (struct DevSvcManager *)inst;
    struct DevSvcRecord *serviceRecord = NULL;
    uint32_t serviceKey = HdfStringMakeHashKey(svcName, 0);
    bool removeFlag = false;

    if (svcName == NULL || devSvcManager == NULL) {
        return;
    }
    OsalMutexLock(&devSvcManager->mutex);
    serviceRecord = DevSvcManagerSearchServiceLocked(inst, serviceKey);
    if (serviceRecord == NULL) {
        OsalMutexUnlock(&devSvcManager->mutex);
        return;
    }
    if (devObj == NULL || (uintptr_t)devObj == (uintptr_t)serviceRecord->value) {
        NotifyServiceStatusLocked(devSvcManager, serviceRecord, SERVIE_STATUS_STOP);
        DListRemove(&serviceRecord->entry);
        removeFlag = true;
    }
    OsalMutexUnlock(&devSvcManager->mutex);

    if (removeFlag) {
        DevSvcRecordFreeInstance(serviceRecord);
    } else {
        HDF_LOGI("%{public}s %{public}s device object is out of date", __func__, svcName);
    }
}

struct HdfDeviceObject *DevSvcManagerGetObject(struct IDevSvcManager *inst, const char *svcName)
{
    uint32_t serviceKey = HdfStringMakeHashKey(svcName, 0);
    struct DevSvcManager *devSvcManager = (struct DevSvcManager *)inst;
    struct DevSvcRecord *serviceRecord = NULL;
    struct HdfDeviceObject *deviceObject = NULL;
    if (svcName == NULL) {
        HDF_LOGE("Get service failed, svcName is null");
        return NULL;
    }
    OsalMutexLock(&devSvcManager->mutex);
    serviceRecord = DevSvcManagerSearchServiceLocked(inst, serviceKey);
    if (serviceRecord != NULL) {
        deviceObject = serviceRecord->value;
        OsalMutexUnlock(&devSvcManager->mutex);
        return deviceObject;
    }
    OsalMutexUnlock(&devSvcManager->mutex);
    return NULL;
}

// only use for kernel space
void DevSvcManagerListService(struct HdfSBuf *serviceNameSet, DeviceClass deviceClass)
{
    struct DevSvcRecord *record = NULL;
    struct DevSvcManager *devSvcManager = (struct DevSvcManager *)DevSvcManagerGetInstance();
    if (devSvcManager == NULL) {
        HDF_LOGE("failed to list service, devSvcManager is null");
        return;
    }

    OsalMutexLock(&devSvcManager->mutex);
    DLIST_FOR_EACH_ENTRY(record, &devSvcManager->services, struct DevSvcRecord, entry) {
        if (record->devClass == deviceClass) {
            HdfSbufWriteString(serviceNameSet, record->servName);
        }
    }
    OsalMutexUnlock(&devSvcManager->mutex);
}

struct HdfObject *DevSvcManagerGetService(struct IDevSvcManager *inst, const char *svcName)
{
    struct HdfDeviceObject *deviceObject = DevSvcManagerGetObject(inst, svcName);
    if (deviceObject == NULL) {
        return NULL;
    }
    return (struct HdfObject *)deviceObject->service;
}

void DevSvcManagerListAllService(struct IDevSvcManager *inst, struct HdfSBuf *reply)
{
    struct DevSvcRecord *record = NULL;
    struct DevSvcManager *devSvcManager = (struct DevSvcManager *)inst;
    if (devSvcManager == NULL || reply == NULL) {
        HDF_LOGE("failed to list all service info, parameter is null");
        return;
    }
    OsalMutexLock(&devSvcManager->mutex);
    DLIST_FOR_EACH_ENTRY(record, &devSvcManager->services, struct DevSvcRecord, entry) {
        HdfSbufWriteString(reply, record->servName);
        HdfSbufWriteUint16(reply, record->devClass);
        HdfSbufWriteUint32(reply, record->devId);
        HDF_LOGD("%{public}s 0x%{public}x %{public}d", record->servName, record->devId, record->devClass);
    }
    OsalMutexUnlock(&devSvcManager->mutex);
    HDF_LOGI("%{public}s end ", __func__);
}

int DevSvcManagerListServiceByInterfaceDesc(
    struct IDevSvcManager *inst, const char *interfaceDesc, struct HdfSBuf *reply)
{
    struct DevSvcRecord *record = NULL;
    struct DevSvcManager *devSvcManager = (struct DevSvcManager *)inst;
    int status = HDF_SUCCESS;
    if (devSvcManager == NULL || reply == NULL) {
        HDF_LOGE("failed to list service collection info, parameter is null");
        return HDF_ERR_INVALID_PARAM;
    }
    const char *serviceNames[SERVICE_LIST_MAX];
    uint32_t serviceNum = 0;
    uint32_t i;
    OsalMutexLock(&devSvcManager->mutex);
    DLIST_FOR_EACH_ENTRY(record, &devSvcManager->services, struct DevSvcRecord, entry) {
        if (record->interfaceDesc == NULL) {
            HDF_LOGD("%{public}s interfacedesc is null", record->servName);
            continue;
        }
        if (serviceNum >= SERVICE_LIST_MAX) {
            status = HDF_ERR_OUT_OF_RANGE;
            HDF_LOGE(
                "%{public}s: More than %{public}d services are found, but up to %{public}d services can be returned",
                interfaceDesc, SERVICE_LIST_MAX, SERVICE_LIST_MAX);
            break;
        }
        if (strcmp(record->interfaceDesc, interfaceDesc) == 0) {
            serviceNames[serviceNum] = record->servName;
            serviceNum = serviceNum + 1;
        }
    }
    OsalMutexUnlock(&devSvcManager->mutex);
    HDF_LOGD("find %{public}u services interfacedesc is %{public}s", serviceNum, interfaceDesc);
    if (!HdfSbufWriteUint32(reply, serviceNum)) {
        HDF_LOGE("failed to write serviceNum to buffer, interfacedesc is %{public}s, serviceNum is %{public}d",
            interfaceDesc, serviceNum);
        return HDF_FAILURE;
    }
    for (i = 0; i < serviceNum; i++) {
        HdfSbufWriteString(reply, serviceNames[i]);
    }
    return status;
}

int DevSvcManagerRegsterServListener(struct IDevSvcManager *inst, struct ServStatListenerHolder *listenerHolder)
{
    struct DevSvcManager *devSvcManager = (struct DevSvcManager *)inst;
    if (devSvcManager == NULL || listenerHolder == NULL) {
        return HDF_ERR_INVALID_PARAM;
    }

    OsalMutexLock(&devSvcManager->mutex);
    DListInsertTail(&listenerHolder->node, &devSvcManager->svcstatListeners);
    NotifyServiceStatusOnRegisterLocked(devSvcManager, listenerHolder);
    OsalMutexUnlock(&devSvcManager->mutex);

    return HDF_SUCCESS;
}

void DevSvcManagerUnregsterServListener(struct IDevSvcManager *inst, struct ServStatListenerHolder *listenerHolder)
{
    struct DevSvcManager *devSvcManager = (struct DevSvcManager *)inst;
    if (devSvcManager == NULL || listenerHolder == NULL) {
        return;
    }

    OsalMutexLock(&devSvcManager->mutex);
    DListRemove(&listenerHolder->node);
    OsalMutexUnlock(&devSvcManager->mutex);
}

bool DevSvcManagerConstruct(struct DevSvcManager *inst)
{
    struct IDevSvcManager *devSvcMgrIf = NULL;
    if (inst == NULL) {
        HDF_LOGE("%s: inst is null!", __func__);
        return false;
    }
    devSvcMgrIf = &inst->super;
    devSvcMgrIf->AddService = DevSvcManagerAddService;
    devSvcMgrIf->UpdateService = DevSvcManagerUpdateService;
    devSvcMgrIf->SubscribeService = DevSvcManagerSubscribeService;
    devSvcMgrIf->UnsubscribeService = NULL;
    devSvcMgrIf->RemoveService = DevSvcManagerRemoveService;
    devSvcMgrIf->GetService = DevSvcManagerGetService;
    devSvcMgrIf->ListAllService = DevSvcManagerListAllService;
    devSvcMgrIf->GetObject = DevSvcManagerGetObject;
    devSvcMgrIf->RegsterServListener = DevSvcManagerRegsterServListener;
    devSvcMgrIf->UnregsterServListener = DevSvcManagerUnregsterServListener;
    devSvcMgrIf->ListServiceByInterfaceDesc = DevSvcManagerListServiceByInterfaceDesc;
    if (OsalMutexInit(&inst->mutex) != HDF_SUCCESS) {
        HDF_LOGE("failed to create device service manager mutex");
        return false;
    }
    DListHeadInit(&inst->services);
    DListHeadInit(&inst->svcstatListeners);
    return true;
}

int DevSvcManagerStartService(void)
{
    int ret;
    struct IDevSvcManager *svcmgr = DevSvcManagerGetInstance();

    if (svcmgr == NULL) {
        return HDF_ERR_INVALID_OBJECT;
    }
    if (svcmgr->StartService == NULL) {
        return HDF_SUCCESS;
    }

    ret = svcmgr->StartService(svcmgr);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("failed to start service manager");
    }

    return ret;
}

struct HdfObject *DevSvcManagerCreate(void)
{
    static bool isDevSvcManagerInit = false;
    static struct DevSvcManager devSvcManagerInstance;
    if (!isDevSvcManagerInit) {
        if (!DevSvcManagerConstruct(&devSvcManagerInstance)) {
            return NULL;
        }
        isDevSvcManagerInit = true;
    }
    return (struct HdfObject *)&devSvcManagerInstance;
}

void DevSvcManagerRelease(struct IDevSvcManager *inst)
{
    struct DevSvcManager *devSvcManager = CONTAINER_OF(inst, struct DevSvcManager, super);
    if (inst == NULL) {
        return;
    }
    struct DevSvcRecord *record = NULL;
    struct DevSvcRecord *tmp = NULL;
    DLIST_FOR_EACH_ENTRY_SAFE(record, tmp, &devSvcManager->services, struct DevSvcRecord, entry) {
        DevSvcRecordFreeInstance(record);
    }
    OsalMutexDestroy(&devSvcManager->mutex);
}

struct IDevSvcManager *DevSvcManagerGetInstance(void)
{
    static struct IDevSvcManager *instance = NULL;
    if (instance == NULL) {
        instance = (struct IDevSvcManager *)HdfObjectManagerGetObject(HDF_OBJECT_ID_DEVSVC_MANAGER);
    }
    return instance;
}
