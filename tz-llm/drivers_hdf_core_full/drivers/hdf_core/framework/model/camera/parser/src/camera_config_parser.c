/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include <hdf_log.h>
#include "camera_config_parser.h"

#define HDF_LOG_TAG HDF_CAMERA_PARSER

#define CHECK_PARSER_CONFIG_RET(ret, str) do { \
    if ((ret) != HDF_SUCCESS) { \
        HDF_LOGE("%s: get %{public}s failed, ret = %{public}d, line : %{public}d", __func__, str, ret, __LINE__); \
        return ret; \
    } \
} while (0)

static struct CameraConfigRoot g_configCameraRoot;

static void SetCtrlCapInfo(struct CtrlCapInfo *ctrlCap, int ctrlValueNum, const int *ctrlValue)
{
    int32_t i;

    for (i = 0; i < ctrlValueNum; i += CTRL_INFO_COUNT) {
        ctrlCap[i / CTRL_INFO_COUNT].ctrlId = ctrlValue[i + CTRL_ID_INDEX];
        ctrlCap[i / CTRL_INFO_COUNT].max = ctrlValue[i + CTRL_MAX_INDEX];
        ctrlCap[i / CTRL_INFO_COUNT].min = ctrlValue[i + CTRL_MIN_INDEX];
        ctrlCap[i / CTRL_INFO_COUNT].step = ctrlValue[i + CTRL_STEP_INDEX];
        ctrlCap[i / CTRL_INFO_COUNT].def = ctrlValue[i + CTRL_DEF_INDEX];
        HDF_LOGD("%s: get ctrlCap[%{public}d]: ctrlId=%{public}d, max=%{public}d, min=%{public}d, "
            "step = %{public}d, def = %{public}d", __func__, (i / CTRL_INFO_COUNT),
            ctrlCap[i / CTRL_INFO_COUNT].ctrlId, ctrlCap[i / CTRL_INFO_COUNT].max, ctrlCap[i / CTRL_INFO_COUNT].min,
            ctrlCap[i / CTRL_INFO_COUNT].step, ctrlCap[i / CTRL_INFO_COUNT].def);
    }
}

static int32_t ParseCameraSensorDeviceConfig(const struct DeviceResourceNode *node,
    struct DeviceResourceIface *drsOps, struct SensorDeviceConfig *sensorConfig)
{
    int32_t ret;

    ret = drsOps->GetString(node, "name", &sensorConfig->name, NULL);
    CHECK_PARSER_CONFIG_RET(ret, "name");
    ret = drsOps->GetUint8(node, "id", &sensorConfig->id, 0);
    CHECK_PARSER_CONFIG_RET(ret, "id");
    ret = drsOps->GetUint8(node, "exposure", &sensorConfig->exposure, 0);
    CHECK_PARSER_CONFIG_RET(ret, "exposure");
    ret = drsOps->GetUint8(node, "mirror", &sensorConfig->mirror, 0);
    CHECK_PARSER_CONFIG_RET(ret, "mirror");
    ret = drsOps->GetUint8(node, "gain", &sensorConfig->gain, 0);
    CHECK_PARSER_CONFIG_RET(ret, "gain");
    sensorConfig->ctrlValueNum = drsOps->GetElemNum(node, "ctrlValue");
    if (sensorConfig->ctrlValueNum > CTRL_VALUE_MAX_NUM) {
        HDF_LOGE("%s: parser ctrlValue num failed! num = %{public}d", __func__, sensorConfig->ctrlValueNum);
        return HDF_ERR_INVALID_PARAM;
    }
    ret = drsOps->GetUint32Array(node, "ctrlValue", sensorConfig->ctrlValue, sensorConfig->ctrlValueNum, 0);
    CHECK_PARSER_CONFIG_RET(ret, "ctrlValue");
    SetCtrlCapInfo(sensorConfig->ctrlCap, sensorConfig->ctrlValueNum, sensorConfig->ctrlValue);

    HDF_LOGD("%s: name=%{public}s, id=%{public}d, exposure=%{public}d, mirror=%{public}d, gain=%{public}d",
        __func__, sensorConfig->name, sensorConfig->id, sensorConfig->exposure, sensorConfig->mirror,
        sensorConfig->gain);

    return HDF_SUCCESS;
}

