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
#include "camera_device_manager.h"
#include "camera_utils.h"

#define HDF_LOG_TAG HDF_CAMERA_UTILS

#define CAMERA_DEVICE_BIT_MAX 2
#define DEC_CALC_COEFF 10
#define FPS_CALC_COEFF 1000
#define UINT_MAX_VAL 0xFFFFFFFF
#define UVC_DEVICE_ID 0

static int32_t g_cameraIdMap[ID_MAX_NUM];

bool FindAvailableId(uint32_t *id)
{
    uint32_t i;
    if (id == NULL) {
        HDF_LOGE("%s: param is null ptr", __func__);
        return false;
    }
    for (i = 0; i < ID_MAX_NUM; i++) {
        if (g_cameraIdMap[i] == 0) {
            *id = i;
            return true;
        }
    }
    return false;
}

int32_t AddPermissionId(int32_t id, int32_t permissionId)
{
    if (id >= ID_MAX_NUM) {
        HDF_LOGE("%s: error: id out of range!", __func__);
        return HDF_FAILURE;
    }
    if (g_cameraIdMap[id] != 0) {
        HDF_LOGE("%s: error: g_cameraIdMap[%{public}d] is already exist!", __func__, id);
        return HDF_FAILURE;
    }
    g_cameraIdMap[id] = permissionId;
    return HDF_SUCCESS;
}

int32_t RemovePermissionId(int32_t id)
{
    int32_t i;
    for (i = 0; i < ID_MAX_NUM; i++) {
        if (g_cameraIdMap[i] != 0) {
            if (g_cameraIdMap[i] == id) {
                g_cameraIdMap[i] = 0;
                return HDF_SUCCESS;
            }
        }
    }
    return HDF_FAILURE;
}

int32_t GetCameraId(const char *str, int length, int *id)
{
    int32_t ret[CAMERA_DEVICE_BIT_MAX];
    int32_t i = 0;
    int32_t j = 0;

    if (str == NULL || id == NULL) {
        HDF_LOGE("%s: param is null ptr", __func__);
        return HDF_ERR_INVALID_PARAM;
    }

    for (i = 0; i < length; i++) {
        if ((*(str + i) < '0') || (*(str + i) > '9')) {
            continue;
        }
        if (j >= CAMERA_DEVICE_BIT_MAX) {
            HDF_LOGE("%s: wrong num: %{public}d", __func__, j);
            return HDF_ERR_INVALID_PARAM;
        }
        ret[j] = (*(str + i) - '0');
        j++;
    }
    *id = 0;
    for (i = 0; i < j; i++) {
        *id = ret[i] + (*id) * DEC_CALC_COEFF;
    }
    if (*id > CAMERA_MAX_NUM) {
        HDF_LOGE("%s: wrong id: %{public}d", __func__, id);
        return HDF_ERR_INVALID_PARAM;
    }
    return HDF_SUCCESS;
}

struct CameraDeviceDriver *GetDeviceDriver(const struct CameraDevice *cameraDev)
{
    if (cameraDev == NULL) {
        HDF_LOGE("%s: cameraDev is null!", __func__);
        return NULL;
    }
    return cameraDev->deviceDriver;
}

int32_t CheckPermission(int permissionId)
{
    if (permissionId != CAMERA_MASTER) {
        HDF_LOGE("%s: No permissions set ctrl!", __func__);
        return HDF_ERR_NOPERM;
    }
    return HDF_SUCCESS;
}

