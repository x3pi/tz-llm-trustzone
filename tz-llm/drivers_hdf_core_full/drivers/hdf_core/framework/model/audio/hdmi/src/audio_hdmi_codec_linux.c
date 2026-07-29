/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include "audio_hdmi_codec_linux.h"
#include <linux/module.h>
#include <linux/of_graph.h>
#include <linux/string.h>
#include "audio_driver_log.h"

#define HDF_LOG_TAG HDF_AUDIO_HDMI

#define AUDIO_HDMI_CODEC_DRV_NAME "hdmi-audio-codec"

static struct HdmiCodecPriv *g_hdmiCodecPriv = NULL;

struct HdmiCodecPriv *AudioGetHdmiCodecPriv(void)
{
    return g_hdmiCodecPriv;
}

static int32_t HdmiCodecProbe(struct platform_device *pdev)
{
    struct hdmi_codec_pdata *hdmiCodecData = NULL;
    struct device *dev = NULL;
    struct HdmiCodecPriv *hdmiCodecPriv = NULL;
    int32_t daiCount;

    AUDIO_DRIVER_LOG_INFO("entry");
    if (pdev == NULL) {
        AUDIO_DRIVER_LOG_ERR("input param is null");
        return -EINVAL;
    }
    dev = &pdev->dev;
    hdmiCodecData = pdev->dev.platform_data;
    if (hdmiCodecData == NULL) {
        AUDIO_DRIVER_LOG_ERR("No hdmi codec data");
        return -EINVAL;
    }

    daiCount = hdmiCodecData->i2s + hdmiCodecData->spdif;
    if (daiCount < 1) { // count minimum 1
        AUDIO_DRIVER_LOG_ERR("daiCount < 1, daiCount is %d", daiCount);
        return -EINVAL;
    }

    hdmiCodecPriv = devm_kzalloc(dev, sizeof(struct HdmiCodecPriv), GFP_KERNEL);
    if (hdmiCodecPriv == NULL) {
        AUDIO_DRIVER_LOG_ERR("hdmiCodecPriv devm_kzalloc failed");
        return -ENOMEM;
    }

    hdmiCodecPriv->hdmiCodecData = *hdmiCodecData;
    mutex_init(&hdmiCodecPriv->lock);
    hdmiCodecPriv->dev = dev;

    g_hdmiCodecPriv = hdmiCodecPriv;
    return 0;
}

static struct platform_driver g_audioHdmiCodecDriver = {
    .driver.name = AUDIO_HDMI_CODEC_DRV_NAME,
    .probe = HdmiCodecProbe,
};

module_platform_driver(g_audioHdmiCodecDriver);