static int32_t ParseCameraIspDeviceConfig(const struct DeviceResourceNode *node,
    struct DeviceResourceIface *drsOps, struct IspDeviceConfig *ispConfig)
{
    int32_t ret;

    ret = drsOps->GetString(node, "name", &ispConfig->name, NULL);
    CHECK_PARSER_CONFIG_RET(ret, "name");
    ret = drsOps->GetUint8(node, "id", &ispConfig->id, 0);
    CHECK_PARSER_CONFIG_RET(ret, "id");
    ret = drsOps->GetUint8(node, "brightness", &ispConfig->brightness, 0);
    CHECK_PARSER_CONFIG_RET(ret, "brightness");
    ret = drsOps->GetUint8(node, "contrast", &ispConfig->contrast, 0);
    CHECK_PARSER_CONFIG_RET(ret, "contrast");
    ret = drsOps->GetUint8(node, "saturation", &ispConfig->saturation, 0);
    CHECK_PARSER_CONFIG_RET(ret, "saturation");
    ret = drsOps->GetUint8(node, "hue", &ispConfig->hue, 0);
    CHECK_PARSER_CONFIG_RET(ret, "hue");
    ret = drsOps->GetUint8(node, "sharpness", &ispConfig->sharpness, 0);
    CHECK_PARSER_CONFIG_RET(ret, "sharpness");
    ret = drsOps->GetUint8(node, "gain", &ispConfig->gain, 0);
    CHECK_PARSER_CONFIG_RET(ret, "gain");
    ret = drsOps->GetUint8(node, "gamma", &ispConfig->gamma, 0);
    CHECK_PARSER_CONFIG_RET(ret, "gamma");
    ret = drsOps->GetUint8(node, "whiteBalance", &ispConfig->whiteBalance, 0);
    CHECK_PARSER_CONFIG_RET(ret, "whiteBalance");
    ispConfig->ctrlValueNum = drsOps->GetElemNum(node, "ctrlValue");
    if (ispConfig->ctrlValueNum > CTRL_VALUE_MAX_NUM) {
        HDF_LOGE("%s: parser ctrlValue num failed! num = %{public}d", __func__, ispConfig->ctrlValueNum);
        return HDF_ERR_INVALID_PARAM;
    }
    ret = drsOps->GetUint32Array(node, "ctrlValue", ispConfig->ctrlValue, ispConfig->ctrlValueNum, 0);
    CHECK_PARSER_CONFIG_RET(ret, "ctrlValue");
    SetCtrlCapInfo(ispConfig->ctrlCap, ispConfig->ctrlValueNum, ispConfig->ctrlValue);

    HDF_LOGD("%s: name=%{public}s, id=%{public}d, brightness=%{public}d, contrast=%{public}d, saturation=%{public}d, "
        "sharpness=%{public}d, gain=%{public}d, gamma=%{public}d, whiteBalance=%{public}d", __func__, ispConfig->name,
        ispConfig->id, ispConfig->brightness, ispConfig->contrast, ispConfig->saturation,
        ispConfig->sharpness, ispConfig->gain, ispConfig->gamma, ispConfig->whiteBalance);
    return HDF_SUCCESS;
}

static int32_t ParseCameraVcmDeviceConfig(const struct DeviceResourceNode *node,
    struct DeviceResourceIface *drsOps, struct VcmDeviceConfig *vcmConfig)
{
    int32_t ret;

    ret = drsOps->GetString(node, "name", &vcmConfig->name, NULL);
    CHECK_PARSER_CONFIG_RET(ret, "name");
    ret = drsOps->GetUint8(node, "id", &vcmConfig->id, 0);
    CHECK_PARSER_CONFIG_RET(ret, "id");
    ret = drsOps->GetUint8(node, "focus", &vcmConfig->focus, 0);
    CHECK_PARSER_CONFIG_RET(ret, "focus");
    ret = drsOps->GetUint8(node, "autoFocus", &vcmConfig->autoFocus, 0);
    CHECK_PARSER_CONFIG_RET(ret, "autoFocus");
    ret = drsOps->GetUint8(node, "zoom", &vcmConfig->zoom, 0);
    CHECK_PARSER_CONFIG_RET(ret, "zoom");
    ret = drsOps->GetUint32(node, "zoomMaxNum", &vcmConfig->zoomMaxNum, 0);
    CHECK_PARSER_CONFIG_RET(ret, "zoomMaxNum");
    vcmConfig->ctrlValueNum = drsOps->GetElemNum(node, "ctrlValue");
    if (vcmConfig->ctrlValueNum > CTRL_VALUE_MAX_NUM) {
        HDF_LOGE("%s: parser ctrlValue num failed! num = %{public}d", __func__, vcmConfig->ctrlValueNum);
        return HDF_ERR_INVALID_PARAM;
    }
    ret = drsOps->GetUint32Array(node, "ctrlValue", vcmConfig->ctrlValue, vcmConfig->ctrlValueNum, 0);
    CHECK_PARSER_CONFIG_RET(ret, "ctrlValue");
    SetCtrlCapInfo(vcmConfig->ctrlCap, vcmConfig->ctrlValueNum, vcmConfig->ctrlValue);

