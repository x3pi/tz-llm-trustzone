/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include <linux/init.h>
#include <linux/slab.h>
#include <linux/usb.h>
#include <linux/usb/audio-v2.h>
#include <linux/usb/audio-v3.h>
#include <linux/usb/audio.h>

#include "audio_driver_log.h"
#include "audio_usb_linux.h"
#include "audio_usb_validate_desc.h"
#include "hdf_base.h"

#define HDF_LOG_TAG HDF_AUDIO_USB

/* Formats - A.2 Format Type Codes */
#define UAC_FORMAT_TYPE_UNDEFINED 0x0
#define UAC_FORMAT_TYPE_I         0x1
#define UAC_FORMAT_TYPE_II        0x2
#define UAC_FORMAT_TYPE_III       0x3

#define FRAME_SIZE_2        2
#define FORMAT_LRENGTH_6    6
#define MOVE_8_BIT          8
#define FORMAT_LRENGTH_8    8
#define MOVE_11_BIT         11
#define MOVE_16_BIT         16
#define SAMPLE_WIDTH        24
#define READ_MAX_ERR_COUNT  2
#define READ_RATE_ERR_COUNT 3

#define MOVE_VAL     (1u)
#define INVALID_RATE 0

#define AUDIO_USB_PCM_FMTBIT_S8       (1 << 0)
#define AUDIO_USB_PCM_FMTBIT_U8       (1 << 1)
#define AUDIO_USB_PCM_FMTBIT_S16_LE   (1 << 2)
#define AUDIO_USB_PCM_FMTBIT_S16_BE   (1 << 3)
#define AUDIO_USB_PCM_FMTBIT_S32_LE   (1 << 10)
#define AUDIO_USB_PCM_FMTBIT_FLOAT_LE (1 << 14)
#define AUDIO_USB_PCM_FMTBIT_MU_LAW   (1 << 20)
#define AUDIO_USB_PCM_FMTBIT_A_LAW    (1 << 21)
#define AUDIO_USB_PCM_FMTBIT_SPECIAL  (1ULL << 31)
#define AUDIO_USB_PCM_FMTBIT_S24_3LE  (1ULL << 32)
#define AUDIO_USB_PCM_FMTBIT_S24_3BE  (1ULL << 33)

#define AUDIO_USB_PCM_RATE_KNOT (1u << 31) /* supports more non-continuos rates */

#define AUDIO_USB_PCM_RATE_5512   (1 << 0)  /* 5512Hz */
#define AUDIO_USB_PCM_RATE_8000   (1 << 1)  /* 8000Hz */
#define AUDIO_USB_PCM_RATE_16000  (1 << 3)  /* 16000Hz */
#define AUDIO_USB_PCM_RATE_32000  (1 << 5)  /* 32000Hz */
#define AUDIO_USB_PCM_RATE_44100  (1 << 6)  /* 44100Hz */
#define AUDIO_USB_PCM_RATE_48000  (1 << 7)  /* 48000Hz */
#define AUDIO_USB_PCM_RATE_96000  (1 << 10) /* 96000Hz */
#define AUDIO_USB_PCM_RATE_192000 (1 << 12) /* 192000Hz */

#define AUDIO_USB_FORMAT_NUM_3 3
#define AUDIO_USB_FORMAT_NUM_4 4

#define FORMAT_QUIRK_ENABLE    1
#define FORMAT_QUIRK_DISENABLE 0

#define FORMAT_ALTSETTING_1 1
#define FORMAT_ALTSETTING_2 2
#define FORMAT_ALTSETTING_3 3
#define FORMAT_ALTSETTING_4 4
#define FORMAT_ALTSETTING_5 5
#define FORMAT_ALTSETTING_6 6
#define FORMAT_ALTSETTING_7 7
#define FORMAT_MAX_SIZE     64

#define ENDPOINT_NUM_2 2

#define DESP_LENTH         7
#define SAMPLE_BYTE_1      1
#define SAMPLE_BYTE_2      2
#define SAMPLE_BYTE_3      3
#define SAMPLE_BYTE_4      4
#define SAMPLE_RATE_2      2
#define SAMPLE_RATE_3      3
#define SAMPLE_RATE_OFFSET 3

#define PACK_SIZE   3
#define RATE_OFFSET 7

#define ARRAY_INDEX_0 0
#define ARRAY_INDEX_1 1
#define ARRAY_INDEX_2 2

#define MAX_PACKET_SIZE 392 /* USB sound card device descriptor default maxPackSize */

enum AudioUsbPcmStreamTpye {
    AUDIO_USB_PCM_STREAM_PLAYBACK = 0,
    AUDIO_USB_PCM_STREAM_CAPTURE,
    AUDIO_USB_PCM_STREAM_LAST = AUDIO_USB_PCM_STREAM_CAPTURE,
};

struct AudioFmtInfo {
    int32_t sampleWidth;
    int32_t sampleBytes;
    uint64_t pcmFormats;
};

struct AudioUsbUacFormat {
    int32_t altno;
    int32_t bmQuirk;
    int32_t clock;
    int32_t ifaceNo;
    int32_t num;
    int32_t protocol;
    int32_t stream;
    uint32_t channelsNum;
    uint32_t chconfig;
    uint64_t format;
    struct usb_interface_descriptor *altsd;
};

/* find an input terminal descriptor (either UAC1 or UAC2) with the given
 * terminal id
 */
static void *AudioUsbFindInputDesc(struct usb_host_interface *ctlIface, int32_t terminalId, int32_t protocol)
{
    struct uac2_input_terminal_descriptor *term = NULL;
    if (ctlIface == NULL) {
        AUDIO_DRIVER_LOG_ERR("ctlIface is NULL.");
        return NULL;
    }

    term = AudioUsbFindCsintDesc(ctlIface->extra, ctlIface->extralen, term, UAC_INPUT_TERMINAL);
    while (term != NULL) {
        if (!AudioUsbValidateAudioDesc(term, protocol)) {
            term = AudioUsbFindCsintDesc(ctlIface->extra, ctlIface->extralen, term, UAC_INPUT_TERMINAL);
            continue;
        }
        if (term->bTerminalID == terminalId) {
            return term;
        }
        term = AudioUsbFindCsintDesc(ctlIface->extra, ctlIface->extralen, term, UAC_INPUT_TERMINAL);
    }
    return NULL;
}

