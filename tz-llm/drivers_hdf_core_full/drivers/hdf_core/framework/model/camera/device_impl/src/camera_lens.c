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
#include "camera_lens.h"

#define HDF_LOG_TAG HDF_CAMERA_LENS

int32_t CameraCmdLensQueryConfig(struct SubDevice subDev)
{
    int32_t i;
    uint32_t ctrlSize;
    bool isFailed = false;
    int32_t devId = subDev.devId;
    uint32_t ctrlId = subDev.ctrlId;
    struct CameraDeviceConfig *deviceConfig = subDev.cameraHcsConfig;
    struct HdfSBuf *rspData = subDev.rspData;

    ctrlSize = deviceConfig->lens.lens[devId].ctrlValueNum;
    for (i = 0; i < ctrlSize; i++) {
        if (ctrlId == deviceConfig->lens.lens[devId].ctrlCap[i].ctrlId) {
            isFailed |= !HdfSbufWriteInt32(rspData, deviceConfig->lens.lens[devId].ctrlCap[i].ctrlId);
            isFailed |= !HdfSbufWriteInt32(rspData, deviceConfig->lens.lens[devId].ctrlCap[i].max);
            isFailed |= !HdfSbufWriteInt32(rspData, deviceConfig->lens.lens[devId].ctrlCap[i].min);
            isFailed |= !HdfSbufWriteInt32(rspData, deviceConfig->lens.lens[devId].ctrlCap[i].step);
            isFailed |= !HdfSbufWriteInt32(rspData, deviceConfig->lens.lens[devId].ctrlCap[i].def);
            CHECK_RETURN_RET(isFailed);
            return HDF_SUCCESS;
        }
    }
    return HDF_ERR_INVALID_PARAM;
}

static int32_t CameraLensGetConfig(struct CameraDevice *camDev, int32_t devId, struct CameraCtrlConfig *ctrlConfig)
{
    struct CameraDeviceDriver *deviceDriver = GetDeviceDriver(camDev);
    if (deviceDriver == NULL) {
        HDF_LOGE("%s: deviceDriver ptr is null!", __func__);
        return HDF_FAILURE;
    }
    if (deviceDriver->lens[devId]->devOps->getConfig == NULL) {
        HDF_LOGE("%s: lens getConfig ptr is null!", __func__);
        return HDF_FAILURE;
    }
    return deviceDriver->lens[devId]->devOps->getConfig(deviceDriver, ctrlConfig);
}

static int32_t CheckLensGetConfigId(struct CameraDeviceConfig *deviceConfig,
    struct CameraCtrlConfig ctrlConfig, int32_t devId)
{
    switch (ctrlConfig.ctrl.id) {
        case CAMERA_CMD_GET_IRIS_ABSOLUTE:
        case CAMERA_CMD_GET_IRIS_RELATIVE:
            if ((deviceConfig->lens.lens[devId].aperture) == DEVICE_NOT_SUPPORT) {
                HDF_LOGE("%s: lens[%{public}d] not support get aperture", devId, __func__);
                return HDF_ERR_NOT_SUPPORT;
            }
            break;
        default:
            HDF_LOGE("%s: wrong ctrl id, ctrl id=%{public}d", __func__, ctrlConfig.ctrl.id);
            return HDF_ERR_NOT_SUPPORT;
    }
    return HDF_SUCCESS;
}

