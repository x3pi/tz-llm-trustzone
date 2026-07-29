/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include <securec.h>
#include <hdf_log.h>
#include <camera/camera_product.h>
#include "camera_buffer_manager.h"
#include "camera_config_parser.h"
#include "camera_utils.h"
#include "camera_stream.h"

#define HDF_LOG_TAG HDF_CAMERA_STREAM

static int32_t StreamGetImageSetting(struct CommonDevice *comDev, struct CameraCtrlConfig *ctrlConfig, int cmd)
{
    int32_t num = 0;
    struct StreamOps *streamOps = NULL;
    struct CameraDeviceDriver *deviceDriver = GetDeviceDriver(comDev->camDev);
    if (deviceDriver == NULL) {
        HDF_LOGE("%s: deviceDriver ptr is null!", __func__);
        return HDF_FAILURE;
    }
    num = GetStreamNum(comDev->driverName, deviceDriver);
    if (num < 0 || num >= DEVICE_NUM) {
        return HDF_ERR_INVALID_PARAM;
    }
    if (deviceDriver->stream[num]->streamOps == NULL) {
        HDF_LOGE("%s: streamOps ptr is null!", __func__);
        return HDF_FAILURE;
    }
    streamOps = deviceDriver->stream[num]->streamOps;
    switch (cmd) {
        case CMD_GET_FMT:
            if (streamOps->streamGetFormat == NULL) {
                HDF_LOGE("%s: streamGetFormat ptr is null!", __func__);
                return HDF_FAILURE;
            }
            return streamOps->streamGetFormat(ctrlConfig, deviceDriver->stream[num]);
        case CMD_GET_CROP:
            if (streamOps->streamGetCrop == NULL) {
                HDF_LOGE("%s: streamGetCrop ptr is null!", __func__);
                return HDF_FAILURE;
            }
            return streamOps->streamGetCrop(ctrlConfig, deviceDriver->stream[num]);
        case CMD_GET_FPS:
            if (streamOps->streamGetFps == NULL) {
                HDF_LOGE("%s: streamGetFps ptr is null!", __func__);
                return HDF_FAILURE;
            }
            return streamOps->streamGetFps(ctrlConfig, deviceDriver->stream[num]);
        default:
            HDF_LOGE("%s: wrong cmd: %{public}d", __func__, cmd);
            return HDF_ERR_NOT_SUPPORT;
    }
}

static int32_t StreamSetImageSetting(struct CommonDevice *comDev, struct CameraCtrlConfig *ctrlConfig, int cmd)
{
    int32_t num = 0;
    struct StreamOps *streamOps = NULL;
    struct CameraDeviceDriver *deviceDriver = GetDeviceDriver(comDev->camDev);
    if (deviceDriver == NULL) {
        HDF_LOGE("%s: deviceDriver ptr is null!", __func__);
        return HDF_FAILURE;
    }
    num = GetStreamNum(comDev->driverName, deviceDriver);
    if (num < 0 || num >= DEVICE_NUM) {
        return HDF_ERR_INVALID_PARAM;
    }
    if (deviceDriver->stream[num]->streamOps == NULL) {
        HDF_LOGE("%s: streamOps ptr is null!", __func__);
        return HDF_FAILURE;
    }
    streamOps = deviceDriver->stream[num]->streamOps;
    switch (cmd) {
        case CMD_SET_FMT:
            if (streamOps->streamSetFormat == NULL) {
                HDF_LOGE("%s: streamSetFormat ptr is null!", __func__);
                return HDF_FAILURE;
            }
            return streamOps->streamSetFormat(ctrlConfig, deviceDriver->stream[num]);
        case CMD_SET_CROP:
            if (streamOps->streamSetCrop == NULL) {
                HDF_LOGE("%s: streamSetCrop ptr is null!", __func__);
                return HDF_FAILURE;
            }
            return streamOps->streamSetCrop(ctrlConfig, deviceDriver->stream[num]);
        case CMD_SET_FPS:
            if (streamOps->streamSetFps == NULL) {
                HDF_LOGE("%s: streamSetFps ptr is null!", __func__);
                return HDF_FAILURE;
            }
            return streamOps->streamSetFps(ctrlConfig, deviceDriver->stream[num]);
        default:
            HDF_LOGE("%s: wrong cmd: %{public}d", __func__, cmd);
            return HDF_ERR_NOT_SUPPORT;
    }
}

