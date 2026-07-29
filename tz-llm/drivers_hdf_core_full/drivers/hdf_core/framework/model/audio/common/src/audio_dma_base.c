/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include "audio_driver_log.h"
#include "audio_parse.h"
#include "audio_platform_if.h"

#define HDF_LOG_TAG HDF_AUDIO_KADM
#define DMA_TRANSFER_MAX_COUNT 12   // Support 96000 ~ 8000 sampling rate

int32_t AudioDmaBufAlloc(struct PlatformData *data, enum AudioStreamType streamType)
{
    if (data != NULL && data->ops != NULL && data->ops->DmaBufAlloc != NULL) {
        return data->ops->DmaBufAlloc(data, streamType);
    }
    return HDF_FAILURE;
}

int32_t AudioDmaBufFree(struct PlatformData *data, enum AudioStreamType streamType)
{
    if (data != NULL && data->ops != NULL && data->ops->DmaBufFree != NULL) {
        return data->ops->DmaBufFree(data, streamType);
    }
    return HDF_FAILURE;
}

int32_t AudioDmaRequestChannel(struct PlatformData *data, enum AudioStreamType streamType)
{
    if (data != NULL && data->ops != NULL && data->ops->DmaRequestChannel != NULL) {
        return data->ops->DmaRequestChannel(data, streamType);
    }
    return HDF_FAILURE;
}

int32_t AudioDmaConfigChannel(struct PlatformData *data, enum AudioStreamType streamType)
{
    if (data != NULL && data->ops != NULL && data->ops->DmaConfigChannel != NULL) {
        return data->ops->DmaConfigChannel(data, streamType);
    }
    return HDF_FAILURE;
}

int32_t AudioDmaPrep(struct PlatformData *data, enum AudioStreamType streamType)
{
    if (data != NULL && data->ops != NULL && data->ops->DmaPrep != NULL) {
        return data->ops->DmaPrep(data, streamType);
    }
    return HDF_FAILURE;
}

int32_t AudioDmaSubmit(struct PlatformData *data, enum AudioStreamType streamType)
{
    if (data != NULL && data->ops != NULL && data->ops->DmaSubmit != NULL) {
        return data->ops->DmaSubmit(data, streamType);
    }
    return HDF_FAILURE;
}

int32_t AudioDmaPending(struct PlatformData *data, enum AudioStreamType streamType)
{
    if (data != NULL && data->ops != NULL && data->ops->DmaPending != NULL) {
        return data->ops->DmaPending(data, streamType);
    }
    return HDF_FAILURE;
}

int32_t AudioDmaPause(struct PlatformData *data, enum AudioStreamType streamType)
{
    if (data != NULL && data->ops != NULL && data->ops->DmaPause != NULL) {
        return data->ops->DmaPause(data, streamType);
    }
    return HDF_FAILURE;
}

int32_t AudioDmaResume(struct PlatformData *data, enum AudioStreamType streamType)
{
    if (data != NULL && data->ops != NULL && data->ops->DmaResume != NULL) {
        return data->ops->DmaResume(data, streamType);
    }
    return HDF_FAILURE;
}

int32_t AudioDmaPointer(struct PlatformData *data, enum AudioStreamType streamType, uint32_t *pointer)
{
    if (data != NULL && data->ops != NULL && data->ops->DmaPointer != NULL) {
        return data->ops->DmaPointer(data, streamType, pointer);
    }
    return HDF_FAILURE;
}

int32_t AudioDmaGetConfigInfo(const struct HdfDeviceObject *device, struct PlatformData *data)
{
    if (device == NULL || data == NULL) {
        AUDIO_DRIVER_LOG_ERR("param is null!");
        return HDF_ERR_INVALID_PARAM;
    }

    if (data->regConfig != NULL) {
        AUDIO_DRIVER_LOG_INFO("regConfig has been parsed!");
        return HDF_SUCCESS;
    }

    data->regConfig = (struct AudioRegCfgData *)OsalMemCalloc(sizeof(struct AudioRegCfgData));
    if (data->regConfig == NULL) {
        AUDIO_DRIVER_LOG_ERR("malloc AudioRegCfgData fail!");
        return HDF_FAILURE;
    }

    if (AudioGetRegConfig(device, data->regConfig) != HDF_SUCCESS) {
        AUDIO_DRIVER_LOG_ERR("dai GetRegConfig fail!");
        OsalMemFree(data->regConfig);
        data->regConfig = NULL;
        return HDF_FAILURE;
    }

    return HDF_SUCCESS;
}

bool AudioDmaTransferStatusIsNormal(struct PlatformData *data, enum AudioStreamType streamType)
{
    if (data == NULL) {
        AUDIO_DRIVER_LOG_ERR("param is null!");
        return false;
    }

    if (streamType == AUDIO_RENDER_STREAM) {
        if (data->renderBufInfo.rbufOffSet == data->renderBufInfo.wbufOffSet) {
            data->renderBufInfo.trafCompCount++;
            if (data->renderBufInfo.trafCompCount > DMA_TRANSFER_MAX_COUNT) {
                AUDIO_DRIVER_LOG_ERR("audio render send data to DMA too slow DMA will stop!");
                return false;
            }
        } else {
            data->renderBufInfo.rbufOffSet = data->renderBufInfo.wbufOffSet;
            data->renderBufInfo.trafCompCount = 0;
        }
    } else {
        if (data->captureBufInfo.wbufOffSet == data->captureBufInfo.rbufOffSet) {
            data->captureBufInfo.trafCompCount++;
            if (data->captureBufInfo.trafCompCount > DMA_TRANSFER_MAX_COUNT) {
                AUDIO_DRIVER_LOG_ERR("audio capture retrieve data from DMA too slow DMA will stop!");
                return false;
            }
        } else {
            data->captureBufInfo.wbufOffSet = data->captureBufInfo.rbufOffSet;
            data->captureBufInfo.trafCompCount = 0;
        }
    }

    return true;
}
