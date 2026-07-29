/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include "clock_core.h"
#include "hdf_device_desc.h"
#include "hdf_log.h"
#include "osal_mem.h"
#include "osal_spinlock.h"
#include "osal_time.h"
#include "platform_core.h"
#include "platform_trace.h"

#define HDF_LOG_TAG clock_core_c

struct ClockManager {
    struct IDeviceIoService service;
    struct HdfDeviceObject *device;
    struct ClockDevice *devices[CLOCK_DEVICES_MAX];
    OsalSpinlock spin;
};

static struct ClockManager *g_clockManager = NULL;

static int32_t ClockDeviceLockDefault(struct ClockDevice *device)
{
    if (device == NULL) {
        HDF_LOGE("ClockDeviceLockDefault: device is null!");
        return HDF_ERR_INVALID_OBJECT;
    }
    return OsalSpinLock(&device->spin);
}

static void ClockDeviceUnlockDefault(struct ClockDevice *device)
{
    if (device == NULL) {
        HDF_LOGE("ClockDeviceUnlockDefault: device is null!");
        return;
    }
    (void)OsalSpinUnlock(&device->spin);
}

static const struct ClockLockMethod g_clockLockOpsDefault = {
    .lock = ClockDeviceLockDefault,
    .unlock = ClockDeviceUnlockDefault,
};

static inline int32_t ClockDeviceLock(struct ClockDevice *device)
{
    if (device->lockOps == NULL || device->lockOps->lock == NULL) {
        HDF_LOGE("ClockDeviceLock: clock device lock is not support!");
        return HDF_ERR_NOT_SUPPORT;
    }
    return device->lockOps->lock(device);
}

static inline void ClockDeviceUnlock(struct ClockDevice *device)
{
    if (device->lockOps != NULL && device->lockOps->unlock != NULL) {
        device->lockOps->unlock(device);
    }
}

static int32_t ClockManagerAddDevice(struct ClockDevice *device)
{
    int32_t ret;
    struct ClockManager *manager = g_clockManager;

    if (device->deviceIndex >= CLOCK_DEVICES_MAX) {
        HDF_LOGE("ClockManagerAddDevice: deviceIndex:%u exceed!", device->deviceIndex);
        return HDF_ERR_INVALID_OBJECT;
    }

    if (manager == NULL) {
        HDF_LOGE("ClockManagerAddDevice: get clock manager fail!");
        return HDF_ERR_NOT_SUPPORT;
    }

    if (OsalSpinLockIrq(&manager->spin) != HDF_SUCCESS) {
        HDF_LOGE("ClockManagerAddDevice: lock clock manager fail!");
        return HDF_ERR_DEVICE_BUSY;
    }

    if (manager->devices[device->deviceIndex]) {
        HDF_LOGE("ClockManagerAddDevice: clock device num:%u already exits!", device->deviceIndex);
        ret = HDF_FAILURE;
    } else {
        manager->devices[device->deviceIndex] = device;
        HDF_LOGE("ClockManagerAddDevice: clock device num:%u  add success!", device->deviceIndex);
        ret = HDF_SUCCESS;
    }

    (void)OsalSpinUnlockIrq(&manager->spin);
    return ret;
}

static void ClockManagerRemoveDevice(struct ClockDevice *device)
{
    struct ClockManager *manager = g_clockManager;

    if (device->deviceIndex < 0 || device->deviceIndex >= CLOCK_DEVICES_MAX) {
        HDF_LOGE("ClockManagerRemoveDevice: invalid devNum:%u!", device->deviceIndex);
        return;
    }

    if (manager == NULL) {
        HDF_LOGE("ClockManagerRemoveDevice: get clock manager fail!");
        return;
    }

    if (OsalSpinLockIrq(&manager->spin) != HDF_SUCCESS) {
        HDF_LOGE("ClockManagerRemoveDevice: lock clock manager fail!");
        return;
    }

    if (manager->devices[device->deviceIndex] != device) {
        HDF_LOGE("ClockManagerRemoveDevice: clock device(%u) not in manager!", device->deviceIndex);
    } else {
        manager->devices[device->deviceIndex] = NULL;
    }

    (void)OsalSpinUnlockIrq(&manager->spin);
}