static int32_t CameraSendFmtData(struct HdfSBuf *rspData, struct CameraCtrlConfig ctrlConfig)
{
    bool isFailed = false;

    isFailed |= !HdfSbufWriteUint32(rspData, ctrlConfig.pixelFormat.pixel.format);
    isFailed |= !HdfSbufWriteUint32(rspData, ctrlConfig.pixelFormat.pixel.width);
    isFailed |= !HdfSbufWriteUint32(rspData, ctrlConfig.pixelFormat.pixel.height);
    isFailed |= !HdfSbufWriteUint32(rspData, ctrlConfig.pixelFormat.pixel.sizeImage);
    CHECK_RETURN_RET(isFailed);
    return HDF_SUCCESS;
}

int32_t CameraGetFormat(struct CommonDevice *comDev)
{
    int32_t ret;
    struct CameraCtrlConfig ctrlConfig = {};

    ret = StreamGetImageSetting(comDev, &ctrlConfig, CMD_GET_FMT);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: CMD_GET_FMT failed, ret = %{public}d", __func__, ret);
        return ret;
    }
    ret = CameraSendFmtData(comDev->rspData, ctrlConfig);
    CHECK_RETURN_RET(ret);
    return HDF_SUCCESS;
}

static int32_t CameraReceiveFmtData(int camId, const char *driverName,
    struct HdfSBuf *reqData, struct CameraCtrlConfig *ctrlConfig)
{
    bool isFailed = false;

    isFailed |= !HdfSbufReadUint32(reqData, &ctrlConfig->pixelFormat.pixel.format);
    isFailed |= !HdfSbufReadUint32(reqData, &ctrlConfig->pixelFormat.pixel.width);
    isFailed |= !HdfSbufReadUint32(reqData, &ctrlConfig->pixelFormat.pixel.height);
    CHECK_RETURN_RET(isFailed);
    return HDF_SUCCESS;
}

int32_t CameraSetFormat(struct CommonDevice *comDev)
{
    int32_t ret;
    struct CameraCtrlConfig ctrlConfig = {};

    ret = CameraReceiveFmtData(comDev->camId, comDev->driverName, comDev->reqData, &ctrlConfig);
    CHECK_RETURN_RET(ret);
    ret = StreamSetImageSetting(comDev, &ctrlConfig, CMD_SET_FMT);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: CMD_SET_FMT failed, ret = %{public}d", __func__, ret);
        return ret;
    }
    return HDF_SUCCESS;
}

static int32_t CameraSendCropData(struct HdfSBuf *rspData, struct CameraCtrlConfig ctrlConfig)
{
    bool isFailed = false;

    isFailed |= !HdfSbufWriteInt32(rspData, ctrlConfig.pixelFormat.crop.left);
    isFailed |= !HdfSbufWriteInt32(rspData, ctrlConfig.pixelFormat.crop.top);
    isFailed |= !HdfSbufWriteUint32(rspData, ctrlConfig.pixelFormat.crop.width);
    isFailed |= !HdfSbufWriteUint32(rspData, ctrlConfig.pixelFormat.crop.height);
    CHECK_RETURN_RET(isFailed);
    return HDF_SUCCESS;
}

int32_t CameraGetCrop(struct CommonDevice *comDev)
{
    int32_t ret;
    struct CameraCtrlConfig ctrlConfig = {};

    ret = StreamGetImageSetting(comDev, &ctrlConfig, CMD_GET_CROP);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: CMD_GET_CROP failed, ret = %{public}d", __func__, ret);
        return ret;
    }
    ret = CameraSendCropData(comDev->rspData, ctrlConfig);
    CHECK_RETURN_RET(ret);
    return HDF_SUCCESS;
}

static int32_t CameraReceiveCropData(struct HdfSBuf *reqData, struct CameraCtrlConfig *ctrlConfig)
{
    bool isFailed = false;

    isFailed |= !HdfSbufReadInt32(reqData, &ctrlConfig->pixelFormat.crop.left);
    isFailed |= !HdfSbufReadInt32(reqData, &ctrlConfig->pixelFormat.crop.top);
    isFailed |= !HdfSbufReadUint32(reqData, &ctrlConfig->pixelFormat.crop.width);
    isFailed |= !HdfSbufReadUint32(reqData, &ctrlConfig->pixelFormat.crop.height);
    CHECK_RETURN_RET(isFailed);
    return HDF_SUCCESS;
}

