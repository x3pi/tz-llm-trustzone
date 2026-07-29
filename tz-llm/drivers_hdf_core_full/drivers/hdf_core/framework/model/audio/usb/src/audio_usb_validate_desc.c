/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include "audio_usb_validate_desc.h"
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/usb.h>
#include <linux/usb/audio-v2.h>
#include <linux/usb/audio-v3.h>
#include <linux/usb/audio.h>

#include "audio_driver_log.h"
#include "hdf_base.h"
#include "osal_mem.h"
#include "securec.h"

#define HDF_LOG_TAG HDF_AUDIO_USB

#define UAC_VERSION_ALL      255
#define AUDIO_USB_DESC_LEN_1 1
#define AUDIO_USB_DESC_LEN_2 2
#define AUDIO_USB_DESC_LEN_3 3
#define AUDIO_USB_DESC_LEN_4 4

#define AUDIO_USB_INTERVAL_LEN_1 1
#define AUDIO_USB_INTERVAL_LEN_4 4

#define AUDIO_USB_INDEX_1 1
#define AUDIO_USB_INDEX_2 2
#define AUDIO_USB_INDEX_3 3

#define AUDIO_USB_COMBINE_BYTE   1
#define AUDIO_USB_COMBINE_WORD   2
#define AUDIO_USB_COMBINE_TRIPLE 3
#define AUDIO_USB_COMBINE_QUAD   4

#define USB_SHIFT_SIZE_8  8
#define USB_SHIFT_SIZE_16 16
#define USB_SHIFT_SIZE_24 24

struct UsbDescValidator {
    uint8_t protocol;
    uint8_t type;
    bool (*func)(const void *ptr, const struct UsbDescValidator *usbDesc);
    size_t size;
};

struct usb_interface_descriptor *AudioUsbGetIfaceDesc(struct usb_host_interface *interface)
{
    if (interface == NULL) {
        AUDIO_DRIVER_LOG_ERR("input param is NULL.");
        return NULL;
    }
    return (&(interface)->desc);
}

struct usb_endpoint_descriptor *AudioEndpointDescriptor(struct usb_host_interface *alts, uint32_t epIndex)
{
    if (alts == NULL) {
        AUDIO_DRIVER_LOG_ERR("input param is NULL.");
        return NULL;
    }
    return (&(alts)->endpoint[epIndex].desc);
}

enum usb_device_speed AudioUsbGetSpeed(struct usb_device *dev)
{
    if (dev == NULL) {
        AUDIO_DRIVER_LOG_ERR("input param is NULL.");
        return USB_SPEED_UNKNOWN;
    }
    return ((dev)->speed);
}

uint32_t AudioUsbCombineWord(uint8_t *bytes)
{
    // This data is composed of two bytes of data.
    return (*(bytes)) | ((uint32_t)(bytes)[AUDIO_USB_INDEX_1] << USB_SHIFT_SIZE_8);
}

uint32_t AudioUsbCombineTriple(uint8_t *bytes)
{
    // This data is composed of three bytes of data.
    return AudioUsbCombineWord(bytes) | ((uint32_t)(bytes)[AUDIO_USB_INDEX_2] << USB_SHIFT_SIZE_16);
}

uint32_t AudioUsbCombineQuad(uint8_t *bytes)
{
    // This data is composed of four bytes of data.
    return AudioUsbCombineTriple(bytes) | ((uint32_t)(bytes)[AUDIO_USB_INDEX_3] << USB_SHIFT_SIZE_24);
}

uint32_t AudioUsbCombineBytes(uint8_t *bytes, int32_t size)
{
    if (bytes == NULL) {
        AUDIO_DRIVER_LOG_ERR("input para is NULL.");
        return 0; // combine bytes is 0
    }
    switch (size) {
        case AUDIO_USB_COMBINE_BYTE:
            return *bytes;
        case AUDIO_USB_COMBINE_WORD:
            return AudioUsbCombineWord(bytes);
        case AUDIO_USB_COMBINE_TRIPLE:
            return AudioUsbCombineTriple(bytes);
        case AUDIO_USB_COMBINE_QUAD:
            return AudioUsbCombineQuad(bytes);
        default:
            return 0; // combine bytes is 0
    }
}

