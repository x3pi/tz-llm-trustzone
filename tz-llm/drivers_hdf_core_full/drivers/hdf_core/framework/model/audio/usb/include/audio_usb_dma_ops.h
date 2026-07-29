/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#ifndef AUDIO_USB_DMA_OPS_H
#define AUDIO_USB_DMA_OPS_H

#include <linux/dmaengine.h>

#include "audio_core.h"

int32_t AudioUsbDmaDeviceInit(const struct AudioCard *card, const struct PlatformDevice *platform);
int32_t AudioUsbDmaBufAlloc(struct PlatformData *data, const enum AudioStreamType streamType);
int32_t AudioUsbDmaBufFree(struct PlatformData *data, const enum AudioStreamType streamType);
int32_t AudioUsbDmaRequestChannel(const struct PlatformData *data, const enum AudioStreamType streamType);
int32_t AudioUsbDmaConfigChannel(const struct PlatformData *data, const enum AudioStreamType streamType);
int32_t AudioUsbPcmPointer(struct PlatformData *data, const enum AudioStreamType streamType, uint32_t *pointer);
int32_t AudioUsbDmaPrep(const struct PlatformData *data, const enum AudioStreamType streamType);
int32_t AudioUsbDmaSubmit(const struct PlatformData *data, const enum AudioStreamType streamType);
int32_t AudioUsbDmaPending(struct PlatformData *data, const enum AudioStreamType streamType);
int32_t AudioUsbDmaPause(struct PlatformData *data, const enum AudioStreamType streamType);
int32_t AudioUsbDmaResume(const struct PlatformData *data, const enum AudioStreamType streamType);
void AudioUsbDmaDeviceRelease(struct PlatformData *platformData);

#endif