int32_t CameraSetCrop(struct CommonDevice *comDev)
{
    int32_t ret;
    struct CameraCtrlConfig ctrlConfig = {};

    ret = CameraReceiveCropData(comDev->reqData, &ctrlConfig);
    CHECK_RETURN_RET(ret);
    ret = StreamSetImageSetting(comDev, &ctrlConfig, CMD_SET_CROP);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: CMD_SET_CROP failed, ret = %{public}d", __func__, ret);
        return ret;
    }
    return HDF_SUCCESS;
}

static int32_t CameraGetFPSData(struct HdfSBuf *rspData, struct CameraCtrlConfig ctrlConfig)
{
    bool isFailed = false;

    isFailed |= !HdfSbufWriteUint32(rspData, ctrlConfig.pixelFormat.fps.numerator);
    isFailed |= !HdfSbufWriteUint32(rspData, ctrlConfig.pixelFormat.fps.denominator);
    CHECK_RETURN_RET(isFailed);
    return HDF_SUCCESS;
}

int32_t CameraGetFPS(struct CommonDevice *comDev)
{
    int32_t ret;
    struct CameraCtrlConfig ctrlConfig = {};

    ret = StreamGetImageSetting(comDev, &ctrlConfig, CMD_GET_FPS);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: CMD_GET_FPS failed, ret = %{public}d", __func__, ret);
        return ret;
    }
    ret = CameraGetFPSData(comDev->rspData, ctrlConfig);
    CHECK_RETURN_RET(ret);
    return HDF_SUCCESS;
}

static int32_t CameraSetFPSData(int camId, const char *driverName,
    struct HdfSBuf *reqData, struct CameraCtrlConfig *ctrlConfig)
{
    int32_t ret;
    bool isFailed = false;

    isFailed |= !HdfSbufReadUint32(reqData, &ctrlConfig->pixelFormat.fps.denominator);
    isFailed |= !HdfSbufReadUint32(reqData, &ctrlConfig->pixelFormat.fps.numerator);
    CHECK_RETURN_RET(isFailed);
    ret = CheckFrameRate(camId, driverName, STREAM_TYPE, ctrlConfig);
    return ret;
}

int32_t CameraSetFPS(struct CommonDevice *comDev)
{
    int32_t ret;
    struct CameraCtrlConfig ctrlConfig = {};

    ret = CameraSetFPSData(comDev->camId, comDev->driverName, comDev->reqData, &ctrlConfig);
    CHECK_RETURN_RET(ret);
    ret = StreamSetImageSetting(comDev, &ctrlConfig, CMD_SET_FPS);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: CMD_SET_FPS failed, ret = %{public}d", __func__, ret);
        return ret;
    }
    return HDF_SUCCESS;
}

static int32_t CameraStreamEnumFmt(struct CameraDevice *camDev, const char *driverName, int camId,
    struct EnumPixelFormatData *enumFormat, uint32_t cmd)
{
    int32_t ret;
    int32_t num;
    struct CameraDeviceDriver *deviceDriver = GetDeviceDriver(camDev);
    if (deviceDriver == NULL) {
        HDF_LOGE("%s: deviceDriver ptr is null!", __func__);
        return HDF_FAILURE;
    }
    num = GetStreamNum(driverName, deviceDriver);
    if (num < 0 || num >= DEVICE_NUM) {
        return HDF_ERR_INVALID_PARAM;
    }
    if (deviceDriver->stream[num]->streamOps == NULL) {
        HDF_LOGE("%s: streamOps ptr is null!", __func__);
        return HDF_FAILURE;
    }
    if (deviceDriver->stream[num]->streamOps->streamEnumFormat == NULL) {
        HDF_LOGE("%s: streamEnumFormat ptr is null!", __func__);
        return HDF_FAILURE;
    }
    ret = deviceDriver->stream[num]->streamOps->streamEnumFormat(&enumFormat->pixelFormat,
        deviceDriver->stream[num], enumFormat->index, cmd);
    return ret;
}

static int32_t CameraEnumFmtData(struct CameraDevice *camDev, const char *driverName, int camId,
    struct EnumPixelFormatData enumFmt, struct HdfSBuf *rspData)
{
    int32_t ret;