static int32_t CheckSwitchType(int type, struct CameraConfigRoot *rootConfig, int32_t camId)
{
    switch (type) {
        case SENSOR_TYPE:
            if (rootConfig->deviceConfig[camId].sensor.mode == DEVICE_NOT_SUPPORT) {
                HDF_LOGE("%s: sendor device %{public}d not supported!", __func__, camId);
                return HDF_ERR_NOT_SUPPORT;
            }
            break;
        case ISP_TYPE:
            if (rootConfig->deviceConfig[camId].isp.mode == DEVICE_NOT_SUPPORT) {
                HDF_LOGE("%s: isp device %{public}d not supported!", __func__, camId);
                return HDF_ERR_NOT_SUPPORT;
            }
            break;
        case VCM_TYPE:
            if (rootConfig->deviceConfig[camId].vcm.mode == DEVICE_NOT_SUPPORT) {
                HDF_LOGE("%s: vcm device %{public}d not supported!", __func__, camId);
                return HDF_ERR_NOT_SUPPORT;
            }
            break;
        case LENS_TYPE:
            if (rootConfig->deviceConfig[camId].lens.mode == DEVICE_NOT_SUPPORT) {
                HDF_LOGE("%s: lens device %{public}d not supported!", __func__, camId);
                return HDF_ERR_NOT_SUPPORT;
            }
            break;
        case FLASH_TYPE:
            if (rootConfig->deviceConfig[camId].flash.mode == DEVICE_NOT_SUPPORT) {
                HDF_LOGE("%s: flash device %{public}d not supported!", __func__, camId);
                return HDF_ERR_NOT_SUPPORT;
            }
            break;
        case UVC_TYPE:
            if (rootConfig->uvcMode == DEVICE_NOT_SUPPORT) {
                HDF_LOGE("%s: uvc device %{public}d not supported!", __func__, camId);
                return HDF_ERR_NOT_SUPPORT;
            }
            break;
        case STREAM_TYPE:
            break;
        default:
            HDF_LOGE("%s: wrong type: %{public}d", __func__, type);
            return HDF_ERR_NOT_SUPPORT;
    }
    return HDF_SUCCESS;
}

int32_t CheckCameraDevice(const char *deviceName, int type)
{
    int32_t ret;
    int32_t camId = 0;
    struct CameraConfigRoot *rootConfig = NULL;

    if (deviceName == NULL) {
        HDF_LOGE("%s: get deviceName failed!", __func__);
        return HDF_FAILURE;
    }
    rootConfig = HdfCameraGetConfigRoot();
    if (rootConfig == NULL) {
        HDF_LOGE("%s: get rootConfig failed!", __func__);
        return HDF_FAILURE;
    }
    ret = GetCameraId(deviceName, strlen(deviceName), &camId);
    CHECK_RETURN_RET(ret);

    ret = CheckSwitchType(type, rootConfig, camId);
    return ret;
}

struct SensorDevice *GetSensorDevice(const char *kernelDrvName, struct CameraDeviceDriver *regDev)
{
    int32_t i;
    for (i = 0; i < DEVICE_NUM; i++) {
        if (regDev->sensor[i] == NULL) {
            continue;
        }
        if (strcmp(regDev->sensor[i]->kernelDrvName, kernelDrvName) == 0) {
            return regDev->sensor[i];
        }
    }
    return NULL;
}

struct IspDevice *GetIspDevice(const char *kernelDrvName, struct CameraDeviceDriver *regDev)
{
    int32_t i;
    for (i = 0; i < DEVICE_NUM; i++) {
        if (regDev->isp[i] == NULL) {
            continue;
        }
        if (strcmp(regDev->isp[i]->kernelDrvName, kernelDrvName) == 0) {
            return regDev->isp[i];
        }
    }
    return NULL;
}

struct LensDevice *GetLensDevice(const char *kernelDrvName, struct CameraDeviceDriver *regDev)
{
    int32_t i;
    for (i = 0; i < DEVICE_NUM; i++) {
        if (regDev->lens[i] == NULL) {
            continue;
        }
        if (strcmp(regDev->lens[i]->kernelDrvName, kernelDrvName) == 0) {
            return regDev->lens[i];
        }
    }
    return NULL;
}

struct FlashDevice *GetFlashDevice(const char *kernelDrvName, struct CameraDeviceDriver *regDev)
{
    int32_t i;
    for (i = 0; i < DEVICE_NUM; i++) {
        if (regDev->flash[i] == NULL) {
            continue;
        }
        if (strcmp(regDev->flash[i]->kernelDrvName, kernelDrvName) == 0) {
            return regDev->flash[i];
        }
    }
    return NULL;
}

struct VcmDevice *GetVcmDevice(const char *kernelDrvName, struct CameraDeviceDriver *regDev)
{
    int32_t i;
    for (i = 0; i < DEVICE_NUM; i++) {
        if (regDev->vcm[i] == NULL) {
            continue;
        }
        if (strcmp(regDev->vcm[i]->kernelDrvName, kernelDrvName) == 0) {
            return regDev->vcm[i];
        }
    }
    return NULL;
}

