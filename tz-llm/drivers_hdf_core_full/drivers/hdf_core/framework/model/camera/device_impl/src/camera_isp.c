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
#include "camera_isp.h"

#define HDF_LOG_TAG HDF_CAMERA_ISP

int32_t CameraCmdIspQueryConfig(struct SubDevice subDev)
{
    int32_t i;
    uint32_t ctrlSize;
    bool isFailed = false;
    int32_t devId = subDev.devId;
    uint32_t ctrlId = subDev.ctrlId;
    struct CameraDeviceConfig *deviceConfig = subDev.cameraHcsConfig;
    struct HdfSBuf *rspData = subDev.rspData;

    ctrlSize = deviceConfig->isp.isp[devId].ctrlValueNum;
    for (i = 0; i < ctrlSize; i++) {
        if (ctrlId == deviceConfig->isp.isp[devId].ctrlCap[i].ctrlId) {
            isFailed |= !HdfSbufWriteInt32(rspData, deviceConfig->isp.isp[devId].ctrlCap[i].ctrlId);
            isFailed |= !HdfSbufWriteInt32(rspData, deviceConfig->isp.isp[devId].ctrlCap[i].max);
            isFailed |= !HdfSbufWriteInt32(rspData, deviceConfig->isp.isp[devId].ctrlCap[i].min);
            isFailed |= !HdfSbufWriteInt32(rspData, deviceConfig->isp.isp[devId].ctrlCap[i].step);
            isFailed |= !HdfSbufWriteInt32(rspData, deviceConfig->isp.isp[devId].ctrlCap[i].def);
            CHECK_RETURN_RET(isFailed);
            return HDF_SUCCESS;
        }
    }
    return HDF_ERR_INVALID_PARAM;
}

static int32_t CameraIspGetConfig(struct CameraDevice *camDev, int32_t devId, struct CameraCtrlConfig *ctrlConfig)
{
    struct CameraDeviceDriver *deviceDriver = GetDeviceDriver(camDev);
    if (deviceDriver == NULL) {
        HDF_LOGE("%s: deviceDriver ptr is null!", __func__);
        return HDF_FAILURE;
    }
    if (deviceDriver->isp[devId]->devOps->getConfig == NULL) {
        HDF_LOGE("%s: isp getConfig ptr is null!", __func__);
        return HDF_FAILURE;
    }
    return deviceDriver->isp[devId]->devOps->getConfig(deviceDriver, ctrlConfig);
}

static int32_t CheckIspGetConfigId(struct CameraDeviceConfig *deviceConfig,
    struct CameraCtrlConfig ctrlConfig, int32_t devId)
{
    switch (ctrlConfig.ctrl.id) {
        case CAMERA_CMD_GET_WHITE_BALANCE_MODE:
        case CAMERA_CMD_GET_WHITE_BALANCE:
            if ((deviceConfig->isp.isp[devId].whiteBalance) == DEVICE_NOT_SUPPORT) {
                HDF_LOGE("%s: isp not support get whiteBalance", __func__);
                return HDF_ERR_NOT_SUPPORT;
            }
            break;
        case CAMERA_CMD_GET_BRIGHTNESS:
            if ((deviceConfig->isp.isp[devId].brightness) == DEVICE_NOT_SUPPORT) {
                HDF_LOGE("%s: isp not support get brightness", __func__);
                return HDF_ERR_NOT_SUPPORT;
            }
            break;
        case CAMERA_CMD_GET_CONTRAST:
            if ((deviceConfig->isp.isp[devId].contrast) == DEVICE_NOT_SUPPORT) {
                HDF_LOGE("%s: isp not support get contrast", __func__);
                return HDF_ERR_NOT_SUPPORT;
            }
            break;
        case CAMERA_CMD_GET_SATURATION:
            if ((deviceConfig->isp.isp[devId].saturation) == DEVICE_NOT_SUPPORT) {
                HDF_LOGE("%s: isp not support get saturation", __func__);
                return HDF_ERR_NOT_SUPPORT;
            }
            break;
        case CAMERA_CMD_GET_HUE:
            if ((deviceConfig->isp.isp[devId].hue) == DEVICE_NOT_SUPPORT) {
                HDF_LOGE("%s: isp not support get hue", __func__);
                return HDF_ERR_NOT_SUPPORT;
            }
            break;
        case CAMERA_CMD_GET_SHARPNESS:
            if ((deviceConfig->isp.isp[devId].sharpness) == DEVICE_NOT_SUPPORT) {
                HDF_LOGE("%s: isp not support get contrast", __func__);
                return HDF_ERR_NOT_SUPPORT;
            }
            break;
        case CAMERA_CMD_GET_GAIN:
            if ((deviceConfig->isp.isp[devId].gain) == DEVICE_NOT_SUPPORT) {
                HDF_LOGE("%s: isp not support get gain", __func__);
                return HDF_ERR_NOT_SUPPORT;
            }
            break;
        case CAMERA_CMD_GET_GAMMA:
            if ((deviceConfig->isp.isp[devId].gamma) == DEVICE_NOT_SUPPORT) {
                HDF_LOGE("%s: isp not support get gamma", __func__);
                return HDF_ERR_NOT_SUPPORT;
            }
            break;
        default:
            HDF_LOGE("%s: wrong ctrl id, ctrl id=%{public}d", __func__, ctrlConfig.ctrl.id);
            return HDF_ERR_NOT_SUPPORT;
    }
    return HDF_SUCCESS;
}

