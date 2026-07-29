/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include "devmgrservicestub_fuzzer.h"
#include "devmgr_service_stub.h"
#include "hdf_base.h"
#include "hdf_log.h"

extern "C" int32_t DevmgrServiceStubDispatch(
    struct HdfRemoteService *stub, int code, struct HdfSBuf *data, struct HdfSBuf *reply);

static const char *g_devmgrInterfaceToken = "HDI.IDeviceManager.V1_0";
static struct HdfRemoteDispatcher g_devmgrDispatcher = {
    .Dispatch = DevmgrServiceStubDispatch,
};

static int32_t g_devMgrCode[] = {
    DEVMGR_SERVICE_ATTACH_DEVICE_HOST,
    DEVMGR_SERVICE_ATTACH_DEVICE,
    DEVMGR_SERVICE_DETACH_DEVICE,
    DEVMGR_SERVICE_LOAD_DEVICE,
    DEVMGR_SERVICE_UNLOAD_DEVICE,
    DEVMGR_SERVICE_QUERY_DEVICE,
    DEVMGR_SERVICE_LIST_ALL_DEVICE
};

static struct DevmgrServiceStub *GetDevmgrServiceStubInstance()
{
    static struct DevmgrServiceStub *instance = nullptr;
    if (instance != nullptr) {
        return instance;
    }

    instance = reinterpret_cast<struct DevmgrServiceStub *>(DevmgrServiceStubCreate());
    if (instance == nullptr) {
        HDF_LOGI("%{public}s:%{public}d: failed to create DevmgrServiceStub object", __func__, __LINE__);
        return nullptr;
    }

    struct HdfRemoteService *remoteService = HdfRemoteServiceObtain(
        reinterpret_cast<struct HdfObject *>(instance), &g_devmgrDispatcher);
    if (remoteService == nullptr) {
        HDF_LOGI("%{public}s:%{public}d: failed to bind dispatcher", __func__, __LINE__);
        return nullptr;
    }
    if (!HdfRemoteServiceSetInterfaceDesc(remoteService, g_devmgrInterfaceToken)) {
        HDF_LOGE("%{public}s:%{public}d: failed to init interface desc", __func__, __LINE__);
        HdfRemoteServiceRecycle(remoteService);
        return nullptr;
    }

    instance->remote = remoteService;
    return instance;
}

static bool AttachDeviceHostFuzzTest(int32_t code, const uint8_t *data, size_t size)
{
    struct DevmgrServiceStub *instance = GetDevmgrServiceStubInstance();
    if (instance == nullptr) {
        HDF_LOGE("%{public}s:%{public}d: failed to get DevmgrServiceStub object", __func__, __LINE__);
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
    int32_t codeSize = sizeof(g_devMgrCode) / sizeof(g_devMgrCode[0]);
    for (int32_t i = 0; i < codeSize; ++i) {
        int32_t code = g_devMgrCode[i];
        AttachDeviceHostFuzzTest(code, data, size);
    }
    return HDF_SUCCESS;
}