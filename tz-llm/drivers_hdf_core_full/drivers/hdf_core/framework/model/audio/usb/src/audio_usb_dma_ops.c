/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include "audio_usb_dma_ops.h"
#include <linux/device.h>
#include <linux/dma-mapping.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/notifier.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_dma.h>
#include <linux/stat.h>
#include <linux/string.h>
#include <linux/suspend.h>
#include <linux/sysfs.h>
#include <linux/usb.h>
#include <linux/usb/audio.h>
#include <linux/vmalloc.h>

#include "audio_driver_log.h"
#include "audio_platform_base.h"
#include "audio_usb_endpoints.h"
#include "audio_usb_linux.h"
#include "audio_usb_parse_interface.h"
#include "audio_usb_validate_desc.h"
#include "hdf_base.h"
#include "osal_io.h"
#include "osal_time.h"
#include "osal_uaccess.h"

#define HDF_LOG_TAG HDF_AUDIO_USB

#define I2S_TXDR 0x24
#define I2S_RXDR 0x28

#define DMA_TX_CHANNEL  0
#define DMA_RX_CHANNEL  1
#define DMA_CHANNEL_MAX 2

#define SUBSTREAM_FLAG_DATA_EP_STARTED 0
#define SUBSTREAM_FLAG_SYNC_EP_STARTED 1

#define SYNC_INTERVAL_0  0
#define SYNC_INTERVAL_1  1
#define SYNC_INTERVAL_2  2
#define SYNC_INTERVAL_3  3
#define SYNC_INTERVAL_16 16
#define SYNC_REFRESH_1   1
#define SYNC_REFRESH_9   9

#define USB_ENDPOINT_INDEX_0 0
#define USB_ENDPOINT_INDEX_1 1
#define USB_ENDPOINT_INDEX_2 2

#define AUDIO_USB_PCM_RATE_CONTINUOUS (1 << 30) /* continuous range */

int32_t AudioUsbDmaDeviceInit(const struct AudioCard *card, const struct PlatformDevice *platform)
{
    struct AudioUsbDriver *audioUsbDriver = NULL;

    if (platform == NULL || platform->devData == NULL) {
        AUDIO_DEVICE_LOG_ERR("PlatformData is null.");
        return HDF_FAILURE;
    }

    audioUsbDriver = GetLinuxAudioUsb();
    if (audioUsbDriver == NULL) {
        AUDIO_DEVICE_LOG_ERR("audioUsbDriver is failed.");
        return HDF_FAILURE;
    }

    DListHeadInit(&audioUsbDriver->endpointList);
    platform->devData->dmaPrv = (void *)audioUsbDriver;

    AUDIO_DEVICE_LOG_DEBUG("success.");
    return HDF_SUCCESS;
}

void AudioUsbDmaDeviceRelease(struct PlatformData *platformData)
{
    struct AudioUsbDriver *audioUsbDriver = NULL;
    struct AudioUsbEndpoint *endpoint = NULL;
    struct AudioUsbEndpoint *endpointTmp = NULL;
    struct AudioUsbFormat *audioUsbFormat = NULL;
    struct AudioUsbFormat *audioUsbFormatTmp = NULL;

    if (platformData->dmaPrv == NULL) {
        return;
    }
    audioUsbDriver = platformData->dmaPrv;

    DLIST_FOR_EACH_ENTRY_SAFE(endpoint, endpointTmp, &audioUsbDriver->endpointList, struct AudioUsbEndpoint, list) {
        if (endpoint != NULL) {
            DListRemove(&endpoint->list);
            OsalMemFree(endpoint);
        }
    }

    DLIST_FOR_EACH_ENTRY_SAFE(
        audioUsbFormat, audioUsbFormatTmp, &audioUsbDriver->renderUsbFormatList, struct AudioUsbFormat, list) {
        if (audioUsbFormat != NULL) {
            DListRemove(&audioUsbFormat->list);
            OsalMemFree(audioUsbFormat);
        }
    }
    DLIST_FOR_EACH_ENTRY_SAFE(
        audioUsbFormat, audioUsbFormatTmp, &audioUsbDriver->captureUsbFormatList, struct AudioUsbFormat, list) {
        if (audioUsbFormat != NULL) {
            DListRemove(&audioUsbFormat->list);
            OsalMemFree(audioUsbFormat);
        }
    }
}

static int32_t AudioUsbAutoresume(struct AudioUsbDriver *audioUsbDriver)
{
    int32_t err;
    AUDIO_DEVICE_LOG_DEBUG("entry.");

    if (atomic_read(&audioUsbDriver->shutdown)) {
        return -EIO;
    }
    if (atomic_inc_return(&audioUsbDriver->active) != 1) { /* result is greater than or equal to zero */
        return HDF_SUCCESS;
    }
    err = usb_autopm_get_interface(audioUsbDriver->usbIf);
    if (err < HDF_SUCCESS) {
        AUDIO_DEVICE_LOG_ERR("usb_autopm_get_interface failed.");
        /* rollback */
        usb_autopm_put_interface(audioUsbDriver->usbIf);
        atomic_dec(&audioUsbDriver->active);
        return err;
    }

    return HDF_SUCCESS;
}

int32_t AudioUsbDmaBufAlloc(struct PlatformData *data, const enum AudioStreamType streamType)
{
    uint32_t preallocBufSize;
    int32_t ret;
    struct AudioUsbDriver *audioUsbDriver = NULL;

    AUDIO_DEVICE_LOG_DEBUG("entry.");

    if (data == NULL) {
        AUDIO_DEVICE_LOG_ERR("PlatformData is null.");
        return HDF_FAILURE;
    }
    audioUsbDriver = (struct AudioUsbDriver *)data->dmaPrv;
    if (audioUsbDriver == NULL) {
        AUDIO_DEVICE_LOG_ERR("hdfAudioUsbDriver is null.");
        return HDF_FAILURE;
    }

    ret = AudioUsbAutoresume(audioUsbDriver);
    if (ret != 0) {
        AUDIO_DEVICE_LOG_ERR("AudioUsbAutoresume is failed.");
        return HDF_FAILURE;
    }

    if (streamType == AUDIO_CAPTURE_STREAM) {
        if (data->captureBufInfo.virtAddr == NULL) {
            preallocBufSize = data->captureBufInfo.cirBufMax;
            data->captureBufInfo.virtAddr = __vmalloc(preallocBufSize, GFP_KERNEL | __GFP_HIGHMEM);
        }
    } else if (streamType == AUDIO_RENDER_STREAM) {
        if (data->renderBufInfo.virtAddr == NULL) {
            preallocBufSize = data->renderBufInfo.cirBufMax;
            data->renderBufInfo.virtAddr = __vmalloc(preallocBufSize, GFP_KERNEL | __GFP_HIGHMEM);
        }
    } else {
        AUDIO_DEVICE_LOG_ERR("stream Type is invalude.");
        return HDF_FAILURE;
    }

    AUDIO_DEVICE_LOG_INFO("success.");
    return HDF_SUCCESS;
}

