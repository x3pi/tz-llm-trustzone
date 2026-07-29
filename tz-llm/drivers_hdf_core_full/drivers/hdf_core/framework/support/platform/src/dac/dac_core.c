/*
 * Copyright (c) 2021-2023 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include "dac_core.h"
#include "hdf_device_desc.h"
#include "hdf_log.h"
#include "osal_mem.h"
#include "osal_spinlock.h"
#include "osal_time.h"
#include "platform_core.h"
#include "platform_trace.h"

#define DAC_HANDLE_SHIFT    0xFF00U
#define DAC_TRACE_BASIC_PARAM_NUM  2
#define DAC_TRACE_PARAM_STOP_NUM   2
#define HDF_LOG_TAG dac_core_c

struct DacManager {
    struct IDeviceIoService service;
    struct HdfDeviceObject *device;
    struct DacDevice *devices[DAC_DEVICES_MAX];
    OsalSpinlock spin;
};

static struct DacManager *g_dacManager = NULL;

static int32_t DacDeviceLockDefault(struct DacDevice *device)
{
    if (device == NULL) {
        HDF_LOGE("DacDeviceLockDefault: device is null!");
        return HDF_ERR_INVALID_OBJECT;
    }
    return OsalSpinLock(&device->spin);
}

static void DacDeviceUnlockDefault(struct DacDevice *device)
{
    if (device == NULL) {
        HDF_LOGE("DacDeviceUnlockDefault: device is null!");
        return;
    }
    (void)OsalSpinUnlock(&device->spin);
}

static const struct DacLockMethod g_dacLockOpsDefault = {
    .lock = DacDeviceLockDefault,
    .unlock = DacDeviceUnlockDefault,
};

static inline int32_t DacDeviceLock(struct DacDevice *device)
{
    if (device == NULL) {
        HDF_LOGE("DacDeviceLock: device is null!");
        return HDF_ERR_INVALID_OBJECT;
    }
    if (device->lockOps == NULL || device->lockOps->lock == NULL) {
        HDF_LOGE("DacDeviceLock: dac device lock is not support!");
        return HDF_ERR_NOT_SUPPORT;
    }
    return device->lockOps->lock(device);
}

static inline void DacDeviceUnlock(struct DacDevice *device)
{
    if (device == NULL) {
        HDF_LOGE("DacDeviceUnlock: device is null!");
        return;
    }
    if (device->lockOps != NULL && device->lockOps->unlock != NULL) {
        device->lockOps->unlock(device);
    }
}

static int32_t DacManagerAddDevice(struct DacDevice *device)
{
    int32_t ret;
    struct DacManager *manager = g_dacManager;

    if (device == NULL) {
        HDF_LOGE("DacManagerAddDevice: device is null!");
        return HDF_ERR_INVALID_OBJECT;
    }
    if (device->devNum >= DAC_DEVICES_MAX) {
        HDF_LOGE("DacManagerAddDevice: devNum:%u exceed!", device->devNum);
        return HDF_ERR_INVALID_OBJECT;
    }

    if (manager == NULL) {
        HDF_LOGE("DacManagerAddDevice: get dac manager fail!");
        return HDF_ERR_NOT_SUPPORT;
    }

    if (OsalSpinLockIrq(&manager->spin) != HDF_SUCCESS) {
        HDF_LOGE("DacManagerAddDevice: lock dac manager fail!");
        return HDF_ERR_DEVICE_BUSY;
    }

    if (manager->devices[device->devNum] != NULL) {
        HDF_LOGE("DacManagerAddDevice: dac device num:%u alwritey exits!", device->devNum);
        ret = HDF_FAILURE;
    } else {
        manager->devices[device->devNum] = device;
        ret = HDF_SUCCESS;
    }

    (void)OsalSpinUnlockIrq(&manager->spin);
    return ret;
}

static void DacManagerRemoveDevice(const struct DacDevice *device)
{
    struct DacManager *manager = g_dacManager;

    if (device == NULL) {
        HDF_LOGE("DacManagerRemoveDevice: device is null!");
        return;
    }
    if (device->devNum < 0 || device->devNum >= DAC_DEVICES_MAX) {
        HDF_LOGE("DacManagerRemoveDevice: invalid devNum:%u!", device->devNum);
        return;
    }

    if (manager == NULL) {
        HDF_LOGE("DacManagerRemoveDevice: get dac manager fail!");
        return;
    }

    if (OsalSpinLockIrq(&manager->spin) != HDF_SUCCESS) {
        HDF_LOGE("DacManagerRemoveDevice: lock dac manager fail!");
        return;
    }

    if (manager->devices[device->devNum] != device) {
        HDF_LOGE("DacManagerRemoveDevice: dac device(%u) not in manager!", device->devNum);
    } else {
        manager->devices[device->devNum] = NULL;
    }

    (void)OsalSpinUnlockIrq(&manager->spin);
}

static struct DacDevice *DacManagerFindDevice(uint32_t number)
{
    struct DacDevice *device = NULL;
    struct DacManager *manager = g_dacManager;

    if (number >= DAC_DEVICES_MAX) {
        HDF_LOGE("DacManagerFindDevice: invalid devNum:%u!", number);
        return NULL;
    }

    if (manager == NULL) {
        HDF_LOGE("DacManagerFindDevice: get dac manager fail!");
        return NULL;
    }

    if (OsalSpinLockIrq(&manager->spin) != HDF_SUCCESS) {
        HDF_LOGE("DacManagerFindDevice: lock dac manager fail!");
        return NULL;
    }

    device = manager->devices[number];
    (void)OsalSpinUnlockIrq(&manager->spin);

    return device;
}

int32_t DacDeviceAdd(struct DacDevice *device)
{
    int32_t ret;

    if (device == NULL) {
        HDF_LOGE("DacDeviceAdd: device is null!");
        return HDF_ERR_INVALID_OBJECT;
    }

    if (device->ops == NULL) {
        HDF_LOGE("DacDeviceAdd: no ops supplied!");
        return HDF_ERR_INVALID_OBJECT;
    }

    if (device->lockOps == NULL) {
        HDF_LOGI("DacDeviceAdd: use default lockOps!");
        device->lockOps = &g_dacLockOpsDefault;
    }

    if (OsalSpinInit(&device->spin) != HDF_SUCCESS) {
        HDF_LOGE("DacDeviceAdd: init lock fail!");
        return HDF_FAILURE;
    }

    ret = DacManagerAddDevice(device);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("DacDeviceAdd: dac manager add device fail!");
        (void)OsalSpinDestroy(&device->spin);
    }
    return ret;
}

void DacDeviceRemove(struct DacDevice *device)
{
    if (device == NULL) {
        HDF_LOGE("DacDeviceRemove: device is null!");
        return;
    }
    DacManagerRemoveDevice(device);
    (void)OsalSpinDestroy(&device->spin);
}

struct DacDevice *DacDeviceGet(uint32_t number)
{
    return DacManagerFindDevice(number);
}

void DacDevicePut(const struct DacDevice *device)
{
    (void)device;
}

static struct DacDevice *DacDeviceOpen(uint32_t number)
{
    int32_t ret;
    struct DacDevice *device = NULL;

    device = DacDeviceGet(number);
    if (device == NULL) {
        HDF_LOGE("DacDeviceOpen: get device fail!");
        return NULL;
    }

    ret = DacDeviceStart(device);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("DacDeviceOpen: start device fail!");
        return NULL;
    }

    return device;
}

static void DacDeviceClose(struct DacDevice *device)
{
    if (device == NULL) {
        HDF_LOGE("DacDeviceClose: device is null!");
        return;
    }

    (void)DacDeviceStop(device);
    DacDevicePut(device);
}

int32_t DacDeviceWrite(struct DacDevice *device, uint32_t channel, uint32_t val)
{
    int32_t ret;

    if (device == NULL) {
        HDF_LOGE("DacDeviceWrite: device is null!");
        return HDF_ERR_INVALID_OBJECT;
    }

    if (device->ops == NULL || device->ops->write == NULL) {
        HDF_LOGE("DacDeviceWrite: ops or write is null!");
        return HDF_ERR_NOT_SUPPORT;
    }

    if (DacDeviceLock(device) != HDF_SUCCESS) {
        HDF_LOGE("DacDeviceWrite: lock add device fail!");
        return HDF_ERR_DEVICE_BUSY;
    }

    ret = device->ops->write(device, channel, val);
    DacDeviceUnlock(device);
    return ret;
}

int32_t DacDeviceStart(struct DacDevice *device)
{
    int32_t ret;

    if (device == NULL) {
        HDF_LOGE("DacDeviceStart: device is null!");
        return HDF_ERR_INVALID_OBJECT;
    }

    if (device->ops == NULL || device->ops->start == NULL) {
        HDF_LOGE("DacDeviceStart: ops or start is null!");
        return HDF_ERR_NOT_SUPPORT;
    }

    if (DacDeviceLock(device) != HDF_SUCCESS) {
        HDF_LOGE("DacDeviceStart: lock dac device fail!");
        return HDF_ERR_DEVICE_BUSY;
    }

    ret = device->ops->start(device);
    if (PlatformTraceStart() == HDF_SUCCESS) {
        unsigned int infos[DAC_TRACE_BASIC_PARAM_NUM];
        infos[PLATFORM_TRACE_UINT_PARAM_SIZE_1 - 1] = device->devNum;
        infos[PLATFORM_TRACE_UINT_PARAM_SIZE_2 - 1] = device->chanNum;
        PlatformTraceAddUintMsg(
            PLATFORM_TRACE_MODULE_DAC, PLATFORM_TRACE_MODULE_DAC_FUN_START, infos, DAC_TRACE_BASIC_PARAM_NUM);
        PlatformTraceStop();
    }
    DacDeviceUnlock(device);
    return ret;
}

int32_t DacDeviceStop(struct DacDevice *device)
{
    int32_t ret;

    if (device == NULL) {
        HDF_LOGE("DacDeviceStop: device is null!");
        return HDF_ERR_INVALID_OBJECT;
    }

    if (device->ops == NULL || device->ops->stop == NULL) {
        HDF_LOGE("DacDeviceStop: ops or stop is null!");
        return HDF_ERR_NOT_SUPPORT;
    }

    if (DacDeviceLock(device) != HDF_SUCCESS) {
        HDF_LOGE("DacDeviceStop: lock dac device fail!");
        return HDF_ERR_DEVICE_BUSY;
    }

    ret = device->ops->stop(device);
    if (PlatformTraceStart() == HDF_SUCCESS) {
        unsigned int infos[DAC_TRACE_PARAM_STOP_NUM];
        infos[PLATFORM_TRACE_UINT_PARAM_SIZE_1 - 1] = device->devNum;
        infos[PLATFORM_TRACE_UINT_PARAM_SIZE_2 - 1] = device->chanNum;
        PlatformTraceAddUintMsg(
            PLATFORM_TRACE_MODULE_DAC, PLATFORM_TRACE_MODULE_DAC_FUN_STOP, infos, DAC_TRACE_PARAM_STOP_NUM);
        PlatformTraceStop();
        PlatformTraceInfoDump();
    }
    DacDeviceUnlock(device);
    return ret;
}

static int32_t DacManagerIoOpen(struct HdfSBuf *data, struct HdfSBuf *reply)
{
    uint32_t number;

    if (data == NULL || reply == NULL) {
        HDF_LOGE("DacManagerIoOpen: invalid data or reply!");
        return HDF_ERR_INVALID_PARAM;
    }

    if (!HdfSbufReadUint32(data, &number)) {
        HDF_LOGE("DacManagerIoOpen: read number fail!");
        return HDF_ERR_IO;
    }

    if (number >= DAC_DEVICES_MAX) {
        HDF_LOGE("DacManagerIoOpen: invalid number %u!", number);
        return HDF_ERR_INVALID_PARAM;
    }

    if (DacDeviceOpen(number) == NULL) {
        HDF_LOGE("DacManagerIoOpen: get device %u fail!", number);
        return HDF_ERR_NOT_SUPPORT;
    }

    number = (uint32_t)(number + DAC_HANDLE_SHIFT);
    if (!HdfSbufWriteUint32(reply, (uint32_t)(uintptr_t)number)) {
        HDF_LOGE("DacManagerIoOpen: write number fail!");
        return HDF_ERR_IO;
    }
    return HDF_SUCCESS;
}

static int32_t DacManagerIoClose(struct HdfSBuf *data, struct HdfSBuf *reply)
{
    uint32_t number;

    (void)reply;
    if (data == NULL) {
        HDF_LOGE("DacManagerIoClose: invalid data!");
        return HDF_ERR_INVALID_PARAM;
    }

    if (!HdfSbufReadUint32(data, &number)) {
        HDF_LOGE("DacManagerIoClose: read number fail!");
        return HDF_ERR_IO;
    }

    number  = (uint32_t)(number - DAC_HANDLE_SHIFT);
    if (number >= DAC_DEVICES_MAX) {
        HDF_LOGE("DacManagerIoClose: invalid number %u!", number);
        return HDF_ERR_INVALID_PARAM;
    }

    DacDeviceClose(DacDeviceGet(number));
    return HDF_SUCCESS;
}

static int32_t DacManagerIoWrite(struct HdfSBuf *data, struct HdfSBuf *reply)
{
    int32_t ret;
    uint32_t channel;
    uint32_t val;
    uint32_t number;

    (void)reply;
    if (data == NULL) {
        HDF_LOGE("DacManagerIoWrite: invalid data!");
        return HDF_ERR_INVALID_PARAM;
    }

    if (!HdfSbufReadUint32(data, &number)) {
        HDF_LOGE("DacManagerIoWrite: read number fail!");
        return HDF_ERR_IO;
    }

    number = (uint32_t)(number - DAC_HANDLE_SHIFT);
    if (number >= DAC_DEVICES_MAX) {
        HDF_LOGE("DacManagerIoWrite: invalid number %u!", number);
        return HDF_ERR_INVALID_PARAM;
    }

    if (!HdfSbufReadUint32(data, &channel)) {
        HDF_LOGE("DacManagerIoWrite: read dac channel fail");
        return HDF_ERR_IO;
    }

    if (!HdfSbufReadUint32(data, &val)) {
        HDF_LOGE("DacManagerIoWrite: read dac value fail!");
        return HDF_ERR_IO;
    }

    ret = DacDeviceWrite(DacDeviceGet(number), channel, val);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("DacManagerIoWrite: write dac fail, ret: %d!", ret);
        return ret;
    }

    return HDF_SUCCESS;
}

static int32_t DacManagerDispatch(struct HdfDeviceIoClient *client, int cmd,
    struct HdfSBuf *data, struct HdfSBuf *reply)
{
    (void)client;
    switch (cmd) {
        case DAC_IO_OPEN:
            return DacManagerIoOpen(data, reply);
        case DAC_IO_CLOSE:
            return DacManagerIoClose(data, reply);
        case DAC_IO_WRITE:
            return DacManagerIoWrite(data, reply);
        default:
            HDF_LOGE("DacManagerDispatch: cmd %d is not support!", cmd);
            return HDF_ERR_NOT_SUPPORT;
    }
    return HDF_SUCCESS;
}
static int32_t DacManagerBind(struct HdfDeviceObject *device)
{
    (void)device;
    return HDF_SUCCESS;
}
static int32_t DacManagerInit(struct HdfDeviceObject *device)
{
    int32_t ret;
    struct DacManager *manager = NULL;

    if (device == NULL) {
        HDF_LOGE("DacManagerInit: device is null!");
        return HDF_ERR_INVALID_OBJECT;
    }

    manager = (struct DacManager *)OsalMemCalloc(sizeof(*manager));
    if (manager == NULL) {
        HDF_LOGE("DacManagerInit: memcalloc manager fail!");
        return HDF_ERR_MALLOC_FAIL;
    }

    ret = OsalSpinInit(&manager->spin);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("DacManagerInit: spinlock init fail!");
        OsalMemFree(manager);
        return ret;
    }

    manager->device = device;
    g_dacManager = manager;
    device->service = &manager->service;
    device->service->Dispatch = DacManagerDispatch;
    return HDF_SUCCESS;
}

static void DacManagerRelease(struct HdfDeviceObject *device)
{
    struct DacManager *manager = NULL;

    if (device == NULL) {
        HDF_LOGE("DacManagerRelease: device is null!");
        return;
    }

    manager = (struct DacManager *)device->service;
    if (manager == NULL) {
        HDF_LOGI("DacManagerRelease: no service bind!");
        return;
    }

    g_dacManager = NULL;
    OsalMemFree(manager);
}

struct HdfDriverEntry g_dacManagerEntry = {
    .moduleVersion = 1,
    .Bind = DacManagerBind,
    .Init = DacManagerInit,
    .Release = DacManagerRelease,
    .moduleName = "HDF_PLATFORM_DAC_MANAGER",
};
HDF_INIT(g_dacManagerEntry);