int32_t CameraCmdIspGetConfig(struct SubDevice subDev)
{
    int32_t ret;
    struct CameraCtrlConfig ctrlConfig = {};
    int32_t devId = subDev.devId;
    struct CameraDevice *camDev = subDev.camDev;
    struct CameraDeviceConfig *deviceConfig = subDev.cameraHcsConfig;
    struct HdfSBuf *rspData = subDev.rspData;

    ctrlConfig.ctrl.id = subDev.ctrlId;
    ret = CheckIspGetConfigId(deviceConfig, ctrlConfig, devId);
    CHECK_RETURN_RET(ret);
    ret = CameraIspGetConfig(camDev, devId, &ctrlConfig);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: CameraIspGetConfig failed, ret = %{public}d", __func__, ret);
        return ret;
    }
    if (!HdfSbufWriteUint32(rspData, ctrlConfig.ctrl.value)) {
        HDF_LOGE("%s: Write ctrl value failed!", __func__);
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

static int32_t CameraIspSetConfig(struct CameraDevice *camDev, int32_t devId, struct CameraCtrlConfig *ctrlConfig)
{
    struct CameraDeviceDriver *deviceDriver = GetDeviceDriver(camDev);
    if (deviceDriver == NULL) {
        HDF_LOGE("%s: deviceDriver ptr is null!", __func__);
        return HDF_FAILURE;
    }
    if (deviceDriver->isp[devId]->devOps->setConfig == NULL) {
        HDF_LOGE("%s: isp setConfig ptr is null!", __func__);
        return HDF_FAILURE;
    }
    return deviceDriver->isp[devId]->devOps->setConfig(deviceDriver, ctrlConfig);
}

static int32_t CheckIspSetConfigId(struct CameraDeviceConfig *deviceConfig,
    struct CameraCtrlConfig ctrlConfig, int32_t devId)
{
    switch (ctrlConfig.ctrl.id) {
        case CAMERA_CMD_SET_WHITE_BALANCE_MODE:
        case CAMERA_CMD_SET_WHITE_BALANCE:
            if ((deviceConfig->isp.isp[devId].whiteBalance) == DEVICE_NOT_SUPPORT) {
                HDF_LOGE("%s: isp not support set whiteBalance", __func__);
                return HDF_ERR_NOT_SUPPORT;
            }
            break;
        case CAMERA_CMD_SET_BRIGHTNESS:
            if ((deviceConfig->isp.isp[devId].brightness) == DEVICE_NOT_SUPPORT) {
                HDF_LOGE("%s: isp not support set brightness", __func__);
                return HDF_ERR_NOT_SUPPORT;
            }
            break;
        case CAMERA_CMD_SET_CONTRAST:
            if ((deviceConfig->isp.isp[devId].contrast) == DEVICE_NOT_SUPPORT) {
                HDF_LOGE("%s: isp not support set contrast", __func__);
                return HDF_ERR_NOT_SUPPORT;
            }
            break;
        case CAMERA_CMD_SET_SATURATION:
            if ((deviceConfig->isp.isp[devId].saturation) == DEVICE_NOT_SUPPORT) {
                HDF_LOGE("%s: isp not support set saturation", __func__);
                return HDF_ERR_NOT_SUPPORT;
            }
            break;
        case CAMERA_CMD_SET_HUE:
            if ((deviceConfig->isp.isp[devId].hue) == DEVICE_NOT_SUPPORT) {
                HDF_LOGE("%s: isp not support set hue", __func__);
                return HDF_ERR_NOT_SUPPORT;
            }
            break;
        case CAMERA_CMD_SET_SHARPNESS:
            if ((deviceConfig->isp.isp[devId].sharpness) == DEVICE_NOT_SUPPORT) {
                HDF_LOGE("%s: isp not support set contrast", __func__);
                return HDF_ERR_NOT_SUPPORT;
            }
            break;
        case CAMERA_CMD_SET_GAIN:
            if ((deviceConfig->isp.isp[devId].gain) == DEVICE_NOT_SUPPORT) {
                HDF_LOGE("%s: isp not support set gain", __func__);
                return HDF_ERR_NOT_SUPPORT;
            }
            break;
        case CAMERA_CMD_SET_GAMMA:
            if ((deviceConfig->isp.isp[devId].gamma) == DEVICE_NOT_SUPPORT) {
                HDF_LOGE("%s: isp not support set gamma", __func__);
                return HDF_ERR_NOT_SUPPORT;
            }
            break;
        default:
            HDF_LOGE("%s: wrong ctrl id, ctrl id=%{public}d", __func__, ctrlConfig.ctrl.id);
            return HDF_ERR_NOT_SUPPORT;
    }
    return HDF_SUCCESS;
}

int32_t CameraCmdIspSetConfig(struct SubDevice subDev)
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
    ret = CheckIspSetConfigId(deviceConfig, ctrlConfig, devId);
    CHECK_RETURN_RET(ret);
    ret = CameraIspSetConfig(camDev, devId, &ctrlConfig);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: CameraIspSetConfig failed, ret = %{public}d", __func__, ret);
        return ret;
    }
    return HDF_SUCCESS;
}