void ClockDeviceRemove(struct ClockDevice *device)
{
    if (device == NULL) {
        HDF_LOGE("ClockDeviceRemove: device is null!");
        return;
    }
    ClockManagerRemoveDevice(device);
    (void)OsalSpinDestroy(&device->spin);
}

static struct ClockDevice *ClockManagerFindDevice(uint32_t number)
{
    struct ClockDevice *device = NULL;
    struct ClockManager *manager = g_clockManager;

    if (number >= CLOCK_DEVICES_MAX) {
        HDF_LOGE("ClockManagerFindDevice: invalid devNum:%u!", number);
        return NULL;
    }

    if (manager == NULL) {
        HDF_LOGE("ClockManagerFindDevice: get clock manager fail!");
        return NULL;
    }

    if (OsalSpinLockIrq(&manager->spin) != HDF_SUCCESS) {
        HDF_LOGE("ClockManagerFindDevice: lock clock manager fail!");
        return NULL;
    }

    device = manager->devices[number];
    (void)OsalSpinUnlockIrq(&manager->spin);

    return device;
}

int32_t ClockDeviceAdd(struct ClockDevice *device)
{
    int32_t ret;
    if (device == NULL) {
        HDF_LOGE("ClockDeviceAdd: device is null!");
        return HDF_ERR_INVALID_OBJECT;
    }

    if (device->ops == NULL) {
        HDF_LOGE("ClockDeviceAdd: no ops supplied!");
        return HDF_ERR_INVALID_OBJECT;
    }

    if (device->lockOps == NULL) {
        HDF_LOGI("ClockDeviceAdd: use default lockOps!");
        device->lockOps = &g_clockLockOpsDefault;
    }

    if (OsalSpinInit(&device->spin) != HDF_SUCCESS) {
        HDF_LOGE("ClockDeviceAdd: init lock fail!");
        return HDF_FAILURE;
    }

    ret = ClockManagerAddDevice(device);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("ClockDeviceAdd: clock manager add device fail!");
        (void)OsalSpinDestroy(&device->spin);
    }
    return ret;
}

int32_t ClockManagerGetAIdleDeviceId(void)
{
    int32_t ret = -1;
    int32_t id;
    struct ClockManager *manager = g_clockManager;

    if (manager == NULL) {
        HDF_LOGE("ClockManagerAddDevice: get clock manager fail!");
        return HDF_ERR_NOT_SUPPORT;
    }

    if (OsalSpinLockIrq(&manager->spin) != HDF_SUCCESS) {
        HDF_LOGE("ClockManagerAddDevice: lock clock manager fail!");
        return HDF_ERR_DEVICE_BUSY;
    }

    for (id = 0; id < CLOCK_DEVICES_MAX; id++) {
        if (manager->devices[id] == NULL) {
            ret = id;
            break;
        }
    }

    (void)OsalSpinUnlockIrq(&manager->spin);
    return ret;
}

struct ClockDevice *ClockDeviceGet(uint32_t number)
{
    return ClockManagerFindDevice(number);
}

int32_t ClockDeviceStart(struct ClockDevice *device)
{
    int32_t ret;

    if (device == NULL) {
        HDF_LOGE("ClockDeviceStart: device is null!");
        return HDF_ERR_INVALID_OBJECT;
    }

    if (device->ops == NULL || device->ops->start == NULL) {
        HDF_LOGE("ClockDeviceStart: ops or start is null!");
        return HDF_ERR_NOT_SUPPORT;
    }

    if (ClockDeviceLock(device) != HDF_SUCCESS) {
        HDF_LOGE("ClockDeviceStart: lock clock device fail!");
        return HDF_ERR_DEVICE_BUSY;
    }

    ret = device->ops->start(device);

    ClockDeviceUnlock(device);
    return ret;
}