static int32_t ParseUacEndpointAttr(struct usb_host_interface *alts, int32_t protocol, int32_t *attributes)
{
    struct uac_iso_endpoint_descriptor *csep = NULL;
    struct usb_interface_descriptor *altsd = AudioUsbGetIfaceDesc(alts);

    csep = AudioUsbFindDesc(alts->endpoint[0].extra, alts->endpoint[0].extralen, NULL, USB_DT_CS_ENDPOINT);
    /* Creamware Noah has this descriptor after the 2nd endpoint */
    if (csep == NULL && altsd->bNumEndpoints >= ENDPOINT_NUM_2) {
        csep = AudioUsbFindDesc(alts->endpoint[1].extra, alts->endpoint[1].extralen, NULL, USB_DT_CS_ENDPOINT);
    }

    if (csep == NULL) {
        csep = AudioUsbFindDesc(alts->extra, alts->extralen, NULL, USB_DT_CS_ENDPOINT);
    }
    if (csep == NULL || csep->bLength < DESP_LENTH || csep->bDescriptorSubtype != UAC_EP_GENERAL) {
        AUDIO_DRIVER_LOG_ERR("uac iso endpoint descriptor is invalid.");
        return HDF_FAILURE;
    }

    if (protocol == UAC_VERSION_1) {
        *attributes = csep->bmAttributes;
    } else if (protocol == UAC_VERSION_2) {
        struct uac2_iso_endpoint_descriptor *csep2 = (struct uac2_iso_endpoint_descriptor *)csep;
        if (csep2->bLength < sizeof(*csep2)) {
            AUDIO_DRIVER_LOG_ERR("uac2 iso endpoint descriptor length is invalid.");
            return HDF_FAILURE;
        }
        *attributes = csep->bmAttributes & UAC_EP_CS_ATTR_FILL_MAX;
        /* emulate the endpoint attributes of a v1 device */
        if (csep2->bmControls & UAC2_CONTROL_PITCH) {
            *attributes |= UAC_EP_CS_ATTR_PITCH_CONTROL;
        }
    } else { /* UAC_VERSION_3 */
        struct uac3_iso_endpoint_descriptor *csep3 = (struct uac3_iso_endpoint_descriptor *)csep;
        if (csep3->bLength < sizeof(*csep3)) {
            AUDIO_DRIVER_LOG_ERR("uac3 iso endpoint descriptor length is invalid.");
            return HDF_FAILURE;
        }
        /* emulate the endpoint attributes of a v1 device */
        if (le32_to_cpu(csep3->bmControls) & UAC2_CONTROL_PITCH) {
            *attributes |= UAC_EP_CS_ATTR_PITCH_CONTROL;
        }
    }

    return HDF_SUCCESS;
}

/* check if the device uses big-endian samples */
static bool AudioUsbCheckFormat(struct AudioUsbDriver *audioUsbDriver, struct AudioUsbFormat *audioUsbFormat)
{
    /* it depends on altsetting whether the device is big-endian or not */
    switch (audioUsbDriver->usbId) {
        case 0x7632001: /* 0x0763 0x2001 M-Audio Quattro: captured data only */
            if (audioUsbFormat->altsetting == FORMAT_ALTSETTING_2 ||
                audioUsbFormat->altsetting == FORMAT_ALTSETTING_3 ||
                audioUsbFormat->altsetting == FORMAT_ALTSETTING_5 ||
                audioUsbFormat->altsetting == FORMAT_ALTSETTING_6) {
                return true;
            }
            break;
        case 0x7632003: /* 0x0763 0x2003 M-Audio Audiophile USB */
            if (audioUsbDriver->setup == 0x00 || audioUsbFormat->altsetting == 1 ||
                audioUsbFormat->altsetting == FORMAT_ALTSETTING_2 ||
                audioUsbFormat->altsetting == FORMAT_ALTSETTING_3) {
                return true;
            }
            break;
        case 0x7632012: /* 0x0763 0x2012 M-Audio Fast Track Pro */
            if (audioUsbFormat->altsetting == FORMAT_ALTSETTING_2 ||
                audioUsbFormat->altsetting == FORMAT_ALTSETTING_3 ||
                audioUsbFormat->altsetting == FORMAT_ALTSETTING_5 ||
                audioUsbFormat->altsetting == FORMAT_ALTSETTING_6) {
                return true;
            }
            break;
        default:
            AUDIO_DRIVER_LOG_WARNING("usb device not support.");
            return false;
    }
    return false;
}

static int32_t AudioUsbFormatTypeInit(
    struct AudioUsbFormat *audioUsbFormat, struct AudioFmtInfo *audioFmtInfo, void *pFmt, uint64_t *format)
{
    switch (audioUsbFormat->protocol) {
        case UAC_VERSION_1: {
            struct uac_format_type_i_discrete_descriptor *fmt = pFmt;
            if (*format >= FORMAT_MAX_SIZE) {
                return HDF_ERR_INVALID_PARAM; /* invalid format */
            }
            audioFmtInfo->sampleWidth = fmt->bBitResolution;
            audioFmtInfo->sampleBytes = fmt->bSubframeSize;
            *format = 1ULL << *format;
            break;
        }
        case UAC_VERSION_2: {
            struct uac_format_type_i_ext_descriptor *fmt = pFmt;
            audioFmtInfo->sampleWidth = fmt->bBitResolution;
            audioFmtInfo->sampleBytes = fmt->bSubslotSize;
            if (*format & UAC2_FORMAT_TYPE_I_RAW_DATA) {
                audioFmtInfo->pcmFormats |= AUDIO_USB_PCM_FMTBIT_SPECIAL;
                /* flag potentially raw DSD capable altsettings */
                audioUsbFormat->dsdRaw = true;
            }
            *format <<= 1;
            break;
        }
        case UAC_VERSION_3: {
            struct uac3_as_header_descriptor *as = pFmt;
            audioFmtInfo->sampleWidth = as->bBitResolution;
            audioFmtInfo->sampleBytes = as->bSubslotSize;
            if (*format & UAC3_FORMAT_TYPE_I_RAW_DATA) {
                audioFmtInfo->pcmFormats |= AUDIO_USB_PCM_FMTBIT_SPECIAL;
            }
            *format <<= 1;
            break;
        }
        default: {
            struct uac_format_type_i_discrete_descriptor *fmt = pFmt;
            if (*format >= FORMAT_MAX_SIZE) {
                return HDF_ERR_INVALID_PARAM; /* invalid format */
            }
            audioFmtInfo->sampleWidth = fmt->bBitResolution;
            audioFmtInfo->sampleBytes = fmt->bSubframeSize;
            *format = 1ULL << *format;
            break;
        }
    }

