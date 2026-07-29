/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include <securec.h>
#include <hdf_log.h>
#include "camera_config_parser.h"
#include "camera_utils.h"
#include "camera_flash.h"

#define HDF_LOG_TAG HDF_CAMERA_FLASH

int32_t CameraCmdFlashQueryConfig(struct SubDevice subDev)
{
    int32_t i;
    uint32_t ctrlSize;
    bool isFailed = false;
    int32_t devId = subDev.devId;
    uint32_t ctrlId = subDev.ctrlId;
    struct CameraDeviceConfig *deviceConfig = subDev.cameraHcsConfig;
    struct HdfSBuf *rspData = subDev.rspData;

    ctrlSize = deviceConfig->flash.flash[devId].ctrlValueNum;
    for (i = 0; i < ctrlSize; i++) {
        if (ctrlId == deviceConfig->flash.flash[devId].ctrlCap[i].ctrlId) {
            isFailed |= !HdfSbufWriteInt32(rspData, deviceConfig->flash.flash[devId].ctrlCap[i].ctrlId);
            isFailed |= !HdfSbufWriteInt32(rspData, deviceConfig->flash.flash[devId].ctrlCap[i].max);
            isFailed |= !HdfSbufWriteInt32(rspData, deviceConfig->flash.flash[devId].ctrlCap[i].min);
            isFailed |= !HdfSbufWriteInt32(rspData, deviceConfig->flash.flash[devId].ctrlCap[i].step);
            isFailed |= !HdfSbufWriteInt32(rspData, deviceConfig->flash.flash[devId].ctrlCap[i].def);
            CHECK_RETURN_RET(isFailed);
            return HDF_SUCCESS;
        }
    }
    return HDF_ERR_INVALID_PARAM;
}

static int32_t CameraFlashGetConfig(struct CameraDevice *camDev, int32_t devId, struct CameraCtrlConfig *ctrlConfig)
{
    struct CameraDeviceDriver *deviceDriver = GetDeviceDriver(camDev);
    if (deviceDriver == NULL) {
        HDF_LOGE("%s: deviceDriver ptr is null!", __func__);
        return HDF_FAILURE;
    }
    if (deviceDriver->flash[devId]->devOps->getConfig == NULL) {
        HDF_LOGE("%s: flash getConfig ptr is null!", __func__);
        return HDF_FAILURE;
    }
    return deviceDriver->flash[devId]->devOps->getConfig(deviceDriver, ctrlConfig);
}

static int32_t CheckFlashGetConfigId(struct CameraDeviceConfig *deviceConfig,
    struct CameraCtrlConfig ctrlConfig, int32_t devId)
{
    switch (ctrlConfig.ctrl.id) {
        case CAMERA_CMD_GET_FLASH_FAULT:
        case CAMERA_CMD_GET_FLASH_READY:
            if ((deviceConfig->flash.flash[devId].flashIntensity) == DEVICE_NOT_SUPPORT) {
                HDF_LOGE("%s: flash not support get flashIntensity", __func__);
                return HDF_ERR_NOT_SUPPORT;
            }
            break;
        default:
            HDF_LOGE("%s: wrong ctrl id, ctrl id=%{public}d", __func__, ctrlConfig.ctrl.id);
            return HDF_ERR_NOT_SUPPORT;
    }
    return HDF_SUCCESS;
}