int32_t AudioUsbDmaBufFree(struct PlatformData *data, const enum AudioStreamType streamType)
{
    struct AudioUsbDriver *audioUsbDriver = NULL;

    if (data == NULL) {
        AUDIO_DEVICE_LOG_ERR("PlatformData is null.");
        return HDF_FAILURE;
    }
    audioUsbDriver = (struct AudioUsbDriver *)data->dmaPrv;
    if (audioUsbDriver == NULL || audioUsbDriver->usbIf == NULL) {
        AUDIO_DEVICE_LOG_ERR("audioUsbDriver is null.");
        return HDF_FAILURE;
    }

    if (streamType == AUDIO_CAPTURE_STREAM) {
        vfree(data->captureBufInfo.virtAddr);
    } else if (streamType == AUDIO_RENDER_STREAM) {
        vfree(data->renderBufInfo.virtAddr);
    } else {
        AUDIO_DEVICE_LOG_ERR("stream Type is invalude.");
        return HDF_FAILURE;
    }

    AUDIO_DEVICE_LOG_DEBUG("success");
    return HDF_SUCCESS;
}

int32_t AudioUsbDmaRequestChannel(const struct PlatformData *data, const enum AudioStreamType streamType)
{
    (void)data;
    AUDIO_DEVICE_LOG_DEBUG("sucess");
    return HDF_SUCCESS;
}

static void AudioUsbEndpointInit(struct AudioUsbDriver *audioUsbDriver, struct AudioUsbEndpoint *ep,
    struct usb_host_interface *alts, uint32_t *epNum, int type)
{
    struct usb_device *dev = audioUsbDriver->dev;
    struct usb_endpoint_descriptor *epDesc = NULL;

    ep->audioUsbDriver = audioUsbDriver;
    spin_lock_init(&ep->lock);
    ep->type = type;
    ep->endpointNum = *epNum;
    ep->iface = alts->desc.bInterfaceNumber;
    ep->altsetting = alts->desc.bAlternateSetting;
    DListHeadInit(&ep->list);
    INIT_LIST_HEAD(&ep->readyPlaybackUrbs);
    *epNum &= USB_ENDPOINT_NUMBER_MASK;

    epDesc = AudioEndpointDescriptor(alts, USB_ENDPOINT_INDEX_1);
    if (type == AUDIO_USB_ENDPOINT_TYPE_SYNC) {
        if (epDesc->bLength >= USB_DT_ENDPOINT_AUDIO_SIZE && epDesc->bRefresh >= SYNC_REFRESH_1 &&
            epDesc->bRefresh <= SYNC_REFRESH_9) {
            ep->syncInterval = epDesc->bRefresh;
        } else if (AudioUsbGetSpeed(dev) == USB_SPEED_FULL) {
            ep->syncInterval = SYNC_INTERVAL_1;
        } else if (epDesc->bInterval >= SYNC_INTERVAL_1 && epDesc->bInterval <= SYNC_INTERVAL_16) {
            ep->syncInterval = epDesc->bInterval - SYNC_INTERVAL_1;
        } else {
            ep->syncInterval = SYNC_INTERVAL_3;
        }
        ep->syncMaxSize = le16_to_cpu(epDesc->wMaxPacketSize);
    }
}

static struct AudioUsbEndpoint *AudioUsbAddEndpoint(struct AudioUsbDriver *audioUsbDriver,
    struct usb_host_interface *alts, uint32_t epNum, const enum AudioStreamType streamType, int type)
{
    struct usb_device *dev = NULL;
    struct AudioUsbEndpoint *ep = NULL;
    bool isRender;

    if (audioUsbDriver == NULL || alts == NULL) {
        AUDIO_DEVICE_LOG_ERR("input param is null.");
        return NULL;
    }

    if (audioUsbDriver->usbIf == NULL) {
        AUDIO_DEVICE_LOG_ERR("usbIf is null.");
        return NULL;
    }
    dev = audioUsbDriver->dev;
    isRender = streamType == AUDIO_RENDER_STREAM ? true : false;

    mutex_lock(&audioUsbDriver->mutex);

    DLIST_FOR_EACH_ENTRY(ep, &audioUsbDriver->endpointList, struct AudioUsbEndpoint, list) {
        if (ep->endpointNum == epNum && ep->iface == alts->desc.bInterfaceNumber &&
            ep->altsetting == alts->desc.bAlternateSetting) {
            AUDIO_DEVICE_LOG_INFO("Render using endpoint %d in iface %d,%d\n", epNum, ep->iface, ep->altsetting);
            mutex_unlock(&audioUsbDriver->mutex);
            return ep;
        }
    }

    ep = OsalMemCalloc(sizeof(struct AudioUsbEndpoint));
    if (ep == NULL) {
        AUDIO_DEVICE_LOG_ERR("kzalloc failed.");
        mutex_unlock(&audioUsbDriver->mutex);
        return NULL;
    }

    AudioUsbEndpointInit(audioUsbDriver, ep, alts, &epNum, type);
    if (isRender) {
        ep->pipe = usb_sndisocpipe(dev, epNum);
    } else {
        ep->pipe = usb_rcvisocpipe(dev, epNum);
    }

    DListInsertHead(&ep->list, &audioUsbDriver->endpointList);

    ep->isImplicitFeedback = 0;

    mutex_unlock(&audioUsbDriver->mutex);
    return ep;
}

