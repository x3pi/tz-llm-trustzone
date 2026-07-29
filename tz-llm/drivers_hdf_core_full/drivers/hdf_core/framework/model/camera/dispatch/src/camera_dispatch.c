/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include <securec.h>
#include <hdf_log.h>
#include "camera_common_device.h"
#include "camera_stream.h"
#include "camera_sensor.h"
#include "camera_isp.h"
#include "camera_vcm.h"
#include "camera_flash.h"
#include "camera_lens.h"
#include "camera_uvc.h"
#include "camera_config_parser.h"
#include "camera_device_manager.h"
#include "camera_utils.h"
#include "camera_dispatch.h"

#define HDF_LOG_TAG HDF_CAMERA_DISPATCH

static int32_t CheckDeviceOps(int camId, struct CameraDeviceDriver *deviceDriver, struct CameraConfigRoot *rootConfig)
{
    int32_t i;
    if (rootConfig->deviceConfig[camId].sensor.mode == DEVICE_SUPPORT) {
        for (i = 0; i < rootConfig->deviceConfig[camId].sensor.sensorNum; i++) {
            if (deviceDriver->sensor[i]->devOps == NULL) {
                HDF_LOGE("%s: sensorOps is null!", __func__);
                return HDF_FAILURE;
            }
        }
    }
    if (rootConfig->deviceConfig[camId].isp.mode == DEVICE_SUPPORT) {
        for (i = 0; i < rootConfig->deviceConfig[camId].isp.ispNum; i++) {
            if (deviceDriver->isp[i]->devOps == NULL) {
                HDF_LOGE("%s: ispOps is null!", __func__);
                return HDF_FAILURE;
            }
        }
    }
    if (rootConfig->deviceConfig[camId].lens.mode == DEVICE_SUPPORT) {
        for (i = 0; i < rootConfig->deviceConfig[camId].lens.lensNum; i++) {
            if (deviceDriver->lens[i]->devOps == NULL) {
                HDF_LOGE("%s: lensOps is null!", __func__);
                return HDF_FAILURE;
            }
        }
    }
    if (rootConfig->deviceConfig[camId].vcm.mode == DEVICE_SUPPORT) {
        for (i = 0; i < rootConfig->deviceConfig[camId].vcm.vcmNum; i++) {
            if (deviceDriver->vcm[i]->devOps == NULL) {
                HDF_LOGE("%s: vcmOps is null!", __func__);
                return HDF_FAILURE;
            }
        }
    }
    if (rootConfig->deviceConfig[camId].flash.mode == DEVICE_SUPPORT) {
        for (i = 0; i < rootConfig->deviceConfig[camId].flash.flashNum; i++) {
            if (deviceDriver->flash[i]->devOps == NULL) {
                HDF_LOGE("%s: flashOps is null!", __func__);
                return HDF_FAILURE;
            }
        }
    }
    if (rootConfig->deviceConfig[camId].stream.mode == DEVICE_SUPPORT) {
        for (i = 0; i < rootConfig->deviceConfig[camId].stream.streamNum; i++) {
            if (deviceDriver->stream[i]->streamOps == NULL) {
                HDF_LOGE("%s: streamOps is null!", __func__);
                return HDF_FAILURE;
            }
        }
    }
    return HDF_SUCCESS;
}

