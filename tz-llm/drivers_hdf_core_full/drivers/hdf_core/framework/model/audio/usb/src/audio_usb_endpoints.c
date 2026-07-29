/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include "audio_usb_endpoints.h"

#include <linux/fs.h>
#include <linux/gfp.h>
#include <linux/init.h>
#include <linux/ratelimit.h>
#include <linux/slab.h>
#include <linux/usb.h>
#include <linux/usb/audio.h>

#include "audio_driver_log.h"
#include "audio_usb_validate_desc.h"

#define HDF_LOG_TAG HDF_AUDIO_USB

#define SYNC_URBS 4  /* always four urbs for sync */
#define MAX_QUEUE 18 /* try not to exceed this queue length, in ms */

#define EP_FLAG_RUNNING  1
#define EP_FLAG_STOPPING 2

#define AUDIO_USB_PCM_FORMAT_DSD_U8     48 /* DSD, 1-byte samples DSD (x8) */
#define AUDIO_USB_PCM_FORMAT_DSD_U16_LE 49 /* DSD, 2-byte samples DSD (x16), little endian */
#define AUDIO_USB_PCM_FORMAT_DSD_U32_LE 50 /* DSD, 4-byte samples DSD (x32), little endian */
#define AUDIO_USB_PCM_FORMAT_DSD_U16_BE 51 /* DSD, 2-byte samples DSD (x16), big endian */
#define AUDIO_USB_PCM_FORMAT_DSD_U32_BE 52 /* DSD, 4-byte samples DSD (x32), big endian */

#define AUDIO_USB_PCM_FORMAT_S8     0
#define AUDIO_USB_PCM_FORMAT_U8     1
#define AUDIO_USB_PCM_FORMAT_S16_LE 2
#define AUDIO_USB_PCM_FORMAT_S16_BE 3
#define AUDIO_USB_PCM_FORMAT_U16_LE 4
#define AUDIO_USB_PCM_FORMAT_U16_BE 5
#define AUDIO_USB_PCM_FORMAT_S24_LE 6 /* low three bytes */
#define AUDIO_USB_PCM_FORMAT_S24_BE 7 /* low three bytes */
#define AUDIO_USB_PCM_FORMAT_U24_LE 8 /* low three bytes */
#define AUDIO_USB_PCM_FORMAT_U24_BE 9 /* low three bytes */
#define AUDIO_USB_PCM_FORMAT_S32_LE 10
#define AUDIO_USB_PCM_FORMAT_S32_BE 11
#define AUDIO_USB_PCM_FORMAT_U32_LE 12
#define AUDIO_USB_PCM_FORMAT_U32_BE 13

#define RIGHT_SHIFT_2  2
#define RIGHT_SHIFT_8  8
#define RIGHT_SHIFT_10 10
#define RIGHT_SHIFT_16 16
#define RIGHT_SHIFT_18 18
#define RIGHT_SHIFT_24 24

#define USB_HIGH_SPEED 10
#define USB_FULL_SPEED 13

// expected length in the struct usb_iso_packet_descriptor
#define ISO_FRAME_DESC_3 3
#define ISO_FRAME_DESC_4 4

#define USB_SPEED_RATE_CONVERT      62
#define USB_SPEED_RATE_CONVERT_COEF 125

#define SILENCE_VALUE_8     0x80
#define SILENCE_VALUE_OTHER 0x69

#define DATA_INTERVAL_MAX 5
#define FRAME_BITS_MASK   3

#define ARRAY_INDEX_0 0
#define ARRAY_INDEX_1 1
#define ARRAY_INDEX_2 2
#define ARRAY_INDEX_3 3

#define PACKETS_PER_SECOND_1000 1000
#define PACKETS_PER_SECOND_8000 8000

bool AudioUsbEndpointImplicitFeedbackSink(struct AudioUsbEndpoint *endpoint)
{
    if (endpoint == NULL) {
        AUDIO_DEVICE_LOG_ERR("input param is null.");
        return false;
    }
    if (endpoint->syncMasterEndpoint == NULL) {
        return false;
    }
    if (endpoint->syncMasterEndpoint->type != AUDIO_USB_ENDPOINT_TYPE_DATA) {
        AUDIO_DEVICE_LOG_ERR("type not is AUDIO_USB_ENDPOINT_TYPE_DATA.");
        return false;
    }
    if (endpoint->type != AUDIO_USB_ENDPOINT_TYPE_DATA) {
        AUDIO_DEVICE_LOG_ERR("type not is AUDIO_USB_ENDPOINT_TYPE_DATA.");
        return false;
    }
    if (usb_pipeout(endpoint->pipe) == 0) {
        AUDIO_DEVICE_LOG_ERR("usb_pipeout failed.");
        return false;
    }
    return true;
}

static void AudioUsbPrepareInboundUrb(struct AudioUsbEndpoint *endpoint, struct AudioUsbUrbCtx *urbCtx)
{
    uint32_t i;
    uint32_t offs;
    struct urb *urb = urbCtx->urb;

    urb->dev = endpoint->audioUsbDriver->dev; /* we need to set this at each time */
    switch (endpoint->type) {
        case AUDIO_USB_ENDPOINT_TYPE_DATA:
            offs = 0;
            for (i = 0; i < urbCtx->packets; i++) {
                urb->iso_frame_desc[i].offset = offs;
                urb->iso_frame_desc[i].length = endpoint->curPackSize;
                offs += endpoint->curPackSize;
            }
            urb->transfer_buffer_length = offs;
            urb->number_of_packets = urbCtx->packets;
            break;
        case AUDIO_USB_ENDPOINT_TYPE_SYNC:
            urb->iso_frame_desc[0].length = min((uint32_t)SYNC_URBS, endpoint->syncMaxSize);
            urb->iso_frame_desc[0].offset = 0;
            break;
        default:
            AUDIO_DEVICE_LOG_ERR("endpoint->type is invalid.");
            return;
    }
}