static int32_t AudioUsbGetEndpoint(struct AudioUsbFormat *audioUsbFormat, struct usb_host_interface *alts,
    uint32_t attr, int isRender, uint32_t *epNum)
{
    struct usb_endpoint_descriptor *epDesc = NULL;
    bool epTypeIsoc = false;
    bool epDescAudioSize = false;
    bool usbDirInva = false;

    if ((isRender != 0 && (attr == USB_ENDPOINT_SYNC_SYNC || attr == USB_ENDPOINT_SYNC_ADAPTIVE)) ||
        (isRender == 0 && attr != USB_ENDPOINT_SYNC_ADAPTIVE)) {
        return HDF_SUCCESS;
    }

    epDesc = AudioEndpointDescriptor(alts, USB_ENDPOINT_INDEX_1);
    epTypeIsoc = ((epDesc->bmAttributes & USB_ENDPOINT_XFERTYPE_MASK) != USB_ENDPOINT_XFER_ISOC);
    epDescAudioSize = (epDesc->bLength >= USB_DT_ENDPOINT_AUDIO_SIZE);
    if (epTypeIsoc || (epDescAudioSize && epDesc->bSynchAddress != 0)) {
        AUDIO_DEVICE_LOG_ERR("%d:%d : invalid sync pipe. bmAttributes %02x, bLength %d, bSynchAddress %02x\n",
            audioUsbFormat->iface, audioUsbFormat->altsetting, epDesc->bmAttributes, epDesc->bLength,
            epDesc->bSynchAddress);
        if (isRender != 0 && attr == USB_ENDPOINT_SYNC_NONE) {
            return HDF_SUCCESS;
        }
        return -EINVAL;
    }
    *epNum = epDesc->bEndpointAddress;

    epDesc = AudioEndpointDescriptor(alts, 0);
    epDescAudioSize = (epDesc->bLength >= USB_DT_ENDPOINT_AUDIO_SIZE && epDesc->bSynchAddress != 0);
    usbDirInva = (*epNum != (uint32_t)(epDesc->bSynchAddress | USB_DIR_IN) ||
                  *epNum != (uint32_t)(epDesc->bSynchAddress & ~USB_DIR_IN));
    if (epDescAudioSize && isRender != 0 && usbDirInva) {
        AUDIO_DEVICE_LOG_ERR("%d:%d : invalid sync pipe. isRender %d, ep %02x, bSynchAddress %02x\n",
            audioUsbFormat->iface, audioUsbFormat->altsetting, isRender, *epNum, epDesc->bSynchAddress);
        if (attr == USB_ENDPOINT_SYNC_NONE) {
            return HDF_SUCCESS;
        }
        return -EINVAL;
    }

    return HDF_SUCCESS;
}

static int32_t AudioUsbSetSyncEndpoint(struct AudioUsbDriver *audioUsbDriver, struct AudioUsbFormat *audioUsbFormat,
    const enum AudioStreamType streamType, struct usb_host_interface *alts, struct usb_interface_descriptor *altsd)
{
    uint32_t isRender = streamType;
    uint32_t epNum, attr;
    bool implicitFb;
    struct AudioUsbEndpoint *syncEndpoint = NULL;
    struct usb_endpoint_descriptor *epDesc = NULL;
    int32_t ret;

    attr = audioUsbFormat->epAttr & USB_ENDPOINT_SYNCTYPE;
    if (isRender == AUDIO_RENDER_STREAM) {
        if (attr != USB_ENDPOINT_SYNC_ASYNC) {
            audioUsbDriver->renderSyncEndpoint = NULL;
            audioUsbDriver->renderDataEndpoint->syncMasterEndpoint = NULL;
        }
    } else {
        if (attr != USB_ENDPOINT_SYNC_ADAPTIVE) {
            audioUsbDriver->captureSyncEndpoint = NULL;
            audioUsbDriver->captureDataEndpoint->syncMasterEndpoint = NULL;
        }
    }

    if (altsd->bNumEndpoints < USB_ENDPOINT_INDEX_2) {
        return HDF_SUCCESS;
    }

    ret = AudioUsbGetEndpoint(audioUsbFormat, alts, attr, isRender, &epNum);
    if (ret != HDF_SUCCESS) {
        return ret;
    }
    epDesc = AudioEndpointDescriptor(alts, USB_ENDPOINT_INDEX_1);
    implicitFb = (epDesc->bmAttributes & USB_ENDPOINT_USAGE_MASK) == USB_ENDPOINT_USAGE_IMPLICIT_FB;

    syncEndpoint = AudioUsbAddEndpoint(audioUsbDriver, alts, epNum, streamType,
        implicitFb ? AUDIO_USB_ENDPOINT_TYPE_DATA : AUDIO_USB_ENDPOINT_TYPE_SYNC);
    if (syncEndpoint == NULL) {
        if (isRender && attr == USB_ENDPOINT_SYNC_NONE) {
            return HDF_SUCCESS;
        }
        return -EINVAL;
    }

    if (isRender) {
        audioUsbDriver->renderSyncEndpoint = syncEndpoint;
        audioUsbDriver->renderSyncEndpoint->isImplicitFeedback = implicitFb;
        audioUsbDriver->renderDataEndpoint->syncMasterEndpoint = audioUsbDriver->renderSyncEndpoint;
    } else {
        audioUsbDriver->captureSyncEndpoint = syncEndpoint;
        audioUsbDriver->captureSyncEndpoint->isImplicitFeedback = implicitFb;
        audioUsbDriver->captureDataEndpoint->syncMasterEndpoint = audioUsbDriver->captureSyncEndpoint;
    }
    return HDF_SUCCESS;
}

static int32_t AudioSetFormatSub(struct AudioUsbDriver *audioUsbDriver, struct AudioUsbFormat *audioUsbFormat,
    struct usb_host_interface *alts, struct usb_interface_descriptor *altsd, const enum AudioStreamType streamType)
{
    struct AudioUsbEndpoint *dataEndpoint = NULL;
    int ret;

    dataEndpoint =
        AudioUsbAddEndpoint(audioUsbDriver, alts, audioUsbFormat->endpoint, streamType, AUDIO_USB_ENDPOINT_TYPE_DATA);
    if (dataEndpoint == NULL) {
        AUDIO_DEVICE_LOG_ERR("dataEndpoint is null.");
        return -EINVAL;
    }

    if (streamType == AUDIO_RENDER_STREAM) {
        audioUsbDriver->renderDataEndpoint = dataEndpoint;
    } else {
        audioUsbDriver->captureDataEndpoint = dataEndpoint;
    }

    ret = AudioUsbSetSyncEndpoint(audioUsbDriver, audioUsbFormat, streamType, alts, altsd);
    if (ret < HDF_SUCCESS) {
        AUDIO_DEVICE_LOG_ERR("AudioUsbSetSyncEndpoint failed.");
        return ret;
    }

    if (streamType == AUDIO_RENDER_STREAM) {
        audioUsbDriver->renderUsbFormat = audioUsbFormat;
    } else {
        audioUsbDriver->captureUsbFormat = audioUsbFormat;
    }

    return HDF_SUCCESS;
}