    return HDF_SUCCESS;
}

static void AudioUsbPcmFormatSub(
    struct AudioUsbDriver *audioUsbDriver, struct AudioUsbFormat *audioUsbFmt, struct AudioFmtInfo *audioFmtInfo)
{
    if (((audioUsbDriver->usbId == AudioUsbGetUsbId(0x0582, 0x0016)) ||     /* Edirol SD-90 */
            (audioUsbDriver->usbId == AudioUsbGetUsbId(0x0582, 0x000c))) && /* Roland SC-D70 */
        audioFmtInfo->sampleWidth == SAMPLE_WIDTH &&
        audioFmtInfo->sampleBytes == SAMPLE_BYTE_2) {
        audioFmtInfo->sampleBytes = SAMPLE_BYTE_3;
    } else if (audioFmtInfo->sampleWidth > (audioFmtInfo->sampleBytes) * 8) { /* 8 for Byte to bits */
        AUDIO_DRIVER_LOG_INFO("%u:%d : sample bitwidth %d in over sample bytes %d.", audioUsbFmt->iface,
            audioUsbFmt->altsetting, audioFmtInfo->sampleWidth, audioFmtInfo->sampleBytes);
    }
    /* check the format byte size */
    switch (audioFmtInfo->sampleBytes) {
        case SAMPLE_BYTE_1:
            audioFmtInfo->pcmFormats |= AUDIO_USB_PCM_FMTBIT_S8;
            break;
        case SAMPLE_BYTE_2:
            if (AudioUsbCheckFormat(audioUsbDriver, audioUsbFmt)) {
                audioFmtInfo->pcmFormats |= AUDIO_USB_PCM_FMTBIT_S16_BE; /* grrr, big endian!! */
            } else {
                audioFmtInfo->pcmFormats |= AUDIO_USB_PCM_FMTBIT_S16_LE;
            }
            break;
        case SAMPLE_BYTE_3:
            if (AudioUsbCheckFormat(audioUsbDriver, audioUsbFmt)) {
                audioFmtInfo->pcmFormats |= AUDIO_USB_PCM_FMTBIT_S24_3BE; /* grrr, big endian!! */
            } else {
                audioFmtInfo->pcmFormats |= AUDIO_USB_PCM_FMTBIT_S24_3LE;
            }
            break;
        case SAMPLE_BYTE_4:
            audioFmtInfo->pcmFormats |= AUDIO_USB_PCM_FMTBIT_S32_LE;
            break;
        default:
            AUDIO_DRIVER_LOG_INFO("%u:%d : unsupported sample bitwidth %d in %d bytes.", audioUsbFmt->iface,
                audioUsbFmt->altsetting, audioFmtInfo->sampleWidth, audioFmtInfo->sampleBytes);
            break;
    }
}

static uint64_t ParseAudioFormatType(
    struct AudioUsbDriver *audioUsbDriver, struct AudioUsbFormat *audioUsbFormat, uint64_t format, void *pFmt)
{
    uint64_t pcmFormats = 0;

    struct AudioFmtInfo audioFmtInfo;
    (void)memset_s(&audioFmtInfo, sizeof(struct AudioFmtInfo), 0, sizeof(struct AudioFmtInfo));
    int32_t ret = AudioUsbFormatTypeInit(audioUsbFormat, &audioFmtInfo, pFmt, &format);
    if (ret == HDF_ERR_INVALID_PARAM) {
        return 0; /* invalid format, return 0 */
    }
    audioUsbFormat->fmtBits = audioFmtInfo.sampleWidth;
    pcmFormats = audioFmtInfo.pcmFormats;
    if ((pcmFormats == 0) && (format == 0 || format == (1 << UAC_FORMAT_TYPE_I_UNDEFINED))) {
        AUDIO_DRIVER_LOG_INFO(
            "%u:%d : format type 0 is detected, processed as PCM.", audioUsbFormat->iface, audioUsbFormat->altsetting);
        format = 1 << UAC_FORMAT_TYPE_I_PCM;
    }
    if (format & (1 << UAC_FORMAT_TYPE_I_PCM)) { /* format type is pcm */
        AudioUsbPcmFormatSub(audioUsbDriver, audioUsbFormat, &audioFmtInfo);
        pcmFormats = audioFmtInfo.pcmFormats;
    }
    if (format & (1 << UAC_FORMAT_TYPE_I_PCM8)) {                        /* format type is pcm8 */
        if (audioUsbDriver->usbId == AudioUsbGetUsbId(0x04fa, 0x4201)) { /* Dallas DS4201 workaround */
            pcmFormats = pcmFormats | AUDIO_USB_PCM_FMTBIT_S8;
        } else {
            pcmFormats = pcmFormats | AUDIO_USB_PCM_FMTBIT_U8;
        }
    }
    if (format & (1 << UAC_FORMAT_TYPE_I_ALAW)) { /* format type is alaw */
        pcmFormats = pcmFormats | AUDIO_USB_PCM_FMTBIT_A_LAW;
    }

    if (format & (1 << UAC_FORMAT_TYPE_I_IEEE_FLOAT)) { /* format type is ieee float */
        pcmFormats = pcmFormats | AUDIO_USB_PCM_FMTBIT_FLOAT_LE;
    }

    if (format & (1 << UAC_FORMAT_TYPE_I_MULAW)) { /* format type is mulaw */
        pcmFormats = pcmFormats | AUDIO_USB_PCM_FMTBIT_MU_LAW;
    }

    return pcmFormats;
}

static uint32_t AudioPcmRateToRateBit(uint32_t rate)
{
    uint32_t i;
    uint32_t count;
    uint32_t rateMask;
    static const uint32_t rates[] = {5512, 8000, 11025, 16000, 22050, 32000, 44100, 48000, 64000, 88200, 96000, 176400,
        192000, 352800, 384000}; /* Pcm sampling rate */

    count = sizeof(rates) / sizeof(rates[0]);
    for (i = 0; i < count; i++) {
        if (rates[i] == rate) {
            rateMask = MOVE_VAL << i;
            return rateMask;
        }
    }
    return AUDIO_USB_PCM_RATE_KNOT;
}

