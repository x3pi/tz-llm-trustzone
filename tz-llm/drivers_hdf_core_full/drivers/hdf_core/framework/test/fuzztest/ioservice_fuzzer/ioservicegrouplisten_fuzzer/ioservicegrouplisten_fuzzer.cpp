/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include "ioservicegrouplisten_fuzzer.h"
#include "hdf_base.h"
#include "hdf_log.h"
#include "hdf_io_service.h"
#include "osal_time.h"
#include "parcel.h"

namespace OHOS {
constexpr size_t THRESHOLD = 10;

struct Eventlistener {
    struct HdfDevEventlistener listener;
    int32_t eventCount;
};
static int OnDevEventReceived(
    struct HdfDevEventlistener *listener, struct HdfIoService *service, uint32_t id, struct HdfSBuf *data);

static struct Eventlistener g_listener0 = {
    .listener.onReceive = OnDevEventReceived,
    .listener.priv = const_cast<void *>(static_cast<const void *>("listener0")),
    .eventCount = 0,
};

static int OnDevEventReceived(
    struct HdfDevEventlistener *listener, struct HdfIoService *service, uint32_t id, struct HdfSBuf *data)
{
    OsalTimespec time;
    OsalGetTime(&time);
    (void)service;
    (void)id;
    const char *string = HdfSbufReadString(data);
    if (string == nullptr) {
        HDF_LOGE("failed to read string in event data");
        return HDF_SUCCESS;
    }
    struct Eventlistener *listenercount = CONTAINER_OF(listener, struct Eventlistener, listener);
    listenercount->eventCount++;
    return HDF_SUCCESS;
}

bool IoserviceGroupListenFuzzTest(const uint8_t *data, size_t size)
{
    if (data == nullptr) {
        HDF_LOGE("%{public}s: data is nullptr!", __func__);
        return false;
    }

    bool result = false;
    Parcel parcel;
    parcel.WriteBuffer(data, size);
    auto servicename = parcel.ReadCString();
    struct HdfIoService *serv = HdfIoServiceBind(servicename);
    if (serv == nullptr) {
        HDF_LOGE("%{public}s: HdfIoServiceBind failed!", __func__);
        return false;
    }
    struct HdfIoServiceGroup *group = HdfIoServiceGroupObtain();
    if (group == nullptr) {
        HDF_LOGE("%{public}s: HdfIoServiceGroupObtain failed!", __func__);
        HdfIoServiceRecycle(serv);
        return false;
    }
    int ret = HdfIoServiceGroupAddService(group, serv);
    if (ret != HDF_SUCCESS) {
        HdfIoServiceGroupRecycle(group);
        HdfIoServiceRecycle(serv);
        return false;
    }
    if (HdfIoServiceGroupRegisterListener(group, &g_listener0.listener) == HDF_SUCCESS) {
        ret = HdfIoServiceGroupUnregisterListener(group, &g_listener0.listener);
        if (ret != HDF_SUCCESS) {
            HdfIoServiceGroupRecycle(group);
            HdfIoServiceRecycle(serv);
            return false;
        }
        result = true;
    }
    HdfIoServiceGroupRecycle(group);
    HdfIoServiceRecycle(serv);
    return result;
}
} // namespace OHOS

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    if (size < OHOS::THRESHOLD) {
        return HDF_SUCCESS;
    }

    OHOS::IoserviceGroupListenFuzzTest(data, size);
    return HDF_SUCCESS;
}