uint32_t AudioUsbEndpointSlaveNextPacketSize(struct AudioUsbEndpoint *endpoint)
{
    unsigned long flags;
    uint32_t val;

    if (endpoint == NULL) {
        AUDIO_DEVICE_LOG_ERR("input param is null.");
        return 0; // 0 packet size is invalid
    }

    if (endpoint->fillMax != 0) {
        AUDIO_DEVICE_LOG_ERR("endpoint->fillMax not is 0.");
        return endpoint->maxFrameSize;
    }
    spin_lock_irqsave(&endpoint->lock, flags);
    endpoint->phase = (endpoint->phase & 0xffff) + (endpoint->freqm << endpoint->dataInterval);
    val = min(endpoint->phase >> RIGHT_SHIFT_16, endpoint->maxFrameSize);
    spin_unlock_irqrestore(&endpoint->lock, flags);

    return val;
}

uint32_t AudioUsbEndpointNextPacketSize(struct AudioUsbEndpoint *endpoint)
{
    uint32_t val;

    if (endpoint == NULL) {
        AUDIO_DEVICE_LOG_ERR("input param is null.");
        return 0; // 0 packet size is invalid
    }

    if (endpoint->fillMax != 0) {
        AUDIO_DEVICE_LOG_ERR("endpoint->fillMax not is 0.");
        return endpoint->maxFrameSize;
    }
    endpoint->sampleAccum += endpoint->sampleRem;
    if (endpoint->sampleAccum >= endpoint->pps) {
        endpoint->sampleAccum -= endpoint->pps;
        val = endpoint->packsize[1];
    } else {
        val = endpoint->packsize[0];
    }

    return val;
}

static void AudioUsbPrepareSilentUrb(struct AudioUsbEndpoint *endpoint, struct AudioUsbUrbCtx *urbCtx)
{
    struct urb *urb = urbCtx->urb;
    uint32_t offs = 0;
    uint32_t i;
    uint32_t offset;
    uint32_t length;
    uint32_t counts;

    for (i = 0; i < urbCtx->packets; ++i) {
        counts = AudioUsbEndpointNextPacketSize(endpoint);

        length = counts * endpoint->stride; /* number of silent bytes */
        offset = offs * endpoint->stride;
        urb->iso_frame_desc[i].offset = offset;
        urb->iso_frame_desc[i].length = length;

        (void)memset_s(urb->transfer_buffer + offset, length, endpoint->silenceValue, length);
        offs += counts;
    }

    urb->number_of_packets = urbCtx->packets;
    urb->transfer_buffer_length = offs * endpoint->stride;
}

/*
 * Prepare a PLAYBACK urb for submission to the bus.
 */
static void AudioUsbPrepareOutboundUrb(struct AudioUsbEndpoint *endpoint, struct AudioUsbUrbCtx *urbCtx)
{
    struct urb *urb = urbCtx->urb;
    unsigned char *cp = urb->transfer_buffer;

    urb->dev = endpoint->audioUsbDriver->dev; /* we need to set this at each time */
    switch (endpoint->type) {
        case AUDIO_USB_ENDPOINT_TYPE_DATA:
            if (endpoint->AudioPrepareDataUrb) {
                endpoint->AudioPrepareDataUrb(endpoint->audioUsbDriver, urb);
            } else {
                /* no data, so send quiet */
                AudioUsbPrepareSilentUrb(endpoint, urbCtx);
            }
            break;
        case AUDIO_USB_ENDPOINT_TYPE_SYNC:
            if (AudioUsbGetSpeed(endpoint->audioUsbDriver->dev) >= USB_SPEED_HIGH) {
                urb->iso_frame_desc[ARRAY_INDEX_0].length = ISO_FRAME_DESC_4;
                urb->iso_frame_desc[ARRAY_INDEX_0].offset = 0;
                cp[ARRAY_INDEX_0] = endpoint->freqn;
                cp[ARRAY_INDEX_1] = endpoint->freqn >> RIGHT_SHIFT_8;
                cp[ARRAY_INDEX_2] = endpoint->freqn >> RIGHT_SHIFT_16;
                cp[ARRAY_INDEX_3] = endpoint->freqn >> RIGHT_SHIFT_24;
            } else {
                urb->iso_frame_desc[ARRAY_INDEX_0].length = ISO_FRAME_DESC_3;
                urb->iso_frame_desc[ARRAY_INDEX_0].offset = 0;
                cp[ARRAY_INDEX_0] = endpoint->freqn >> RIGHT_SHIFT_2;
                cp[ARRAY_INDEX_1] = endpoint->freqn >> RIGHT_SHIFT_10;
                cp[ARRAY_INDEX_2] = endpoint->freqn >> RIGHT_SHIFT_18;
            }
            break;
        default:
            AUDIO_DEVICE_LOG_ERR("endpoint->type is invalid.");
            return;
    }
}

/*
 * unlink active urbs.
 */
static void AudioUsbDeactivateUrbs(struct AudioUsbEndpoint *endpoint)
{
    uint32_t i;
    struct urb *urb = NULL;

    clear_bit(EP_FLAG_RUNNING, &endpoint->flags);
    INIT_LIST_HEAD(&endpoint->readyPlaybackUrbs);
    endpoint->nextPacketReadPos = 0;
    endpoint->nextPacketWritePos = 0;

    for (i = 0; i < endpoint->nurbs; i++) {
        urb = endpoint->urbContext[i].urb;
        if (test_bit(i, &endpoint->activeMask)) {
            if (!test_and_set_bit(i, &endpoint->unlinkMask)) {
                usb_unlink_urb(urb);
            }
        }
    }
}