static void AudioUsbInitRate(
    struct AudioUsbDriver *audioUsbDriver, struct AudioUsbFormat *audioUsbFormat, uint32_t *rate, int32_t nrRates)
{
    if (*rate == AUDIO_SAMPLE_RATE_48000 && nrRates == 1 &&
        (audioUsbDriver->usbId == AudioUsbGetUsbId(0x0d8c, 0x0201) ||     /* CM6501 */
            audioUsbDriver->usbId == AudioUsbGetUsbId(0x0d8c, 0x0102) ||  /* CM6501 */
            audioUsbDriver->usbId == AudioUsbGetUsbId(0x0d8c, 0x0078) ||  /* CM6206 */
            audioUsbDriver->usbId == AudioUsbGetUsbId(0x0ccd, 0x00b1)) && /* Ozone Z90 */
        audioUsbFormat->altsetting == FORMAT_ALTSETTING_5 &&
        audioUsbFormat->maxPackSize == MAX_PACKET_SIZE) {
        *rate = AUDIO_SAMPLE_RATE_96000;
    }
    /* Creative VF0420/VF0470 Live Cams report 16 kHz instead of 8kHz */
    if (*rate == AUDIO_SAMPLE_RATE_16000 &&
        (audioUsbDriver->usbId == AudioUsbGetUsbId(0x041e, 0x4064) ||     /* VF0420 */
            audioUsbDriver->usbId == AudioUsbGetUsbId(0x041e, 0x4068))) { /* VF0470 */
        *rate = AUDIO_SAMPLE_RATE_8000;
    }
}

static int32_t AudioUsbParseFormatRates(
    struct AudioUsbDriver *audioUsbDriver, struct AudioUsbFormat *audioUsbFormat, uint8_t *fmt, int32_t offset)
{
    int32_t nrRates = fmt[offset];
    int32_t r, idx;
    int32_t temp;

    if (nrRates == 0) {
        nrRates = SAMPLE_RATE_2;
    }
    temp = offset + 1 + SAMPLE_RATE_3 * nrRates;
    if (fmt[0] < temp) {
        AUDIO_DRIVER_LOG_ERR("%u:%d : invalid FORMAT_TYPE desc", audioUsbFormat->iface, audioUsbFormat->altsetting);
        return HDF_ERR_INVALID_PARAM;
    }

    if (nrRates == INVALID_RATE) {
        return HDF_SUCCESS;
    }

    /*
     * build the rate table and bitmap flags
     */
    audioUsbFormat->rateTable = OsalMemCalloc(nrRates * sizeof(int32_t));
    if (audioUsbFormat->rateTable == NULL) {
        return HDF_ERR_INVALID_OBJECT;
    }
    audioUsbFormat->nrRates = 0;
    audioUsbFormat->rateMin = audioUsbFormat->rateMax = 0;
    for (r = 0, idx = offset + 1; r < nrRates; r++, idx += SAMPLE_RATE_OFFSET) {
        uint32_t rate = AudioUsbCombineTriple(&fmt[idx]);
        if (rate == 0) {
            continue;
        }
        /* C-Media CM6501 mislabels its 96 kHz altsetting */
        /* Terratec Aureon 7.1 USB C-Media 6206, too */
        /* Ozone Z90 USB C-Media, too */
        AudioUsbInitRate(audioUsbDriver, audioUsbFormat, &rate, nrRates);

        audioUsbFormat->rateTable[audioUsbFormat->nrRates] = rate;
        if (audioUsbFormat->rateMin == 0 || rate < audioUsbFormat->rateMin) {
            audioUsbFormat->rateMin = rate;
        }
        if (audioUsbFormat->rateMax == 0 || rate > audioUsbFormat->rateMax) {
            audioUsbFormat->rateMax = rate;
        }
        audioUsbFormat->rates |= AudioPcmRateToRateBit(rate);
        audioUsbFormat->nrRates++;
    }
    if (audioUsbFormat->nrRates == 0) {
        return HDF_ERR_INVALID_PARAM;
    }

    return HDF_SUCCESS;
}

/* parse the format type I and III descriptors */
static int32_t AudioUsbParseFormatSub(
    struct AudioUsbDriver *audioUsbDriver, struct AudioUsbFormat *audioUsbFormat, uint64_t format, void *pFmt)
{
    uint32_t fmtType;
    int32_t ret;
    struct uac_format_type_i_continuous_descriptor *fmt = NULL;

    switch (audioUsbFormat->protocol) {
        case UAC_VERSION_1:
        case UAC_VERSION_2: {
            fmt = pFmt;
            fmtType = fmt->bFormatType;
            break;
        }
        case UAC_VERSION_3: {
            /* fp->fmtType is already set in this case */
            fmtType = audioUsbFormat->fmtType;
            break;
        }
        default: {
            fmt = pFmt;
            fmtType = fmt->bFormatType;
            break;
        }
    }

    if (fmtType != UAC_FORMAT_TYPE_III) {
        audioUsbFormat->formats = ParseAudioFormatType(audioUsbDriver, audioUsbFormat, format, pFmt);
        if (audioUsbFormat->formats == 0) {
            return HDF_ERR_INVALID_PARAM;
        }
    }

    if (audioUsbFormat->protocol == UAC_VERSION_1) {
        fmt = pFmt;
        audioUsbFormat->channels = fmt->bNrChannels;
        ret = AudioUsbParseFormatRates(audioUsbDriver, audioUsbFormat, (uint8_t *)fmt, RATE_OFFSET);
    } else {
        return HDF_ERR_NOT_SUPPORT;
    }

    if (audioUsbFormat->channels == 0) {
        AUDIO_DRIVER_LOG_ERR("%u:%d : invalid channels %d.", audioUsbFormat->iface, audioUsbFormat->altsetting,
            audioUsbFormat->channels);
        return HDF_ERR_INVALID_PARAM;
    }

    return ret;
}