int32_t CameraCmdLensGetConfig(struct SubDevice subDev)
{
    int32_t ret;
    struct CameraCtrlConfig ctrlConfig = {};
    int32_t devId = subDev.devId;
    struct CameraDevice *camDev = subDev.camDev;
    struct CameraDeviceConfig *deviceConfig = subDev.cameraHcsConfig;
    struct HdfSBuf *rspData = subDev.rspData;

    ctrlConfig.ctrl.id = subDev.ctrlId;
    ret = CheckLensGetConfigId(deviceConfig, ctrlConfig, devId);
    CHECK_RETURN_RET(ret);
    ret = CameraLensGetConfig(camDev, devId, &ctrlConfig);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: CameraLensGetConfig failed, ret = %{public}d", __func__, ret);
        return ret;
    }
    if (!HdfSbufWriteUint32(rspData, ctrlConfig.ctrl.value)) {
        HDF_LOGE("%s: Write ctrl value failed!", __func__);
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

static int32_t CameraLensSetConfig(struct CameraDevice *camDev, int32_t devId, struct CameraCtrlConfig *ctrlConfig)
{
    struct CameraDeviceDriver *deviceDriver = GetDeviceDriver(camDev);
    if (deviceDriver == NULL) {
        HDF_LOGE("%s: deviceDriver ptr is null!", __func__);
        return HDF_FAILURE;
    }
    if (deviceDriver->lens[devId]->devOps->setConfig == NULL) {
        HDF_LOGE("%s: lens setConfig ptr is null!", __func__);
        return HDF_FAILURE;
    }
    return deviceDriver->lens[devId]->devOps->setConfig(deviceDriver, ctrlConfig);
}

static int32_t CheckLensSetConfigId(struct CameraDeviceConfig *deviceConfig,
    struct CameraCtrlConfig ctrlConfig, int32_t devId)
{
    switch (ctrlConfig.ctrl.id) {
        case CAMERA_CMD_SET_IRIS_ABSOLUTE:
            if ((deviceConfig->lens.lens[devId].aperture) == DEVICE_NOT_SUPPORT) {
                HDF_LOGE("%s: lens[%{public}d] not support set aperture", devId, __func__);
                return HDF_ERR_NOT_SUPPORT;
            }
            break;

        case CAMERA_CMD_SET_IRIS_RELATIVE:
            if ((deviceConfig->lens.lens[devId].aperture) == DEVICE_NOT_SUPPORT) {
                HDF_LOGE("%s: lens[%{public}d] not support set aperture", devId, __func__);
                return HDF_ERR_NOT_SUPPORT;
            }
            break;

        default:
            HDF_LOGE("%s: wrong ctrl id, ctrl id=%{public}d", __func__, ctrlConfig.ctrl.id);
            return HDF_ERR_NOT_SUPPORT;
    }
    return HDF_SUCCESS;
}

int32_t CameraCmdLensSetConfig(struct SubDevice subDev)
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
    ret = CheckLensSetConfigId(deviceConfig, ctrlConfig, devId);
    CHECK_RETURN_RET(ret);
    ret = CameraLensSetConfig(camDev, devId, &ctrlConfig);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: CameraLensSetConfig failed, ret = %{public}d", __func__, ret);
        return ret;
    }
    return HDF_SUCCESS;
}

int32_t CameraCmdLensEnumDevice(struct SubDevice subDev)
{
    bool isFailed = false;
    int32_t devId = subDev.devId;
    struct CameraDeviceConfig *deviceConfig = subDev.cameraHcsConfig;
    struct HdfSBuf *rspData = subDev.rspData;

    isFailed |= !HdfSbufWriteUint8(rspData, deviceConfig->lens.mode);
    isFailed |= !HdfSbufWriteString(rspData, deviceConfig->lens.lens[devId].name);
    isFailed |= !HdfSbufWriteUint8(rspData, deviceConfig->lens.lens[devId].id);
    isFailed |= !HdfSbufWriteUint8(rspData, deviceConfig->lens.lens[devId].aperture);
    CHECK_RETURN_RET(isFailed);
    return HDF_SUCCESS;
}

static struct DeviceConfigOps g_lensDeviceOps = {
    .queryConfig = &CameraCmdLensQueryConfig,
    .getConfig = &CameraCmdLensGetConfig,
    .setConfig = &CameraCmdLensSetConfig,
    .enumDevice = &CameraCmdLensSetConfig,
};

struct DeviceConfigOps *GetLensDeviceOps(void)
{
    return &g_lensDeviceOps;
}