int32_t CameraCmdIspEnumDevice(struct SubDevice subDev)
{
    bool isFailed = false;
    int32_t devId = subDev.devId;
    struct CameraDeviceConfig *deviceConfig = subDev.cameraHcsConfig;
    struct HdfSBuf *rspData = subDev.rspData;

    isFailed |= !HdfSbufWriteUint8(rspData, deviceConfig->isp.mode);
    isFailed |= !HdfSbufWriteString(rspData, deviceConfig->isp.isp[devId].name);
    isFailed |= !HdfSbufWriteUint8(rspData, deviceConfig->isp.isp[devId].id);
    isFailed |= !HdfSbufWriteUint8(rspData, deviceConfig->isp.isp[devId].brightness);
    isFailed |= !HdfSbufWriteUint8(rspData, deviceConfig->isp.isp[devId].contrast);
    isFailed |= !HdfSbufWriteUint8(rspData, deviceConfig->isp.isp[devId].saturation);
    isFailed |= !HdfSbufWriteUint8(rspData, deviceConfig->isp.isp[devId].hue);
    isFailed |= !HdfSbufWriteUint8(rspData, deviceConfig->isp.isp[devId].sharpness);
    isFailed |= !HdfSbufWriteUint8(rspData, deviceConfig->isp.isp[devId].gain);
    isFailed |= !HdfSbufWriteUint8(rspData, deviceConfig->isp.isp[devId].gamma);
    isFailed |= !HdfSbufWriteUint8(rspData, deviceConfig->isp.isp[devId].whiteBalance);
    CHECK_RETURN_RET(isFailed);
    return HDF_SUCCESS;
}

static struct DeviceConfigOps g_ispDeviceOps = {
    .queryConfig = &CameraCmdIspQueryConfig,
    .getConfig = &CameraCmdIspGetConfig,
    .setConfig = &CameraCmdIspSetConfig,
    .enumDevice = &CameraCmdIspSetConfig,
};

struct DeviceConfigOps *GetIspDeviceOps(void)
{
    return &g_ispDeviceOps;
}