static int32_t AudioUsbParseFormat(struct AudioUsbDriver *audioUsbDriver, struct AudioUsbFormat *audioUsbFormat,
    uint64_t format, struct uac_format_type_i_continuous_descriptor *fmt, int32_t stream)
{
    int32_t ret = 0;

    switch (fmt->bFormatType) {
        case UAC_FORMAT_TYPE_I:
        case UAC_FORMAT_TYPE_III:
            ret = AudioUsbParseFormatSub(audioUsbDriver, audioUsbFormat, format, fmt);
            break;
        case UAC_FORMAT_TYPE_II:
            break;
        default:
            AUDIO_DRIVER_LOG_ERR("%u: %d : format type %d is not supported yet.", audioUsbFormat->iface,
                audioUsbFormat->altsetting, fmt->bFormatType);
            return HDF_ERR_NOT_SUPPORT;
    }
    audioUsbFormat->fmtType = fmt->bFormatType;
    if (ret < HDF_SUCCESS) {
        return ret;
    }

    if (audioUsbDriver->usbId == AudioUsbGetUsbId(0x041e, 0x3000) || /* Extigy */
        audioUsbDriver->usbId == AudioUsbGetUsbId(0x041e, 0x3020) || /* Audigy 2 NX */
        audioUsbDriver->usbId == AudioUsbGetUsbId(0x041e, 0x3061)) { /* Audigy */
        if (fmt->bFormatType == UAC_FORMAT_TYPE_I && audioUsbFormat->rates != AUDIO_USB_PCM_RATE_48000 &&
            audioUsbFormat->rates != AUDIO_USB_PCM_RATE_96000) {
            AUDIO_DRIVER_LOG_ERR("audio usb format is not support");
            return HDF_ERR_NOT_SUPPORT;
        }
    }

    return HDF_SUCCESS;
}

static void AudioUsbFreeFormat(struct AudioUsbFormat *fp)
{
    if (fp != NULL) {
        DListRemove(&fp->list);
        OsalMemFree(fp->rateTable);
        OsalMemFree(fp);
    }
}

static struct AudioUsbFormat *AudioUsbFormatInit(struct AudioUsbDriver *audioUsbDriver,
    struct AudioUsbUacFormat *uacFmt, struct usb_host_interface *alts, int32_t altsetIdx)
{
    struct AudioUsbFormat *fp = NULL;
    struct usb_device *dev = NULL;
    struct usb_endpoint_descriptor *epDesc = NULL;

    fp = OsalMemCalloc(sizeof(struct AudioUsbFormat));
    if (fp == NULL) {
        AUDIO_DRIVER_LOG_ERR("OsalMemCalloc failed.");
        return NULL;
    }
    dev = interface_to_usbdev(audioUsbDriver->usbIf);
    if (dev == NULL) {
        AUDIO_DRIVER_LOG_ERR("interface_to_usbdev failed.");
        AudioUsbFreeFormat(fp);
        return NULL;
    }
    epDesc = AudioEndpointDescriptor(alts, 0);
    if (epDesc == NULL) {
        AUDIO_DRIVER_LOG_ERR("epDesc is NULL.");
        AudioUsbFreeFormat(fp);
        return NULL;
    }
    fp->iface = uacFmt->ifaceNo;
    fp->altsetting = uacFmt->altno;
    fp->altsetIdx = altsetIdx;
    fp->endpoint = epDesc->bEndpointAddress;
    fp->epAttr = epDesc->bmAttributes;
    fp->dataInterval = AudioUsbParseDataInterval(dev, alts);
    fp->protocol = uacFmt->protocol;
    fp->maxPackSize = le16_to_cpu(epDesc->wMaxPacketSize);

    fp->channels = uacFmt->channelsNum;

    if (AudioUsbGetSpeed(dev) == USB_SPEED_HIGH) {
        fp->maxPackSize = (((fp->maxPackSize >> MOVE_11_BIT) & PACK_SIZE) + 1) * (fp->maxPackSize & 0x7ff);
    }

    fp->clock = uacFmt->clock;
    DListHeadInit(&fp->list);

    return fp;
}

static int32_t AudioUsbGetFormat(struct AudioUsbDriver *audioUsbDriver, struct usb_host_interface *alts,
    struct AudioUsbUacFormat *uacFmt, struct uac_format_type_i_continuous_descriptor **fmt)
{
    /* get audio formats */
    if (uacFmt->protocol == UAC_VERSION_1) {
        struct uac1_as_header_descriptor *as = AudioUsbFindCsintDesc(alts->extra, alts->extralen, NULL, UAC_AS_GENERAL);
        struct uac_input_terminal_descriptor *iterm;

        if (as == NULL) {
            AUDIO_DRIVER_LOG_ERR("UAC_AS_GENERAL descriptor not found.");
            return HDF_FAILURE;
        }
        if (as->bLength < sizeof(*as)) {
            AUDIO_DRIVER_LOG_ERR("invalid UAC_AS_GENERAL desc.");
            return HDF_FAILURE;
        }
        uacFmt->format = le16_to_cpu(as->wFormatTag); /* remember the format value */
        iterm = AudioUsbFindInputDesc(audioUsbDriver->ctrlIntf, as->bTerminalLink, uacFmt->protocol);
        if (iterm != NULL) {
            uacFmt->channelsNum = iterm->bNrChannels;
            uacFmt->chconfig = le16_to_cpu(iterm->wChannelConfig);
        }
    } else { /* UAC_VERSION_2 */
        AUDIO_DRIVER_LOG_ERR("Version type not currently supported.");
        return HDF_ERR_NOT_SUPPORT;
    }

    /* get format type */
    *fmt = AudioUsbFindCsintDesc(alts->extra, alts->extralen, NULL, UAC_FORMAT_TYPE);
    if (*fmt == NULL) {
        AUDIO_DRIVER_LOG_ERR("%u:%d : no UAC_FORMAT_TYPE desc.", uacFmt->ifaceNo, uacFmt->altno);
        return HDF_FAILURE;
    }
    if (((uacFmt->protocol == UAC_VERSION_1) && ((*fmt)->bLength < FORMAT_LRENGTH_8)) ||
        ((uacFmt->protocol == UAC_VERSION_2) && ((*fmt)->bLength < FORMAT_LRENGTH_6))) {
        AUDIO_DRIVER_LOG_ERR("%u:%d: invalid UAC_FORMAT_TYPE desc.", uacFmt->ifaceNo, uacFmt->altno);
        return HDF_FAILURE;
    }
    audioUsbDriver->fmtType = (*fmt)->bFormatType;
    if (uacFmt->bmQuirk && (*fmt)->bNrChannels == 1 && (*fmt)->bSubframeSize == FRAME_SIZE_2) {
        return HDF_FAILURE;
    }

    return HDF_SUCCESS;
}

static struct AudioUsbFormat *AudioUsbUac12GetFormat(struct AudioUsbDriver *audioUsbDriver,
    struct usb_host_interface *alts, struct AudioUsbUacFormat *uacFmt, int32_t altsetIdx)
{
    struct AudioUsbFormat *audioUsbFormat = NULL;
    struct usb_device *dev = NULL;
    struct uac_format_type_i_continuous_descriptor *fmt = NULL;

