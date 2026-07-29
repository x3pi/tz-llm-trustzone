/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include <dlfcn.h>
#include <unistd.h>
#include "hdf_log.h"
#include "osal_mem.h"
#include "securec.h"
#include "hdf_load_vdi.h"

#define HDF_LOG_TAG dev_load_vdi

#ifdef __ARCH64__
#define VDI_PATH HDF_LIBRARY_DIR"64/"
#else
#define VDI_PATH HDF_LIBRARY_DIR"/"
#endif

struct HdfVdiObject *HdfLoadVdi(const char *libName)
{
    char path[PATH_MAX + 1] = {0};
    char resolvedPath[PATH_MAX + 1] = {0};

    if (libName == NULL) {
        HDF_LOGE("%{public}s libName is NULL", __func__);
        return NULL;
    }

    if (snprintf_s(path, sizeof(path), sizeof(path) - 1, "%s/%s", VDI_PATH, libName) < 0) {
        HDF_LOGE("%{public}s %{public}s snprintf_s failed", __func__, libName);
        return NULL;
    }

    if (realpath(path, resolvedPath) == NULL || strncmp(resolvedPath, VDI_PATH, strlen(VDI_PATH)) != 0) {
        HDF_LOGE("%{public}s %{public}s %{public}s realpath file name failed %{public}d",
            __func__, path, resolvedPath, errno);
        return NULL;
    }

    struct HdfVdiObject *vdiObj = (struct HdfVdiObject *)OsalMemCalloc(sizeof(*vdiObj));
    if (vdiObj == NULL) {
        HDF_LOGE("%{public}s malloc failed", __func__);
        return NULL;
    }

    void *handler = dlopen(resolvedPath, RTLD_LAZY);
    if (handler == NULL) {
        HDF_LOGE("%{public}s dlopen failed %{public}s", __func__, dlerror());
        OsalMemFree(vdiObj);
        return NULL;
    }

    struct HdfVdiBase **vdiBase = (struct HdfVdiBase **)dlsym(handler, "hdfVdiDesc");
    if (vdiBase == NULL || *vdiBase == NULL) {
        HDF_LOGE("%{public}s dlsym hdfVdiDesc failed %{public}s", __func__, dlerror());
        dlclose(handler);
        OsalMemFree(vdiObj);
        return NULL;
    }

    if ((*vdiBase)->CreateVdiInstance) {
        (*vdiBase)->CreateVdiInstance(*vdiBase);
    }

    vdiObj->dlHandler = (uintptr_t)handler;
    vdiObj->vdiBase = *vdiBase;

    return vdiObj;
}

uint32_t HdfGetVdiVersion(const struct HdfVdiObject *vdiObj)
{
    if (vdiObj == NULL || vdiObj->vdiBase == NULL) {
        HDF_LOGE("%{public}s para is invalid", __func__);
        return HDF_INVALID_VERSION;
    }

    return vdiObj->vdiBase->moduleVersion;
}

void HdfCloseVdi(struct HdfVdiObject *vdiObj)
{
    if (vdiObj == NULL || vdiObj->dlHandler == 0 || vdiObj->vdiBase == NULL) {
        HDF_LOGE("%{public}s para invalid", __func__);
        return;
    }

    struct HdfVdiBase *vdiBase = vdiObj->vdiBase;
    if (vdiBase->DestoryVdiInstance) {
        vdiBase->DestoryVdiInstance(vdiBase);
    }

    dlclose((void *)vdiObj->dlHandler);
    vdiObj->dlHandler = 0;
    vdiObj->vdiBase = NULL;
    OsalMemFree(vdiObj);
}

