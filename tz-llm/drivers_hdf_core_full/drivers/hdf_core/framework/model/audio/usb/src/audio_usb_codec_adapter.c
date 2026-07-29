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
#include "audio_usb_codec_ops.h"

#define HDF_LOG_TAG HDF_AUDIO_USB

#define AUDIO_PORT_RENDER  1
#define AUDIO_PORT_CAPTURE 2

static struct CodecData g_usbCodecData = {
    .drvCodecName = "audio_usb_service_0",
    .Init = AudioUsbCodecDeviceInit,
};
static struct DaiData g_usbDaiData = {
    .drvDaiName = "usb_codec_dai",
    .DaiInit = AudioUsbCodecDaiDeviceInit,
};

static struct AudioPortInfo g_usbPortInfo = {
    .render.portDirection = AUDIO_PORT_RENDER,
    .capture.portDirection = AUDIO_PORT_CAPTURE,
};

static int32_t AudioUsbCodecDriverBind(struct HdfDeviceObject *device)
{
    struct CodecHost *audioUsbHost = NULL;

    ADM_LOG_DEBUG("entry.");
    if (device == NULL) {
        ADM_LOG_ERR("device is NULL.");
        return HDF_ERR_INVALID_PARAM;
    }

    audioUsbHost = (struct CodecHost *)OsalMemCalloc(sizeof(struct CodecHost));
    if (audioUsbHost == NULL) {
        ADM_LOG_ERR("Malloc audio host fail!");
        return HDF_ERR_MALLOC_FAIL;
    }

    audioUsbHost->device = device;
    device->service = &audioUsbHost->service;

    ADM_LOG_INFO("success.");
    return HDF_SUCCESS;
}

static int32_t AudioUsbCodecDriverInit(struct HdfDeviceObject *device)
{
    int32_t ret;

    ADM_LOG_DEBUG("entry.");
    if (device == NULL) {
        ADM_LOG_ERR("device is NULL.");
        return HDF_ERR_INVALID_OBJECT;
    }

    g_usbDaiData.portInfo = g_usbPortInfo;

    OsalMutexInit(&g_usbCodecData.mutex);

    ret = AudioRegisterCodec(device, &g_usbCodecData, &g_usbDaiData);
    if (ret != HDF_SUCCESS) {
        AUDIO_DRIVER_LOG_ERR("AudioRegisterCodec failed.");
        return ret;
    }

    ADM_LOG_INFO("success.");
    return HDF_SUCCESS;
}

static void AudioUsbCodecDriverRelease(struct HdfDeviceObject *device)
{
    struct AudioHost *audioHost = NULL;

    ADM_LOG_DEBUG("entry.");
    if (device == NULL) {
        ADM_LOG_ERR("device is NULL.");
        return;
    }

    audioHost = CONTAINER_OF(device->service, struct AudioHost, service);
    if (audioHost != NULL) {
        OsalMemFree(audioHost);
    }
    ADM_LOG_INFO("success.");
}

struct HdfDriverEntry g_audioUsbDriverEntry = {
    .moduleVersion = 1,
    .moduleName = "AUDIO_USB_CODEC",
    .Bind = AudioUsbCodecDriverBind,
    .Init = AudioUsbCodecDriverInit,
    .Release = AudioUsbCodecDriverRelease,
};
HDF_INIT(g_audioUsbDriverEntry);
