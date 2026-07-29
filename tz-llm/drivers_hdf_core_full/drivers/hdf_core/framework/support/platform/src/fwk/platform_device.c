/*
 * Copyright (c) 2020-2023 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include "platform_device.h"
#include "hdf_log.h"
#include "osal_mem.h"
#include "platform_core.h"
#include "securec.h"
#include "stdarg.h"

#define PLATFORM_DEV_NAME_DEFAULT "platform_device"

static void PlatformDeviceOnFirstGet(struct HdfSRef *sref)
{
    (void)sref;
}

static void PlatformDeviceOnLastPut(struct HdfSRef *sref)
{
    struct PlatformDevice *device = NULL;
    int32_t ret;

    if (sref == NULL) {
        PLAT_LOGE("PlatformDeviceOnLastPut: sref is null!");
        return;
    }

    device = CONTAINER_OF(sref, struct PlatformDevice, ref);
    if (device == NULL) {
        PLAT_LOGE("PlatformDeviceOnLastPut: get device is null!");
        return;
    }

    ret = PlatformDevicePostEvent(device, PLAT_EVENT_DEVICE_DEAD);
    if (ret != HDF_SUCCESS) {
        PLAT_LOGE("PlatformDeviceOnLastPut: post event fail, ret: %d!", ret);
        return;
    }
    PLAT_LOGI("PlatformDeviceOnLastPut: device:%s(%d) life cycle end", device->name, device->number);
}

struct IHdfSRefListener g_platObjListener = {
    .OnFirstAcquire = PlatformDeviceOnFirstGet,
    .OnLastRelease = PlatformDeviceOnLastPut,
};

#define PLATFORM_DEV_NAME_LEN 64
int32_t PlatformDeviceSetName(struct PlatformDevice *device, const char *fmt, ...)
{
    int ret;
    char tmpName[PLATFORM_DEV_NAME_LEN + 1] = {0};
    size_t realLen;
    va_list vargs;
    char *realName = NULL;

    if (device == NULL) {
        PLAT_LOGE("PlatformDeviceSetName: device is null!");
        return HDF_ERR_INVALID_OBJECT;
    }

    if (fmt == NULL) {
        PLAT_LOGE("PlatformDeviceSetName: fmt is null!");
        return HDF_ERR_INVALID_PARAM;
    }

    va_start(vargs, fmt);
    ret = vsnprintf_s(tmpName, PLATFORM_DEV_NAME_LEN, PLATFORM_DEV_NAME_LEN, fmt, vargs);
    va_end(vargs);
    if (ret  <= 0) {
        PLAT_LOGE("PlatformDeviceSetName: fail to format device name, ret: %d!", ret);
        return HDF_PLT_ERR_OS_API;
    }

    realLen = strlen(tmpName);
    if (realLen == 0 || realLen > PLATFORM_DEV_NAME_LEN) {
        PLAT_LOGE("PlatformDeviceSetName: invalid name len:%zu!", realLen);
        return HDF_ERR_INVALID_PARAM;
    }
    realName = OsalMemCalloc(realLen + 1);
    if (realName == NULL) {
        PLAT_LOGE("PlatformDeviceSetName: memcalloc name mem fail!");
        return HDF_ERR_MALLOC_FAIL;
    }

    ret = strncpy_s(realName, realLen + 1, tmpName, realLen);
    if (ret != EOK) {
        OsalMemFree(realName);
        PLAT_LOGE("PlatformDeviceSetName: copy name(%s) fail, ret: %d!", tmpName, ret);
        return HDF_ERR_IO;
    }

    PLAT_LOGD("PlatformDeviceSetName: realName:%s, realLen:%zu!", realName, realLen);
    device->name = (const char *)realName;
    return HDF_SUCCESS;
}

void PlatformDeviceClearName(struct PlatformDevice *device)
{
    if (device != NULL && device->name != NULL) {
        OsalMemFree((char *)device->name);
        device->name = NULL;
    }
}

int32_t PlatformDeviceInit(struct PlatformDevice *device)
{
    int32_t ret;

    if (device == NULL) {
        PLAT_LOGE("PlatformDeviceInit: device is null!");
        return HDF_ERR_INVALID_OBJECT;
    }

    if ((ret = OsalSpinInit(&device->spin)) != HDF_SUCCESS) {
        PLAT_LOGE("PlatformDeviceInit: spin init fail!");
        return ret;
    }
    if ((ret = OsalSemInit(&device->released, 0)) != HDF_SUCCESS) {
        (void)OsalSpinDestroy(&device->spin);
        PLAT_LOGE("PlatformDeviceInit: sem init fail!");
        return ret;
    }
    if ((ret = PlatformEventInit(&device->event)) != HDF_SUCCESS) {
        (void)OsalSemDestroy(&device->released);
        (void)OsalSpinDestroy(&device->spin);
        PLAT_LOGE("PlatformDeviceInit: event init fail!");
        return ret;
    }
    DListHeadInit(&device->node);
    HdfSRefConstruct(&device->ref, &g_platObjListener);

    return HDF_SUCCESS;
}

void PlatformDeviceUninit(struct PlatformDevice *device)
{
    if (device == NULL) {
        PLAT_LOGW("PlatformDeviceUninit: device is null!");
        return;
    }

    (void)PlatformEventUninit(&device->event);
    (void)OsalSemDestroy(&device->released);
    (void)OsalSpinDestroy(&device->spin);
}

int32_t PlatformDeviceGet(struct PlatformDevice *device)
{
    if (device == NULL) {
        PLAT_LOGE("PlatformDeviceGet: device is null!");
        return HDF_ERR_INVALID_OBJECT;
    }

    HdfSRefAcquire(&device->ref);
    return HDF_SUCCESS;
}

void PlatformDevicePut(struct PlatformDevice *device)
{
    if (device != NULL) {
        HdfSRefRelease(&device->ref);
    }
}

int32_t PlatformDeviceRefCount(struct PlatformDevice *device)
{
    if (device != NULL) {
        return HdfSRefCount(&device->ref);
    }
    return HDF_ERR_INVALID_OBJECT;
}

int32_t PlatformDeviceAdd(struct PlatformDevice *device)
{
    int32_t ret;
    struct PlatformManager *manager = NULL;

    if (device == NULL) {
        PLAT_LOGE("PlatformDeviceAdd: device is null!");
        return HDF_ERR_INVALID_OBJECT;
    }

    manager = device->manager;
    if (manager == NULL) {
        manager = PlatformManagerGet(PLATFORM_MODULE_DEFAULT);
        if (manager == NULL) {
            PLAT_LOGE("PlatformDeviceAdd: get default manager fail!");
            return HDF_PLT_ERR_INNER;
        }
    }

    if ((ret = PlatformDeviceInit(device)) != HDF_SUCCESS) {
        return ret;
    }

    ret = PlatformManagerAddDevice(manager, device);
    if (ret == HDF_SUCCESS) {
        device->manager = manager;
        PLAT_LOGD("PlatformDeviceAdd: add dev:%s(%d) success", device->name, device->number);
    } else {
        PLAT_LOGE("PlatformDeviceAdd: add %s(%d) fail, ret: %d!", device->name, device->number, ret);
        PlatformDeviceUninit(device);
    }
    return ret;
}

void PlatformDeviceDel(struct PlatformDevice *device)
{
    int32_t ret;
    struct PlatformManager *manager = NULL;

    if (device == NULL) {
        PLAT_LOGE("PlatformDeviceDel: device is null!");
        return;
    }

    manager = device->manager;
    if (manager == NULL) {
        manager = PlatformManagerGet(PLATFORM_MODULE_DEFAULT);
        if (manager == NULL) {
            PLAT_LOGE("PlatformDeviceDel: get default manager fail!");
            return;
        }
    }
    ret = PlatformManagerDelDevice(manager, device);
    if (ret != HDF_SUCCESS) {
        PLAT_LOGW("PlatformDeviceDel: fail to remove device:%s from manager:%s!",
            device->name, manager->device.name);
        return; // it's dagerous to continue ...
    }

    /* make sure no reference anymore before exit. */
    ret = PlatformDeviceWaitEvent(device, PLAT_EVENT_DEVICE_DEAD, HDF_WAIT_FOREVER, NULL);
    if (ret == HDF_SUCCESS) {
        PLAT_LOGD("PlatformDeviceDel: remove dev:%s(%d) success", device->name, device->number);
    } else {
        PLAT_LOGE("PlatformDeviceDel: wait %s(%d) dead fail, ret: %d!", device->name, device->number, ret);
    }
    PlatformDeviceUninit(device);
}

