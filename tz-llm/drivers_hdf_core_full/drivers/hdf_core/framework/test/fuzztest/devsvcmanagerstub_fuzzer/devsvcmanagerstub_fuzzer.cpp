/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include "devsvcmanagerstub_fuzzer.h"
#include "devsvc_manager_proxy.h"
#include "devsvc_manager_stub.h"
#include "hdf_log.h"

extern "C" int DevSvcManagerStubDispatch(
    struct HdfRemoteService *service, int code, struct HdfSBuf *data, struct HdfSBuf *reply);

static const char* g_svcmgrInterfaceToken = "HDI.IServiceManager.V1_0";
static struct HdfRemoteDispatcher g_servmgrDispatcher = {
    .Dispatch = DevSvcManagerStubDispatch
};

static int32_t servmgrCode[] = {
    DEVSVC_MANAGER_ADD_SERVICE,
    DEVSVC_MANAGER_UPDATE_SERVICE,
    DEVSVC_MANAGER_GET_SERVICE,
    DEVSVC_MANAGER_REGISTER_SVCLISTENER,
    DEVSVC_MANAGER_UNREGISTER_SVCLISTENER,
    DEVSVC_MANAGER_LIST_ALL_SERVICE,
    DEVSVC_MANAGER_LIST_SERVICE,
    DEVSVC_MANAGER_REMOVE_SERVICE,
    DEVSVC_MANAGER_LIST_SERVICE_BY_INTERFACEDESC,
};

static struct DevSvcManagerStub *GetDevSvcManagerStubInstance()
{
    static struct DevSvcManagerStub *instance;
    if (instance != NULL) {
        return instance;
    }

    instance = reinterpret_cast<struct DevSvcManagerStub *>(DevSvcManagerStubCreate());
    if (instance == nullptr) {
        HDF_LOGI("%{public}s:%{public}d: failed to create DevSvcManagerStub object", __func__, __LINE__);
        return nullptr;
    }

    struct HdfRemoteService *remoteService = HdfRemoteServiceObtain(
        reinterpret_cast<struct HdfObject *>(instance), &g_servmgrDispatcher);
    if (remoteService == nullptr) {
        HDF_LOGI("%{public}s:%{public}d: failed to bind dispatcher", __func__, __LINE__);
        return nullptr;
    }

    if (!HdfRemoteServiceSetInterfaceDesc(remoteService, g_svcmgrInterfaceToken)) {
        HDF_LOGE("%{public}s: failed to init interface desc", __func__);
        HdfRemoteServiceRecycle(remoteService);
        return nullptr;
    }

    instance->remote = remoteService;
    return instance;
}

static bool DevsvcManagerFuzzTest(int32_t code, const uint8_t *data, size_t size)
{
    struct DevSvcManagerStub *instance = GetDevSvcManagerStubInstance();
    if (instance == nullptr) {
        HDF_LOGE("%{public}s:%{public}d: failed to get DevSvcManagerStub object", __func__, __LINE__);
        return false;
    }

    struct HdfSBuf *dataBuf = HdfSbufTypedObtain(SBUF_IPC);
    if (dataBuf == nullptr) {
        HDF_LOGE("%{public}s:%{public}d: failed to create data buf", __func__, __LINE__);
        return false;
    }

    struct HdfSBuf *replyBuf = HdfSbufTypedObtain(SBUF_IPC);
    if (replyBuf == nullptr) {
        HDF_LOGE("%{public}s:%{public}d: failed to create reply buf", __func__, __LINE__);
        HdfSbufRecycle(dataBuf);
        return false;
    }

    if (!HdfRemoteServiceWriteInterfaceToken(instance->remote, dataBuf)) {
        HDF_LOGE("%{public}s:%{public}d: failed to write interface token", __func__, __LINE__);
        HdfSbufRecycle(dataBuf);
        HdfSbufRecycle(replyBuf);
        return false;
    }

    if (!HdfSbufWriteBuffer(dataBuf, data, size)) {
        HDF_LOGE("%{public}s:%{public}d: failed to write data", __func__, __LINE__);
        HdfSbufRecycle(dataBuf);
        HdfSbufRecycle(replyBuf);
        return false;
    }

    (void)instance->remote->dispatcher->Dispatch(
        reinterpret_cast<HdfRemoteService *>(instance->remote->target), code, dataBuf, replyBuf);

    HdfSbufRecycle(dataBuf);
    HdfSbufRecycle(replyBuf);
    return true;
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    int32_t codeSize = sizeof(servmgrCode) / sizeof(servmgrCode[0]);
    for (int32_t i = 0; i < codeSize; ++i) {
        int32_t code = servmgrCode[i];
        DevsvcManagerFuzzTest(code, data, size);
    }
    return HDF_SUCCESS;
}

