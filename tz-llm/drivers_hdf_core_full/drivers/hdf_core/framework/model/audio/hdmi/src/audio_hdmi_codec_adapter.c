/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include "audio_codec_base.h"
#include "audio_codec_if.h"
#include "audio_driver_log.h"
#include "audio_hdmi_codec_ops.h"

#define HDF_LOG_TAG HDF_AUDIO_HDMI

static struct CodecData g_audioHdmiCodecData = {
    .Init = AudioHdmiCodecDeviceInit,
    .Read = AudioHdmiCodecReadReg,
    .Write = AudioHdmiCodecWriteReg,
};

static struct AudioDaiOps g_audioHdmiDaiDeviceOps = {
    .Startup = AudioHdmiCodecDaiStartup,
    .HwParams = AudioHdmiCodecDaiHwParams,
    .Shutdown = AudioHdmiCodecDaiShutdown,
    .MuteStream = AudioHdmiCodecDaiMuteStream,
};

static struct DaiData g_audioHdmiDaiData = {
    .DaiInit = AudioHdmiCodecDaiDeviceInit,
    .ops = &g_audioHdmiDaiDeviceOps,
    .Read = AudioHdmiCodecDaiReadReg,
    .Write = AudioHdmiCodecDaiWriteReg,
};

/* HdfDriverEntry implementations */
static int32_t AudioHdmiCodecDriverBind(struct HdfDeviceObject *device)
{
    struct CodecHost *codecHost = NULL;

    AUDIO_DRIVER_LOG_INFO("entry!");
    if (device == NULL) {
        AUDIO_DRIVER_LOG_ERR("input para is NULL.");
        return HDF_ERR_INVALID_PARAM;
    }

    codecHost = (struct CodecHost *)OsalMemCalloc(sizeof(*codecHost));
    if (codecHost == NULL) {
        AUDIO_DRIVER_LOG_ERR("malloc codecHost fail!");
        return HDF_FAILURE;
    }
    codecHost->device = device;
    device->service = &codecHost->service;

    AUDIO_DRIVER_LOG_INFO("success!");
    return HDF_SUCCESS;
}

static int32_t AudioHdmiCodecDriverInit(struct HdfDeviceObject *device)
{
    int32_t ret;

    AUDIO_DRIVER_LOG_INFO("entry!");
    if (device == NULL) {
        AUDIO_DRIVER_LOG_ERR("device is NULL.");
        return HDF_ERR_INVALID_PARAM;
    }

    ret = CodecGetConfigInfo(device, &g_audioHdmiCodecData);
    if (ret != HDF_SUCCESS) {
        AUDIO_DRIVER_LOG_ERR("get config info failed.");
        return ret;
    }

    if (CodecDaiGetPortConfigInfo(device, &g_audioHdmiDaiData) != HDF_SUCCESS) {
        AUDIO_DRIVER_LOG_ERR("get port config info failed.");
        return HDF_FAILURE;
    }

    if (CodecSetConfigInfoOfControls(&g_audioHdmiCodecData, &g_audioHdmiDaiData) != HDF_SUCCESS) {
        AUDIO_DRIVER_LOG_ERR("set config info failed.");
        return HDF_FAILURE;
    }

    if (CodecGetServiceName(device, &g_audioHdmiCodecData.drvCodecName) != HDF_SUCCESS) {
        AUDIO_DRIVER_LOG_ERR("get codec service name failed.");
        return HDF_FAILURE;
    }

    if (CodecGetDaiName(device, &g_audioHdmiDaiData.drvDaiName) != HDF_SUCCESS) {
        AUDIO_DRIVER_LOG_ERR("get codec dai service name failed.");
        return HDF_FAILURE;
    }

    OsalMutexInit(&g_audioHdmiCodecData.mutex);

    ret = AudioRegisterCodec(device, &g_audioHdmiCodecData, &g_audioHdmiDaiData);
    if (ret != HDF_SUCCESS) {
        AUDIO_DRIVER_LOG_ERR("AudioRegisterCodec failed.");
        return ret;
    }
    AUDIO_DRIVER_LOG_INFO("success!");
    return HDF_SUCCESS;
}

static void AudioHdmiCodecDriverRelease(struct HdfDeviceObject *device)
{
    struct CodecHost *codecHost = NULL;

    AUDIO_DRIVER_LOG_INFO("entry!");
    if (device == NULL) {
        AUDIO_DRIVER_LOG_ERR("device is NULL");
        return;
    }

    OsalMutexDestroy(&g_audioHdmiCodecData.mutex);
    OsalMemFree(device->priv);

    codecHost = (struct CodecHost *)device->service;
    OsalMemFree(codecHost);
    AUDIO_DRIVER_LOG_INFO("success!");
}

struct HdfDriverEntry g_audioHdmiDriverEntry = {
    .moduleVersion = 1,
    .moduleName = "AUDIO_HDMI_CODEC",
    .Bind = AudioHdmiCodecDriverBind,
    .Init = AudioHdmiCodecDriverInit,
    .Release = AudioHdmiCodecDriverRelease,
};
HDF_INIT(g_audioHdmiDriverEntry);
