/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include "audio_driver_log.h"
#include "audio_hdmi_codec_linux.h"
#include "audio_sapm.h"

#define HDF_LOG_TAG HDF_AUDIO_HDMI

#define HDMI_CODEC_CHMAP_IDX_UNKNOWN (-1)

#define CARD_ID_INDEX_0  0
#define CARD_ID_INDEX_1  1
#define CARD_ID_INDEX_2  2
#define CARD_ID_INDEX_3  3
#define CARD_ID_INDEX_4  4
#define CARD_ID_INDEX_5  5
#define CARD_ID_INDEX_6  6
#define CARD_ID_INDEX_7  7
#define CARD_ID_INDEX_8  8
#define CARD_ID_INDEX_9  9
#define CARD_ID_INDEX_10 0xa
#define CARD_ID_INDEX_11 0xb
#define CARD_ID_INDEX_12 0xc
#define CARD_ID_INDEX_13 0xd
#define CARD_ID_INDEX_14 0xe
#define CARD_ID_INDEX_15 0xf
#define CARD_ID_INDEX_16 0x10
#define CARD_ID_INDEX_17 0x11
#define CARD_ID_INDEX_18 0x12
#define CARD_ID_INDEX_19 0x13
#define CARD_ID_INDEX_20 0x14
#define CARD_ID_INDEX_21 0x15
#define CARD_ID_INDEX_22 0x16
#define CARD_ID_INDEX_23 0x17
#define CARD_ID_INDEX_24 0x18
#define CARD_ID_INDEX_25 0x19
#define CARD_ID_INDEX_26 0x1a
#define CARD_ID_INDEX_27 0x1b
#define CARD_ID_INDEX_28 0x1c
#define CARD_ID_INDEX_29 0x1d
#define CARD_ID_INDEX_30 0x1e
#define CARD_ID_INDEX_31 0x1f

#define CHANNEL_NUM_2 2
#define CHANNEL_NUM_4 4
#define CHANNEL_NUM_6 6
#define CHANNEL_NUM_8 8

enum HdmiCodecCeaSpkPlacement {
    FL = BIT(0),   /* Front Left           */
    FC = BIT(1),   /* Front Center         */
    FR = BIT(2),   /* Front Right          */
    FLC = BIT(3),  /* Front Left Center    */
    FRC = BIT(4),  /* Front Right Center   */
    RL = BIT(5),   /* Rear Left            */
    RC = BIT(6),   /* Rear Center          */
    RR = BIT(7),   /* Rear Right           */
    RLC = BIT(8),  /* Rear Left Center     */
    RRC = BIT(9),  /* Rear Right Center    */
    LFE = BIT(10), /* Low Frequency Effect */
};

/*
 * cea Speaker allocation structure
 */
struct HdmiCodecCeaSpkAlloc {
    const int32_t cardId;
    uint32_t channelNum;
    uint32_t mask;
};

/*
 * g_hdmiCodecCeaSpkAlloc: speaker configuration available for CEA
 *
 * This is an ordered list that must match with hdmi_codec_8ch_chmaps struct
 * The preceding ones have better chances to be selected by
 * HdmiCodecGetChAllocTableIdx().
 */