static int32_t AudioSetFormat(
    struct AudioUsbDriver *audioUsbDriver, struct AudioUsbFormat *audioUsbFormat, const enum AudioStreamType streamType)
{
    struct usb_host_interface *alts = NULL;
    struct usb_interface_descriptor *altsd = NULL;
    struct usb_interface *iface = NULL;
    int ret;
    struct usb_device *dev = NULL;

    if (audioUsbDriver == NULL || audioUsbDriver->usbIf == NULL) {
        AUDIO_DEVICE_LOG_ERR("hdfAudioUsbDriver is null.");
        return HDF_FAILURE;
    }
    dev = audioUsbDriver->dev;
    if (dev == NULL) {
        AUDIO_DEVICE_LOG_ERR("usb_device is null.");
        return HDF_FAILURE;
    }

    iface = usb_ifnum_to_if(dev, audioUsbFormat->iface);
    if (iface == NULL) {
        AUDIO_DEVICE_LOG_ERR("iface is null.");
        return -EINVAL;
    }

    alts = usb_altnum_to_altsetting(iface, audioUsbFormat->altsetting);
    if (alts == NULL) {
        return -EINVAL;
    }

    altsd = AudioUsbGetIfaceDesc(alts);
    if (altsd == NULL) {
        AUDIO_DEVICE_LOG_ERR("altsd is null.");
        return HDF_FAILURE;
    }

    /* set interface */
    if (iface->cur_altsetting != alts) {
        ret = usb_set_interface(dev, audioUsbFormat->iface, audioUsbFormat->altsetting);
        if (ret < HDF_SUCCESS) {
            AUDIO_DEVICE_LOG_ERR("usb_set_interface failed.");
            return -EIO;
        }
    }

    ret = AudioSetFormatSub(audioUsbDriver, audioUsbFormat, alts, altsd, streamType);
    if (ret < HDF_SUCCESS) {
        return ret;
    }

    return HDF_SUCCESS;
}

static bool AudioUsbFindFormatSub(struct PcmInfo *pcmInfo, struct AudioUsbFormat *fp, int usbPcmFormat)
{
    if (pcmInfo == NULL || fp == NULL) {
        AUDIO_DEVICE_LOG_ERR("pcmInfo or fp is null.");
        return false;
    }
    if (!(fp->formats & usbPcmFormat)) {
        AUDIO_DEVICE_LOG_WARNING(
            "The current audio stream format is %d. The audio card support format is %d", usbPcmFormat, fp->formats);
        return false;
    }

    if (fp->channels != pcmInfo->channels) {
        AUDIO_DEVICE_LOG_WARNING("The current audio stream channels are %d. The audio card support channels are %d",
            pcmInfo->channels, fp->channels);
        return false;
    }

    if (pcmInfo->rate < fp->rateMin || pcmInfo->rate > fp->rateMax) {
        AUDIO_DEVICE_LOG_WARNING(
            "The current audio stream rate is %d. The audio card support rateMax is %d rateMin is %d", pcmInfo->rate,
            fp->rateMax, fp->rateMin);
        return false;
    }

    return true;
}

static struct AudioUsbFormat *SeekAudioUsbListFindFormat(struct DListHead *audioUsbFormatList,
    struct PcmInfo *pcmInfo, int usbPcmFormat, const enum AudioStreamType streamType)
{
    struct AudioUsbFormat *fp = NULL, *found = NULL;
    uint32_t curAttr = 0, attr, i;
    DLIST_FOR_EACH_ENTRY(fp, audioUsbFormatList, struct AudioUsbFormat, list) {
        if (!AudioUsbFindFormatSub(pcmInfo, fp, usbPcmFormat)) {
            continue;
        }

        if (!(fp->rates & AUDIO_USB_PCM_RATE_CONTINUOUS)) {
            for (i = 0; i < fp->nrRates; i++) {
                if (fp->rateTable[i] == pcmInfo->rate) {
                    break;
                }
            }
            if (i >= fp->nrRates) {
                continue;
            }
        }
        attr = fp->epAttr & USB_ENDPOINT_SYNCTYPE;

        if (found == NULL) {
            found = fp;
            curAttr = attr;
            continue;
        }

        if (attr != curAttr) {
            if ((attr == USB_ENDPOINT_SYNC_ASYNC && streamType == AUDIO_RENDER_STREAM) ||
                (attr == USB_ENDPOINT_SYNC_ADAPTIVE && streamType == AUDIO_CAPTURE_STREAM)) {
                continue;
            }
            if ((curAttr == USB_ENDPOINT_SYNC_ASYNC && streamType == AUDIO_RENDER_STREAM) ||
                (curAttr == USB_ENDPOINT_SYNC_ADAPTIVE && streamType == AUDIO_CAPTURE_STREAM)) {
                found = fp;
                curAttr = attr;
                continue;
            }
        }
        /* find the format with the largest max. packet size */
        if (fp->maxPackSize > found->maxPackSize) {
            found = fp;
            curAttr = attr;
        }
    }
    return found;
}

static struct AudioUsbFormat *AudioUsbFindFormat(struct AudioUsbDriver *audioUsbDriver, struct PcmInfo *pcmInfo,
    int usbPcmFormat, const enum AudioStreamType streamType)
{
    struct DListHead *audioUsbFormatList = NULL;

    if (streamType == AUDIO_RENDER_STREAM) {
        audioUsbFormatList = &audioUsbDriver->renderUsbFormatList;
    } else {
        audioUsbFormatList = &audioUsbDriver->captureUsbFormatList;
    }

    if (DListIsEmpty(audioUsbFormatList)) {
        ADM_LOG_ERR("audioUsbFormatList is empty.");
        return NULL;
    }
    return SeekAudioUsbListFindFormat(audioUsbFormatList, pcmInfo, usbPcmFormat, streamType);
}