void *AudioUsbFindDesc(void *descStart, int32_t descLen, void *after, uint8_t descType)
{
    uint8_t *start = NULL;
    uint8_t *end = NULL;
    uint8_t *next = NULL;

    if (descStart == NULL) {
        AUDIO_DRIVER_LOG_ERR("input para is NULL.");
        return NULL;
    }
    start = descStart;
    end = start + descLen;
    for (; start < end;) {
        if (start[0] < AUDIO_USB_DESC_LEN_2) {
            AUDIO_DRIVER_LOG_ERR("start is less than 2.");
            return NULL;
        }
        next = start + start[0];
        if (next > end) {
            AUDIO_DRIVER_LOG_ERR("next is greater than end.");
            return NULL;
        }
        if (start[1] == descType && (after == NULL || (void *)start > after)) {
            return start;
        }
        start = next;
    }
    return NULL;
}

void *AudioUsbFindCsintDesc(void *buf, int32_t bufLen, void *after, uint8_t descSubType)
{
    uint8_t *afterDesc = after;

    if (buf == NULL) {
        AUDIO_DRIVER_LOG_ERR("input para is NULL.");
        return NULL;
    }

    while ((afterDesc = AudioUsbFindDesc(buf, bufLen, afterDesc, USB_DT_CS_INTERFACE)) != NULL) {
        if (afterDesc[0] >= AUDIO_USB_DESC_LEN_3 && afterDesc[AUDIO_USB_DESC_LEN_2] == descSubType) {
            return afterDesc;
        }
    }

    return NULL;
}

int32_t AudioUsbCtlMsg(struct usb_device *dev, struct AudioUsbCtlMsgParam *usbCtlMsgParam, void *data, uint16_t size)
{
    int32_t ret, err;
    int32_t timeout;
    void *buf = NULL;

    if (dev == NULL || data == NULL) {
        AUDIO_DRIVER_LOG_ERR("input para is NULL.");
        return HDF_ERR_INVALID_PARAM;
    }

    if (usb_pipe_type_check(dev, usbCtlMsgParam->pipe)) {
        AUDIO_DRIVER_LOG_ERR("usb_pipe_type_check failed.");
        return HDF_FAILURE;
    }

    if (size == 0) {
        AUDIO_DRIVER_LOG_ERR("msg data size is 0.");
        return HDF_FAILURE;
    }

    buf = kmemdup(data, size, GFP_KERNEL);
    if (buf == NULL) {
        AUDIO_DRIVER_LOG_ERR("kmemdup failed.");
        return HDF_ERR_MALLOC_FAIL;
    }

    if ((usbCtlMsgParam->requestType & USB_DIR_IN) != 0) {
        timeout = USB_CTRL_GET_TIMEOUT;
    } else {
        timeout = USB_CTRL_SET_TIMEOUT;
    }

    ret = usb_control_msg(dev, usbCtlMsgParam->pipe, usbCtlMsgParam->request, usbCtlMsgParam->requestType,
        usbCtlMsgParam->value, usbCtlMsgParam->index, buf, size, timeout);

    err = memcpy_s(data, size, buf, size);
    if (err != EOK) {
        AUDIO_DRIVER_LOG_ERR("memcpy_s buf failed!");
        kfree(buf);
        return HDF_ERR_MALLOC_FAIL;
    }
    kfree(buf);
    return ret;
}

uint8_t AudioUsbParseDataInterval(struct usb_device *usbDev, struct usb_host_interface *alts)
{
    struct usb_endpoint_descriptor *epDesc = NULL;
    enum usb_device_speed usbSpeed;

    if (usbDev == NULL || alts == NULL) {
        AUDIO_DRIVER_LOG_ERR("input para is NULL.");
        return HDF_ERR_INVALID_PARAM;
    }
    epDesc = AudioEndpointDescriptor(alts, 0);
    usbSpeed = AudioUsbGetSpeed(usbDev);
    switch (usbSpeed) {
        case USB_SPEED_HIGH:
        case USB_SPEED_WIRELESS:
        case USB_SPEED_SUPER:
        case USB_SPEED_SUPER_PLUS:
            if (epDesc->bInterval >= AUDIO_USB_INTERVAL_LEN_1 && epDesc->bInterval <= AUDIO_USB_INTERVAL_LEN_4) {
                return epDesc->bInterval - AUDIO_USB_INTERVAL_LEN_1;
            }
            break;
        default:
            break;
    }
    return HDF_SUCCESS;
}

