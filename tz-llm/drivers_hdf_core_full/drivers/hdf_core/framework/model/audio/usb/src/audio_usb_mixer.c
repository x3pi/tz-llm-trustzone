/*
 * Copyright (c) 2022-2023 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include "audio_usb_mixer.h"
#include <linux/bitops.h>
#include <linux/init.h>
#include <linux/list.h>
#include <linux/log2.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/usb.h>
#include <linux/usb/audio-v2.h>
#include <linux/usb/audio-v3.h>
#include <linux/usb/audio.h>

#include "audio_control.h"
#include "audio_core.h"
#include "audio_driver_log.h"
#include "audio_host.h"
#include "audio_usb_linux.h"
#include "audio_usb_validate_desc.h"

#define HDF_LOG_TAG HDF_AUDIO_USB

#define MAX_ID_ELEMS      256
#define KCTL_NAME_LEN     64
#define MAX_ITEM_NAME_LEN 64
#define HDR_BLENGTH_MAX   4

#define AUDIO_USB_CTL_ELEM_TYPE_NONE       0 /* invalid */
#define AUDIO_USB_CTL_ELEM_TYPE_BOOLEAN    1 /* boolean type */
#define AUDIO_USB_CTL_ELEM_TYPE_INTEGER    2 /* integer type */
#define AUDIO_USB_CTL_ELEM_TYPE_ENUMERATED 3 /* enumerated type */

#define AUDIO_USB_CTL_ELEM_IFACE_CARD  0 /* global control */
#define AUDIO_USB_CTL_ELEM_IFACE_HWDEP 1 /* hardware dependent device */
#define AUDIO_USB_CTL_ELEM_IFACE_MIXER 2 /* virtual mixer device */

#define CTRL_MIXER_BMA_MAIN    0x0100
#define CTRL_MIXER_BMA_MIC     0x0200
#define CTRL_MIXER_BMA_HEADSET 0x0300
#define CTRL_MIXER_BMA_PHONE   0x0400

#define USB_BA_SOURCE_ID_SIZE 1

#define USB_MIXER_DEF_RES_VAL 1
#define USB_MIXER_DEF_CHAN    1
#define USB_MIXER_DEF_MIN_VAL 1
#define USB_MIX_TYPE_INVALID  (-1)

struct UsbMixerElemList {
    struct UsbMixerInterface *mixer;
    struct UsbMixerElemList *nextIdElem; /* list of controls with same id */
    uint32_t id;
    bool isStdInfo;
};

struct UsbMixerInterface {
    struct AudioUsbDriver *audioUsbDriver;
    struct usb_host_interface *hostIf;
    struct list_head list;
    uint32_t ignoreCtlError;
    struct urb *urb;
    struct UsbMixerElemList **idElems; /* array[MAX_ID_ELEMS], indexed by unit id */
    struct usb_ctrlrequest *rcSetupPacket;
    bool disconnected;
    void (*PrivateFree)(struct UsbMixerInterface *mixer);
    int32_t protocol; /* the usb audio specification version this interface complies to */
    struct urb *rcUrb;
};

enum AudioUsbMixerType {
    USB_MIXER_BOOLEAN = 0,
    USB_MIXER_INV_BOOLEAN,
    USB_MIXER_S8,
    USB_MIXER_U8,
    USB_MIXER_S16,
    USB_MIXER_U16,
    USB_MIXER_S32,
    USB_MIXER_U32,
    USB_MIXER_BESPOKEN, /* non-standard type */
};

#define MAX_CHANNELS  16 /* max logical channels */
#define USB_VAL_LEN_1 1
#define USB_VAL_LEN_2 2
#define USB_VAL_LEN_3 3
#define USB_VAL_LEN_4 4

#define USB_SHIFT_SIZE_3  3
#define USB_SHIFT_SIZE_4  4
#define USB_SHIFT_SIZE_8  8
#define USB_SHIFT_SIZE_16 16
#define USB_SHIFT_SIZE_24 24

#define USB_TIMEOUT 10

#define OVERF_LOWDIVISOR_NUM 8
#define VOLUME_RANGE         2

#define USB_DESCRIPTIONS_CONTAIN_100DB  100
#define USB_DESCRIPTIONS_CONTAIN_256DB  256
#define USB_DESCRIPTIONS_CONTAIN_9600DB 9600

#define CHANNEL_HDR_BLENGTH 7

#define ZERO_BASED_INTEGER 0xff
#define S8_MAX_VAL         0x100
#define S16_MAX_VAL        0x10000
#define S8_CAMPARE_VAL     0x80
#define S16_CAMPARE_VAL    0x8000

#define ARRAY_INDEX_0 0
#define ARRAY_INDEX_1 1
#define ARRAY_INDEX_2 2
#define ARRAY_INDEX_3 3
#define ARRAY_INDEX_4 4

#define DEFAULT_CHANNEL 0

#define PCM_TYPE_VAL 0x0100

struct UsbMixerElemInfo {
    struct UsbMixerElemList head;
    uint32_t control; /* CS or ICN (high byte) */
    uint32_t cmask;   /* channel mask bitmap */
    uint32_t idxOff;  /* Control index offset */
    uint32_t chReadOnly;
    uint32_t masterReadOnly;
    int32_t channels;
    int32_t valType;
    int32_t min, max, res; /* min & max is volume range, res is volume reset state */
    int32_t dBMin, dBMax;
    int32_t cached;
    int32_t cacheVal[MAX_CHANNELS];
    uint8_t initialized;
    uint8_t minMute;
    void *privateData;
};

struct UsbMixerDBMap {
    uint32_t min;
    uint32_t max;
};

struct UsbMixerNameMap {
    int32_t id;
    const char *name;
    int32_t control;
    const struct UsbMixerDBMap *dB;
};

struct UsbMixerSelectorMap {
    int32_t id;
    int32_t count;
    const char **names;
};

struct UsbAudioTerm {
    int32_t id;
    int32_t type;
    int32_t channels;
    uint32_t chconfig;
    int32_t name;
};

struct UsbMixerBuild {
    struct AudioUsbDriver *audioUsbDriver;
    struct UsbMixerInterface *mixer;
    uint8_t *buffer;
    uint32_t bufLen;
    DECLARE_BITMAP(unitBitMap, MAX_ID_ELEMS);
    DECLARE_BITMAP(termBitMap, MAX_ID_ELEMS);
    struct UsbAudioTerm oterm;
    const struct UsbMixerNameMap *map;
    const struct UsbMixerSelectorMap *selectorMap;
};

struct AudioUsbItermNameCombo {
    int32_t type;
    char *name;
};

struct AudioUsbFeatureControl {
    uint32_t ctlMask;
    int32_t control;
    int32_t unitId;
    uint32_t readOnlyMask;
    int32_t nameId;
};

struct AudioUsbFeatureParam {
    uint32_t masterBits;
    uint32_t channels;
    uint32_t controlSize;
    uint8_t *bmaControls;
};

struct MixerUnitCtlParam {
    int32_t numOuts;
    int32_t itemChannels;
};

static void AudioUsbMixerElemInitStd(struct UsbMixerElemList *list, struct UsbMixerInterface *mixer, int32_t unitId)
{
    if (list == NULL) {
        AUDIO_DRIVER_LOG_ERR("param is null");
        return;
    }

    list->mixer = mixer;
    list->id = unitId;
}

static int32_t AudioUsbAutoResume(struct AudioUsbDriver *audioUsbDriver)
{
    int32_t err;

    if (atomic_read(&audioUsbDriver->shutdown) != 0) {
        AUDIO_DRIVER_LOG_ERR("atomic_read fail");
        return HDF_FAILURE;
    }

    if (atomic_inc_return(&audioUsbDriver->active) != 1) {
        AUDIO_DRIVER_LOG_DEBUG("atomic_inc_return success");
        return HDF_SUCCESS;
    }

    err = usb_autopm_get_interface(audioUsbDriver->usbIf);
    if (err < HDF_SUCCESS) {
        /* rollback */
        usb_autopm_put_interface(audioUsbDriver->usbIf);
        atomic_dec(&audioUsbDriver->active);
        AUDIO_DRIVER_LOG_ERR("usb_autopm_get_interface fail");
        return err;
    }

    return HDF_SUCCESS;
}

/* lock the shutdown (disconnect) task and autoresume */
static int32_t AudioUsbLockShutdown(struct AudioUsbDriver *audioUsbDriver)
{
    int32_t err;

    atomic_inc(&audioUsbDriver->usageCount);
    if (atomic_read(&audioUsbDriver->shutdown) != 0) {
        if (atomic_dec_and_test(&audioUsbDriver->usageCount) != 0) {
            AUDIO_DRIVER_LOG_ERR("atomic_dec_and_test fail");
            wake_up(&audioUsbDriver->shutdownWait);
        }
        AUDIO_DRIVER_LOG_ERR("atomic_read fail");
        return HDF_FAILURE;
    }

    err = AudioUsbAutoResume(audioUsbDriver);
    if (err < HDF_SUCCESS) {
        if (atomic_dec_and_test(&audioUsbDriver->usageCount) != 0) {
            AUDIO_DRIVER_LOG_ERR("atomic_dec_and_test fail");
            wake_up(&audioUsbDriver->shutdownWait);
        }
        return err;
    }
    return HDF_SUCCESS;
}

static void AudioUsbAutoSuspend(struct AudioUsbDriver *audioUsbDriver)
{
    if (atomic_read(&audioUsbDriver->shutdown) != 0) {
        AUDIO_DRIVER_LOG_ERR("The USB audio card has been shut down");
        return;
    }

    if (atomic_dec_and_test(&audioUsbDriver->active) == 0) {
        AUDIO_DRIVER_LOG_WARNING("The USB audio card is not active");
        return;
    }

    usb_autopm_put_interface(audioUsbDriver->usbIf);
}

/* autosuspend and unlock the shutdown */
static void AudioUsbUnlockShutdown(struct AudioUsbDriver *audioUsbDriver)
{
    AudioUsbAutoSuspend(audioUsbDriver);
    if (atomic_dec_and_test(&audioUsbDriver->usageCount) != 0) {
        wake_up(&audioUsbDriver->shutdownWait);
    }
}

static const struct UsbMixerNameMap *AudioUsbFindMap(
    const struct UsbMixerNameMap *usbMixerNameMap, int32_t unitId, int32_t control)
{
    for (; usbMixerNameMap != NULL && usbMixerNameMap->id != 0; usbMixerNameMap++) {
        if (usbMixerNameMap->id == unitId &&
            (control == 0 || usbMixerNameMap->control == 0 || control == usbMixerNameMap->control)) {
            return usbMixerNameMap;
        }
    }

    AUDIO_DRIVER_LOG_WARNING("usbMixerNameMap error");
    return NULL;
}

/* get the mapped name if the unit matches */
static int32_t AudioUsbCheckMappedName(const struct UsbMixerNameMap *usbMixerNameMap, char *buf, int32_t bufLen)
{
    if (usbMixerNameMap == NULL || usbMixerNameMap->name == NULL) {
        return 0; /* 0 for usbMixerNameMap name lenght */
    }

    bufLen--;
    return strlcpy(buf, usbMixerNameMap->name, bufLen);
}

/* ignore the error value if ignoreCtlError flag is set */
static inline int32_t AudioUsbFilterError(struct UsbMixerElemInfo *mixElemInfo, int32_t err)
{
    if (mixElemInfo == NULL || mixElemInfo->head.mixer == NULL) {
        AUDIO_DRIVER_LOG_ERR("param is null");
        return HDF_ERR_INVALID_PARAM;
    }

    return (mixElemInfo->head.mixer->ignoreCtlError != 0) ? 0 : err;
}

/* get the mapped selector source name */
static int32_t AudioUsbCheckMappedSelectorName(
    struct UsbMixerBuild *state, int32_t unitId, int32_t index, char *buf, int32_t bufLen)
{
    const struct UsbMixerSelectorMap *usbMixerSelectorMap = NULL;
    (void)bufLen;