void AudioUsbEndpointStop(struct AudioUsbEndpoint *endpoint)
{
    if (endpoint == NULL) {
        AUDIO_DEVICE_LOG_ERR("input param is null.");
        return;
    }
    if (--endpoint->useCount == 0) {
        AudioUsbDeactivateUrbs(endpoint);
        set_bit(EP_FLAG_STOPPING, &endpoint->flags);
    }
}

int32_t AudioUsbEndpointStart(struct AudioUsbEndpoint *endpoint)
{
    int32_t err;
    uint32_t i;
    struct AudioUsbUrbCtx *urbCtx = NULL;
    struct urb *urb = NULL;

    if (endpoint == NULL) {
        AUDIO_DEVICE_LOG_ERR("input param is null.");
        return HDF_ERR_INVALID_OBJECT;
    }

    if (atomic_read(&endpoint->audioUsbDriver->shutdown)) {
        AUDIO_DEVICE_LOG_ERR("atomic_read failed.");
        return HDF_ERR_BAD_FD;
    }
    /* already running */
    if (++endpoint->useCount != 1) {
        return HDF_SUCCESS;
    }
    /* just to be sure */
    AudioUsbDeactivateUrbs(endpoint);

    endpoint->activeMask = 0;
    endpoint->unlinkMask = 0;
    endpoint->phase = 0;
    endpoint->sampleAccum = 0;

    set_bit(EP_FLAG_RUNNING, &endpoint->flags);

    if (AudioUsbEndpointImplicitFeedbackSink(endpoint)) {
        for (i = 0; i < endpoint->nurbs; i++) {
            urbCtx = endpoint->urbContext + i;
            list_add_tail(&urbCtx->readyList, &endpoint->readyPlaybackUrbs);
        }
        return HDF_SUCCESS;
    }

    for (i = 0; i < endpoint->nurbs; i++) {
        urb = endpoint->urbContext[i].urb;
        if (usb_pipeout(endpoint->pipe)) {
            AudioUsbPrepareOutboundUrb(endpoint, urb->context);
        } else {
            AudioUsbPrepareInboundUrb(endpoint, urb->context);
        }

        err = usb_submit_urb(urb, GFP_ATOMIC);
        if (err < HDF_SUCCESS) {
            AUDIO_DEVICE_LOG_ERR("cannot submit urb %d, error %d\n", i, err);
            clear_bit(EP_FLAG_RUNNING, &endpoint->flags);
            endpoint->useCount--;
            AudioUsbDeactivateUrbs(endpoint);
            return HDF_ERR_NOPERM;
        }
        set_bit(i, &endpoint->activeMask);
    }
    return HDF_SUCCESS;
}

static void AudioUsbQueuePendingOutputUrbs(struct AudioUsbEndpoint *endpoint)
{
    unsigned long flags;
    struct AudioUsbPacketInfo *packet = NULL;
    struct AudioUsbUrbCtx *urbCtx = NULL;
    int32_t err;
    uint32_t index;

    while (test_bit(EP_FLAG_RUNNING, &endpoint->flags)) {
        spin_lock_irqsave(&endpoint->lock, flags);
        if (endpoint->nextPacketReadPos != endpoint->nextPacketWritePos) {
            packet = endpoint->nextPacket + endpoint->nextPacketReadPos;
            endpoint->nextPacketReadPos++;
            endpoint->nextPacketReadPos %= MAX_URBS;

            if (!list_empty(&endpoint->readyPlaybackUrbs)) {
                urbCtx = list_first_entry(&endpoint->readyPlaybackUrbs, struct AudioUsbUrbCtx, readyList);
            }
        }
        spin_unlock_irqrestore(&endpoint->lock, flags);
        if (urbCtx == NULL) {
            return;
        }
        list_del_init(&urbCtx->readyList);

        AUDIO_DEVICE_LOG_DEBUG("urbCtx is not null.");

        for (index = 0; index < packet->packets; index++) {
            urbCtx->packetSize[index] = packet->packetSize[index];
        }

        AudioUsbPrepareOutboundUrb(endpoint, urbCtx);

        err = usb_submit_urb(urbCtx->urb, GFP_ATOMIC);
        if (err < HDF_SUCCESS) {
            AUDIO_DEVICE_LOG_ERR("Unable to submit urb #%d: %d\n", urbCtx->index, err);
        } else {
            set_bit(urbCtx->index, &endpoint->activeMask);
        }
    }
}

static void AudioUsbUpdataOutPacket(
    struct AudioUsbEndpoint *endpoint, struct AudioUsbEndpoint *sender, const struct urb *urb)
{
    uint32_t i;
    uint32_t bytes = 0;
    unsigned long flags;
    struct AudioUsbUrbCtx *inUrbCtx = NULL;
    struct AudioUsbPacketInfo *outPacket = NULL;

    inUrbCtx = urb->context;
    for (i = 0; i < inUrbCtx->packets; i++) {
        if (urb->iso_frame_desc[i].status == 0) {
            bytes += urb->iso_frame_desc[i].actual_length;
        }
    }
    if (bytes == 0) {
        AUDIO_DEVICE_LOG_DEBUG("urb iso frame desc lenght is 0");
        return;
    }
    spin_lock_irqsave(&endpoint->lock, flags);
    outPacket = endpoint->nextPacket + endpoint->nextPacketWritePos;

