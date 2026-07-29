/*
 * Copyright (c) 2022-2023 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include "rtc_if.h"
#include "hdf_io_service_if.h"
#include "hdf_log.h"
#include "hdf_sbuf.h"
#include "osal_mem.h"
#include "platform_listener_u.h"
#include "rtc_base.h"
#include "securec.h"

#define HDF_LOG_TAG rtc_if_u_c

DevHandle RtcOpen(void)
{
    static struct HdfIoService *service = NULL;
    if (service != NULL) {
        return service;
    }

    service = HdfIoServiceBind("HDF_PLATFORM_RTC");
    if (service == NULL) {
        HDF_LOGE("RtcOpen: rtc service bind fail!");
        return NULL;
    }

    if (service->priv == NULL) {
        struct PlatformUserListenerManager *manager = PlatformUserListenerManagerGet(PLATFORM_MODULE_RTC);
        if (manager == NULL) {
            HDF_LOGE("RtcOpen: PlatformUserListenerManagerGet fail!");
            return (DevHandle)service;
        }
        service->priv = manager;
        manager->service = service;
    }

    return (DevHandle)service;
}

void RtcClose(DevHandle handle)
{
    struct HdfIoService *service = NULL;
    if (handle == NULL) {
        HDF_LOGE("RtcClose: handle is null!");
        return;
    }

    service = (struct HdfIoService *)handle;
    PlatformUserListenerDestory((struct PlatformUserListenerManager *)service->priv, 0);
}

int32_t RtcReadTime(DevHandle handle, struct RtcTime *time)
{
    int32_t ret;
    uint32_t len = 0;
    struct HdfSBuf *reply = NULL;
    struct HdfIoService *service = NULL;
    struct RtcTime *temp = NULL;

    if (handle == NULL || time == NULL) {
        HDF_LOGE("RtcReadTime: handle or time is null!");
        return HDF_ERR_INVALID_OBJECT;
    }

    reply = HdfSbufObtainDefaultSize();
    if (reply == NULL) {
        HDF_LOGE("RtcReadTime: fail to obtain reply!");
        return HDF_ERR_MALLOC_FAIL;
    }

    service = (struct HdfIoService *)handle;
    if (service->dispatcher == NULL || service->dispatcher->Dispatch == NULL) {
        HDF_LOGE("RtcReadTime: service is invalid!");
        ret = HDF_ERR_MALLOC_FAIL;
        goto EXIT;
    }

    ret = service->dispatcher->Dispatch(&service->object, RTC_IO_READTIME, NULL, reply);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("RtcReadTime: fail, ret is %d!", ret);
        goto EXIT;
    }

    if (!HdfSbufReadBuffer(reply, (const void **)&temp, &len) || temp == NULL) {
        HDF_LOGE("RtcReadTime: read buffer fail!");
        ret = HDF_ERR_IO;
        goto EXIT;
    }

    if (len != sizeof(*time)) {
        HDF_LOGE("RtcReadTime: read error, len: %u, size: %zu!", len, sizeof(*time));
        ret = HDF_FAILURE;
        goto EXIT;
    }

    if (memcpy_s(time, sizeof(*time), temp, len) != EOK) {
        HDF_LOGE("RtcReadTime: memcpy time fail!");
        ret = HDF_ERR_IO;
        goto EXIT;
    }

EXIT:
    HdfSbufRecycle(reply);
    return ret;
}

int32_t RtcWriteTime(DevHandle handle, const struct RtcTime *time)
{
    int32_t ret;
    struct HdfSBuf *data = NULL;
    struct HdfIoService *service = NULL;

    if (handle == NULL || time == NULL) {
        HDF_LOGE("RtcWriteTime: handle or time is null!");
        return HDF_ERR_INVALID_OBJECT;
    }

    if (RtcIsInvalid(time) == RTC_TRUE) {
        HDF_LOGE("RtcWriteTime: time is invalid!");
        return HDF_ERR_INVALID_PARAM;
    }

    data = HdfSbufObtainDefaultSize();
    if (data == NULL) {
        HDF_LOGE("RtcWriteTime: fail to obtain data!");
        return HDF_ERR_MALLOC_FAIL;
    }

    if (!HdfSbufWriteBuffer(data, time, sizeof(*time))) {
        HDF_LOGE("RtcWriteTime: write rtc time fail!");
        HdfSbufRecycle(data);
        return HDF_ERR_IO;
    }

    service = (struct HdfIoService *)handle;
    if (service->dispatcher == NULL || service->dispatcher->Dispatch == NULL) {
        HDF_LOGE("RtcWriteTime: service is invalid!");
        HdfSbufRecycle(data);
        return HDF_ERR_MALLOC_FAIL;
    }

    ret = service->dispatcher->Dispatch(&service->object, RTC_IO_WRITETIME, data, NULL);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("RtcWriteTime: fail, ret is %d!", ret);
        HdfSbufRecycle(data);
        return ret;
    }

    HdfSbufRecycle(data);
    return HDF_SUCCESS;
}

int32_t RtcReadAlarm(DevHandle handle, enum RtcAlarmIndex alarmIndex, struct RtcTime *time)
{
    int32_t ret;
    uint32_t len = 0;
    struct HdfSBuf *data = NULL;
    struct HdfSBuf *reply = NULL;
    struct RtcTime *temp = NULL;
    struct HdfIoService *service = NULL;

    if (handle == NULL || time == NULL) {
        HDF_LOGE("RtcReadAlarm: handle or time is null!");
        return HDF_ERR_INVALID_OBJECT;
    }

    data = HdfSbufObtainDefaultSize();
    if (data == NULL) {
        HDF_LOGE("RtcReadAlarm: fail to obtain data!");
        return HDF_ERR_MALLOC_FAIL;
    }

    reply = HdfSbufObtainDefaultSize();
    if (reply == NULL) {
        HDF_LOGE("RtcReadAlarm: fail to obtain reply!");
        HdfSbufRecycle(data);
        return HDF_ERR_MALLOC_FAIL;
    }

    if (!HdfSbufWriteUint32(data, (uint32_t)alarmIndex)) {
        HDF_LOGE("RtcReadAlarm: write alarmIndex fail!");
        ret = HDF_ERR_IO;
        goto EXIT;
    }

    service = (struct HdfIoService *)handle;
    if (service->dispatcher == NULL || service->dispatcher->Dispatch == NULL) {
        HDF_LOGE("RtcReadAlarm: service is invalid!");
        ret = HDF_ERR_MALLOC_FAIL;
        goto EXIT;
    }

    ret = service->dispatcher->Dispatch(&service->object, RTC_IO_READALARM, data, reply);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("RtcReadAlarm: fail, ret is %d!", ret);
        goto EXIT;
    }

    if (!HdfSbufReadBuffer(reply, (const void **)&temp, &len) || temp == NULL) {
        HDF_LOGE("RtcReadAlarm: read buffer fail!");
        ret = HDF_ERR_IO;
        goto EXIT;
    }

    if (len != sizeof(*time)) {
        HDF_LOGE("RtcReadAlarm: read error, len: %u, size: %zu!", len, sizeof(*time));
        ret = HDF_FAILURE;
        goto EXIT;
    }

    if (memcpy_s(time, sizeof(*time), temp, len) != EOK) {
        HDF_LOGE("RtcReadAlarm: memcpy time fail!");
        ret = HDF_ERR_IO;
        goto EXIT;
    }

EXIT:
    HdfSbufRecycle(data);
    HdfSbufRecycle(reply);
    return ret;
}

int32_t RtcWriteAlarm(DevHandle handle, enum RtcAlarmIndex alarmIndex, const struct RtcTime *time)
{
    int32_t ret;
    struct HdfSBuf *data = NULL;
    struct HdfIoService *service = NULL;

    if (handle == NULL || time == NULL) {
        HDF_LOGE("RtcWriteAlarm: handle or time is null!");
        return HDF_ERR_INVALID_OBJECT;
    }

    if (RtcIsInvalid(time) == RTC_TRUE) {
        HDF_LOGE("RtcWriteAlarm: time is invalid!");
        return HDF_ERR_INVALID_PARAM;
    }

    data = HdfSbufObtainDefaultSize();
    if (data == NULL) {
        HDF_LOGE("RtcWriteAlarm: fail to obtain data!");
        return HDF_ERR_MALLOC_FAIL;
    }

    if (!HdfSbufWriteUint32(data, (uint32_t)alarmIndex)) {
        HDF_LOGE("RtcWriteAlarm: write alarmIndex fail!");
        HdfSbufRecycle(data);
        return HDF_ERR_IO;
    }

    if (!HdfSbufWriteBuffer(data, time, sizeof(*time))) {
        HDF_LOGE("RtcWriteAlarm: write rtc time fail!");
        HdfSbufRecycle(data);
        return HDF_ERR_IO;
    }

    service = (struct HdfIoService *)handle;
    if (service->dispatcher == NULL || service->dispatcher->Dispatch == NULL) {
        HDF_LOGE("RtcWriteAlarm: service is invalid!");
        HdfSbufRecycle(data);
        return HDF_ERR_MALLOC_FAIL;
    }

    ret = service->dispatcher->Dispatch(&service->object, RTC_IO_WRITEALARM, data, NULL);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("RtcWriteAlarm: fail, ret is %d!", ret);
        HdfSbufRecycle(data);
        return ret;
    }

    HdfSbufRecycle(data);
    return HDF_SUCCESS;
}

static int32_t RtcRegListener(struct HdfIoService *service, enum RtcAlarmIndex alarmIndex, RtcAlarmCallback cb)
{
    struct PlatformUserListenerRtcParam *param = NULL;

    param = OsalMemCalloc(sizeof(struct PlatformUserListenerRtcParam));
    if (param == NULL) {
        HDF_LOGE("RtcRegListener: OsalMemCalloc param fail!");
        return HDF_ERR_IO;
    }
    param->index = alarmIndex;
    param->func = cb;

    if (PlatformUserListenerReg((struct PlatformUserListenerManager *)service->priv, 0, (void *)param,
        RtcOnDevEventReceive) != HDF_SUCCESS) {
        HDF_LOGE("RtcRegListener: PlatformUserListenerReg fail!");
        OsalMemFree(param);
        return HDF_ERR_IO;
    }
    HDF_LOGD("RtcRegListener: get rtc listener for %d success!", alarmIndex);
    return HDF_SUCCESS;
}

int32_t RtcRegisterAlarmCallback(DevHandle handle, enum RtcAlarmIndex alarmIndex, RtcAlarmCallback cb)
{
    int ret;
    struct HdfSBuf *data = NULL;
    struct HdfIoService *service = (struct HdfIoService *)handle;

    if (handle == NULL || cb == NULL) {
        HDF_LOGE("RtcRegisterAlarmCallback: handle or cb is null!");
        return HDF_ERR_INVALID_OBJECT;
    }

    data = HdfSbufObtainDefaultSize();
    if (data == NULL) {
        HDF_LOGE("RtcRegisterAlarmCallback: fail to obtain data!");
        return HDF_ERR_MALLOC_FAIL;
    }

    if (!HdfSbufWriteUint32(data, (uint32_t)alarmIndex)) {
        HDF_LOGE("RtcRegisterAlarmCallback: write alarmIndex fail!");
        HdfSbufRecycle(data);
        return HDF_ERR_IO;
    }

    if (service->dispatcher == NULL || service->dispatcher->Dispatch == NULL) {
        HDF_LOGE("RtcRegisterAlarmCallback: service is invalid");
        HdfSbufRecycle(data);
        return HDF_ERR_MALLOC_FAIL;
    }

    ret = RtcRegListener(service, alarmIndex, cb);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("RtcRegisterAlarmCallback: RtcListenerReg fail, ret is %d!", ret);
        HdfSbufRecycle(data);
        return ret;
    }

    ret = service->dispatcher->Dispatch(&service->object, RTC_IO_REGISTERALARMCALLBACK, data, NULL);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("RtcRegisterAlarmCallback: fail, ret is %d!", ret);
        HdfSbufRecycle(data);
        PlatformUserListenerDestory((struct PlatformUserListenerManager *)service->priv, 0);
        return ret;
    }

    HdfSbufRecycle(data);
    return HDF_SUCCESS;
}

int32_t RtcAlarmInterruptEnable(DevHandle handle, enum RtcAlarmIndex alarmIndex, uint8_t enable)
{
    int32_t ret;
    struct HdfSBuf *data = NULL;
    struct HdfIoService *service = NULL;

    if (handle == NULL) {
        HDF_LOGE("RtcAlarmInterruptEnable: handle is null!");
        return HDF_ERR_INVALID_OBJECT;
    }

    data = HdfSbufObtainDefaultSize();
    if (data == NULL) {
        HDF_LOGE("RtcAlarmInterruptEnable: fail to obtain data!");
        return HDF_ERR_MALLOC_FAIL;
    }

    if (!HdfSbufWriteUint32(data, (uint32_t)alarmIndex)) {
        HDF_LOGE("RtcAlarmInterruptEnable: write alarmIndex fail!");
        HdfSbufRecycle(data);
        return HDF_ERR_IO;
    }

    if (!HdfSbufWriteUint8(data, enable)) {
        HDF_LOGE("RtcAlarmInterruptEnable: write enable fail!");
        HdfSbufRecycle(data);
        return HDF_ERR_IO;
    }

    service = (struct HdfIoService *)handle;
    if (service->dispatcher == NULL || service->dispatcher->Dispatch == NULL) {
        HDF_LOGE("RtcAlarmInterruptEnable: service is invalid!");
        HdfSbufRecycle(data);
        return HDF_ERR_MALLOC_FAIL;
    }

    ret = service->dispatcher->Dispatch(&service->object, RTC_IO_ALARMINTERRUPTENABLE, data, NULL);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("RtcAlarmInterruptEnable: fail, ret is %d!", ret);
        HdfSbufRecycle(data);
        return ret;
    }

    HdfSbufRecycle(data);
    return HDF_SUCCESS;
}

int32_t RtcGetFreq(DevHandle handle, uint32_t *freq)
{
    int32_t ret;
    struct HdfSBuf *reply = NULL;
    struct HdfIoService *service = NULL;

    if (handle == NULL || freq == NULL) {
        HDF_LOGE("RtcGetFreq: handle or freq is null!");
        return HDF_ERR_INVALID_OBJECT;
    }

    reply = HdfSbufObtainDefaultSize();
    if (reply == NULL) {
        HDF_LOGE("RtcGetFreq: fail to obtain data!");
        return HDF_ERR_MALLOC_FAIL;
    }

    service = (struct HdfIoService *)handle;
    if (service->dispatcher == NULL || service->dispatcher->Dispatch == NULL) {
        HDF_LOGE("RtcGetFreq: service is invalid!");
        ret = HDF_ERR_MALLOC_FAIL;
        goto EXIT;
    }

    ret = service->dispatcher->Dispatch(&service->object, RTC_IO_GETFREQ, NULL, reply);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("RtcGetFreq: fail, ret is %d!", ret);
        goto EXIT;
    }

    if (!HdfSbufReadUint32(reply, freq)) {
        HDF_LOGE("RtcGetFreq: read buffer fail!");
        goto EXIT;
    }

EXIT:
    HdfSbufRecycle(reply);
    return ret;
}

int32_t RtcSetFreq(DevHandle handle, uint32_t freq)
{
    int32_t ret;
    struct HdfSBuf *data = NULL;
    struct HdfIoService *service = NULL;

    if (handle == NULL) {
        HDF_LOGE("RtcSetFreq: handle is null!");
        return HDF_ERR_INVALID_OBJECT;
    }

    data = HdfSbufObtainDefaultSize();
    if (data == NULL) {
        HDF_LOGE("RtcSetFreq: fail to obtain data!");
        return HDF_ERR_MALLOC_FAIL;
    }

    if (!HdfSbufWriteUint32(data, freq)) {
        HDF_LOGE("RtcSetFreq: write freq fail!");
        HdfSbufRecycle(data);
        return HDF_ERR_IO;
    }

    service = (struct HdfIoService *)handle;
    if (service->dispatcher == NULL || service->dispatcher->Dispatch == NULL) {
        HDF_LOGE("RtcSetFreq: service is invalid!");
        HdfSbufRecycle(data);
        return HDF_ERR_MALLOC_FAIL;
    }

    ret = service->dispatcher->Dispatch(&service->object, RTC_IO_SETFREQ, data, NULL);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("RtcSetFreq: fail, ret is %d!", ret);
        HdfSbufRecycle(data);
        return ret;
    }

    HdfSbufRecycle(data);
    return HDF_SUCCESS;
}

int32_t RtcReset(DevHandle handle)
{
    int32_t ret;
    struct HdfIoService *service = NULL;

    if (handle == NULL) {
        HDF_LOGE("RtcReset: handle is null!");
        return HDF_ERR_INVALID_OBJECT;
    }

    service = (struct HdfIoService *)handle;
    if (service->dispatcher == NULL || service->dispatcher->Dispatch == NULL) {
        HDF_LOGE("RtcReset: service is invalid!");
        return HDF_ERR_INVALID_PARAM;
    }

    ret = service->dispatcher->Dispatch(&service->object, RTC_IO_RESET, NULL, NULL);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("RtcReset: rtc reset fail, ret %d!", ret);
        return ret;
    }

    return HDF_SUCCESS;
}

int32_t RtcReadReg(DevHandle handle, uint8_t usrDefIndex, uint8_t *value)
{
    int32_t ret;
    struct HdfSBuf *data = NULL;
    struct HdfSBuf *reply = NULL;
    struct HdfIoService *service = NULL;

    if (handle == NULL || value == NULL) {
        HDF_LOGE("RtcReadReg: handle or value is null!");
        return HDF_ERR_INVALID_OBJECT;
    }

    data = HdfSbufObtainDefaultSize();
    if (data == NULL) {
        HDF_LOGE("RtcReadReg: fail to obtain data!");
        return HDF_ERR_MALLOC_FAIL;
    }

    reply = HdfSbufObtainDefaultSize();
    if (reply == NULL) {
        HDF_LOGE("RtcReadReg: fail to obtain reply!");
        HdfSbufRecycle(data);
        return HDF_ERR_MALLOC_FAIL;
    }

    if (!HdfSbufWriteUint8(data, usrDefIndex)) {
        HDF_LOGE("RtcReadReg: write usrDefIndex fail!");
        ret = HDF_ERR_IO;
        goto EXIT;
    }

    service = (struct HdfIoService *)handle;
    if (service->dispatcher == NULL || service->dispatcher->Dispatch == NULL) {
        HDF_LOGE("RtcReadReg: service is invalid!");
        ret = HDF_ERR_MALLOC_FAIL;
        goto EXIT;
    }

    ret = service->dispatcher->Dispatch(&service->object, RTC_IO_READREG, data, reply);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("RtcReadReg: fail, ret is %d!", ret);
        goto EXIT;
    }

    if (!HdfSbufReadUint8(reply, value)) {
        HDF_LOGE("RtcReadReg: read value fail!");
        ret = HDF_ERR_IO;
        goto EXIT;
    }

EXIT:
    HdfSbufRecycle(data);
    HdfSbufRecycle(reply);
    return ret;
}

int32_t RtcWriteReg(DevHandle handle, uint8_t usrDefIndex, uint8_t value)
{
    int32_t ret;
    struct HdfSBuf *data = NULL;
    struct HdfIoService *service = NULL;

    if (handle == NULL) {
        HDF_LOGE("RtcWriteReg: handle is null!");
        return HDF_ERR_INVALID_OBJECT;
    }

    data = HdfSbufObtainDefaultSize();
    if (data == NULL) {
        HDF_LOGE("RtcWriteReg: fail to obtain data!");
        return HDF_ERR_MALLOC_FAIL;
    }

    if (!HdfSbufWriteUint8(data, usrDefIndex)) {
        HDF_LOGE("RtcWriteReg: write usrDefIndex fail!");
        HdfSbufRecycle(data);
        return HDF_ERR_IO;
    }

    if (!HdfSbufWriteUint8(data, value)) {
        HDF_LOGE("RtcWriteReg: write value fail!");
        HdfSbufRecycle(data);
        return HDF_ERR_IO;
    }

    service = (struct HdfIoService *)handle;
    if (service->dispatcher == NULL || service->dispatcher->Dispatch == NULL) {
        HDF_LOGE("RtcWriteReg: service is invalid!");
        HdfSbufRecycle(data);
        return HDF_ERR_MALLOC_FAIL;
    }

    ret = service->dispatcher->Dispatch(&service->object, RTC_IO_WRITEREG, data, NULL);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("RtcWriteReg: fail, ret is %d!", ret);
        HdfSbufRecycle(data);
        return ret;
    }

    HdfSbufRecycle(data);
    return HDF_SUCCESS;
}