static bool ValidateDesc(uint8_t *hdr, int32_t protocol, const struct UsbDescValidator *usbDesc)
{
    if (hdr[AUDIO_USB_INDEX_1] != USB_DT_CS_INTERFACE) {
        return true;
    }
    AUDIO_DRIVER_LOG_DEBUG("hdr is USB_DT_CS_INTERFACE.");

    for (; usbDesc != NULL && usbDesc->type != 0; usbDesc++) {
        if (usbDesc->type == hdr[AUDIO_USB_INDEX_2] &&
            (usbDesc->protocol == UAC_VERSION_ALL || usbDesc->protocol == protocol)) {
            if (usbDesc->func != NULL) {
                return usbDesc->func(hdr, usbDesc);
            }
            return hdr[0] >= usbDesc->size;
        }
    }

    return true;
}

static bool ValidateUac1Header(const void *uacDesc, const struct UsbDescValidator *usbDesc)
{
    const struct uac1_ac_header_descriptor *uacHeaderDesc = uacDesc;
    uint32_t descSize = sizeof(*uacHeaderDesc) + uacHeaderDesc->bInCollection;

    if (uacHeaderDesc->bLength >= sizeof(*uacHeaderDesc) && uacHeaderDesc->bLength >= descSize) {
        return true;
    }
    return false;
}

/* for mixer unit; covering all UACs */
static bool ValidateMixerUnit(const void *ptr, const struct UsbDescValidator *usbDesc)
{
    const struct uac_mixer_unit_descriptor *uacMixerDesc = ptr;
    size_t length;
    bool temp;
    AUDIO_DRIVER_LOG_DEBUG("entry.");

    if (uacMixerDesc->bLength < sizeof(*uacMixerDesc) || !uacMixerDesc->bNrInPins) {
        return false;
    }
    length = sizeof(struct uac_mixer_unit_descriptor) + uacMixerDesc->bNrInPins;

    switch (usbDesc->protocol) {
        case UAC_VERSION_1:
            length += AUDIO_USB_DESC_LEN_2 + AUDIO_USB_DESC_LEN_2;
            break;
        case UAC_VERSION_2:
            length += AUDIO_USB_DESC_LEN_4 + AUDIO_USB_DESC_LEN_1;
            length += AUDIO_USB_DESC_LEN_2;
            break;
        case UAC_VERSION_3:
            length += AUDIO_USB_DESC_LEN_2;
            break;
        default:
            length += AUDIO_USB_DESC_LEN_2 + AUDIO_USB_DESC_LEN_2;
            break;
    }
    temp = uacMixerDesc->bLength >= length;
    return temp;
}

static bool UacProcessGetLength(const struct uac_processing_unit_descriptor *uacProcessDesc, const uint8_t *hdr,
    const struct UsbDescValidator *usbDesc, size_t *length)
{
    size_t tmpLen;
    switch (usbDesc->protocol) {
        case UAC_VERSION_1:
        default:
            *length += AUDIO_USB_DESC_LEN_4;
            if (uacProcessDesc->bLength < *length + 1) {
                return false;
            }
            tmpLen = hdr[*length];
            *length += tmpLen + AUDIO_USB_DESC_LEN_2;
            break;
        case UAC_VERSION_2:
            *length += AUDIO_USB_DESC_LEN_4 + AUDIO_USB_DESC_LEN_2;
            if (usbDesc->type == UAC2_PROCESSING_UNIT_V2) {
                *length += AUDIO_USB_DESC_LEN_2;
            } else {
                *length += AUDIO_USB_DESC_LEN_1;
            }
            *length += AUDIO_USB_DESC_LEN_1;
            break;
        case UAC_VERSION_3:
            *length += AUDIO_USB_DESC_LEN_4 + AUDIO_USB_DESC_LEN_2;
            break;
    }

    return true;
}

static bool UacV1ProcessGetLen(
    const struct uac_processing_unit_descriptor *uacProcessDesc, const uint8_t *hdr, size_t *length)
{
    size_t tmpLen;
    switch (le16_to_cpu(uacProcessDesc->wProcessType)) {
        case UAC_PROCESS_UP_DOWNMIX:
        case UAC_PROCESS_DOLBY_PROLOGIC:
            if (uacProcessDesc->bLength < *length + AUDIO_USB_DESC_LEN_1) {
                return false;
            }
            tmpLen = hdr[*length];
            *length += AUDIO_USB_DESC_LEN_1 + tmpLen * AUDIO_USB_DESC_LEN_2;
            break;
        default:
            break;
    }
    return true;
}