    if (audioUsbDriver == NULL || alts == NULL || audioUsbDriver->usbIf == NULL) {
        AUDIO_DRIVER_LOG_ERR("input para is NULL.");
        return NULL;
    }
    dev = interface_to_usbdev(audioUsbDriver->usbIf);
    if (dev == NULL) {
        AUDIO_DRIVER_LOG_ERR("dev is NULL.");
        return NULL;
    }

    if (AudioUsbGetFormat(audioUsbDriver, alts, uacFmt, &fmt) != HDF_SUCCESS) {
        AUDIO_DRIVER_LOG_ERR("AudioUsbGetFormat failed.");
        return NULL;
    }
    audioUsbFormat = AudioUsbFormatInit(audioUsbDriver, uacFmt, alts, altsetIdx);
    if (audioUsbFormat == NULL) {
        AUDIO_DRIVER_LOG_ERR("audioUsbFormat is NULL.");
        return NULL;
    }
    (void)ParseUacEndpointAttr(alts, uacFmt->protocol, &audioUsbFormat->attributes);
    /* ok, let's parse further... */
    if (AudioUsbParseFormat(audioUsbDriver, audioUsbFormat, uacFmt->format, fmt, uacFmt->stream) != HDF_SUCCESS) {
        AudioUsbFreeFormat(audioUsbFormat);
        return NULL;
    }

    if (audioUsbFormat->channels != uacFmt->channelsNum) {
        uacFmt->chconfig = 0; /* 0 is invalid value */
    }
    return audioUsbFormat;
}

static bool AudioUsbGetSampleRateQuirk(struct AudioUsbDriver *audioUsbDriver)
{
    switch (audioUsbDriver->usbId) {
        case 0x41E4080:  /* (0x041e, 0x4080) Creative Live Cam VF0610 */
        case 0x4D8FEEE:  /* (0x04d8, 0xfeee) Benchmark DAC1 Pre */
        case 0x5560014:  /* (0x0556, 0x0014) Phoenix Audio TMX320VC */
        case 0x5A71020:  /* (0x05a7, 0x1020) Bose Companion 5 */
        case 0x74D3553:  /* (0x074d, 0x3553) Outlaw RR2150 (Micronas UAC3553B) */
        case 0x1395740a: /* (0x1395, 0x740a) Sennheiser DECT */
        case 0x19010191: /* (0x1901, 0x0191) GE B850V3 CP2114 audio interface */
        case 0x21b40081: /* (0x21b4, 0x0081) AudioQuest DragonFly */
        case 0x291230c8: /* (0x2912, 0x30c8) Audioengine D1 */
        case 0x413ca506: /* (0x413c, 0xa506) Dell AE515 sound bar */
        case 0x046d084c: /* (0x046d, 0x084c) Logitech ConferenceCam Connect */
            return true;
        default:
            break;
    }

    /* devices of these vendors don't support reading rate, either */
    switch (audioUsbDriver->usbId >> MOVE_16_BIT) {
        case 0x045e: /* MS Lifecam */
        case 0x047f: /* Plantronics */
        case 0x1de7: /* Phoenix Audio */
            return true;
        default:
            return false;
    }

    return false;
}

static int32_t AudioUsbV1SetSampleRateSub(
    struct AudioUsbDriver *audioUsbDriver, struct usb_device *dev, uint32_t ep, uint8_t *data, uint32_t lenth)
{
    struct AudioUsbCtlMsgParam usbCtlMsgParam;
    int32_t ret;

    usbCtlMsgParam.pipe = usb_sndctrlpipe(dev, 0);
    usbCtlMsgParam.request = UAC_SET_CUR;
    usbCtlMsgParam.requestType = USB_TYPE_CLASS | USB_RECIP_ENDPOINT | USB_DIR_OUT;
    usbCtlMsgParam.value = UAC_EP_CS_ATTR_SAMPLE_RATE << MOVE_8_BIT;
    usbCtlMsgParam.index = ep;

    ret = AudioUsbCtlMsg(dev, &usbCtlMsgParam, data, lenth);
    if (ret < HDF_SUCCESS) {
        AUDIO_DRIVER_LOG_ERR("cannot set freq at ep %#x\n", ep);
        return ret;
    }

    /* Don't check the sample rate for devices which we know don't
     * support reading */
    if (AudioUsbGetSampleRateQuirk(audioUsbDriver)) {
        return HDF_SUCCESS;
    }
    /* the firmware is likely buggy, don't repeat to fail too many times */
    if (audioUsbDriver->sampleRateReadError > READ_MAX_ERR_COUNT) {
        return HDF_SUCCESS;
    }

    usbCtlMsgParam.pipe = usb_rcvctrlpipe(dev, 0);
    usbCtlMsgParam.request = UAC_GET_CUR;
    usbCtlMsgParam.requestType = USB_TYPE_CLASS | USB_RECIP_ENDPOINT | USB_DIR_IN;
    usbCtlMsgParam.value = UAC_EP_CS_ATTR_SAMPLE_RATE << MOVE_8_BIT;

    ret = AudioUsbCtlMsg(dev, &usbCtlMsgParam, data, lenth);
    if (ret < HDF_SUCCESS) {
        AUDIO_DRIVER_LOG_ERR("cannot get freq at ep %#x.", ep);
        audioUsbDriver->sampleRateReadError++;
        return HDF_SUCCESS; /* some devices don't support reading */
    }
    return HDF_SUCCESS;
}

static int32_t AudioUsbV1SetSampleRate(struct AudioUsbDriver *audioUsbDriver, int32_t iface,
    struct usb_host_interface *alts, struct AudioUsbFormat *audioUsbFormat, int32_t rate)
{
    struct usb_device *dev = NULL;
    uint32_t ep;
    uint8_t data[SAMPLE_RATE_3];
    int32_t ret, currentRate;
    struct usb_interface_descriptor *inteDesc = NULL;
    struct usb_endpoint_descriptor *epDesc = NULL;

    dev = interface_to_usbdev(audioUsbDriver->usbIf);

    inteDesc = AudioUsbGetIfaceDesc(alts);
    if (inteDesc->bNumEndpoints < 1) {
        return HDF_ERR_INVALID_PARAM;
    }
    epDesc = AudioEndpointDescriptor(alts, 0);
    if (epDesc == NULL) {
        AUDIO_DRIVER_LOG_ERR("epDesc is NULL.");
        return HDF_FAILURE;
    }
    ep = epDesc->bEndpointAddress;