static int32_t CameraOpenCamera(struct HdfDeviceIoClient *client, struct HdfSBuf *reqData, struct HdfSBuf *rspData)
{
    int32_t ret;
    int32_t camId = 0;
    const char *driverName = NULL;
    const char *deviceName = NULL;
    struct CameraDevice *camDev = NULL;
    struct CameraConfigRoot *rootConfig = NULL;

    rootConfig = HdfCameraGetConfigRoot();
    if (rootConfig == NULL) {
        HDF_LOGE("%s: get rootConfig failed!", __func__);
        return HDF_FAILURE;
    }
    deviceName = HdfSbufReadString(reqData);
    if (deviceName == NULL) {
        HDF_LOGE("%s: fail to get deviceName!", __func__);
        return HDF_FAILURE;
    }
    ret = GetCameraId(deviceName, strlen(deviceName), &camId);
    CHECK_RETURN_RET(ret);
    if (camId > CAMERA_DEVICE_MAX_NUM) {
        HDF_LOGE("%s: wrong camId, camId = %{public}d", __func__, camId);
        return HDF_FAILURE;
    }
    driverName = HdfSbufReadString(reqData);
    if (driverName == NULL) {
        HDF_LOGE("%s: Read driverName failed!", __func__);
        return HDF_FAILURE;
    }

    camDev = CameraDeviceGetByName(deviceName);
    if (camDev == NULL) {
        HDF_LOGE("%s: camDev not found! deviceName=%{public}s", __func__, deviceName);
        return HDF_FAILURE;
    }

    struct CameraDeviceDriver *deviceDriver = GetDeviceDriver(camDev);
    if (deviceDriver == NULL) {
        HDF_LOGE("%s: deviceDriver ptr is null!", __func__);
        return HDF_FAILURE;
    }
    ret = CheckDeviceOps(camId, deviceDriver, rootConfig);

    return ret;
}

static int32_t CameraCmdOpenCamera(struct HdfDeviceIoClient *client, struct HdfSBuf *reqData, struct HdfSBuf *rspData)
{
    int32_t ret;
    uint32_t index = 0;
    int32_t permissionId = 0;

    if (client == NULL || reqData == NULL || rspData == NULL) {
        return HDF_ERR_INVALID_PARAM;
    }
    if (!HdfSbufReadInt32(reqData, &permissionId)) {
        HDF_LOGE("%s: Read request data failed! permissionId = %{public}d", __func__, permissionId);
        return HDF_FAILURE;
    }
    if (FindAvailableId(&index)) {
        AddPermissionId(index, permissionId);
    } else {
        HDF_LOGE("%s: AddPermissionId failed!", __func__);
        return HDF_FAILURE;
    }

    ret = CameraOpenCamera(client, reqData, rspData);

    return ret;
}

static int32_t CameraCmdCloseCamera(const struct HdfDeviceIoClient *client,
    struct HdfSBuf *reqData, const struct HdfSBuf *rspData)
{
    int32_t ret;
    int32_t permissionId = 0;
    const char *deviceName = NULL;

    if (client == NULL || reqData == NULL || rspData == NULL) {
        return HDF_ERR_INVALID_PARAM;
    }
    if (!HdfSbufReadInt32(reqData, &permissionId)) {
        HDF_LOGE("%s: Read request data failed! permissionId = %{public}d", __func__, permissionId);
        return HDF_FAILURE;
    }
    ret = CheckPermission(permissionId);
    CHECK_RETURN_RET(ret);
    deviceName = HdfSbufReadString(reqData);
    if (deviceName == NULL) {
        HDF_LOGE("%s: fail to get deviceName!", __func__);
        return HDF_FAILURE;
    }
    ret = RemovePermissionId(permissionId);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: RemovePermissionId failed, ret=%{public}d", __func__, ret);
        return ret;
    }

    return HDF_SUCCESS;
}

static int32_t CameraCmdPowerUp(struct HdfDeviceIoClient *client, struct HdfSBuf *reqData, struct HdfSBuf *rspData)
{
    int32_t ret;
    struct CommonDevice comDev;

    ret = CameraDispatchCommonInfo(client, reqData, rspData, &comDev);
    CHECK_RETURN_RET(ret);
    ret = CommonDevicePowerOperation(&comDev, CAMERA_DEVICE_POWER_UP);
    CHECK_RETURN_RET(ret);
    return HDF_SUCCESS;
}