    HDF_LOGD("%s: name=%{public}s, id=%{public}d, focus=%{public}d, autoFocus=%{public}d, zoom=%{public}d,"
        "zoomMaxNum=%{public}d", __func__, vcmConfig->name, vcmConfig->id, vcmConfig->focus, vcmConfig->autoFocus,
        vcmConfig->zoom, vcmConfig->zoomMaxNum);

    return HDF_SUCCESS;
}

static int32_t ParseCameraLensDeviceConfig(const struct DeviceResourceNode *node,
    struct DeviceResourceIface *drsOps, struct LensDeviceConfig *lensConfig)
{
    int32_t ret;

    ret = drsOps->GetString(node, "name", &lensConfig->name, NULL);
    CHECK_PARSER_CONFIG_RET(ret, "name");
    ret = drsOps->GetUint8(node, "id", &lensConfig->id, 0);
    CHECK_PARSER_CONFIG_RET(ret, "id");
    ret = drsOps->GetUint8(node, "aperture", &lensConfig->aperture, 0);
    CHECK_PARSER_CONFIG_RET(ret, "aperture");
    lensConfig->ctrlValueNum = drsOps->GetElemNum(node, "ctrlValue");
    if (lensConfig->ctrlValueNum > CTRL_VALUE_MAX_NUM) {
        HDF_LOGE("%s: parser ctrlValue num failed! num = %{public}d", __func__, lensConfig->ctrlValueNum);
        return HDF_ERR_INVALID_PARAM;
    }
    ret = drsOps->GetUint32Array(node, "ctrlValue", lensConfig->ctrlValue, lensConfig->ctrlValueNum, 0);
    CHECK_PARSER_CONFIG_RET(ret, "ctrlValue");
    SetCtrlCapInfo(lensConfig->ctrlCap, lensConfig->ctrlValueNum, lensConfig->ctrlValue);

    HDF_LOGD("%s: name=%{public}s, id=%{public}d, aperture=%{public}d", __func__,
        lensConfig->name, lensConfig->id, lensConfig->aperture);
    return HDF_SUCCESS;
}

static int32_t ParseCameraFlashDeviceConfig(const struct DeviceResourceNode *node,
    struct DeviceResourceIface *drsOps, struct FlashDeviceConfig *flashConfig)
{
    int32_t ret;

    ret = drsOps->GetString(node, "name", &flashConfig->name, NULL);
    CHECK_PARSER_CONFIG_RET(ret, "name");
    ret = drsOps->GetUint8(node, "id", &flashConfig->id, 0);
    CHECK_PARSER_CONFIG_RET(ret, "id");
    ret = drsOps->GetUint8(node, "flashMode", &flashConfig->flashMode, 0);
    CHECK_PARSER_CONFIG_RET(ret, "flashMode");
    ret = drsOps->GetUint8(node, "flashIntensity", &flashConfig->flashIntensity, 0);
    CHECK_PARSER_CONFIG_RET(ret, "flashIntensity");
    flashConfig->ctrlValueNum = drsOps->GetElemNum(node, "ctrlValue");
    if (flashConfig->ctrlValueNum > CTRL_VALUE_MAX_NUM) {
        HDF_LOGE("%s: parser ctrlValue num failed! num = %{public}d", __func__, flashConfig->ctrlValueNum);
        return HDF_ERR_INVALID_PARAM;
    }
    ret = drsOps->GetUint32Array(node, "ctrlValue", flashConfig->ctrlValue, flashConfig->ctrlValueNum, 0);
    CHECK_PARSER_CONFIG_RET(ret, "ctrlValue");
    SetCtrlCapInfo(flashConfig->ctrlCap, flashConfig->ctrlValueNum, flashConfig->ctrlValue);