    if (state == NULL || state->selectorMap == NULL || buf == NULL) {
        AUDIO_DRIVER_LOG_ERR("param is null");
        return HDF_FAILURE;
    }

    for (usbMixerSelectorMap = state->selectorMap; usbMixerSelectorMap != NULL && usbMixerSelectorMap->id != 0;
         usbMixerSelectorMap++) {
        if (usbMixerSelectorMap->id == unitId && index < usbMixerSelectorMap->count) {
            return strlcpy(buf, usbMixerSelectorMap->names[index], bufLen);
        }
    }

    return HDF_FAILURE;
}

/* find an audio control unit with the given unit id */
static void *AudioUsbFindAudioControlUnit(struct UsbMixerBuild *state, unsigned char unit)
{
    /* we just parse the header */
    struct uac_feature_unit_descriptor *featureUnitDesc = NULL;
    bool lenth = false;
    bool subtypeInput = false;
    bool subtypeRate = false;
    bool unitId = false;

    featureUnitDesc = AudioUsbFindDesc(state->buffer, state->bufLen, featureUnitDesc, USB_DT_CS_INTERFACE);

    while (featureUnitDesc != NULL) {
        lenth = featureUnitDesc->bLength >= HDR_BLENGTH_MAX;
        subtypeInput = featureUnitDesc->bDescriptorSubtype >= UAC_INPUT_TERMINAL;
        subtypeRate = featureUnitDesc->bDescriptorSubtype <= UAC3_SAMPLE_RATE_CONVERTER;
        unitId = featureUnitDesc->bUnitID == unit;
        if (lenth && subtypeInput && subtypeRate && unitId) {
            return featureUnitDesc;
        }
        featureUnitDesc = AudioUsbFindDesc(state->buffer, state->bufLen, featureUnitDesc, USB_DT_CS_INTERFACE);
    }

    AUDIO_DRIVER_LOG_ERR("featureUnitDesc is null");
    return NULL;
}

/* copy a string with the given id */
static uint32_t AudioUsbCopyStringDesc(struct AudioUsbDriver *audioUsbDriver, int32_t index, char *buf, int32_t maxLen)
{
    int32_t len = usb_string(audioUsbDriver->dev, index, buf, maxLen - 1);
    if (len < 0) {
        AUDIO_DRIVER_LOG_ERR("len error, len = %d", len);
        return 0; /* if usb_string error, default len is 0 */
    }

    buf[len] = 0;
    return len;
}

/* convert from the byte/word on usb descriptor to the zero-based integer */
static int32_t AudioUsbConvertSignedValue(struct UsbMixerElemInfo *mixElemInfo, int32_t value)
{
    switch (mixElemInfo->valType) {
        case USB_MIXER_BOOLEAN:
            value = value != 0 ? 1 : 0; /* 1 is reached; 0 is not reached */
            break;
        case USB_MIXER_INV_BOOLEAN:
            value = value != 0 ? 0 : 1; /* 1 is reached; 0 is not reached */
            break;
        case USB_MIXER_S8:
            value &= ZERO_BASED_INTEGER;
            if (value >= S8_CAMPARE_VAL) {
                value = value - S8_MAX_VAL;
            }
            break;
        case USB_MIXER_U8:
            value = value & ZERO_BASED_INTEGER;
            break;
        case USB_MIXER_S16:
            value = value & 0xffff;
            if (value >= S16_CAMPARE_VAL) {
                value = value - S16_MAX_VAL;
            }
            break;
        case USB_MIXER_U16:
            value = value & 0xffff;
            break;
        default:
            AUDIO_DRIVER_LOG_ERR("valType not reached");
    }
    return value;
}

/* convert from the zero-based int32_t to the byte/word for usb descriptor */
static int32_t AudioUsbConvertBytesValue(struct UsbMixerElemInfo *mixElemInfo, int32_t value)
{
    int32_t temp = 0;

    switch (mixElemInfo->valType) {
        case USB_MIXER_BOOLEAN:
            temp = value != 0 ? 1 : 0; /* 1 is reached; 0 is not reached */
            break;
        case USB_MIXER_INV_BOOLEAN:
            temp = value != 0 ? 0 : 1; /* 1 is reached; 0 is not reached */
            break;
        case USB_MIXER_S16:
        case USB_MIXER_U16:
            temp = value & 0xffff;
            break;
        case USB_MIXER_S8:
        case USB_MIXER_U8:
            temp = value & ZERO_BASED_INTEGER;
            break;
        default:
            AUDIO_DRIVER_LOG_ERR("valType not reached");
            temp = 0; /* 1 is reached; 0 is not reached */
            break;
    }
    return temp;
}

static int32_t AudioUsbGetRelativeValue(struct UsbMixerElemInfo *mixElemInfo, int32_t value)
{
    if (mixElemInfo->res == 0) { /* 0 is volume reset off */
        mixElemInfo->res = 1;    /* 1 is volume reset on */
    }

    if (value < mixElemInfo->min) {
        return 0; /* if value error, return default value 0 */
    }
    if (value >= mixElemInfo->max) {
        return (mixElemInfo->max - mixElemInfo->min + mixElemInfo->res - 1) / mixElemInfo->res;
    }

    return (value - mixElemInfo->min) / mixElemInfo->res;
}

static int32_t AudioUsbGetAbsValue(struct UsbMixerElemInfo *mixElemInfo, int32_t value)
{
    if (value < 0) {
        return mixElemInfo->min;
    }
    if (mixElemInfo->res == 0) {
        mixElemInfo->res = 1;
    }
    value *= mixElemInfo->res;
    value += mixElemInfo->min;
    if (value > mixElemInfo->max) {
        return mixElemInfo->max;
    }
    return value;
}

static int32_t AudioUsbCtlValueSize(int32_t valType)
{
    switch (valType) {
        case USB_MIXER_S32:
        case USB_MIXER_U32:
            return USB_MIXER_S16;
        case USB_MIXER_S16:
        case USB_MIXER_U16:
            return USB_MIXER_S8;
        default:
            AUDIO_DRIVER_LOG_ERR("not find usb mixer type %d", valType);
            return USB_MIXER_INV_BOOLEAN;
    }
}

/* retrieve a mixer value */
static inline int32_t AudioUsbMixerCtrlIntf(struct UsbMixerInterface *mixer)
{
    struct usb_interface_descriptor *inteDesc = NULL;

    inteDesc = AudioUsbGetIfaceDesc(mixer->hostIf);
    if (inteDesc == NULL) {
        AUDIO_DRIVER_LOG_ERR("inteDesc is null");
        return HDF_FAILURE;
    }
    return inteDesc->bInterfaceNumber;
}

static int32_t AudioUsbGetCtlValueV1(
    struct UsbMixerElemInfo *mixElemInfo, int32_t request, int32_t valIdx, int32_t *value)
{
    struct AudioUsbDriver *audioUsbDriver = mixElemInfo->head.mixer->audioUsbDriver;
    uint8_t buf[USB_VAL_LEN_2] = {0};
    int32_t valLen = mixElemInfo->valType >= USB_MIXER_S16 ? USB_VAL_LEN_2 : USB_VAL_LEN_1;
    int32_t timeout = USB_TIMEOUT;
    int32_t idx = 0;
    int32_t err;
    struct AudioUsbCtlMsgParam usbCtlMsgParam;

    err = AudioUsbLockShutdown(audioUsbDriver);
    if (err < HDF_SUCCESS) {
        return HDF_ERR_IO;
    }

    while (timeout-- > 0) {
        idx = AudioUsbMixerCtrlIntf(mixElemInfo->head.mixer) | (mixElemInfo->head.id << USB_SHIFT_SIZE_8);
        usbCtlMsgParam.pipe = usb_rcvctrlpipe(audioUsbDriver->dev, 0);
        usbCtlMsgParam.request = request;
        usbCtlMsgParam.requestType = USB_RECIP_INTERFACE | USB_TYPE_CLASS | USB_DIR_IN;
        usbCtlMsgParam.value = valIdx;
        usbCtlMsgParam.index = idx;

        err = AudioUsbCtlMsg(audioUsbDriver->dev, &usbCtlMsgParam, buf, valLen);
        if (err >= valLen) {
            *value = AudioUsbConvertSignedValue(mixElemInfo, AudioUsbCombineBytes(buf, valLen));
            err = 0;
            AudioUsbUnlockShutdown(audioUsbDriver);
            return err;
        } else if (err == -ETIMEDOUT) {
            AudioUsbUnlockShutdown(audioUsbDriver);
            return err;
        }
    }
    AUDIO_DRIVER_LOG_ERR("cannot get ctl value: req = %#x, wValue = %#x, wIndex = %#x, type = %d\n", request, valIdx,
        idx, mixElemInfo->valType);
    err = -EINVAL;
    AudioUsbUnlockShutdown(audioUsbDriver);
    return err;
}

static int32_t AudioUsbGetCtlValueV2(
    struct UsbMixerElemInfo *mixElemInfo, int32_t request, int32_t valIdx, int32_t *value)
{
    struct AudioUsbDriver *audioUsbDriver = mixElemInfo->head.mixer->audioUsbDriver;
    /* enough space for one range */
    uint8_t buf[sizeof(uint16_t) + USB_VAL_LEN_3 * sizeof(uint32_t)];
    uint8_t *valTemp = NULL;
    int32_t ret, valSize, size, shift;
    struct AudioUsbCtlMsgParam usbCtlMsgParam;

    valSize = AudioUsbCtlValueSize(mixElemInfo->valType);
    if (request == UAC_GET_CUR) {
        usbCtlMsgParam.request = UAC2_CS_CUR;
        size = valSize;
    } else {
        usbCtlMsgParam.request = UAC2_CS_RANGE;
        size = sizeof(uint16_t) + USB_VAL_LEN_3 * valSize;
    }

    AudioUsbLockShutdown(audioUsbDriver);
    usbCtlMsgParam.index = AudioUsbMixerCtrlIntf(mixElemInfo->head.mixer) | (mixElemInfo->head.id << USB_SHIFT_SIZE_8);
    usbCtlMsgParam.pipe = usb_rcvctrlpipe(audioUsbDriver->dev, 0);
    usbCtlMsgParam.requestType = USB_RECIP_INTERFACE | USB_TYPE_CLASS | USB_DIR_IN;
    usbCtlMsgParam.value = valIdx;

    (void)memset_s(buf, sizeof(buf), 0, sizeof(buf));
    ret = AudioUsbCtlMsg(audioUsbDriver->dev, &usbCtlMsgParam, buf, size);
    AudioUsbUnlockShutdown(audioUsbDriver);

    if (ret < HDF_SUCCESS) {
        AUDIO_DRIVER_LOG_ERR("cannot get ctl type = %d\n", mixElemInfo->valType);
        return ret;
    }

    switch (request) {
        case UAC_GET_CUR:
            valTemp = buf;
            break;
        case UAC_GET_RES:
            shift = sizeof(uint16_t) + valSize * USB_VAL_LEN_2;
            valTemp = buf + shift;
            break;
        case UAC_GET_MIN:
            shift = sizeof(uint16_t);
            valTemp = buf + shift;
            break;
        case UAC_GET_MAX:
            shift = sizeof(uint16_t) + valSize;
            valTemp = buf + shift;
            break;
        default:
            return HDF_ERR_INVALID_PARAM;
    }

    *value = AudioUsbConvertSignedValue(mixElemInfo, AudioUsbCombineBytes(valTemp, valSize));

    return HDF_SUCCESS;
}

static int32_t AudioUsbGetCtlValue(
    struct UsbMixerElemInfo *mixElemInfo, int32_t request, int32_t valIdx, int32_t *value)
{
    valIdx += mixElemInfo->idxOff;

    if (mixElemInfo->head.mixer->protocol == UAC_VERSION_1) {
        return AudioUsbGetCtlValueV1(mixElemInfo, request, valIdx, value);
    } else {
        return AudioUsbGetCtlValueV2(mixElemInfo, request, valIdx, value);
    }
}