static bool UacV2ProcessGetLen(
    const struct uac_processing_unit_descriptor *uacProcessDesc, const uint8_t *hdr, size_t *length)
{
    size_t tmpLen;
    switch (le16_to_cpu(uacProcessDesc->wProcessType)) {
        case UAC2_PROCESS_UP_DOWNMIX:
        case UAC2_PROCESS_DOLBY_PROLOCIC:
            if (uacProcessDesc->bLength < *length + 1) {
                return false;
            }
            tmpLen = hdr[*length];
            *length += AUDIO_USB_DESC_LEN_1 + tmpLen * AUDIO_USB_DESC_LEN_4;
            break;
        default:
            break;
    }
    return true;
}

static bool UacV3ProcessGetLen(
    const struct uac_processing_unit_descriptor *uacProcessDesc, const uint8_t *hdr, size_t *length)
{
    size_t tmpLen;
    switch (le16_to_cpu(uacProcessDesc->wProcessType)) {
        case UAC3_PROCESS_UP_DOWNMIX:
            if (uacProcessDesc->bLength < *length + 1) {
                return false;
            }
            tmpLen = hdr[*length];
            *length += AUDIO_USB_DESC_LEN_1 + tmpLen * AUDIO_USB_DESC_LEN_2;
            break;
        case UAC3_PROCESS_MULTI_FUNCTION:
            *length += AUDIO_USB_DESC_LEN_2 + AUDIO_USB_DESC_LEN_4;
            break;
        default:
            break;
    }
    return true;
}

static bool ValidateProcessingUnitSub(const struct UsbDescValidator *usbDesc,
    const struct uac_processing_unit_descriptor *uacProcessDesc, const uint8_t *hdr, size_t *length)
{
    switch (usbDesc->protocol) {
        case UAC_VERSION_1:
            if (!UacV1ProcessGetLen(uacProcessDesc, hdr, length)) {
                AUDIO_DRIVER_LOG_ERR("UacV1ProcessGetLen failed.");
                return false;
            }
            break;
        case UAC_VERSION_2:
            if (!UacV2ProcessGetLen(uacProcessDesc, hdr, length)) {
                AUDIO_DRIVER_LOG_ERR("UacV1ProcessGetLen failed.");
                return false;
            }
            break;
        case UAC_VERSION_3:
            if (usbDesc->type == UAC3_EXTENSION_UNIT) {
                *length += AUDIO_USB_DESC_LEN_2; /* wClusterDescrID */
                break;
            }
            if (!UacV3ProcessGetLen(uacProcessDesc, hdr, length)) {
                AUDIO_DRIVER_LOG_ERR("UacV1ProcessGetLen failed.");
                return false;
            }
            break;
        default:
            if (!UacV1ProcessGetLen(uacProcessDesc, hdr, length)) {
                AUDIO_DRIVER_LOG_ERR("UacV1ProcessGetLen failed.");
                return false;
            }
            break;
    }
    return true;
}

/* both for processing and extension units; covering all UACs */
static bool ValidateProcessingUnit(const void *ptr, const struct UsbDescValidator *usbDesc)
{
    size_t length;
    const struct uac_processing_unit_descriptor *uacProcessDesc = ptr;
    const uint8_t *hdr = ptr;

    if (uacProcessDesc->bLength < sizeof(*uacProcessDesc)) {
        return false;
    }
    length = sizeof(*uacProcessDesc) + uacProcessDesc->bNrInPins;
    if (uacProcessDesc->bLength < length) {
        return false;
    }

    if (!UacProcessGetLength(uacProcessDesc, hdr, usbDesc, &length)) {
        AUDIO_DRIVER_LOG_ERR("UacProcessGetLength failed.");
        return false;
    }

    if (uacProcessDesc->bLength < length) {
        return false;
    }
    if (usbDesc->type == UAC1_EXTENSION_UNIT || usbDesc->type == UAC2_EXTENSION_UNIT_V2) {
        return true; /* OK */
    }
    if (!ValidateProcessingUnitSub(usbDesc, uacProcessDesc, hdr, &length)) {
        AUDIO_DRIVER_LOG_ERR("ValidateProcessingUnitSub failed.");
        return false;
    }

    if (uacProcessDesc->bLength < length) {
        return false;
    }

    return true;
}

/* both for selector and clock selector units; covering all UACs */
static bool ValidateSelectorUnit(const void *ptr, const struct UsbDescValidator *usbDesc)
{
    const struct uac_selector_unit_descriptor *uacSelectorDesc = ptr;
    size_t length;

    if (uacSelectorDesc->bLength < sizeof(*uacSelectorDesc)) {
        return false;
    }
    length = sizeof(*uacSelectorDesc) + uacSelectorDesc->bNrInPins;
    switch (usbDesc->protocol) {
        case UAC_VERSION_1:
        default:
            length += AUDIO_USB_DESC_LEN_1;
            break;
        case UAC_VERSION_2:
            length += AUDIO_USB_DESC_LEN_2;
            break;
        case UAC_VERSION_3:
            length += AUDIO_USB_DESC_LEN_4 + AUDIO_USB_DESC_LEN_2;
            break;
    }
    return uacSelectorDesc->bLength >= length;
}