    HDF_LOGD("%s: name=%{public}s, id=%{public}d, flashMode=%{public}d, flashIntensity=%{public}d", __func__,
        flashConfig->name, flashConfig->id, flashConfig->flashMode, flashConfig->flashIntensity);
    return HDF_SUCCESS;
}

static int32_t ParseCameraStreamDeviceConfigs(const struct DeviceResourceNode *node,
    struct DeviceResourceIface *drsOps, struct StreamDeviceConfig *streamConfig)
{
    int32_t ret;

    ret = drsOps->GetString(node, "name", &streamConfig->name, NULL);
    CHECK_PARSER_CONFIG_RET(ret, "name");
    ret = drsOps->GetUint8(node, "id", &streamConfig->id, 0);
    CHECK_PARSER_CONFIG_RET(ret, "id");
    ret = drsOps->GetUint32(node, "heightMaxNum", &streamConfig->heightMaxNum, 0);
    CHECK_PARSER_CONFIG_RET(ret, "heightMaxNum");
    ret = drsOps->GetUint32(node, "widthMaxNum", &streamConfig->widthMaxNum, 0);
    CHECK_PARSER_CONFIG_RET(ret, "widthMaxNum");
    ret = drsOps->GetUint32(node, "frameRateMaxNum", &streamConfig->frameRateMaxNum, 0);
    CHECK_PARSER_CONFIG_RET(ret, "frameRateMaxNum");
    ret = drsOps->GetUint8(node, "bufferCount", &streamConfig->bufferCount, 0);
    CHECK_PARSER_CONFIG_RET(ret, "bufferCount");

    return HDF_SUCCESS;
}

static int32_t ParseCameraStreamDeviceConfig(const struct DeviceResourceNode *node,
    struct DeviceResourceIface *drsOps, struct StreamDeviceConfig *streamConfig)
{
    int32_t i;
    int32_t ret;

    ret = ParseCameraStreamDeviceConfigs(node, drsOps, streamConfig);
    CHECK_PARSER_CONFIG_RET(ret, "streamStatus");
    streamConfig->bufferTypeNum = drsOps->GetElemNum(node, "bufferType");
    if (streamConfig->bufferTypeNum == 0 || streamConfig->bufferTypeNum > BUFFER_TYPE_MAX_NUM) {
        HDF_LOGE("%s: parser bufferType element num failed! num = %{public}d", __func__, streamConfig->bufferTypeNum);
        return HDF_ERR_INVALID_PARAM;
    }
    ret = drsOps->GetUint32Array(node, "bufferType", streamConfig->bufferType, streamConfig->bufferTypeNum, 0);
    CHECK_PARSER_CONFIG_RET(ret, "bufferType");

    for (i = 0; i < streamConfig->bufferTypeNum; i++) {
        HDF_LOGD("%s: get bufferType[%{public}d] = %{public}d", __func__, i, streamConfig->bufferType[i]);
    }

    streamConfig->formatTypeNum = drsOps->GetElemNum(node, "formatType");
    if (streamConfig->formatTypeNum == 0 || streamConfig->formatTypeNum > FORMAT_TYPE_MAX_NUM) {
        HDF_LOGE("%s: parser formatType element num failed! num = %{public}d", __func__, streamConfig->formatTypeNum);
        return HDF_ERR_INVALID_PARAM;
    }
    ret = drsOps->GetUint32Array(node, "formatType", streamConfig->formatType, streamConfig->formatTypeNum, 0);
    CHECK_PARSER_CONFIG_RET(ret, "formatType");

    for (i = 0; i < streamConfig->formatTypeNum; i++) {
        HDF_LOGD("%s: get formatType[%{public}d] = %{public}d", __func__, i, streamConfig->formatType[i]);
    }

    HDF_LOGD("%s: name=%{public}s, id=%{public}d, heightMaxNum=%{public}d, widthMaxNum=%{public}d, "
        "frameRateMaxNum=%{public}d, bufferCount=%{public}d", __func__, streamConfig->name, streamConfig->id,
        streamConfig->heightMaxNum, streamConfig->widthMaxNum, streamConfig->frameRateMaxNum,
        streamConfig->bufferCount);
    return HDF_SUCCESS;
}