    outPacket->packets = inUrbCtx->packets;
    for (i = 0; i < inUrbCtx->packets; i++) {
        if (urb->iso_frame_desc[i].status == 0) {
            outPacket->packetSize[i] = urb->iso_frame_desc[i].actual_length / sender->stride;
        } else {
            outPacket->packetSize[i] = 0;
        }
    }

    endpoint->nextPacketWritePos++;
    endpoint->nextPacketWritePos %= MAX_URBS;
    spin_unlock_irqrestore(&endpoint->lock, flags);
    AudioUsbQueuePendingOutputUrbs(endpoint);
    return;
}

void AudioUsbHandleSyncUrb(struct AudioUsbEndpoint *endpoint, struct AudioUsbEndpoint *sender, const struct urb *urb)
{
    int32_t shift;
    uint32_t frequency;
    unsigned long flags;

    if (AudioUsbEndpointImplicitFeedbackSink(endpoint) && endpoint->useCount != 0) {
        return AudioUsbUpdataOutPacket(endpoint, sender, urb);
    }

    if (urb->iso_frame_desc[0].status != 0 || urb->iso_frame_desc[0].actual_length < ISO_FRAME_DESC_3) {
        return;
    }
    frequency = le32_to_cpup(urb->transfer_buffer);
    if (urb->iso_frame_desc[0].actual_length == ISO_FRAME_DESC_3) {
        frequency &= 0x00ffffff; // ISO_FRAME_DESC_3 frequency is the valid value of the lower 24 bits
    } else {
        frequency &= 0x0fffffff; // other frequency is the valid value of the lower 28 bits
    }
    if (frequency == 0) {
        return;
    }
    if (unlikely(sender->tenorFbQuirk)) {
        if (frequency < endpoint->freqn - 0x8000) {
            // Devices based on Tenor 8802 chipsets sometimes change the feedback value by + 0x1.0000
            frequency += 0xf000;
        } else if (frequency > endpoint->freqn + 0x8000) {
            // Devices based on Tenor 8802 chipsets sometimes change the feedback value by - 0x1.0000
            frequency -= 0xf000;
        }
    } else if (unlikely(endpoint->freqShift == INT_MIN)) {
        shift = 0;
        while (frequency < endpoint->freqn - endpoint->freqn / SYNC_URBS) {
            frequency <<= 1;
            shift++;
        }
        while (frequency > endpoint->freqn + endpoint->freqn / EP_FLAG_STOPPING) {
            frequency >>= 1;
            shift--;
        }
        endpoint->freqShift = shift;
    } else if (endpoint->freqShift >= 0) {
        frequency <<= endpoint->freqShift;
    } else {
        frequency >>= -endpoint->freqShift;
    }
    if (likely(frequency >= endpoint->freqn - endpoint->freqn / RIGHT_SHIFT_8 && frequency <= endpoint->freqMax)) {
        spin_lock_irqsave(&endpoint->lock, flags);
        endpoint->freqm = frequency;
        spin_unlock_irqrestore(&endpoint->lock, flags);
    } else {
        endpoint->freqShift = INT_MIN;
    }
}

static void AudioUsbRetireInboundUrb(struct AudioUsbEndpoint *endpoint, struct AudioUsbUrbCtx *urbCtx)
{
    struct urb *urb = urbCtx->urb;

    if (unlikely(endpoint->skipPackets > 0)) {
        endpoint->skipPackets--;
        return;
    }

    if (endpoint->syncSlaveEndpoint != NULL) {
        AudioUsbHandleSyncUrb(endpoint->syncSlaveEndpoint, endpoint, urb);
    }
    if (endpoint->AudioRetireDataUrb != NULL) {
        endpoint->AudioRetireDataUrb(endpoint->audioUsbDriver, urb);
    }
}

static void AudioUsbRetireOutboundUrb(struct AudioUsbEndpoint *endpoint, struct AudioUsbUrbCtx *urbCtx)
{
    if (endpoint->AudioRetireDataUrb != NULL) {
        endpoint->AudioRetireDataUrb(endpoint->audioUsbDriver, urbCtx->urb);
    }
}

static int32_t AudioUsbOutboundUrb(struct AudioUsbEndpoint *endpoint, struct AudioUsbUrbCtx *urbCtx)
{
    unsigned long flags;

    AudioUsbRetireOutboundUrb(endpoint, urbCtx);
    /* can be stopped during retire callback */
    if (unlikely(!test_bit(EP_FLAG_RUNNING, &endpoint->flags))) {
        clear_bit(urbCtx->index, &endpoint->activeMask);
        return HDF_FAILURE;
    }
    if (AudioUsbEndpointImplicitFeedbackSink(endpoint)) {
        spin_lock_irqsave(&endpoint->lock, flags);
        list_add_tail(&urbCtx->readyList, &endpoint->readyPlaybackUrbs);
        spin_unlock_irqrestore(&endpoint->lock, flags);
        AudioUsbQueuePendingOutputUrbs(endpoint);
        clear_bit(urbCtx->index, &endpoint->activeMask);
        AUDIO_DEVICE_LOG_ERR("AudioUsbEndpointImplicitFeedbackSink failed");
        return HDF_FAILURE;
    }

    AudioUsbPrepareOutboundUrb(endpoint, urbCtx);
    /* can be stopped during prepare callback */
    if (unlikely(!test_bit(EP_FLAG_RUNNING, &endpoint->flags))) {
        clear_bit(urbCtx->index, &endpoint->activeMask);
        return HDF_FAILURE;
    }

    return HDF_SUCCESS;
}

/*
 * complete callback for urbs
 */
