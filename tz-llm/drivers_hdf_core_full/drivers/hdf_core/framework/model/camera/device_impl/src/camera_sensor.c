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
#include "camera_sensor.h"

#define HDF_LOG_TAG HDF_CAMERA_SENSOR

int32_t CameraCmdSensorQueryConfig(struct SubDevice subDev)
{
    int32_t i;
    uint32_t ctrlSize;
    bool isFailed = false;
    int32_t devId = subDev.devId;
    uint32_t ctrlId = subDev.ctrlId;
    struct CameraDeviceConfig *deviceConfig = subDev.cameraHcsConfig;
    struct HdfSBuf *rspData = subDev.rspData;

    ctrlSize = deviceConfig->sensor.sensor[devId].ctrlValueNum;
    for (i = 0; i < ctrlSize; i++) {
        if (ctrlId == deviceConfig->sensor.sensor[devId].ctrlCap[i].ctrlId) {
            isFailed |= !HdfSbufWriteInt32(rspData, deviceConfig->sensor.sensor[devId].ctrlCap[i].ctrlId);
            isFailed |= !HdfSbufWriteInt32(rspData, deviceConfig->sensor.sensor[devId].ctrlCap[i].max);
            isFailed |= !HdfSbufWriteInt32(rspData, deviceConfig->sensor.sensor[devId].ctrlCap[i].min);
            isFailed |= !HdfSbufWriteInt32(rspData, deviceConfig->sensor.sensor[devId].ctrlCap[i].step);
            isFailed |= !HdfSbufWriteInt32(rspData, deviceConfig->sensor.sensor[devId].ctrlCap[i].def);
            CHECK_RETURN_RET(isFailed);
            return HDF_SUCCESS;
        }
    }
    return HDF_ERR_INVALID_PARAM;
}

static int32_t CameraSensorGetConfig(struct CameraDevice *camDev, int32_t devId, struct CameraCtrlConfig *ctrlConfig)
{
    struct CameraDeviceDriver *deviceDriver = GetDeviceDriver(camDev);
    if (deviceDriver == NULL) {
        HDF_LOGE("%s: deviceDriver ptr is null!", __func__);
        return HDF_FAILURE;
    }
    if (deviceDriver->sensor[devId]->devOps->getConfig == NULL) {
        HDF_LOGE("%s: sensor getConfig ptr is null!", __func__);
        return HDF_FAILURE;
    }
    return deviceDriver->sensor[devId]->devOps->getConfig(deviceDriver, ctrlConfig);
}

static int32_t CheckSensorGetConfigId(struct CameraDeviceConfig *deviceConfig,
    struct CameraCtrlConfig ctrlConfig, int32_t devId)
{
    switch (ctrlConfig.ctrl.id) {
        case CAMERA_CMD_GET_EXPOSURE:
            if ((deviceConfig->sensor.sensor[devId].exposure) == DEVICE_NOT_SUPPORT) {
                HDF_LOGE("%s: sensor[%{public}d] not support get exposure", devId, __func__);
                return HDF_ERR_NOT_SUPPORT;
            }
            break;

        case CAMERA_CMD_GET_HFLIP:
        case CAMERA_CMD_GET_VFLIP:
            if ((deviceConfig->sensor.sensor[devId].mirror) == DEVICE_NOT_SUPPORT) {
                HDF_LOGE("%s: sensor[%{public}d] not support get mirror", devId, __func__);
                return HDF_ERR_NOT_SUPPORT;
            }
            break;
        
        case CAMERA_CMD_GET_GAIN:
            if ((deviceConfig->sensor.sensor[devId].gain) == DEVICE_NOT_SUPPORT) {
                HDF_LOGE("%s: sensor[%{public}d] not support get gain", devId, __func__);
                return HDF_ERR_NOT_SUPPORT;
            }
            break;

        default:
            HDF_LOGE("%s: wrong ctrl id, ctrl id=%{public}d", __func__, ctrlConfig.ctrl.id);
            return HDF_ERR_NOT_SUPPORT;
    }
    return HDF_SUCCESS;
}