static int32_t ParseCameraSensorConfig(const struct DeviceResourceNode *node,
    struct DeviceResourceIface *drsOps, struct CameraSensorConfig *sensorConfig)
{
    struct DeviceResourceNode *childNode = NULL;
    int32_t ret;
    uint32_t cnt = 0;

    ret = drsOps->GetUint8(node, "mode", &sensorConfig->mode, 0);
    CHECK_PARSER_CONFIG_RET(ret, "mode");

    if (sensorConfig->mode == DEVICE_NOT_SUPPORT) {
        HDF_LOGD("%s: not support sensor!", __func__);
        return HDF_SUCCESS;
    }

    DEV_RES_NODE_FOR_EACH_CHILD_NODE(node, childNode)
    {
        if (ParseCameraSensorDeviceConfig(childNode, drsOps, &sensorConfig->sensor[cnt]) != HDF_SUCCESS) {
            HDF_LOGE("%s: Parse sensor[%{public}d] failed!", __func__, cnt);
            return HDF_FAILURE;
        }
        cnt++;
        sensorConfig->sensorNum++;
    }
    HDF_LOGD("%s: sensorConfig->sensorNum = %{public}d!", __func__, sensorConfig->sensorNum);

    return HDF_SUCCESS;
}

static int32_t ParseCameraIspConfig(const struct DeviceResourceNode *node,
    struct DeviceResourceIface *drsOps, struct CameraIspConfig *ispConfig)
{
    struct DeviceResourceNode *childNode = NULL;
    int32_t ret;
    uint32_t cnt = 0;

    ret = drsOps->GetUint8(node, "mode", &ispConfig->mode, 0);
    CHECK_PARSER_CONFIG_RET(ret, "mode");

    if (ispConfig->mode == DEVICE_NOT_SUPPORT) {
        HDF_LOGD("%s: not support isp!", __func__);
        return HDF_SUCCESS;
    }

    DEV_RES_NODE_FOR_EACH_CHILD_NODE(node, childNode)
    {
        if (ParseCameraIspDeviceConfig(childNode, drsOps, &ispConfig->isp[cnt]) != HDF_SUCCESS) {
            HDF_LOGE("%s: Parse isp[%{public}d] failed!", __func__, cnt);
            return HDF_FAILURE;
        }
        cnt++;
        ispConfig->ispNum++;
    }
    HDF_LOGD("%s: ispConfig->ispNum = %{public}d!", __func__, ispConfig->ispNum);

    return HDF_SUCCESS;
}

static int32_t ParseCameraLensConfig(const struct DeviceResourceNode *node,
    struct DeviceResourceIface *drsOps, struct CameraLensConfig *lensConfig)
{
    struct DeviceResourceNode *childNode = NULL;
    int32_t ret;
    uint32_t cnt = 0;

    ret = drsOps->GetUint8(node, "mode", &lensConfig->mode, 0);
    CHECK_PARSER_CONFIG_RET(ret, "mode");

    if (lensConfig->mode == DEVICE_NOT_SUPPORT) {
        HDF_LOGD("%s: not support lens!", __func__);
        return HDF_SUCCESS;
    }

    DEV_RES_NODE_FOR_EACH_CHILD_NODE(node, childNode)
    {
        if (ParseCameraLensDeviceConfig(childNode, drsOps, &lensConfig->lens[cnt]) != HDF_SUCCESS) {
            HDF_LOGE("%s: Parse lens[%{public}d] failed!", __func__, cnt);
            return HDF_FAILURE;
        }
        cnt++;
        lensConfig->lensNum++;
    }
    HDF_LOGD("%s: lensConfig->lensNum = %{public}d!", __func__, lensConfig->lensNum);

    return HDF_SUCCESS;
}

static int32_t ParseCameraVcmConfig(const struct DeviceResourceNode *node,
    struct DeviceResourceIface *drsOps, struct CameraVcmConfig *vcmConfig)
{
    struct DeviceResourceNode *childNode = NULL;
    int32_t ret;
    uint32_t cnt = 0;

    ret = drsOps->GetUint8(node, "mode", &vcmConfig->mode, 0);
    CHECK_PARSER_CONFIG_RET(ret, "mode");