struct UvcDevice *GetUvcDevice(const char *kernelDrvName, struct CameraDeviceDriver *regDev)
{
    int32_t i;
    for (i = 0; i < DEVICE_NUM; i++) {
        if (regDev->uvc[i] == NULL) {
            continue;
        }
        if (strcmp(regDev->uvc[i]->kernelDrvName, kernelDrvName) == 0) {
            return regDev->uvc[i];
        }
    }
    return NULL;
}

struct StreamDevice *GetStreamDevice(const char *kernelDrvName, struct CameraDeviceDriver *regDev)
{
    int32_t i;
    for (i = 0; i < DEVICE_NUM; i++) {
        if (regDev->stream[i] == NULL) {
            continue;
        }
        if (strcmp(regDev->stream[i]->kernelDrvName, kernelDrvName) == 0) {
            return regDev->stream[i];
        }
    }
    return NULL;
}

static int32_t GetSensorNum(const char *driverName, int camId, struct CameraConfigRoot *rootConfig)
{
    int32_t i, j;
    for (i = 0; i < rootConfig->deviceNum; i++) {
        for (j = 0; j < rootConfig->deviceConfig[camId].sensor.sensorNum; j++) {
            if (strcmp(rootConfig->deviceConfig[camId].sensor.sensor[j].name, driverName) == 0) {
                return j;
            }
        }
    }
    return HDF_FAILURE;
}

static int32_t GetIspNum(const char *driverName, int camId, struct CameraConfigRoot *rootConfig)
{
    int32_t i, j;
    for (i = 0; i < rootConfig->deviceNum; i++) {
        for (j = 0; j < rootConfig->deviceConfig[camId].isp.ispNum; j++) {
            if (strcmp(rootConfig->deviceConfig[camId].isp.isp[j].name, driverName) == 0) {
                return j;
            }
        }
    }
    return HDF_FAILURE;
}

static int32_t GetVcmNum(const char *driverName, int camId, struct CameraConfigRoot *rootConfig)
{
    int32_t i, j;
    for (i = 0; i < rootConfig->deviceNum; i++) {
        for (j = 0; j < rootConfig->deviceConfig[camId].vcm.vcmNum; j++) {
            if (strcmp(rootConfig->deviceConfig[camId].vcm.vcm[j].name, driverName) == 0) {
                return j;
            }
        }
    }
    return HDF_FAILURE;
}

static int32_t GetLensNum(const char *driverName, int camId, struct CameraConfigRoot *rootConfig)
{
    int32_t i, j;
    for (i = 0; i < rootConfig->deviceNum; i++) {
        for (j = 0; j < rootConfig->deviceConfig[camId].lens.lensNum; j++) {
            if (strcmp(rootConfig->deviceConfig[camId].lens.lens[j].name, driverName) == 0) {
                return j;
            }
        }
    }
    return HDF_FAILURE;
}

static int32_t GetFlashNum(const char *driverName, int camId, struct CameraConfigRoot *rootConfig)
{
    int32_t i, j;
    for (i = 0; i < rootConfig->deviceNum; i++) {
        for (j = 0; j < rootConfig->deviceConfig[camId].flash.flashNum; j++) {
            if (strcmp(rootConfig->deviceConfig[camId].flash.flash[j].name, driverName) == 0) {
                return j;
            }
        }
    }
    return HDF_FAILURE;
}

int32_t GetStreamNum(const char *driverName, struct CameraDeviceDriver *deviceDriver)
{
    int32_t i;
    for (i = 0; i < DEVICE_NUM; i++) {
        if (deviceDriver->stream[i] == NULL) {
            continue;
        }
        if (strcmp(deviceDriver->stream[i]->kernelDrvName, driverName) == 0) {
            return i;
        }
    }
    return HDF_FAILURE;
}

int32_t GetUvcNum(const char *driverName, struct CameraDeviceDriver *deviceDriver)
{
    int32_t i;
    for (i = 0; i < DEVICE_NUM; i++) {
        if (deviceDriver->uvc[i] == NULL) {
            continue;
        }
        if (strcmp(deviceDriver->uvc[i]->kernelDrvName, driverName) == 0) {
            return i;
        }
    }
    return HDF_FAILURE;
}

