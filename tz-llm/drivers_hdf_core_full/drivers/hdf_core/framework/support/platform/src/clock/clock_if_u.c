/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include "hdf_io_service_if.h"
#include "hdf_log.h"
#include "clock_if.h"

#define HDF_LOG_TAG clock_if_u_c
#define CLOCK_SERVICE_NAME "HDF_PLATFORM_CLOCK_MANAGER"

static void *ClockManagerGetService(void)
{
    static void *manager = NULL;

    if (manager != NULL) {
        return manager;
    }

    manager = (void *)HdfIoServiceBind(CLOCK_SERVICE_NAME);
    if (manager == NULL) {
        HDF_LOGE("ClockManagerGetService: fail to get clock manager!");
    }
    return manager;
}

DevHandle ClockOpen(uint32_t number)
{
    struct HdfIoService *service = NULL;
    struct HdfSBuf *data = NULL;
    struct HdfSBuf *reply = NULL;
    uint32_t handle = 0;
    int ret = -1;

    service = (struct HdfIoService *)ClockManagerGetService();
    if (service == NULL || service->dispatcher == NULL || service->dispatcher->Dispatch == NULL) {
        HDF_LOGE("ClockOpen: service is null!");
        return NULL;
    }

    data = HdfSbufObtainDefaultSize();
    if (data == NULL) {
        HDF_LOGE("ClockOpen: fail to obtain data!");
        return NULL;
    }

    reply = HdfSbufObtainDefaultSize();
    if (reply == NULL) {
        HDF_LOGE("ClockOpen: fail to obtain reply!");
        HdfSbufRecycle(data);
        return NULL;
    }

    if (!HdfSbufWriteUint32(data, number)) {
        HDF_LOGE("ClockOpen: fail to write nodepath!");
        HdfSbufRecycle(data);
        HdfSbufRecycle(reply);
        return NULL;
    }

    ret = service->dispatcher->Dispatch(&service->object, CLOCK_IO_OPEN, data, reply);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("ClockOpen: service call CLOCK_IO_OPEN fail, ret: %d!", ret);
        HdfSbufRecycle(data);
        HdfSbufRecycle(reply);
        return NULL;
    }

    if (!HdfSbufReadUint32(reply, &handle)) {
        HDF_LOGE("ClockOpen: read handle fail!");
        ret = HDF_ERR_IO;
    }

    HdfSbufRecycle(data);
    HdfSbufRecycle(reply);
    return (DevHandle)(uintptr_t)handle;
}

int32_t ClockClose(DevHandle handle)
{
    int32_t ret;
    struct HdfIoService *service = NULL;
    struct HdfSBuf *data = NULL;

    if (handle == NULL) {
        HDF_LOGE("ClockClose: handle is invalid!");
        return HDF_FAILURE;
    }

    service = (struct HdfIoService *)ClockManagerGetService();
    if (service == NULL || service->dispatcher == NULL || service->dispatcher->Dispatch == NULL) {
        HDF_LOGE("ClockClose: service is invalid!");
        return HDF_ERR_INVALID_PARAM;
    }

    data = HdfSbufObtainDefaultSize();
    if (data == NULL) {
        HDF_LOGE("ClockClose: fail to obtain data!");
        return HDF_ERR_MALLOC_FAIL;
    }

    if (!HdfSbufWriteUint32(data, (uint32_t)(uintptr_t)handle)) {
        HDF_LOGE("ClockClose: write handle fail!");
        HdfSbufRecycle(data);
        return HDF_FAILURE;
    }

    ret = service->dispatcher->Dispatch(&service->object, CLOCK_IO_CLOSE, data, NULL);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("ClockClose: service call CLOCK_IO_CLOSE fail, ret: %d", ret);
    }
    HdfSbufRecycle(data);
    return ret;
}

int32_t ClockEnable(DevHandle handle)
{
    int32_t ret;
    struct HdfIoService *service = NULL;
    struct HdfSBuf *data = NULL;

    if (handle == NULL) {
        HDF_LOGE("ClockEnable: handle is invalid!");
        return HDF_FAILURE;
    }

    service = (struct HdfIoService *)ClockManagerGetService();
    if (service == NULL || service->dispatcher == NULL || service->dispatcher->Dispatch == NULL) {
        HDF_LOGE("ClockEnable: service is invalid!");
        return HDF_ERR_INVALID_PARAM;
    }

    data = HdfSbufObtainDefaultSize();
    if (data == NULL) {
        HDF_LOGE("ClockEnable: fail to obtain data!");
        return HDF_ERR_MALLOC_FAIL;
    }

    if (!HdfSbufWriteUint32(data, (uint32_t)(uintptr_t)handle)) {
        HDF_LOGE("ClockEnable: write handle fail!");
        HdfSbufRecycle(data);
        return HDF_FAILURE;
    }

    ret = service->dispatcher->Dispatch(&service->object, CLOCK_IO_ENABLE, data, NULL);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("ClockEnable: service call CLOCK_IO_ENABLE fail, ret: %d", ret);
    }
    HdfSbufRecycle(data);
    return ret;
}