static const struct HdmiCodecCeaSpkAlloc g_hdmiCodecCeaSpkAlloc[] = {
    {.cardId = CARD_ID_INDEX_0,  .channelNum = CHANNEL_NUM_2, .mask = FL | FR                                 },
 /* 2.1 */
    {.cardId = CARD_ID_INDEX_1,  .channelNum = CHANNEL_NUM_4, .mask = FL | FR | LFE                           },
 /* Dolby Surround */
    {.cardId = CARD_ID_INDEX_2,  .channelNum = CHANNEL_NUM_4, .mask = FL | FR | FC                            },
 /* surround51 */
    {.cardId = CARD_ID_INDEX_11, .channelNum = CHANNEL_NUM_6, .mask = FL | FR | LFE | FC | RL | RR            },
 /* surround40 */
    {.cardId = CARD_ID_INDEX_8,  .channelNum = CHANNEL_NUM_6, .mask = FL | FR | RL | RR                       },
 /* surround41 */
    {.cardId = CARD_ID_INDEX_9,  .channelNum = CHANNEL_NUM_6, .mask = FL | FR | LFE | RL | RR                 },
 /* surround50 */
    {.cardId = CARD_ID_INDEX_10, .channelNum = CHANNEL_NUM_6, .mask = FL | FR | FC | RL | RR                  },
 /* 6.1 */
    {.cardId = CARD_ID_INDEX_15, .channelNum = CHANNEL_NUM_8, .mask = FL | FR | LFE | FC | RL | RR | RC       },
 /* surround71 */
    {.cardId = CARD_ID_INDEX_19, .channelNum = CHANNEL_NUM_8, .mask = FL | FR | LFE | FC | RL | RR | RLC | RRC},
 /* others */
    {.cardId = CARD_ID_INDEX_3,  .channelNum = CHANNEL_NUM_8, .mask = FL | FR | LFE | FC                      },
    {.cardId = CARD_ID_INDEX_4,  .channelNum = CHANNEL_NUM_8, .mask = FL | FR | RC                            },
    {.cardId = CARD_ID_INDEX_5,  .channelNum = CHANNEL_NUM_8, .mask = FL | FR | LFE | RC                      },
    {.cardId = CARD_ID_INDEX_6,  .channelNum = CHANNEL_NUM_8, .mask = FL | FR | FC | RC                       },
    {.cardId = CARD_ID_INDEX_7,  .channelNum = CHANNEL_NUM_8, .mask = FL | FR | LFE | FC | RC                 },
    {.cardId = CARD_ID_INDEX_12, .channelNum = CHANNEL_NUM_8, .mask = FL | FR | RC | RL | RR                  },
    {.cardId = CARD_ID_INDEX_13, .channelNum = CHANNEL_NUM_8, .mask = FL | FR | LFE | RL | RR | RC            },
    {.cardId = CARD_ID_INDEX_14, .channelNum = CHANNEL_NUM_8, .mask = FL | FR | FC | RL | RR | RC             },
    {.cardId = CARD_ID_INDEX_16, .channelNum = CHANNEL_NUM_8, .mask = FL | FR | RL | RR | RLC | RRC           },
    {.cardId = CARD_ID_INDEX_17, .channelNum = CHANNEL_NUM_8, .mask = FL | FR | LFE | RL | RR | RLC | RRC     },
    {.cardId = CARD_ID_INDEX_18, .channelNum = CHANNEL_NUM_8, .mask = FL | FR | FC | RL | RR | RLC | RRC      },
    {.cardId = CARD_ID_INDEX_20, .channelNum = CHANNEL_NUM_8, .mask = FL | FR | FLC | FRC                     },
    {.cardId = CARD_ID_INDEX_21, .channelNum = CHANNEL_NUM_8, .mask = FL | FR | LFE | FLC | FRC               },
    {.cardId = CARD_ID_INDEX_22, .channelNum = CHANNEL_NUM_8, .mask = FL | FR | FC | FLC | FRC                },
    {.cardId = CARD_ID_INDEX_23, .channelNum = CHANNEL_NUM_8, .mask = FL | FR | LFE | FC | FLC | FRC          },
    {.cardId = CARD_ID_INDEX_24, .channelNum = CHANNEL_NUM_8, .mask = FL | FR | RC | FLC | FRC                },
    {.cardId = CARD_ID_INDEX_25, .channelNum = CHANNEL_NUM_8, .mask = FL | FR | LFE | RC | FLC | FRC          },
    {.cardId = CARD_ID_INDEX_26, .channelNum = CHANNEL_NUM_8, .mask = FL | FR | RC | FC | FLC | FRC           },
    {.cardId = CARD_ID_INDEX_27, .channelNum = CHANNEL_NUM_8, .mask = FL | FR | LFE | RC | FC | FLC | FRC     },
    {.cardId = CARD_ID_INDEX_28, .channelNum = CHANNEL_NUM_8, .mask = FL | FR | RL | RR | FLC | FRC           },
    {.cardId = CARD_ID_INDEX_29, .channelNum = CHANNEL_NUM_8, .mask = FL | FR | LFE | RL | RR | FLC | FRC     },
    {.cardId = CARD_ID_INDEX_30, .channelNum = CHANNEL_NUM_8, .mask = FL | FR | FC | RL | RR | FLC | FRC      },
    {.cardId = CARD_ID_INDEX_31, .channelNum = CHANNEL_NUM_8, .mask = FL | FR | LFE | FC | RL | RR | FLC | FRC},
};

static const struct AudioSapmRoute g_audioRoutes[] = {
    {"SPKL",     NULL,                "SPKL PGA"},
    {"HPL",      NULL,                "HPL PGA" },
    {"HPR",      NULL,                "HPR PGA" },
    {"SPKL PGA", "Speaker1 Switch",   "DAC1"    },
    {"HPL PGA",  "Headphone1 Switch", "DAC2"    },
    {"HPR PGA",  "Headphone2 Switch", "DAC3"    },

    {"ADCL",     NULL,                "LPGA"    },
    {"ADCR",     NULL,                "RPGA"    },
    {"LPGA",     "LPGA MIC Switch",   "MIC1"    },
    {"RPGA",     "RPGA MIC Switch",   "MIC2"    },
};