int32_t PlatformDeviceCreateService(struct PlatformDevice *device,
    int32_t (*dispatch)(struct HdfDeviceIoClient *client, int cmd, struct HdfSBuf *data, struct HdfSBuf *reply))
{
    if (device == NULL) {
        PLAT_LOGE("PlatformDeviceCreateService: device is null!");
        return HDF_ERR_INVALID_OBJECT;
    }

    if (device->service != NULL) {
        PLAT_LOGE("PlatformDeviceCreateService: service already creatted!");
        return HDF_FAILURE;
    }

    device->service = (struct IDeviceIoService *)OsalMemCalloc(sizeof(*(device->service)));
    if (device->service == NULL) {
        PLAT_LOGE("PlatformDeviceCreateService: memcalloc service fail!");
        return HDF_ERR_MALLOC_FAIL;
    }

    device->service->Dispatch = dispatch;
    return HDF_SUCCESS;
}

void PlatformDeviceDestroyService(struct PlatformDevice *device)
{
    if (device == NULL) {
        PLAT_LOGE("PlatformDeviceDestroyService: device is null!");
        return;
    }

    if (device->service == NULL) {
        PLAT_LOGE("PlatformDeviceDestroyService: service is null!");
        return;
    }

    OsalMemFree(device->service);
    device->service = NULL;
}

