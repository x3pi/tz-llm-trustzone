/*
 * Copyright (c) 2021-2023 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include "adc_core.h"
#include "hdf_device_desc.h"
#include "hdf_log.h"
#include "osal_mem.h"
#include "osal_spinlock.h"
#include "osal_time.h"
#include "platform_core.h"
#include "platform_trace.h"

#define HDF_LOG_TAG adc_core_c
#define ADC_TRACE_BASIC_PARAM_NUM  2
#define ADC_TRACE_PARAM_STOP_NUM   2

#define ADC_HANDLE_SHIFT    0xFF00U

struct AdcManager {
    struct IDeviceIoService service;
    struct HdfDeviceObject *device;
    struct AdcDevice *devices[ADC_DEVICES_MAX];
    OsalSpinlock spin;
};

static struct AdcManager *g_adcManager = NULL;

static int32_t AdcDeviceLockDefault(struct AdcDevice *device)
{
    if (device == NULL) {
        HDF_LOGE("AdcDeviceLockDefault: device is null!");
        return HDF_ERR_INVALID_OBJECT;
    }
    return OsalSpinLock(&device->spin);
}

static void AdcDeviceUnlockDefault(struct AdcDevice *device)
{
    if (device == NULL) {
        HDF_LOGE("AdcDeviceUnlockDefault: device is null!");
        return;
    }
    (void)OsalSpinUnlock(&device->spin);
}

static const struct AdcLockMethod g_adcLockOpsDefault = {
    .lock = AdcDeviceLockDefault,
    .unlock = AdcDeviceUnlockDefault,
};

static inline int32_t AdcDeviceLock(struct AdcDevice *device)
{
    if (device->lockOps == NULL || device->lockOps->lock == NULL) {
        HDF_LOGE("AdcDeviceLock: adc device lock is not support!");
        return HDF_ERR_NOT_SUPPORT;
    }
    return device->lockOps->lock(device);
}

static inline void AdcDeviceUnlock(struct AdcDevice *device)
{
    if (device->lockOps != NULL && device->lockOps->unlock != NULL) {
        device->lockOps->unlock(device);
    }
}

int32_t AdcDeviceStart(struct AdcDevice *device)
{
    int32_t ret;

    if (device == NULL) {
        HDF_LOGE("AdcDeviceStart: device is null!");
        return HDF_ERR_INVALID_OBJECT;
    }

    if (device->ops == NULL || device->ops->start == NULL) {
        HDF_LOGE("AdcDeviceStart: ops or start is null!");
        return HDF_ERR_NOT_SUPPORT;
    }

    if (AdcDeviceLock(device) != HDF_SUCCESS) {
        HDF_LOGE("AdcDeviceStart: lock adc device fail!");
        return HDF_ERR_DEVICE_BUSY;
    }

    ret = device->ops->start(device);
    if (PlatformTraceStart() == HDF_SUCCESS) {
        unsigned int infos[ADC_TRACE_BASIC_PARAM_NUM];
        infos[PLATFORM_TRACE_UINT_PARAM_SIZE_1 - 1] = device->devNum;
        infos[PLATFORM_TRACE_UINT_PARAM_SIZE_2 - 1] = device->chanNum;
        PlatformTraceAddUintMsg(PLATFORM_TRACE_MODULE_ADC, PLATFORM_TRACE_MODULE_ADC_FUN_START,
            infos, ADC_TRACE_BASIC_PARAM_NUM);
        PlatformTraceStop();
    }
    AdcDeviceUnlock(device);
    return ret;
}

int32_t AdcDeviceStop(struct AdcDevice *device)
{
    int32_t ret;

    if (device == NULL) {
        HDF_LOGE("AdcDeviceStop: device is null!");
        return HDF_ERR_INVALID_OBJECT;
    }

    if (device->ops == NULL || device->ops->stop == NULL) {
        HDF_LOGE("AdcDeviceStop: ops or stop is null!");
        return HDF_ERR_NOT_SUPPORT;
    }

    if (AdcDeviceLock(device) != HDF_SUCCESS) {
        HDF_LOGE("AdcDeviceStop: lock adc device fail!");
        return HDF_ERR_DEVICE_BUSY;
    }

    ret = device->ops->stop(device);
    if (PlatformTraceStart() == HDF_SUCCESS) {
        unsigned int infos[ADC_TRACE_PARAM_STOP_NUM];
        infos[PLATFORM_TRACE_UINT_PARAM_SIZE_1 - 1] = device->devNum;
        infos[PLATFORM_TRACE_UINT_PARAM_SIZE_2 - 1] = device->chanNum;
        PlatformTraceAddUintMsg(PLATFORM_TRACE_MODULE_ADC, PLATFORM_TRACE_MODULE_ADC_FUN_STOP,
            infos, ADC_TRACE_PARAM_STOP_NUM);
        PlatformTraceStop();
        PlatformTraceInfoDump();
    }
    AdcDeviceUnlock(device);
    return ret;
}

static int32_t AdcManagerAddDevice(struct AdcDevice *device)
{
    int32_t ret;
    struct AdcManager *manager = g_adcManager;

    if (device->devNum >= ADC_DEVICES_MAX) {
        HDF_LOGE("AdcManagerAddDevice: devNum:%u exceed!", device->devNum);
        return HDF_ERR_INVALID_OBJECT;
    }

    if (manager == NULL) {
        HDF_LOGE("AdcManagerAddDevice: get adc manager fail!");
        return HDF_ERR_NOT_SUPPORT;
    }

    if (OsalSpinLockIrq(&manager->spin) != HDF_SUCCESS) {
        HDF_LOGE("AdcManagerAddDevice: lock adc manager fail!");
        return HDF_ERR_DEVICE_BUSY;
    }

    if (manager->devices[device->devNum] != NULL) {
        HDF_LOGE("AdcManagerAddDevice: adc device num:%u already exits!", device->devNum);
        ret = HDF_FAILURE;
    } else {
        manager->devices[device->devNum] = device;
        ret = HDF_SUCCESS;
    }

    (void)OsalSpinUnlockIrq(&manager->spin);
    return ret;
}

static void AdcManagerRemoveDevice(struct AdcDevice *device)
{
    struct AdcManager *manager = g_adcManager;

    if (device->devNum < 0 || device->devNum >= ADC_DEVICES_MAX) {
        HDF_LOGE("AdcManagerRemoveDevice: invalid devNum:%u!", device->devNum);
        return;
    }

    if (manager == NULL) {
        HDF_LOGE("AdcManagerRemoveDevice: get adc manager fail!");
        return;
    }

    if (OsalSpinLockIrq(&manager->spin) != HDF_SUCCESS) {
        HDF_LOGE("AdcManagerRemoveDevice: lock adc manager fail!");
        return;
    }

    if (manager->devices[device->devNum] != device) {
        HDF_LOGE("AdcManagerRemoveDevice: adc device(%u) not in manager!", device->devNum);
    } else {
        manager->devices[device->devNum] = NULL;
    }

    (void)OsalSpinUnlockIrq(&manager->spin);
}

static struct AdcDevice *AdcManagerFindDevice(uint32_t number)
{
    struct AdcDevice *device = NULL;
    struct AdcManager *manager = g_adcManager;

    if (number >= ADC_DEVICES_MAX) {
        HDF_LOGE("AdcManagerFindDevice: invalid devNum:%u!", number);
        return NULL;
    }

    if (manager == NULL) {
        HDF_LOGE("AdcManagerFindDevice: get adc manager fail!");
        return NULL;
    }

    if (OsalSpinLockIrq(&manager->spin) != HDF_SUCCESS) {
        HDF_LOGE("AdcManagerFindDevice: lock adc manager fail!");
        return NULL;
    }

    device = manager->devices[number];
    (void)OsalSpinUnlockIrq(&manager->spin);

    return device;
}

struct AdcDevice *AdcDeviceGet(uint32_t number)
{
    return AdcManagerFindDevice(number);
}

void AdcDevicePut(struct AdcDevice *device)
{
    (void)device;
}

static struct AdcDevice *AdcDeviceOpen(uint32_t number)
{
    int32_t ret;
    struct AdcDevice *device = NULL;

    device = AdcDeviceGet(number);
    if (device == NULL) {
        HDF_LOGE("AdcDeviceOpen: get adc device fail!");
        return NULL;
    }

    ret = AdcDeviceStart(device);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("AdcDeviceOpen: start adc device fail!");
        return NULL;
    }

    return device;
}

static void AdcDeviceClose(struct AdcDevice *device)
{
    if (device == NULL) {
        HDF_LOGE("AdcDeviceClose: close adc device fail!");
        return;
    }

    (void)AdcDeviceStop(device);
    AdcDevicePut(device);
}

int32_t AdcDeviceAdd(struct AdcDevice *device)
{
    int32_t ret;

    if (device == NULL) {
        HDF_LOGE("AdcDeviceAdd: device is null!");
        return HDF_ERR_INVALID_OBJECT;
    }

    if (device->ops == NULL) {
        HDF_LOGE("AdcDeviceAdd: no ops supplied!");
        return HDF_ERR_INVALID_OBJECT;
    }

    if (device->lockOps == NULL) {
        HDF_LOGI("AdcDeviceAdd: use default lockOps!");
        device->lockOps = &g_adcLockOpsDefault;
    }

    if (OsalSpinInit(&device->spin) != HDF_SUCCESS) {
        HDF_LOGE("AdcDeviceAdd: init lock fail!");
        return HDF_FAILURE;
    }

    ret = AdcManagerAddDevice(device);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("AdcDeviceAdd: adc manager add device fail!");
        (void)OsalSpinDestroy(&device->spin);
    }
    return ret;
}

void AdcDeviceRemove(struct AdcDevice *device)
{
    if (device == NULL) {
        HDF_LOGE("AdcDeviceRemove: device is null!");
        return;
    }
    AdcManagerRemoveDevice(device);
    (void)OsalSpinDestroy(&device->spin);
}

int32_t AdcDeviceRead(struct AdcDevice *device, uint32_t channel, uint32_t *val)
{
    int32_t ret;

    if (device == NULL) {
        HDF_LOGE("AdcDeviceRead: device is null!");
        return HDF_ERR_INVALID_OBJECT;
    }

    if (device->ops == NULL || device->ops->read == NULL) {
        HDF_LOGE("AdcDeviceRead: ops or read is null!");
        return HDF_ERR_NOT_SUPPORT;
    }

    if (val == NULL) {
        HDF_LOGE("AdcDeviceRead: invalid val pointer!");
        return HDF_ERR_INVALID_PARAM;
    }

    if (AdcDeviceLock(device) != HDF_SUCCESS) {
        HDF_LOGE("AdcDeviceRead: lock adc device fail!");
        return HDF_ERR_DEVICE_BUSY;
    }

    ret = device->ops->read(device, channel, val);
    AdcDeviceUnlock(device);
    return ret;
}

static int32_t AdcManagerIoOpen(struct HdfSBuf *data, struct HdfSBuf *reply)
{
    uint32_t number;

    if (data == NULL || reply == NULL) {
        HDF_LOGE("AdcManagerIoOpen: invalid data or reply!");
        return HDF_ERR_INVALID_PARAM;
    }

    if (!HdfSbufReadUint32(data, &number)) {
        HDF_LOGE("AdcManagerIoOpen: read number fail!");
        return HDF_ERR_IO;
    }

    if (number >= ADC_DEVICES_MAX) {
        HDF_LOGE("AdcManagerIoOpen: invalid number!");
        return HDF_ERR_INVALID_PARAM;
    }

    if (AdcDeviceOpen(number) == NULL) {
        HDF_LOGE("AdcManagerIoOpen: get device %u fail!", number);
        return HDF_ERR_NOT_SUPPORT;
    }

    number = (uint32_t)(number + ADC_HANDLE_SHIFT);
    if (!HdfSbufWriteUint32(reply, (uint32_t)(uintptr_t)number)) {
        HDF_LOGE("AdcManagerIoOpen: write number fail!");
        return HDF_ERR_IO;
    }

    return HDF_SUCCESS;
}

static int32_t AdcManagerIoClose(struct HdfSBuf *data, struct HdfSBuf *reply)
{
    uint32_t number;

    (void)reply;
    if (data == NULL) {
        HDF_LOGE("AdcManagerIoClose: invalid data!");
        return HDF_ERR_INVALID_PARAM;
    }

    if (!HdfSbufReadUint32(data, &number)) {
        HDF_LOGE("AdcManagerIoClose: read number fail!");
        return HDF_ERR_IO;
    }

    number = (uint32_t)(number - ADC_HANDLE_SHIFT);
    if (number >= ADC_DEVICES_MAX) {
        HDF_LOGE("AdcManagerIoClose: get device %u fail!", number);
        return HDF_ERR_INVALID_PARAM;
    }

    AdcDeviceClose(AdcManagerFindDevice(number));
    return HDF_SUCCESS;
}

static int32_t AdcManagerIoRead(struct HdfSBuf *data, struct HdfSBuf *reply)
{
    int32_t ret;
    uint32_t number;
    uint32_t channel;
    uint32_t val;

    if (data == NULL || reply == NULL) {
        HDF_LOGE("AdcManagerIoRead: invalid data or reply!");
        return HDF_ERR_INVALID_PARAM;
    }

    if (!HdfSbufReadUint32(data, &number)) {
        HDF_LOGE("AdcManagerIoRead: read number fail!");
        return HDF_ERR_IO;
    }

    if (!HdfSbufReadUint32(data, &channel)) {
        HDF_LOGE("AdcManagerIoRead: read channel fail!");
        return HDF_ERR_IO;
    }

    number  = (uint32_t)(number - ADC_HANDLE_SHIFT);
    ret = AdcDeviceRead(AdcManagerFindDevice(number), channel, &val);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("AdcManagerIoRead: read val fail!");
        return HDF_ERR_IO;
    }

    if (!HdfSbufWriteUint32(reply, val)) {
        HDF_LOGE("AdcManagerIoRead: write val fail!");
        return HDF_ERR_IO;
    }

    return ret;
}

static int32_t AdcManagerDispatch(struct HdfDeviceIoClient *client, int cmd,
    struct HdfSBuf *data, struct HdfSBuf *reply)
{
    (void)client;
    switch (cmd) {
        case ADC_IO_OPEN:
            return AdcManagerIoOpen(data, reply);
        case ADC_IO_CLOSE:
            return AdcManagerIoClose(data, reply);
        case ADC_IO_READ:
            return AdcManagerIoRead(data, reply);
        default:
            HDF_LOGE("AdcManagerDispatch: cmd %d is not support!", cmd);
            return HDF_ERR_NOT_SUPPORT;
    }
    return HDF_SUCCESS;
}

static int32_t AdcManagerBind(struct HdfDeviceObject *device)
{
    (void)device;
    return HDF_SUCCESS;
}

static int32_t AdcManagerInit(struct HdfDeviceObject *device)
{
    int32_t ret;
    struct AdcManager *manager = NULL;

    HDF_LOGI("AdcManagerInit: enter!");
    if (device == NULL) {
        HDF_LOGE("AdcManagerInit: device is null!");
        return HDF_ERR_INVALID_OBJECT;
    }

    manager = (struct AdcManager *)OsalMemCalloc(sizeof(*manager));
    if (manager == NULL) {
        HDF_LOGE("AdcManagerInit: memcalloc for manager fail!");
        return HDF_ERR_MALLOC_FAIL;
    }

    ret = OsalSpinInit(&manager->spin);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("AdcManagerInit: spinlock init fail!");
        OsalMemFree(manager);
        return HDF_FAILURE;
    }

    manager->device = device;
    g_adcManager = manager;
    device->service = &manager->service;
    device->service->Dispatch = AdcManagerDispatch;
    return HDF_SUCCESS;
}

static void AdcManagerRelease(struct HdfDeviceObject *device)
{
    struct AdcManager *manager = NULL;

    HDF_LOGI("AdcManagerRelease: enter!");
    if (device == NULL) {
        HDF_LOGE("AdcManagerRelease: device is null!");
        return;
    }

    manager = (struct AdcManager *)device->service;
    if (manager == NULL) {
        HDF_LOGI("AdcManagerRelease: no service bind!");
        return;
    }

    g_adcManager = NULL;
    OsalMemFree(manager);
}

struct HdfDriverEntry g_adcManagerEntry = {
    .moduleVersion = 1,
    .Bind = AdcManagerBind,
    .Init = AdcManagerInit,
    .Release = AdcManagerRelease,
    .moduleName = "HDF_PLATFORM_ADC_MANAGER",
};
HDF_INIT(g_adcManagerEntry);