    /* if endpoint doesn't have sampling rate control, bail out */
    if ((audioUsbFormat->attributes & UAC_EP_CS_ATTR_SAMPLE_RATE) == 0) {
        return HDF_SUCCESS;
    }
    data[ARRAY_INDEX_0] = rate;
    data[ARRAY_INDEX_1] = rate >> MOVE_8_BIT;
    data[ARRAY_INDEX_2] = rate >> MOVE_16_BIT;

    ret = AudioUsbV1SetSampleRateSub(audioUsbDriver, dev, ep, data, sizeof(data));
    if (ret != HDF_SUCCESS) {
        AUDIO_DRIVER_LOG_ERR("%d:%d: cannot set freq %d to ep %#x\n", iface, audioUsbFormat->altsetting, rate, ep);
        return ret;
    }

    // The rate value is composed of data [0], data [1] and data [2].
    currentRate = data[0] | (data[1] << MOVE_8_BIT) | (data[FORMAT_ALTSETTING_2] << MOVE_16_BIT);
    if (currentRate == 0) {
        AUDIO_DRIVER_LOG_INFO("failed to read current rate; disabling the check.");
        audioUsbDriver->sampleRateReadError = READ_RATE_ERR_COUNT; /* three strikes, see above */
        return HDF_SUCCESS;
    }

    if (currentRate != rate) {
        AUDIO_DRIVER_LOG_WARNING("current rate %d is different from the runtime rate %d.", currentRate, rate);
    }

    return HDF_SUCCESS;
}

int32_t AudioUsbInitSampleRate(struct AudioUsbDriver *audioUsbDriver, int32_t iface, struct usb_host_interface *alts,
    struct AudioUsbFormat *audioUsbFormat, int32_t rate)
{
    if (audioUsbFormat->protocol == UAC_VERSION_1) {
        return AudioUsbV1SetSampleRate(audioUsbDriver, iface, alts, audioUsbFormat, rate);
    } else {
        return HDF_ERR_NOT_SUPPORT;
    }
}

static int32_t AudioUsbGetAltsd(struct usb_host_interface **alts, struct usb_interface *iface,
    struct AudioUsbUacFormat *uacFmt, int32_t altsetIdx, bool *dataFlag)
{
    struct usb_endpoint_descriptor *epDesc = NULL;
    bool audioInterface = false;
    bool vendorInterface = false;
    bool specInterface = false;
    bool invalidEndpoint = false;
    bool invalidPacket = false;

    *alts = &iface->altsetting[altsetIdx];
    if (*alts == NULL) {
        AUDIO_DRIVER_LOG_ERR("alts is NULL.");
        return HDF_FAILURE;
    }
    uacFmt->altsd = AudioUsbGetIfaceDesc(*alts);
    if (uacFmt->altsd == NULL) {
        AUDIO_DRIVER_LOG_ERR("altsd is NULL.");
        return HDF_FAILURE;
    }
    uacFmt->protocol = uacFmt->altsd->bInterfaceProtocol;
    epDesc = AudioEndpointDescriptor(*alts, 0);
    if (epDesc != NULL) {
        if (le16_to_cpu(epDesc->wMaxPacketSize) == 0) {
            invalidPacket = true;
        }
    }
    if (uacFmt->altsd->bInterfaceClass != USB_CLASS_AUDIO) {
        audioInterface = true;
    }
    if (uacFmt->altsd->bInterfaceSubClass != USB_SUBCLASS_AUDIOSTREAMING &&
        uacFmt->altsd->bInterfaceSubClass != USB_SUBCLASS_VENDOR_SPEC) {
        vendorInterface = true;
    }
    if (uacFmt->altsd->bInterfaceClass != USB_CLASS_VENDOR_SPEC) {
        specInterface = true;
    }
    if (uacFmt->altsd->bNumEndpoints < 1) {
        invalidEndpoint = true;
    }

    /* skip invalid one */
    if (((audioInterface || vendorInterface) && specInterface) || invalidEndpoint || invalidPacket) {
        *dataFlag = false; /* invalid data flag */
    }
    return HDF_SUCCESS;
}

static void AudioUsbInitFormatAndRate(struct AudioUsbDriver *audioUsbDriver, struct AudioUsbFormat *audioUsbFormat,
    struct AudioUsbUacFormat *uacFmt, struct usb_host_interface *alts, struct usb_device *dev)
{
    if (uacFmt->stream == AUDIO_USB_PCM_STREAM_PLAYBACK) {
        DListInsertHead(&audioUsbFormat->list, &audioUsbDriver->renderUsbFormatList);
    } else {
        DListInsertHead(&audioUsbFormat->list, &audioUsbDriver->captureUsbFormatList);
    }
    /* try to set the interface... */
    usb_set_interface(dev, uacFmt->ifaceNo, uacFmt->altno);
    (void)AudioUsbInitSampleRate(audioUsbDriver, uacFmt->ifaceNo, alts, audioUsbFormat, audioUsbFormat->rateMax);
}

static void AudioUsbGetFormatSub(struct AudioUsbDriver *audioUsbDriver, struct usb_host_interface *alts,
    struct AudioUsbUacFormat *uacFmt, int32_t altsetIdx, struct AudioUsbFormat **audioUsbFormat)
{
    struct usb_endpoint_descriptor *epDesc = NULL;
    bool uacFmtbm = false;
    bool usbFmtbm = false;
    bool maxPackSizebm = false;

    epDesc = AudioEndpointDescriptor(alts, 0);
    if (epDesc == NULL) {
        AUDIO_DRIVER_LOG_ERR("epDesc is NULL.");
        return;
    }
    /* check direction */
    if (epDesc->bEndpointAddress & USB_DIR_IN) {
        uacFmt->stream = AUDIO_USB_PCM_STREAM_CAPTURE;
    } else {
        uacFmt->stream = AUDIO_USB_PCM_STREAM_PLAYBACK;
    }
    uacFmt->altno = uacFmt->altsd->bAlternateSetting;
    if ((audioUsbDriver->usbId >> MOVE_16_BIT) == 0x0582 && /* Edirol */
        uacFmt->altsd->bInterfaceClass == USB_CLASS_VENDOR_SPEC && uacFmt->protocol <= UAC_VERSION_3) {
        uacFmt->protocol = UAC_VERSION_1;
    }