static int32_t AudioUsbGetCurCtlValue(struct UsbMixerElemInfo *mixElemInfo, int32_t valIdx, int32_t *value)
{
    return AudioUsbGetCtlValue(mixElemInfo, UAC_GET_CUR, valIdx, value);
}

static inline int32_t AudioUsbGetCurMixRaw(struct UsbMixerElemInfo *mixElemInfo, int32_t channel, int32_t *value)
{
    return AudioUsbGetCtlValue(mixElemInfo, UAC_GET_CUR, (mixElemInfo->control << USB_SHIFT_SIZE_8) | channel, value);
}

int32_t AudioUsbGetCurMixValue(struct UsbMixerElemInfo *mixElemInfo, int32_t channel, int32_t index, int32_t *value)
{
    if (mixElemInfo->cached & (1 << channel)) {
        *value = mixElemInfo->cacheVal[index];
        return HDF_SUCCESS;
    }

    if (index >= MAX_CHANNELS) {
        AUDIO_DRIVER_LOG_ERR("index is invalid");
        return HDF_FAILURE;
    }

    if (AudioUsbGetCurMixRaw(mixElemInfo, channel, value) < HDF_SUCCESS) {
        return HDF_FAILURE;
    }

    mixElemInfo->cached |= 1 << channel; // channel mask
    mixElemInfo->cacheVal[index] = *value;
    return HDF_SUCCESS;
}

/* set a mixer value */
int32_t AudioUsbMixerSetCtlValue(
    struct UsbMixerElemInfo *mixElemInfo, int32_t request, int32_t valIdx, int32_t valueSet)
{
    struct AudioUsbDriver *audioUsbDriver = mixElemInfo->head.mixer->audioUsbDriver;
    uint8_t buf[USB_VAL_LEN_4] = {0};
    int32_t valLen, err;
    int32_t timeout = USB_TIMEOUT;
    struct AudioUsbCtlMsgParam usbCtlMsgParam;

    valIdx += mixElemInfo->idxOff;

    if (mixElemInfo->head.mixer->protocol == UAC_VERSION_1) {
        valLen = mixElemInfo->valType >= USB_MIXER_S16 ? USB_VAL_LEN_2 : USB_VAL_LEN_1;
    } else { /* UAC_VERSION_2/3 */
        valLen = AudioUsbCtlValueSize(mixElemInfo->valType);
        if (request != UAC_SET_CUR) {
            AUDIO_DEVICE_LOG_INFO("RANGE setting not yet supported\n");
            return -EINVAL;
        }
        request = UAC2_CS_CUR;
    }

    valueSet = AudioUsbConvertBytesValue(mixElemInfo, valueSet);
    buf[ARRAY_INDEX_0] = valueSet & ZERO_BASED_INTEGER;
    buf[ARRAY_INDEX_1] = (valueSet >> USB_SHIFT_SIZE_8) & ZERO_BASED_INTEGER;
    buf[ARRAY_INDEX_2] = (valueSet >> USB_SHIFT_SIZE_16) & ZERO_BASED_INTEGER;
    buf[ARRAY_INDEX_3] = (valueSet >> USB_SHIFT_SIZE_24) & ZERO_BASED_INTEGER;

    err = AudioUsbLockShutdown(audioUsbDriver);
    if (err < HDF_SUCCESS) {
        AUDIO_DEVICE_LOG_INFO("AudioUsbLockShutdown failed\n");
        return HDF_ERR_IO;
    }
    while (timeout-- > 0) {
        usbCtlMsgParam.index =
            AudioUsbMixerCtrlIntf(mixElemInfo->head.mixer) | (mixElemInfo->head.id << USB_SHIFT_SIZE_8);

        usbCtlMsgParam.pipe = usb_sndctrlpipe(audioUsbDriver->dev, 0);
        usbCtlMsgParam.request = request;
        usbCtlMsgParam.requestType = USB_RECIP_INTERFACE | USB_TYPE_CLASS | USB_DIR_OUT;
        usbCtlMsgParam.value = valIdx;

        err = AudioUsbCtlMsg(audioUsbDriver->dev, &usbCtlMsgParam, buf, valLen);
        if (err >= HDF_SUCCESS) {
            AudioUsbUnlockShutdown(audioUsbDriver);
            return HDF_SUCCESS;
        } else if (err == -ETIMEDOUT) {
            AudioUsbUnlockShutdown(audioUsbDriver);
            return err;
        }
    }
    AudioUsbUnlockShutdown(audioUsbDriver);
    return HDF_ERR_INVALID_PARAM;
}

static int32_t AudioUsbSetCurCtlValue(struct UsbMixerElemInfo *mixElemInfo, int32_t valIdx, int32_t value)
{
    return AudioUsbMixerSetCtlValue(mixElemInfo, UAC_SET_CUR, valIdx, value);
}

int32_t AudioUsbSetCurMixValue(struct UsbMixerElemInfo *mixElemInfo, int32_t channel, int32_t index, int32_t value)
{
    uint32_t readOnly;

    if (channel < 0) {
        AUDIO_DEVICE_LOG_ERR("error channel value = %d", channel);
        return HDF_ERR_INVALID_PARAM;
    }
    readOnly = channel == 0 ? mixElemInfo->masterReadOnly : mixElemInfo->chReadOnly & (1 << (channel - 1));
    if (readOnly != 0) {
        AUDIO_DEVICE_LOG_DEBUG("%s(): channel %d of control %d is readOnly\n", __func__, channel, mixElemInfo->control);
        return HDF_SUCCESS;
    }

    if (AudioUsbMixerSetCtlValue(
        mixElemInfo, UAC_SET_CUR, (mixElemInfo->control << USB_SHIFT_SIZE_8) | channel, value) < HDF_SUCCESS) {
        return HDF_FAILURE;
    }

    mixElemInfo->cached |= 1 << channel;
    mixElemInfo->cacheVal[index] = value;
    return HDF_SUCCESS;
}

static int32_t AudioUsbParseUnit(struct UsbMixerBuild *state, int32_t unitId);

static int32_t AudioUsbCheckMatrixBitmap(const uint8_t *bmap, int32_t itemChannels,
    int32_t outChannel, int32_t numOuts)
{
    int32_t idx = itemChannels * numOuts + outChannel;
    int32_t bit = idx >> USB_SHIFT_SIZE_3;
    int32_t map = idx & CHANNEL_HDR_BLENGTH;
    return bmap[bit] & (S8_CAMPARE_VAL >> map);
}

static int32_t AudioUsbGetTermNameSub(struct UsbAudioTerm *usbAudioTerm, uint8_t *name, uint32_t len)
{
    char *itemName = "Main";

    switch (usbAudioTerm->type) {
        case 0x0300 ... 0x0307: /* Output, Speaker, Headphone, HMD Audio, Desktop Speaker, Room Speaker, Com Speaker,
                                   LFE */
        case 0x0600 ... 0x0607: /* External In, Analog In, Digital In, Line, Legacy In, IEC958 In, 1394 DA Stream, 1394
                                   DV Stream */
        case 0x0700 ... 0x0713: /* Embedded, Noise Source, Equalization Noise, CD, DAT, DCC, MiniDisk, Analog Tape,
                                   Phonograph, VCR Audio, Video Disk Audio, DVD Audio, TV Tuner Audio, Satellite Rec
                                   Audio, Cable Tuner Audio, DSS Audio, Radio Receiver, Radio Transmitter, Multi-Track
                                   Recorder, Synthesizer */
            (void)strcpy_s(name, len, itemName);
            return strlen(itemName);
        default:
            return 0; /* item name lenth */
    }
}

static int32_t AudioUsbGetTermName(struct AudioUsbDriver *audioUsbDriver, struct UsbAudioTerm *usbAudioTerm,
    uint8_t *name, int32_t maxLen, int32_t termOnly)
{
    int32_t len;

    if (usbAudioTerm->name) {
        len = AudioUsbCopyStringDesc(audioUsbDriver, usbAudioTerm->name, name, maxLen);
        if (len != 0) {
            return len;
        }
    }

    /* virtual type - not a real terminal */
    if ((usbAudioTerm->type >> USB_SHIFT_SIZE_16) != 0) {
        if (termOnly != 0) {
            return 0; // name length
        }
        switch (usbAudioTerm->type >> USB_SHIFT_SIZE_16) {
            case UAC3_SELECTOR_UNIT:
                (void)strcpy_s(name, KCTL_NAME_LEN, "Selector");
                return strlen("Selector");
            case UAC3_PROCESSING_UNIT:
                (void)strcpy_s(name, KCTL_NAME_LEN, "Process Unit");
                return strlen("Process Unit");
            case UAC3_EXTENSION_UNIT:
                (void)strcpy_s(name, KCTL_NAME_LEN, "Ext Unit");
                return strlen("Ext Unit");
            case UAC3_MIXER_UNIT:
                (void)strcpy_s(name, KCTL_NAME_LEN, "Mixer");
                return strlen("Mixer");
            default:
                return sprintf_s(name, KCTL_NAME_LEN, "Unit %d", usbAudioTerm->id);
        }
    }

    switch (usbAudioTerm->type & 0xff00) {
        case CTRL_MIXER_BMA_MAIN:
            (void)strcpy_s(name, KCTL_NAME_LEN, "Main");
            return strlen("Main");
        case CTRL_MIXER_BMA_MIC:
            (void)strcpy_s(name, KCTL_NAME_LEN, "Mic");
            return strlen("Mic");
        case CTRL_MIXER_BMA_HEADSET:
            (void)strcpy_s(name, KCTL_NAME_LEN, "Main");
            return strlen("Main");
        case CTRL_MIXER_BMA_PHONE:
            (void)strcpy_s(name, KCTL_NAME_LEN, "Phone");
            return strlen("Phone");
        default:
            break;
    }

    return AudioUsbGetTermNameSub(usbAudioTerm, name, KCTL_NAME_LEN);
}

static inline int32_t AudioUsbCtrlIntf(struct AudioUsbDriver *audioUsbDriver)
{
    struct usb_interface_descriptor *inteDesc = NULL;

    inteDesc = AudioUsbGetIfaceDesc(audioUsbDriver->ctrlIntf);

    return inteDesc->bInterfaceNumber;
}

/* Get logical cluster information for UAC3 devices */
static int32_t AudioUsbGetClusterChannelsV3(struct UsbMixerBuild *state, uint32_t clusterId)
{
    int32_t err;
    struct uac3_cluster_header_descriptor uacHeader;

    struct AudioUsbCtlMsgParam usbCtlMsgParam;

    usbCtlMsgParam.pipe = usb_rcvctrlpipe(state->audioUsbDriver->dev, 0);
    usbCtlMsgParam.request = UAC3_CS_REQ_HIGH_CAPABILITY_DESCRIPTOR;
    usbCtlMsgParam.requestType = USB_RECIP_INTERFACE | USB_TYPE_CLASS | USB_DIR_IN;
    usbCtlMsgParam.value = clusterId;
    usbCtlMsgParam.index = AudioUsbCtrlIntf(state->audioUsbDriver);

    err = AudioUsbCtlMsg(state->audioUsbDriver->dev, &usbCtlMsgParam, &uacHeader, sizeof(uacHeader));
    if (err < 0) {
        AUDIO_DRIVER_LOG_ERR("cannot request logical cluster ID: %d (err: %d)\n", clusterId, err);
        return err;
    }
    if (err != sizeof(uacHeader)) {
        err = -EIO;
        AUDIO_DRIVER_LOG_ERR("cannot request logical cluster ID: %d (err: %d)\n", clusterId, err);
        return err;
    }

    return uacHeader.bNrChannels;
}

/* Get number of channels for a Mixer Unit */
static int32_t AudioUsbMixerUnitGetChannels(struct UsbMixerBuild *state, struct uac_mixer_unit_descriptor *desc)
{
    int32_t muChannels;
    uint32_t temp;

    if (state->mixer->protocol == UAC_VERSION_3) {
        muChannels = AudioUsbGetClusterChannelsV3(state, uac3_mixer_unit_wClusterDescrID(desc));
    } else {
        temp = sizeof(*desc) + desc->bNrInPins + 1;
        if (desc->bLength < temp) {
            return 0; // channel number
        }
        muChannels = uac_mixer_unit_bNrChannels(desc);
    }
    return muChannels;
}

