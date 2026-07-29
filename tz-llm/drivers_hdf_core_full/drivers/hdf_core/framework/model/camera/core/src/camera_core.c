/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include <securec.h>
#include <hdf_log.h>
#include <osal_mem.h>
#include <core/hdf_device_desc.h>
#include <camera/camera_product.h>
#include "camera_config_parser.h"
#include "camera_device_manager.h"
#include "camera_dispatch.h"
#include "camera_event.h"
#include "camera_utils.h"
#include "camera_core.h"

#define HDF_LOG_TAG HDF_CAMERA_CORE
bool g_hdfDeviceInited = false;

static int32_t HdfCameraGetConfig(const struct HdfDeviceObject *device)
{
    if (HdfParseCameraConfig(device->property) != HDF_SUCCESS) {
        HDF_LOGE("%s: HdfParseCameraConfig failed!", __func__);
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

static struct CameraDeviceDriverFactory *HdfCameraGetDriverFactory(const char *deviceName)
{
    struct CameraDeviceDriverManager *drvMgr = NULL;
    drvMgr = CameraDeviceDriverManagerGet();
    if (drvMgr == NULL) {
        HDF_LOGE("%s: drvMgr is NULL", __func__);
        return NULL;
    }
    return drvMgr->getDeviceDriverFactoryByName(deviceName);
}

static int32_t HdfCameraReleaseInterface(const char *deviceName, struct CameraDeviceDriverFactory *factory)
{
    int32_t ret;
    struct CameraDevice *camDev = NULL;
    struct CameraDeviceDriver *deviceDriver = NULL;

    camDev = CameraDeviceGetByName(deviceName);
    if (camDev == NULL) {
        HDF_LOGI("%s: CameraDevice already released! deviceName=%{public}s", __func__, deviceName);
        return HDF_SUCCESS;
    }
    deviceDriver = GetDeviceDriver(camDev);
    if (deviceDriver == NULL) {
        HDF_LOGE("%s: deviceDriver is NULL. deviceName=%{public}s", __func__, deviceName);
        return HDF_FAILURE;
    }
    ret = deviceDriver->deinit(deviceDriver, camDev);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: device driver deinit failed! deviceName=%{public}s, ret=%{public}d", __func__, deviceName, ret);
        return ret;
    }

    ret = CameraDeviceRelease(camDev);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: release cameradevice failed! deviceName=%{public}s, ret=%{public}d", __func__, deviceName, ret);
        return ret;
    }

    if (factory->release != NULL) {
        factory->release(deviceDriver);
    }
    deviceDriver = NULL;
    HDF_LOGI("%s: HdfCameraReleaseInterface successful", __func__);

    return HDF_SUCCESS;
}

static int32_t HdfCameraInitInterfaces(const char *deviceName, struct CameraDeviceDriverFactory *factory)
{
    int32_t ret;
    struct CameraDevice *camDev = NULL;
    struct CameraDeviceDriver *deviceDriver = NULL;

    deviceDriver = factory->build(deviceName);
    if (deviceDriver == NULL) {
        HDF_LOGE("%s: device driver %{public}s build fail!", __func__, factory->deviceName);
        return HDF_FAILURE;
    }

    if (deviceDriver->init == NULL) {
        HDF_LOGI("%s: device driver %{public}s not 'init' api", __func__, factory->deviceName);
        return HDF_DEV_ERR_OP;
    }

    camDev = CameraDeviceCreate(deviceName, strlen(deviceName));
    if (camDev == NULL) {
        HDF_LOGE("%s: allocate camera device failed!", __func__);
        return HDF_FAILURE;
    }

    camDev->deviceDriver = deviceDriver;

    ret = deviceDriver->init(deviceDriver, camDev);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: init device %{public}s failed! ret=%{public}d", __func__, factory->deviceName, ret);
        if (factory->release != NULL) {
            factory->release(deviceDriver);
        }
        deviceDriver = NULL;
        CameraDeviceRelease(camDev);
        camDev = NULL;
    }
    return ret;
}

static int32_t HdfCameraDeinitInterface(const char *deviceName, struct CameraDeviceDriverFactory *factory)
{
    int32_t ret;

    ret = HdfCameraReleaseInterface(deviceName, factory);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: Deinit camif failed! ret=%{public}d", __func__, ret);
    }

    HDF_LOGI("%s: camera deinit interface finished!", __func__);
    return ret;
}

