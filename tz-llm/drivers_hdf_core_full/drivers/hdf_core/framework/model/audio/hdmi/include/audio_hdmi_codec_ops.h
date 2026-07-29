/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#ifndef AUDIO_HDMI_CODEC_OPS_H
#define AUDIO_HDMI_CODEC_OPS_H

#include <linux/types.h>
#include "audio_codec_if.h"

int32_t AudioHdmiCodecDeviceInit(struct AudioCard *audioCard, const struct CodecDevice *device);
int32_t AudioHdmiCodecDaiDeviceInit(struct AudioCard *card, const struct DaiDevice *device);
int32_t AudioHdmiCodecDaiStartup(const struct AudioCard *card, const struct DaiDevice *device);
int32_t AudioHdmiCodecDaiHwParams(const struct AudioCard *card, const struct AudioPcmHwParams *param);
int32_t AudioHdmiCodecDaiShutdown(const struct AudioCard *card, const struct DaiDevice *device);
int32_t AudioHdmiCodecDaiMuteStream(
    const struct AudioCard *audioCard, const struct DaiDevice *dai, bool mute, int32_t direction);
int32_t AudioHdmiCodecReadReg(const struct CodecDevice *codec, uint32_t reg, uint32_t *val);
int32_t AudioHdmiCodecWriteReg(const struct CodecDevice *codec, uint32_t reg, uint32_t value);
int32_t AudioHdmiCodecDaiReadReg(const struct DaiDevice *dai, uint32_t reg, uint32_t *value);
int32_t AudioHdmiCodecDaiWriteReg(const struct DaiDevice *dai, uint32_t reg, uint32_t value);

#endif
