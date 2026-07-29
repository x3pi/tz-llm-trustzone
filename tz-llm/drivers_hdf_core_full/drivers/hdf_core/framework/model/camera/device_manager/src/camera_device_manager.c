/*
 * Copyright (c) 2022 - 2023 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include <securec.h>
#include <hdf_log.h>
#include <osal_mem.h>
#include "camera_config_parser.h"
#include "camera_device_manager.h"

#define HDF_LOG_TAG HDF_CAMERA_DEVICE_MANAGER

static struct CameraDeviceDriverFactory *g_cameraDeviceDriverFactory[DEVICE_DRIVER_MAX_NUM];
static struct CameraDevice *g_cameraDeviceTable[CAMERA_MAX_NUM];

static void ReleaseCameraDevice(struct CameraDevice *camDev)
{
    OsalMemFree(camDev);
    HDF_LOGI("%s: success!", __func__);
    return;
}

static bool FindAvailableIndex(uint32_t *index)
{
    uint32_t i;
    for (i = 0; i < CAMERA_MAX_NUM; i++) {
        if (g_cameraDeviceTable[i] == NULL) {
            *index = i;
            return true;
        }
    }
    return false;
}

static bool AddCameraDeviceToTable(uint32_t index, struct CameraDevice *cameraDevice)
{
    if (index >= CAMERA_MAX_NUM) {
        HDF_LOGE("%s: error: index out of range!", __func__);
        return false;
    }
    if (g_cameraDeviceTable[index] != NULL) {
        HDF_LOGE("%s: error: g_cameraDeviceTable[%{public}d] is already exist!", __func__, index);
        return false;
    }
    g_cameraDeviceTable[index] = cameraDevice;
    return true;
}

static void RemoveCameraDeviceFromTable(const struct CameraDevice *cameraDevice)
{
    int32_t i;
    for (i = 0; i < CAMERA_MAX_NUM; i++) {
        if (g_cameraDeviceTable[i] == cameraDevice) {
            g_cameraDeviceTable[i] = NULL;
            return;
        }
    }
    return;
}

int32_t CameraDeviceRelease(struct CameraDevice *camDev)
{
    if (camDev == NULL) {
        HDF_LOGI("%s: success: already deinit!", __func__);
        return HDF_SUCCESS;
    }
    RemoveCameraDeviceFromTable(camDev);
    OsalMemFree(camDev);
    return HDF_SUCCESS;
}

struct CameraDevice *CameraDeviceCreate(const char *deviceName, uint32_t len)
{
    struct CameraDevice *camDevice = NULL;
    uint32_t index = 0;

    if ((deviceName == NULL) || (strlen(deviceName) != len) || (strlen(deviceName) > DEVICE_NAME_SIZE - 1)) {
        HDF_LOGE("%s: fail: deviceName is null or len not right!", __func__);
        return NULL;
    }
    camDevice = (struct CameraDevice *)OsalMemCalloc(sizeof(struct CameraDevice));
    if (camDevice == NULL) {
        HDF_LOGE("%s: fail: OsalMemCalloc fail!", __func__);
        return NULL;
    }
    if (strcpy_s(camDevice->deviceName, DEVICE_NAME_SIZE, deviceName) != EOK) {
        HDF_LOGE("%s: fail: strcpy_s fail!", __func__);
        OsalMemFree(camDevice);
        return NULL;
    }
    if (FindAvailableIndex(&index)) {
        AddCameraDeviceToTable(index, camDevice);
    } else {
        ReleaseCameraDevice(camDevice);
        HDF_LOGE("%s: fail: Not extra table.", __func__);
        return NULL;
    }
    HDF_LOGI("%s: Init Camera Device success!", __func__);

    return camDevice;
}

struct CameraDevice *CameraDeviceGetByName(const char *deviceName)
{
    int32_t i;
    if (deviceName == NULL) {
        HDF_LOGE("%s: fail: deviceName is NULL.", __func__);
        return NULL;
    }

    for (i = 0; i < CAMERA_MAX_NUM; i++) {
        if (g_cameraDeviceTable[i] == NULL) {
            continue;
        }
        if (strcmp(g_cameraDeviceTable[i]->deviceName, deviceName) == 0) {
            return g_cameraDeviceTable[i];
        }
    }

    HDF_LOGE("%s: fail: %{public}s: deviceName not exist.", __func__, deviceName);
    return NULL;
}

struct CameraDeviceDriverFactory *CameraDeviceDriverFactoryGetByName(const char *deviceName)
{
    int32_t i;
    if (deviceName == NULL) {
        HDF_LOGE("%s: fail: deviceName is NULL", __func__);
        return NULL;
    }

    for (i = 0; i < DEVICE_DRIVER_MAX_NUM; i++) {
        if (g_cameraDeviceDriverFactory[i] == NULL || g_cameraDeviceDriverFactory[i]->deviceName == NULL) {
            continue;
        }
        struct CameraDeviceDriverFactory *factory = g_cameraDeviceDriverFactory[i];
        if (strcmp(factory->deviceName, deviceName) == 0) {
            return factory;
        }
    }
    HDF_LOGE("%s: fail: return NULL", __func__);
    return NULL;
}

int32_t CameraDeviceDriverFactoryRegister(struct CameraDeviceDriverFactory *obj)
{
    int32_t index;
    if (obj == NULL || obj->deviceName == NULL) {
        HDF_LOGE("%s: CameraDeviceDriverFactory obj is NULL", __func__);
        return HDF_ERR_INVALID_PARAM;
    }
    if (CameraDeviceDriverFactoryGetByName(obj->deviceName) != NULL) {
        HDF_LOGI("%s: devicedriver factory is already registered. name=%{public}s", __func__, obj->deviceName);
        return HDF_SUCCESS;
    }
    for (index = 0; index < DEVICE_DRIVER_MAX_NUM; index++) {
        if (g_cameraDeviceDriverFactory[index] == NULL) {
            g_cameraDeviceDriverFactory[index] = obj;
            HDF_LOGI("%s: device driver %{public}s registered.", __func__, obj->deviceName);
            return HDF_SUCCESS;
        }
    }
    HDF_LOGE("%s: Factory table is full", __func__);
    return HDF_FAILURE;
}

int32_t DeviceDriverManagerDeInit(void)
{
    int32_t cnt;
    for (cnt = 0; cnt < DEVICE_DRIVER_MAX_NUM; cnt++) {
        if (g_cameraDeviceDriverFactory[cnt] == NULL) {
            continue;
        }
        if (g_cameraDeviceDriverFactory[cnt]->releaseFactory != NULL) {
            g_cameraDeviceDriverFactory[cnt]->releaseFactory(g_cameraDeviceDriverFactory[cnt]);
        }
        g_cameraDeviceDriverFactory[cnt] = NULL;
    }
    return HDF_SUCCESS;
}

static struct CameraDeviceDriverManager g_deviceDriverManager = {
    .deviceFactoryInsts = g_cameraDeviceDriverFactory,
    .regDeviceDriverFactory = CameraDeviceDriverFactoryRegister,
    .getDeviceDriverFactoryByName = CameraDeviceDriverFactoryGetByName,
};

struct CameraDeviceDriverManager *CameraDeviceDriverManagerGet(void)
{
    return &g_deviceDriverManager;
}