static int32_t CameraCmdPowerDown(struct HdfDeviceIoClient *client, struct HdfSBuf *reqData, struct HdfSBuf *rspData)
{
    int32_t ret;
    struct CommonDevice comDev;

    ret = CameraDispatchCommonInfo(client, reqData, rspData, &comDev);
    CHECK_RETURN_RET(ret);
    ret = CommonDevicePowerOperation(&comDev, CAMERA_DEVICE_POWER_DOWN);
    CHECK_RETURN_RET(ret);
    return HDF_SUCCESS;
}

static int32_t CameraCmdQueryConfig(struct HdfDeviceIoClient *client, struct HdfSBuf *reqData, struct HdfSBuf *rspData)
{
    int32_t ret;
    struct CommonDevice comDev;

    ret = CameraDispatchCommonInfo(client, reqData, rspData, &comDev);
    CHECK_RETURN_RET(ret);
    ret = CommonDeviceQueryConfig(&comDev);
    CHECK_RETURN_RET(ret);
    return HDF_SUCCESS;
}

static int32_t CameraCmdEnumDevice(struct HdfDeviceIoClient *client, struct HdfSBuf *reqData, struct HdfSBuf *rspData)
{
    int32_t ret;
    struct CommonDevice comDev;

    ret = CameraDispatchCommonInfo(client, reqData, rspData, &comDev);
    CHECK_RETURN_RET(ret);
    ret = CommonDeviceEnumDevice(&comDev);
    CHECK_RETURN_RET(ret);
    return HDF_SUCCESS;
}

static int32_t CameraCmdGetConfig(struct HdfDeviceIoClient *client, struct HdfSBuf *reqData, struct HdfSBuf *rspData)
{
    int32_t ret;
    struct CommonDevice comDev;

    ret = CameraDispatchCommonInfo(client, reqData, rspData, &comDev);
    CHECK_RETURN_RET(ret);
    ret = CommonDeviceGetConfig(&comDev);
    CHECK_RETURN_RET(ret);
    return HDF_SUCCESS;
}

static int32_t CameraCmdSetConfig(struct HdfDeviceIoClient *client, struct HdfSBuf *reqData, struct HdfSBuf *rspData)
{
    int32_t ret;
    struct CommonDevice comDev;

    ret = CameraDispatchCommonInfo(client, reqData, rspData, &comDev);
    CHECK_RETURN_RET(ret);
    ret = CommonDeviceSetConfig(&comDev);
    CHECK_RETURN_RET(ret);
    return HDF_SUCCESS;
}

static int32_t CameraCmdGetFormat(struct HdfDeviceIoClient *client, struct HdfSBuf *reqData, struct HdfSBuf *rspData)
{
    int32_t ret;
    struct CommonDevice comDev;
    
    ret = CameraDispatchCommonInfo(client, reqData, rspData, &comDev);
    CHECK_RETURN_RET(ret);
    if (comDev.type != STREAM_TYPE) {
        HDF_LOGE("%s: CameraGetFormat wrong type: %{public}d", __func__, comDev.type);
        return HDF_ERR_INVALID_PARAM;
    }
    ret = CameraGetFormat(&comDev);
    CHECK_RETURN_RET(ret);
    return HDF_SUCCESS;
}

static int32_t CameraCmdSetFormat(struct HdfDeviceIoClient *client, struct HdfSBuf *reqData, struct HdfSBuf *rspData)
{
    int32_t ret;
    struct CommonDevice comDev;
    
    ret = CameraDispatchCommonInfo(client, reqData, rspData, &comDev);
    CHECK_RETURN_RET(ret);
    if (comDev.type != STREAM_TYPE) {
        HDF_LOGE("%s: CameraSetFormat wrong type: %{public}d", __func__, comDev.type);
        return HDF_ERR_INVALID_PARAM;
    }
    ret = CameraSetFormat(&comDev);
    CHECK_RETURN_RET(ret);
    return HDF_SUCCESS;
}