int32_t AudioHdmiCodecDeviceInit(struct AudioCard *audioCard, const struct CodecDevice *device)
{
    if ((audioCard == NULL) || (device == NULL) || device->devData == NULL) {
        AUDIO_DRIVER_LOG_ERR("input para is NULL.");
        return HDF_ERR_INVALID_OBJECT;
    }

    if (AudioAddControls(audioCard, device->devData->controls, device->devData->numControls) != HDF_SUCCESS) {
        AUDIO_DRIVER_LOG_ERR("add controls failed.");
        return HDF_FAILURE;
    }

    if (AudioSapmNewComponents(audioCard, device->devData->sapmComponents, device->devData->numSapmComponent) !=
        HDF_SUCCESS) {
        AUDIO_DRIVER_LOG_ERR("new components failed.");
        return HDF_FAILURE;
    }

    if (AudioSapmAddRoutes(audioCard, g_audioRoutes, HDF_ARRAY_SIZE(g_audioRoutes)) != HDF_SUCCESS) {
        AUDIO_DRIVER_LOG_ERR("add route failed.");
        return HDF_FAILURE;
    }

    if (AudioSapmNewControls(audioCard) != HDF_SUCCESS) {
        AUDIO_DRIVER_LOG_ERR("add sapm controls failed.");
        return HDF_FAILURE;
    }

    AUDIO_DRIVER_LOG_DEBUG("success.");
    return HDF_SUCCESS;
}

int32_t AudioHdmiCodecDaiDeviceInit(struct AudioCard *card, const struct DaiDevice *device)
{
    struct HdmiCodecPriv *hdmiCodecPriv = NULL;

    hdmiCodecPriv = AudioGetHdmiCodecPriv();
    if (hdmiCodecPriv == NULL) {
        AUDIO_DRIVER_LOG_ERR("HdmiCodecPriv is null.");
        return HDF_FAILURE;
    }

    (void)card;
    (void)device;

    mutex_lock(&hdmiCodecPriv->lock);
    hdmiCodecPriv->busy = false;
    mutex_unlock(&hdmiCodecPriv->lock);

    AUDIO_DRIVER_LOG_DEBUG("success.");
    return HDF_SUCCESS;
}

int32_t AudioHdmiCodecDaiStartup(const struct AudioCard *card, const struct DaiDevice *device)
{
    int32_t ret;
    struct HdmiCodecPriv *hdmiCodecPriv = NULL;

    hdmiCodecPriv = AudioGetHdmiCodecPriv();
    if (hdmiCodecPriv == NULL) {
        AUDIO_DRIVER_LOG_ERR("HdmiCodecPriv is null.");
        return HDF_FAILURE;
    }

    (void)card;
    (void)device;
    mutex_lock(&hdmiCodecPriv->lock);
    if (hdmiCodecPriv->busy) {
        AUDIO_DRIVER_LOG_ERR("Only one simultaneous stream supported!");
        mutex_unlock(&hdmiCodecPriv->lock);
        return HDF_FAILURE;
    }

    if (hdmiCodecPriv->hdmiCodecData.ops == NULL) {
        mutex_unlock(&hdmiCodecPriv->lock);
        AUDIO_DRIVER_LOG_ERR("hdmiCodecData.ops is null.");
        return HDF_FAILURE;
    }
    if (hdmiCodecPriv->hdmiCodecData.ops->audio_startup != NULL) {
        ret = hdmiCodecPriv->hdmiCodecData.ops->audio_startup(hdmiCodecPriv->dev, hdmiCodecPriv->hdmiCodecData.data);
        if (ret != 0) {
            mutex_unlock(&hdmiCodecPriv->lock);
            AUDIO_DRIVER_LOG_ERR("audio_startup is failed.");
            return ret;
        }
    }

    hdmiCodecPriv->busy = true;
    mutex_unlock(&hdmiCodecPriv->lock);

    AUDIO_DRIVER_LOG_DEBUG("success.");
    return HDF_SUCCESS;
}