    ret = CameraStreamEnumFmt(camDev, driverName, camId, &enumFmt, CAMERA_CMD_ENUM_FMT);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: CameraStreamEnumFmt failed, ret = %{public}d", __func__, ret);
        return ret;
    }
    if (!HdfSbufWriteString(rspData, enumFmt.pixelFormat.pixel.description)) {
        HDF_LOGE("%s: Write description failed!", __func__);
        return HDF_FAILURE;
    }
    if (!HdfSbufWriteUint32(rspData, enumFmt.pixelFormat.pixel.format)) {
        HDF_LOGE("%s: Write format failed!", __func__);
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

static int32_t CameraEnumFramesizesData(struct CameraDevice *camDev, const char *driverName, int camId,
    struct EnumPixelFormatData enumFmt, struct HdfSBuf *rspData)
{
    int32_t ret;

    ret = CameraStreamEnumFmt(camDev, driverName, camId, &enumFmt, CAMERA_CMD_ENUM_FRAMESIZES);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: CameraStreamEnumFmt failed, ret = %{public}d", __func__, ret);
        return ret;
    }
    if (!HdfSbufWriteUint32(rspData, enumFmt.pixelFormat.pixel.height)) {
        HDF_LOGE("%s: Write height failed!", __func__);
        return HDF_FAILURE;
    }
    if (!HdfSbufWriteUint32(rspData, enumFmt.pixelFormat.pixel.width)) {
        HDF_LOGE("%s: Write width failed!", __func__);
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

static int32_t CameraEnumFrameintervalData(struct CameraDevice *camDev, const char *driverName, int camId,
    struct EnumPixelFormatData enumFmt, struct HdfSBuf *rspData)
{
    int32_t ret;

    ret = CameraStreamEnumFmt(camDev, driverName, camId, &enumFmt, CAMERA_CMD_ENUM_FRAMEINTERVALS);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: CameraStreamEnumFmt failed, ret = %{public}d", __func__, ret);
        return ret;
    }
    if (!HdfSbufWriteUint32(rspData, enumFmt.pixelFormat.fps.numerator)) {
        HDF_LOGE("%s: Write numerator failed!", __func__);
        return HDF_FAILURE;
    }
    if (!HdfSbufWriteUint32(rspData, enumFmt.pixelFormat.fps.denominator)) {
        HDF_LOGE("%s: Write denominator failed!", __func__);
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

int32_t CameraEnumFmt(struct CommonDevice *comDev)
{
    int32_t ret;
    uint32_t cmd = 0;
    uint32_t index = 0;
    bool isFailed = false;
    int32_t camId = comDev->camId;
    struct CameraDevice *camDev = comDev->camDev;
    const char *driverName = comDev->driverName;
    struct HdfSBuf *reqData = comDev->reqData;
    struct HdfSBuf *rspData = comDev->rspData;
    struct EnumPixelFormatData enumFormat = {};

    isFailed |= !HdfSbufReadUint32(reqData, &cmd);
    isFailed |= !HdfSbufReadUint32(reqData, &index);
    CHECK_RETURN_RET(isFailed);
    enumFormat.index = index;
    switch (cmd) {
        case CAMERA_CMD_ENUM_FMT:
            ret = CameraEnumFmtData(camDev, driverName, camId, enumFormat, rspData);
            CHECK_RETURN_RET(ret);
            break;
        case CAMERA_CMD_ENUM_FRAMESIZES:
            isFailed |= !HdfSbufReadUint32(reqData, &enumFormat.pixelFormat.pixel.format);
            CHECK_RETURN_RET(isFailed);
            ret = CameraEnumFramesizesData(camDev, driverName, camId, enumFormat, rspData);
            CHECK_RETURN_RET(ret);
            break;
        case CAMERA_CMD_ENUM_FRAMEINTERVALS:
            isFailed |= !HdfSbufReadUint32(reqData, &enumFormat.pixelFormat.pixel.format);
            isFailed |= !HdfSbufReadUint32(reqData, &enumFormat.pixelFormat.pixel.width);
            isFailed |= !HdfSbufReadUint32(reqData, &enumFormat.pixelFormat.pixel.height);
            CHECK_RETURN_RET(isFailed);
            ret = CameraEnumFrameintervalData(camDev, driverName, camId, enumFormat, rspData);
            CHECK_RETURN_RET(ret);
            break;
        default:
            HDF_LOGE("%s: wrong cmd, cmd=%{public}d", __func__, cmd);
            return HDF_ERR_NOT_SUPPORT;
    }
    return HDF_SUCCESS;
}

static int32_t CameraWriteAbilityData(struct HdfSBuf *rspData, struct Capability capability)
{
    bool isFailed = false;

    isFailed |= !HdfSbufWriteString(rspData, capability.driver);
    isFailed |= !HdfSbufWriteString(rspData, capability.card);
    isFailed |= !HdfSbufWriteString(rspData, capability.busInfo);
    isFailed |= !HdfSbufWriteUint32(rspData, capability.capabilities);
    CHECK_RETURN_RET(isFailed);
    return HDF_SUCCESS;
}

int32_t CameraStreamGetAbility(struct CommonDevice *comDev)
{
    int32_t ret;
    int32_t num;
    struct Capability capability = {};
    struct CameraDeviceDriver *deviceDriver = GetDeviceDriver(comDev->camDev);

    if (deviceDriver == NULL) {
        HDF_LOGE("%s: deviceDriver ptr is null!", __func__);
        return HDF_FAILURE;
    }
    num = GetStreamNum(comDev->driverName, deviceDriver);
    if (num < 0 || num >= DEVICE_NUM) {
        return HDF_ERR_INVALID_PARAM;
    }
    if (deviceDriver->stream[num]->streamOps == NULL) {
        HDF_LOGE("%s: streamOps ptr is null!", __func__);
        return HDF_FAILURE;
    }
    if (deviceDriver->stream[num]->streamOps->streamGetAbility == NULL) {
        HDF_LOGE("%s: streamGetAbility ptr is null!", __func__);
        return HDF_FAILURE;
    }
    ret = deviceDriver->stream[num]->streamOps->streamGetAbility(&capability, deviceDriver->stream[num]);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: CameraStreamGetAbilitys failed, ret = %{public}d", __func__, ret);
        return ret;
    }
    ret = CameraWriteAbilityData(comDev->rspData, capability);
    CHECK_RETURN_RET(ret);
    return HDF_SUCCESS;
}

int32_t CameraStreamEnumDevice(struct CommonDevice *comDev)
{
    int32_t i;
    int32_t num;
    bool isFailed = false;
    int camId = comDev->camId;
    struct HdfSBuf *rspData = comDev->rspData;
    struct CameraDevice *camDev = comDev->camDev;
    const char *driverName = comDev->driverName;
    struct CameraConfigRoot *rootConfig = NULL;
    struct CameraDeviceDriver *deviceDriver = GetDeviceDriver(camDev);
    
    if (rspData == NULL) {
        return HDF_ERR_INVALID_PARAM;
    }
    if (deviceDriver == NULL) {
        HDF_LOGE("%s: deviceDriver ptr is null!", __func__);
        return HDF_FAILURE;
    }
    rootConfig = HdfCameraGetConfigRoot();
    if (rootConfig == NULL) {
        HDF_LOGE("%s: get rootConfig failed!", __func__);
        return HDF_FAILURE;
    }
    num = GetStreamNum(driverName, deviceDriver);
    if (num < 0 || num >= DEVICE_NUM) {
        return HDF_ERR_INVALID_PARAM;
    }
    isFailed |= !HdfSbufWriteUint8(rspData, rootConfig->deviceConfig[camId].stream.mode);
    isFailed |= !HdfSbufWriteString(rspData, rootConfig->deviceConfig[camId].stream.stream[num].name);
    isFailed |= !HdfSbufWriteUint8(rspData, rootConfig->deviceConfig[camId].stream.stream[num].id);
    isFailed |= !HdfSbufWriteUint32(rspData, rootConfig->deviceConfig[camId].stream.stream[num].heightMaxNum);
    isFailed |= !HdfSbufWriteUint32(rspData, rootConfig->deviceConfig[camId].stream.stream[num].widthMaxNum);
    isFailed |= !HdfSbufWriteUint32(rspData, rootConfig->deviceConfig[camId].stream.stream[num].frameRateMaxNum);
    isFailed |= !HdfSbufWriteUint32(rspData, rootConfig->deviceConfig[camId].stream.stream[num].bufferTypeNum);
    isFailed |= !HdfSbufWriteUint8(rspData, rootConfig->deviceConfig[camId].stream.stream[num].bufferCount);
    for (i = 0; i <= rootConfig->deviceConfig[camId].stream.stream[num].bufferTypeNum; i++) {
        isFailed |= !HdfSbufWriteUint32(rspData, rootConfig->deviceConfig[camId].stream.stream[num].bufferType[i]);
    }
    CHECK_RETURN_RET(isFailed);
    return HDF_SUCCESS;
}

int32_t CameraQueueInit(struct CommonDevice *comDev)
{
    int32_t ret;
    int32_t num = 0;
    struct CameraDevice *camDev = comDev->camDev;
    const char *driverName = comDev->driverName;
    struct CameraDeviceDriver *deviceDriver = GetDeviceDriver(camDev);
    if (deviceDriver == NULL) {
        HDF_LOGE("%s: deviceDriver ptr is null!", __func__);
        return HDF_FAILURE;
    }
    num = GetStreamNum(driverName, deviceDriver);
    if (num < 0 || num >= DEVICE_NUM) {
        return HDF_ERR_INVALID_PARAM;
    }
    if (deviceDriver->stream[num]->streamOps == NULL) {
        HDF_LOGE("%s: streamOps ptr is null!", __func__);
        return HDF_FAILURE;
    }
    if (deviceDriver->stream[num]->streamOps->streamQueueInit == NULL) {
        HDF_LOGE("%s: streamQueueInit ptr is null!", __func__);
        return HDF_FAILURE;
    }
    ret = deviceDriver->stream[num]->streamOps->streamQueueInit(camDev->deviceDriver->stream[num]);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: Driver streamQueueInit failed, line: %{public}d", __func__, __LINE__);
        return ret;
    }
    ret = BufferQueueInit(&camDev->deviceDriver->stream[num]->queueImp.queue);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: Driver BufferQueueInit failed, line: %{public}d", __func__, __LINE__);
        return ret;
    }
    return HDF_SUCCESS;
}