static int32_t CameraCmdGetCrop(struct HdfDeviceIoClient *client, struct HdfSBuf *reqData, struct HdfSBuf *rspData)
{
    int32_t ret;
    struct CommonDevice comDev;
    
    ret = CameraDispatchCommonInfo(client, reqData, rspData, &comDev);
    CHECK_RETURN_RET(ret);
    if (comDev.type != STREAM_TYPE) {
        HDF_LOGE("%s: CameraGetCrop wrong type: %{public}d", __func__, comDev.type);
        return HDF_ERR_INVALID_PARAM;
    }
    ret = CameraGetCrop(&comDev);
    CHECK_RETURN_RET(ret);
    return HDF_SUCCESS;
}

static int32_t CameraCmdSetCrop(struct HdfDeviceIoClient *client, struct HdfSBuf *reqData, struct HdfSBuf *rspData)
{
    int32_t ret;
    struct CommonDevice comDev;
    
    ret = CameraDispatchCommonInfo(client, reqData, rspData, &comDev);
    CHECK_RETURN_RET(ret);
    if (comDev.type != STREAM_TYPE) {
        HDF_LOGE("%s: CameraSetCrop wrong type: %{public}d", __func__, comDev.type);
        return HDF_ERR_INVALID_PARAM;
    }
    ret = CameraSetCrop(&comDev);
    CHECK_RETURN_RET(ret);
    return HDF_SUCCESS;
}

static int32_t CameraCmdGetFPS(struct HdfDeviceIoClient *client, struct HdfSBuf *reqData, struct HdfSBuf *rspData)
{
    int32_t ret;
    struct CommonDevice comDev;
    
    ret = CameraDispatchCommonInfo(client, reqData, rspData, &comDev);
    CHECK_RETURN_RET(ret);
    if (comDev.type != STREAM_TYPE) {
        HDF_LOGE("%s: CameraGetFPS wrong type: %{public}d", __func__, comDev.type);
        return HDF_ERR_INVALID_PARAM;
    }
    ret = CameraGetFPS(&comDev);
    CHECK_RETURN_RET(ret);
    return HDF_SUCCESS;
}

static int32_t CameraCmdSetFPS(struct HdfDeviceIoClient *client, struct HdfSBuf *reqData, struct HdfSBuf *rspData)
{
    int32_t ret;
    struct CommonDevice comDev;
    
    ret = CameraDispatchCommonInfo(client, reqData, rspData, &comDev);
    CHECK_RETURN_RET(ret);
    if (comDev.type != STREAM_TYPE) {
        HDF_LOGE("%s: CameraSetFPS wrong type: %{public}d", __func__, comDev.type);
        return HDF_ERR_INVALID_PARAM;
    }
    ret = CameraSetFPS(&comDev);
    CHECK_RETURN_RET(ret);
    return HDF_SUCCESS;
}

static int32_t CameraCmdEnumFmt(struct HdfDeviceIoClient *client, struct HdfSBuf *reqData, struct HdfSBuf *rspData)
{
    int32_t ret;
    struct CommonDevice comDev;
    
    ret = CameraDispatchCommonInfo(client, reqData, rspData, &comDev);
    CHECK_RETURN_RET(ret);
    if (comDev.type != STREAM_TYPE) {
        HDF_LOGE("%s: CameraEnumFmt wrong type: %{public}d", __func__, comDev.type);
        return HDF_ERR_INVALID_PARAM;
    }
    ret = CameraEnumFmt(&comDev);
    CHECK_RETURN_RET(ret);
    return HDF_SUCCESS;
}

static int32_t CameraCmdGetAbility(struct HdfDeviceIoClient *client, struct HdfSBuf *reqData, struct HdfSBuf *rspData)
{
    int32_t ret;
    struct CommonDevice comDev;
    
    ret = CameraDispatchCommonInfo(client, reqData, rspData, &comDev);
    CHECK_RETURN_RET(ret);
    if (comDev.type != STREAM_TYPE) {
        HDF_LOGE("%s: CameraStreamGetAbility wrong type: %{public}d", __func__, comDev.type);
        return HDF_ERR_INVALID_PARAM;
    }
    ret = CameraStreamGetAbility(&comDev);
    CHECK_RETURN_RET(ret);
    return HDF_SUCCESS;
}