int32_t AudioUsbDmaConfigChannel(const struct PlatformData *data, const enum AudioStreamType streamType)
{
    int32_t ret;
    struct AudioUsbDriver *audioUsbDriver = NULL;
    struct AudioUsbFormat *audioUsbFormat = NULL;
    struct PcmInfo *pcmInfo = NULL;
    uint32_t usbPcmFormat;

    AUDIO_DEVICE_LOG_INFO("entry.");

    if (data == NULL) {
        AUDIO_DEVICE_LOG_ERR("input param is null.");
        return HDF_FAILURE;
    }
    audioUsbDriver = (struct AudioUsbDriver *)data->dmaPrv;
    if (audioUsbDriver == NULL) {
        AUDIO_DEVICE_LOG_ERR("audioUsbDriver is null.");
        return HDF_FAILURE;
    }

    if (streamType == AUDIO_RENDER_STREAM) {
        audioUsbDriver->renderPcmInfo = data->renderPcmInfo;
        audioUsbDriver->renderBufInfo = data->renderBufInfo;
        audioUsbDriver->renderBufInfo.periodSize = data->renderBufInfo.periodSize / data->renderPcmInfo.frameSize;
        audioUsbDriver->renderBufInfo.cirBufSize = data->renderBufInfo.cirBufSize / data->renderPcmInfo.frameSize;
        pcmInfo = &audioUsbDriver->renderPcmInfo;
    } else {
        audioUsbDriver->capturePcmInfo = data->capturePcmInfo;
        audioUsbDriver->captureBufInfo = data->captureBufInfo;
        audioUsbDriver->captureBufInfo.periodSize = data->captureBufInfo.periodSize / data->capturePcmInfo.frameSize;
        audioUsbDriver->captureBufInfo.cirBufSize = data->captureBufInfo.cirBufSize / data->capturePcmInfo.frameSize;
        pcmInfo = &audioUsbDriver->capturePcmInfo;
    }

    ret = AudioUSBPcmFormat(pcmInfo->bitWidth, pcmInfo->isBigEndian, &usbPcmFormat);
    if (ret != HDF_SUCCESS) {
        AUDIO_DEVICE_LOG_ERR("AudioUSBPcmFormat failed.");
        return HDF_FAILURE;
    }

    audioUsbFormat = AudioUsbFindFormat(audioUsbDriver, pcmInfo, usbPcmFormat, streamType);
    if (audioUsbFormat == NULL) {
        AUDIO_DEVICE_LOG_ERR("audioUsbFormat is null.");
        return HDF_FAILURE;
    }

    ret = AudioSetFormat(audioUsbDriver, audioUsbFormat, streamType);
    if (ret < HDF_SUCCESS) {
        AUDIO_DEVICE_LOG_ERR("AudioSetFormat failed.");
        return HDF_FAILURE;
    }
    audioUsbDriver->needSetupEp = true;

    AUDIO_DEVICE_LOG_INFO("success");
    return HDF_SUCCESS;
}

static int32_t BytesToFrames(uint32_t frameBits, uint32_t size, uint32_t *pointer)
{
    if (pointer == NULL || frameBits == 0) {
        AUDIO_DEVICE_LOG_ERR("input para is error.");
        return HDF_FAILURE;
    }
    *pointer = size / frameBits;
    return HDF_SUCCESS;
}

int32_t AudioUsbPcmPointer(struct PlatformData *data, const enum AudioStreamType streamType, uint32_t *pointer)
{
    uint32_t hwptrDone;
    struct AudioUsbDriver *audioUsbDriver = NULL;

    if (data == NULL || pointer == NULL) {
        AUDIO_DEVICE_LOG_ERR("PlatformData is null.");
        return HDF_FAILURE;
    }
    audioUsbDriver = (struct AudioUsbDriver *)data->dmaPrv;
    if (audioUsbDriver == NULL) {
        AUDIO_DEVICE_LOG_ERR("audioUsbDriver is null.");
        return HDF_FAILURE;
    }

    if (atomic_read(&audioUsbDriver->shutdown)) {
        AUDIO_DEVICE_LOG_ERR("UsbDriver is shutdown.");
        return HDF_FAILURE; // AUDIO_USB_PCM_POS_XRUN
    }

    if (streamType == AUDIO_RENDER_STREAM) {
        hwptrDone = audioUsbDriver->renderHwptr;
        *pointer = hwptrDone / data->renderPcmInfo.frameSize;
    } else {
        hwptrDone = audioUsbDriver->captureHwptr;
        *pointer = hwptrDone / data->capturePcmInfo.frameSize;
    }

    return HDF_SUCCESS;
}

/*
 * configure endpoint params
 *
 * called  during initial setup and upon resume
 */
static int32_t AudioUsbConfigureEndpoint(const struct PlatformData *data, const enum AudioStreamType streamType)
{
    int32_t ret = 0;
    struct AudioUsbEndpoint *dataEndpoint = NULL;
    struct AudioUsbEndpoint *syncEndpoint = NULL;
    struct AudioUsbDriver *audioUsbDriver = NULL;
    struct PcmInfo pcmInfo;
    struct CircleBufInfo bufInfo;

    audioUsbDriver = (struct AudioUsbDriver *)data->dmaPrv;
    if (audioUsbDriver == NULL) {
        AUDIO_DEVICE_LOG_ERR("hdfAudioUsbDriver is null.");
        return HDF_FAILURE;
    }

    if (streamType == AUDIO_RENDER_STREAM) {
        dataEndpoint = audioUsbDriver->renderDataEndpoint;
        if (dataEndpoint == NULL) {
            AUDIO_DEVICE_LOG_ERR("renderDataEndpoint is null.");
            return HDF_FAILURE;
        }
        pcmInfo = data->renderPcmInfo;
        bufInfo = data->renderBufInfo;
        syncEndpoint = audioUsbDriver->renderSyncEndpoint;
        /* format changed */
        ret =
            AudioUsbEndpointSetParams(dataEndpoint, &pcmInfo, &bufInfo, audioUsbDriver->renderUsbFormat, syncEndpoint);
        if (ret < HDF_SUCCESS) {
            AUDIO_DEVICE_LOG_ERR("AudioUsbEndpointSetParams is failed %d.", ret);
            return ret;
        }
    } else {
        dataEndpoint = audioUsbDriver->captureDataEndpoint;
        if (dataEndpoint == NULL) {
            AUDIO_DEVICE_LOG_ERR("captureDataEndpoint is null.");
            return HDF_FAILURE;
        }

        pcmInfo = data->capturePcmInfo;
        bufInfo = data->captureBufInfo;
        syncEndpoint = audioUsbDriver->captureSyncEndpoint;
        /* format changed */
        ret =
            AudioUsbEndpointSetParams(dataEndpoint, &pcmInfo, &bufInfo, audioUsbDriver->captureUsbFormat, syncEndpoint);
        if (ret < HDF_SUCCESS) {
            AUDIO_DEVICE_LOG_ERR("AudioUsbEndpointSetParams is failed %d.", ret);
            return ret;
        }
    }

    return ret;
}

static int32_t AudioUsbStartEndpoints(struct AudioUsbDriver *audioUsbDriver, struct AudioUsbEndpoint *dataEndpoint,
    struct AudioUsbEndpoint *syncEndpoint, unsigned long *flags)
{
    int32_t err;