static void AudioUsbCompleteUrb(struct urb *urb)
{
    struct AudioUsbUrbCtx *urbCtx = urb->context;
    struct AudioUsbEndpoint *endpoint = urbCtx->endpoint;
    int32_t err;

    if (unlikely(urb->status == -ENOENT || /* unlinked */
            urb->status == -ENODEV ||      /* device removed */
            urb->status == -ECONNRESET ||  /* unlinked */
            urb->status == -ESHUTDOWN)) {  /* device disabled */
        clear_bit(urbCtx->index, &endpoint->activeMask);
        AUDIO_DEVICE_LOG_DEBUG("urb status is invalid");
        return;
    }
    /* device disconnected */

    if (unlikely(atomic_read(&endpoint->audioUsbDriver->shutdown))) {
        clear_bit(urbCtx->index, &endpoint->activeMask);
        return;
    }
    if (unlikely(!test_bit(EP_FLAG_RUNNING, &endpoint->flags))) {
        clear_bit(urbCtx->index, &endpoint->activeMask);
        return;
    }
    if (usb_pipeout(endpoint->pipe)) {
        err = AudioUsbOutboundUrb(endpoint, urbCtx);
        if (err != HDF_SUCCESS) {
            AUDIO_DEVICE_LOG_ERR("AudioUsbOutboundUrb failed");
            return;
        }
    } else {
        AudioUsbRetireInboundUrb(endpoint, urbCtx);
        /* can be stopped during retire callback */
        if (unlikely(!test_bit(EP_FLAG_RUNNING, &endpoint->flags))) {
            clear_bit(urbCtx->index, &endpoint->activeMask);
            return;
        }
        AudioUsbPrepareInboundUrb(endpoint, urbCtx);
    }

    err = usb_submit_urb(urb, GFP_ATOMIC);
    if (err != 0) {
        AUDIO_DEVICE_LOG_ERR("usb_submit_urb failed");
        return;
    }
}

/*
 *  wait until all urbs are processed.
 */
static int32_t AudioUsbWaitClearUrbs(struct AudioUsbEndpoint *endpoint)
{
    unsigned long endTime = jiffies + msecs_to_jiffies(PACKETS_PER_SECOND_1000);
    int32_t alive;
    AUDIO_DEVICE_LOG_DEBUG("entry");

    do {
        alive = bitmap_weight(&endpoint->activeMask, endpoint->nurbs);
        if (alive == 0) {
            break;
        }
        schedule_timeout_uninterruptible(1);
    } while (time_before(jiffies, endTime));

    if (alive != 0) {
        AUDIO_DEVICE_LOG_INFO("timeout: still %d active urbs on EP #%x\n", alive, endpoint->endpointNum);
    }
    clear_bit(EP_FLAG_STOPPING, &endpoint->flags);

    endpoint->syncSlaveEndpoint = NULL;
    endpoint->AudioRetireDataUrb = NULL;
    endpoint->AudioPrepareDataUrb = NULL;

    return HDF_SUCCESS;
}

/*
 * release a urb data
 */
static void AudioUsbReleaseUrbCtx(struct AudioUsbUrbCtx *urb)
{
    if (urb->bufferSize) {
        usb_free_coherent(
            urb->endpoint->audioUsbDriver->dev, urb->bufferSize, urb->urb->transfer_buffer, urb->urb->transfer_dma);
    }
    usb_free_urb(urb->urb);
    urb->urb = NULL;
}

/*
 * release an endpoint's urbs
 */
static void AudioUsbReleaseUrbs(struct AudioUsbEndpoint *endpoint, int force)
{
    int32_t i;

    /* route incoming urbs to nirvana */
    endpoint->AudioRetireDataUrb = NULL;
    endpoint->AudioPrepareDataUrb = NULL;

    /* stop urbs */
    AudioUsbDeactivateUrbs(endpoint);
    AudioUsbWaitClearUrbs(endpoint);

    for (i = 0; i < endpoint->nurbs; i++) {
        AudioUsbReleaseUrbCtx(&endpoint->urbContext[i]);
    }
    usb_free_coherent(endpoint->audioUsbDriver->dev, SYNC_URBS * SYNC_URBS, endpoint->syncbuf, endpoint->syncDma);

    endpoint->syncbuf = NULL;
    endpoint->nurbs = 0;
}

/*
 * configure a sync endpoint
 */
static int32_t AudioUsbSyncEpSetParams(struct AudioUsbEndpoint *endpoint)
{
    uint32_t i;
    struct AudioUsbUrbCtx *urbContext = NULL;

    endpoint->syncbuf =
        usb_alloc_coherent(endpoint->audioUsbDriver->dev, SYNC_URBS * SYNC_URBS, GFP_KERNEL, &endpoint->syncDma);
    if (endpoint->syncbuf == NULL) {
        AUDIO_DEVICE_LOG_ERR("usb_alloc_coherent failed.");
        return HDF_DEV_ERR_NO_MEMORY;
    }
    for (i = 0; i < SYNC_URBS; i++) {
        urbContext = &endpoint->urbContext[i];
        urbContext->index = i;
        urbContext->endpoint = endpoint;
        urbContext->packets = 1;
        urbContext->urb = usb_alloc_urb(1, GFP_KERNEL);
        if (urbContext->urb == NULL) {
            AudioUsbReleaseUrbs(endpoint, 0);
            return HDF_DEV_ERR_NO_MEMORY;
        }
        urbContext->urb->transfer_buffer = endpoint->syncbuf + i * SYNC_URBS;
        urbContext->urb->transfer_dma = endpoint->syncDma + i * SYNC_URBS;
        urbContext->urb->transfer_buffer_length = SYNC_URBS;
        urbContext->urb->pipe = endpoint->pipe;
        urbContext->urb->transfer_flags = URB_NO_TRANSFER_DMA_MAP;
        urbContext->urb->number_of_packets = 1;
        urbContext->urb->interval = 1 << endpoint->syncInterval;
        urbContext->urb->context = urbContext;
        urbContext->urb->complete = AudioUsbCompleteUrb;
    }

    endpoint->nurbs = SYNC_URBS;
    AUDIO_DEVICE_LOG_INFO("success.");
    return HDF_SUCCESS;
}

