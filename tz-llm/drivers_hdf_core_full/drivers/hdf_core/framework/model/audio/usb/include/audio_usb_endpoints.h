/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#ifndef AUDIO_USB_ENDPOINTS_H
#define AUDIO_USB_ENDPOINTS_H

#include <linux/gfp.h>
#include <linux/init.h>
#include <linux/ratelimit.h>
#include <linux/slab.h>
#include <linux/usb.h>
#include <linux/usb/audio.h>

#include "audio_host.h"
#include "audio_usb_linux.h"

int32_t AudioUsbEndpointStart(struct AudioUsbEndpoint *ep);
void AudioUsbEndpointStop(struct AudioUsbEndpoint *ep);

uint32_t AudioUsbEndpointNextPacketSize(struct AudioUsbEndpoint *ep);
uint32_t AudioUsbEndpointSlaveNextPacketSize(struct AudioUsbEndpoint *ep);
bool AudioUsbEndpointImplicitFeedbackSink(struct AudioUsbEndpoint *ep);
int32_t AudioUsbEndpointSetParams(struct AudioUsbEndpoint *ep, struct PcmInfo *pcmInfo, struct CircleBufInfo *bufInfo,
    struct AudioUsbFormat *audioUsbFormat, struct AudioUsbEndpoint *syncEp);
int32_t AudioUSBPcmFormat(uint32_t bitWidth, bool isBigEndian, uint32_t *usbPcmFormat);

#endif