int32_t AudioHdmiCodecDaiShutdown(const struct AudioCard *card, const struct DaiDevice *device)
{
    struct HdmiCodecPriv *hdmiCodecPriv = NULL;

    (void)card;
    (void)device;
    hdmiCodecPriv = AudioGetHdmiCodecPriv();
    if (hdmiCodecPriv == NULL) {
        AUDIO_DRIVER_LOG_ERR("hdmiCodecPriv is null.");
        return HDF_FAILURE;
    }

    hdmiCodecPriv->chmapIdx = HDMI_CODEC_CHMAP_IDX_UNKNOWN;
    if (hdmiCodecPriv->hdmiCodecData.ops == NULL || hdmiCodecPriv->hdmiCodecData.ops->audio_shutdown == NULL) {
        AUDIO_DRIVER_LOG_ERR("hdmiCodecData.ops->audio_shutdown is null.");
        return HDF_FAILURE;
    }
    hdmiCodecPriv->hdmiCodecData.ops->audio_shutdown(hdmiCodecPriv->dev, hdmiCodecPriv->hdmiCodecData.data);

    mutex_lock(&hdmiCodecPriv->lock);
    hdmiCodecPriv->busy = false;
    mutex_unlock(&hdmiCodecPriv->lock);

    AUDIO_DRIVER_LOG_DEBUG("success.");
    return HDF_SUCCESS;
}

static unsigned long HdmiCodecSpkMaskFromAlloc(uint32_t spkAlloc)
{
    uint32_t index;
    uint32_t temp;
    static const uint32_t hdmiCodecEldSpkAllocBits[] = {
        FL | FR, LFE, FC, RL | RR, RC, FLC | FRC, RLC | RRC,
    };
    uint32_t spkMask = 0;

    for (index = 0; index < HDF_ARRAY_SIZE(hdmiCodecEldSpkAllocBits); index++) {
        temp = spkAlloc & (1 << index);
        if (temp != 0) {
            spkMask |= hdmiCodecEldSpkAllocBits[index];
        }
    }
    return spkMask;
}

static int32_t HdmiCodecGetChAllocTableIdx(struct HdmiCodecPriv *hdmiCodecPriv, uint8_t channels, int32_t *index)
{
    int32_t item;
    uint8_t spkAlloc;
    unsigned long spkMask;

    spkAlloc = drm_eld_get_spk_alloc(hdmiCodecPriv->eld);
    spkMask = HdmiCodecSpkMaskFromAlloc(spkAlloc);

    for (item = 0; item < HDF_ARRAY_SIZE(g_hdmiCodecCeaSpkAlloc); item++) {
        if (spkAlloc == 0 && g_hdmiCodecCeaSpkAlloc[item].cardId == 0) {
            *index = item;
            return HDF_SUCCESS;
        }
        if (g_hdmiCodecCeaSpkAlloc[item].channelNum != channels) {
            continue;
        }
        if (!(g_hdmiCodecCeaSpkAlloc[item].mask == (spkMask & g_hdmiCodecCeaSpkAlloc[item].mask))) {
            continue;
        }
        *index = item;
        return HDF_SUCCESS;
    }
    return HDF_FAILURE;
}