static int32_t CameraDeviceReqMemory(struct CameraDevice *camDev,
    const char *driverName, int camId, struct UserCameraReq *requestData)
{
    int32_t ret;
    int32_t num = 0;
    struct CameraDeviceDriver *deviceDriver = GetDeviceDriver(camDev);
    if (deviceDriver == NULL) {
        HDF_LOGE("%s: deviceDriver ptr is null!", __func__);
        return HDF_FAILURE;
    }
    num = GetStreamNum(driverName, deviceDriver);
    if (num < 0 || num >= DEVICE_NUM) {
        return HDF_ERR_INVALID_PARAM;
    }
    ret = BufferQueueRequest(&camDev->deviceDriver->stream[num]->queueImp.queue, requestData);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: Core request buffers failed!", __func__);
        return ret;
    }
    return HDF_SUCCESS;
}

int32_t CameraReqMemory(struct CommonDevice *comDev)
{
    int32_t ret;
    bool isFailed = false;
    struct UserCameraReq requestData = {};

    isFailed |= !HdfSbufReadUint32(comDev->reqData, &requestData.count);
    isFailed |= !HdfSbufReadUint32(comDev->reqData, &requestData.memType);
    isFailed |= !HdfSbufReadUint32(comDev->reqData, &requestData.capabilities);
    CHECK_RETURN_RET(isFailed);
    ret = CameraDeviceReqMemory(comDev->camDev, comDev->driverName, comDev->camId, &requestData);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: CameraDeviceReqMemory failed, ret = %{public}d", __func__, ret);
        return ret;
    }
    return HDF_SUCCESS;
}

