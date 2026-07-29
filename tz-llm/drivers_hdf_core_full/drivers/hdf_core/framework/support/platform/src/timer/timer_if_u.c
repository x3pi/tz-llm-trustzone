/*
 * Copyright (c) 2022-2023 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include "timer_if.h"
#include "hdf_io_service_if.h"
#include "hdf_log.h"
#include "osal_mem.h"
#include "platform_listener_u.h"
#include "securec.h"

#define HDF_LOG_TAG timer_if_u

#define TIMER_SERVICE_NAME "HDF_PLATFORM_TIMER_MANAGER"
static struct HdfIoService *TimerManagerGetService(void)
{
    static struct HdfIoService *service = NULL;

    if (service != NULL) {
        return service;
    }
    service = HdfIoServiceBind(TIMER_SERVICE_NAME);
    if (service == NULL) {
        HDF_LOGE("TimerManagerGetService: fail to get timer manager!");
        return NULL;
    }

    if (service->priv == NULL) {
        struct PlatformUserListenerManager *manager = PlatformUserListenerManagerGet(PLATFORM_MODULE_TIMER);
        if (manager == NULL) {
            HDF_LOGE("TimerManagerGetService: PlatformUserListenerManagerGet fail!");
            return service;
        }
        service->priv = manager;
        manager->service = service;
    }
    return service;
}

DevHandle HwTimerOpen(const uint32_t number)
{
    int32_t ret;
    struct HdfIoService *service = NULL;
    struct HdfSBuf *data = NULL;
    struct HdfSBuf *reply = NULL;
    uint32_t handle;

    service = TimerManagerGetService();
    if (service == NULL || service->dispatcher == NULL || service->dispatcher->Dispatch == NULL) {
        HDF_LOGE("HwTimerOpen: service is invalid!");
        return NULL;
    }

    data = HdfSbufObtainDefaultSize();
    if (data == NULL) {
        HDF_LOGE("HwTimerOpen: fail to obtain data!");
        return NULL;
    }
    reply = HdfSbufObtainDefaultSize();
    if (reply == NULL) {
        HDF_LOGE("HwTimerOpen: fail to obtain reply!");
        HdfSbufRecycle(data);
        return NULL;
    }

    if (!HdfSbufWriteUint16(data, (uint16_t)number)) {
        HDF_LOGE("HwTimerOpen: write number fail!");
        HdfSbufRecycle(data);
        HdfSbufRecycle(reply);
        return NULL;
    }

    ret = service->dispatcher->Dispatch(&service->object, TIMER_IO_OPEN, data, reply);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("HwTimerOpen: TIMER_IO_OPEN service process fail, ret: %d!", ret);
        HdfSbufRecycle(data);
        HdfSbufRecycle(reply);
        return NULL;
    }

    if (!HdfSbufReadUint32(reply, &handle)) {
        HDF_LOGE("HwTimerOpen: read handle fail!");
        HdfSbufRecycle(data);
        HdfSbufRecycle(reply);
        return NULL;
    }
    HdfSbufRecycle(data);
    HdfSbufRecycle(reply);
    return (DevHandle)(uintptr_t)handle;
}

void HwTimerClose(DevHandle handle)
{
    int32_t ret;
    struct HdfIoService *service = NULL;
    struct HdfSBuf *data = NULL;

    if (handle == NULL) {
        HDF_LOGE("HwTimerClose: handle is invalid!");
        return;
    }

    service = TimerManagerGetService();
    if (service == NULL || service->dispatcher == NULL || service->dispatcher->Dispatch == NULL) {
        HDF_LOGE("HwTimerClose: service is invalid!");
        return;
    }

    data = HdfSbufObtainDefaultSize();
    if (data == NULL) {
        HDF_LOGE("HwTimerClose: fail to obtain data!");
        return;
    }

    if (!HdfSbufWriteUint32(data, (uint32_t)(uintptr_t)handle)) {
        HDF_LOGE("HwTimerClose: write handle fail!");
        HdfSbufRecycle(data);
        return;
    }

    ret = service->dispatcher->Dispatch(&service->object, TIMER_IO_CLOSE, data, NULL);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("HwTimerClose: TIMER_IO_CLOSE service process fail, ret: %d!", ret);
    }

    HdfSbufRecycle(data);
    PlatformUserListenerDestory(service->priv, (uint32_t)(uintptr_t)handle);
}

int32_t HwTimerStart(DevHandle handle)
{
    int32_t ret;
    struct HdfIoService *service = NULL;
    struct HdfSBuf *data = NULL;

    if (handle == NULL) {
        HDF_LOGE("HwTimerStart: handle is invalid!");
        return HDF_FAILURE;
    }
    service = (struct HdfIoService *)TimerManagerGetService();
    if (service == NULL || service->dispatcher == NULL || service->dispatcher->Dispatch == NULL) {
        HDF_LOGE("HwTimerStart: service is invalid!");
        return HDF_ERR_INVALID_PARAM;
    }

    data = HdfSbufObtainDefaultSize();
    if (data == NULL) {
        HDF_LOGE("HwTimerStart: fail to obtain data!");
        return HDF_FAILURE;
    }

    if (!HdfSbufWriteUint32(data, (uint32_t)(uintptr_t)handle)) {
        HDF_LOGE("HwTimerStart: write handle fail!");
        HdfSbufRecycle(data);
        return HDF_FAILURE;
    }

    ret = service->dispatcher->Dispatch(&service->object, TIMER_IO_START, data, NULL);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("HwTimerStart: TIMER_IO_START service process fail, ret: %d!", ret);
        HdfSbufRecycle(data);
        return HDF_FAILURE;
    }
    HdfSbufRecycle(data);

    return HDF_SUCCESS;
}

int32_t HwTimerStop(DevHandle handle)
{
    int32_t ret;
    struct HdfIoService *service = NULL;
    struct HdfSBuf *data = NULL;

    if (handle == NULL) {
        HDF_LOGE("HwTimerStop: handle is invalid!");
        return HDF_FAILURE;
    }

    service = (struct HdfIoService *)TimerManagerGetService();
    if (service == NULL || service->dispatcher == NULL || service->dispatcher->Dispatch == NULL) {
        HDF_LOGE("HwTimerStop: service is invalid!");
        return HDF_ERR_INVALID_PARAM;
    }

    data = HdfSbufObtainDefaultSize();
    if (data == NULL) {
        HDF_LOGE("HwTimerStop: fail to obtain data!");
        return HDF_FAILURE;
    }

    if (!HdfSbufWriteUint32(data, (uint32_t)(uintptr_t)handle)) {
        HDF_LOGE("HwTimerStop: write handle fail!");
        HdfSbufRecycle(data);
        return HDF_FAILURE;
    }

    ret = service->dispatcher->Dispatch(&service->object, TIMER_IO_STOP, data, NULL);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("HwTimerStop: TIMER_IO_STOP service process fail, ret: %d!", ret);
        HdfSbufRecycle(data);
        return HDF_FAILURE;
    }
    HdfSbufRecycle(data);

    return HDF_SUCCESS;
}

static int32_t TimerListenerReg(struct HdfIoService *service, DevHandle handle, TimerHandleCb cb, bool isOnce)
{
    struct PlatformUserListenerTimerParam *param = NULL;

    param = OsalMemCalloc(sizeof(struct PlatformUserListenerTimerParam));
    if (param == NULL) {
        HDF_LOGE("TimerListenerReg: OsalMemCalloc param fail!");
        return HDF_ERR_IO;
    }
    param->handle = (uint32_t)(uintptr_t)handle;
    param->func = cb;
    param->isOnce = isOnce;
    param->manager = (struct PlatformUserListenerManager *)service->priv;

    if (PlatformUserListenerReg((struct PlatformUserListenerManager *)service->priv, param->handle, (void *)param,
        TimerOnDevEventReceive) != HDF_SUCCESS) {
        HDF_LOGE("TimerListenerReg: PlatformUserListenerReg fail!");
        OsalMemFree(param);
        return HDF_ERR_IO;
    }
    HDF_LOGD("TimerListenerReg: get timer listener for %d success!", param->handle);
    return HDF_SUCCESS;
}

int32_t HwTimerSet(DevHandle handle, uint32_t useconds, TimerHandleCb cb)
{
    int32_t ret;
    struct HdfIoService *service = NULL;
    struct HdfSBuf *buf = NULL;
    struct TimerConfig cfg;

    if (handle == NULL || cb == NULL) {
        HDF_LOGE("HwTimerSet: param is invalid!");
        return HDF_ERR_INVALID_PARAM;
    }

    service = (struct HdfIoService *)TimerManagerGetService();
    if (service == NULL || service->dispatcher == NULL || service->dispatcher->Dispatch == NULL) {
        HDF_LOGE("HwTimerSet: param is invalid!");
        return HDF_ERR_INVALID_PARAM;
    }

    if (memset_s(&cfg, sizeof(cfg), 0, sizeof(cfg)) != EOK) {
        HDF_LOGE("HwTimerSet: memset_s fail!");
        return HDF_ERR_IO;
    }
    cfg.useconds = useconds;

    ret = TimerListenerReg(service, handle, cb, false);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("HwTimerSet: TimerListenerReg fail, ret: %d!", ret);
        return HDF_FAILURE;
    }

    buf = HdfSbufObtainDefaultSize();
    if (buf == NULL) {
        HDF_LOGE("HwTimerSet: fail to obtain buf!");
        PlatformUserListenerDestory(service->priv, (uint32_t)(uintptr_t)handle);
        return HDF_ERR_MALLOC_FAIL;
    }
    if (!HdfSbufWriteUint32(buf, (uint32_t)(uintptr_t)handle)) {
        HDF_LOGE("HwTimerSet: sbuf write handle fail!");
        PlatformUserListenerDestory(service->priv, (uint32_t)(uintptr_t)handle);
        HdfSbufRecycle(buf);
        return HDF_ERR_IO;
    }
    if (!HdfSbufWriteBuffer(buf, &cfg, sizeof(cfg))) {
        HDF_LOGE("HwTimerSet: sbuf write cfg fail!");
        PlatformUserListenerDestory(service->priv, (uint32_t)(uintptr_t)handle);
        HdfSbufRecycle(buf);
        return HDF_ERR_IO;
    }

    ret = service->dispatcher->Dispatch(&service->object, TIMER_IO_SET, buf, NULL);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("HwTimerSet: TIMER_IO_SET service process fail, ret: %d!", ret);
        HdfSbufRecycle(buf);
        PlatformUserListenerDestory(service->priv, (uint32_t)(uintptr_t)handle);
        return HDF_FAILURE;
    }
    HdfSbufRecycle(buf);

    return HDF_SUCCESS;
}

int32_t HwTimerSetOnce(DevHandle handle, uint32_t useconds, TimerHandleCb cb)
{
    int32_t ret;
    struct HdfIoService *service = NULL;
    struct HdfSBuf *buf = NULL;
    struct TimerConfig cfg;

    if (handle == NULL) {
        HDF_LOGE("HwTimerSetOnce: handle is invalid!");
        return HDF_FAILURE;
    }

    service = TimerManagerGetService();
    if (service == NULL || service->dispatcher == NULL || service->dispatcher->Dispatch == NULL || cb == NULL) {
        HDF_LOGE("HwTimerSetOnce: service is invalid!");
        return HDF_ERR_INVALID_PARAM;
    }

    if (memset_s(&cfg, sizeof(cfg), 0, sizeof(cfg)) != EOK) {
        HDF_LOGE("HwTimerSetOnce: memset_s fail!");
        return HDF_ERR_IO;
    }
    cfg.useconds = useconds;

    ret = TimerListenerReg(service, handle, cb, true);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("HwTimerSetOnce: TimerListenerReg fail, ret: %d!", ret);
        return HDF_FAILURE;
    }

    buf = HdfSbufObtainDefaultSize();
    if (buf == NULL) {
        HDF_LOGE("HwTimerSetOnce: fail to obtain buf!");
        PlatformUserListenerDestory(service->priv, (uint32_t)(uintptr_t)handle);
        return HDF_ERR_MALLOC_FAIL;
    }
    if (!HdfSbufWriteUint32(buf, (uint32_t)(uintptr_t)handle)) {
        HDF_LOGE("HwTimerSetOnce: sbuf write handle fail!");
        PlatformUserListenerDestory(service->priv, (uint32_t)(uintptr_t)handle);
        HdfSbufRecycle(buf);
        return HDF_ERR_IO;
    }
    if (!HdfSbufWriteBuffer(buf, &cfg, sizeof(cfg))) {
        HDF_LOGE("HwTimerSetOnce: sbuf write cfg fail!");
        PlatformUserListenerDestory(service->priv, (uint32_t)(uintptr_t)handle);
        HdfSbufRecycle(buf);
        return HDF_ERR_IO;
    }

    ret = service->dispatcher->Dispatch(&service->object, TIMER_IO_SETONCE, buf, NULL);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("HwTimerSetOnce: TIMER_IO_SETONCE service process fail, ret: %d!", ret);
        HdfSbufRecycle(buf);
        PlatformUserListenerDestory(service->priv, (uint32_t)(uintptr_t)handle);
        return HDF_FAILURE;
    }
    HdfSbufRecycle(buf);

    return HDF_SUCCESS;
}

int32_t HwTimerGet(DevHandle handle, uint32_t *useconds, bool *isPeriod)
{
    int32_t ret = HDF_SUCCESS;
    struct HdfIoService *service = NULL;
    struct HdfSBuf *data = NULL;
    const void *rBuf = NULL;
    struct HdfSBuf *reply = NULL;
    struct TimerConfig cfg;
    uint32_t rLen;

    if ((handle == NULL) || (useconds == NULL) || (isPeriod == NULL)) {
        HDF_LOGE("HwTimerGet: param is invalid!");
        return HDF_FAILURE;
    }

    service = (struct HdfIoService *)TimerManagerGetService();
    if (service == NULL || service->dispatcher == NULL || service->dispatcher->Dispatch == NULL) {
        HDF_LOGE("HwTimerGet: service is invalid!");
        return HDF_ERR_INVALID_PARAM;
    }

    data = HdfSbufObtainDefaultSize();
    if (data == NULL) {
        HDF_LOGE("HwTimerGet: fail to obtain data!");
        return HDF_FAILURE;
    }

    reply = HdfSbufObtainDefaultSize();
    if (reply == NULL) {
        HDF_LOGE("HwTimerGet: fail to obtain reply!");
        ret = HDF_ERR_MALLOC_FAIL;
        goto __EXIT;
    }

    if (!HdfSbufWriteUint32(data, (uint32_t)(uintptr_t)handle)) {
        HDF_LOGE("HwTimerGet: write handle fail!");
        ret = HDF_FAILURE;
        goto __EXIT;
    }

    ret = service->dispatcher->Dispatch(&service->object, TIMER_IO_GET, data, reply);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("HwTimerGet: TIMER_IO_GET service process fail, ret: %d!", ret);
        ret = HDF_FAILURE;
        goto __EXIT;
    }

    if (!HdfSbufReadBuffer(reply, &rBuf, &rLen)) {
        HDF_LOGE("HwTimerGet: sbuf read buffer fail!");
        ret = HDF_ERR_IO;
        goto __EXIT;
    }
    if (rLen != sizeof(struct TimerConfig)) {
        HDF_LOGE("HwTimerGet: sbuf read buffer len error %u != %zu", rLen, sizeof(struct TimerConfig));
        ret = HDF_FAILURE;
        goto __EXIT;
    }
    if (memcpy_s(&cfg, sizeof(struct TimerConfig), rBuf, rLen) != EOK) {
        HDF_LOGE("HwTimerGet: memcpy rBuf fail!");
        ret = HDF_ERR_IO;
        goto __EXIT;
    }

    *useconds = cfg.useconds;
    *isPeriod = cfg.isPeriod;

__EXIT:
    HdfSbufRecycle(data);
    HdfSbufRecycle(reply);
    return ret;
}