int32_t ClockDeviceStop(struct ClockDevice *device)
{
    int32_t ret;

    if (device == NULL) {
        HDF_LOGE("ClockDeviceStart: device is null!");
        return HDF_ERR_INVALID_OBJECT;
    }

    if (device->ops == NULL || device->ops->stop == NULL) {
        HDF_LOGE("ClockDeviceStart: ops or start is null!");
        return HDF_ERR_NOT_SUPPORT;
    }

    if (ClockDeviceLock(device) != HDF_SUCCESS) {
        HDF_LOGE("ClockDeviceStart: lock clock device fail!");
        return HDF_ERR_DEVICE_BUSY;
    }

    ret = device->ops->stop(device);

    ClockDeviceUnlock(device);
    return ret;
}

struct ClockDevice *ClockDeviceOpen(uint32_t number)
{
    int32_t ret;
    struct ClockDevice *device = NULL;

    device = ClockDeviceGet(number);
    if (device == NULL) {
        HDF_LOGE("ClockDeviceOpen: get clock device fail!");
        return NULL;
    }

    ret = ClockDeviceStart(device);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("ClockDeviceOpen: start clock device fail!");
        return NULL;
    }

    return device;
}

int32_t ClockDeviceClose(struct ClockDevice *device)
{
    int32_t ret;

    if (device == NULL) {
        HDF_LOGE("ClockDeviceClose: get clock device fail!");
        return NULL;
    }
    ret = ClockDeviceStop(device);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("ClockDeviceClose: start clock device fail!");
        return NULL;
    }

    return HDF_SUCCESS;
}

int32_t ClockDeviceSetRate(struct ClockDevice *device, uint32_t rate)
{
    int32_t ret;

    if (device == NULL) {
        HDF_LOGE("ClockDeviceSetRate: device is null!");
        return HDF_ERR_INVALID_OBJECT;
    }

    if (device->ops == NULL || device->ops->setRate == NULL) {
        HDF_LOGE("ClockDeviceSetRate: ops or start is null!");
        return HDF_ERR_NOT_SUPPORT;
    }

    if (ClockDeviceLock(device) != HDF_SUCCESS) {
        HDF_LOGE("ClockDeviceSetRate: lock clock device fail!");
        return HDF_ERR_DEVICE_BUSY;
    }

    ret = device->ops->setRate(device, rate);

    ClockDeviceUnlock(device);
    return ret;
}

int32_t ClockDeviceGetRate(struct ClockDevice *device, uint32_t *rate)
{
    int32_t ret;
    if (device == NULL) {
        HDF_LOGE("ClockDeviceGetRate: device is null!");
        return HDF_ERR_INVALID_OBJECT;
    }

    if (device->ops == NULL || device->ops->getRate == NULL) {
        HDF_LOGE("ClockDeviceGetRate: ops or start is null!");
        return HDF_ERR_NOT_SUPPORT;
    }

    if (ClockDeviceLock(device) != HDF_SUCCESS) {
        HDF_LOGE("ClockDeviceGetRate: lock clock device fail!");
        return HDF_ERR_DEVICE_BUSY;
    }

    ret = device->ops->getRate(device, rate);
    ClockDeviceUnlock(device);
    return ret;
}

struct ClockDevice *ClockDeviceGetParent(struct ClockDevice *device)
{
    struct ClockDevice *parent = NULL;

    if (device == NULL) {
        HDF_LOGE("ClockDeviceGetParent: device is null!");
        return NULL;
    }

    if (device->ops == NULL || device->ops->getParent == NULL) {
        HDF_LOGE("ClockDeviceGetParent: ops or getParent is null!");
        return NULL;
    }

    if (ClockDeviceLock(device) != HDF_SUCCESS) {
        HDF_LOGE("ClockDeviceGetParent: lock clock device fail!");
        return NULL;
    }

    parent = device->ops->getParent(device);

    ClockDeviceUnlock(device);
    return parent;
}