static int32_t CameraDeviceQueryMemory(struct CameraDevice *camDev,
    const char *driverName, int camId, struct UserCameraBuffer *userBuffer, uint32_t flag)
{
    int32_t ret;
    int32_t num = 0;
    struct CameraDeviceDriver *deviceDriver = GetDeviceDriver(camDev);
    if (deviceDriver == NULL) {
        HDF_LOGE("%s: deviceDriver ptr is null!", __func__);
        return HDF_FAILURE;
    }
    num = GetStreamNum(driverName, deviceDriver);
    if (num < 0 || num >= DEVICE_NUM) {
        return HDF_ERR_INVALID_PARAM;
    }
    ret = BufferQueueQueryBuffer(&camDev->deviceDriver->stream[num]->queueImp.queue, userBuffer);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: Driver BufferQueueQueryBuffer failed, line: %{public}d", __func__, __LINE__);
        return ret;
    }
    if (flag != RELEASE_FLAG) {
        BufferQueueSetQueueForMmap(&camDev->deviceDriver->stream[num]->queueImp.queue);
    }
    return HDF_SUCCESS;
}

int32_t CameraQueryMemory(struct CommonDevice *comDev)
{
    int32_t ret;
    bool isFailed = false;
    uint32_t flag = 0;
    struct UserCameraBuffer userBuffer = {};
    struct UserCameraPlane userPlane[1] = {};

    isFailed |= !HdfSbufReadUint32(comDev->reqData, &userBuffer.memType);
    isFailed |= !HdfSbufReadUint32(comDev->reqData, &userBuffer.id);
    isFailed |= !HdfSbufReadUint32(comDev->reqData, &userBuffer.planeCount);
    isFailed |= !HdfSbufReadUint32(comDev->reqData, &flag);
    userBuffer.planes = userPlane;
    CHECK_RETURN_RET(isFailed);
    ret = CameraDeviceQueryMemory(comDev->camDev, comDev->driverName, comDev->camId, &userBuffer, flag);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: CameraDeviceQueryMemory failed, ret = %{public}d", __func__, ret);
        return ret;
    }

    if (userBuffer.memType == MEMTYPE_MMAP) {
        if (!HdfSbufWriteUint32(comDev->rspData, userBuffer.planes[0].memory.offset)) {
            HDF_LOGE("%s: HdfSbufWriteUint32 failed, line: %{public}d", __func__, __LINE__);
            return HDF_FAILURE;
        }
        if (!HdfSbufWriteUint32(comDev->rspData, userBuffer.planes[0].length)) {
            HDF_LOGE("%s: HdfSbufWriteUint32 failed, line: %{public}d", __func__, __LINE__);
            return HDF_FAILURE;
        }
    } else if (userBuffer.memType == MEMTYPE_USERPTR) {
        if (!HdfSbufWriteUint32(comDev->rspData, userBuffer.planeCount)) {
            HDF_LOGE("%s: HdfSbufWriteUint32 failed, line: %{public}d", __func__, __LINE__);
            return HDF_FAILURE;
        }
    } else if (userBuffer.memType == MEMTYPE_DMABUF) {
        if (!HdfSbufWriteUint32(comDev->rspData, userBuffer.planes[0].length)) {
            HDF_LOGE("%s: HdfSbufWriteUint32 failed, line: %{public}d", __func__, __LINE__);
            return HDF_FAILURE;
        }
    }
    return HDF_SUCCESS;
}