int32_t CameraCmdSensorGetConfig(struct SubDevice subDev)
{
    int32_t ret;
    struct CameraCtrlConfig ctrlConfig = {};
    int32_t devId = subDev.devId;
    struct CameraDevice *camDev = subDev.camDev;
    struct CameraDeviceConfig *deviceConfig = subDev.cameraHcsConfig;
    struct HdfSBuf *rspData = subDev.rspData;

    ctrlConfig.ctrl.id = subDev.ctrlId;
    ret = CheckSensorGetConfigId(deviceConfig, ctrlConfig, devId);
    CHECK_RETURN_RET(ret);
    ret = CameraSensorGetConfig(camDev, devId, &ctrlConfig);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: CameraSensorGetConfig failed, ret = %{public}d", __func__, ret);
        return ret;
    }
    if (!HdfSbufWriteUint32(rspData, ctrlConfig.ctrl.value)) {
        HDF_LOGE("%s: Write ctrl value failed!", __func__);
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

static int32_t CameraSensorSetConfig(struct CameraDevice *camDev, int32_t devId, struct CameraCtrlConfig *ctrlConfig)
{
    struct CameraDeviceDriver *deviceDriver = GetDeviceDriver(camDev);
    if (deviceDriver == NULL) {
        HDF_LOGE("%s: deviceDriver ptr is null!", __func__);
        return HDF_FAILURE;
    }
    if (deviceDriver->sensor[devId]->devOps->setConfig == NULL) {
        HDF_LOGE("%s: sensor setConfig ptr is null!", __func__);
        return HDF_FAILURE;
    }
    return deviceDriver->sensor[devId]->devOps->setConfig(deviceDriver, ctrlConfig);
}

static int32_t CheckSensorSetConfigId(struct CameraDeviceConfig *deviceConfig,
    struct CameraCtrlConfig ctrlConfig, int32_t devId)
{
    switch (ctrlConfig.ctrl.id) {
        case CAMERA_CMD_SET_EXPOSURE:
            if ((deviceConfig->sensor.sensor[devId].exposure) == DEVICE_NOT_SUPPORT) {
                HDF_LOGE("%s: sensor[%{public}d] not support set exposure", devId, __func__);
                return HDF_ERR_NOT_SUPPORT;
            }
            break;

        case CAMERA_CMD_SET_HFLIP:
        case CAMERA_CMD_SET_VFLIP:
            if ((deviceConfig->sensor.sensor[devId].mirror) == DEVICE_NOT_SUPPORT) {
                HDF_LOGE("%s: sensor[%{public}d] not support set mirror", devId, __func__);
                return HDF_ERR_NOT_SUPPORT;
            }
            break;

        case CAMERA_CMD_SET_GAIN:
            if ((deviceConfig->sensor.sensor[devId].gain) == DEVICE_NOT_SUPPORT) {
                HDF_LOGE("%s: sensor[%{public}d] not support set gain", devId, __func__);
                return HDF_ERR_NOT_SUPPORT;
            }
            break;

        default:
            HDF_LOGE("%s: wrong ctrl id, ctrl id=%{public}d", __func__, ctrlConfig.ctrl.id);
            return HDF_ERR_NOT_SUPPORT;
    }
    return HDF_SUCCESS;
}

int32_t CameraCmdSensorSetConfig(struct SubDevice subDev)
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
    ret = CheckSensorSetConfigId(deviceConfig, ctrlConfig, devId);
    CHECK_RETURN_RET(ret);
    ret = CameraSensorSetConfig(camDev, devId, &ctrlConfig);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: CameraSensorSetConfig failed, ret = %{public}d", __func__, ret);
        return ret;
    }
    return HDF_SUCCESS;
}

int32_t CameraCmdSensorEnumDevice(struct SubDevice subDev)
{
    bool isFailed = false;
    int32_t devId = subDev.devId;
    struct CameraDeviceConfig *deviceConfig = subDev.cameraHcsConfig;
    struct HdfSBuf *rspData = subDev.rspData;

    isFailed |= !HdfSbufWriteUint8(rspData, deviceConfig->sensor.mode);
    isFailed |= !HdfSbufWriteString(rspData, deviceConfig->sensor.sensor[devId].name);
    isFailed |= !HdfSbufWriteUint8(rspData, deviceConfig->sensor.sensor[devId].id);
    isFailed |= !HdfSbufWriteUint8(rspData, deviceConfig->sensor.sensor[devId].exposure);
    isFailed |= !HdfSbufWriteUint8(rspData, deviceConfig->sensor.sensor[devId].mirror);
    isFailed |= !HdfSbufWriteUint8(rspData, deviceConfig->sensor.sensor[devId].gain);
    CHECK_RETURN_RET(isFailed);
    return HDF_SUCCESS;
}

static struct DeviceConfigOps g_sensorDeviceOps = {
    .queryConfig = &CameraCmdSensorQueryConfig,
    .getConfig = &CameraCmdSensorGetConfig,
    .setConfig = &CameraCmdSensorSetConfig,
    .enumDevice = &CameraCmdSensorSetConfig,
};

struct DeviceConfigOps *GetSensorDeviceOps(void)
{
    return &g_sensorDeviceOps;
}
