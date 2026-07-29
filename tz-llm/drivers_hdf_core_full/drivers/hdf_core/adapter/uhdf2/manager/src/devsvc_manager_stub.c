/*
 * Copyright (c) 2021-2022 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "devsvc_manager_stub.h"

#ifdef WITH_SELINUX
#include <hdf_service_checker.h>
#endif

#include "devmgr_service_stub.h"
#include "devsvc_listener_holder.h"
#include "devsvc_manager_proxy.h"
#include "hdf_cstring.h"
#include "hdf_log.h"
#include "hdf_remote_service.h"
#include "hdf_sbuf.h"
#include "hdf_slist.h"
#include "osal_mem.h"
#include "securec.h"

#define HDF_LOG_TAG devsvc_manager_stub

static int32_t AddServicePermCheck(const char *servName)
{
#ifdef WITH_SELINUX
    pid_t callingPid = HdfRemoteGetCallingPid();
    if (HdfAddServiceCheck(callingPid, servName) != 0) {
        HDF_LOGE("[selinux] %{public}d haven't \"add service\" permission to %{public}s", callingPid, servName);
        return HDF_ERR_NOPERM;
    }
#endif
    return HDF_SUCCESS;
}

static int32_t GetServicePermCheck(const char *servName)
{
#ifdef WITH_SELINUX
    pid_t callingPid = HdfRemoteGetCallingPid();
    if (HdfGetServiceCheck(callingPid, servName) != 0) {
        HDF_LOGE("[selinux] %{public}d haven't \"get service\" permission to %{public}s", callingPid, servName);
        return HDF_ERR_NOPERM;
    }
#endif

    return HDF_SUCCESS;
}

static int32_t ListServicePermCheck(void)
{
#ifdef WITH_SELINUX
    pid_t callingPid = HdfRemoteGetCallingPid();
    if (HdfListServiceCheck(callingPid) != 0) {
        HDF_LOGE("[selinux] %{public}d haven't \"list service\" permission", callingPid);
        return HDF_ERR_NOPERM;
    }
#endif

    return HDF_SUCCESS;
}

static bool CheckServiceObjectValidNoLock(const struct DevSvcManagerStub *stub, const struct HdfDeviceObject *service)
{
    if (service == NULL) {
        HDF_LOGW("%{public}s service object is null", __func__);
        return false;
    }

    struct HdfSListIterator it;
    HdfSListIteratorInit(&it, &stub->devObjHolderList);
    while (HdfSListIteratorHasNext(&it)) {
        struct HdfSListNode *node = HdfSListIteratorNext(&it);
        struct HdfDeviceObjectHolder *holder =
            HDF_SLIST_CONTAINER_OF(struct HdfSListNode, node, struct HdfDeviceObjectHolder, entry);

        if (((uintptr_t)(&holder->devObj) == (uintptr_t)service) && (holder->serviceName != NULL) &&
            (service->priv != NULL) && (strcmp(holder->serviceName, (char *)service->priv) == 0)) {
            HDF_LOGD("%{public}s %{public}s service object is valid", __func__, holder->serviceName);
            return true;
        }
    }

    HDF_LOGW("%{public}s service object is invalid", __func__);
    return false;
}

static bool CheckRemoteObjectValidNoLock(const struct DevSvcManagerStub *stub, const struct HdfRemoteService *service)
{
    if (service == NULL) {
        HDF_LOGW("%{public}s remote object is null", __func__);
        return false;
    }

    struct HdfSListIterator it;
    HdfSListIteratorInit(&it, &stub->devObjHolderList);
    while (HdfSListIteratorHasNext(&it)) {
        struct HdfSListNode *node = HdfSListIteratorNext(&it);
        struct HdfDeviceObjectHolder *holder =
            HDF_SLIST_CONTAINER_OF(struct HdfSListNode, node, struct HdfDeviceObjectHolder, entry);

        if (holder->remoteSvcAddr == (uintptr_t)service) {
            HDF_LOGD("%{public}s remote object is valid", __func__);
            return true;
        }
    }

    HDF_LOGW("%{public}s remote object is invalid", __func__);
    return false;
}

static void ReleaseServiceObjectHolder(struct DevSvcManagerStub *stub, struct HdfDeviceObjectHolder *devObjHolder)
{
    if (devObjHolder != NULL) {
        struct HdfDeviceObject *serviceObject = &devObjHolder->devObj;
        struct HdfRemoteService *serviceRemote = (struct HdfRemoteService *)serviceObject->service;
        HdfRemoteServiceRemoveDeathRecipient(serviceRemote, &stub->recipient);
        HdfRemoteServiceRecycle((struct HdfRemoteService *)serviceObject->service);
        serviceObject->service = NULL;
        OsalMemFree(serviceObject->priv);
        serviceObject->priv = NULL;
        OsalMemFree(devObjHolder->serviceName);
        devObjHolder->serviceName = NULL;
        OsalMemFree(devObjHolder);
    }
}

static struct HdfDeviceObject *ObtainServiceObject(
    struct DevSvcManagerStub *stub, const char *name, struct HdfRemoteService *service)
{
    struct HdfDeviceObjectHolder *serviceObjectHolder = OsalMemCalloc(sizeof(*serviceObjectHolder));
    if (serviceObjectHolder == NULL) {
        return NULL;
    }

    serviceObjectHolder->remoteSvcAddr = (uintptr_t)service;
    serviceObjectHolder->serviceName = (void *)HdfStringCopy(name);
    if (serviceObjectHolder->serviceName == NULL) {
        OsalMemFree(serviceObjectHolder);
        return NULL;
    }

    struct HdfDeviceObject *serviceObject = &serviceObjectHolder->devObj;
    serviceObject->priv = (void *)HdfStringCopy(name);
    if (serviceObject->priv == NULL) {
        OsalMemFree(serviceObjectHolder->serviceName);
        OsalMemFree(serviceObjectHolder);
        return NULL;
    }
    serviceObject->service = (struct IDeviceIoService *)service;
    service->target = (struct HdfObject *)serviceObject;

    OsalMutexLock(&stub->devSvcStubMutex);
    HdfSListAdd(&stub->devObjHolderList, &serviceObjectHolder->entry);
    OsalMutexUnlock(&stub->devSvcStubMutex);

    HdfRemoteServiceAddDeathRecipient(service, &stub->recipient);

    return serviceObject;
}

static void ReleaseServiceObject(struct DevSvcManagerStub *stub, struct HdfDeviceObject *serviceObject)
{
    OsalMutexLock(&stub->devSvcStubMutex);
    if (serviceObject == NULL) {
        OsalMutexUnlock(&stub->devSvcStubMutex);
        return;
    }

    if (!CheckServiceObjectValidNoLock(stub, serviceObject)) {
        OsalMutexUnlock(&stub->devSvcStubMutex);
        return;
    }

    struct HdfRemoteService *serviceRemote = (struct HdfRemoteService *)serviceObject->service;
    HdfRemoteServiceRemoveDeathRecipient(serviceRemote, &stub->recipient);

    struct HdfDeviceObjectHolder *serviceObjectHolder = (struct HdfDeviceObjectHolder *)serviceObject;
    HdfSListRemove(&stub->devObjHolderList, &serviceObjectHolder->entry);
    ReleaseServiceObjectHolder(stub, serviceObjectHolder);

    OsalMutexUnlock(&stub->devSvcStubMutex);
}

static int32_t DevSvcMgrStubGetPara(
    struct HdfSBuf *data, struct HdfServiceInfo *info, struct HdfRemoteService **service)
{
    int ret = HDF_FAILURE;
    info->servName = HdfSbufReadString(data);
    if (info->servName == NULL) {
        HDF_LOGE("%{public}s failed, name is null", __func__);
        return ret;
    }
    ret = AddServicePermCheck(info->servName);
    if (ret != HDF_SUCCESS) {
        return ret;
    }

    info->devClass = DEVICE_CLASS_DEFAULT;
    if (!HdfSbufReadUint16(data, &info->devClass)) {
        HDF_LOGE("%{public}s failed, devClass invalid", __func__);
        return HDF_FAILURE;
    }
    if (!HdfSbufReadUint32(data, &info->devId)) {
        HDF_LOGE("%{public}s failed, devId invalid", __func__);
        return HDF_FAILURE;
    }

    *service = HdfSbufReadRemoteService(data);
    if (*service == NULL) {
        HDF_LOGE("%{public}s failed, service is null", __func__);
        return HDF_FAILURE;
    }
    info->servInfo = HdfSbufReadString(data);
    info->interfaceDesc = HdfSbufReadString(data);
    return HDF_SUCCESS;
}

static int32_t DevSvcManagerStubAddService(struct IDevSvcManager *super, struct HdfSBuf *data)
{
    int ret = HDF_FAILURE;
    struct DevSvcManagerStub *stub = (struct DevSvcManagerStub *)super;
    if (!HdfRemoteServiceCheckInterfaceToken(stub->remote, data)) {
        HDF_LOGE("%{public}s: invalid interface token", __func__);
        return HDF_ERR_INVALID_PARAM;
    }
    struct HdfServiceInfo info;
    struct HdfRemoteService *service = NULL;
    (void)memset_s(&info, sizeof(info), 0, sizeof(info));
    if (DevSvcMgrStubGetPara(data, &info, &service) != HDF_SUCCESS) {
        return ret;
    }

    struct HdfDeviceObject *serviceObject = ObtainServiceObject(stub, info.servName, service);
    if (serviceObject == NULL) {
        return HDF_ERR_MALLOC_FAIL;
    }

    struct HdfDeviceObject *oldServiceObject = super->GetObject(super, info.servName);
    ret = super->AddService(super, serviceObject, &info);
    if (ret != HDF_SUCCESS) {
        ReleaseServiceObject(stub, serviceObject);
    } else {
        ReleaseServiceObject(stub, oldServiceObject);
    }
    HDF_LOGI("add service %{public}s, %{public}d", info.servName, ret);
    return ret;
}

static int32_t DevSvcManagerStubUpdateService(struct IDevSvcManager *super, struct HdfSBuf *data)
{
    int ret = HDF_FAILURE;
    struct DevSvcManagerStub *stub = (struct DevSvcManagerStub *)super;
    if (!HdfRemoteServiceCheckInterfaceToken(stub->remote, data)) {
        HDF_LOGE("%{public}s: invalid interface token", __func__);
        return HDF_ERR_INVALID_PARAM;
    }

    struct HdfRemoteService *service = NULL;
    struct HdfServiceInfo info;
    (void)memset_s(&info, sizeof(info), 0, sizeof(info));
    if (DevSvcMgrStubGetPara(data, &info, &service) != HDF_SUCCESS) {
        return ret;
    }

    struct HdfDeviceObject *oldServiceObject = super->GetObject(super, info.servName);
    if (oldServiceObject == NULL) {
        HDF_LOGE("update service %{public}s not exist", info.servName);
        return HDF_DEV_ERR_NO_DEVICE_SERVICE;
    }

    struct HdfDeviceObject *serviceObject = ObtainServiceObject(stub, info.servName, service);
    if (serviceObject == NULL) {
        return HDF_ERR_MALLOC_FAIL;
    }

    ret = super->UpdateService(super, serviceObject, &info);
    if (ret != HDF_SUCCESS) {
        ReleaseServiceObject(stub, serviceObject);
    } else {
        ReleaseServiceObject(stub, oldServiceObject);
    }
    HDF_LOGI("update service %{public}s, %{public}d", info.servName, ret);
    return ret;
}

static int32_t DevSvcManagerStubGetService(struct IDevSvcManager *super, struct HdfSBuf *data, struct HdfSBuf *reply)
{
    int ret = HDF_FAILURE;
    struct DevSvcManagerStub *stub = (struct DevSvcManagerStub *)super;
    if (!HdfRemoteServiceCheckInterfaceToken(stub->remote, data)) {
        HDF_LOGE("%{public}s: invalid interface token", __func__);
        return HDF_ERR_INVALID_PARAM;
    }
    const char *name = HdfSbufReadString(data);
    if (name == NULL) {
        HDF_LOGE("%{public}s failed, name is null", __func__);
        return ret;
    }
    ret = GetServicePermCheck(name);
    if (ret != HDF_SUCCESS) {
        return ret;
    }
    struct HdfDeviceObject *serviceObject = super->GetObject(super, name);
    if (serviceObject == NULL) {
        HDF_LOGE("StubGetService service %{public}s not found", name);
        return HDF_FAILURE;
    }

    const char *svcMgrName = "hdf_device_manager";
    if (strcmp(name, svcMgrName) != 0) {
        OsalMutexLock(&stub->devSvcStubMutex);
        if (!CheckServiceObjectValidNoLock(stub, serviceObject)) {
            OsalMutexUnlock(&stub->devSvcStubMutex);
            HDF_LOGE("StubGetService service %{public}s is invalid", name);
            return HDF_FAILURE;
        }
    }
    struct HdfRemoteService *remoteService = (struct HdfRemoteService *)serviceObject->service;
    if (remoteService != NULL) {
        HdfSbufWriteRemoteService(reply, remoteService);
        ret = HDF_SUCCESS;
        HDF_LOGI("StubGetService service %{public}s found", name);
    } else {
        HDF_LOGE("StubGetService %{public}s remoteService is null", name);
        ret = HDF_FAILURE;
    }

    if (strcmp(name, svcMgrName) != 0) {
        OsalMutexUnlock(&stub->devSvcStubMutex);
    }

    return ret;
}

static int32_t DevSvcManagerStubListAllService(
    struct IDevSvcManager *super, struct HdfSBuf *data, struct HdfSBuf *reply)
{
    struct DevSvcManagerStub *stub = (struct DevSvcManagerStub *)super;
    if (!HdfRemoteServiceCheckInterfaceToken(stub->remote, data)) {
        HDF_LOGE("%{public}s: invalid interface token", __func__);
        return HDF_ERR_INVALID_PARAM;
    }
    int ret = ListServicePermCheck();
    if (ret != HDF_SUCCESS) {
        return ret;
    }
    super->ListAllService(super, reply);

    return HDF_SUCCESS;
}

static int32_t DevSvcManagerStubListServiceByInterfaceDesc(
    struct IDevSvcManager *super, struct HdfSBuf *data, struct HdfSBuf *reply)
{
    int ret;
    struct DevSvcManagerStub *stub = (struct DevSvcManagerStub *)super;
    if (!HdfRemoteServiceCheckInterfaceToken(stub->remote, data)) {
        HDF_LOGE("%{public}s: invalid interface token", __func__);
        return HDF_ERR_INVALID_PARAM;
    }
    const char *interfaceDesc = HdfSbufReadString(data);
    if (interfaceDesc == NULL) {
        HDF_LOGE("%{public}s failed, interfaceDesc is null", __func__);
        return HDF_FAILURE;
    }
    ret = ListServicePermCheck();
    if (ret != HDF_SUCCESS) {
        return ret;
    }
    ret = super->ListServiceByInterfaceDesc(super, interfaceDesc, reply);

    return ret;
}

static int32_t DevSvcManagerStubRemoveService(struct IDevSvcManager *super, struct HdfSBuf *data)
{
    struct DevSvcManagerStub *stub = (struct DevSvcManagerStub *)super;
    if (!HdfRemoteServiceCheckInterfaceToken(stub->remote, data)) {
        HDF_LOGE("%{public}s: invalid interface token", __func__);
        return HDF_ERR_INVALID_PARAM;
    }
    const char *name = HdfSbufReadString(data);
    if (name == NULL) {
        HDF_LOGE("%{public}s failed, name is null", __func__);
        return HDF_FAILURE;
    }
    int32_t ret = AddServicePermCheck(name);
    if (ret != HDF_SUCCESS) {
        return ret;
    }
    struct HdfDeviceObject *serviceObject = super->GetObject(super, name);
    if (serviceObject == NULL) {
        HDF_LOGE("remove service %{public}s not exist", name);
        return HDF_DEV_ERR_NO_DEVICE_SERVICE;
    }

    OsalMutexLock(&stub->devSvcStubMutex);
    if (!CheckServiceObjectValidNoLock(stub, serviceObject)) {
        OsalMutexUnlock(&stub->devSvcStubMutex);
        HDF_LOGI("StubRemoveService service %{public}s is invalid", name);
        return HDF_FAILURE;
    }

    const char *servName = (const char *)serviceObject->priv;
    if (servName == NULL) {
        OsalMutexUnlock(&stub->devSvcStubMutex);
        HDF_LOGE("remove service %{public}s is broken object", name);
        return HDF_ERR_INVALID_OBJECT;
    }

    if (strcmp(name, servName) != 0) {
        OsalMutexUnlock(&stub->devSvcStubMutex);
        HDF_LOGE("remove service %{public}s name mismatch with %{public}s", name, servName);
        return HDF_ERR_INVALID_OBJECT;
    }
    OsalMutexUnlock(&stub->devSvcStubMutex);
    super->RemoveService(super, name, serviceObject);
    HDF_LOGI("service %{public}s removed", name);

    ReleaseServiceObject(stub, serviceObject);
    return HDF_SUCCESS;
}
static int32_t DevSvcManagerStubRegisterServListener(struct IDevSvcManager *super, struct HdfSBuf *data)
{
    struct DevSvcManagerStub *stub = (struct DevSvcManagerStub *)super;
    if (!HdfRemoteServiceCheckInterfaceToken(stub->remote, data)) {
        HDF_LOGE("%{public}s: invalid interface token", __func__);
        return HDF_ERR_INVALID_PARAM;
    }
    uint16_t listenClass = DEVICE_CLASS_DEFAULT;
    if (!HdfSbufReadUint16(data, &listenClass)) {
        return HDF_ERR_INVALID_PARAM;
    }
    struct HdfRemoteService *listenerRemote = HdfSbufReadRemoteService(data);
    if (listenerRemote == NULL) {
        return HDF_ERR_INVALID_PARAM;
    }

    struct ServStatListenerHolder *listenerHolder =
        ServStatListenerHolderCreate((uintptr_t)listenerRemote, listenClass);
    if (listenerHolder == NULL) {
        HdfRemoteServiceRecycle(listenerRemote);
        return HDF_ERR_MALLOC_FAIL;
    }

    int ret = super->RegsterServListener(super, listenerHolder);
    if (ret != HDF_SUCCESS) {
        ServStatListenerHolderRelease(listenerHolder);
    } else {
        HDF_LOGI("register servstat listener success");
    }

    return HDF_SUCCESS;
}

static int32_t DevSvcManagerStubUnregisterServListener(struct IDevSvcManager *super, struct HdfSBuf *data)
{
    struct DevSvcManagerStub *stub = (struct DevSvcManagerStub *)super;
    if (!HdfRemoteServiceCheckInterfaceToken(stub->remote, data)) {
        HDF_LOGE("%{public}s: invalid interface token", __func__);
        return HDF_ERR_INVALID_PARAM;
    }
    struct HdfRemoteService *listenerRemote = HdfSbufReadRemoteService(data);
    if (listenerRemote == NULL) {
        return HDF_ERR_INVALID_PARAM;
    }
    struct ServStatListenerHolder *listenerHolder = ServStatListenerHolderGet(listenerRemote->index);
    if (listenerHolder == NULL) {
        HDF_LOGE("failed to unregister svcstat listener, unknown listener");
        HdfRemoteServiceRecycle(listenerRemote);
        return HDF_ERR_INVALID_OBJECT;
    }
    super->UnregsterServListener(super, listenerHolder);
    ServStatListenerHolderRelease(listenerHolder);
    HdfRemoteServiceRecycle(listenerRemote);
    HDF_LOGI("unregister servstat listener success");
    return HDF_SUCCESS;
}

int DevSvcManagerStubDispatch(struct HdfRemoteService *service, int code, struct HdfSBuf *data, struct HdfSBuf *reply)
{
    int ret = HDF_FAILURE;
    struct DevSvcManagerStub *stub = (struct DevSvcManagerStub *)service;
    if (stub == NULL) {
        HDF_LOGE("DevSvcManagerStubDispatch failed, object is null, code is %{public}d", code);
        return ret;
    }
    struct IDevSvcManager *super = (struct IDevSvcManager *)&stub->super;
    HDF_LOGD("DevSvcManagerStubDispatch called: code=%{public}d", code);
    switch (code) {
        case DEVSVC_MANAGER_ADD_SERVICE:
            ret = DevSvcManagerStubAddService(super, data);
            break;
        case DEVSVC_MANAGER_UPDATE_SERVICE:
            ret = DevSvcManagerStubUpdateService(super, data);
            break;
        case DEVSVC_MANAGER_GET_SERVICE:
            ret = DevSvcManagerStubGetService(super, data, reply);
            break;
        case DEVSVC_MANAGER_REMOVE_SERVICE:
            ret = DevSvcManagerStubRemoveService(super, data);
            break;
        case DEVSVC_MANAGER_REGISTER_SVCLISTENER:
            ret = DevSvcManagerStubRegisterServListener(super, data);
            break;
        case DEVSVC_MANAGER_UNREGISTER_SVCLISTENER:
            ret = DevSvcManagerStubUnregisterServListener(super, data);
            break;
        case DEVSVC_MANAGER_LIST_ALL_SERVICE:
            ret = DevSvcManagerStubListAllService(super, data, reply);
            break;
        case DEVSVC_MANAGER_LIST_SERVICE_BY_INTERFACEDESC:
            ret = DevSvcManagerStubListServiceByInterfaceDesc(super, data, reply);
            break;
        default:
            ret = HdfRemoteServiceDefaultDispatch(stub->remote, code, data, reply);
            break;
    }
    return ret;
}

void DevSvcManagerOnServiceDied(struct HdfDeathRecipient *recipient, struct HdfRemoteService *remote)
{
    struct DevSvcManagerStub *stub =
        HDF_SLIST_CONTAINER_OF(struct HdfDeathRecipient, recipient, struct DevSvcManagerStub, recipient);
    if (stub == NULL) {
        return;
    }

    struct IDevSvcManager *iSvcMgr = &stub->super.super;
    if (iSvcMgr->GetService == NULL || iSvcMgr->RemoveService == NULL) {
        HDF_LOGI("%{public}s:invalid svcmgr object", __func__);
        return;
    }

    OsalMutexLock(&stub->devSvcStubMutex);
    if (!CheckRemoteObjectValidNoLock(stub, remote)) {
        OsalMutexUnlock(&stub->devSvcStubMutex);
        return;
    }
    struct HdfDeviceObject *serviceObject = (struct HdfDeviceObject *)remote->target;

    if (!CheckServiceObjectValidNoLock(stub, serviceObject)) {
        OsalMutexUnlock(&stub->devSvcStubMutex);
        return;
    }

    char *serviceName = HdfStringCopy((char *)serviceObject->priv);
    OsalMutexUnlock(&stub->devSvcStubMutex);
    if (serviceName == NULL) {
        HDF_LOGI("%{public}s HdfStringCopy fail", __func__);
        return;
    }

    HDF_LOGI("service %{public}s died", serviceName);
    iSvcMgr->RemoveService(iSvcMgr, serviceName, serviceObject);

    ReleaseServiceObject(stub, serviceObject);
    OsalMemFree(serviceName);
}

int DevSvcManagerStubStart(struct IDevSvcManager *svcmgr)
{
    struct DevSvcManagerStub *inst = (struct DevSvcManagerStub *)svcmgr;
    if (inst == NULL) {
        return HDF_ERR_INVALID_PARAM;
    }
    if (inst->started) {
        return HDF_SUCCESS;
    }

    ServStatListenerHolderinit();

    static struct HdfRemoteDispatcher dispatcher = {.Dispatch = DevSvcManagerStubDispatch};
    inst->remote = HdfRemoteServiceObtain((struct HdfObject *)inst, &dispatcher);
    if (inst->remote == NULL) {
        HDF_LOGE("failed to obtain device service manager remote service");
        return HDF_ERR_MALLOC_FAIL;
    }
    if (!HdfRemoteServiceSetInterfaceDesc(inst->remote, "HDI.IServiceManager.V1_0")) {
        HDF_LOGE("%{public}s: failed to init interface desc", __func__);
        HdfRemoteServiceRecycle(inst->remote);
        return HDF_ERR_INVALID_OBJECT;
    }

    inst->recipient.OnRemoteDied = DevSvcManagerOnServiceDied;
    int ret = HdfRemoteServiceRegister(DEVICE_SERVICE_MANAGER_SA_ID, inst->remote);
    if (ret != 0) {
        HDF_LOGE("failed to publish device service manager, %{public}d", ret);
        HdfRemoteServiceRecycle(inst->remote);
        inst->remote = NULL;
    } else {
        HDF_LOGI("publish device service manager success");
        inst->started = true;
    }

    return ret;
}

static bool DevSvcManagerStubConstruct(struct DevSvcManagerStub *inst)
{
    if (inst == NULL) {
        return false;
    }
    if (!DevSvcManagerConstruct(&inst->super)) {
        HDF_LOGE("failed to construct device service manager");
        return false;
    }
    inst->super.super.StartService = DevSvcManagerStubStart;
    OsalMutexInit(&inst->devSvcStubMutex);
    HdfSListInit(&inst->devObjHolderList);

    return true;
}

struct HdfObject *DevSvcManagerStubCreate(void)
{
    static struct DevSvcManagerStub *instance;
    if (instance != NULL) {
        return (struct HdfObject *)instance;
    }

    instance = OsalMemCalloc(sizeof(struct DevSvcManagerStub));
    if (!DevSvcManagerStubConstruct(instance)) {
        OsalMemFree(instance);
        instance = NULL;
    }

    return (struct HdfObject *)instance;
}

static void DevObjHolderListReleaseNoLock(struct DevSvcManagerStub *stub)
{
    struct HdfSListIterator it;
    HdfSListIteratorInit(&it, &stub->devObjHolderList);
    while (HdfSListIteratorHasNext(&it)) {
        struct HdfSListNode *node = HdfSListIteratorNext(&it);
        HdfSListIteratorRemove(&it);
        struct HdfDeviceObjectHolder *holder =
            HDF_SLIST_CONTAINER_OF(struct HdfSListNode, node, struct HdfDeviceObjectHolder, entry);

        ReleaseServiceObjectHolder(stub, holder);
    }
}

void DevSvcManagerStubRelease(struct HdfObject *object)
{
    struct DevSvcManagerStub *instance = (struct DevSvcManagerStub *)object;
    if (instance != NULL) {
        if (instance->remote != NULL) {
            HdfRemoteServiceRecycle(instance->remote);
            instance->remote = NULL;
        }
        OsalMutexLock(&instance->devSvcStubMutex);
        DevObjHolderListReleaseNoLock(instance);
        OsalMutexUnlock(&instance->devSvcStubMutex);
        OsalMutexDestroy(&instance->devSvcStubMutex);
    }
}
