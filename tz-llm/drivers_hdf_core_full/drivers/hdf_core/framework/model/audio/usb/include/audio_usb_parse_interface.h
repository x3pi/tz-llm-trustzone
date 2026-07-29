/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#ifndef AUDIO_USB_PARSE_INTERFACE_H
#define AUDIO_USB_PARSE_INTERFACE_H

#include "audio_usb_linux.h"

int32_t AudioUsbParseAudioInterface(struct AudioUsbDriver *audioUsbDriver, int32_t ifaceNo);

int32_t AudioUsbInitSampleRate(struct AudioUsbDriver *audioUsbDriver, int32_t iface, struct usb_host_interface *alts,
    struct AudioUsbFormat *audioUsbFormat, int32_t rate);

#endif
