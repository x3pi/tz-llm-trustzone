/*
 * Copyright (c) 2022-2023 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include "devsvc_manager_clnt.h"
#include "hdf_device_desc.h"
#include "hdf_device_object.h"
#include "hdf_log.h"
#include "osal_mem.h"
#include "platform_listener_common.h"
#include "rtc_core.h"
#include "rtc_if.h"

#define HDF_LOG_TAG rtc_service_c

static int32_t RtcServiceIoReadTime(struct RtcHost *host, struct HdfSBuf *reply)
{
    int32_t ret;
    struct RtcTime time;

    ret = RtcHostReadTime(host, &time);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("RtcServiceIoReadTime: host read time fail!");
        return ret;
    }

    if (!HdfSbufWriteBuffer(reply, &time, sizeof(time))) {
        HDF_LOGE("RtcServiceIoReadTime: write buffer fail!");
        return HDF_ERR_IO;
    }

    return HDF_SUCCESS;
}

static int32_t RtcServiceIoWriteTime(struct RtcHost *host, struct HdfSBuf *data)
{
    int32_t ret;
    uint32_t len;
    struct RtcTime *time = NULL;

    if (!HdfSbufReadBuffer(data, (const void **)&time, &len) || sizeof(*time) != len) {
        HDF_LOGE("RtcServiceIoWriteTime: read buffer fail!");
        return HDF_ERR_IO;
    }

    ret = RtcHostWriteTime(host, time);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("RtcServiceIoWriteTime: host write time fail!");
        return ret;
    }

    return HDF_SUCCESS;
}

static int32_t RtcServiceIoReadAlarm(struct RtcHost *host, struct HdfSBuf *data, struct HdfSBuf *reply)
{
    int32_t ret;
    uint32_t alarmIndex = 0;
    struct RtcTime time;

    if (!HdfSbufReadUint32(data, &alarmIndex)) {
        HDF_LOGE("RtcServiceIoReadAlarm: read alarmIndex fail!");
        return HDF_ERR_IO;
    }

    ret = RtcHostReadAlarm(host, (enum RtcAlarmIndex)alarmIndex, &time);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("RtcServiceIoReadAlarm: host read alarm fail!");
        return ret;
    }

    if (!HdfSbufWriteBuffer(reply, &time, sizeof(time))) {
        HDF_LOGE("RtcServiceIoReadAlarm: write buffer fail!");
        return HDF_ERR_IO;
    }

    return HDF_SUCCESS;
}

static int32_t RtcServiceIoWriteAlarm(struct RtcHost *host, struct HdfSBuf *data)
{
    int32_t ret;
    uint32_t len;
    uint32_t alarmIndex = 0;
    struct RtcTime *time = NULL;

    if (!HdfSbufReadUint32(data, &alarmIndex)) {
        HDF_LOGE("RtcServiceIoWriteAlarm: read alarmIndex fail!");
        return HDF_ERR_IO;
    }

    if (!HdfSbufReadBuffer(data, (const void **)&time, &len) || sizeof(*time) != len) {
        HDF_LOGE("RtcServiceIoWriteAlarm: read buffer fail!");
        return HDF_ERR_IO;
    }

    ret = RtcHostWriteAlarm(host, (enum RtcAlarmIndex)alarmIndex, time);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("RtcServiceIoWriteAlarm: host write alarm fail!");
        return ret;
    }

    return HDF_SUCCESS;
}

static int32_t RtcAlarmServiceCallback(enum RtcAlarmIndex index)
{
    struct RtcHost *host = NULL;
    int32_t ret;
    struct HdfSBuf *data = NULL;

    host = (struct RtcHost *)DevSvcManagerClntGetService("HDF_PLATFORM_RTC");
    if (host == NULL) {
        HDF_LOGE("RtcAlarmServiceCallback: rtc get service fail!");
        return HDF_FAILURE;
    }

    data = HdfSbufObtainDefaultSize();
    if (data == NULL) {
        HDF_LOGE("RtcAlarmServiceCallback: fail to obtain data!");
        return HDF_ERR_IO;
    }
    if (!HdfSbufWriteUint8(data, index)) {
        HDF_LOGE("RtcAlarmServiceCallback: write index fail!");
        HdfSbufRecycle(data);
        return HDF_ERR_IO;
    }
    ret = HdfDeviceSendEvent(host->device, PLATFORM_LISTENER_EVENT_RTC_ALARM_NOTIFY, data);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("RtcAlarmServiceCallback: send event fail, ret=%d!", ret);
        HdfSbufRecycle(data);
        return ret;
    }
    HdfSbufRecycle(data);
    return HDF_SUCCESS;
}

static int32_t RtcServiceIoRegisterAlarmCallback(struct RtcHost *host, struct HdfSBuf *data)
{
    uint32_t alarmIndex;

    if (data == NULL) {
        HDF_LOGE("RtcServiceIoRegisterAlarmCallback: data is null!");
        return HDF_ERR_INVALID_PARAM;
    }
    if (!HdfSbufReadUint32(data, &alarmIndex)) {
        HDF_LOGE("RtcServiceIoRegisterAlarmCallback: read alarmIndex fail!");
        return HDF_ERR_IO;
    }

    return RtcHostRegisterAlarmCallback(host, alarmIndex, RtcAlarmServiceCallback);
}

static int32_t RtcServiceIoInterruptEnable(struct RtcHost *host, struct HdfSBuf *data)
{
    int32_t ret;
    uint32_t alarmIndex = 0;
    uint8_t enable = 0;

    if (!HdfSbufReadUint32(data, &alarmIndex)) {
        HDF_LOGE("RtcServiceIoInterruptEnable: read alarmIndex fail!");
        return HDF_ERR_IO;
    }

    if (!HdfSbufReadUint8(data, &enable)) {
        HDF_LOGE("RtcServiceIoInterruptEnable: read enable fail!");
        return HDF_ERR_IO;
    }

    ret = RtcHostAlarmInterruptEnable(host, (enum RtcAlarmIndex)alarmIndex, enable);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("RtcServiceIoInterruptEnable: host alarm interrupt enable fail, ret: %d!", ret);
        return ret;
    }

    return HDF_SUCCESS;
}

static int32_t RtcServiceIoGetFreq(struct RtcHost *host, struct HdfSBuf *reply)
{
    int32_t ret;
    uint32_t freq = 0;

    ret = RtcHostGetFreq(host, &freq);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("RtcServiceIoGetFreq: host get freq fail! ret: %d!", ret);
        return ret;
    }

    if (!HdfSbufWriteUint32(reply, freq)) {
        HDF_LOGE("RtcServiceIoGetFreq: write freq fail!");
        return HDF_ERR_IO;
    }

    return HDF_SUCCESS;
}

static int32_t RtcServiceIoSetFreq(struct RtcHost *host, struct HdfSBuf *data)
{
    int32_t ret;
    uint32_t freq = 0;

    if (!HdfSbufReadUint32(data, &freq)) {
        HDF_LOGE("RtcServiceIoSetFreq: read freq fail");
        return HDF_ERR_IO;
    }

    ret = RtcHostSetFreq(host, freq);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("RtcServiceIoSetFreq: host set freq fail! ret: %d!", ret);
        return ret;
    }

    return HDF_SUCCESS;
}

static int32_t RtcServiceIoReset(struct RtcHost *host)
{
    int32_t ret;

    ret = RtcHostReset(host);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("RtcServiceIoReset: host reset fail, ret: %d!", ret);
        return ret;
    }

    return HDF_SUCCESS;
}

static int32_t RtcServiceIoReadReg(struct RtcHost *host, struct HdfSBuf *data, struct HdfSBuf *reply)
{
    int32_t ret;
    uint8_t usrDefIndex = 0;
    uint8_t value = 0;

    if (!HdfSbufReadUint8(data, &usrDefIndex)) {
        HDF_LOGE("RtcServiceIoReadReg: read usrDefIndex fail!");
        return HDF_ERR_IO;
    }

    ret = RtcHostReadReg(host, usrDefIndex, &value);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("RtcServiceIoReadReg: host read reg fail, ret: %d!", ret);
        return ret;
    }

    if (!HdfSbufWriteUint8(reply, value)) {
        HDF_LOGE("RtcServiceIoReadReg: write value fail!");
        return HDF_ERR_IO;
    }

    return HDF_SUCCESS;
}

static int32_t RtcServiceIoWriteReg(struct RtcHost *host, struct HdfSBuf *data)
{
    int32_t ret;
    uint8_t usrDefIndex = 0;
    uint8_t value = 0;

    if (!HdfSbufReadUint8(data, &usrDefIndex)) {
        HDF_LOGE("RtcServiceIoWriteReg: read usrDefIndex fail!");
        return HDF_ERR_IO;
    }

    if (!HdfSbufReadUint8(data, &value)) {
        HDF_LOGE("RtcServiceIoWriteReg: read value fail!");
        return HDF_ERR_IO;
    }

    ret = RtcHostWriteReg(host, usrDefIndex, value);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("RtcServiceIoWriteReg: host write reg fail, ret: %d!", ret);
        return ret;
    }

    return HDF_SUCCESS;
}

int32_t RtcIoDispatch(struct HdfDeviceIoClient *client, int cmd, struct HdfSBuf *data, struct HdfSBuf *reply)
{
    struct RtcHost *host = NULL;

    if (client == NULL) {
        HDF_LOGE("RtcIoDispatch: client is null!");
        return HDF_ERR_INVALID_OBJECT;
    }

    if (client->device == NULL) {
        HDF_LOGE("RtcIoDispatch: client->device is null!");
        return HDF_ERR_INVALID_OBJECT;
    }

    if (client->device->service == NULL) {
        HDF_LOGE("RtcIoDispatch: client->device->service is null!");
        return HDF_ERR_INVALID_OBJECT;
    }

    host = (struct RtcHost *)client->device->service;

    switch (cmd) {
        case RTC_IO_READTIME:
            return RtcServiceIoReadTime(host, reply);
        case RTC_IO_WRITETIME:
            return RtcServiceIoWriteTime(host, data);
        case RTC_IO_READALARM:
            return RtcServiceIoReadAlarm(host, data, reply);
        case RTC_IO_WRITEALARM:
            return RtcServiceIoWriteAlarm(host, data);
        case RTC_IO_REGISTERALARMCALLBACK:
            return RtcServiceIoRegisterAlarmCallback(host, data);
        case RTC_IO_ALARMINTERRUPTENABLE:
            return RtcServiceIoInterruptEnable(host, data);
        case RTC_IO_GETFREQ:
            return RtcServiceIoGetFreq(host, reply);
        case RTC_IO_SETFREQ:
            return RtcServiceIoSetFreq(host, data);
        case RTC_IO_RESET:
            return RtcServiceIoReset(host);
        case RTC_IO_READREG:
            return RtcServiceIoReadReg(host, data, reply);
        case RTC_IO_WRITEREG:
            return RtcServiceIoWriteReg(host, data);
        default:
            HDF_LOGE("RtcIoDispatch: cmd %d is not support!", cmd);
            return HDF_ERR_NOT_SUPPORT;
    }
}