static int32_t CameraDeviceStreamQueue(struct CameraDevice *camDev,
    const char *driverName, int camId, struct UserCameraBuffer *userBuffer)
{
    int32_t ret;
    int32_t num = 0;
    struct CameraDeviceDriver *deviceDriver = GetDeviceDriver(camDev);
    if (deviceDriver == NULL) {
        HDF_LOGE("%s: deviceDriver ptr is null!", __func__);
        return HDF_FAILURE;
    }
    num = GetStreamNum(driverName, deviceDriver);
    if (num < 0 || num >= DEVICE_NUM) {
        return HDF_ERR_INVALID_PARAM;
    }

    ret = BufferQueueReturnBuffer(&camDev->deviceDriver->stream[num]->queueImp.queue, userBuffer);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: Driver BufferQueueReturnBuffer failed, line: %{public}d", __func__, __LINE__);
        return ret;
    }
    return HDF_SUCCESS;
}

int32_t CameraStreamQueue(struct CommonDevice *comDev)
{
    int32_t ret;
    bool isFailed = false;
    struct UserCameraBuffer userBuffer = {};
    struct UserCameraPlane userPlane[1] = {};

    isFailed |= !HdfSbufReadUint32(comDev->reqData, &userBuffer.memType);
    isFailed |= !HdfSbufReadUint32(comDev->reqData, &userBuffer.id);
    isFailed |= !HdfSbufReadUint32(comDev->reqData, &userBuffer.planeCount);
    userBuffer.planes = userPlane;
    isFailed |= !HdfSbufReadUint32(comDev->reqData, &userBuffer.planes[0].length);
    if (userBuffer.memType == MEMTYPE_USERPTR) {
        isFailed |= !HdfSbufReadUint64(comDev->reqData, &userBuffer.planes[0].memory.userPtr);
    } else if (userBuffer.memType == MEMTYPE_MMAP) {
        isFailed |= !HdfSbufReadUint32(comDev->reqData, &userBuffer.planes[0].memory.offset);
    } else if (userBuffer.memType == MEMTYPE_DMABUF) {
        isFailed |= !HdfSbufReadUint32(comDev->reqData, &userBuffer.planes[0].memory.fd);
    }
    CHECK_RETURN_RET(isFailed);
    ret = CameraDeviceStreamQueue(comDev->camDev, comDev->driverName, comDev->camId,  &userBuffer);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: CameraDeviceStreamQueue failed, ret = %{public}d", __func__, ret);
        return ret;
    }
    return HDF_SUCCESS;
}

