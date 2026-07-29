/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include <linux/slab.h>

#include "audio_core.h"
#include "audio_dma_base.h"
#include "audio_driver_log.h"
#include "audio_platform_base.h"
#include "audio_usb_dma_ops.h"
#include "audio_usb_linux.h"
#include "gpio_if.h"
#include "osal_io.h"
#include "osal_mem.h"

#define HDF_LOG_TAG HDF_AUDIO_USB

struct AudioDmaOps g_usbDmaDeviceOps = {
    .DmaBufAlloc = AudioUsbDmaBufAlloc,
    .DmaBufFree = AudioUsbDmaBufFree,
    .DmaRequestChannel = AudioUsbDmaRequestChannel,
    .DmaConfigChannel = AudioUsbDmaConfigChannel,
    .DmaPrep = AudioUsbDmaPrep,
    .DmaSubmit = AudioUsbDmaSubmit,
    .DmaPending = AudioUsbDmaPending,
    .DmaPause = AudioUsbDmaPause,
    .DmaResume = AudioUsbDmaResume,
    .DmaPointer = AudioUsbPcmPointer,
};

/* HdfDriverEntry implementations */
static int32_t AudioUsbDmaDriverBind(struct HdfDeviceObject *device)
{
    struct PlatformHost *platformUsbHost = NULL;
    AUDIO_DEVICE_LOG_DEBUG("entry!");

    if (device == NULL) {
        AUDIO_DEVICE_LOG_ERR("input para is NULL.");
        return HDF_FAILURE;
    }

    platformUsbHost = (struct PlatformHost *)OsalMemCalloc(sizeof(*platformUsbHost));
    if (platformUsbHost == NULL) {
        AUDIO_DEVICE_LOG_ERR("malloc platform usb host fail!");
        return HDF_FAILURE;
    }

    platformUsbHost->device = device;
    device->service = &platformUsbHost->service;

    AUDIO_DEVICE_LOG_DEBUG("success!");
    return HDF_SUCCESS;
}

static int32_t AudioUsbDmaDriverInit(struct HdfDeviceObject *device)
{
    int32_t ret;
    struct PlatformData *platformUsbData = NULL;
    struct PlatformHost *platformUsbHost = NULL;
    AUDIO_DEVICE_LOG_DEBUG("entry!");

    if (device == NULL) {
        AUDIO_DEVICE_LOG_ERR("device is NULL.");
        return HDF_ERR_INVALID_OBJECT;
    }
    platformUsbHost = (struct PlatformHost *)device->service;
    if (platformUsbHost == NULL) {
        AUDIO_DEVICE_LOG_ERR("platformUsbHost is NULL");
        return HDF_FAILURE;
    }

    platformUsbData = (struct PlatformData *)OsalMemCalloc(sizeof(*platformUsbData));
    if (platformUsbData == NULL) {
        AUDIO_DEVICE_LOG_ERR("malloc PlatformUsbData fail!");
        return HDF_FAILURE;
    }

    platformUsbData->PlatformInit = AudioUsbDmaDeviceInit;
    platformUsbData->ops = &g_usbDmaDeviceOps;
    platformUsbData->drvPlatformName = "usb_dma_service_0";

    OsalMutexInit(&platformUsbData->renderBufInfo.buffMutex);
    OsalMutexInit(&platformUsbData->captureBufInfo.buffMutex);
    ret = AudioSocRegisterPlatform(device, platformUsbData);
    if (ret != HDF_SUCCESS) {
        OsalMemFree(platformUsbData);
        return ret;
    }

    platformUsbHost->priv = platformUsbData;
    AUDIO_DEVICE_LOG_DEBUG("success.\n");
    return HDF_SUCCESS;
}

static void AudioUsbDmaDriverRelease(struct HdfDeviceObject *device)
{
    struct PlatformData *platformUsbData = NULL;
    struct PlatformHost *platformUsbHost = NULL;
    AUDIO_DEVICE_LOG_DEBUG("entry!");

    if (device == NULL) {
        AUDIO_DEVICE_LOG_ERR("audio usb device is NULL");
        return;
    }

    platformUsbHost = (struct PlatformHost *)device->service;
    if (platformUsbHost == NULL) {
        AUDIO_DEVICE_LOG_ERR("platformUsbHost is NULL");
        return;
    }

    platformUsbData = (struct PlatformData *)platformUsbHost->priv;
    if (platformUsbData != NULL) {
        AudioUsbDmaDeviceRelease(platformUsbData);
        OsalMutexDestroy(&platformUsbData->renderBufInfo.buffMutex);
        OsalMutexDestroy(&platformUsbData->captureBufInfo.buffMutex);
        OsalMemFree(platformUsbData);
    }

    OsalMemFree(platformUsbHost);
    AUDIO_DEVICE_LOG_DEBUG("audio usb dirver release success.");
    return;
}

struct HdfDriverEntry g_platformUsbDriverEntry = {
    .moduleVersion = 1,
    .moduleName = "AUDIO_USB_DMA",
    .Bind = AudioUsbDmaDriverBind,
    .Init = AudioUsbDmaDriverInit,
    .Release = AudioUsbDmaDriverRelease,
};
HDF_INIT(g_platformUsbDriverEntry);