    if (vcmConfig->mode == DEVICE_NOT_SUPPORT) {
        HDF_LOGD("%s: not support vcm!", __func__);
        return HDF_SUCCESS;
    }

    DEV_RES_NODE_FOR_EACH_CHILD_NODE(node, childNode)
    {
        if (ParseCameraVcmDeviceConfig(childNode, drsOps, &vcmConfig->vcm[cnt]) != HDF_SUCCESS) {
            HDF_LOGE("%s: Parse vcm[%{public}d] failed!", __func__, cnt);
            return HDF_FAILURE;
        }
        cnt++;
        vcmConfig->vcmNum++;
    }
    HDF_LOGD("%s: vcmConfig->vcmNum = %{public}d!", __func__, vcmConfig->vcmNum);

    return HDF_SUCCESS;
}

static int32_t ParseCameraFlashConfig(const struct DeviceResourceNode *node,
    struct DeviceResourceIface *drsOps, struct CameraFlashConfig *flashConfig)
{
    struct DeviceResourceNode *childNode = NULL;
    int32_t ret;
    uint32_t cnt = 0;

    ret = drsOps->GetUint8(node, "mode", &flashConfig->mode, 0);
    CHECK_PARSER_CONFIG_RET(ret, "mode");

    if (flashConfig->mode == DEVICE_NOT_SUPPORT) {
        HDF_LOGD("%s: not support flash!", __func__);
        return HDF_SUCCESS;
    }

    DEV_RES_NODE_FOR_EACH_CHILD_NODE(node, childNode)
    {
        if (ParseCameraFlashDeviceConfig(childNode, drsOps, &flashConfig->flash[cnt]) != HDF_SUCCESS) {
            HDF_LOGE("%s: Parse flash[%{public}d] failed!", __func__, cnt);
            return HDF_FAILURE;
        }
        cnt++;
        flashConfig->flashNum++;
    }
    HDF_LOGD("%s: flashConfig->flashNum = %{public}d!", __func__, flashConfig->flashNum);

    return HDF_SUCCESS;
}

static int32_t ParseCameraStreamConfig(const struct DeviceResourceNode *node,
    struct DeviceResourceIface *drsOps, struct CameraStreamConfig *streamConfig)
{
    struct DeviceResourceNode *childNode = NULL;
    int32_t ret;
    uint32_t cnt = 0;

    ret = drsOps->GetUint8(node, "mode", &streamConfig->mode, 0);
    CHECK_PARSER_CONFIG_RET(ret, "mode");

    if (streamConfig->mode == DEVICE_NOT_SUPPORT) {
        HDF_LOGD("%s: not support stream!", __func__);
        return HDF_SUCCESS;
    }

    DEV_RES_NODE_FOR_EACH_CHILD_NODE(node, childNode)
    {
        if (ParseCameraStreamDeviceConfig(childNode, drsOps, &streamConfig->stream[cnt]) != HDF_SUCCESS) {
            HDF_LOGE("%s: Parse stream[%{public}d] failed!", __func__, cnt);
            return HDF_FAILURE;
        }
        cnt++;
        streamConfig->streamNum++;
    }
    HDF_LOGD("%s: streamConfig->streamNum = %{public}d!", __func__, streamConfig->streamNum);

    return HDF_SUCCESS;
}

static int32_t ParseCameraDeviceConfig(const struct DeviceResourceNode *node,
    struct DeviceResourceIface *drsOps, struct CameraDeviceConfig *deviceConfig)
{
    const struct DeviceResourceNode *sensorConfigNode = NULL;
    const struct DeviceResourceNode *ispConfigNode = NULL;
    const struct DeviceResourceNode *vcmConfigNode = NULL;
    const struct DeviceResourceNode *lensConfigNode = NULL;
    const struct DeviceResourceNode *flashConfigNode = NULL;
    const struct DeviceResourceNode *streamConfigNode = NULL;

