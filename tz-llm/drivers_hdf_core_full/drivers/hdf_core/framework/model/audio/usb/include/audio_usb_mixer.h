/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#ifndef AUDIO_USB_MIXER_H
#define AUDIO_USB_MIXER_H

#include "audio_usb_linux.h"

int32_t AudioUsbCreateMixer(struct AudioUsbDriver *audioUsbDriver, int32_t ctrlIf, int32_t ignoreError);

#endif