static inline unsigned AudioUsbGetUsbSpeedRate(uint32_t rate, uint32_t speed)
{
    return ((rate << speed) + USB_SPEED_RATE_CONVERT) / USB_SPEED_RATE_CONVERT_COEF;
}

int32_t AudioUSBPcmFormat(uint32_t bitWidth, bool isBigEndian, uint32_t *usbPcmFormat)
{
    if (usbPcmFormat == NULL) {
        AUDIO_DEVICE_LOG_ERR("paras is NULL!");
        return HDF_FAILURE;
    }

    /** Little Endian */
    if (!isBigEndian) {
        switch (bitWidth) {
            case BIT_WIDTH8:
                *usbPcmFormat = AUDIO_USB_PCM_FORMAT_U8; /** unsigned 8 bit */
                break;
            case BIT_WIDTH16:
                *usbPcmFormat = AUDIO_USB_PCM_FORMAT_U16_LE; /** unsigned 16 bit Little Endian */
                break;
            case BIT_WIDTH24:
                *usbPcmFormat = AUDIO_USB_PCM_FORMAT_U24_LE; /** unsigned 24 bit Little Endian */
                break;
            case BIT_WIDTH32:
                *usbPcmFormat = AUDIO_USB_PCM_FORMAT_U32_LE; /** unsigned 32 bit Little Endian */
                break;
            default:
                return HDF_ERR_NOT_SUPPORT;
        }
    } else { /** Big Endian */
        switch (bitWidth) {
            case BIT_WIDTH8:
                *usbPcmFormat = AUDIO_USB_PCM_FORMAT_U8; /** unsigned 8 bit */
                break;
            case BIT_WIDTH16:
                *usbPcmFormat = AUDIO_USB_PCM_FORMAT_U16_BE; /** unsigned 16 bit Big Endian */
                break;
            case BIT_WIDTH24:
                *usbPcmFormat = AUDIO_USB_PCM_FORMAT_U24_BE; /** unsigned 24 bit Big Endian */
                break;
            case BIT_WIDTH32:
                *usbPcmFormat = AUDIO_USB_PCM_FORMAT_U32_BE; /** unsigned 32 bit Big Endian */
                break;
            default:
                return HDF_ERR_NOT_SUPPORT;
        }
    }

    return HDF_SUCCESS;
}

static int32_t AudioUsbDataEpSetParamsSub(struct AudioUsbEndpoint *endpoint, struct PcmInfo *pcmInfo,
    struct CircleBufInfo *bufInfo, struct AudioUsbFormat *audioUsbFormat, uint32_t *maxsize)
{
    uint32_t frameBits;
    int32_t ret;
    uint32_t periodBytes;
    uint32_t framesPerPeriod;
    uint32_t periodsPerBuffer;
    uint32_t format;

    framesPerPeriod = bufInfo->periodSize / pcmInfo->frameSize;
    periodBytes = bufInfo->periodSize;
    periodsPerBuffer = bufInfo->periodCount;

    ret = AudioUSBPcmFormat(pcmInfo->bitWidth, pcmInfo->isBigEndian, &format);
    if (ret < HDF_SUCCESS) {
        AUDIO_DEVICE_LOG_ERR("AudioBitWidthToFormat is failed.");
        return ret;
    }

    frameBits = pcmInfo->bitWidth * pcmInfo->channels;

    if (format == AUDIO_USB_PCM_FORMAT_DSD_U16_LE && audioUsbFormat->dsdDop) {
        frameBits += pcmInfo->channels << FRAME_BITS_MASK;
    }

    endpoint->dataInterval = audioUsbFormat->dataInterval;
    endpoint->stride = frameBits >> FRAME_BITS_MASK;

    switch (format) {
        case AUDIO_USB_PCM_FORMAT_U8:
            endpoint->silenceValue = SILENCE_VALUE_8;
            break;
        case AUDIO_USB_PCM_FORMAT_DSD_U8:
        case AUDIO_USB_PCM_FORMAT_DSD_U16_LE:
        case AUDIO_USB_PCM_FORMAT_DSD_U32_LE:
        case AUDIO_USB_PCM_FORMAT_DSD_U16_BE:
        case AUDIO_USB_PCM_FORMAT_DSD_U32_BE:
            endpoint->silenceValue = SILENCE_VALUE_OTHER;
            break;
        default:
            endpoint->silenceValue = 0;
    }
    endpoint->freqMax = endpoint->freqn + (endpoint->freqn >> 1);
    *maxsize =
        (((endpoint->freqMax << endpoint->dataInterval) + 0xffff) >> RIGHT_SHIFT_16) * (frameBits >> FRAME_BITS_MASK);

    /* but wMaxPacketSize might reduce this */
    if (endpoint->maxPackSize && endpoint->maxPackSize < *maxsize) {
        uint32_t data_maxsize = *maxsize = endpoint->maxPackSize;
        endpoint->freqMax = (data_maxsize / (frameBits >> FRAME_BITS_MASK))
            << (RIGHT_SHIFT_16 - endpoint->dataInterval);
    }

    if (endpoint->fillMax != 0) {
        endpoint->curPackSize = endpoint->maxPackSize;
    } else {
        endpoint->curPackSize = *maxsize;
    }
    return HDF_SUCCESS;
}