/* Parse Input Terminal Unit */
static int32_t AudioUsbCheckInputTermSub(struct UsbMixerBuild *state, int32_t id, struct UsbAudioTerm *term);

static int32_t AudioUsbParseTermUac1ItermUnit(
    struct UsbMixerBuild *state, struct UsbAudioTerm *term, void *unitDesc, int32_t id)
{
    struct uac_input_terminal_descriptor *inptTermDesc = unitDesc;

    term->type = le16_to_cpu(inptTermDesc->wTerminalType);
    term->channels = inptTermDesc->bNrChannels;
    term->chconfig = le16_to_cpu(inptTermDesc->wChannelConfig);
    term->name = inptTermDesc->iTerminal;
    return HDF_SUCCESS;
}

static int32_t AudioUsbParseTermMixerUnit(
    struct UsbMixerBuild *state, struct UsbAudioTerm *term, void *unitDesc, int32_t id)
{
    struct uac_mixer_unit_descriptor *mixUnitDesc = unitDesc;
    int32_t protocol = state->mixer->protocol;
    int32_t err;

    err = AudioUsbMixerUnitGetChannels(state, mixUnitDesc);
    if (err <= HDF_SUCCESS) {
        return err;
    }

    term->type = UAC3_MIXER_UNIT << USB_SHIFT_SIZE_16; /* virtual type */
    term->channels = err;
    if (protocol != UAC_VERSION_3) {
        term->chconfig = uac_mixer_unit_wChannelConfig(mixUnitDesc, protocol);
        term->name = uac_mixer_unit_iMixer(mixUnitDesc);
    }
    return HDF_SUCCESS;
}

static int32_t AudioUsbParseTermSelectorUnit(
    struct UsbMixerBuild *state, struct UsbAudioTerm *term, void *unitDesc, int32_t id)
{
    struct uac_selector_unit_descriptor *selUnitDesc = unitDesc;
    int32_t err;

    /* call recursively to retrieve the channel info */
    err = AudioUsbCheckInputTermSub(state, selUnitDesc->baSourceID[0], term);
    if (err < HDF_SUCCESS)
        return err;
    term->type = UAC3_SELECTOR_UNIT << USB_SHIFT_SIZE_16; /* virtual type */
    term->id = id;
    if (state->mixer->protocol != UAC_VERSION_3)
        term->name = uac_selector_unit_iSelector(selUnitDesc);
    return HDF_SUCCESS;
}

static int32_t AudioUsbParseTermProcUnit(
    struct UsbMixerBuild *state, struct UsbAudioTerm *term, void *unitDesc, int32_t id, int32_t uacProcessingUnit)
{
    struct uac_processing_unit_descriptor *procUnitDesc = unitDesc;
    int32_t protocol = state->mixer->protocol;
    int32_t err;
    AUDIO_DRIVER_LOG_DEBUG("entry");

    if (procUnitDesc->bNrInPins) {
        err = AudioUsbCheckInputTermSub(state, procUnitDesc->baSourceID[0], term);
        if (err < HDF_SUCCESS) {
            AUDIO_DRIVER_LOG_ERR("AudioUsbCheckInputTermSub if failed");
            return err;
        }
    }

    term->type = uacProcessingUnit << USB_SHIFT_SIZE_16;
    term->id = id;

    if (protocol == UAC_VERSION_3) {
        AUDIO_DRIVER_LOG_INFO("UAC_VERSION_3 is not support");
        return HDF_SUCCESS;
    }
    if (!term->channels) {
        term->channels = uac_processing_unit_bNrChannels(procUnitDesc);
        term->chconfig = uac_processing_unit_wChannelConfig(procUnitDesc, protocol);
    }
    term->name = uac_processing_unit_iProcessing(procUnitDesc, protocol);
    return HDF_SUCCESS;
}

static int32_t AudioUsbCheckInputTermSub(struct UsbMixerBuild *state, int32_t id, struct UsbAudioTerm *term)
{
    int32_t protocol = state->mixer->protocol;
    void *hdrRet = NULL;
    uint8_t *hdr = NULL;

    for (;;) {
        /* a loop in the terminal chain? */
        if (test_and_set_bit(id, state->termBitMap)) {
            AUDIO_DRIVER_LOG_ERR("test_and_set_bit failed");
            return HDF_ERR_INVALID_PARAM;
        }

        hdrRet = AudioUsbFindAudioControlUnit(state, id);
        if (hdrRet == NULL || !AudioUsbValidateAudioDesc(hdrRet, protocol)) {
            break; /* bad descriptor */
        }

        hdr = hdrRet;
        term->id = id;

        // here use version and UAC interface descriptor subtypes to determine the parsing function.
        switch (((protocol << USB_SHIFT_SIZE_8) | hdr[USB_VAL_LEN_2])) {
            case 0x0006: // UAC_VERSION_1 << 8 | UAC_FEATURE_UNIT
            case 0x2006: // UAC_VERSION_2 << 8 | UAC_FEATURE_UNIT
            case 0x3007: // UAC_VERSION_3 << 8 | UAC3_FEATURE_UNIT
                id = ((struct uac_feature_unit_descriptor *)hdrRet)->bSourceID;
                break;   /* continue to parse */
            case 0x0002: // UAC_VERSION_1 << 8 | UAC_INPUT_TERMINAL
                return AudioUsbParseTermUac1ItermUnit(state, term, hdrRet, id);
            case 0x0004: // UAC_VERSION_1 << 8 | UAC_MIXER_UNIT
            case 0x2004: // UAC_VERSION_2 << 8 | UAC_MIXER_UNIT
            case 0x3005: // UAC_VERSION_3 << 8 | UAC3_MIXER_UNIT
                return AudioUsbParseTermMixerUnit(state, term, hdrRet, id);
            case 0x0005: // UAC_VERSION_1 << 8 | UAC_SELECTOR_UNIT
            case 0x2005: // UAC_VERSION_2 << 8 | UAC_SELECTOR_UNIT
            case 0x200b: // UAC_VERSION_2 << 8 | UAC2_CLOCK_SELECTOR
            case 0x3006: // UAC_VERSION_3 << 8 | UAC3_SELECTOR_UNIT
            case 0x300c: // UAC_VERSION_3 << 8 | UAC3_CLOCK_SELECTOR
                return AudioUsbParseTermSelectorUnit(state, term, hdrRet, id);
            case 0x0007: // UAC_VERSION_1 << 8 | UAC1_PROCESSING_UNIT
            case 0x2008: // UAC_VERSION_2 << 8 | UAC2_PROCESSING_UNIT_V2
            case 0x3009: // UAC_VERSION_3 << 8 | UAC3_PROCESSING_UNIT
                return AudioUsbParseTermProcUnit(state, term, hdrRet, id, UAC3_PROCESSING_UNIT);
            case 0x0008: // UAC_VERSION_1 << 8 | UAC1_EXTENSION_UNIT
            case 0x2009: // UAC_VERSION_2 << 8 | UAC2_EXTENSION_UNIT_V2
            case 0x300a: // UAC_VERSION_3 << 8 | UAC3_EXTENSION_UNIT
                return AudioUsbParseTermProcUnit(state, term, hdrRet, id, UAC3_EXTENSION_UNIT);
            default:
                AUDIO_DRIVER_LOG_ERR("error UAC interface descriptor");
                return HDF_DEV_ERR_NO_DEVICE;
        }
    }
    AUDIO_DRIVER_LOG_ERR("error no device");
    return HDF_DEV_ERR_NO_DEVICE;
}

static int32_t AudioUsbCheckInputTerm(struct UsbMixerBuild *state, int32_t id, struct UsbAudioTerm *term)
{
    (void)memset_s(term, sizeof(*term), 0, sizeof(*term));
    (void)memset_s(state->termBitMap, sizeof(state->termBitMap), 0, sizeof(state->termBitMap));
    return AudioUsbCheckInputTermSub(state, id, term);
}

/* feature unit control information */
struct AudioUsbFeatureControlInfo {
    int32_t control;
    const char *name;
    int32_t type;     /* data type for uac1 */
    int32_t typeUac2; /* data type for uac2 if different from uac1, else -1 */
};

static const struct AudioUsbFeatureControlInfo g_audioFeatureInfo[] = {
    {UAC_FU_MUTE,              "Mute",                   USB_MIXER_INV_BOOLEAN, USB_MIX_TYPE_INVALID},
    {UAC_FU_VOLUME,            "Volume",                 USB_MIXER_S16,         USB_MIX_TYPE_INVALID},
    {UAC_FU_BASS,              "Tone Control - Bass",    USB_MIXER_S8,          USB_MIX_TYPE_INVALID},
    {UAC_FU_MID,               "Tone Control - Mid",     USB_MIXER_S8,          USB_MIX_TYPE_INVALID},
    {UAC_FU_TREBLE,            "Tone Control - Treble",  USB_MIXER_S8,          USB_MIX_TYPE_INVALID},
    {UAC_FU_GRAPHIC_EQUALIZER, "Graphic Equalizer",      USB_MIXER_S8,          USB_MIX_TYPE_INVALID},
    {UAC_FU_AUTOMATIC_GAIN,    "Auto Gain Control",      USB_MIXER_BOOLEAN,     USB_MIX_TYPE_INVALID},
    {UAC_FU_DELAY,             "Delay Control",          USB_MIXER_U16,         USB_MIXER_U32       },
    {UAC_FU_BASS_BOOST,        "Bass Boost",             USB_MIXER_BOOLEAN,     USB_MIX_TYPE_INVALID},
    {UAC_FU_LOUDNESS,          "Loudness",               USB_MIXER_BOOLEAN,     USB_MIX_TYPE_INVALID},
    {UAC2_FU_INPUT_GAIN,       "Input Gain Control",     USB_MIXER_S16,         USB_MIX_TYPE_INVALID},
    {UAC2_FU_INPUT_GAIN_PAD,   "Input Gain Pad Control", USB_MIXER_S16,         USB_MIX_TYPE_INVALID},
    {UAC2_FU_PHASE_INVERTER,   "Phase Inverter Control", USB_MIXER_BOOLEAN,     USB_MIX_TYPE_INVALID},
};

static void AudioUsbCtlGetMaxVal(struct UsbMixerElemInfo *mixElemInfo, int32_t minChannel)
{
    int32_t saved, temp, check;

    if (AudioUsbGetCurMixRaw(mixElemInfo, minChannel, &saved) < 0) {
        mixElemInfo->initialized = 1; /* 1 is initialized on */
        return;
    }
    // No endless loop
    for (;;) {
        temp = saved;
        if (temp < mixElemInfo->max) {
            temp = temp + mixElemInfo->res;
        } else {
            temp = temp - mixElemInfo->res;
        }

        if (temp < mixElemInfo->min || temp > mixElemInfo->max) {
            break;
        }
        if (AudioUsbSetCurMixValue(mixElemInfo, minChannel, 0, temp) != HDF_SUCCESS) {
            break;
        }
        if (AudioUsbGetCurMixRaw(mixElemInfo, minChannel, &check) != HDF_SUCCESS) {
            break;
        }
        if (temp == check) {
            break;
        }
        mixElemInfo->res *= VOLUME_RANGE;
    }
    (void)AudioUsbSetCurMixValue(mixElemInfo, minChannel, 0, saved);
}