int32_t GetDeviceNum(const char *driverName, int camId, int type)
{
    int32_t ret;
    struct CameraConfigRoot *rootConfig = NULL;

    rootConfig = HdfCameraGetConfigRoot();
    if (rootConfig == NULL) {
        HDF_LOGE("%s: get rootConfig failed!", __func__);
        return HDF_FAILURE;
    }
    switch (type) {
        case SENSOR_TYPE:
            ret = GetSensorNum(driverName, camId, rootConfig);
            break;
        case ISP_TYPE:
            ret = GetIspNum(driverName, camId, rootConfig);
            break;
        case VCM_TYPE:
            ret = GetVcmNum(driverName, camId, rootConfig);
            break;
        case LENS_TYPE:
            ret = GetLensNum(driverName, camId, rootConfig);
            break;
        case FLASH_TYPE:
            ret = GetFlashNum(driverName, camId, rootConfig);
            break;
        case UVC_TYPE:
            ret = UVC_DEVICE_ID;
            break;
        default:
            HDF_LOGE("%s: wrong type: %{public}d", __func__, type);
            return HDF_ERR_NOT_SUPPORT;
    }
    return ret;
}

int32_t CheckFrameRate(int camId, const char *driverName, int type, struct CameraCtrlConfig *ctrlConfig)
{
    int32_t fps;
    int32_t deviceNum;
    struct CameraConfigRoot *rootConfig = NULL;

    rootConfig = HdfCameraGetConfigRoot();
    if (rootConfig == NULL) {
        HDF_LOGE("%s: get rootConfig failed!", __func__);
        return HDF_FAILURE;
    }
    deviceNum = GetDeviceNum(driverName, camId, type);
    if (deviceNum < 0 || deviceNum >= DEVICE_NUM) {
        return HDF_ERR_INVALID_PARAM;
    }
    if (ctrlConfig->pixelFormat.fps.numerator == 0 || ctrlConfig->pixelFormat.fps.denominator == 0) {
        HDF_LOGE("%s: wrong numerator %{public}d or wrong denominator %{public}d!", __func__,
            ctrlConfig->pixelFormat.fps.numerator, ctrlConfig->pixelFormat.fps.denominator);
        return HDF_ERR_INVALID_PARAM;
    }
    if (FPS_CALC_COEFF > UINT_MAX_VAL / ctrlConfig->pixelFormat.fps.denominator) {
        HDF_LOGE("%s: wrong num!", __func__);
        return HDF_ERR_INVALID_PARAM;
    }
    fps = FPS_CALC_COEFF * (ctrlConfig->pixelFormat.fps.denominator) / ctrlConfig->pixelFormat.fps.numerator;

    switch (type) {
        case STREAM_TYPE:
            if (fps > FPS_CALC_COEFF * (rootConfig->deviceConfig[camId].stream.stream[deviceNum].frameRateMaxNum)) {
                HDF_LOGE("%s: stream%{public}d wrong fps = %{public}d", __func__, deviceNum, fps);
                return HDF_ERR_INVALID_PARAM;
            }
            break;
        default:
            HDF_LOGE("%s: wrong type: %{public}d", __func__, type);
            return HDF_ERR_NOT_SUPPORT;
    }
    return HDF_SUCCESS;
}