    if (uacFmt->protocol == UAC_VERSION_3) {
        // UAC_VERSION_3 is not support
        return;
    }
    if (uacFmt->protocol != UAC_VERSION_2) {
        AUDIO_DRIVER_LOG_INFO(
            "%u:%d: unknown interface protocol %#02x, assuming v1.", uacFmt->ifaceNo, uacFmt->altno, uacFmt->protocol);
        uacFmt->protocol = UAC_VERSION_1;
    }

    uacFmt->bmQuirk = FORMAT_QUIRK_DISENABLE;
    if (uacFmt->altno == UAC_FORMAT_TYPE_II && uacFmt->num == AUDIO_USB_FORMAT_NUM_3) {
        uacFmtbm = true;
    }

    if (*audioUsbFormat != NULL) {
        if ((*audioUsbFormat)->altsetting == FORMAT_ALTSETTING_1 && (*audioUsbFormat)->channels == 1 &&
            (*audioUsbFormat)->formats == AUDIO_USB_PCM_FMTBIT_S16_LE) {
            usbFmtbm = true;
        }
        if (le16_to_cpu(epDesc->wMaxPacketSize) == (*audioUsbFormat)->maxPackSize * FRAME_SIZE_2) {
            maxPackSizebm = true;
        }
    }

    if (uacFmtbm && usbFmtbm && uacFmt->protocol == UAC_VERSION_1 && maxPackSizebm) {
        uacFmt->bmQuirk = FORMAT_QUIRK_ENABLE;
    }
    *audioUsbFormat = AudioUsbUac12GetFormat(audioUsbDriver, alts, uacFmt, altsetIdx);
    return;
}

static int32_t AudioUsbSetFormat(struct AudioUsbDriver *audioUsbDriver, struct AudioUsbUacFormat uacFmt,
    struct usb_interface *iface, bool *hasNonPcm, bool nonPcm)
{
    int32_t altsetIdx;
    struct usb_host_interface *alts = NULL;
    bool dataFlag = true;
    struct usb_endpoint_descriptor *epDesc = NULL;
    struct AudioUsbFormat *audioUsbFormat = NULL;
    struct usb_device *dev = audioUsbDriver->dev;

    for (altsetIdx = 0; altsetIdx < uacFmt.num; altsetIdx++) {
        if (AudioUsbGetAltsd(&alts, iface, &uacFmt, altsetIdx, &dataFlag) != HDF_SUCCESS) {
            AUDIO_DRIVER_LOG_ERR("AudioUsbGetAltsd failed.");
            continue;
        }
        if (!dataFlag) {
            dataFlag = true;
            continue;
        }
        epDesc = AudioEndpointDescriptor(alts, 0);
        if (epDesc == NULL) {
            AUDIO_DRIVER_LOG_ERR("epDesc is NULL.");
            continue;
        }
        /* must be isochronous */
        if ((epDesc->bmAttributes & USB_ENDPOINT_XFERTYPE_MASK) != USB_ENDPOINT_XFER_ISOC) {
            continue;
        }
        AudioUsbGetFormatSub(audioUsbDriver, alts, &uacFmt, altsetIdx, &audioUsbFormat);
        if (audioUsbFormat == NULL) {
            continue;
        } else if (IS_ERR(audioUsbFormat)) {
            return PTR_ERR(audioUsbFormat);
        }
        if (audioUsbFormat->fmtType != UAC_FORMAT_TYPE_I) {
            *hasNonPcm = true;
        }
        if ((audioUsbFormat->fmtType == UAC_FORMAT_TYPE_I) == nonPcm) {
            AudioUsbFreeFormat(audioUsbFormat);
            audioUsbFormat = NULL;
            continue;
        }
        AudioUsbInitFormatAndRate(audioUsbDriver, audioUsbFormat, &uacFmt, alts, dev);
    }
    return HDF_SUCCESS;
}

static int32_t AudioUsbParseIfaceSub(
    struct AudioUsbDriver *audioUsbDriver, int32_t ifaceNo, bool *hasNonPcm, bool nonPcm)
{
    struct AudioUsbUacFormat uacFmt;
    struct usb_device *dev = NULL;
    struct usb_interface *iface = NULL;
    int ret;

    if (audioUsbDriver == NULL || hasNonPcm == NULL) {
        AUDIO_DRIVER_LOG_ERR("audioUsbDriver is NULL.");
        return HDF_FAILURE;
    }
    (void)memset_s(&uacFmt, sizeof(struct AudioUsbUacFormat), 0, sizeof(struct AudioUsbUacFormat));
    uacFmt.ifaceNo = ifaceNo;
    dev = audioUsbDriver->dev;

    /* parse the interface's altsettings */
    iface = usb_ifnum_to_if(dev, ifaceNo);
    if (iface == NULL) {
        AUDIO_DRIVER_LOG_ERR("iface is NULL.");
        return HDF_FAILURE;
    }
    uacFmt.num = iface->num_altsetting;

    /*
     * Dallas DS4201 workaround: It presents 5 altsettings, but the last
     * one misses syncpipe, and does not produce any sound.
     */
    if (audioUsbDriver->usbId == AudioUsbGetUsbId(0x04fa, 0x4201)) { /* Dallas DS4201 */
        uacFmt.num = AUDIO_USB_FORMAT_NUM_4;
    }

    ret = AudioUsbSetFormat(audioUsbDriver, uacFmt, iface, hasNonPcm, nonPcm);
    if (ret != HDF_SUCCESS) {
        AUDIO_DRIVER_LOG_ERR("AudioUsbSetFormat is failed.");
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

int32_t AudioUsbParseAudioInterface(struct AudioUsbDriver *audioUsbDriver, int32_t ifaceNo)
{
    int32_t ret;
    bool hasNonPcm = false;

    /* parse PCM formats */
    ret = AudioUsbParseIfaceSub(audioUsbDriver, ifaceNo, &hasNonPcm, false);
    if (ret != HDF_SUCCESS) {
        AUDIO_DRIVER_LOG_ERR("AudioUsbParseIfaceSub failed.");
        return ret;
    }

    if (!hasNonPcm) {
        return HDF_SUCCESS;
    }

    /* parse non-PCM formats */
    ret = AudioUsbParseIfaceSub(audioUsbDriver, ifaceNo, &hasNonPcm, true);
    if (ret < 0) {
        AUDIO_DRIVER_LOG_ERR("parse non-PCM formats failed.");
        return ret;
    }

    return HDF_SUCCESS;
}
