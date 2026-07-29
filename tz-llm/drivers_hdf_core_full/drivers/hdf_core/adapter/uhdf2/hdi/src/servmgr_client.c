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

#include <devsvc_manager_proxy.h>
#include <devsvc_manager_stub.h>
#include <hdf_base.h>
#include <hdf_log.h>
#include <osal_mem.h>
#include "hdf_cstring.h"
#include "hdf_service_status.h"
#include "servmgr_hdi.h"

#define SERVICE_LIST_MAX 16

struct HDIServiceManagerClient {
    struct HDIServiceManager iservmgr;
    struct HdfRemoteService *remote;
};
int ServiceStatusListenerMarshalling(struct ServiceStatusListener *listener, struct HdfSBuf *buf);

static int32_t ServiceManagerHdiCall(struct HDIServiceManagerClient *servMgrClient, int32_t id,
    struct HdfSBuf *data, struct HdfSBuf *reply)
{
    if (servMgrClient->remote == NULL || servMgrClient->remote->dispatcher == NULL ||
        servMgrClient->remote->dispatcher->Dispatch == NULL) {
        return HDF_ERR_INVALID_OBJECT;
    }

    return servMgrClient->remote->dispatcher->Dispatch(servMgrClient->remote, id, data, reply);
}

static struct HdiServiceSet *HdiServiceSetGetInstance(const uint32_t serviceNum)
{
    struct HdiServiceSet *serviceSet = OsalMemAlloc(sizeof(struct HdiServiceSet));
    if (serviceSet == NULL) {
        HDF_LOGE("%{public}s: OOM", __func__);
        return NULL;
    }
    serviceSet->serviceNames = OsalMemCalloc(sizeof(char *) * serviceNum);
    if (serviceSet->serviceNames == NULL) {
        HDF_LOGE("%{public}s: OOM", __func__);
        HdiServiceSetRelease(serviceSet);
        return NULL;
    }
    serviceSet->count = serviceNum;
    return serviceSet;
}

static int32_t HdiServiceSetUnMarshalling(struct HdfSBuf *buf, struct HdiServiceSet *serviceSet)
{
    for (uint32_t i = 0; i < serviceSet->count; i++) {
        const char *serviceName = HdfSbufReadString(buf);
        if (serviceName == NULL) {
            break;
        }
        serviceSet->serviceNames[i] = HdfStringCopy(serviceName);
    }

    return HDF_SUCCESS;
}

struct HdiServiceSet *HDIServMgrListServiceByInterfaceDesc(
    struct HDIServiceManager *iServMgr, const char *interfaceDesc)
{
    if (iServMgr == NULL || interfaceDesc == NULL || strlen(interfaceDesc) == 0) {
        return NULL;
    }
    struct HDIServiceManagerClient *servMgrClient = CONTAINER_OF(iServMgr, struct HDIServiceManagerClient, iservmgr);
    struct HdiServiceSet *serviceSet = NULL;
    struct HdfSBuf *data = NULL;
    struct HdfSBuf *reply = NULL;
    uint32_t serviceNum = 0;

    do {
        data = HdfSbufTypedObtain(SBUF_IPC);
        reply = HdfSbufTypedObtain(SBUF_IPC);
        if (data == NULL || reply == NULL) {
            break;
        }
        if (!HdfRemoteServiceWriteInterfaceToken(servMgrClient->remote, data) ||
            !HdfSbufWriteString(data, interfaceDesc)) {
            break;
        }
        int32_t status =
            ServiceManagerHdiCall(servMgrClient, DEVSVC_MANAGER_LIST_SERVICE_BY_INTERFACEDESC, data, reply);
        if (status != HDF_SUCCESS) {
            HDF_LOGE("%{public}s: failed to get %{public}s service collection, the status is %{public}d", __func__,
                interfaceDesc, status);
            break;
        }
        if (!HdfSbufReadUint32(reply, &serviceNum)) {
            break;
        }
        if (serviceNum == 0 || serviceNum > SERVICE_LIST_MAX) {
            break;
        }
        serviceSet = HdiServiceSetGetInstance(serviceNum);
        if (serviceSet == NULL) {
            break;
        }
        HdiServiceSetUnMarshalling(reply, serviceSet);
    } while (0);

    HdfSbufRecycle(reply);
    HdfSbufRecycle(data);

    return serviceSet;
}

struct HdfRemoteService *HDIServMgrGetService(struct HDIServiceManager *iServMgr, const char* serviceName)
{
    if (iServMgr == NULL || serviceName == NULL) {
        return NULL;
    }
    struct HDIServiceManagerClient *servMgrClient = CONTAINER_OF(iServMgr, struct HDIServiceManagerClient, iservmgr);
    struct HdfSBuf *data = NULL;
    struct HdfSBuf *reply = NULL;
    struct HdfRemoteService *service = NULL;

    do {
        data = HdfSbufTypedObtain(SBUF_IPC);
        reply = HdfSbufTypedObtain(SBUF_IPC);
        if (data == NULL || reply == NULL) {
            break;
        }

        if (!HdfRemoteServiceWriteInterfaceToken(servMgrClient->remote, data) ||
            !HdfSbufWriteString(data, serviceName)) {
            break;
        }
        int status = ServiceManagerHdiCall(servMgrClient, DEVSVC_MANAGER_GET_SERVICE, data, reply);
        if (status == HDF_SUCCESS) {
            service = HdfSbufReadRemoteService(reply);
        } else {
            HDF_LOGI("%{public}s: %{public}s not found", __func__, serviceName);
        }
    } while (0);