int32_t PlatformDeviceSetHdfDev(struct PlatformDevice *device, struct HdfDeviceObject *hdfDevice)
{
    if (device == NULL || hdfDevice == NULL) {
        PLAT_LOGE("PlatformDeviceSetHdfDev: device or hdfDevice is null!");
        return HDF_ERR_INVALID_OBJECT;
    }

    if (device->hdfDev != NULL) {
        PLAT_LOGE("PlatformDeviceSetHdfDev: already bind a hdf device!");
        return HDF_FAILURE;
    }

    device->hdfDev = hdfDevice;
    hdfDevice->priv = device;
    return HDF_SUCCESS;
}

int32_t PlatformDeviceBind(struct PlatformDevice *device, struct HdfDeviceObject *hdfDevice)
{
    int32_t ret;

    ret = PlatformDeviceSetHdfDev(device, hdfDevice);
    if (ret == HDF_SUCCESS) {
        hdfDevice->service = device->service;
    }
    return ret;
}

void PlatformDeviceUnbind(struct PlatformDevice *device, const struct HdfDeviceObject *hdfDev)
{
    if (device == NULL) {
        PLAT_LOGE("PlatformDeviceUnbind: device is null!");
        return;
    }
    if (device->hdfDev == NULL) {
        PLAT_LOGE("PlatformDeviceUnbind: hdfDev is null!");
        return;
    }
    if (device->hdfDev != hdfDev) {
        PLAT_LOGW("PlatformDeviceUnbind: hdf device not match!");
        return;
    }

    device->hdfDev->service = NULL;
    device->hdfDev->priv = NULL;
    device->hdfDev = NULL;
}

struct PlatformDevice *PlatformDeviceFromHdfDev(const struct HdfDeviceObject *hdfDev)
{
    if (hdfDev == NULL || hdfDev->priv == NULL) {
        PLAT_LOGE("PlatformDeviceFromHdfDev: hdf device or priv is null!");
        return NULL;
    }

    return (struct PlatformDevice *)hdfDev->priv;
}

int32_t PlatformDevicePostEvent(struct PlatformDevice *device, uint32_t events)
{
    if (device == NULL) {
        PLAT_LOGE("PlatformDevicePostEvent: device is null!");
        return HDF_ERR_INVALID_OBJECT;
    }
    return PlatformEventPost(&device->event, events);
}

int32_t PlatformDeviceWaitEvent(struct PlatformDevice *device, uint32_t mask, uint32_t tms, uint32_t *events)
{
    int32_t ret;
    uint32_t eventsRead;

    if (device == NULL) {
        PLAT_LOGE("PlatformDeviceWaitEvent: device is null!");
        return HDF_ERR_INVALID_OBJECT;
    }
    ret = PlatformEventWait(&device->event, mask, PLAT_EVENT_MODE_CLEAR, tms, &eventsRead);
    if (ret != HDF_SUCCESS) {
        PLAT_LOGE("PlatformDeviceWaitEvent: event wait fail!");
        return ret;
    }

    if (eventsRead == 0) {
        PLAT_LOGE("PlatformDeviceWaitEvent: no event read!");
        return HDF_FAILURE;
    }
    if (events != NULL) {
        *events = eventsRead;
    }
    return HDF_SUCCESS;
}

int32_t PlatformDeviceListenEvent(struct PlatformDevice *device, struct PlatformEventListener *listener)
{
    if (device == NULL) {
        PLAT_LOGE("PlatformDeviceListenEvent: device is null!");
        return HDF_ERR_INVALID_OBJECT;
    }
    if (listener == NULL) {
        PLAT_LOGE("PlatformDeviceListenEvent: listener is null!");
        return HDF_ERR_INVALID_PARAM;
    }
    return PlatformEventListen(&device->event, listener);
}

void PlatformDeviceUnListenEvent(struct PlatformDevice *device, struct PlatformEventListener *listener)
{
    if (device != NULL && listener != NULL) {
        PlatformEventUnlisten(&device->event, listener);
    }
}

const struct DeviceResourceNode *PlatformDeviceGetDrs(const struct PlatformDevice *device)
{
#ifdef LOSCFG_DRIVERS_HDF_CONFIG_MACRO
    (void)device;
    return NULL;
#else
    if (device != NULL && device->hdfDev != NULL) {
        return device->hdfDev->property;
    }
    return NULL;
#endif
}