int32_t CameraCmdFlashGetConfig(struct SubDevice subDev)
{
    int32_t ret;
    struct CameraCtrlConfig ctrlConfig = {};
    int32_t devId = subDev.devId;
    struct CameraDevice *camDev = subDev.camDev;
    struct CameraDeviceConfig *deviceConfig = subDev.cameraHcsConfig;
    struct HdfSBuf *rspData = subDev.rspData;

    ctrlConfig.ctrl.id = subDev.ctrlId;
    ret = CheckFlashGetConfigId(deviceConfig, ctrlConfig, devId);
    CHECK_RETURN_RET(ret);
    ret = CameraFlashGetConfig(camDev, devId, &ctrlConfig);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: CameraFlashGetConfig failed, ret = %{public}d", __func__, ret);
        return ret;
    }
    if (!HdfSbufWriteUint32(rspData, ctrlConfig.ctrl.value)) {
        HDF_LOGE("%s: Write ctrl value failed!", __func__);
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

static int32_t CameraFlashSetConfig(struct CameraDevice *camDev, int32_t devId, struct CameraCtrlConfig *ctrlConfig)
{
    struct CameraDeviceDriver *deviceDriver = GetDeviceDriver(camDev);
    if (deviceDriver == NULL) {
        HDF_LOGE("%s: deviceDriver ptr is null!", __func__);
        return HDF_FAILURE;
    }
    if (deviceDriver->flash[devId]->devOps->setConfig == NULL) {
        HDF_LOGE("%s: flash setConfig ptr is null!", __func__);
        return HDF_FAILURE;
    }
    return deviceDriver->flash[devId]->devOps->setConfig(deviceDriver, ctrlConfig);
}

static int32_t CheckFlashSetConfigId(struct CameraDeviceConfig *deviceConfig,
    struct CameraCtrlConfig ctrlConfig, int32_t devId)
{
    switch (ctrlConfig.ctrl.id) {
        case CAMERA_CMD_SET_FLASH_STROBE:
        case CAMERA_CMD_SET_FLASH_INTENSITY:
            if ((deviceConfig->flash.flash[devId].flashIntensity) == DEVICE_NOT_SUPPORT) {
                HDF_LOGE("%s: flash not support set flashIntensity", __func__);
                return HDF_ERR_NOT_SUPPORT;
            }
            break;
        default:
            HDF_LOGE("%s: wrong ctrl id, ctrl id=%{public}d", __func__, ctrlConfig.ctrl.id);
            return HDF_ERR_NOT_SUPPORT;
    }
    return HDF_SUCCESS;
}

int32_t CameraCmdFlashSetConfig(struct SubDevice subDev)
{
    int32_t ret;
    bool isFailed = false;
    struct CameraCtrlConfig ctrlConfig = {};
    int32_t devId = subDev.devId;
    struct CameraDevice *camDev = subDev.camDev;
    struct CameraDeviceConfig *deviceConfig = subDev.cameraHcsConfig;
    struct HdfSBuf *reqData = subDev.reqData;

    isFailed |= !HdfSbufReadUint32(reqData, &ctrlConfig.ctrl.id);
    isFailed |= !HdfSbufReadUint32(reqData, &ctrlConfig.ctrl.value);
    CHECK_RETURN_RET(isFailed);
    ret = CheckFlashSetConfigId(deviceConfig, ctrlConfig, devId);
    CHECK_RETURN_RET(ret);
    ret = CameraFlashSetConfig(camDev, devId, &ctrlConfig);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: CameraFlashSetConfig failed, ret = %{public}d", __func__, ret);
        return ret;
    }
    return HDF_SUCCESS;
}

int32_t CameraCmdFlashEnumDevice(struct SubDevice subDev)
{
    bool isFailed = false;
    int32_t devId = subDev.devId;
    struct CameraDeviceConfig *deviceConfig = subDev.cameraHcsConfig;
    struct HdfSBuf *rspData = subDev.rspData;

    isFailed |= !HdfSbufWriteUint8(rspData, deviceConfig->flash.mode);
    isFailed |= !HdfSbufWriteString(rspData, deviceConfig->flash.flash[devId].name);
    isFailed |= !HdfSbufWriteUint8(rspData, deviceConfig->flash.flash[devId].id);
    isFailed |= !HdfSbufWriteUint8(rspData, deviceConfig->flash.flash[devId].flashMode);
    isFailed |= !HdfSbufWriteUint8(rspData, deviceConfig->flash.flash[devId].flashIntensity);
    CHECK_RETURN_RET(isFailed);
    return HDF_SUCCESS;
}

static struct DeviceConfigOps g_flashDeviceOps = {
    .queryConfig = &CameraCmdFlashQueryConfig,
    .getConfig = &CameraCmdFlashGetConfig,
    .setConfig = &CameraCmdFlashSetConfig,
    .enumDevice = &CameraCmdFlashSetConfig,
};

struct DeviceConfigOps *GetFlashDeviceOps(void)
{
    return &g_flashDeviceOps;
}