    HdfSbufRecycle(reply);
    HdfSbufRecycle(data);

    return service;
}

int32_t HDIServMgrRegisterServiceStatusListener(struct HDIServiceManager *self,
    struct ServiceStatusListener *listener, uint16_t deviceClass)
{
    if (self == NULL || listener == NULL) {
        return HDF_ERR_INVALID_PARAM;
    }
    struct HDIServiceManagerClient *servMgrClient = CONTAINER_OF(self, struct HDIServiceManagerClient, iservmgr);

    struct HdfSBuf *data = HdfSbufTypedObtain(SBUF_IPC);
    if (data == NULL) {
        return HDF_ERR_MALLOC_FAIL;
    }

    if (!HdfRemoteServiceWriteInterfaceToken(servMgrClient->remote, data) ||
        !HdfSbufWriteUint16(data, deviceClass) ||
        ServiceStatusListenerMarshalling(listener, data) != HDF_SUCCESS) {
        HdfSbufRecycle(data);
        return HDF_FAILURE;
    }

    int32_t ret = ServiceManagerHdiCall(servMgrClient, DEVSVC_MANAGER_REGISTER_SVCLISTENER, data, NULL);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("failed to register hdi service listener");
    }
    HdfSbufRecycle(data);
    return ret;
}

int32_t HDIServMgrUnregisterServiceStatusListener(struct HDIServiceManager *self,
    struct ServiceStatusListener *listener)
{
    if (self == NULL || listener == NULL) {
        return HDF_ERR_INVALID_PARAM;
    }
    struct HDIServiceManagerClient *servMgrClient = CONTAINER_OF(self, struct HDIServiceManagerClient, iservmgr);
    struct HdfSBuf *data = HdfSbufTypedObtain(SBUF_IPC);
    if (data == NULL) {
        return HDF_ERR_MALLOC_FAIL;
    }

    if (!HdfRemoteServiceWriteInterfaceToken(servMgrClient->remote, data) ||
        ServiceStatusListenerMarshalling(listener, data) != HDF_SUCCESS) {
        HdfSbufRecycle(data);
        return HDF_FAILURE;
    }

    int32_t ret = ServiceManagerHdiCall(servMgrClient, DEVSVC_MANAGER_UNREGISTER_SVCLISTENER, data, NULL);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("failed to unregister hdi service listener");
    }
    HdfSbufRecycle(data);
    return ret;
}

void HDIServiceManagerConstruct(struct HDIServiceManager *inst)
{
    inst->GetService = HDIServMgrGetService;
    inst->RegisterServiceStatusListener = HDIServMgrRegisterServiceStatusListener;
    inst->UnregisterServiceStatusListener = HDIServMgrUnregisterServiceStatusListener;
    inst->ListServiceByInterfaceDesc = HDIServMgrListServiceByInterfaceDesc;
}

struct HDIServiceManager *HDIServiceManagerGet(void)
{
    struct HdfRemoteService *remote = HdfRemoteServiceGet(DEVICE_SERVICE_MANAGER_SA_ID);
    if (remote == NULL) {
        HDF_LOGE("%{public}s: hdi service %{public}s not found", __func__, DEVICE_SERVICE_MANAGER);
        return NULL;
    }

    struct HDIServiceManagerClient *iServMgrClient = OsalMemAlloc(sizeof(struct HDIServiceManagerClient));
    if (iServMgrClient == NULL) {
        HDF_LOGE("%{public}s: OOM", __func__);
        HdfRemoteServiceRecycle(remote);
        return NULL;
    }
    if (!HdfRemoteServiceSetInterfaceDesc(remote, "HDI.IServiceManager.V1_0")) {
        HDF_LOGE("%{public}s: failed to init interface desc", __func__);
        HdfRemoteServiceRecycle(remote);
        OsalMemFree(iServMgrClient);
        return NULL;
    }
    iServMgrClient->remote = remote;

    HDIServiceManagerConstruct(&iServMgrClient->iservmgr);
    return &iServMgrClient->iservmgr;
}

void HDIServiceManagerRelease(struct HDIServiceManager *servmgr)
{
    if (servmgr == NULL) {
        return;
    }
    struct HDIServiceManagerClient *iServMgrClient = CONTAINER_OF(servmgr, struct HDIServiceManagerClient, iservmgr);
    HdfRemoteServiceRecycle(iServMgrClient->remote);
    OsalMemFree(iServMgrClient);
}

int32_t HdiServiceSetRelease(struct HdiServiceSet *serviceSet)
{
    if (serviceSet == NULL) {
        return HDF_SUCCESS;
    }
    if (serviceSet->count > SERVICE_LIST_MAX) {
        HDF_LOGE("%{public}s: failed to release serviceSet, serviceSet->count is tainted", __func__);
        return HDF_FAILURE;
    }
    if (serviceSet->serviceNames != NULL) {
        for (uint32_t i = 0; i < serviceSet->count; i++) {
            if (serviceSet->serviceNames[i] != NULL) {
                OsalMemFree((void *)serviceSet->serviceNames[i]);
                serviceSet->serviceNames[i] = NULL;
            }
        }
        OsalMemFree(serviceSet->serviceNames);
        serviceSet->serviceNames = NULL;
    }
    OsalMemFree(serviceSet);
    return HDF_SUCCESS;
}
