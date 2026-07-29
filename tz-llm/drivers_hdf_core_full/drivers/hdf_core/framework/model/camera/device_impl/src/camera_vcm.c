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
#include "camera_vcm.h"

#define HDF_LOG_TAG HDF_CAMERA_VCM

int32_t CameraCmdVcmQueryConfig(struct SubDevice subDev)
{
    int32_t i;
    uint32_t ctrlSize;
    bool isFailed = false;
    int32_t devId = subDev.devId;
    uint32_t ctrlId = subDev.ctrlId;
    struct CameraDeviceConfig *deviceConfig = subDev.cameraHcsConfig;
    struct HdfSBuf *rspData = subDev.rspData;

    ctrlSize = deviceConfig->vcm.vcm[devId].ctrlValueNum;
    for (i = 0; i < ctrlSize; i++) {
        if (ctrlId == deviceConfig->vcm.vcm[devId].ctrlCap[i].ctrlId) {
            isFailed |= !HdfSbufWriteInt32(rspData, deviceConfig->vcm.vcm[devId].ctrlCap[i].ctrlId);
            isFailed |= !HdfSbufWriteInt32(rspData, deviceConfig->vcm.vcm[devId].ctrlCap[i].max);
            isFailed |= !HdfSbufWriteInt32(rspData, deviceConfig->vcm.vcm[devId].ctrlCap[i].min);
            isFailed |= !HdfSbufWriteInt32(rspData, deviceConfig->vcm.vcm[devId].ctrlCap[i].step);
            isFailed |= !HdfSbufWriteInt32(rspData, deviceConfig->vcm.vcm[devId].ctrlCap[i].def);
            CHECK_RETURN_RET(isFailed);
            return HDF_SUCCESS;
        }
    }
    return HDF_ERR_INVALID_PARAM;
}

static int32_t CameraVcmGetConfig(struct CameraDevice *camDev, int32_t devId, struct CameraCtrlConfig *ctrlConfig)
{
    struct CameraDeviceDriver *deviceDriver = GetDeviceDriver(camDev);
    if (deviceDriver == NULL) {
        HDF_LOGE("%s: deviceDriver ptr is null!", __func__);
        return HDF_FAILURE;
    }
    if (deviceDriver->vcm[devId]->devOps->getConfig == NULL) {
        HDF_LOGE("%s: vcm getConfig ptr is null!", __func__);
        return HDF_FAILURE;
    }
    return deviceDriver->vcm[devId]->devOps->getConfig(deviceDriver, ctrlConfig);
}

static int32_t CheckVcmGetConfigId(struct CameraDeviceConfig *deviceConfig,
    struct CameraCtrlConfig ctrlConfig, int32_t devId)
{
    switch (ctrlConfig.ctrl.id) {
        case CAMERA_CMD_GET_FOCUS_MODE:
            if ((deviceConfig->vcm.vcm[devId].autoFocus) == DEVICE_NOT_SUPPORT) {
                HDF_LOGE("%s: vcm[%{public}d] not support get autoFocus", devId, __func__);
                return HDF_ERR_NOT_SUPPORT;
            }
            break;

        case CAMERA_CMD_GET_FOCUS_ABSOLUTE:
            if ((deviceConfig->vcm.vcm[devId].focus) == DEVICE_NOT_SUPPORT) {
                HDF_LOGE("%s: vcm[%{public}d] not support get focus", devId, __func__);
                return HDF_ERR_NOT_SUPPORT;
            }
            break;

        case CAMERA_CMD_GET_ZOOM_ABSOLUTE:
        case CAMERA_CMD_GET_ZOOM_CONTINUOUS:
            if ((deviceConfig->vcm.vcm[devId].zoom) == DEVICE_NOT_SUPPORT) {
                HDF_LOGE("%s: vcm[%{public}d] not support get zoom", devId, __func__);
                return HDF_ERR_NOT_SUPPORT;
            }
            break;

        default:
            HDF_LOGE("%s: wrong ctrl id, ctrl id=%{public}d", __func__, ctrlConfig.ctrl.id);
            return HDF_ERR_NOT_SUPPORT;
    }
    return HDF_SUCCESS;
}