static uint32_t AudioUsbSetCaptureEpParams(struct AudioUsbEndpoint *endpoint, uint32_t maxPacksPerUrb, uint32_t maxsize,
    uint32_t periodBytes, uint32_t packsPerMs)
{
    uint32_t urbPacks;
    uint32_t temp;
    uint32_t interval;

    urbPacks = packsPerMs;
    if (AudioUsbGetSpeed(endpoint->audioUsbDriver->dev) == USB_SPEED_WIRELESS) {
        interval = endpoint->dataInterval;
        while (interval < DATA_INTERVAL_MAX) {
            urbPacks <<= 1;
            ++interval;
        }
    }

    urbPacks = min(maxPacksPerUrb, urbPacks);
    temp = urbPacks * maxsize;
    while (urbPacks > 1 && temp >= periodBytes) {
        urbPacks >>= 1;
    }
    endpoint->nurbs = MAX_URBS;
    return urbPacks;
}

static int32_t AudioUsbSetRenderEpParams(struct AudioUsbEndpoint *endpoint, struct PcmInfo *pcmInfo,
    struct CircleBufInfo *bufInfo, struct AudioUsbEndpoint *syncEp, struct AudioUsbFormat *audioUsbFormat)
{
    uint32_t maxPacksPerPeriod, minsize, urbsPerPeriod;
    uint32_t max_urbs;
    uint32_t periodBytes;
    uint32_t framesPerPeriod;
    uint32_t periodsPerBuffer;
    uint32_t format, frameBits;
    uint32_t packsPerMs, maxPacksPerUrb;
    uint32_t urbPacks;

    framesPerPeriod = bufInfo->periodSize / pcmInfo->frameSize;
    periodBytes = bufInfo->periodSize;
    periodsPerBuffer = bufInfo->periodCount;

    (void)AudioUSBPcmFormat(pcmInfo->bitWidth, pcmInfo->isBigEndian, &format);

    frameBits = pcmInfo->bitWidth * pcmInfo->channels;

    if (format == AUDIO_USB_PCM_FORMAT_DSD_U16_LE && audioUsbFormat->dsdDop) {
        frameBits += pcmInfo->channels << FRAME_BITS_MASK;
    }

    if (AudioUsbGetSpeed(endpoint->audioUsbDriver->dev) != USB_SPEED_FULL) {
        packsPerMs = RIGHT_SHIFT_8 >> endpoint->dataInterval;
        maxPacksPerUrb = MAX_PACKS_HS;
    } else {
        packsPerMs = 1;
        maxPacksPerUrb = MAX_PACKS;
    }

    if (syncEp != NULL && !AudioUsbEndpointImplicitFeedbackSink(endpoint)) {
        maxPacksPerUrb = min(maxPacksPerUrb, 1U << syncEp->syncInterval);
    }
    maxPacksPerUrb = max(1u, maxPacksPerUrb >> endpoint->dataInterval);

    minsize = (endpoint->freqn >> (RIGHT_SHIFT_16 - endpoint->dataInterval)) * (frameBits >> FRAME_BITS_MASK);
    if (syncEp != NULL) {
        minsize -= minsize >> FRAME_BITS_MASK;
    }
    minsize = max(minsize, 1u);
    maxPacksPerPeriod = DIV_ROUND_UP(periodBytes, minsize);
    urbsPerPeriod = DIV_ROUND_UP(maxPacksPerPeriod, maxPacksPerUrb);
    urbPacks = DIV_ROUND_UP(maxPacksPerPeriod, urbsPerPeriod);
    endpoint->maxUrbFrames = DIV_ROUND_UP(framesPerPeriod, urbsPerPeriod);
    if (urbPacks == 0) {
        AUDIO_DEVICE_LOG_ERR("urbPacks is 0");
        return HDF_FAILURE;
    }
    max_urbs = min((unsigned)MAX_URBS, MAX_QUEUE * packsPerMs / urbPacks);
    endpoint->nurbs = min(max_urbs, urbsPerPeriod * periodsPerBuffer);

    return urbPacks;
}

static int32_t AudioUsbDataUrbInit(
    struct AudioUsbEndpoint *endpoint, struct AudioUsbFormat *audioUsbFormat, uint32_t urbPacks, uint32_t maxsize)
{
    uint32_t i;
    struct AudioUsbUrbCtx *urbCtx = NULL;

    for (i = 0; i < endpoint->nurbs; i++) {
        urbCtx = &endpoint->urbContext[i];
        urbCtx->index = i;
        urbCtx->endpoint = endpoint;
        urbCtx->packets = urbPacks;
        urbCtx->bufferSize = maxsize * urbCtx->packets;

        if (audioUsbFormat->fmtType == UAC_FORMAT_TYPE_II) {
            urbCtx->packets++; /* for transfer delimiter */
        }

        urbCtx->urb = usb_alloc_urb(urbCtx->packets, GFP_KERNEL);
        if (urbCtx->urb == NULL) {
            AUDIO_DEVICE_LOG_ERR("usb_alloc_urb failed.");
            AudioUsbReleaseUrbs(endpoint, 0);
            return HDF_DEV_ERR_NO_MEMORY;
        }

        urbCtx->urb->transfer_buffer = usb_alloc_coherent(
            endpoint->audioUsbDriver->dev, urbCtx->bufferSize, GFP_KERNEL, &urbCtx->urb->transfer_dma);

        if (urbCtx->urb->transfer_buffer == NULL) {
            AUDIO_DEVICE_LOG_ERR("usb_alloc_coherent failed.");
            AudioUsbReleaseUrbs(endpoint, 0);
            return HDF_DEV_ERR_NO_MEMORY;
        }
        urbCtx->urb->pipe = endpoint->pipe;
        urbCtx->urb->transfer_flags = URB_NO_TRANSFER_DMA_MAP;
        urbCtx->urb->interval = 1 << endpoint->dataInterval;
        urbCtx->urb->context = urbCtx;
        urbCtx->urb->complete = AudioUsbCompleteUrb;

        INIT_LIST_HEAD(&urbCtx->readyList);
    }
    return HDF_SUCCESS;
}

