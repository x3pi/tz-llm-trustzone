/*
 * Copyright (c) 2020-2023 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include "platform_manager.h"
#include "hdf_log.h"
#include "osal_mem.h"
#include "osal_sem.h"
#include "platform_core.h"

#define PLATFORM_MANAGER_NAME_DEFAULT "PlatformManagerDefault"

static inline void PlatformManagerLock(struct PlatformManager *manager)
{
    (void)OsalSpinLockIrqSave(&manager->device.spin, &manager->device.irqSave);
}

static inline void PlatformManagerUnlock(struct PlatformManager *manager)
{
    (void)OsalSpinUnlockIrqRestore(&manager->device.spin, &manager->device.irqSave);
}

static int32_t PlatformManagerInit(struct PlatformManager *manager)
{
    int32_t ret;

    DListHeadInit(&manager->devices);

    if ((ret = PlatformDeviceInit(&manager->device)) != HDF_SUCCESS) {
        PLAT_LOGE("PlatformManagerInit: device init fail!");
        return ret;
    }

    manager->add = NULL;
    manager->del = NULL;
    return HDF_SUCCESS;
}

static void PlatformManagerClearDevice(struct PlatformManager *manager)
{
    struct PlatformDevice *tmp = NULL;
    struct PlatformDevice *pos = NULL;

    if (manager == NULL) {
        PLAT_LOGE("PlatformManagerClearDevice: manager is null!");
        return;
    }

    PlatformManagerLock(manager);
    DLIST_FOR_EACH_ENTRY_SAFE(pos, tmp, &manager->devices, struct PlatformDevice, node) {
        DListRemove(&pos->node);
        PlatformDevicePut(pos);  // put the reference hold by manager
    }
    PlatformManagerUnlock(manager);
}

static void PlatformManagerUninit(struct PlatformManager *manager)
{
    PlatformManagerClearDevice(manager);
    PlatformDeviceUninit(&manager->device);
    manager->add = NULL;
    manager->del = NULL;
}

int32_t PlatformManagerCreate(const char *name, struct PlatformManager **manager)
{
    int32_t ret;
    struct PlatformManager *managerNew = NULL;

    managerNew = (struct PlatformManager *)OsalMemCalloc(sizeof(*managerNew));
    if (managerNew == NULL) {
        PLAT_LOGE("PlatformManagerCreate: malloc fail!");
        return HDF_ERR_MALLOC_FAIL;
    }
    managerNew->device.name = name;
    if ((ret = PlatformManagerInit(managerNew)) != HDF_SUCCESS) {
        OsalMemFree(managerNew);
        PLAT_LOGE("PlatformManagerCreate: platform manager init fail!");
        return ret;
    }
    *manager = managerNew;
    return ret;
}

void PlatformManagerDestroy(struct PlatformManager *manager)
{
    if (manager == NULL) {
        PLAT_LOGE("PlatformManagerDestroy: manager is null!");
        return;
    }
    PlatformManagerUninit(manager);
    OsalMemFree(manager);
}

struct PlatformManager *PlatformManagerGet(int module)
{
    int32_t ret;
    struct PlatformManager *manager = NULL;
    struct PlatformModuleInfo *info = NULL;

    info = PlatformModuleInfoGet(module);
    if (info == NULL) {
        PLAT_LOGE("PlatformManagerGet: get module(%d) info fail!", module);
        return NULL;
    }

    PlatformGlobalLock();
    if (info->priv == NULL) {
        ret = PlatformManagerCreate(info->moduleName, &manager);
        if (ret != HDF_SUCCESS) {
            PLAT_LOGE("PlatformManagerGet: create manager fail, ret: %d!", ret);
        } else {
            info->priv = manager;
        }
    } else {
        manager = (struct PlatformManager *)info->priv;
    }
    PlatformGlobalUnlock();

    return manager;
}

static int32_t PlatformManagerAddDeviceDefault(struct PlatformManager *manager, struct PlatformDevice *device)
{
    struct PlatformDevice *tmp = NULL;
    bool repeatId = false;
    bool repeatName = false;

    DLIST_FOR_EACH_ENTRY(tmp, &manager->devices, struct PlatformDevice, node) {
        if (device->number == tmp->number) {
            repeatId = true;
            PLAT_LOGE("PlatformManagerAddDeviceDefault: device:%s(%d) num repeated in manager:%s!",
                device->name, device->number, manager->device.name);
            break;
        }
        if (device->name != NULL && device->name == tmp->name) {
            repeatName = true;
            PLAT_LOGE("PlatformManagerAddDeviceDefault: device:%s(%d) name repeated in manager:%s!",
                device->name, device->number, manager->device.name);
            break;
        }
    }

    if (repeatId) {
        return HDF_PLT_ERR_ID_REPEAT;
    }
    if (repeatName) {
        return HDF_PLT_ERR_NAME_REPEAT;
    }

    DListInsertTail(&device->node, &manager->devices);
    return HDF_SUCCESS;
}

int32_t PlatformManagerAddDevice(struct PlatformManager *manager, struct PlatformDevice *device)
{
    int32_t ret;
    struct PlatformDevice *pos = NULL;

    if (manager == NULL || device == NULL) {
        PLAT_LOGE("PlatformManagerAddDevice: manager or device is null!");
        return HDF_ERR_INVALID_OBJECT;
    }

    if (PlatformDeviceGet(device) != HDF_SUCCESS) { // keep a reference by manager
        PLAT_LOGE("PlatformManagerAddDevice: get device fail!");
        return HDF_PLT_ERR_DEV_GET;
    }

    PlatformManagerLock(manager);
    DLIST_FOR_EACH_ENTRY(pos, &manager->devices, struct PlatformDevice, node) {
        if (pos == device) {
            PlatformManagerUnlock(manager);
            PLAT_LOGE("PlatformManagerAddDevice: device:%s(%d) already in manager:%s!",
                device->name, device->number, manager->device.name);
            PlatformDevicePut(device);
            return HDF_PLT_ERR_OBJ_REPEAT;
        }
    }
    if (manager->add != NULL) {
        ret = manager->add(manager, device);
    } else {
        ret = PlatformManagerAddDeviceDefault(manager, device);
    }
    PlatformManagerUnlock(manager);

    if (ret == HDF_SUCCESS) {
        PLAT_LOGD("PlatformManagerAddDevice: add dev:%s(%d) to %s success!",
            device->name, device->number, manager->device.name);
    } else {
        PlatformDevicePut(device);
        PLAT_LOGE("PlatformManagerAddDevice: add dev:%s(%d) to %s fail, ret:%d!",
            device->name, device->number, manager->device.name, ret);
    }

    return ret;
}

static int32_t PlatformManagerDelDeviceDefault(const struct PlatformManager *manager, struct PlatformDevice *device)
{
    if (manager == NULL || device == NULL) {
        PLAT_LOGE("PlatformManagerDelDeviceDefault: manager or device is null!");
        return HDF_ERR_INVALID_OBJECT;
    }
    if (!DListIsEmpty(&device->node)) {
        DListRemove(&device->node);
    } else {
        PLAT_LOGE("PlatformManagerDelDeviceDefault: device:%s already moved!", device->name);
        return HDF_ERR_INVALID_PARAM;
    }

    return HDF_SUCCESS;
}

int32_t PlatformManagerDelDevice(struct PlatformManager *manager, struct PlatformDevice *device)
{
    int32_t ret;
    struct PlatformDevice *pos = NULL;

    if (manager == NULL || device == NULL) {
        PLAT_LOGE("PlatformManagerDelDevice: manager or device is null!");
        return HDF_ERR_INVALID_OBJECT;
    }

    PlatformManagerLock(manager);
    DLIST_FOR_EACH_ENTRY(pos, &manager->devices, struct PlatformDevice, node) {
        if (pos == device) {
            break;
        }
    }
    if (pos != device) {
        PLAT_LOGE("PlatformManagerDelDevice: device:%s(%d) not in manager:%s!",
            device->name, device->number, manager->device.name);
        PlatformManagerUnlock(manager);
        return HDF_PLT_ERR_NO_DEV;
    }
    if (manager->del != NULL) {
        ret = manager->del(manager, device);
    } else {
        ret = PlatformManagerDelDeviceDefault(manager, device);
    }
    PlatformManagerUnlock(manager);

    if (ret == HDF_SUCCESS) {
        PlatformDevicePut(device);  // put the reference hold by manager
        PLAT_LOGD("PlatformManagerDelDevice: remove %s(%d) from %s success!",
            device->name, device->number, manager->device.name);
    } else {
        PLAT_LOGE("PlatformManagerDelDevice: remove %s(%d) from %s fail, ret:%d!",
            device->name, device->number, manager->device.name, ret);
    }
    return ret;
}

struct PlatformDevice *PlatformManagerFindDevice(struct PlatformManager *manager, void *data,
    bool (*match)(struct PlatformDevice *pdevice, void *data))
{
    struct PlatformDevice *tmp = NULL;
    struct PlatformDevice *pdevice = NULL;

    if (manager == NULL || match == NULL) {
        PLAT_LOGE("PlatformManagerFindDevice: manager or match is null!");
        return NULL;
    }
    if (manager->devices.prev == NULL || manager->devices.next == NULL) {
        PLAT_LOGE("PlatformManagerFindDevice: devices not init!");
        return NULL;
    }

    PlatformManagerLock(manager);
    DLIST_FOR_EACH_ENTRY(tmp, &manager->devices, struct PlatformDevice, node) {
        if (tmp == NULL || !match(tmp, data)) {
            continue;
        }
        if (PlatformDeviceGet(tmp) == HDF_SUCCESS) {
            pdevice = tmp;
        }
        break;
    }
    PlatformManagerUnlock(manager);

    return pdevice;
}

static bool PlatformDeviceMatchByNumber(struct PlatformDevice *device, void *data)
{
    uint32_t number = (uint32_t)(uintptr_t)data;

    return (device != NULL && (uint32_t)device->number == number);
}

struct PlatformDevice *PlatformManagerGetDeviceByNumber(struct PlatformManager *manager, uint32_t number)
{
    if (manager == NULL) {
        PLAT_LOGE("PlatformManagerGetDeviceByNumber: manager is null!");
        return NULL;
    }
    return PlatformManagerFindDevice(manager, (void *)(uintptr_t)number, PlatformDeviceMatchByNumber);
}

static bool PlatformDeviceMatchByName(struct PlatformDevice *device, void *data)
{
    const char *name = (const char *)data;

    if (name == NULL || device->name == NULL) {
        PLAT_LOGE("PlatformDeviceMatchByName: name or device->name is null!");
        return false;
    }

    return (strcmp(name, device->name) == 0);
}

struct PlatformDevice *PlatformManagerGetDeviceByName(struct PlatformManager *manager, const char *name)
{
    if (manager == NULL) {
        PLAT_LOGE("PlatformManagerGetDeviceByName: manager is null!");
        return NULL;
    }
    return PlatformManagerFindDevice(manager, (void *)name, PlatformDeviceMatchByName);
}