static int32_t CameraDeviceStreamDequeue(struct CameraDevice *camDev,
    const char *driverName, int camId, struct UserCameraBuffer *userBuffer)
{
    int32_t ret;
    int32_t num = 0;
    struct CameraDeviceDriver *deviceDriver = GetDeviceDriver(camDev);
    if (deviceDriver == NULL) {
        HDF_LOGE("%s: deviceDriver ptr is null!", __func__);
        return HDF_FAILURE;
    }
    num = GetStreamNum(driverName, deviceDriver);
    if (num < 0 || num >= DEVICE_NUM) {
        return HDF_ERR_INVALID_PARAM;
    }
    ret = BufferQueueAcquireBuffer(&camDev->deviceDriver->stream[num]->queueImp.queue, userBuffer);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: Driver BufferQueueAcquireBuffer failed, line: %{public}d", __func__, __LINE__);
        return ret;
    }
    return HDF_SUCCESS;
}

int32_t CameraStreamDeQueue(struct CommonDevice *comDev)
{
    int32_t ret;
    bool isFailed = false;
    struct UserCameraBuffer userBuffer = {};
    struct UserCameraPlane userPlane[1] = {};

    isFailed |= !HdfSbufReadUint32(comDev->reqData, &userBuffer.memType);
    isFailed |= !HdfSbufReadUint32(comDev->reqData, &userBuffer.planeCount);
    userBuffer.planes = userPlane;
    isFailed |= !HdfSbufReadUint32(comDev->reqData, &userBuffer.flags);
    CHECK_RETURN_RET(isFailed);

    ret = CameraDeviceStreamDequeue(comDev->camDev, comDev->driverName, comDev->camId, &userBuffer);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: CameraDeviceStreamDequeue failed, ret = %{public}d", __func__, ret);
        return ret;
    }
    if (!HdfSbufWriteUint32(comDev->rspData, userBuffer.id)) {
        HDF_LOGE("%s: write userBuffer.id failed", __func__);
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

int32_t CameraStreamOn(struct CommonDevice *comDev)
{
    int32_t ret;
    int32_t num = 0;
    struct CameraDevice *camDev = comDev->camDev;
    const char *driverName = comDev->driverName;
    struct CameraDeviceDriver *deviceDriver = GetDeviceDriver(camDev);
    struct StreamDevice *streamDev = NULL;

    if (deviceDriver == NULL) {
        HDF_LOGE("%s: deviceDriver ptr is null!", __func__);
        return HDF_FAILURE;
    }
    num = GetStreamNum(driverName, deviceDriver);
    if (num < 0 || num >= DEVICE_NUM) {
        return HDF_ERR_INVALID_PARAM;
    }
    if (deviceDriver->stream[num]->streamOps == NULL) {
        HDF_LOGE("%s: streamOps ptr is null!", __func__);
        return HDF_FAILURE;
    }
    streamDev = deviceDriver->stream[num];
    ret = BufferQueueStreamOn(&streamDev->queueImp.queue);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: Driver BufferQueueStreamOn failed, line: %{public}d", __func__, __LINE__);
        return ret;
    }
    return HDF_SUCCESS;
}

int32_t CameraStreamOff(struct CommonDevice *comDev)
{
    int32_t ret;
    int32_t num = 0;
    struct CameraDevice *camDev = comDev->camDev;
    const char *driverName = comDev->driverName;
    struct CameraDeviceDriver *deviceDriver = GetDeviceDriver(camDev);
    struct StreamDevice *streamDev = NULL;

    if (deviceDriver == NULL) {
        HDF_LOGE("%s: deviceDriver ptr is null!", __func__);
        return HDF_FAILURE;
    }
    num = GetStreamNum(driverName, deviceDriver);
    if (num < 0 || num >= DEVICE_NUM) {
        return HDF_ERR_INVALID_PARAM;
    }
    if (deviceDriver->stream[num]->streamOps == NULL) {
        HDF_LOGE("%s: streamOps ptr is null!", __func__);
        return HDF_FAILURE;
    }
    streamDev = camDev->deviceDriver->stream[num];

    if ((streamDev->queueImp.queue.flags & QUEUE_STATE_STREAMING_CALLED) != 0) {
        if ((streamDev->streamOps->stopStreaming) != NULL) {
            streamDev->streamOps->stopStreaming(streamDev);
        }
    }
    ret = BufferQueueStreamOff(&streamDev->queueImp.queue);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: Driver BufferQueueStreamOff failed, line: %{public}d", __func__, __LINE__);
        return ret;
    }
    return HDF_SUCCESS;
}