    if (audioUsbDriver == NULL || dataEndpoint == NULL) {
        AUDIO_DEVICE_LOG_ERR("input param is null.");
        return -EINVAL;
    }
    if (!test_and_set_bit(SUBSTREAM_FLAG_DATA_EP_STARTED, flags)) {
        err = AudioUsbEndpointStart(dataEndpoint);
        if (err < HDF_SUCCESS) {
            clear_bit(SUBSTREAM_FLAG_DATA_EP_STARTED, flags);
            AUDIO_DEVICE_LOG_ERR("AudioUsbEndpointStart failed.");
            return err;
        }
    }

    if (syncEndpoint && !test_and_set_bit(SUBSTREAM_FLAG_SYNC_EP_STARTED, flags)) {
        if (dataEndpoint->iface != syncEndpoint->iface || dataEndpoint->altsetting != syncEndpoint->altsetting) {
            err = usb_set_interface(audioUsbDriver->dev, syncEndpoint->iface, syncEndpoint->altsetting);
            if (err < HDF_SUCCESS) {
                clear_bit(SUBSTREAM_FLAG_SYNC_EP_STARTED, flags);
                AUDIO_DEVICE_LOG_ERR(
                    "%d:%d: cannot set interface (%d)\n", syncEndpoint->iface, syncEndpoint->altsetting, err);
                return -EIO;
            }
        }

        syncEndpoint->syncSlaveEndpoint = dataEndpoint;
        err = AudioUsbEndpointStart(syncEndpoint);
        if (err < HDF_SUCCESS) {
            clear_bit(SUBSTREAM_FLAG_SYNC_EP_STARTED, flags);
            AUDIO_DEVICE_LOG_ERR("AudioUsbEndpointStart is failed.");
            return err;
        }
    }

    return HDF_SUCCESS;
}

static int32_t AudioUsbBytesToFrames(
    const struct PlatformData *data, struct AudioUsbDriver *audioUsbDriver, const enum AudioStreamType streamType)
{
    int32_t ret;
    if (streamType == AUDIO_RENDER_STREAM) {
        if (audioUsbDriver->renderDataEndpoint == NULL) {
            AUDIO_DEVICE_LOG_ERR("renderDataEndpoint is null.");
            return HDF_FAILURE;
        }
        /* some unit conversions in runtime */
        ret = BytesToFrames(data->renderPcmInfo.frameSize, audioUsbDriver->renderDataEndpoint->maxPackSize,
            &audioUsbDriver->renderDataEndpoint->maxFrameSize);
        if (ret != HDF_SUCCESS) {
            return ret;
        }
        ret = BytesToFrames(data->renderPcmInfo.frameSize, audioUsbDriver->renderDataEndpoint->curPackSize,
            &audioUsbDriver->renderDataEndpoint->curframesize);
        if (ret != HDF_SUCCESS) {
            return ret;
        }
    } else {
        if (audioUsbDriver->captureDataEndpoint == NULL) {
            AUDIO_DEVICE_LOG_ERR("captureDataEndpoint is null.");
            return HDF_FAILURE;
        }
        /* some unit conversions in runtime */
        ret = BytesToFrames(data->capturePcmInfo.frameSize, audioUsbDriver->captureDataEndpoint->maxPackSize,
            &audioUsbDriver->captureDataEndpoint->maxFrameSize);
        if (ret != HDF_SUCCESS) {
            return ret;
        }

        ret = BytesToFrames(data->capturePcmInfo.frameSize, audioUsbDriver->captureDataEndpoint->curPackSize,
            &audioUsbDriver->captureDataEndpoint->curframesize);
        if (ret != HDF_SUCCESS) {
            return ret;
        }
    }
    return HDF_SUCCESS;
}

static int32_t AudioUsbSetupEp(const struct PlatformData *data, struct AudioUsbDriver *audioUsbDriver,
    struct AudioUsbFormat *audioUsbFormat, const enum AudioStreamType streamType, uint32_t rate)
{
    struct usb_host_interface *alts = NULL;
    struct usb_interface *iface = NULL;
    struct usb_device *dev = NULL;
    int32_t ret;

    dev = interface_to_usbdev(audioUsbDriver->usbIf);
    if (dev == NULL) {
        AUDIO_DEVICE_LOG_ERR("usb dev is null.");
        return HDF_FAILURE;
    }

    iface = usb_ifnum_to_if(dev, audioUsbFormat->iface);
    if (iface == NULL) {
        AUDIO_DEVICE_LOG_ERR("usb iface is null.");
        return HDF_FAILURE;
    }

    alts = &iface->altsetting[audioUsbFormat->altsetIdx];
    if (alts == NULL) {
        AUDIO_DEVICE_LOG_ERR("usb alts is null.");
        return HDF_FAILURE;
    }

    ret = AudioUsbInitSampleRate(audioUsbDriver, audioUsbFormat->iface, alts, audioUsbFormat, rate);
    if (ret < HDF_SUCCESS) {
        AUDIO_DEVICE_LOG_ERR("AudioUsbInitSampleRate failed.");
        return ret;
    }

    ret = AudioUsbConfigureEndpoint(data, streamType);
    if (ret < HDF_SUCCESS) {
        AUDIO_DEVICE_LOG_ERR("AudioUsbConfigureEndpoint failed.");
        return ret;
    }
    return HDF_SUCCESS;
}

int32_t AudioUsbDmaPrep(const struct PlatformData *data, const enum AudioStreamType streamType)
{
    int32_t ret;
    struct AudioUsbDriver *audioUsbDriver = NULL;
    struct AudioUsbFormat *audioUsbFormat = NULL;
    uint32_t rate;
    AUDIO_DEVICE_LOG_DEBUG("entry.");

    if (data == NULL) {
        AUDIO_DEVICE_LOG_ERR("data is null.");
        return HDF_ERR_INVALID_OBJECT;
    }
    audioUsbDriver = (struct AudioUsbDriver *)data->dmaPrv;
    if (audioUsbDriver == NULL) {
        AUDIO_DEVICE_LOG_ERR("audioUsbDriver is null.");
        return HDF_FAILURE;
    }

    if (streamType == AUDIO_RENDER_STREAM) {
        audioUsbFormat = audioUsbDriver->renderUsbFormat;
        rate = data->renderPcmInfo.rate;
    } else {
        audioUsbFormat = audioUsbDriver->captureUsbFormat;
        rate = data->capturePcmInfo.rate;
    }

    // Find the matching format and set the interface,
    // which will set the synchronization endpoint, that is, this is synchronous transmission
    ret = AudioSetFormat(audioUsbDriver, audioUsbFormat, streamType);
    if (ret < 0) {
        AUDIO_DEVICE_LOG_ERR("AudioSetFormat failed.");
        return HDF_FAILURE;
    }

    if (audioUsbDriver->needSetupEp) {
        ret = AudioUsbSetupEp(data, audioUsbDriver, audioUsbFormat, streamType, rate);
        if (ret != HDF_SUCCESS) {
            return HDF_FAILURE;
        }
        audioUsbDriver->needSetupEp = false;
    }

    ret = AudioUsbBytesToFrames(data, audioUsbDriver, streamType);
    if (ret != HDF_SUCCESS) {
        return HDF_FAILURE;
    }

    /* reset the pointer */
    audioUsbDriver->renderHwptr = 0;
    audioUsbDriver->captureHwptr = 0;
    audioUsbDriver->renderTransferDone = 0;
    audioUsbDriver->captureTransferDone = 0;

    return HDF_SUCCESS;
}