static bool ValidateUac1FeatureUnit(const void *ptr, const struct UsbDescValidator *usbDesc)
{
    (void)usbDesc;
    const struct uac_feature_unit_descriptor *featureDesc = ptr;

    if (featureDesc->bLength < sizeof(*featureDesc) || !featureDesc->bControlSize) {
        return false;
    }

    return featureDesc->bLength >= sizeof(*featureDesc) + featureDesc->bControlSize + AUDIO_USB_DESC_LEN_1;
}

static bool ValidateUac2FeatureUnit(const void *ptr, const struct UsbDescValidator *usbDesc)
{
    (void)usbDesc;
    const struct uac2_feature_unit_descriptor *featureDesc = ptr;

    if (featureDesc->bLength < sizeof(*featureDesc)) {
        return false;
    }

    return featureDesc->bLength >= sizeof(*featureDesc) + AUDIO_USB_DESC_LEN_4 + AUDIO_USB_DESC_LEN_1;
}

static const struct UsbDescValidator audioValidators[] = {
    {.protocol = UAC_VERSION_1, .type = UAC_HEADER,           .func = ValidateUac1Header                            },
    {.protocol = UAC_VERSION_1, .type = UAC_INPUT_TERMINAL,   .size = sizeof(struct uac_input_terminal_descriptor)  },
    {.protocol = UAC_VERSION_1, .type = UAC_OUTPUT_TERMINAL,  .size = sizeof(struct uac1_output_terminal_descriptor)},
    {.protocol = UAC_VERSION_1, .type = UAC_MIXER_UNIT,       .func = ValidateMixerUnit                             },
    {.protocol = UAC_VERSION_1, .type = UAC_SELECTOR_UNIT,    .func = ValidateSelectorUnit                          },
    {.protocol = UAC_VERSION_1, .type = UAC_FEATURE_UNIT,     .func = ValidateUac1FeatureUnit                       },
    {.protocol = UAC_VERSION_1, .type = UAC1_PROCESSING_UNIT, .func = ValidateProcessingUnit                        },
    {.protocol = UAC_VERSION_1, .type = UAC1_EXTENSION_UNIT,  .func = ValidateProcessingUnit                        },

    {.protocol = UAC_VERSION_2, .type = UAC_HEADER,           .size = sizeof(struct uac2_ac_header_descriptor)      },
    {.protocol = UAC_VERSION_2, .type = UAC_INPUT_TERMINAL,   .size = sizeof(struct uac2_input_terminal_descriptor) },
    {.protocol = UAC_VERSION_2, .type = UAC_OUTPUT_TERMINAL,  .size = sizeof(struct uac2_output_terminal_descriptor)},
    {.protocol = UAC_VERSION_2, .type = UAC_MIXER_UNIT,       .func = ValidateMixerUnit                             },
    {.protocol = UAC_VERSION_2, .type = UAC_SELECTOR_UNIT,    .func = ValidateSelectorUnit                          },
    {.protocol = UAC_VERSION_2, .type = UAC_FEATURE_UNIT,     .func = ValidateUac2FeatureUnit                       },
    {.protocol = UAC_VERSION_2, .type = UAC2_PROCESSING_UNIT_V2, .func = ValidateProcessingUnit                     },
    {.protocol = UAC_VERSION_2, .type = UAC2_EXTENSION_UNIT_V2,  .func = ValidateProcessingUnit                     },
    {.protocol = UAC_VERSION_2, .type = UAC2_CLOCK_SOURCE,       .size = sizeof(struct uac_clock_source_descriptor) },
    {.protocol = UAC_VERSION_2, .type = UAC2_CLOCK_SELECTOR,     .func = ValidateSelectorUnit                       },
    {.protocol = UAC_VERSION_2, .type = UAC2_CLOCK_MULTIPLIER, .size = sizeof(struct uac_clock_multiplier_descriptor)},
};

bool AudioUsbValidateAudioDesc(void *ptr, int32_t protocol)
{
    if (ptr == NULL) {
        AUDIO_DRIVER_LOG_ERR("input para is NULL.");
        return false;
    }

    bool valid = ValidateDesc(ptr, protocol, audioValidators);

    return valid;
}