int32_t ClockDeviceSetParent(struct ClockDevice *device, struct ClockDevice *parent)
{
    int32_t ret;

    if (device == NULL || parent == NULL) {
        HDF_LOGE("ClockDeviceSetParent: device or is parent null!");
        return HDF_ERR_INVALID_OBJECT;
    }

    if (device->ops == NULL || device->ops->setParent == NULL) {
        HDF_LOGE("ClockDeviceSetParent: ops or setParent is null!");
        return HDF_ERR_NOT_SUPPORT;
    }

    if (ClockDeviceLock(device) != HDF_SUCCESS) {
        HDF_LOGE("ClockDeviceSetParent: lock clock device fail!");
        return HDF_ERR_DEVICE_BUSY;
    }

    if (ClockDeviceLock(parent) != HDF_SUCCESS) {
        ClockDeviceUnlock(device);
        HDF_LOGE("ClockDeviceGetParent: lock clock device fail!");
        return HDF_ERR_DEVICE_BUSY;
    }
    ret = device->ops->setParent(device, parent);

    ClockDeviceUnlock(device);
    ClockDeviceUnlock(parent);
    return ret;
}

int32_t ClockDeviceDisable(struct ClockDevice *device)
{
    int32_t ret;

    if (device == NULL) {
        HDF_LOGE("ClockDeviceDisable: device is null!");
        return HDF_ERR_INVALID_OBJECT;
    }

    if (device->ops == NULL || device->ops->disable == NULL) {
        HDF_LOGE("ClockDeviceDisable: ops or start is null!");
        return HDF_ERR_NOT_SUPPORT;
    }

    if (ClockDeviceLock(device) != HDF_SUCCESS) {
        HDF_LOGE("ClockDeviceDisable: lock clock device fail!");
        return HDF_ERR_DEVICE_BUSY;
    }

    ret = device->ops->disable(device);

    ClockDeviceUnlock(device);
    return ret;
}

int32_t ClockDeviceEnable(struct ClockDevice *device)
{
    int32_t ret;

    if (device == NULL) {
        HDF_LOGE("ClockDeviceEnable: device is null!");
        return HDF_ERR_INVALID_OBJECT;
    }

    if (device->ops == NULL || device->ops->enable == NULL) {
        HDF_LOGE("ClockDeviceEnable: ops or start is null!");
        return HDF_ERR_NOT_SUPPORT;
    }

    if (ClockDeviceLock(device) != HDF_SUCCESS) {
        HDF_LOGE("ClockDeviceEnable: lock clock device fail!");
        return HDF_ERR_DEVICE_BUSY;
    }

    ret = device->ops->enable(device);

    ClockDeviceUnlock(device);
    return ret;
}

static int32_t ClockManagerOpen(struct HdfSBuf *data, struct HdfSBuf *reply)
{
    uint32_t number = 0;
    int32_t ret = HDF_SUCCESS;

    if (data == NULL || reply == NULL) {
        HDF_LOGE("ClockManagerOpen: invalid data or reply!");
        return HDF_ERR_INVALID_PARAM;
    }

    if (!HdfSbufReadUint32(data, &number)) {
        HDF_LOGE("ClockManagerOpen: fail to read number!\n");
        return HDF_ERR_INVALID_PARAM;
    }

    if (number >= CLOCK_DEVICES_MAX) {
        HDF_LOGE("ClockManagerOpen: invalid number!");
        return HDF_ERR_INVALID_PARAM;
    }

    if (ClockDeviceOpen(number) == NULL) {
        HDF_LOGE("ClockManagerOpen: get device %u fail!", number);
        return HDF_ERR_NOT_SUPPORT;
    }

    if (!HdfSbufWriteUint32(reply, number + CLOCK_HANDLE_SHIFT)) {
        HDF_LOGE("ClockManagerOpen:  fail to write number!\n");
        return HDF_ERR_INVALID_PARAM;
    }

    return ret;
}