static void AudioUsbCopyToUrb(
    struct AudioUsbDriver *audioUsbDriver, struct urb *urb, int offset, int stride, uint32_t bytes)
{
    uint32_t bytes1;

    if (audioUsbDriver == NULL) {
        AUDIO_DEVICE_LOG_ERR("hdfAudioUsbDriver is null.");
        return;
    }

    if (audioUsbDriver->renderHwptr + bytes > audioUsbDriver->renderBufInfo.cirBufSize * stride) {
        /* err, the transferred area goes over buffer boundary. */
        bytes1 = audioUsbDriver->renderBufInfo.cirBufSize * stride - audioUsbDriver->renderHwptr;
        (void)memcpy_s(urb->transfer_buffer + offset, bytes1,
            (char *)audioUsbDriver->renderBufInfo.virtAddr + audioUsbDriver->renderHwptr, bytes1);
        (void)memcpy_s(urb->transfer_buffer + offset + bytes1, bytes - bytes1,
            (char *)audioUsbDriver->renderBufInfo.virtAddr, bytes - bytes1);
    } else {
        (void)memcpy_s(urb->transfer_buffer + offset, bytes,
            (char *)audioUsbDriver->renderBufInfo.virtAddr + audioUsbDriver->renderHwptr, bytes);
    }
    audioUsbDriver->renderHwptr += bytes;

    if (audioUsbDriver->renderHwptr >= audioUsbDriver->renderBufInfo.cirBufSize * stride) {
        audioUsbDriver->renderHwptr -= audioUsbDriver->renderBufInfo.cirBufSize * stride;
    }
}

static void AudioUsbUpdataEndpointPacket(struct AudioUsbDriver *audioUsbDriver, struct AudioUsbUrbCtx *ctx,
    struct AudioUsbEndpoint *ep, struct urb *urb, uint32_t *frames)
{
    uint32_t i;
    uint32_t periodElapsed = 0;
    uint32_t counts;

    if (ctx->packets > MAX_PACKS_HS) {
        return;
    }

    for (i = 0; i < ctx->packets; i++) {
        if (ctx->packetSize[i]) {
            counts = ctx->packetSize[i];
        } else if (ep->syncMasterEndpoint) {
            counts = AudioUsbEndpointSlaveNextPacketSize(ep);
        } else {
            counts = AudioUsbEndpointNextPacketSize(ep);
        }

        /* set up descriptor */
        urb->iso_frame_desc[i].offset = *frames * ep->stride;
        urb->iso_frame_desc[i].length = counts * ep->stride;
        *frames += counts;
        urb->number_of_packets++;

        audioUsbDriver->renderTransferDone += counts;

        if (audioUsbDriver->renderTransferDone >= audioUsbDriver->renderBufInfo.periodSize) {
            audioUsbDriver->renderTransferDone -= audioUsbDriver->renderBufInfo.periodSize;
            audioUsbDriver->frameLimit = 0;
            periodElapsed = 1;
        }
        /* finish at the period boundary or after enough frames */
        if ((periodElapsed != 0 || audioUsbDriver->renderTransferDone >= audioUsbDriver->frameLimit) &&
            !AudioUsbEndpointImplicitFeedbackSink(ep)) {
            break;
        }
    }
}

static void AudioUsbPreparePlaybackUrb(struct AudioUsbDriver *audioUsbDriver, struct urb *urb)
{
    struct AudioUsbEndpoint *ep = NULL;
    uint32_t frames, bytes;
    uint32_t stride;
    unsigned long flags;

    if (audioUsbDriver == NULL || urb == NULL) {
        AUDIO_DEVICE_LOG_ERR("input param is null.");
        return;
    }

    ep = audioUsbDriver->renderDataEndpoint;
    if (ep == NULL) {
        AUDIO_DEVICE_LOG_ERR("ep is null.");
        return;
    }

    stride = audioUsbDriver->renderPcmInfo.frameSize;
    frames = 0;
    urb->number_of_packets = 0;
    spin_lock_irqsave(&audioUsbDriver->lock, flags);
    audioUsbDriver->frameLimit += ep->maxUrbFrames;

    AudioUsbUpdataEndpointPacket(audioUsbDriver, urb->context, ep, urb, &frames);
    bytes = frames * ep->stride;

    /* usual PCM */
    AudioUsbCopyToUrb(audioUsbDriver, urb, 0, stride, bytes);

    spin_unlock_irqrestore(&audioUsbDriver->lock, flags);
    urb->transfer_buffer_length = bytes;
}

/*
 * process after playback data complete
 * - decrease the delay count again
 */
static void AudioUsbRetirePlaybackUrb(struct AudioUsbDriver *audioUsbDriver, struct urb *urb)
{
    (void)audioUsbDriver;
    (void)urb;
    return;
}

int32_t AudioUsbDmaSubmit(const struct PlatformData *data, const enum AudioStreamType streamType)
{
    AUDIO_DEVICE_LOG_DEBUG("success");
    return HDF_SUCCESS;
}

