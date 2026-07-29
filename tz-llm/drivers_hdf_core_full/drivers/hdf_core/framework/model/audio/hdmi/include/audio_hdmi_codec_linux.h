/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#ifndef AUDIO_HDMI_CODEC_LINUX_H
#define AUDIO_HDMI_CODEC_LINUX_H

#include <drm/drm_crtc.h>
#include <sound/hdmi-codec.h>

struct HdmiCodecPriv {
    struct hdmi_codec_pdata hdmiCodecData;
    uint8_t eld[MAX_ELD_BYTES];
    struct snd_pcm_chmap *chmapInfo;
    uint32_t chmapIdx;
    struct mutex lock;
    bool busy;
    struct snd_soc_jack *jack;
    uint32_t jackStatus;
    struct device *dev;
};

struct HdmiCodecPriv *AudioGetHdmiCodecPriv(void);

#endif