static int32_t AudioUsbCtlGetMinMaxValSub(struct UsbMixerElemInfo *mixElemInfo)
{
    int32_t minChannel = 0;
    int32_t i;
    int32_t request = mixElemInfo->control << USB_SHIFT_SIZE_8;
    if (mixElemInfo->cmask) {
        for (i = 0; i < MAX_CHANNELS; i++) {
            if (mixElemInfo->cmask & (1 << i)) {
                minChannel = i + 1;
                break;
            }
        }
    }
    if (AudioUsbGetCtlValue(mixElemInfo, UAC_GET_MAX, request, &mixElemInfo->max) < HDF_SUCCESS ||
        AudioUsbGetCtlValue(mixElemInfo, UAC_GET_MIN, request, &mixElemInfo->min) < HDF_SUCCESS) {
        AUDIO_DRIVER_LOG_ERR("%d:%d: cannot get min/max values for control %d (id %d)\n", mixElemInfo->head.id,
            AudioUsbMixerCtrlIntf(mixElemInfo->head.mixer), mixElemInfo->control, mixElemInfo->head.id);
        return HDF_ERR_INVALID_PARAM;
    }

    if (AudioUsbGetCtlValue(mixElemInfo, UAC_GET_RES, request, &mixElemInfo->res) < HDF_SUCCESS) {
        mixElemInfo->res = USB_MIXER_DEF_RES_VAL;
    } else {
        int32_t lastValidRes = mixElemInfo->res;
        while (mixElemInfo->res > USB_MIXER_DEF_RES_VAL) {
            if (AudioUsbMixerSetCtlValue(mixElemInfo, UAC_SET_RES, request, mixElemInfo->res / VOLUME_RANGE) <
                HDF_SUCCESS) {
                break;
            }
            mixElemInfo->res /= VOLUME_RANGE;
        }
        if (AudioUsbGetCtlValue(mixElemInfo, UAC_GET_RES, request, &mixElemInfo->res) < HDF_SUCCESS) {
            mixElemInfo->res = lastValidRes;
        }
    }
    if (mixElemInfo->res == 0) {
        mixElemInfo->res = USB_MIXER_DEF_RES_VAL;
    }

    if (mixElemInfo->min + mixElemInfo->res < mixElemInfo->max) {
        AudioUsbCtlGetMaxVal(mixElemInfo, minChannel);
    }

    return HDF_SUCCESS;
}

static int32_t AudioUsbCtlGetMinMaxVal(
    struct UsbMixerElemInfo *mixElemInfo, int32_t default_min, struct AudioKcontrol *kcontrol)
{
    /* for failsafe */
    mixElemInfo->min = default_min;
    mixElemInfo->max = mixElemInfo->min + 1;
    mixElemInfo->res = USB_MIXER_DEF_RES_VAL;
    mixElemInfo->dBMin = mixElemInfo->dBMax = 0;

    if (mixElemInfo->valType == USB_MIXER_BOOLEAN || mixElemInfo->valType == USB_MIXER_INV_BOOLEAN) {
        mixElemInfo->initialized = 1;
    } else {
        AudioUsbCtlGetMinMaxValSub(mixElemInfo);
    }

    mixElemInfo->dBMin = (AudioUsbConvertSignedValue(mixElemInfo, mixElemInfo->min) * USB_DESCRIPTIONS_CONTAIN_100DB) /
        USB_DESCRIPTIONS_CONTAIN_256DB;
    mixElemInfo->dBMax = (AudioUsbConvertSignedValue(mixElemInfo, mixElemInfo->max) * USB_DESCRIPTIONS_CONTAIN_100DB) /
        USB_DESCRIPTIONS_CONTAIN_256DB;
    if (mixElemInfo->dBMin > mixElemInfo->dBMax) {
        /* something is wrong; assume it's either from/to 0dB */
        if (mixElemInfo->dBMin < 0) {
            mixElemInfo->dBMax = 0;
        } else if (mixElemInfo->dBMin > 0) {
            mixElemInfo->dBMin = 0;
        }
        if (mixElemInfo->dBMin > mixElemInfo->dBMax) {
            /* totally crap, return an error */
            AUDIO_DRIVER_LOG_ERR("mixElemInfo is invalid.");
            OsalMemFree(mixElemInfo);
            return HDF_FAILURE;
        }
    } else {
        /* if the max volume is too low, it's likely a bogus range;
         * here we use -96dB as the threshold
         */
        if (mixElemInfo->dBMax <= -USB_DESCRIPTIONS_CONTAIN_9600DB) {
            AUDIO_DRIVER_LOG_INFO("%d:%d: bogus dB values (%d/%d), disabling dB reporting\n", mixElemInfo->head.id,
                AudioUsbMixerCtrlIntf(mixElemInfo->head.mixer), mixElemInfo->dBMin, mixElemInfo->dBMax);
            mixElemInfo->dBMin = mixElemInfo->dBMax = 0;
        }
    }

    return HDF_SUCCESS;
}

/* get a feature/mixer unit info */
static int32_t AudioUsbMixerCtlFeatureInfo(const struct AudioKcontrol *kcontrol, struct AudioCtrlElemInfo *ctrlElemInfo)
{
    struct UsbMixerElemInfo *mixElemInfo = (struct UsbMixerElemInfo *)((volatile uintptr_t)kcontrol->privateValue);
    AUDIO_DEVICE_LOG_INFO("entry.");

    if (mixElemInfo->valType == USB_MIXER_BOOLEAN || mixElemInfo->valType == USB_MIXER_INV_BOOLEAN) {
        ctrlElemInfo->type = AUDIO_USB_CTL_ELEM_TYPE_BOOLEAN;
    } else {
        ctrlElemInfo->type = AUDIO_USB_CTL_ELEM_TYPE_INTEGER;
    }
    ctrlElemInfo->count = mixElemInfo->channels;
    if (mixElemInfo->valType == USB_MIXER_BOOLEAN || mixElemInfo->valType == USB_MIXER_INV_BOOLEAN) {
        ctrlElemInfo->min = 0;
        ctrlElemInfo->max = 1;
    } else {
        ctrlElemInfo->min = 0;
        ctrlElemInfo->max = (mixElemInfo->max - mixElemInfo->min + mixElemInfo->res - 1) / mixElemInfo->res;
    }
    return HDF_SUCCESS;
}

/* get the current value from feature/mixer unit */
static int32_t AudioUsbMixerCtlFeatureGet(
    const struct AudioKcontrol *kcontrol, struct AudioCtrlElemValue *ctrlElemValue)
{
    struct UsbMixerElemInfo *mixElemInfo = (struct UsbMixerElemInfo *)((volatile uintptr_t)kcontrol->privateValue);
    int32_t value, err;
    AUDIO_DEVICE_LOG_INFO("entry");

    ctrlElemValue->value[0] = mixElemInfo->min;
    ctrlElemValue->value[1] = mixElemInfo->max;

    err = AudioUsbGetCurMixValue(mixElemInfo, DEFAULT_CHANNEL, DEFAULT_CHANNEL, &value);
    if (err < HDF_SUCCESS) {
        return AudioUsbFilterError(mixElemInfo, err);
    }
    value = AudioUsbGetRelativeValue(mixElemInfo, value);
    ctrlElemValue->value[0] = value;

    return HDF_SUCCESS;
}

/* get the current value from feature/mixer unit */
static int32_t AudioUsbMixerCtlFeatureMultGet(
    const struct AudioKcontrol *kcontrol, struct AudioCtrlElemValue *ctrlElemValue)
{
    struct UsbMixerElemInfo *mixElemInfo = (struct UsbMixerElemInfo *)((volatile uintptr_t)kcontrol->privateValue);
    int32_t value, err;
    ctrlElemValue->value[0] = mixElemInfo->min;
    ctrlElemValue->value[1] = mixElemInfo->max;

    err = AudioUsbGetCurMixValue(mixElemInfo, DEFAULT_CHANNEL, DEFAULT_CHANNEL, &value);
    if (err < HDF_SUCCESS) {
        return AudioUsbFilterError(mixElemInfo, err);
    }
    value = AudioUsbGetRelativeValue(mixElemInfo, value);
    ctrlElemValue->value[0] = value > 0 ? false : true;

    return HDF_SUCCESS;
}

/* put the current value to feature/mixer unit */
static int32_t AudioUsbmixerCtlFeatureSet(
    const struct AudioKcontrol *kcontrol, const struct AudioCtrlElemValue *ctrlElemValue)
{
    struct UsbMixerElemInfo *mixElemInfo = (struct UsbMixerElemInfo *)((volatile uintptr_t)kcontrol->privateValue);
    int32_t value, oval, err;

    err = AudioUsbGetCurMixValue(mixElemInfo, DEFAULT_CHANNEL, DEFAULT_CHANNEL, &oval);
    if (err < HDF_SUCCESS) {
        return AudioUsbFilterError(mixElemInfo, err);
    }
    value = ctrlElemValue->value[0];
    value = AudioUsbGetAbsValue(mixElemInfo, value);
    if (value != oval) {
        AudioUsbSetCurMixValue(mixElemInfo, DEFAULT_CHANNEL, DEFAULT_CHANNEL, value);
    }

    return HDF_SUCCESS;
}

/* put the current value to feature/mixer unit */
static int32_t AudioUsbMixerCtlFeatureMultSet(
    const struct AudioKcontrol *kcontrol, const struct AudioCtrlElemValue *ctrlElemValue)
{
    struct UsbMixerElemInfo *mixElemInfo = (struct UsbMixerElemInfo *)((volatile uintptr_t)kcontrol->privateValue);
    int32_t value, oval, err;

    err = AudioUsbGetCurMixValue(mixElemInfo, DEFAULT_CHANNEL, DEFAULT_CHANNEL, &oval);
    if (err < HDF_SUCCESS) {
        return AudioUsbFilterError(mixElemInfo, err);
    }

    value = AudioUsbGetAbsValue(mixElemInfo, ctrlElemValue->value[0] > DEFAULT_CHANNEL ? false : true);
    if (value != oval) {
        AudioUsbSetCurMixValue(mixElemInfo, DEFAULT_CHANNEL, DEFAULT_CHANNEL, value);
    }

    return HDF_SUCCESS;
}

static struct AudioKcontrol g_usbFeatureUnitCtl = {
    .iface = AUDIO_USB_CTL_ELEM_IFACE_MIXER,
    .name = "", /* will be filled later manually */
    .Info = AudioUsbMixerCtlFeatureInfo,
    .Get = AudioUsbMixerCtlFeatureGet,
    .Set = AudioUsbmixerCtlFeatureSet,
};

static struct AudioKcontrol g_usbFeatureUnitCtlMult = {
    .iface = AUDIO_USB_CTL_ELEM_IFACE_MIXER,
    .name = "", /* will be filled later manually */
    .Info = AudioUsbMixerCtlFeatureInfo,
    .Get = AudioUsbMixerCtlFeatureMultGet,
    .Set = AudioUsbMixerCtlFeatureMultSet,
};

/* the read-only variant */
static struct AudioKcontrol g_usbFeatureUnitCtlRo = {
    .iface = AUDIO_USB_CTL_ELEM_IFACE_MIXER,
    .name = "", /* will be filled later manually */
    .Info = AudioUsbMixerCtlFeatureInfo,
    .Get = AudioUsbMixerCtlFeatureGet,
    .Set = NULL,
};

/* build a feature control */
static size_t AudioUsbAppendCtlName(struct AudioKcontrol *kcontrol, const char *str, int32_t size)
{
    return strlcat(kcontrol->name, str, size);
}

static const struct AudioUsbFeatureControlInfo *AudioUsbGetFeatureControlInfo(int32_t control)
{
    int32_t i;

    for (i = 0; i < ARRAY_SIZE(g_audioFeatureInfo); ++i) {
        if (g_audioFeatureInfo[i].control == control) {
            return &g_audioFeatureInfo[i];
        }
    }
    return NULL;
}

static int32_t AudioUsbFeatureCtlInit(struct UsbMixerInterface *mixer, struct AudioKcontrol **kcontrol,
    struct UsbMixerElemInfo *mixElemInfo, struct AudioUsbFeatureControl *featureControl)
{
    const struct AudioUsbFeatureControlInfo *ctlInfo = NULL;
    int32_t i;
    int32_t channel = 0;

    AudioUsbMixerElemInitStd(&mixElemInfo->head, mixer, featureControl->unitId);
    mixElemInfo->control = featureControl->control;
    mixElemInfo->cmask = featureControl->ctlMask;