static void AudioUsbRetireCaptureUrb(struct AudioUsbDriver *audioUsbDriver, struct urb *urb)
{
    uint32_t frameSize, frames, bytes, oldptr, offset;
    uint32_t i;
    unsigned long flags;
    unsigned char *capPoint = NULL;

    frameSize = audioUsbDriver->capturePcmInfo.frameSize;

    for (i = 0; i < urb->number_of_packets; i++) {
        offset = urb->iso_frame_desc[i].offset;
        capPoint = (unsigned char *)urb->transfer_buffer + offset;

        bytes = urb->iso_frame_desc[i].actual_length;
        frames = bytes / frameSize;

        if (bytes % audioUsbDriver->capturePcmInfo.frameSize != 0) {
            bytes = frames * frameSize;
            AUDIO_DEVICE_LOG_DEBUG("capture bytes = %d", bytes);
        }

        spin_lock_irqsave(&audioUsbDriver->lock, flags);
        oldptr = audioUsbDriver->captureHwptr;
        audioUsbDriver->captureHwptr += bytes;
        if (audioUsbDriver->captureHwptr >= audioUsbDriver->captureBufInfo.cirBufSize * frameSize)
            audioUsbDriver->captureHwptr -= audioUsbDriver->captureBufInfo.cirBufSize * frameSize;
        frames = (bytes + (oldptr % frameSize)) / frameSize;
        audioUsbDriver->captureTransferDone += frames;
        spin_unlock_irqrestore(&audioUsbDriver->lock, flags);

        /* copy a data chunk */
        if (oldptr + bytes > audioUsbDriver->captureBufInfo.cirBufSize * frameSize) {
            uint32_t bytes1 = audioUsbDriver->captureBufInfo.cirBufSize * frameSize - oldptr;
            (void)memcpy_s((char *)audioUsbDriver->captureBufInfo.virtAddr + oldptr, bytes1, capPoint, bytes1);
            (void)memcpy_s(
                (char *)audioUsbDriver->captureBufInfo.virtAddr, bytes - bytes1, capPoint + bytes1, bytes - bytes1);
        } else {
            (void)memcpy_s((char *)audioUsbDriver->captureBufInfo.virtAddr + oldptr, bytes, capPoint, bytes);
        }
    }
}

int32_t AudioUsbDmaPending(struct PlatformData *data, const enum AudioStreamType streamType)
{
    int32_t ret;
    AUDIO_DEVICE_LOG_INFO("entry.");

    ret = AudioUsbDmaResume(data, streamType);
    if (ret != HDF_SUCCESS) {
        AUDIO_DEVICE_LOG_ERR("AudioUsbDmaResume is failed.");
        return HDF_FAILURE;
    }

    AUDIO_DEVICE_LOG_INFO("success");
    return HDF_SUCCESS;
}

static void AudioUsbStopEndpoints(struct AudioUsbDriver *audioUsbDriver, struct AudioUsbEndpoint *dataEndpoint,
    struct AudioUsbEndpoint *syncEndpoint, unsigned long *flags)
{
    if (test_and_clear_bit(SUBSTREAM_FLAG_SYNC_EP_STARTED, flags)) {
        AudioUsbEndpointStop(syncEndpoint);
    }

    if (test_and_clear_bit(SUBSTREAM_FLAG_DATA_EP_STARTED, flags)) {
        AudioUsbEndpointStop(dataEndpoint);
    }
}

int32_t AudioUsbDmaPause(struct PlatformData *data, const enum AudioStreamType streamType)
{
    struct AudioUsbDriver *audioUsbDriver = NULL;
    AUDIO_DEVICE_LOG_INFO("entry.");

    if (data == NULL) {
        AUDIO_DEVICE_LOG_ERR("PlatformData is null.");
        return HDF_ERR_INVALID_OBJECT;
    }
    audioUsbDriver = (struct AudioUsbDriver *)data->dmaPrv;
    if (audioUsbDriver == NULL) {
        AUDIO_DEVICE_LOG_ERR("audio usb driver is null.");
        return HDF_FAILURE;
    }

    if (streamType == AUDIO_RENDER_STREAM) {
        audioUsbDriver->renderDataEndpoint->AudioPrepareDataUrb = NULL;
        /* keep AudioRetireDataUrb for delay calculation */
        audioUsbDriver->renderDataEndpoint->AudioRetireDataUrb = AudioUsbRetirePlaybackUrb;
        AudioUsbStopEndpoints(audioUsbDriver, audioUsbDriver->renderDataEndpoint, audioUsbDriver->renderSyncEndpoint,
            &audioUsbDriver->renderFlags);
    } else {
        audioUsbDriver->captureDataEndpoint->AudioRetireDataUrb = NULL;
        AudioUsbStopEndpoints(audioUsbDriver, audioUsbDriver->captureDataEndpoint, audioUsbDriver->captureSyncEndpoint,
            &audioUsbDriver->captureFlags);
    }

    audioUsbDriver->running = false;

    AUDIO_DEVICE_LOG_DEBUG("success");
    return HDF_SUCCESS;
}
int32_t AudioUsbDmaResume(const struct PlatformData *data, const enum AudioStreamType streamType)
{
    struct AudioUsbDriver *audioUsbDriver = NULL;
    int32_t ret;
    AUDIO_DEVICE_LOG_DEBUG("entry.");

    if (data == NULL) {
        AUDIO_DEVICE_LOG_ERR("PlatformData is null.");
        return HDF_ERR_INVALID_OBJECT;
    }
    audioUsbDriver = (struct AudioUsbDriver *)data->dmaPrv;
    if (audioUsbDriver == NULL) {
        AUDIO_DEVICE_LOG_ERR("usb driver is null.");
        return HDF_FAILURE;
    }
    if (streamType == AUDIO_RENDER_STREAM) {
        audioUsbDriver->renderDataEndpoint->AudioPrepareDataUrb = AudioUsbPreparePlaybackUrb;
        audioUsbDriver->renderDataEndpoint->AudioRetireDataUrb = AudioUsbRetirePlaybackUrb;
        ret = AudioUsbStartEndpoints(audioUsbDriver, audioUsbDriver->renderDataEndpoint,
            audioUsbDriver->renderSyncEndpoint, &audioUsbDriver->renderFlags);
        if (ret < 0) {
            AUDIO_DEVICE_LOG_ERR("AudioUsbStartEndpoints failed.");
            return ret;
        }
    } else {
        audioUsbDriver->captureDataEndpoint->AudioRetireDataUrb = AudioUsbRetireCaptureUrb;
        ret = AudioUsbStartEndpoints(audioUsbDriver, audioUsbDriver->captureDataEndpoint,
            audioUsbDriver->captureSyncEndpoint, &audioUsbDriver->captureFlags);
        if (ret < 0) {
            AUDIO_DEVICE_LOG_ERR("AudioUsbStartEndpoints failed.");
            return ret;
        }
    }

    audioUsbDriver->triggerTstampPendingUpdate = true;
    audioUsbDriver->running = true;

    AUDIO_DEVICE_LOG_DEBUG("success");
    return HDF_SUCCESS;
}