/*
 * configure a data endpoint
 */
static int32_t AudioUsbDataEpSetParams(struct AudioUsbEndpoint *endpoint, struct PcmInfo *pcmInfo,
    struct CircleBufInfo *bufInfo, struct AudioUsbFormat *audioUsbFormat, struct AudioUsbEndpoint *syncEp)
{
    uint32_t maxsize, packsPerMs, maxPacksPerUrb;
    uint32_t urbPacks;
    uint32_t periodBytes;
    uint32_t format;
    enum usb_device_speed usbSpeed;

    periodBytes = bufInfo->periodSize;

    if (AudioUSBPcmFormat(pcmInfo->bitWidth, pcmInfo->isBigEndian, &format) < HDF_SUCCESS) {
        AUDIO_DEVICE_LOG_ERR("AudioUSBPcmFormat is failed.");
        return HDF_FAILURE;
    }

    if (AudioUsbDataEpSetParamsSub(endpoint, pcmInfo, bufInfo, audioUsbFormat, &maxsize) < HDF_SUCCESS) {
        AUDIO_DEVICE_LOG_ERR("AudioUsbDataEpSetParamsSub is failed.");
        return HDF_FAILURE;
    }

    usbSpeed = AudioUsbGetSpeed(endpoint->audioUsbDriver->dev);
    if (usbSpeed != USB_SPEED_FULL) {
        packsPerMs = RIGHT_SHIFT_8 >> endpoint->dataInterval;
        maxPacksPerUrb = MAX_PACKS_HS;
    } else {
        maxPacksPerUrb = MAX_PACKS;
        packsPerMs = 1;
    }

    if (syncEp != NULL && !AudioUsbEndpointImplicitFeedbackSink(endpoint)) {
        maxPacksPerUrb = min(maxPacksPerUrb, 1U << syncEp->syncInterval);
    }
    maxPacksPerUrb = max(1u, maxPacksPerUrb >> endpoint->dataInterval);

    if (usb_pipein(endpoint->pipe) || AudioUsbEndpointImplicitFeedbackSink(endpoint)) {
        urbPacks = AudioUsbSetCaptureEpParams(endpoint, maxPacksPerUrb, maxsize, periodBytes, packsPerMs);
    } else {
        urbPacks = AudioUsbSetRenderEpParams(endpoint, pcmInfo, bufInfo, syncEp, audioUsbFormat);
    }

    if (AudioUsbDataUrbInit(endpoint, audioUsbFormat, urbPacks, maxsize) < HDF_SUCCESS) {
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

int32_t AudioUsbEndpointSetParams(struct AudioUsbEndpoint *endpoint, struct PcmInfo *pcmInfo,
    struct CircleBufInfo *bufInfo, struct AudioUsbFormat *audioUsbFormat, struct AudioUsbEndpoint *syncEp)
{
    int32_t err;
    struct AudioUsbDriver *audioUsbDriver = NULL;
    struct usb_device *dev = NULL;

    if (endpoint == NULL || pcmInfo == NULL || bufInfo == NULL || audioUsbFormat == NULL) {
        AUDIO_DEVICE_LOG_ERR("input param is null.");
        return HDF_ERR_INVALID_OBJECT;
    }

    audioUsbDriver = endpoint->audioUsbDriver;
    if (audioUsbDriver == NULL) {
        AUDIO_DEVICE_LOG_ERR("hdfAudioUsbDriver is null.");
        return HDF_FAILURE;
    }
    dev = interface_to_usbdev(audioUsbDriver->usbIf);

    endpoint->dataInterval = audioUsbFormat->dataInterval;
    endpoint->maxPackSize = audioUsbFormat->maxPackSize;
    endpoint->fillMax = !!(audioUsbFormat->attributes & UAC_EP_CS_ATTR_FILL_MAX);

    if (AudioUsbGetSpeed(dev) == USB_SPEED_FULL) {
        endpoint->freqn = AudioUsbGetUsbSpeedRate(pcmInfo->rate, USB_FULL_SPEED);
        endpoint->pps = PACKETS_PER_SECOND_1000 >> endpoint->dataInterval;
    } else {
        endpoint->freqn = AudioUsbGetUsbSpeedRate(pcmInfo->rate, USB_HIGH_SPEED);
        endpoint->pps = PACKETS_PER_SECOND_8000 >> endpoint->dataInterval;
    }

    endpoint->sampleRem = pcmInfo->rate % endpoint->pps;
    endpoint->packsize[0] = pcmInfo->rate / endpoint->pps;
    endpoint->packsize[1] = (pcmInfo->rate + (endpoint->pps - 1)) / endpoint->pps;

    /* calculate the frequency in 16.16 format */
    endpoint->freqm = endpoint->freqn;
    endpoint->freqShift = INT_MIN;
    endpoint->phase = 0;

    switch (endpoint->type) {
        case AUDIO_USB_ENDPOINT_TYPE_DATA:
            err = AudioUsbDataEpSetParams(endpoint, pcmInfo, bufInfo, audioUsbFormat, syncEp);
            break;
        case AUDIO_USB_ENDPOINT_TYPE_SYNC:
            err = AudioUsbSyncEpSetParams(endpoint);
            break;
        default:
            AUDIO_DEVICE_LOG_ERR("endpoint->type is invalid.");
            err = HDF_ERR_INVALID_PARAM;
            return err;
    }

    return err;
}