    ctlInfo = AudioUsbGetFeatureControlInfo(featureControl->control);
    if (ctlInfo == NULL) {
        OsalMemFree(mixElemInfo);
        AUDIO_DEVICE_LOG_ERR("AudioUsbGetFeatureControlInfo failed.");
        return HDF_FAILURE;
    }

    if (mixer->protocol == UAC_VERSION_1) {
        mixElemInfo->valType = ctlInfo->type;
    } else { /* UAC_VERSION_2 */
        mixElemInfo->valType = ctlInfo->typeUac2 >= 0 ? ctlInfo->typeUac2 : ctlInfo->type;
    }
    if (featureControl->ctlMask == 0) {
        mixElemInfo->channels = 1; /* 1 is first channel */
        mixElemInfo->masterReadOnly = featureControl->readOnlyMask;
    } else {
        for (i = 0; i < USB_SHIFT_SIZE_16; i++) {
            if (featureControl->ctlMask & (1 << i)) {
                channel++;
            }
        }
        mixElemInfo->channels = channel;
        mixElemInfo->chReadOnly = featureControl->readOnlyMask;
    }

    if (mixElemInfo->channels == featureControl->readOnlyMask) {
        *kcontrol = &g_usbFeatureUnitCtlRo;
    } else {
        *kcontrol = &g_usbFeatureUnitCtl;
    }

    if (featureControl->control == UAC_FU_MUTE) {
        *kcontrol = &g_usbFeatureUnitCtlMult;
    }
    DListHeadInit(&((*kcontrol)->list));
    return HDF_SUCCESS;
}

static void AudioUsbSetVolumeItemName(struct UsbMixerBuild *state, struct AudioKcontrol *kcontrol,
    struct UsbAudioTerm *usbAudioTerm, struct AudioUsbFeatureControl *featureControl, const struct UsbMixerNameMap *map)
{
    struct UsbMixerInterface *mixer = state->mixer;
    struct UsbAudioTerm *oterm = &state->oterm;
    uint32_t len = 0;
    int32_t mappedName = 0;

    len = AudioUsbCheckMappedName(map, kcontrol->name, KCTL_NAME_LEN);
    mappedName = len != 0;
    if (len == 0 && featureControl->nameId != 0) {
        len = AudioUsbCopyStringDesc(mixer->audioUsbDriver, featureControl->nameId, kcontrol->name, KCTL_NAME_LEN);
    }

    if (len == 0) {
        if (usbAudioTerm != NULL) {
            len = AudioUsbGetTermName(mixer->audioUsbDriver, usbAudioTerm, kcontrol->name, KCTL_NAME_LEN, 1);
        }
        if (len == 0 && oterm != NULL) {
            len = AudioUsbGetTermName(mixer->audioUsbDriver, oterm, kcontrol->name, KCTL_NAME_LEN, 1);
        }
        if (len == 0) {
            (void)snprintf_s(kcontrol->name, KCTL_NAME_LEN + 1, KCTL_NAME_LEN, "Feature %d", featureControl->unitId);
        }
    }
    if (mappedName == 0 && oterm != NULL && !(oterm->type >> USB_SHIFT_SIZE_16)) {
        if ((oterm->type & 0xff00) == PCM_TYPE_VAL) {
            AudioUsbAppendCtlName(kcontrol, " Capture", KCTL_NAME_LEN);
        } else {
            AudioUsbAppendCtlName(kcontrol, " Playback", KCTL_NAME_LEN);
        }
    }
    AudioUsbAppendCtlName(kcontrol, featureControl->control == UAC_FU_MUTE ? " Mute" : " Volume", KCTL_NAME_LEN);
}

static int32_t AudioUsbSetKctlItermName(struct UsbMixerBuild *state, struct AudioKcontrol *kcontrol,
    struct UsbAudioTerm *usbAudioTerm, struct AudioUsbFeatureControl *featureControl)
{
    uint32_t len = 0;
    bool mappedName = 0;
    struct UsbMixerInterface *mixer = state->mixer;
    const struct UsbMixerNameMap *imap = state->map;
    struct UsbAudioTerm *oterm = &state->oterm;
    const struct UsbMixerNameMap *map = NULL;

    map = AudioUsbFindMap(imap, featureControl->unitId, featureControl->control);
    if (!(map == NULL || map->name != NULL || map->dB != NULL)) {
        AUDIO_DRIVER_LOG_ERR("param is null");
        return HDF_FAILURE;
    }

    len = AudioUsbCheckMappedName(map, kcontrol->name, KCTL_NAME_LEN);
    mappedName = len != 0;
    if (len == 0 && featureControl->nameId != 0) {
        len = AudioUsbCopyStringDesc(mixer->audioUsbDriver, featureControl->nameId, kcontrol->name, KCTL_NAME_LEN);
    }

    switch (featureControl->control) {
        case UAC_FU_VOLUME:
            AudioUsbSetVolumeItemName(state, kcontrol, usbAudioTerm, featureControl, map);
            break;
        case UAC_FU_MUTE:
            if (!mappedName && oterm != NULL && !(oterm->type >> USB_SHIFT_SIZE_16)) {
                if ((oterm->type & 0xff00) == PCM_TYPE_VAL) {
                    AudioUsbAppendCtlName(kcontrol, "Capture", KCTL_NAME_LEN);
                } else {
                    AudioUsbAppendCtlName(kcontrol, "Playback", KCTL_NAME_LEN);
                }
            }
            AudioUsbAppendCtlName(
                kcontrol, featureControl->control == UAC_FU_MUTE ? " Mute" : " Volume", KCTL_NAME_LEN);
            break;
        default:
            if (len == 0) {
                strlcpy(kcontrol->name, g_audioFeatureInfo[featureControl->control - 1].name, KCTL_NAME_LEN);
            }
            break;
    }
    return HDF_SUCCESS;
}

static void AudioUsbBuildFeatureCtlSub(
    struct UsbMixerBuild *state, struct UsbAudioTerm *usbAudioTerm, struct AudioUsbFeatureControl *featureControl)
{
    struct AudioKcontrol *kcontrol = NULL;
    struct AudioKcontrol *kControl = NULL;
    struct UsbMixerElemInfo *mixElemInfo = NULL;
    struct AudioCard *audioCard = NULL;
    int ret;

    if (featureControl->control == UAC_FU_GRAPHIC_EQUALIZER) {
        AUDIO_DEVICE_LOG_INFO("not supported yet");
        return;
    }

    mixElemInfo = OsalMemCalloc(sizeof(*mixElemInfo));
    if (!mixElemInfo) {
        AUDIO_DEVICE_LOG_ERR("OsalMemCalloc is failed");
        return;
    }

    ret = AudioUsbFeatureCtlInit(state->mixer, &kcontrol, mixElemInfo, featureControl);
    if (ret != HDF_SUCCESS || kcontrol == NULL) {
        AUDIO_DEVICE_LOG_INFO("AudioUsbFeatureCtlInit fail or kcontrol  is null");
        OsalMemFree(mixElemInfo);
        return;
    }

    kcontrol->name = (char *)OsalMemCalloc(KCTL_NAME_LEN);
    ret = AudioUsbSetKctlItermName(state, kcontrol, usbAudioTerm, featureControl);
    if (ret != HDF_SUCCESS) {
        OsalMemFree(mixElemInfo);
        OsalMemFree(kcontrol->name);
        return;
    }

    /* get min/max values */
    ret = AudioUsbCtlGetMinMaxVal(mixElemInfo, 0, kcontrol);
    if (ret != HDF_SUCCESS || mixElemInfo->max <= mixElemInfo->min) {
        AUDIO_DEVICE_LOG_INFO("AudioUsbCtlGetMinMaxVal fail or mixElemInfo->min error");
        OsalMemFree(mixElemInfo);
        OsalMemFree(kcontrol->name);
        return;
    }

    audioCard = state->mixer->audioUsbDriver->audioCard;
    kcontrol->privateValue = (unsigned long)(uintptr_t)(void *)mixElemInfo;

    kControl = AudioAddControl(audioCard, kcontrol);
    if (kControl == NULL) {
        OsalMemFree(mixElemInfo);
        OsalMemFree(kcontrol->name);
        return;
    }

    DListInsertHead(&kControl->list, &audioCard->controls);
}

static void AudioUsbBuildFeatureCtl(struct UsbMixerBuild *state, void *rawDesc, struct UsbAudioTerm *usbAudioTerm,
    struct AudioUsbFeatureControl *featureControl)
{
    struct uac_feature_unit_descriptor *desc = rawDesc;
    featureControl->nameId = uac_feature_unit_iFeature(desc);

    AudioUsbBuildFeatureCtlSub(state, usbAudioTerm, featureControl);
}

static int32_t AudioUsbParseFeatureDescriptorParam(
    struct UsbMixerBuild *state, void *ftrSub, struct AudioUsbFeatureParam *featureParam)
{
    struct uac_feature_unit_descriptor *hdr = ftrSub;