int32_t CameraGetDeviceInfo(int type, struct HdfSBuf *reqData,
    const char **driverName, int32_t *camId, struct CameraDevice **camDev)
{
    int32_t ret;
    const char *deviceName = NULL;

    deviceName = HdfSbufReadString(reqData);
    if (deviceName == NULL) {
        HDF_LOGE("%s: Read deviceName failed!", __func__);
        return HDF_FAILURE;
    }

    ret = GetCameraId(deviceName, strlen(deviceName), camId);
    CHECK_RETURN_RET(ret);
    if ((*camId) > CAMERA_DEVICE_MAX_NUM) {
        HDF_LOGE("%s: wrong camId! camId=%{public}d", __func__, (*camId));
        return HDF_FAILURE;
    }

    ret = CheckCameraDevice(deviceName, type);
    CHECK_RETURN_RET(ret);

    *driverName = HdfSbufReadString(reqData);
    if (*driverName == NULL) {
        HDF_LOGE("%s: Read driverName failed!", __func__);
        return HDF_FAILURE;
    }

    *camDev = CameraDeviceGetByName(deviceName);
    if (*camDev == NULL) {
        HDF_LOGE("%s: camDev not found! deviceName=%{public}s", __func__, deviceName);
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

int32_t CameraGetNames(int type, struct HdfSBuf *reqData, const char **deviceName, const char **driverName)
{
    int32_t ret;

    *deviceName = HdfSbufReadString(reqData);
    if (*deviceName == NULL) {
        HDF_LOGE("%s: Read deviceName failed!", __func__);
        return HDF_FAILURE;
    }
    ret = CheckCameraDevice(*deviceName, type);
    CHECK_RETURN_RET(ret);
    *driverName = HdfSbufReadString(reqData);
    if (*driverName == NULL) {
        HDF_LOGE("%s: Read driverName failed!", __func__);
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

int32_t GetDeviceOps(struct CameraDeviceDriver *deviceDriver,
    int type, int32_t camId, const char *driverName, struct DeviceOps **devOps)
{
    int32_t num;

    num = GetDeviceNum(driverName, camId, type);
    if (num < 0 || num >= DEVICE_NUM) {
        HDF_LOGE("%s: wrong num: %{public}d", __func__, num);
        return HDF_ERR_INVALID_PARAM;
    }
    switch (type) {
        case SENSOR_TYPE:
            *devOps = deviceDriver->sensor[num]->devOps;
            break;
        case ISP_TYPE:
            *devOps = deviceDriver->isp[num]->devOps;
            break;
        case VCM_TYPE:
            *devOps = deviceDriver->vcm[num]->devOps;
            break;
        case LENS_TYPE:
            *devOps = deviceDriver->lens[num]->devOps;
            break;
        case FLASH_TYPE:
            *devOps = deviceDriver->flash[num]->devOps;
            break;
        case UVC_TYPE:
            *devOps = deviceDriver->uvc[num]->devOps;
            break;
        default:
            HDF_LOGE("%s: wrong type: %{public}d", __func__, type);
            return HDF_ERR_NOT_SUPPORT;
    }
    return HDF_SUCCESS;
}

int32_t CameraDeviceGetCtrlConfig(struct CommonDevice *comDev,
    struct CameraDeviceConfig **deviceConfig, int32_t *deviceId)
{
    int32_t num;
    struct CameraConfigRoot *rootConfig = NULL;

    rootConfig = HdfCameraGetConfigRoot();
    if (rootConfig == NULL) {
        HDF_LOGE("%s: get rootConfig failed!", __func__);
        return HDF_FAILURE;
    }
    if (comDev->camId > CAMERA_DEVICE_MAX_NUM) {
        HDF_LOGE("%s: wrong camId! camId=%{public}d", __func__, comDev->camId);
        return HDF_FAILURE;
    }
    *deviceConfig  = &rootConfig->deviceConfig[comDev->camId];
    num = GetDeviceNum(comDev->driverName, comDev->camId, comDev->type);
    if (num < 0 || num >= DEVICE_NUM) {
        return HDF_ERR_INVALID_PARAM;
    }
    *deviceId = num;
    return HDF_SUCCESS;
}

int32_t CameraGetDevice(struct HdfSBuf *reqData, struct CommonDevice *comDev)
{
    int32_t ret;

    if (!HdfSbufReadInt32(reqData, &comDev->type)) {
        HDF_LOGE("%s: Read request data failed! type = %{public}d", __func__, comDev->type);
        return HDF_FAILURE;
    }
    if (!HdfSbufReadInt32(reqData, &comDev->permissionId)) {
        HDF_LOGE("%s: Read request data failed! permissionId = %{public}d", __func__, comDev->permissionId);
        return HDF_FAILURE;
    }
    ret = CheckPermission(comDev->permissionId);
    CHECK_RETURN_RET(ret);
    ret = CameraGetDeviceInfo(comDev->type, reqData, &comDev->driverName, &comDev->camId, &comDev->camDev);
    CHECK_RETURN_RET(ret);
    return HDF_SUCCESS;
}

int32_t CameraDispatchCommonInfo(const struct HdfDeviceIoClient *client,
    struct HdfSBuf *reqData, struct HdfSBuf *rspData, struct CommonDevice *comDev)
{
    int32_t ret;

    if (client == NULL || reqData == NULL || rspData == NULL) {
        return HDF_ERR_INVALID_PARAM;
    }
    ret = CameraGetDevice(reqData, comDev);
    CHECK_RETURN_RET(ret);
    comDev->reqData = reqData;
    comDev->rspData = rspData;
    return HDF_SUCCESS;
}