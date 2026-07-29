/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#ifndef AUDIO_USB_CODEC_OPS_H
#define AUDIO_USB_CODEC_OPS_H

#include "audio_core.h"

int32_t AudioUsbCodecDeviceInit(struct AudioCard *audioCard, const struct CodecDevice *device);
int32_t AudioUsbCodecDaiDeviceInit(struct AudioCard *card, const struct DaiDevice *device);

#endif