    if (state->mixer->protocol == UAC_VERSION_1) {
        featureParam->controlSize = hdr->bControlSize;
        featureParam->channels = (hdr->bLength - CHANNEL_HDR_BLENGTH) / featureParam->controlSize - 1;
        featureParam->bmaControls = hdr->bmaControls;
    } else { /* UAC_VERSION_3 */
        AUDIO_DEVICE_LOG_ERR("not support!");
        return HDF_FAILURE;
    }
    featureParam->masterBits = AudioUsbCombineBytes(featureParam->bmaControls, featureParam->controlSize);
    switch (state->audioUsbDriver->usbId) {
        case 0x08bb2702: /* PCM2702 */
            featureParam->masterBits &= ~UAC_CONTROL_BIT(UAC_FU_VOLUME);
            break;
        case 0x1130f211: /* TP6911 */
            featureParam->channels = 0;
            break;
        default:
            AUDIO_DEVICE_LOG_ERR("usbID err!");
            return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

/* parse a feature unit; most of controls are defined here. */
static int32_t AudioUsbParseFeatureUnit(struct UsbMixerBuild *state, int32_t unitId, void *ftrSub)
{
    struct UsbAudioTerm usbAudioTerm;
    int32_t i, j, err;
    struct uac_feature_unit_descriptor *hdr = ftrSub;
    struct AudioUsbFeatureControl featureControl;
    struct AudioUsbFeatureParam featureParam;
    uint32_t chBits, mask;

    err = AudioUsbParseFeatureDescriptorParam(state, ftrSub, &featureParam);
    if (err < HDF_SUCCESS) {
        return err;
    }

    featureControl.unitId = unitId;
    /* parse the source unit */
    err = AudioUsbParseUnit(state, hdr->bSourceID);
    if (err < HDF_SUCCESS) {
        return err;
    }

    err = AudioUsbCheckInputTerm(state, hdr->bSourceID, &usbAudioTerm);
    if (err < HDF_SUCCESS) {
        return err;
    }

    for (i = 0; i < USB_TIMEOUT; i++) {
        chBits = 0;
        featureControl.control = g_audioFeatureInfo[i].control;
        for (j = 0; j < featureParam.channels; j++) {
            mask = AudioUsbCombineBytes(
                featureParam.bmaControls + featureParam.controlSize * (j + 1), featureParam.controlSize);
            if (mask & (1 << i)) {
                chBits |= (1 << j);
            }
        }
        featureControl.readOnlyMask = 0;
        if (chBits & 1) {
            featureControl.ctlMask = chBits;
            AudioUsbBuildFeatureCtl(state, ftrSub, &usbAudioTerm, &featureControl);
        }
        if (featureParam.masterBits & (1 << i)) {
            featureControl.ctlMask = 0;
            AudioUsbBuildFeatureCtl(state, ftrSub, &usbAudioTerm, &featureControl);
        }
    }

    return HDF_SUCCESS;
}

static bool AudioUsbMixerBitmapOverflow(
    struct uac_mixer_unit_descriptor *desc, int32_t protocol, int32_t numIns, int32_t numOuts)
{
    uint8_t *hdr = (uint8_t *)desc;
    uint8_t *controls = uac_mixer_unit_bmControls(desc, protocol);
    size_t restVal;
    uint32_t numVal, ctlTem, hdrTem;

    switch (protocol) {
        case UAC_VERSION_1:
        default:
            restVal = 1; /* 1 is iMixer */
            break;
        case UAC_VERSION_2:
            restVal = 2; /* 2 is bmControls + iMixer */
            break;
        case UAC_VERSION_3:
            restVal = 6; /* 6 is bmControls + wMixerDescrStr */
            break;
    }
    numVal = (numIns * numOuts + CHANNEL_HDR_BLENGTH) / OVERF_LOWDIVISOR_NUM;
    ctlTem = (uintptr_t)(controls + numVal + restVal);
    hdrTem = (uintptr_t)(hdr + hdr[0]);
    return ctlTem > hdrTem;
}

static void AudioUsbBuildMixerUnitCtl(struct UsbMixerBuild *state, struct uac_mixer_unit_descriptor *desc,
    struct MixerUnitCtlParam *mixCtlParam, int32_t unitId, struct UsbAudioTerm *usbAudioTerm)
{
    struct UsbMixerElemInfo *mixElemInfo = NULL;
    uint32_t i;
    struct AudioKcontrol *kcontrol = NULL;
    uint8_t *controls = NULL;
    int32_t len;
    const struct UsbMixerNameMap *map = NULL;
    struct AudioCard *audioCard = NULL;

    map = AudioUsbFindMap(state->map, unitId, 0);
    if (!(map == NULL || map->name != NULL || map->dB != NULL)) {
        AUDIO_DRIVER_LOG_ERR("param is null");
        return;
    }

    mixElemInfo = OsalMemCalloc(sizeof(*mixElemInfo));
    if (mixElemInfo == NULL) {
        AUDIO_DEVICE_LOG_ERR("OsalMemCalloc failed");
        return;
    }
    AudioUsbMixerElemInitStd(&mixElemInfo->head, state->mixer, unitId);
    mixElemInfo->control = mixCtlParam->itemChannels + 1; /* based on 1 */
    mixElemInfo->valType = USB_MIXER_S16;
    for (i = 0; i < mixCtlParam->numOuts; i++) {
        controls = uac_mixer_unit_bmControls(desc, state->mixer->protocol);
        if (controls != NULL &&
            AudioUsbCheckMatrixBitmap(controls, mixCtlParam->itemChannels, i, mixCtlParam->numOuts)) {
            mixElemInfo->cmask |= (1 << i);
            mixElemInfo->channels++;
        }
    }

    /* get min/max values */
    AudioUsbCtlGetMinMaxVal(mixElemInfo, 0, NULL);

    kcontrol = &g_usbFeatureUnitCtl;
    kcontrol->name = (char *)OsalMemCalloc(KCTL_NAME_LEN);
    kcontrol->privateValue = (unsigned long)(uintptr_t)(void *)mixElemInfo;
    DListHeadInit(&kcontrol->list);

    len = AudioUsbCheckMappedName(map, kcontrol->name, KCTL_NAME_LEN);
    if (len == 0 && AudioUsbGetTermName(state->audioUsbDriver, usbAudioTerm, kcontrol->name, KCTL_NAME_LEN, 0) == 0) {
        (void)sprintf_s(kcontrol->name, KCTL_NAME_LEN, "Mixer Source %d", mixCtlParam->itemChannels + 1);
    }

    AudioUsbAppendCtlName(kcontrol, " Volume", KCTL_NAME_LEN);
    audioCard = state->audioUsbDriver->audioCard;

    if (AudioAddControl(audioCard, kcontrol) == NULL) {
        OsalMemFree(mixElemInfo);
        OsalMemFree(kcontrol->name);
        return;
    }
    DListInsertHead(&kcontrol->list, &audioCard->controls);
}

static bool AudioUsbParseMixerUnitSub(int32_t itemChannels, struct UsbMixerBuild *state,
    struct MixerUnitCtlParam mixCtlParam, struct uac_mixer_unit_descriptor *desc)
{
    int32_t outChannel;
    uint8_t *control = NULL;

    for (outChannel = 0; outChannel < mixCtlParam.numOuts; outChannel++) {
        control = uac_mixer_unit_bmControls(desc, state->mixer->protocol);
        if (control != NULL && AudioUsbCheckMatrixBitmap(control, itemChannels, outChannel, mixCtlParam.numOuts)) {
            return true;
        }
    }

    return false;
}

static int32_t AudioUsbParseMixerUnit(struct UsbMixerBuild *state, int32_t unitId, void *rawDesc)
{
    struct uac_mixer_unit_descriptor *desc = rawDesc;
    struct UsbAudioTerm usbAudioTerm;
    int32_t inputPins, pin, err;
    int32_t numIns = 0;
    int32_t itemChannels = 0;
    struct MixerUnitCtlParam mixCtlParam = {0};

    err = AudioUsbMixerUnitGetChannels(state, desc);
    if (err < 0) {
        AUDIO_DEVICE_LOG_ERR("invalid MIXER UNIT descriptor %d\n", unitId);
        return err;
    }

    mixCtlParam.numOuts = err;
    inputPins = desc->bNrInPins;

    for (pin = 0; pin < inputPins; pin++) {
        err = AudioUsbParseUnit(state, desc->baSourceID[pin]);
        if (err < HDF_SUCCESS || mixCtlParam.numOuts == 0) {
            continue;
        }

        err = AudioUsbCheckInputTerm(state, desc->baSourceID[pin], &usbAudioTerm);
        if (err < HDF_SUCCESS) {
            return err;
        }
        numIns += usbAudioTerm.channels;
        if (AudioUsbMixerBitmapOverflow(desc, state->mixer->protocol, numIns, mixCtlParam.numOuts)) {
            break;
        }
        for (; itemChannels < numIns; itemChannels++) {
            if (AudioUsbParseMixerUnitSub(itemChannels, state, mixCtlParam, desc)) {
                mixCtlParam.itemChannels = itemChannels;
                AudioUsbBuildMixerUnitCtl(state, desc, &mixCtlParam, unitId, &usbAudioTerm);
            }
        }
    }
    return HDF_SUCCESS;
}

static int32_t AudioUsbMixerCtlSelectorInfo(
    const struct AudioKcontrol *kcontrol, struct AudioCtrlElemInfo *ctrlElemInfo)
{
    (void)kcontrol;
    (void)ctrlElemInfo;
    return HDF_SUCCESS;
}

/* get callback for selector unit */
static int32_t AudioUsbMixerCtlSelectorGet(
    const struct AudioKcontrol *kcontrol, struct AudioCtrlElemValue *ctrlElemValue)
{
    struct UsbMixerElemInfo *mixElemInfo = (struct UsbMixerElemInfo *)((volatile uintptr_t)kcontrol->privateValue);
    int32_t value, err;

    err = AudioUsbGetCurCtlValue(mixElemInfo, mixElemInfo->control << USB_SHIFT_SIZE_8, &value);
    if (err < HDF_SUCCESS) {
        ctrlElemValue->value[0] = 0;
        return AudioUsbFilterError(mixElemInfo, err);
    }
    value = AudioUsbGetRelativeValue(mixElemInfo, value);
    ctrlElemValue->value[0] = value;
    return HDF_SUCCESS;
}

/* put callback for selector unit */
static int32_t AudioUsbMixerCtlSelectorSet(
    const struct AudioKcontrol *kcontrol, const struct AudioCtrlElemValue *ctrlElemValue)
{
    struct UsbMixerElemInfo *mixElemInfo = (struct UsbMixerElemInfo *)((volatile uintptr_t)kcontrol->privateValue);
    int32_t value;
    int32_t err;
    int32_t outValue = 0;

    err = AudioUsbGetCurCtlValue(mixElemInfo, mixElemInfo->control << USB_SHIFT_SIZE_8, &outValue);
    if (err < HDF_SUCCESS) {
        return AudioUsbFilterError(mixElemInfo, err);
    }
    value = ctrlElemValue->value[0];
    value = AudioUsbGetAbsValue(mixElemInfo, value);
    if (value != outValue) {
        err = AudioUsbSetCurCtlValue(mixElemInfo, mixElemInfo->control << USB_SHIFT_SIZE_8, value);
        return err;
    }
    return HDF_SUCCESS;
}

/* alsa control interface for selector unit */
static struct AudioKcontrol g_mixerSelectUnitCtl = {
    .iface = AUDIO_USB_CTL_ELEM_IFACE_MIXER,
    .name = "", /* will be filled later */
    .Info = AudioUsbMixerCtlSelectorInfo,
    .Get = AudioUsbMixerCtlSelectorGet,
    .Set = AudioUsbMixerCtlSelectorSet,
};

static int32_t AudioUsbSetTermName(
    struct UsbMixerBuild *state, void *rawDesc, struct UsbMixerElemInfo *mixElemInfo, int32_t unitId)
{
    struct uac_selector_unit_descriptor *desc = rawDesc;
    char **nameList = NULL;
    uint32_t i, len;
    struct UsbAudioTerm usbAudioTerm;

    AudioUsbMixerElemInitStd(&mixElemInfo->head, state->mixer, unitId);
    mixElemInfo->valType = USB_MIXER_U8;
    mixElemInfo->channels = USB_MIXER_DEF_CHAN;
    mixElemInfo->min = USB_MIXER_DEF_MIN_VAL;
    mixElemInfo->max = desc->bNrInPins;
    mixElemInfo->res = USB_MIXER_DEF_RES_VAL;
    mixElemInfo->initialized = true;
    mixElemInfo->control = 0;

    nameList = OsalMemCalloc(desc->bNrInPins * sizeof(char *));
    if (nameList == NULL) {
        AUDIO_DEVICE_LOG_ERR("OsalMemCalloc failed.");
        return HDF_ERR_MALLOC_FAIL;
    }
    for (i = 0; i < desc->bNrInPins; i++) {
        len = 0;
        nameList[i] = OsalMemCalloc(MAX_ITEM_NAME_LEN);
        if (nameList[i] == NULL) {
            for (i = 0; i < desc->bNrInPins; i++) {
                OsalMemFree(nameList[i]);
            }
            OsalMemFree(nameList);
            return HDF_ERR_MALLOC_FAIL;
        }
        len = AudioUsbCheckMappedSelectorName(state, unitId, i, nameList[i], MAX_ITEM_NAME_LEN);
        if (len != 0 && AudioUsbCheckInputTerm(state, desc->baSourceID[i], &usbAudioTerm) >= 0) {
            len = AudioUsbGetTermName(state->audioUsbDriver, &usbAudioTerm, nameList[i], MAX_ITEM_NAME_LEN, 0);
        }
        if (len == 0) {
            sprintf_s(nameList[i], MAX_ITEM_NAME_LEN, "Input %u", i);
        }
    }
    return HDF_SUCCESS;
}

static void AudioUsbSetCtlName(struct UsbMixerBuild *state, struct AudioKcontrol *kcontrol,
    struct uac_selector_unit_descriptor *desc, const struct UsbMixerNameMap *map)
{
    uint32_t len;
    uint32_t nameId;

    /* check the static mapping table at first */
    len = AudioUsbCheckMappedName(map, kcontrol->name, KCTL_NAME_LEN);
    if (len == 0) { /* no mapping */
        if (state->mixer->protocol != UAC_VERSION_3) {
            /* if iSelector is given, use it */
            nameId = (uint32_t)uac_selector_unit_iSelector(desc);
            if (nameId != 0) {
                len = AudioUsbCopyStringDesc(state->audioUsbDriver, nameId, kcontrol->name, KCTL_NAME_LEN);
            }
        }

        if (len == 0) { /* pick up the terminal name at next */
            len = AudioUsbGetTermName(state->audioUsbDriver, &state->oterm, kcontrol->name, KCTL_NAME_LEN, 0);
        }
        if (len == 0) { /* use the fixed string "USB" as the last resort */
            strlcpy(kcontrol->name, "USB", KCTL_NAME_LEN);
        }
        /* and add the proper suffix */
        if (desc->bDescriptorSubtype == UAC2_CLOCK_SELECTOR || desc->bDescriptorSubtype == UAC3_CLOCK_SELECTOR) {
            AudioUsbAppendCtlName(kcontrol, " Clock Source", KCTL_NAME_LEN);
        } else if ((state->oterm.type & 0xff00) == PCM_TYPE_VAL) {
            AudioUsbAppendCtlName(kcontrol, " Capture Source", KCTL_NAME_LEN);
        } else {
            AudioUsbAppendCtlName(kcontrol, " Playback Source", KCTL_NAME_LEN);
        }
    }
}

static int32_t AudioUsbParseSelectorUnit(struct UsbMixerBuild *state, int32_t unitId, void *rawDesc)
{
    struct uac_selector_unit_descriptor *desc = rawDesc;
    uint32_t i;
    int32_t err;
    struct UsbMixerElemInfo *mixElemInfo = NULL;
    struct AudioKcontrol *kcontrol = NULL;
    const struct UsbMixerNameMap *map = NULL;
    struct AudioCard *audioCard = NULL;

    for (i = 0; i < desc->bNrInPins; i++) {
        err = AudioUsbParseUnit(state, desc->baSourceID[i]);
        if (err < HDF_SUCCESS) {
            AUDIO_DEVICE_LOG_ERR("AudioUsbParseUnit is failed");
            return err;
        }
    }

    if (desc->bNrInPins == USB_BA_SOURCE_ID_SIZE) {
        return HDF_SUCCESS;
    }
    map = AudioUsbFindMap(state->map, unitId, 0);
    if (!(map == NULL || map->name != NULL || map->dB != NULL)) {
        AUDIO_DRIVER_LOG_ERR("param is null");
        return HDF_FAILURE;
    }

    mixElemInfo = OsalMemCalloc(sizeof(*mixElemInfo));
    if (mixElemInfo == NULL) {
        AUDIO_DEVICE_LOG_ERR("OsalMemCalloc failed");
        return HDF_ERR_MALLOC_FAIL;
    }

    err = AudioUsbSetTermName(state, rawDesc, mixElemInfo, unitId);
    if (err != HDF_SUCCESS) {
        OsalMemFree(mixElemInfo);
        return err;
    }

    kcontrol = &g_mixerSelectUnitCtl;
    kcontrol->name = (char *)OsalMemCalloc(KCTL_NAME_LEN);
    DListHeadInit(&kcontrol->list);

    AudioUsbSetCtlName(state, kcontrol, desc, map);
    audioCard = state->audioUsbDriver->audioCard;
    kcontrol->privateValue = (unsigned long)(uintptr_t)(void *)mixElemInfo;

    if (AudioAddControl(audioCard, kcontrol) == NULL) {
        OsalMemFree(mixElemInfo);
        OsalMemFree(kcontrol->name);
        return HDF_FAILURE;
    }

    DListInsertHead(&kcontrol->list, &audioCard->controls);
    return HDF_SUCCESS;
}

/* parse an audio unit recursively */
static int32_t AudioUsbParseUnit(struct UsbMixerBuild *state, int unitId)
{
    unsigned char *unitDesc = NULL;
    uint32_t type;
    int protocol = state->mixer->protocol;

    AUDIO_DEVICE_LOG_INFO("entry");
    if (test_and_set_bit(unitId, state->unitBitMap)) {
        return HDF_SUCCESS; /* the unit already visited */
    }
    unitDesc = AudioUsbFindAudioControlUnit(state, unitId);
    if (unitDesc == NULL) {
        AUDIO_DRIVER_LOG_ERR("unit %d not found!\n", unitId);
        return HDF_ERR_INVALID_PARAM;
    }

    if (!AudioUsbValidateAudioDesc(unitDesc, protocol)) {
        AUDIO_DRIVER_LOG_DEBUG("invalid unit %d\n", unitId);
        return HDF_SUCCESS; /* skip invalid unit */
    }

    // here use version and UAC interface descriptor subtypes to determine the parsing function.
    type = (protocol << USB_SHIFT_SIZE_8) | unitDesc[USB_VAL_LEN_2];
    switch (type) {
        case 0x0002: // UAC_VERSION_1, UAC_INPUT_TERMINAL
        case 0x2002: // UAC_VERSION_2, UAC_INPUT_TERMINAL
            return HDF_SUCCESS;
        case 0x0004: // UAC_VERSION_1, UAC_MIXER_UNIT
        case 0x2004: // UAC_VERSION_2, UAC_MIXER_UNIT
            return AudioUsbParseMixerUnit(state, unitId, unitDesc);
        case 0x0005: // UAC_VERSION_1, UAC_SELECTOR_UNIT
        case 0x2005: // UAC_VERSION_2, UAC_SELECTOR_UNIT
        case 0x200b: // UAC_VERSION_2, UAC2_CLOCK_SELECTOR
            return AudioUsbParseSelectorUnit(state, unitId, unitDesc);
        case 0x0006: // UAC_VERSION_1, UAC_FEATURE_UNIT
        case 0x2006: // UAC_VERSION_2, UAC_FEATURE_UNIT
            return AudioUsbParseFeatureUnit(state, unitId, unitDesc);
        case 0x0007: // UAC_VERSION_1, UAC1_PROCESSING_UNIT
        case 0x2008: // UAC_VERSION_2, UAC2_PROCESSING_UNIT_V2
        case 0x0008: // UAC_VERSION_1, UAC1_EXTENSION_UNIT
        case 0x2009: // UAC_VERSION_2, UAC2_EXTENSION_UNIT_V2
        case 0x2007: // UAC_VERSION_2, UAC2_EFFECT_UNIT
        case 0x200a: // UAC_VERSION_3, UAC3_EXTENSION_UNIT
            return HDF_SUCCESS;
        default:
            AUDIO_DRIVER_LOG_ERR("unit %u: unexpected type 0x%02x\n", unitId, unitDesc[ARRAY_INDEX_2]);
            return HDF_ERR_INVALID_PARAM;
    }
}

void AudioUsbMixerDisconnect(struct UsbMixerInterface *mixer)
{
    if (mixer->disconnected) {
        AUDIO_DEVICE_LOG_WARNING("mixer is disconnected");
        return;
    }
    if (mixer->urb != NULL) {
        usb_kill_urb(mixer->urb);
    }
    if (mixer->rcUrb != NULL) {
        usb_kill_urb(mixer->rcUrb);
    }
    if (mixer->PrivateFree != NULL) {
        mixer->PrivateFree(mixer);
    }
    mixer->disconnected = true;
}

static void AudioUsbMixerFree(struct UsbMixerInterface *mixer)
{
    /* kill pending URBs */
    AudioUsbMixerDisconnect(mixer);

    OsalMemFree(mixer->idElems);
    if (mixer->urb != NULL) {
        kfree(mixer->urb->transfer_buffer);
        usb_free_urb(mixer->urb);
    }
    usb_free_urb(mixer->rcUrb);
    OsalMemFree(mixer->rcSetupPacket);
    OsalMemFree(mixer);
}

static int32_t AudioUsbMixerControls(struct UsbMixerInterface *mixer)
{
    struct UsbMixerBuild state;
    int32_t err;
    void *pAfter = NULL;
    struct uac1_output_terminal_descriptor *desc = NULL;

    if (mixer == NULL || mixer->hostIf == NULL) {
        AUDIO_DEVICE_LOG_ERR("mixer or mixer->hostIf is null");
        return HDF_ERR_INVALID_PARAM;
    }
    (void)memset_s(&state, sizeof(state), 0, sizeof(state));
    state.audioUsbDriver = mixer->audioUsbDriver;
    state.mixer = mixer;
    state.buffer = mixer->hostIf->extra;
    state.bufLen = mixer->hostIf->extralen;

    pAfter = AudioUsbFindCsintDesc(mixer->hostIf->extra, mixer->hostIf->extralen, pAfter, UAC_OUTPUT_TERMINAL);
    while (pAfter != NULL) {
        if (!AudioUsbValidateAudioDesc(pAfter, mixer->protocol)) {
            pAfter = AudioUsbFindCsintDesc(mixer->hostIf->extra, mixer->hostIf->extralen, pAfter, UAC_OUTPUT_TERMINAL);
            continue;
        }
        if (mixer->protocol == UAC_VERSION_1) {
            desc = pAfter;
            set_bit(desc->bTerminalID, state.unitBitMap);
            state.oterm.name = desc->iTerminal;
            state.oterm.id = desc->bTerminalID;
            state.oterm.type = le16_to_cpu(desc->wTerminalType);
            AUDIO_DEVICE_LOG_DEBUG("name = %d id = %d type = %d", state.oterm.name, state.oterm.id, state.oterm.type);

            err = AudioUsbParseUnit(&state, desc->bSourceID);
            if (err < HDF_SUCCESS && err != -EINVAL) {
                AUDIO_DEVICE_LOG_ERR("AudioUsbParseUnit is failed");
                return err;
            }
        }
        pAfter = AudioUsbFindCsintDesc(mixer->hostIf->extra, mixer->hostIf->extralen, pAfter, UAC_OUTPUT_TERMINAL);
    }

    return HDF_SUCCESS;
}

static int32_t AudioUsbChoiceUsbVersion(struct UsbMixerInterface *mixer)
{
    struct usb_interface_descriptor *inteDesc = AudioUsbGetIfaceDesc(mixer->hostIf);
    if (inteDesc == NULL) {
        AUDIO_DEVICE_LOG_ERR("inteDesc is null");
        return HDF_FAILURE;
    }

    switch (inteDesc->bInterfaceProtocol) {
        case UAC_VERSION_1:
            mixer->protocol = UAC_VERSION_1;
            break;
        case UAC_VERSION_2:
            mixer->protocol = UAC_VERSION_2;
            break;
        case UAC_VERSION_3:
            mixer->protocol = UAC_VERSION_3;
            break;
        default:
            mixer->protocol = UAC_VERSION_1;
            break;
    }
    AUDIO_DEVICE_LOG_DEBUG("mixer protocol is %d", mixer->protocol);

    return HDF_SUCCESS;
}

int32_t AudioUsbCreateMixer(struct AudioUsbDriver *audioUsbDriver, int32_t ctrlIf, int32_t ignoreError)
{
    struct UsbMixerInterface *mixer = NULL;

    AUDIO_DEVICE_LOG_INFO("entry");
    if (audioUsbDriver == NULL) {
        AUDIO_DEVICE_LOG_ERR("audioUsbDriver is null");
        return HDF_ERR_INVALID_PARAM;
    }
    mixer = OsalMemCalloc(sizeof(*mixer));
    if (mixer == NULL) {
        AUDIO_DEVICE_LOG_ERR("OsalMemCalloc mixer failed");
        return HDF_ERR_MALLOC_FAIL;
    }
    mixer->audioUsbDriver = audioUsbDriver;
    mixer->ignoreCtlError = ignoreError;
    mixer->idElems = OsalMemCalloc(sizeof(struct UsbMixerElemList) * MAX_ID_ELEMS);
    if (mixer->idElems == NULL) {
        AUDIO_DEVICE_LOG_ERR("OsalMemCalloc mixer->idElems failed");
        OsalMemFree(mixer);
        return HDF_ERR_MALLOC_FAIL;
    }
    mixer->hostIf = &usb_ifnum_to_if(audioUsbDriver->dev, ctrlIf)->altsetting[0];
    if (mixer->hostIf == NULL) {
        AUDIO_DEVICE_LOG_ERR("mixer->hostIf is null");
        AudioUsbMixerFree(mixer);
        return HDF_FAILURE;
    }

    if (AudioUsbChoiceUsbVersion(mixer) != HDF_SUCCESS) {
        AUDIO_DEVICE_LOG_ERR("AudioUsbChoiceUsbVersion fail");
        AudioUsbMixerFree(mixer);
        return HDF_FAILURE;
    }

    if (mixer->protocol != UAC_VERSION_3 && AudioUsbMixerControls(mixer) < 0) {
        AUDIO_DEVICE_LOG_ERR("AudioUsbMixerControls fail");
        AudioUsbMixerFree(mixer);
        return HDF_FAILURE;
    }

    return HDF_SUCCESS;
}