int32_t CameraCmdVcmGetConfig(struct SubDevice subDev)
{
    int32_t ret;
    struct CameraCtrlConfig ctrlConfig = {};
    int32_t devId = subDev.devId;
    struct CameraDevice *camDev = subDev.camDev;
    struct CameraDeviceConfig *deviceConfig = subDev.cameraHcsConfig;
    struct HdfSBuf *rspData = subDev.rspData;

    ctrlConfig.ctrl.id = subDev.ctrlId;
    ret = CheckVcmGetConfigId(deviceConfig, ctrlConfig, devId);
    CHECK_RETURN_RET(ret);
    ret = CameraVcmGetConfig(camDev, devId, &ctrlConfig);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: CameraVcmGetConfig failed, ret = %{public}d", __func__, ret);
        return ret;
    }
    if (!HdfSbufWriteUint32(rspData, ctrlConfig.ctrl.value)) {
        HDF_LOGE("%s: Write ctrl value failed!", __func__);
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

static int32_t CameraVcmSetConfig(struct CameraDevice *camDev, int32_t devId, struct CameraCtrlConfig *ctrlConfig)
{
    struct CameraDeviceDriver *deviceDriver = GetDeviceDriver(camDev);
    if (deviceDriver == NULL) {
        HDF_LOGE("%s: deviceDriver ptr is null!", __func__);
        return HDF_FAILURE;
    }
    if (deviceDriver->vcm[devId]->devOps->setConfig == NULL) {
        HDF_LOGE("%s: vcm setConfig ptr is null!", __func__);
        return HDF_FAILURE;
    }
    return deviceDriver->vcm[devId]->devOps->setConfig(deviceDriver, ctrlConfig);
}

static int32_t CheckVcmSetConfigId(struct CameraDeviceConfig *deviceConfig,
    struct CameraCtrlConfig ctrlConfig, int32_t devId)
{
    switch (ctrlConfig.ctrl.id) {
        case CAMERA_CMD_SET_FOCUS_MODE:
            if ((deviceConfig->vcm.vcm[devId].autoFocus) == DEVICE_NOT_SUPPORT) {
                HDF_LOGE("%s: vcm[%{public}d] not support set autoFocus", devId, __func__);
                return HDF_ERR_NOT_SUPPORT;
            }
            break;

        case CAMERA_CMD_SET_FOCUS_ABSOLUTE:
        case CAMERA_CMD_SET_FOCUS_RELATIVE:
            if ((deviceConfig->vcm.vcm[devId].focus) == DEVICE_NOT_SUPPORT) {
                HDF_LOGE("%s: vcm[%{public}d] not support set focus", devId, __func__);
                return HDF_ERR_NOT_SUPPORT;
            }
            break;

        case CAMERA_CMD_SET_ZOOM_ABSOLUTE:
        case CAMERA_CMD_SET_ZOOM_RELATIVE:
        case CAMERA_CMD_SET_ZOOM_CONTINUOUS:
            if ((deviceConfig->vcm.vcm[devId].zoom) == DEVICE_NOT_SUPPORT) {
                HDF_LOGE("%s: vcm[%{public}d] not support set zoom", devId, __func__);
                return HDF_ERR_NOT_SUPPORT;
            }
            break;

        default:
            HDF_LOGE("%s: wrong ctrl id, ctrl id=%{public}d", __func__, ctrlConfig.ctrl.id);
            return HDF_ERR_NOT_SUPPORT;
    }
    return HDF_SUCCESS;
}

int32_t CameraCmdVcmSetConfig(struct SubDevice subDev)
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
    ret = CheckVcmSetConfigId(deviceConfig, ctrlConfig, devId);
    CHECK_RETURN_RET(ret);
    ret = CameraVcmSetConfig(camDev, devId, &ctrlConfig);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: CameraVcmSetConfig failed, ret = %{public}d", __func__, ret);
        return ret;
    }
    return HDF_SUCCESS;
}

int32_t CameraCmdVcmEnumDevice(struct SubDevice subDev)
{
    bool isFailed = false;
    int32_t devId = subDev.devId;
    struct CameraDeviceConfig *deviceConfig = subDev.cameraHcsConfig;
    struct HdfSBuf *rspData = subDev.rspData;

    isFailed |= !HdfSbufWriteUint8(rspData, deviceConfig->vcm.mode);
    isFailed |= !HdfSbufWriteString(rspData, deviceConfig->vcm.vcm[devId].name);
    isFailed |= !HdfSbufWriteUint8(rspData, deviceConfig->vcm.vcm[devId].id);
    isFailed |= !HdfSbufWriteUint8(rspData, deviceConfig->vcm.vcm[devId].autoFocus);
    isFailed |= !HdfSbufWriteUint8(rspData, deviceConfig->vcm.vcm[devId].zoom);
    isFailed |= !HdfSbufWriteUint32(rspData, deviceConfig->vcm.vcm[devId].zoomMaxNum);
    CHECK_RETURN_RET(isFailed);
    return HDF_SUCCESS;
}

static struct DeviceConfigOps g_vcmDeviceOps = {
    .queryConfig = &CameraCmdVcmQueryConfig,
    .getConfig = &CameraCmdVcmGetConfig,
    .setConfig = &CameraCmdVcmSetConfig,
    .enumDevice = &CameraCmdVcmSetConfig,
};

struct DeviceConfigOps *GetVcmDeviceOps(void)
{
    return &g_vcmDeviceOps;
}