int32_t HdfCameraDeinitDevice(const char *deviceName)
{
    struct CameraDeviceDriverFactory *factory = NULL;
    int32_t ret;
    if (deviceName == NULL) {
        HDF_LOGE("%s: Input param is null", __func__);
        return HDF_ERR_INVALID_PARAM;
    }
    factory = HdfCameraGetDriverFactory(deviceName);
    if (factory == NULL) {
        HDF_LOGE("%s: get factory failed! deviceName=%{public}s", __func__, deviceName);
        return HDF_FAILURE;
    }
    ret = HdfCameraDeinitInterface(deviceName, factory);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: Deinit interface failed! ret=%{public}d", __func__, ret);
    }
    g_hdfDeviceInited = false;
    return ret;
}

static int32_t HdfCameraInitDevice(const char *deviceName)
{
    int32_t ret;
    struct CameraDeviceDriverFactory *factory = NULL;

    factory = HdfCameraGetDriverFactory(deviceName);
    if (factory == NULL) {
        HDF_LOGE("%s: get factory failed! deviceName=%{public}s", __func__, deviceName);
        return HDF_FAILURE;
    }

    ret = HdfCameraInitInterfaces(deviceName, factory);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: init interfaces failed! ret=%{public}d!", __func__, ret);
    } else {
        HDF_LOGD("%s: init interfaces success!", __func__);
    }
    return ret;
}

static int32_t HdfCameraDriverInit(struct HdfDeviceObject *device)
{
    HDF_LOGI("%s: HdfCameraDriverInit start...", __func__);
    struct CameraConfigRoot *rootConfig = NULL;
    uint16_t i;
    int32_t ret;

    if (device == NULL) {
        HDF_LOGE("%s: Input param is null", __func__);
        return HDF_ERR_INVALID_PARAM;
    }
    ret = HdfCameraGetConfig(device);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: HdfCameraGetConfig get camera config failed, ret=%{public}d", __func__, ret);
        return ret;
    }
    rootConfig = HdfCameraGetConfigRoot();
    if (rootConfig == NULL) {
        HDF_LOGE("%s: get rootConfig failed!", __func__);
        return HDF_FAILURE;
    }
    ret = HdfCameraAddDevice(device);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: HdfCameraAddDevice failed, ret=%{public}d", __func__, ret);
        return ret;
    }
    for (i = 0; i < rootConfig->deviceNum; i++) {
        ret = HdfCameraInitDevice(rootConfig->deviceConfig[i].deviceName);
        if (ret != HDF_SUCCESS) {
            HDF_LOGE("%s: HdfCameraInitDevice failed, ret=%{public}d", __func__, ret);
            return ret;
        }
    }
    g_hdfDeviceInited = true;
    HDF_LOGD("%s: HdfCameraDriverInit finished.", __func__);
    return HDF_SUCCESS;
}

bool HdfCameraGetDeviceInitStatus(void)
{
    return g_hdfDeviceInited;
}

static int32_t HdfCameraDriverBind(struct HdfDeviceObject *device)
{
    static struct IDeviceIoService cameraService = {
        .object.objectId = 1,
        .Dispatch = HdfCameraDispatch,
    };

    if (device == NULL) {
        HDF_LOGE("%s: Input param is null", __func__);
        return HDF_ERR_INVALID_PARAM;
    }

    device->service = &cameraService;
    HDF_LOGD("%s: camera driver bind successed!", __func__);
    return HDF_SUCCESS;
}

static void HdfCameraDriverRelease(struct HdfDeviceObject *device)
{
    HDF_LOGI("%s: Driver HdfCamera deiniting...", __func__);
    if (device == NULL) {
        HDF_LOGE("%s: Input param is null", __func__);
        return;
    }
    struct IDeviceIoService *cameraService = device->service;
    if (cameraService == NULL) {
        HDF_LOGE("%s: cameraService is null", __func__);
        return;
    }
    OsalMemFree(cameraService);
    device->service = NULL;
    HdfCameraDelDevice();

    HDF_LOGI("%s: Driver HdfCamera deinited.", __func__);
}

struct HdfDriverEntry g_hdfCameraEntry = {
    .moduleVersion = 1,
    .Bind = HdfCameraDriverBind,
    .Init = HdfCameraDriverInit,
    .Release = HdfCameraDriverRelease,
    .moduleName = "HDF_CAMERA"
};

HDF_INIT(g_hdfCameraEntry);