static int32_t ClockManagerClose(struct HdfSBuf *data, struct HdfSBuf *reply)
{
    uint32_t number = 0;
    int32_t ret;
    struct ClockDevice *device = NULL;

    if (data == NULL || reply == NULL) {
        HDF_LOGE("ClockManagerClose: invalid data or reply!");
        return HDF_ERR_INVALID_PARAM;
    }

    if (!HdfSbufReadUint32(data, &number)) {
        HDF_LOGE("ClockManagerClose: fail to read handle!\n");
        return HDF_ERR_INVALID_PARAM;
    }

    number -= CLOCK_HANDLE_SHIFT;
    if (number >= CLOCK_DEVICES_MAX) {
        HDF_LOGE("ClockManagerClose: invalid number!");
        return HDF_ERR_INVALID_PARAM;
    }

    device = ClockDeviceGet(number);
    if (device == NULL) {
        HDF_LOGE("ClockManagerEnable: get clock device fail!");
        return HDF_ERR_INVALID_PARAM;
    }

    ret = ClockDeviceClose(device);
    return ret;
}

static int32_t ClockManagerEnable(struct HdfSBuf *data)
{
    uint32_t number = 0;
    int32_t ret;
    struct ClockDevice *device = NULL;

    if (data == NULL) {
        HDF_LOGE("ClockManagerEnable: invalid data!");
        return HDF_ERR_INVALID_PARAM;
    }
    if (!HdfSbufReadUint32(data, &number)) {
        HDF_LOGE("ClockManagerEnable:  fail to read number!\n");
        return HDF_ERR_INVALID_PARAM;
    }

    number -= CLOCK_HANDLE_SHIFT;
    if (number >= CLOCK_DEVICES_MAX) {
        HDF_LOGE("ClockManagerEnable: invalid number!");
        return HDF_ERR_INVALID_PARAM;
    }

    device = ClockDeviceGet(number);
    if (device == NULL) {
        HDF_LOGE("ClockManagerEnable: get clock device fail!");
        return HDF_ERR_INVALID_PARAM;
    }

    ret = ClockDeviceEnable(device);
    return ret;
}

static int32_t ClockManagerDisable(struct HdfSBuf *data)
{
    uint32_t number = 0;
    int32_t ret;
    struct ClockDevice *device = NULL;

    if (data == NULL) {
        HDF_LOGE("ClockManagerDisable: invalid data!");
        return HDF_ERR_INVALID_PARAM;
    }
    if (!HdfSbufReadUint32(data, &number)) {
        HDF_LOGE("ClockManagerDisable:  fail to read number!\n");
        return HDF_ERR_INVALID_PARAM;
    }

    number -= CLOCK_HANDLE_SHIFT;
    if (number >= CLOCK_DEVICES_MAX) {
        HDF_LOGE("ClockManagerDisable: invalid number!");
        return HDF_ERR_INVALID_PARAM;
    }

    device = ClockDeviceGet(number);
    if (device == NULL) {
        HDF_LOGE("ClockManagerDisable: get clock device fail!");
        return HDF_ERR_INVALID_PARAM;
    }
    ret = ClockDeviceDisable(device);
    return ret;
}

static int32_t ClockManagerSetRate(struct HdfSBuf *data, struct HdfSBuf *reply)
{
    uint32_t number = 0;
    uint32_t rate = 0;
    int32_t ret;
    struct ClockDevice *device = NULL;
    HDF_LOGE("func = %s line = %d", __FUNCTION__, __LINE__);
    if (data == NULL || reply == NULL) {
        HDF_LOGE("ClockManagerSetRate: invalid data or reply!");
        return HDF_ERR_INVALID_PARAM;
    }

    if (!HdfSbufReadUint32(data, &number)) {
        HDF_LOGE("ClockManagerSetRate:  fail to read handle!\n");
        return HDF_ERR_INVALID_PARAM;
    }

    number -= CLOCK_HANDLE_SHIFT;
    if (number >= CLOCK_DEVICES_MAX) {
        HDF_LOGE("ClockManagerSetRate: invalid number!");
        return HDF_ERR_INVALID_PARAM;
    }

    if (!HdfSbufReadUint32(data, &rate)) {
        HDF_LOGE("ClockManagerSetRate:  fail to read rate!\n");
        return HDF_ERR_INVALID_PARAM;
    }

    device = ClockDeviceGet(number);
    if (device == NULL) {
        HDF_LOGE("ClockManagerSetRate: get clock device fail!");
        return HDF_ERR_INVALID_PARAM;
    }

    ret = ClockDeviceSetRate(device, rate);
    return ret;
}