    if (drsOps->GetString(node, "deviceName", &deviceConfig->deviceName, NULL) != HDF_SUCCESS) {
        HDF_LOGE("%s: get deviceName fail!", __func__);
        return HDF_FAILURE;
    }
    sensorConfigNode = drsOps->GetChildNode(node, "SensorConfig");
    ispConfigNode = drsOps->GetChildNode(node, "IspConfig");
    vcmConfigNode = drsOps->GetChildNode(node, "VcmConfig");
    lensConfigNode = drsOps->GetChildNode(node, "LensConfig");
    flashConfigNode = drsOps->GetChildNode(node, "FlashConfig");
    streamConfigNode = drsOps->GetChildNode(node, "StreamConfig");
    if (sensorConfigNode == NULL || ispConfigNode == NULL || vcmConfigNode == NULL ||
        lensConfigNode == NULL || flashConfigNode == NULL || streamConfigNode == NULL) {
        HDF_LOGE("%s: get child node fail!", __func__);
        return HDF_FAILURE;
    }
    CHECK_PARSER_CONFIG_RET(ParseCameraSensorConfig(sensorConfigNode, drsOps, &deviceConfig->sensor), "SensorConfig");
    CHECK_PARSER_CONFIG_RET(ParseCameraIspConfig(ispConfigNode, drsOps, &deviceConfig->isp), "IspConfig");
    CHECK_PARSER_CONFIG_RET(ParseCameraVcmConfig(vcmConfigNode, drsOps, &deviceConfig->vcm), "VcmConfig");
    CHECK_PARSER_CONFIG_RET(ParseCameraLensConfig(lensConfigNode, drsOps, &deviceConfig->lens), "LensConfig");
    CHECK_PARSER_CONFIG_RET(ParseCameraFlashConfig(flashConfigNode, drsOps, &deviceConfig->flash), "FlashConfig");
    CHECK_PARSER_CONFIG_RET(ParseCameraStreamConfig(streamConfigNode, drsOps, &deviceConfig->stream), "StreamConfig");

    return HDF_SUCCESS;
}

static int32_t ParseCameraConfig(const struct DeviceResourceNode *node,
    struct DeviceResourceIface *drsOps, struct CameraConfigRoot *cameraConfig)
{
    struct DeviceResourceNode *childNode = NULL;
    uint32_t cnt = 0;
    int32_t ret;

    ret = drsOps->GetUint8(node, "uvcMode", &cameraConfig->uvcMode, 0);
    CHECK_PARSER_CONFIG_RET(ret, "uvcMode");

    if (cameraConfig->uvcMode == DEVICE_NOT_SUPPORT) {
        HDF_LOGD("%s: not support uvc!", __func__);
    }

    DEV_RES_NODE_FOR_EACH_CHILD_NODE(node, childNode)
    {
        if (ParseCameraDeviceConfig(childNode, drsOps, &cameraConfig->deviceConfig[cnt]) != HDF_SUCCESS) {
            return HDF_FAILURE;
        }
        cnt++;
        cameraConfig->deviceNum++;
    }
    HDF_LOGD("%s: uvcMode = %{public}d, cameraConfig->deviceNum = %{public}d!", __func__,
        cameraConfig->uvcMode, cameraConfig->deviceNum);

    return HDF_SUCCESS;
}

int32_t HdfParseCameraConfig(const struct DeviceResourceNode *node)
{
    struct DeviceResourceIface *drsOps = NULL;
    const struct DeviceResourceNode *cameraConfigNode = NULL;
    int32_t ret;

    if (node == NULL) {
        HDF_LOGE("%s: node is null!", __func__);
        return HDF_ERR_INVALID_PARAM;
    }
    drsOps = DeviceResourceGetIfaceInstance(HDF_CONFIG_SOURCE);
    if (drsOps == NULL || drsOps->GetString == NULL || drsOps->GetUint8 == NULL ||
        drsOps->GetUint8Array == NULL || drsOps->GetUint32 == NULL ||
        drsOps->GetElemNum == NULL || drsOps->GetUint32Array == NULL) {
        HDF_LOGE("%s: invalid drs ops fail!", __func__);
        return HDF_FAILURE;
    }

    cameraConfigNode = drsOps->GetChildNode(node, "abilityConfig");
    if (cameraConfigNode == NULL) {
        HDF_LOGE("%s: get child node fail!", __func__);
        return HDF_FAILURE;
    }
    ret = ParseCameraConfig(cameraConfigNode, drsOps, &g_configCameraRoot);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: ParseCameraConfig failed!", __func__);
    }
    return ret;
}

struct CameraConfigRoot *HdfCameraGetConfigRoot(void)
{
    return &g_configCameraRoot;
}