static int32_t CameraCmdQueueInit(struct HdfDeviceIoClient *client, struct HdfSBuf *reqData, struct HdfSBuf *rspData)
{
    int32_t ret;
    struct CommonDevice comDev;
    
    ret = CameraDispatchCommonInfo(client, reqData, rspData, &comDev);
    CHECK_RETURN_RET(ret);
    if (comDev.type != STREAM_TYPE) {
        HDF_LOGE("%s: CameraQueueInit wrong type: %{public}d", __func__, comDev.type);
        return HDF_ERR_INVALID_PARAM;
    }
    ret = CameraQueueInit(&comDev);
    CHECK_RETURN_RET(ret);
    return HDF_SUCCESS;
}

static int32_t CameraCmdReqMemory(struct HdfDeviceIoClient *client, struct HdfSBuf *reqData, struct HdfSBuf *rspData)
{
    int32_t ret;
    struct CommonDevice comDev;
    
    ret = CameraDispatchCommonInfo(client, reqData, rspData, &comDev);
    CHECK_RETURN_RET(ret);
    if (comDev.type != STREAM_TYPE) {
        HDF_LOGE("%s: CameraReqMemory wrong type: %{public}d", __func__, comDev.type);
        return HDF_ERR_INVALID_PARAM;
    }
    ret = CameraReqMemory(&comDev);
    CHECK_RETURN_RET(ret);
    return HDF_SUCCESS;
}

static int32_t CameraCmdQueryMemory(struct HdfDeviceIoClient *client, struct HdfSBuf *reqData, struct HdfSBuf *rspData)
{
    int32_t ret;
    struct CommonDevice comDev;
    
    ret = CameraDispatchCommonInfo(client, reqData, rspData, &comDev);
    CHECK_RETURN_RET(ret);
    if (comDev.type != STREAM_TYPE) {
        HDF_LOGE("%s: CameraQueryMemory wrong type: %{public}d", __func__, comDev.type);
        return HDF_ERR_INVALID_PARAM;
    }
    ret = CameraQueryMemory(&comDev);
    CHECK_RETURN_RET(ret);
    return HDF_SUCCESS;
}

static int32_t CameraCmdStreamQueue(struct HdfDeviceIoClient *client, struct HdfSBuf *reqData, struct HdfSBuf *rspData)
{
    int32_t ret;
    struct CommonDevice comDev;
    
    ret = CameraDispatchCommonInfo(client, reqData, rspData, &comDev);
    CHECK_RETURN_RET(ret);
    if (comDev.type != STREAM_TYPE) {
        HDF_LOGE("%s: CameraStreamQueue wrong type: %{public}d", __func__, comDev.type);
        return HDF_ERR_INVALID_PARAM;
    }
    ret = CameraStreamQueue(&comDev);
    CHECK_RETURN_RET(ret);
    return HDF_SUCCESS;
}

static int32_t CameraCmdStreamDeQueue(struct HdfDeviceIoClient *client,
    struct HdfSBuf *reqData, struct HdfSBuf *rspData)
{
    int32_t ret;
    struct CommonDevice comDev;
    
    ret = CameraDispatchCommonInfo(client, reqData, rspData, &comDev);
    CHECK_RETURN_RET(ret);
    if (comDev.type != STREAM_TYPE) {
        HDF_LOGE("%s: CameraStreamDeQueue wrong type: %{public}d", __func__, comDev.type);
        return HDF_ERR_INVALID_PARAM;
    }
    ret = CameraStreamDeQueue(&comDev);
    CHECK_RETURN_RET(ret);
    return HDF_SUCCESS;
}

