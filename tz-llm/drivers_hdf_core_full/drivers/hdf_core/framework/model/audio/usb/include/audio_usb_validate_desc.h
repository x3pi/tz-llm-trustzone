/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#ifndef AUDIO_USB_VALIDATE_DESC_H
#define AUDIO_USB_VALIDATE_DESC_H

#include <linux/usb.h>

struct AudioUsbCtlMsgParam {
    uint32_t pipe;
    uint8_t request;
    uint8_t requestType;
    uint16_t value;
    uint16_t index;
};
uint32_t AudioUsbCombineTriple(uint8_t *bytes);
struct usb_interface_descriptor *AudioUsbGetIfaceDesc(struct usb_host_interface *interface);
struct usb_endpoint_descriptor *AudioEndpointDescriptor(struct usb_host_interface *alts, uint32_t epIndex);
enum usb_device_speed AudioUsbGetSpeed(struct usb_device *dev);

uint32_t AudioUsbCombineBytes(uint8_t *bytes, int32_t size);
void *AudioUsbFindDesc(void *descStart, int32_t descLen, void *after, uint8_t dtype);
void *AudioUsbFindCsintDesc(void *buf, int32_t bufLen, void *after, uint8_t dsubType);
int32_t AudioUsbCtlMsg(struct usb_device *dev, struct AudioUsbCtlMsgParam *usbCtlMsgParam, void *data, uint16_t size);
uint8_t AudioUsbParseDataInterval(struct usb_device *usbDev, struct usb_host_interface *alts);
bool AudioUsbValidateAudioDesc(void *ptr, int32_t protocol);

#endif