static int32_t ClockManagerGetRate(struct HdfSBuf *data, struct HdfSBuf *reply)
{
    uint32_t number = 0;
    uint32_t rate = 0;
    struct ClockDevice *device = NULL;

    if (data == NULL || reply == NULL) {
        HDF_LOGE("ClockManagerSetRate: invalid data or reply!");
        return HDF_ERR_INVALID_PARAM;
    }

    if (!HdfSbufReadUint32(data, &number)) {
        HDF_LOGE("ClockManagerGetRate:  fail to read handle!\n");
        return HDF_ERR_INVALID_PARAM;
    }

    number -= CLOCK_HANDLE_SHIFT;
    if (number >= CLOCK_DEVICES_MAX) {
        HDF_LOGE("ClockManagerGetRate: invalid number!");
        return HDF_ERR_INVALID_PARAM;
    }

    device = ClockDeviceGet(number);
    if (device == NULL) {
        HDF_LOGE("ClockManagerSetRate: get clock device fail!");
        return HDF_ERR_INVALID_PARAM;
    }
    ClockDeviceGetRate(device, &rate);

    if (!HdfSbufWriteUint32(reply, rate)) {
        HDF_LOGE("ClockManagerGetRate:  fail to write rate!\n");
        return HDF_ERR_INVALID_PARAM;
    }
    HDF_LOGI("ClockManagerGetRate: get rate = %u !", rate);
    return HDF_SUCCESS;
}

static int32_t ClockManagerGetParent(struct HdfSBuf *data, struct HdfSBuf *reply)
{
    uint32_t number = 0;
    struct ClockDevice *device = NULL;
    struct ClockDevice *parent = NULL;

    if (reply == NULL) {
        HDF_LOGE("ClockManagerGetParent: invalid reply!\n");
        return HDF_ERR_INVALID_PARAM;
    }

    if (!HdfSbufReadUint32(data, &number)) {
        HDF_LOGE("ClockManagerGetParent:  fail to read number!\n");
        return HDF_ERR_INVALID_PARAM;
    }

    number -= CLOCK_HANDLE_SHIFT;
    if (number >= CLOCK_DEVICES_MAX) {
        HDF_LOGE("ClockManagerGetParent: invalid number!");
        return HDF_ERR_INVALID_PARAM;
    }

    device = ClockDeviceGet(number);
    if (device == NULL) {
        HDF_LOGE("ClockManagerGetParent: get clock device fail!");
        return HDF_ERR_INVALID_PARAM;
    }

    parent = ClockDeviceGetParent(device);
    if (parent == NULL) {
        HDF_LOGE("ClockManagerGetParent: get clock device fail!");
        return HDF_ERR_INVALID_PARAM;
    }

    if (!HdfSbufWriteUint32(reply, parent->deviceIndex + CLOCK_HANDLE_SHIFT)) {
        HDF_LOGE("ClockManagerGetParent:  fail to write parent!\n");
        return HDF_ERR_INVALID_PARAM;
    }

    return HDF_SUCCESS;
}

static int32_t ClockManagerSetParent(struct HdfSBuf *data, struct HdfSBuf *reply)
{
    uint32_t number = 0;
    uint32_t number_p = 0;
    struct ClockDevice *device = NULL;
    struct ClockDevice *parent = NULL;
    int32_t ret;

    if (!HdfSbufReadUint32(data, &number)) {
        HDF_LOGE("ClockManagerSetParent: fail to read handle!\n");
        return HDF_ERR_INVALID_PARAM;
    }

    number -= CLOCK_HANDLE_SHIFT;
    if (number >= CLOCK_DEVICES_MAX) {
        HDF_LOGE("ClockManagerSetParent: invalid number!");
        return HDF_ERR_INVALID_PARAM;
    }

    if (!HdfSbufReadUint32(data, &number_p)) {
        HDF_LOGE("ClockManagerSetParent: fail to read parent!");
        return HDF_ERR_INVALID_PARAM;
    }

    number_p -= CLOCK_HANDLE_SHIFT;
    if (number_p >= CLOCK_DEVICES_MAX) {
        HDF_LOGE("ClockManagerSetParent: invalid number_p!");
        return HDF_ERR_INVALID_PARAM;
    }

    device = ClockDeviceGet(number);
    if (device == NULL) {
        HDF_LOGE("ClockManagerSetParent: get clock device fail!");
        return HDF_ERR_INVALID_PARAM;
    }

    parent = ClockDeviceGet(number_p);
    if (parent == NULL) {
        HDF_LOGE("ClockManagerSetParent: get clock device fail!");
        return HDF_ERR_INVALID_PARAM;
    }

    ret = ClockDeviceSetParent(device, parent);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("ClockManagerSetParent: number_p =%u , number =%u ", number_p, number);
    }

    return ret;
}