int32_t ClockDisable(DevHandle handle)
{
    int32_t ret;
    struct HdfIoService *service = NULL;
    struct HdfSBuf *data = NULL;

    if (handle == NULL) {
        HDF_LOGE("ClockDisable: handle is invalid!");
        return HDF_FAILURE;
    }

    service = (struct HdfIoService *)ClockManagerGetService();
    if (service == NULL || service->dispatcher == NULL || service->dispatcher->Dispatch == NULL) {
        HDF_LOGE("ClockDisable: service is invalid!");
        return HDF_ERR_INVALID_PARAM;
    }

    data = HdfSbufObtainDefaultSize();
    if (data == NULL) {
        HDF_LOGE("ClockDisable: fail to obtain data!");
        return HDF_ERR_MALLOC_FAIL;
    }

    if (!HdfSbufWriteUint32(data, (uint32_t)(uintptr_t)handle)) {
        HDF_LOGE("ClockDisable: write handle fail!");
        HdfSbufRecycle(data);
        return HDF_ERR_IO;
    }

    ret = service->dispatcher->Dispatch(&service->object, CLOCK_IO_DISABLE, data, NULL);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("ClockDisable: service call CLOCK_IO_DISABLE fail, ret: %d", ret);
    }
    HdfSbufRecycle(data);
    return ret;
}

int32_t ClockSetRate(DevHandle handle, uint32_t value)
{
    int32_t ret;
    struct HdfIoService *service = NULL;
    struct HdfSBuf *data = NULL;

    if (handle == NULL) {
        HDF_LOGE("ClockSetRate: handle is invalid!");
        return HDF_FAILURE;
    }

    service = (struct HdfIoService *)ClockManagerGetService();
    if (service == NULL || service->dispatcher == NULL || service->dispatcher->Dispatch == NULL) {
        HDF_LOGE("ClockSetRate: service is invalid!");
        return HDF_ERR_INVALID_PARAM;
    }

    data = HdfSbufObtainDefaultSize();
    if (data == NULL) {
        HDF_LOGE("ClockSetRate: fail to obtain data!");
        return HDF_ERR_MALLOC_FAIL;
    }

    if (!HdfSbufWriteUint32(data, (uint32_t)(uintptr_t)handle)) {
        HDF_LOGE("ClockSetRate: write handle fail!");
        HdfSbufRecycle(data);
        return HDF_ERR_IO;
    }

    if (!HdfSbufWriteUint32(data, value)) {
        HDF_LOGE("ClockSetRate: write channel fail!");
        HdfSbufRecycle(data);
        return HDF_ERR_IO;
    }

    ret = service->dispatcher->Dispatch(&service->object, CLOCK_IO_SET_RATE, data, NULL);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("ClockSetRate: service call CLOCK_IO_SET_RATE fail, ret: %d", ret);
    }
    HdfSbufRecycle(data);
    return ret;
}

int32_t ClockGetRate(DevHandle handle, uint32_t *rate)
{
    int32_t ret;
    struct HdfSBuf *data = NULL;
    struct HdfSBuf *reply = NULL;
    struct HdfIoService *service = NULL;

    if (handle == NULL) {
        HDF_LOGE("ClockGetRate: handle is invalid!");
        return HDF_FAILURE;
    }

    service = (struct HdfIoService *)ClockManagerGetService();
    if (service == NULL || service->dispatcher == NULL || service->dispatcher->Dispatch == NULL) {
        HDF_LOGE("ClockGetRate: service is invalid!");
        return HDF_ERR_INVALID_PARAM;
    }

    data = HdfSbufObtainDefaultSize();
    if (data == NULL) {
        HDF_LOGE("ClockGetRate: fail to obtain data!");
        return HDF_ERR_MALLOC_FAIL;
    }

    reply = HdfSbufObtainDefaultSize();
    if (reply == NULL) {
        HdfSbufRecycle(data);
        HDF_LOGE("ClockGetRate: fail to obtain reply!");
        return HDF_ERR_MALLOC_FAIL;
    }

    if (!HdfSbufWriteUint32(data, (uint32_t)(uintptr_t)handle)) {
        HDF_LOGE("ClockGetRate: write handle fail!");
        ret = HDF_ERR_IO;
        HdfSbufRecycle(data);
        HdfSbufRecycle(reply);
        return ret;
    }

    ret = service->dispatcher->Dispatch(&service->object, CLOCK_IO_GET_RATE, data, reply);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("ClockGetRate: service call CLOCK_IO_GET_RATE fail, ret: %d!", ret);
        HdfSbufRecycle(data);
        HdfSbufRecycle(reply);
        return ret;
    }

    if (!HdfSbufReadUint32(reply, rate)) {
        HDF_LOGE("ClockGetRate: read val fail!");
        ret = HDF_ERR_IO;
    }

    HdfSbufRecycle(data);
    HdfSbufRecycle(reply);
    return ret;
}

