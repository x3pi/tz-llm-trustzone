/*
 * Copyright (c) 2020-2023 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include "rtc_core.h"
#include "hdf_log.h"
#include "osal_mem.h"
#include "rtc_if.h"

#define HDF_LOG_TAG rtc_core_c

int32_t RtcHostReadTime(struct RtcHost *host, struct RtcTime *time)
{
    if (host == NULL || time == NULL) {
        HDF_LOGE("RtcHostReadTime: host or time is null!");
        return HDF_ERR_INVALID_OBJECT;
    }

    if (host->method == NULL || host->method->ReadTime == NULL) {
        HDF_LOGE("RtcHostReadTime: method or ReadTime is null!");
        return HDF_ERR_NOT_SUPPORT;
    }

    return host->method->ReadTime(host, time);
}

int32_t RtcHostWriteTime(struct RtcHost *host, const struct RtcTime *time)
{
    if (host == NULL || time == NULL) {
        HDF_LOGE("RtcHostWriteTime: host or time is null!");
        return HDF_ERR_INVALID_OBJECT;
    }

    if (host->method == NULL || host->method->WriteTime == NULL) {
        HDF_LOGE("RtcHostWriteTime: method or WriteTime is null!");
        return HDF_ERR_NOT_SUPPORT;
    }

    return host->method->WriteTime(host, time);
}

int32_t RtcHostReadAlarm(struct RtcHost *host, enum RtcAlarmIndex alarmIndex, struct RtcTime *time)
{
    if (host == NULL || time == NULL) {
        HDF_LOGE("RtcHostReadAlarm: host is null!");
        return HDF_ERR_INVALID_OBJECT;
    }

    if (host->method == NULL || host->method->ReadAlarm == NULL) {
        HDF_LOGE("RtcHostReadAlarm: method or ReadAlarm is null!");
        return HDF_ERR_NOT_SUPPORT;
    }

    return host->method->ReadAlarm(host, alarmIndex, time);
}

int32_t RtcHostWriteAlarm(struct RtcHost *host, enum RtcAlarmIndex alarmIndex, const struct RtcTime *time)
{
    if (host == NULL || time == NULL) {
        HDF_LOGE("RtcHostWriteAlarm: host is null!");
        return HDF_ERR_INVALID_OBJECT;
    }

    if (host->method == NULL || host->method->WriteAlarm == NULL) {
        HDF_LOGE("RtcHostWriteAlarm: method or WriteAlarm is null!");
        return HDF_ERR_NOT_SUPPORT;
    }

    return host->method->WriteAlarm(host, alarmIndex, time);
}

int32_t RtcHostRegisterAlarmCallback(struct RtcHost *host, enum RtcAlarmIndex alarmIndex, RtcAlarmCallback cb)
{
    if (host == NULL || cb == NULL) {
        HDF_LOGE("RtcHostRegisterAlarmCallback: host or cb is null!");
        return HDF_ERR_INVALID_OBJECT;
    }

    if (host->method == NULL || host->method->RegisterAlarmCallback == NULL) {
        HDF_LOGE("RtcHostRegisterAlarmCallback: method or RegisterAlarmCallback is null!");
        return HDF_ERR_NOT_SUPPORT;
    }

    return host->method->RegisterAlarmCallback(host, alarmIndex, cb);
}

int32_t RtcHostAlarmInterruptEnable(struct RtcHost *host, enum RtcAlarmIndex alarmIndex, uint8_t enable)
{
    if (host == NULL) {
        HDF_LOGE("RtcHostAlarmInterruptEnable: host is null!");
        return HDF_ERR_INVALID_OBJECT;
    }

    if (host->method == NULL || host->method->AlarmInterruptEnable == NULL) {
        HDF_LOGE("RtcHostAlarmInterruptEnable: method or AlarmInterruptEnable is null!");
        return HDF_ERR_NOT_SUPPORT;
    }

    return host->method->AlarmInterruptEnable(host, alarmIndex, enable);
}

int32_t RtcHostGetFreq(struct RtcHost *host, uint32_t *freq)
{
    if (host == NULL || freq == NULL) {
        HDF_LOGE("RtcHostGetFreq: host or freq is null!");
        return HDF_ERR_INVALID_OBJECT;
    }

    if (host->method == NULL || host->method->GetFreq == NULL) {
        HDF_LOGE("RtcHostGetFreq: method or GetFreq is null!");
        return HDF_ERR_NOT_SUPPORT;
    }

    return host->method->GetFreq(host, freq);
}

int32_t RtcHostSetFreq(struct RtcHost *host, uint32_t freq)
{
    if (host == NULL) {
        HDF_LOGE("RtcHostSetFreq: host is null!");
        return HDF_ERR_INVALID_OBJECT;
    }

    if (host->method == NULL || host->method->SetFreq == NULL) {
        HDF_LOGE("RtcHostSetFreq: method or SetFreq is null!");
        return HDF_ERR_NOT_SUPPORT;
    }

    return host->method->SetFreq(host, freq);
}

int32_t RtcHostReset(struct RtcHost *host)
{
    if (host == NULL) {
        HDF_LOGE("RtcHostReset: host is null!");
        return HDF_ERR_INVALID_OBJECT;
    }

    if (host->method == NULL || host->method->Reset == NULL) {
        HDF_LOGE("RtcHostReset: method or Reset is null!");
        return HDF_ERR_NOT_SUPPORT;
    }

    return host->method->Reset(host);
}

int32_t RtcHostReadReg(struct RtcHost *host, uint8_t usrDefIndex, uint8_t *value)
{
    if (host == NULL || value == NULL) {
        HDF_LOGE("RtcHostReadReg: host or value is null!");
        return HDF_ERR_INVALID_OBJECT;
    }

    if (host->method == NULL || host->method->ReadReg == NULL) {
        HDF_LOGE("RtcHostReadReg: method or ReadReg is null!");
        return HDF_ERR_NOT_SUPPORT;
    }

    return host->method->ReadReg(host, usrDefIndex, value);
}

int32_t RtcHostWriteReg(struct RtcHost *host, uint8_t usrDefIndex, uint8_t value)
{
    if (host == NULL) {
        HDF_LOGE("RtcHostWriteReg: host is null!");
        return HDF_ERR_INVALID_OBJECT;
    }

    if (host->method == NULL || host->method->WriteReg == NULL) {
        HDF_LOGE("RtcHostWriteReg: method or WriteReg is null!");
        return HDF_ERR_NOT_SUPPORT;
    }

    return host->method->WriteReg(host, usrDefIndex, value);
}

struct RtcHost *RtcHostCreate(struct HdfDeviceObject *device)
{
    struct RtcHost *host = NULL;

    if (device == NULL) {
        HDF_LOGE("RtcHostCreate: device is null!");
        return NULL;
    }

    host = (struct RtcHost *)OsalMemCalloc(sizeof(*host));
    if (host == NULL) {
        HDF_LOGE("RtcHostCreate: malloc host fail!");
        return NULL;
    }

    host->device = device;
    device->service = &(host->service);
    host->method = NULL;
    host->data = NULL;
    host->device->service->Dispatch = RtcIoDispatch;
    return host;
}

void RtcHostDestroy(struct RtcHost *host)
{
    if (host == NULL) {
        HDF_LOGE("RtcHostDestroy: host is null!");
        return;
    }

    host->device = NULL;
    host->method = NULL;
    host->data = NULL;
    OsalMemFree(host);
}