static int32_t ClockManagerDispatch(struct HdfDeviceIoClient *client, int cmd, struct HdfSBuf *data,
                                    struct HdfSBuf *reply)
{
    if (data == NULL) {
        HDF_LOGE("ClockManagerDispatch: invalid data cmd =%d!", cmd);
        return HDF_ERR_INVALID_PARAM;
    }

    (void)client;
    switch (cmd) {
        case CLOCK_IO_OPEN:
            return ClockManagerOpen(data, reply);
            break;
        case CLOCK_IO_CLOSE:
            return ClockManagerClose(data, reply);
            break;
        case CLOCK_IO_ENABLE:
            return ClockManagerEnable(data);
            break;
        case CLOCK_IO_DISABLE:
            return ClockManagerDisable(data);
            break;
        case CLOCK_IO_SET_RATE:
            return ClockManagerSetRate(data, reply);
            break;
        case CLOCK_IO_GET_RATE:
            return ClockManagerGetRate(data, reply);
            break;
        case CLOCK_IO_SET_PARENT:
            return ClockManagerSetParent(data, reply);
            break;
        case CLOCK_IO_GET_PARENT:
            return ClockManagerGetParent(data, reply);
            break;
        default:
            HDF_LOGE("ClockManagerDispatch: cmd %d is not support!", cmd);
            return HDF_ERR_NOT_SUPPORT;
    }
    return HDF_SUCCESS;
}

static int32_t ClockManagerBind(struct HdfDeviceObject *device)
{
    (void)device;
    return HDF_SUCCESS;
}

static int32_t ClockManagerInit(struct HdfDeviceObject *device)
{
    int32_t ret;
    struct ClockManager *manager = NULL;

    if (device == NULL) {
        HDF_LOGE("ClockManagerInit: device is null!");
        return HDF_ERR_INVALID_OBJECT;
    }

    manager = (struct ClockManager *)OsalMemCalloc(sizeof(*manager));
    if (manager == NULL) {
        HDF_LOGE("ClockManagerInit: memcalloc for manager fail!");
        return HDF_ERR_MALLOC_FAIL;
    }

    ret = OsalSpinInit(&manager->spin);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("ClockManagerInit: spinlock init fail!");
        OsalMemFree(manager);
        return HDF_FAILURE;
    }

    manager->device = device;
    g_clockManager = manager;
    device->service = &manager->service;
    device->service->Dispatch = ClockManagerDispatch;
    return HDF_SUCCESS;
}

static void ClockManagerRelease(struct HdfDeviceObject *device)
{
    struct ClockManager *manager = NULL;

    if (device == NULL) {
        HDF_LOGE("ClockManagerRelease: device is null!");
        return;
    }

    manager = (struct ClockManager *)device->service;
    if (manager == NULL) {
        HDF_LOGE("ClockManagerRelease: no service bind!");
        return;
    }
    g_clockManager = NULL;
    OsalMemFree(manager);
}

struct HdfDriverEntry g_clockManagerEntry = {
    .moduleVersion = 1,
    .Bind = ClockManagerBind,
    .Init = ClockManagerInit,
    .Release = ClockManagerRelease,
    .moduleName = "HDF_PLATFORM_CLOCK_MANAGER",
};
HDF_INIT(g_clockManagerEntry);