DevHandle ClockGetParent(DevHandle handle)
{
    int32_t ret;
    struct HdfSBuf *data = NULL;
    struct HdfSBuf *reply = NULL;
    struct HdfIoService *service = NULL;
    uint32_t parent = 0;

    if (handle == NULL) {
        HDF_LOGE("ClockGetParent: handle is invalid!");
        return NULL;
    }

    service = (struct HdfIoService *)ClockManagerGetService();
    if (service == NULL || service->dispatcher == NULL || service->dispatcher->Dispatch == NULL) {
        HDF_LOGE("ClockGetParent: service is invalid!");
        return NULL;
    }

    data = HdfSbufObtainDefaultSize();
    if (data == NULL) {
        HDF_LOGE("ClockGetParent: fail to obtain data!");
        return NULL;
    }

    reply = HdfSbufObtainDefaultSize();
    if (reply == NULL) {
        HdfSbufRecycle(data);
        HDF_LOGE("ClockGetParent: fail to obtain reply!");
        return NULL;
    }

    if (!HdfSbufWriteUint32(data, (uint32_t)(uintptr_t)handle)) {
        HDF_LOGE("ClockGetParent: write handle fail!");
        ret = HDF_ERR_IO;
        HdfSbufRecycle(data);
        HdfSbufRecycle(reply);
        return NULL;
    }

    ret = service->dispatcher->Dispatch(&service->object, CLOCK_IO_GET_PARENT, data, reply);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("ClockGetParent: service call CLOCK_IO_GET_PARENT fail, ret: %d!", ret);
        HdfSbufRecycle(data);
        HdfSbufRecycle(reply);
        return NULL;
    }

    if (!HdfSbufReadUint32(reply, &parent)) {
        HDF_LOGE("ClockGetParent: read val fail!");
        ret = HDF_ERR_IO;
    }

    HdfSbufRecycle(data);
    HdfSbufRecycle(reply);
    return (DevHandle)(uintptr_t)parent;
}

int32_t ClockSetParent(DevHandle handle, DevHandle parent)
{
    int32_t ret;
    struct HdfSBuf *data = NULL;
    struct HdfSBuf *reply = NULL;
    struct HdfIoService *service = NULL;

    if (handle == NULL || parent == NULL) {
        HDF_LOGE("ClockSetParent: handle is invalid!");
        return HDF_FAILURE;
    }

    service = (struct HdfIoService *)ClockManagerGetService();
    if (service == NULL || service->dispatcher == NULL || service->dispatcher->Dispatch == NULL) {
        HDF_LOGE("ClockSetParent: service is invalid!");
        return HDF_ERR_INVALID_PARAM;
    }

    data = HdfSbufObtainDefaultSize();
    if (data == NULL) {
        HDF_LOGE("ClockSetParent: fail to obtain data!");
        return HDF_ERR_MALLOC_FAIL;
    }

    reply = HdfSbufObtainDefaultSize();
    if (reply == NULL) {
        HdfSbufRecycle(data);
        HDF_LOGE("ClockSetParent: fail to obtain reply!");
        return HDF_ERR_MALLOC_FAIL;
    }

    if (!HdfSbufWriteUint32(data, (uint32_t)(uintptr_t)handle)) {
        HDF_LOGE("ClockSetParent: write handle fail!");
        ret = HDF_ERR_IO;
        HdfSbufRecycle(data);
        HdfSbufRecycle(reply);
        return ret;
    }

    if (!HdfSbufWriteUint32(data, (uint32_t)(uintptr_t)parent)) {
        HDF_LOGE("ClockSetParent: write handle fail!");
        ret = HDF_ERR_IO;
        HdfSbufRecycle(data);
        HdfSbufRecycle(reply);
        return ret;
    }

    ret = service->dispatcher->Dispatch(&service->object, CLOCK_IO_SET_PARENT, data, NULL);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("ClockSetParent: service call CLOCK_IO_SET_PARENT fail, ret: %d!", ret);
    }

    HdfSbufRecycle(data);
    HdfSbufRecycle(reply);
    return ret;
}