int32_t AudioHdmiCodecDaiHwParams(const struct AudioCard *card, const struct AudioPcmHwParams *param)
{
    struct HdmiCodecPriv *hdmiCodecPriv = NULL;
    struct hdmi_codec_daifmt hdmiCodecDaifmt;
    struct hdmi_codec_params hdmiCodecParams;
    int32_t ret;
    int32_t idx = 0;

    (void)card;
    if (param == NULL) {
        AUDIO_DRIVER_LOG_ERR("input param is null.");
        return HDF_FAILURE;
    }

    hdmiCodecPriv = AudioGetHdmiCodecPriv();
    if (hdmiCodecPriv == NULL) {
        AUDIO_DRIVER_LOG_ERR("hdmiCodecPriv is null.");
        return HDF_FAILURE;
    }

    (void)memset_s(&hdmiCodecParams, sizeof(struct hdmi_codec_params), 0, sizeof(struct hdmi_codec_params));
    (void)memset_s(&hdmiCodecDaifmt, sizeof(struct hdmi_codec_daifmt), 0, sizeof(struct hdmi_codec_daifmt));
#ifdef CONFIG_DRM_DW_HDMI_I2S_AUDIO
    hdmi_audio_infoframe_init(&hdmiCodecParams.cea);
#endif
    hdmiCodecParams.cea.channels = param->channels;
    hdmiCodecParams.cea.coding_type = HDMI_AUDIO_CODING_TYPE_STREAM;
    hdmiCodecParams.cea.sample_size = HDMI_AUDIO_SAMPLE_SIZE_STREAM;
    hdmiCodecParams.cea.sample_frequency = HDMI_AUDIO_SAMPLE_FREQUENCY_STREAM;

    /* Select a channel allocation that matches with ELD and pcm channels */
    ret = HdmiCodecGetChAllocTableIdx(hdmiCodecPriv, hdmiCodecParams.cea.channels, &idx);
    if (ret != HDF_SUCCESS) {
        AUDIO_DRIVER_LOG_ERR("Not able to map channels to speakers", ret);
        hdmiCodecPriv->chmapIdx = HDMI_CODEC_CHMAP_IDX_UNKNOWN;
        return ret;
    }
    hdmiCodecParams.cea.channel_allocation = g_hdmiCodecCeaSpkAlloc[idx].cardId;
    hdmiCodecPriv->chmapIdx = g_hdmiCodecCeaSpkAlloc[idx].cardId;

    hdmiCodecParams.sample_width = param->format;
    hdmiCodecParams.sample_rate = param->rate;
    hdmiCodecParams.channels = param->channels;

    if (hdmiCodecPriv->hdmiCodecData.ops == NULL || hdmiCodecPriv->hdmiCodecData.ops->hw_params == NULL) {
        AUDIO_DRIVER_LOG_ERR("hdmiCodecData.ops->audio_shutdown is null.");
        return HDF_FAILURE;
    }
    ret = hdmiCodecPriv->hdmiCodecData.ops->hw_params(
        hdmiCodecPriv->dev, hdmiCodecPriv->hdmiCodecData.data, &hdmiCodecDaifmt, &hdmiCodecParams);
    if (ret != HDF_SUCCESS) {
        AUDIO_DRIVER_LOG_ERR("hw_params is failed.");
        return ret;
    }

    AUDIO_DRIVER_LOG_DEBUG("success.");
    return HDF_SUCCESS;
}

int32_t AudioHdmiCodecDaiMuteStream(
    const struct AudioCard *audioCard, const struct DaiDevice *dai, bool mute, int32_t direction)
{
    struct HdmiCodecPriv *hdmiCodecPriv = NULL;
    int32_t ret;

    (void)audioCard;
    (void)dai;
    hdmiCodecPriv = AudioGetHdmiCodecPriv();
    if (hdmiCodecPriv == NULL) {
        AUDIO_DRIVER_LOG_ERR("hdmiCodecPriv is null.");
        return HDF_FAILURE;
    }

    if (hdmiCodecPriv->hdmiCodecData.ops == NULL) {
        AUDIO_DRIVER_LOG_ERR("hdmiCodecPriv->hdmiCodecData.ops is null.");
        return HDF_FAILURE;
    }
    if (hdmiCodecPriv->hdmiCodecData.ops->mute_stream != NULL &&
        (direction == AUDIO_RENDER_STREAM_OUT || !hdmiCodecPriv->hdmiCodecData.ops->no_capture_mute)) {
        ret = hdmiCodecPriv->hdmiCodecData.ops->mute_stream(
            hdmiCodecPriv->dev, hdmiCodecPriv->hdmiCodecData.data, mute, direction);
        if (ret != HDF_SUCCESS) {
            AUDIO_DRIVER_LOG_ERR("mute_stream is failed.");
            return ret;
        }
    }
    AUDIO_DRIVER_LOG_DEBUG("success.");
    return HDF_SUCCESS;
}

int32_t AudioHdmiCodecReadReg(const struct CodecDevice *codec, uint32_t reg, uint32_t *val)
{
    (void)codec;
    (void)reg;
    (void)val;

    AUDIO_DRIVER_LOG_DEBUG("success.");
    return HDF_SUCCESS;
}

int32_t AudioHdmiCodecWriteReg(const struct CodecDevice *codec, uint32_t reg, uint32_t value)
{
    (void)codec;
    (void)reg;
    (void)value;

    AUDIO_DRIVER_LOG_DEBUG("success.");
    return HDF_SUCCESS;
}

int32_t AudioHdmiCodecDaiReadReg(const struct DaiDevice *dai, uint32_t reg, uint32_t *value)
{
    (void)dai;
    (void)reg;
    (void)value;

    AUDIO_DRIVER_LOG_DEBUG("success.");
    return HDF_SUCCESS;
}

int32_t AudioHdmiCodecDaiWriteReg(const struct DaiDevice *dai, uint32_t reg, uint32_t value)
{
    (void)dai;
    (void)reg;
    (void)value;

    AUDIO_DRIVER_LOG_DEBUG("success.");
    return HDF_SUCCESS;
}