static int32_t CameraCmdStreamOn(struct HdfDeviceIoClient *client, struct HdfSBuf *reqData, struct HdfSBuf *rspData)
{
    int32_t ret;
    struct CommonDevice comDev;
    
    ret = CameraDispatchCommonInfo(client, reqData, rspData, &comDev);
    CHECK_RETURN_RET(ret);
    if (comDev.type != STREAM_TYPE) {
        HDF_LOGE("%s: CameraStreamOn wrong type: %{public}d", __func__, comDev.type);
        return HDF_ERR_INVALID_PARAM;
    }
    ret = CameraStreamOn(&comDev);
    CHECK_RETURN_RET(ret);
    return HDF_SUCCESS;
}

static int32_t CameraCmdStreamOff(struct HdfDeviceIoClient *client, struct HdfSBuf *reqData, struct HdfSBuf *rspData)
{
    int32_t ret;
    struct CommonDevice comDev;
    
    ret = CameraDispatchCommonInfo(client, reqData, rspData, &comDev);
    CHECK_RETURN_RET(ret);
    if (comDev.type != STREAM_TYPE) {
        HDF_LOGE("%s: CameraStreamOff wrong type: %{public}d", __func__, comDev.type);
        return HDF_ERR_INVALID_PARAM;
    }
    ret = CameraStreamOff(&comDev);
    CHECK_RETURN_RET(ret);
    return HDF_SUCCESS;
}

static struct CameraCmdHandle g_cameraCmdHandle[CAMERA_MAX_CMD_ID] = {
    {CMD_OPEN_CAMERA, CameraCmdOpenCamera},
    {CMD_CLOSE_CAMERA, CameraCmdCloseCamera},
    {CMD_POWER_UP, CameraCmdPowerUp},
    {CMD_POWER_DOWN, CameraCmdPowerDown},
    {CMD_QUERY_CONFIG, CameraCmdQueryConfig},
    {CMD_GET_CONFIG, CameraCmdGetConfig},
    {CMD_SET_CONFIG, CameraCmdSetConfig},
    {CMD_GET_FMT, CameraCmdGetFormat},
    {CMD_SET_FMT, CameraCmdSetFormat},
    {CMD_GET_CROP, CameraCmdGetCrop},
    {CMD_SET_CROP, CameraCmdSetCrop},
    {CMD_GET_FPS, CameraCmdGetFPS},
    {CMD_SET_FPS, CameraCmdSetFPS},
    {CMD_ENUM_FMT, CameraCmdEnumFmt},
    {CMD_ENUM_DEVICES, CameraCmdEnumDevice},
    {CMD_GET_ABILITY, CameraCmdGetAbility},
    {CMD_QUEUE_INIT, CameraCmdQueueInit},
    {CMD_REQ_MEMORY, CameraCmdReqMemory},
    {CMD_QUERY_MEMORY, CameraCmdQueryMemory},
    {CMD_STREAM_QUEUE, CameraCmdStreamQueue},
    {CMD_STREAM_DEQUEUE, CameraCmdStreamDeQueue},
    {CMD_STREAM_ON, CameraCmdStreamOn},
    {CMD_STREAM_OFF, CameraCmdStreamOff},
};

int32_t HdfCameraDispatch(struct HdfDeviceIoClient *client, int cmdId,
    struct HdfSBuf *reqData, struct HdfSBuf *rspData)
{
    uint32_t i;
    HDF_LOGD("%s: [cmdId = %{public}d].", __func__, cmdId);
    if (client == NULL || reqData == NULL) {
        HDF_LOGE("%s: Input params check error", __func__);
        return HDF_FAILURE;
    }

    if ((cmdId >= CAMERA_MAX_CMD_ID) || (cmdId < 0)) {
        HDF_LOGE("%s: Invalid [cmdId=%{public}d].", __func__, cmdId);
        return HDF_ERR_INVALID_PARAM;
    }

    for (i = 0; i < HDF_ARRAY_SIZE(g_cameraCmdHandle); ++i) {
        if ((cmdId == (int)(g_cameraCmdHandle[i].cmd)) && (g_cameraCmdHandle[i].func != NULL)) {
            return g_cameraCmdHandle[i].func(client, reqData, rspData);
        }
    }
    return HDF_FAILURE;
}