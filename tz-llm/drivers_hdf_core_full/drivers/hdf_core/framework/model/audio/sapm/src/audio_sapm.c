/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include "audio_sapm.h"
#include "audio_driver_log.h"
#include "osal_io.h"
#include "osal_time.h"
#include "osal_thread.h"

#define HDF_LOG_TAG HDF_AUDIO_SAPM

#define SAPM_POLL_TIME     10      /* 10s */
#define SAPM_SLEEP_TIMES    ((3 * 60) / (SAPM_POLL_TIME)) /* sleep times */
#define SAPM_THREAD_NAME   60
#define SAPM_POWER_DOWN    0
#define SAPM_POWER_UP      1
#define SAPM_STACK_SIZE    10000

#define CONNECT_CODEC_PIN 1
#define UNCONNECT_CODEC_PIN 0

#define EXIST_EXTERNAL_WIDGET 1
#define UNEXIST_EXTERNAL_WIDGET 1

#define CONNECT_SINK_AND_SOURCE 1
#define UNCONNECT_SINK_AND_SOURCE 0

uint32_t g_cardNum = 0;
static void AudioSapmTimerCallback(struct AudioCard *audioCard);
static int32_t AudioSapmRefreshTime(struct AudioCard *audioCard, bool bRefresh);

/* power up sequences */
static int32_t g_audioSapmPowerUpSeq[] = {
    [AUDIO_SAPM_PRE] = 0,             /* 0 is audio sapm power up sequences */
    [AUDIO_SAPM_SUPPLY] = 1,          /* 1 is audio sapm power up sequences */
    [AUDIO_SAPM_MICBIAS] = 2,         /* 2 is audio sapm power up sequences */
    [AUDIO_SAPM_AIF_IN] = 3,          /* 3 is audio sapm power up sequences */
    [AUDIO_SAPM_AIF_OUT] = 3,         /* 3 is audio sapm power up sequences */
    [AUDIO_SAPM_MIC] = 4,             /* 4 is audio sapm power up sequences */
    [AUDIO_SAPM_MUX] = 5,             /* 5 is audio sapm power up sequences */
    [AUDIO_SAPM_VIRT_MUX] = 5,        /* 5 is audio sapm power up sequences */
    [AUDIO_SAPM_VALUE_MUX] = 5,       /* 5 is audio sapm power up sequences */
    [AUDIO_SAPM_DAC] = 6,             /* 6 is audio sapm power up sequences */
    [AUDIO_SAPM_MIXER] = 7,           /* 7 is audio sapm power up sequences */
    [AUDIO_SAPM_MIXER_NAMED_CTRL] = 7, /* 7 is audio sapm power up sequences */
    [AUDIO_SAPM_PGA] = 8,             /* 8 is audio sapm power up sequences */
    [AUDIO_SAPM_ADC] = 9,             /* 9 is audio sapm power up sequences */
    [AUDIO_SAPM_OUT_DRV] = 10,        /* 10 is audio sapm power up sequences */
    [AUDIO_SAPM_HP] = 10,             /* 10 is audio sapm power up sequences */
    [AUDIO_SAPM_SPK] = 10,            /* 10 is audio sapm power up sequences */
    [AUDIO_SAPM_POST] = 11,           /* 11 is audio sapm power up sequences */
};

/* power down sequences */
static int32_t g_audioSapmPowerDownSeq[] = {
    [AUDIO_SAPM_PRE] = 0,             /* 0 is audio sapm power down sequences */
    [AUDIO_SAPM_ADC] = 1,             /* 1 is audio sapm power down sequences */
    [AUDIO_SAPM_HP] = 2,              /* 2 is audio sapm power down sequences */
    [AUDIO_SAPM_SPK] = 2,             /* 2 is audio sapm power down sequences */
    [AUDIO_SAPM_OUT_DRV] = 2,         /* 2 is audio sapm power down sequences */
    [AUDIO_SAPM_PGA] = 4,             /* 4 is audio sapm power down sequences */
    [AUDIO_SAPM_MIXER_NAMED_CTRL] = 5, /* 5 is audio sapm power down sequences */
    [AUDIO_SAPM_MIXER] = 5,           /* 5 is audio sapm power down sequences */
    [AUDIO_SAPM_DAC] = 6,             /* 6 is audio sapm power down sequences */
    [AUDIO_SAPM_MIC] = 7,             /* 7 is audio sapm power down sequences */
    [AUDIO_SAPM_MICBIAS] = 8,         /* 8 is audio sapm power down sequences */
    [AUDIO_SAPM_MUX] = 9,             /* 9 is audio sapm power down sequences */
    [AUDIO_SAPM_VIRT_MUX] = 9,        /* 9 is audio sapm power down sequences */
    [AUDIO_SAPM_VALUE_MUX] = 9,       /* 9 is audio sapm power down sequences */
    [AUDIO_SAPM_AIF_IN] = 10,         /* 10 is audio sapm power down sequences */
    [AUDIO_SAPM_AIF_OUT] = 10,        /* 10 is audio sapm power down sequences */
    [AUDIO_SAPM_SUPPLY] = 11,         /* 11 is audio sapm power down sequences */
    [AUDIO_SAPM_POST] = 12,           /* 12 is audio sapm power down sequences */
};

static int32_t ConnectedInputEndPoint(const struct AudioSapmComponent *sapmComponent)
{
    struct AudioSapmpath *path = NULL;
    int32_t count = 0;
    const int32_t endPointVal = 1;

    if (sapmComponent == NULL) {
        ADM_LOG_ERR("input param sapmComponent is NULL.");
        return HDF_FAILURE;
    }

    switch (sapmComponent->sapmType) {
        case AUDIO_SAPM_DAC:
        case AUDIO_SAPM_AIF_IN:
        case AUDIO_SAPM_INPUT:
        case AUDIO_SAPM_MIC:
        case AUDIO_SAPM_LINE:
            return endPointVal;
        default:
            break;
    }

    DLIST_FOR_EACH_ENTRY(path, &sapmComponent->sources, struct AudioSapmpath, listSink) {
        if ((path->source != NULL) && (path->connect == CONNECT_SINK_AND_SOURCE)) {
            count += ConnectedInputEndPoint(path->source);
        }
    }
    return count;
}

static int32_t ConnectedOutputEndPoint(const struct AudioSapmComponent *sapmComponent)
{
    struct AudioSapmpath *path = NULL;
    int32_t count = 0;
    const int32_t endPointVal = 1;

    if (sapmComponent == NULL) {
        ADM_LOG_ERR("input param sapmComponent is NULL.");
        return HDF_FAILURE;
    }

    switch (sapmComponent->sapmType) {
        case AUDIO_SAPM_ADC:
        case AUDIO_SAPM_AIF_OUT:
        case AUDIO_SAPM_OUTPUT:
        case AUDIO_SAPM_HP:
        case AUDIO_SAPM_SPK:
        case AUDIO_SAPM_LINE:
            return endPointVal;
        default:
            break;
    }

    DLIST_FOR_EACH_ENTRY(path, &sapmComponent->sinks, struct AudioSapmpath, listSource) {
        if ((path->sink != NULL) && (path->connect == CONNECT_SINK_AND_SOURCE)) {
            count += ConnectedOutputEndPoint(path->sink);
        }
    }
    return count;
}

static int32_t AudioSapmGenericCheckPower(const struct AudioSapmComponent *sapmComponent)
{
    int32_t input;
    int32_t output;

    if (sapmComponent == NULL) {
        ADM_LOG_ERR("input param cpt is NULL.");
        return HDF_FAILURE;
    }

    input = ConnectedInputEndPoint(sapmComponent);
    if (input == HDF_FAILURE) {
        ADM_LOG_ERR("input endpoint fail!");
        return HDF_FAILURE;
    }
    output = ConnectedOutputEndPoint(sapmComponent);
    if (output == HDF_FAILURE) {
        ADM_LOG_ERR("output endpoint fail!");
        return HDF_FAILURE;
    }

    if ((input == 0) || (output == 0)) {
        ADM_LOG_DEBUG("component %s is not in a complete path.", sapmComponent->componentName);
        return SAPM_POWER_DOWN;
    }
    return SAPM_POWER_UP;
}

static int32_t AudioSapmAdcPowerClock(struct AudioSapmComponent *sapmComponent)
{
    if (sapmComponent == NULL) {
        ADM_LOG_ERR("param sapmComponent is NULL.");
        return HDF_ERR_INVALID_PARAM;
    }

    ADM_LOG_INFO("%s standby mode entry!", sapmComponent->componentName);
    return HDF_SUCCESS;
}

static int32_t AudioSapmDacPowerClock(struct AudioSapmComponent *sapmComponent)
{
    if (sapmComponent == NULL) {
        ADM_LOG_ERR("param sapmComponent is NULL.");
        return HDF_ERR_INVALID_PARAM;
    }

    ADM_LOG_INFO("%s standby mode entry!", sapmComponent->componentName);
    return HDF_SUCCESS;
}

static int32_t AudioSapmAdcCheckPower(const struct AudioSapmComponent *sapmComponent)
{
    int32_t input;

    if (sapmComponent == NULL) {
        ADM_LOG_ERR("input param sapmComponent is NULL.");
        return HDF_FAILURE;
    }

    if (sapmComponent->active == 0) {
        input = AudioSapmGenericCheckPower(sapmComponent);
    } else {
        input = ConnectedInputEndPoint(sapmComponent);
    }
    if (input == HDF_FAILURE) {
        ADM_LOG_ERR("input endpoint fail!");
        return HDF_FAILURE;
    }
    return input;
}

static int32_t AudioSapmDacCheckPower(const struct AudioSapmComponent *sapmComponent)
{
    int32_t output;

    if (sapmComponent == NULL) {
        ADM_LOG_ERR("input sapmComponent cpt is NULL.");
        return HDF_FAILURE;
    }

    if (sapmComponent->active == 0) {
        output = AudioSapmGenericCheckPower(sapmComponent);
    } else {
        output = ConnectedOutputEndPoint(sapmComponent);
    }
    if (output == HDF_FAILURE) {
        ADM_LOG_ERR("output endpoint fail!");
        return HDF_FAILURE;
    }
    return output;
}

static void AudioSampCheckPowerCallback(struct AudioSapmComponent *sapmComponent)
{
    if (sapmComponent == NULL) {
        ADM_LOG_ERR("input param cpt is NULL.");
        return;
    }

    switch (sapmComponent->sapmType) {
        case AUDIO_SAPM_ANALOG_SWITCH:
        case AUDIO_SAPM_MIXER:
        case AUDIO_SAPM_MIXER_NAMED_CTRL:
            sapmComponent->PowerCheck = AudioSapmGenericCheckPower;
            break;
        case AUDIO_SAPM_MUX:
        case AUDIO_SAPM_VIRT_MUX:
        case AUDIO_SAPM_VALUE_MUX:
            sapmComponent->PowerCheck = AudioSapmGenericCheckPower;
            break;
        case AUDIO_SAPM_ADC:
        case AUDIO_SAPM_AIF_OUT:
            sapmComponent->PowerCheck = AudioSapmAdcCheckPower;
            break;
        case AUDIO_SAPM_DAC:
        case AUDIO_SAPM_AIF_IN:
            sapmComponent->PowerCheck = AudioSapmDacCheckPower;
            break;
        case AUDIO_SAPM_PGA:
        case AUDIO_SAPM_OUT_DRV:
        case AUDIO_SAPM_INPUT:
        case AUDIO_SAPM_OUTPUT:
        case AUDIO_SAPM_MICBIAS:
        case AUDIO_SAPM_SPK:
        case AUDIO_SAPM_HP:
        case AUDIO_SAPM_MIC:
        case AUDIO_SAPM_LINE:
            sapmComponent->PowerCheck = AudioSapmGenericCheckPower;
            break;
        default:
            sapmComponent->PowerCheck = AudioSapmGenericCheckPower;
            break;
    }

    return;
}

static void AudioSampPowerClockCallback(struct AudioSapmComponent *sapmComponent)
{
    if (sapmComponent == NULL) {
        ADM_LOG_ERR("input param cpt is NULL.");
        return;
    }

    switch (sapmComponent->sapmType) {
        case AUDIO_SAPM_ANALOG_SWITCH:
        case AUDIO_SAPM_MIXER:
        case AUDIO_SAPM_MIXER_NAMED_CTRL:
            sapmComponent->PowerClockOp = NULL;
            break;
        case AUDIO_SAPM_MUX:
        case AUDIO_SAPM_VIRT_MUX:
        case AUDIO_SAPM_VALUE_MUX:
            sapmComponent->PowerClockOp = NULL;
            break;
        case AUDIO_SAPM_ADC:
        case AUDIO_SAPM_AIF_OUT:
            sapmComponent->PowerClockOp = AudioSapmAdcPowerClock;
            break;
        case AUDIO_SAPM_DAC:
        case AUDIO_SAPM_AIF_IN:
            sapmComponent->PowerClockOp = AudioSapmDacPowerClock;
            break;
        case AUDIO_SAPM_PGA:
        case AUDIO_SAPM_OUT_DRV:
        case AUDIO_SAPM_INPUT:
        case AUDIO_SAPM_OUTPUT:
        case AUDIO_SAPM_MICBIAS:
        case AUDIO_SAPM_MIC:
        case AUDIO_SAPM_SPK:
        case AUDIO_SAPM_HP:
        case AUDIO_SAPM_LINE:
            sapmComponent->PowerClockOp = NULL;
            break;
        default:
            sapmComponent->PowerClockOp = NULL;
            break;
    }

    return;
}

static int32_t AudioSapmNewComponent(struct AudioCard *audioCard, const struct AudioSapmComponent *component)
{
    struct AudioSapmComponent *sapmComponent = NULL;

    if ((audioCard == NULL || audioCard->rtd == NULL)) {
        ADM_LOG_ERR("input params check error: audioCard is NULL.");
        return HDF_FAILURE;
    }
    if (component == NULL || component->componentName == NULL) {
        ADM_LOG_ERR("params component or component->componentName is null.");
        return HDF_FAILURE;
    }

    sapmComponent = (struct AudioSapmComponent *)OsalMemCalloc(sizeof(struct AudioSapmComponent));
    if (sapmComponent == NULL) {
        ADM_LOG_ERR("malloc cpt fail!");
        return HDF_FAILURE;
    }
    if (memcpy_s(sapmComponent, sizeof(struct AudioSapmComponent),
        component, sizeof(struct AudioSapmComponent)) != EOK) {
        ADM_LOG_ERR("memcpy cpt fail!");
        OsalMemFree(sapmComponent);
        return HDF_FAILURE;
    }

    sapmComponent->componentName = (char *)OsalMemCalloc(strlen(component->componentName) + 1);
    if (sapmComponent->componentName == NULL) {
        ADM_LOG_ERR("malloc cpt->componentName fail!");
        OsalMemFree(sapmComponent);
        return HDF_FAILURE;
    }
    if (memcpy_s(sapmComponent->componentName, strlen(component->componentName) + 1,
        component->componentName, strlen(component->componentName) + 1) != EOK) {
        ADM_LOG_ERR("memcpy cpt->componentName fail!");
        OsalMemFree(sapmComponent->componentName);
        OsalMemFree(sapmComponent);
        return HDF_FAILURE;
    }

    sapmComponent->codec = audioCard->rtd->codec;
    sapmComponent->kcontrolsNum = component->kcontrolsNum;
    sapmComponent->active = 0;
    AudioSampCheckPowerCallback(sapmComponent);
    AudioSampPowerClockCallback(sapmComponent);

    DListHeadInit(&sapmComponent->sources);
    DListHeadInit(&sapmComponent->sinks);
    DListHeadInit(&sapmComponent->list);
    DListHeadInit(&sapmComponent->dirty);
    DListInsertHead(&sapmComponent->list, &audioCard->components);

    sapmComponent->connected = CONNECT_CODEC_PIN;

    return HDF_SUCCESS;
}

int32_t AudioSapmNewComponents(struct AudioCard *audioCard,
    const struct AudioSapmComponent *component, int32_t cptMaxNum)
{
    int32_t i;
    int32_t ret;

    if (audioCard == NULL) {
        ADM_LOG_ERR("input params check error: audioCard is NULL.");
        return HDF_FAILURE;
    }
    if (component == NULL) {
        ADM_LOG_ERR("input params check error: component is NULL.");
        return HDF_FAILURE;
    }

    for (i = 0; i < cptMaxNum; i++) {
        ret = AudioSapmNewComponent(audioCard, component);
        if (ret != HDF_SUCCESS) {
            ADM_LOG_ERR("AudioSapmNewComponent fail!");
            return HDF_FAILURE;
        }
        component++;
    }

    return HDF_SUCCESS;
}

static void MuxSetPathStatus(const struct AudioSapmComponent *sapmComponent, struct AudioSapmpath *path,
    const struct AudioEnumKcontrol *enumKtl, int32_t i)
{
    int32_t ret;
    uint32_t val = 0;
    int32_t curValue;
    uint32_t shift;

    if (sapmComponent == NULL || sapmComponent->codec == NULL) {
        ADM_LOG_ERR("input MuxSet params check error");
        return;
    }
    if (path == NULL || path->name == NULL) {
        ADM_LOG_ERR("input params check error: path is NULL.");
        return;
    }
    if (enumKtl == NULL) {
        ADM_LOG_ERR("input params check error: enumKtl is NULL.");
        return;
    }

    shift = enumKtl->shiftLeft;
    ret = AudioCodecReadReg(sapmComponent->codec, enumKtl->reg, &val);
    if (ret != HDF_SUCCESS) {
        ADM_LOG_ERR("codec read reg fail!");
        return;
    }

    curValue = (val >> shift) & enumKtl->mask;
    path->connect = UNCONNECT_SINK_AND_SOURCE;

    if (enumKtl->texts != NULL) {
        for (i = 0; i < enumKtl->max; i++) {
            if (enumKtl->texts[i] == NULL) {
                ADM_LOG_ERR("enumKtl->texts[%d] is NULL", i);
                continue;
            }

            if ((strcmp(path->name, enumKtl->texts[i]) == 0) && curValue == i) {
                path->connect = CONNECT_SINK_AND_SOURCE;
            }
        }
    } else {
        if (curValue) {
            path->connect = CONNECT_SINK_AND_SOURCE;
        }
    }

    return;
}

static void MuxValueSetPathStatus(const struct AudioSapmComponent *sapmComponent, struct AudioSapmpath *path,
    const struct AudioEnumKcontrol *enumKtl, int32_t i)
{
    int32_t ret;
    uint32_t val = 0;
    uint32_t item;
    uint32_t shift;
    if (sapmComponent == NULL || sapmComponent->codec == NULL) {
        ADM_LOG_ERR("input muxValueSet params check error");
        return;
    }
    if (path == NULL || path->name == NULL) {
        ADM_LOG_ERR("input MuxValueSet params check error: path is NULL.");
        return;
    }
    if (enumKtl == NULL) {
        ADM_LOG_ERR("input MuxValueSet params check error: enumKtl is NULL.");
        return;
    }
    shift = enumKtl->shiftLeft;
    ret = AudioCodecReadReg(sapmComponent->codec, enumKtl->reg, &val);
    if (ret != HDF_SUCCESS) {
        ADM_LOG_ERR("muxValueSet read reg fail!");
        return;
    }

    val = (val >> shift) & enumKtl->mask;
    path->connect = UNCONNECT_SINK_AND_SOURCE;

    if (enumKtl->values != NULL && enumKtl->texts != NULL) {
        for (item = 0; item < enumKtl->max; item++) {
            if (val == enumKtl->values[item]) {
                break;
            }
        }

        for (i = 0; i < enumKtl->max; i++) {
            if (enumKtl->texts[i] == NULL) {
                continue;
            }
            if ((strcmp(path->name, enumKtl->texts[i]) == 0) && item == i) {
                path->connect = CONNECT_SINK_AND_SOURCE;
            }
        }
    } else {
        if (val) {
            path->connect = CONNECT_SINK_AND_SOURCE;
        }
    }

    return;
}

static void MixerSetPathStatus(const struct AudioSapmComponent *sapmComponent, struct AudioSapmpath *path,
    const struct AudioMixerControl *mixerCtrl)
{
    int32_t ret;
    uint32_t reg;
    uint32_t mask;
    uint32_t shift;
    uint32_t invert;
    uint32_t curValue = 0;

    if (sapmComponent == NULL) {
        ADM_LOG_ERR("input params check error: sapmComponent is NULL.");
        return;
    }
    if (path == NULL) {
        ADM_LOG_ERR("input params check error: path is NULL.");
        return;
    }
    if (mixerCtrl == NULL) {
        ADM_LOG_ERR("input params check error: mixerCtrl is NULL.");
        return;
    }

    reg = mixerCtrl->reg;
    shift = mixerCtrl->shift;
    mask = mixerCtrl->mask;
    invert = mixerCtrl->invert;

    if (sapmComponent->codec != NULL) {
        ret = AudioCodecReadReg(sapmComponent->codec, reg, &curValue);
        if (ret != HDF_SUCCESS) {
            ADM_LOG_ERR("read reg fail!");
            return;
        }
    } else {
        ADM_LOG_ERR("codec is null!");
        return;
    }

    curValue = (curValue >> shift) & mask;
    if ((invert && !curValue) || (!invert && curValue)) {
        path->connect = CONNECT_SINK_AND_SOURCE;
    } else {
        path->connect = UNCONNECT_SINK_AND_SOURCE;
    }

    return;
}

static int32_t AudioSapmSetPathStatus(const struct AudioSapmComponent *sapmComponent,
    struct AudioSapmpath *path, int32_t i)
{
    if ((sapmComponent == NULL) || (path == NULL)) {
        ADM_LOG_ERR("input params check error");
        return HDF_FAILURE;
    }
    switch (sapmComponent->sapmType) {
        case AUDIO_SAPM_MIXER:
        case AUDIO_SAPM_ANALOG_SWITCH:
        case AUDIO_SAPM_MIXER_NAMED_CTRL:
            MixerSetPathStatus(sapmComponent, path,
                (struct AudioMixerControl *)((volatile uintptr_t)sapmComponent->kcontrolNews[i].privateValue));
            break;
        case AUDIO_SAPM_MUX:
            MuxSetPathStatus(sapmComponent, path,
                (struct AudioEnumKcontrol *)((volatile uintptr_t)sapmComponent->kcontrolNews[i].privateValue), i);
            break;
        case AUDIO_SAPM_VALUE_MUX:
            MuxValueSetPathStatus(sapmComponent, path,
                (struct AudioEnumKcontrol *)((volatile uintptr_t)sapmComponent->kcontrolNews[i].privateValue), i);
            break;
        default:
            path->connect = CONNECT_SINK_AND_SOURCE;
            break;
    }

    return HDF_SUCCESS;
}

static int32_t AudioSapmConnectMux(struct AudioCard *audioCard,
    struct AudioSapmComponent *source, struct AudioSapmComponent *sink,
    struct AudioSapmpath *path, const char *controlName)
{
    int32_t i;
    struct AudioEnumKcontrol *enumKtl = NULL;

    if ((audioCard == NULL) || (source == NULL) || (sink == NULL) || (path == NULL) || (controlName == NULL)) {
        ADM_LOG_ERR("input params check error");
        return HDF_FAILURE;
    }

    if (sink->kcontrolNews == NULL) {
        ADM_LOG_ERR("input params sink kcontrolNews is null.");
        return HDF_FAILURE;
    }
    enumKtl = (struct AudioEnumKcontrol *)&sink->kcontrolNews[0].privateValue;
    if (enumKtl == NULL || enumKtl->texts == NULL) {
        ADM_LOG_ERR("kcontrolNews privateValue is null.");
        return HDF_FAILURE;
    }

    for (i = 0; i < enumKtl->max; i++) {
        if (strcmp(controlName, enumKtl->texts[i]) == 0) {
            DListInsertHead(&path->list, &audioCard->paths);
            DListInsertHead(&path->listSink, &sink->sources);
            DListInsertHead(&path->listSource, &source->sinks);
            path->name = (char*)enumKtl->texts[i];
            AudioSapmSetPathStatus(sink, path, i);

            return HDF_SUCCESS;
        }
    }

    return HDF_FAILURE;
}

static int32_t AudioSapmConnectMixer(struct AudioCard *audioCard,
    struct AudioSapmComponent *source, struct AudioSapmComponent *sink,
    struct AudioSapmpath *path, const char *controlName)
{
    int32_t i;

    if ((audioCard == NULL) || (source == NULL) || (sink == NULL) || (path == NULL) || (controlName == NULL)) {
        ADM_LOG_ERR("input params check error");
        return HDF_FAILURE;
    }

    for (i = 0; i < sink->kcontrolsNum; i++) {
        if (sink->kcontrolNews[i].name == NULL) {
            continue;
        }

        if (strcmp(controlName, sink->kcontrolNews[i].name) == 0) {
            path->name = (char *)OsalMemCalloc(strlen(sink->kcontrolNews[i].name) + 1);
            if (path->name == NULL) {
                ADM_LOG_ERR("malloc path->name fail!");
                return HDF_FAILURE;
            }
            if (memcpy_s(path->name, strlen(sink->kcontrolNews[i].name) + 1, sink->kcontrolNews[i].name,
                strlen(sink->kcontrolNews[i].name) + 1) != EOK) {
                OsalMemFree(path->name);
                ADM_LOG_ERR("memcpy cpt->componentName fail!");
                return HDF_FAILURE;
            }
            DListInsertHead(&path->list, &audioCard->paths);
            DListInsertHead(&path->listSink, &sink->sources);
            DListInsertHead(&path->listSource, &source->sinks);

            AudioSapmSetPathStatus(sink, path, i);

            return HDF_SUCCESS;
        }
    }

    return HDF_FAILURE;
}

static int32_t AudioSampStaticOrDynamicPath(struct AudioCard *audioCard,
    struct AudioSapmComponent *source, struct AudioSapmComponent *sink,
    struct AudioSapmpath *path, const struct AudioSapmRoute *route)
{
    int32_t ret;

    if (route->control == NULL) {
        DListInsertHead(&path->list, &audioCard->paths);
        DListInsertHead(&path->listSink, &sink->sources);
        DListInsertHead(&path->listSource, &source->sinks);
        path->connect = CONNECT_SINK_AND_SOURCE;
        return HDF_SUCCESS;
    }

    switch (sink->sapmType) {
        case AUDIO_SAPM_MUX:
        case AUDIO_SAPM_VIRT_MUX:
        case AUDIO_SAPM_VALUE_MUX:
            ret = AudioSapmConnectMux(audioCard, source, sink, path, route->control);
            if (ret != HDF_SUCCESS) {
                ADM_LOG_ERR("connect mux fail!");
                return HDF_FAILURE;
            }
            break;
        case AUDIO_SAPM_ANALOG_SWITCH:
        case AUDIO_SAPM_MIXER:
        case AUDIO_SAPM_MIXER_NAMED_CTRL:
        case AUDIO_SAPM_PGA:
        case AUDIO_SAPM_SPK:
            ret = AudioSapmConnectMixer(audioCard, source, sink, path, route->control);
            if (ret != HDF_SUCCESS) {
                ADM_LOG_ERR("connect mixer fail!");
                return HDF_FAILURE;
            }
            break;
        case AUDIO_SAPM_HP:
        case AUDIO_SAPM_MIC:
        case AUDIO_SAPM_LINE:
            DListInsertHead(&path->list, &audioCard->paths);
            DListInsertHead(&path->listSink, &sink->sources);
            DListInsertHead(&path->listSource, &source->sinks);
            path->connect = CONNECT_SINK_AND_SOURCE;
            break;
        default:
            DListInsertHead(&path->list, &audioCard->paths);
            DListInsertHead(&path->listSink, &sink->sources);
            DListInsertHead(&path->listSource, &source->sinks);
            path->connect = CONNECT_SINK_AND_SOURCE;
            break;
    }

    return HDF_SUCCESS;
}

static void AudioSampExtComponentsCheck(struct AudioSapmComponent *cptSource, struct AudioSapmComponent *cptSink)
{
    if ((cptSource == NULL) || (cptSink == NULL)) {
        ADM_LOG_ERR("input params check error");
        return;
    }

    /* check for external components */
    if (cptSink->sapmType == AUDIO_SAPM_INPUT) {
        if (cptSource->sapmType == AUDIO_SAPM_MICBIAS || cptSource->sapmType == AUDIO_SAPM_MIC ||
            cptSource->sapmType == AUDIO_SAPM_LINE || cptSource->sapmType == AUDIO_SAPM_OUTPUT) {
            cptSink->external = EXIST_EXTERNAL_WIDGET;
        }
    }
    if (cptSource->sapmType == AUDIO_SAPM_OUTPUT) {
        if (cptSink->sapmType == AUDIO_SAPM_SPK || cptSink->sapmType == AUDIO_SAPM_HP ||
            cptSink->sapmType == AUDIO_SAPM_LINE || cptSink->sapmType == AUDIO_SAPM_INPUT) {
            cptSource->external = EXIST_EXTERNAL_WIDGET;
        }
    }

    return;
}

static int32_t AudioSapmAddRoute(struct AudioCard *audioCard, const struct AudioSapmRoute *route)
{
    struct AudioSapmpath *path = NULL;
    struct AudioSapmComponent *cptSource = NULL;
    struct AudioSapmComponent *cptSink = NULL;
    struct AudioSapmComponent *sapmComponent = NULL;
    int32_t ret;

    if (route == NULL || route->source == NULL || route->sink == NULL) {
        ADM_LOG_ERR("input params check error: route is NULL.");
        return HDF_FAILURE;
    }

    DLIST_FOR_EACH_ENTRY(sapmComponent, &audioCard->components, struct AudioSapmComponent, list) {
        if (sapmComponent->componentName == NULL) {
            continue;
        }
        if ((cptSource == NULL) && (strcmp(sapmComponent->componentName, route->source) == 0)) {
            cptSource = sapmComponent;
            continue;
        }
        if ((cptSink == NULL) && (strcmp(sapmComponent->componentName, route->sink) == 0)) {
            cptSink = sapmComponent;
        }
        if ((cptSource != NULL) && (cptSink != NULL)) {
            break;
        }
    }
    if ((cptSource == NULL) || (cptSink == NULL)) {
        ADM_LOG_ERR("find component fail!");
        return HDF_FAILURE;
    }

    path = (struct AudioSapmpath *)OsalMemCalloc(sizeof(struct AudioSapmpath));
    if (path == NULL) {
        ADM_LOG_ERR("malloc path fail!");
        return HDF_FAILURE;
    }
    path->source = cptSource;
    path->sink = cptSink;
    DListHeadInit(&path->list);
    DListHeadInit(&path->listSink);
    DListHeadInit(&path->listSource);

    /* check for external components */
    AudioSampExtComponentsCheck(cptSource, cptSink);

    ret = AudioSampStaticOrDynamicPath(audioCard, cptSource, cptSink, path, route);
    if (ret != HDF_SUCCESS) {
        OsalMemFree(path);
        ADM_LOG_ERR("static or dynamic path fail!");
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

int32_t AudioSapmAddRoutes(struct AudioCard *audioCard, const struct AudioSapmRoute *route, int32_t routeMaxNum)
{
    int32_t i;
    int32_t ret;

    if (audioCard == NULL) {
        ADM_LOG_ERR("input params check error: audioCard is NULL.");
        return HDF_FAILURE;
    }
    if (route == NULL) {
        ADM_LOG_ERR("input params check error: route is NULL.");
        return HDF_FAILURE;
    }

    for (i = 0; i < routeMaxNum; i++) {
        ret = AudioSapmAddRoute(audioCard, route);
        if (ret != HDF_SUCCESS) {
            ADM_LOG_ERR("AudioSapmAddRoute failed!");
            return HDF_FAILURE;
        }
        route++;
    }
    return HDF_SUCCESS;
}

static int32_t AudioSapmNewMixerControls(
    const struct AudioSapmComponent *sapmComponent, struct AudioCard *audioCard)
{
    struct AudioSapmpath *path = NULL;
    int32_t i;

    if (sapmComponent == NULL || sapmComponent->kcontrols == NULL) {
        ADM_LOG_ERR("input params check error: sapmComponent is NULL.");
        return HDF_FAILURE;
    }
    if (audioCard == NULL) {
        ADM_LOG_ERR("input params check error: audioCard is NULL.");
        return HDF_FAILURE;
    }

    for (i = 0; i < sapmComponent->kcontrolsNum; i++) {
        DLIST_FOR_EACH_ENTRY(path, &sapmComponent->sources, struct AudioSapmpath, listSink) {
            if (path->name == NULL || sapmComponent->kcontrolNews == NULL
                || sapmComponent->kcontrolNews[i].name == NULL) {
                continue;
            }

            if (strcmp(path->name, sapmComponent->kcontrolNews[i].name) != 0) {
                continue;
            }

            path->kcontrol = AudioAddControl(audioCard, &sapmComponent->kcontrolNews[i]);
            if (path->kcontrol == NULL) {
                ADM_LOG_ERR("add control fail!");
                return HDF_FAILURE;
            }
            sapmComponent->kcontrols[i] = path->kcontrol;
            DListInsertHead(&sapmComponent->kcontrols[i]->list, &audioCard->controls);
        }
    }

    return HDF_SUCCESS;
}

static int32_t AudioSapmNewMuxControls(struct AudioSapmComponent *sapmComponent, struct AudioCard *audioCard)
{
    struct AudioKcontrol *kctrl = NULL;

    if (sapmComponent == NULL || sapmComponent->kcontrolNews == NULL || audioCard == NULL) {
        ADM_LOG_ERR("input param is NULL.");
        return HDF_FAILURE;
    }

    if (sapmComponent->kcontrolsNum != 1) {
        ADM_LOG_ERR("incorrect number of controls.");
        return HDF_FAILURE;
    }

    kctrl = AudioAddControl(audioCard, &sapmComponent->kcontrolNews[0]);
    if (kctrl == NULL) {
        ADM_LOG_ERR("add control fail!");
        return HDF_FAILURE;
    }

    if (sapmComponent->kcontrols == NULL) {
        OsalMemFree(kctrl);
        kctrl = NULL;
        ADM_LOG_ERR("sapmComponent->kcontrols is NULL!");
        return HDF_FAILURE;
    }
    sapmComponent->kcontrols[0] = kctrl;
    DListInsertHead(&sapmComponent->kcontrols[0]->list, &audioCard->controls);

    return HDF_SUCCESS;
}

static void AudioSapmPowerSeqInsert(struct AudioSapmComponent *newSapmComponent,
    struct DListHead *list, int8_t isPowerUp)
{
    struct AudioSapmComponent *sapmComponent = NULL;
    int32_t *seq = {0};

    if (newSapmComponent == NULL || list == NULL || newSapmComponent->componentName == NULL) {
        ADM_LOG_ERR("input param newCpt is NULL.");
        return;
    }

    if (isPowerUp) {
        seq = g_audioSapmPowerUpSeq;
    } else {
        seq = g_audioSapmPowerDownSeq;
    }

    DLIST_FOR_EACH_ENTRY(sapmComponent, list, struct AudioSapmComponent, powerList) {
        if ((seq[newSapmComponent->sapmType] - seq[sapmComponent->sapmType]) < 0) {
            DListInsertTail(&newSapmComponent->powerList, &sapmComponent->powerList);
            return;
        }
    }
    DListInsertTail(&newSapmComponent->powerList, list);

    ADM_LOG_DEBUG("[%s] success.", newSapmComponent->componentName);
    return;
}

static void AudioSapmSetPower(struct AudioCard *audioCard, struct AudioSapmComponent *sapmComponent,
    uint8_t power, struct DListHead *upList, struct DListHead *downList)
{
    struct AudioSapmpath *path = NULL;

    if (sapmComponent == NULL) {
        ADM_LOG_ERR("input param sapmComponent is NULL.");
        return;
    }

    DLIST_FOR_EACH_ENTRY(path, &sapmComponent->sources, struct AudioSapmpath, listSink) {
        if (path->source != NULL) {
            if ((path->source->power != power) && path->connect) {
                if (DListIsEmpty(&path->source->dirty)) {
                    DListInsertTail(&path->source->dirty, &audioCard->sapmDirty);
                }
            }
        }
    }
    DLIST_FOR_EACH_ENTRY(path, &sapmComponent->sinks, struct AudioSapmpath, listSource) {
        if (path->sink != NULL) {
            if ((path->sink->power != power) && path->connect) {
                if (DListIsEmpty(&path->sink->dirty)) {
                    DListInsertTail(&path->sink->dirty, &audioCard->sapmDirty);
                }
            }
        }
    }

    if (power) {
        AudioSapmPowerSeqInsert(sapmComponent, upList, power);
    } else {
        AudioSapmPowerSeqInsert(sapmComponent, downList, power);
    }

    return;
}

static void AudioSapmPowerUpSeqRun(const struct DListHead *list)
{
    uint32_t val;
    struct AudioMixerControl mixerControl;
    struct AudioSapmComponent *sapmComponent = NULL;
    ADM_LOG_DEBUG("entry!");
    if (list == NULL) {
        ADM_LOG_ERR("input param list is NULL.");
        return;
    }

    DLIST_FOR_EACH_ENTRY(sapmComponent, list, struct AudioSapmComponent, powerList) {
        if (sapmComponent->power == SAPM_POWER_DOWN) {
            val = SAPM_POWER_UP;
            if (sapmComponent->invert) {
                val = !val;
            }
            sapmComponent->power = SAPM_POWER_UP;
            if (sapmComponent->reg != AUDIO_NO_SAPM_REG) {
                mixerControl.reg = sapmComponent->reg;
                mixerControl.mask = sapmComponent->mask;
                mixerControl.shift = sapmComponent->shift;
                AudioUpdateCodecRegBits(sapmComponent->codec, mixerControl.reg, mixerControl.mask,
                    mixerControl.shift, val);
                ADM_LOG_INFO("Sapm Codec %s Power Up.", sapmComponent->componentName);
            }
        }
    }

    return;
}

static void AudioSapmPowerDownSeqRun(const struct DListHead *list)
{
    uint32_t val;
    struct AudioMixerControl mixerControl;
    struct AudioSapmComponent *sapmComponent = NULL;
    ADM_LOG_DEBUG("entry!");

    if (list == NULL) {
        ADM_LOG_ERR("sapm input param list is NULL.");
        return;
    }
    DLIST_FOR_EACH_ENTRY(sapmComponent, list, struct AudioSapmComponent, powerList) {
        if (sapmComponent->power == SAPM_POWER_UP) {
            val = SAPM_POWER_DOWN;
            if (sapmComponent->invert) {
                val = !val;
            }
            sapmComponent->power = SAPM_POWER_DOWN;

            if (sapmComponent->reg != AUDIO_NO_SAPM_REG) {
                mixerControl.mask = sapmComponent->mask;
                mixerControl.reg = sapmComponent->reg;
                mixerControl.shift = sapmComponent->shift;

                AudioUpdateCodecRegBits(sapmComponent->codec, mixerControl.reg, mixerControl.mask,
                    mixerControl.shift, val);
                ADM_LOG_INFO("Sapm Codec %s Power Down.", sapmComponent->componentName);
            }
        }
    }

    return;
}

static void AudioSapmPowerComponents(struct AudioCard *audioCard)
{
    int32_t ret;
    struct AudioSapmComponent *sapmComponent = NULL;
    struct DListHead upList;
    struct DListHead downList;
    ADM_LOG_DEBUG("entry!");

    if (audioCard == NULL) {
        ADM_LOG_ERR("input param audioCard is NULL.");
        return;
    }

    DListHeadInit(&upList);
    DListHeadInit(&downList);

    DLIST_FOR_EACH_ENTRY(sapmComponent, &audioCard->sapmDirty, struct AudioSapmComponent, dirty) {
        sapmComponent->newPower = sapmComponent->PowerCheck(sapmComponent);
        if (sapmComponent->newPower == sapmComponent->power) {
            continue;
        }

        if (audioCard->sapmStandbyState && sapmComponent->PowerClockOp != NULL) {
            ret = sapmComponent->PowerClockOp(sapmComponent);
            if (ret != HDF_SUCCESS) {
                continue;
            }
        }

        AudioSapmSetPower(audioCard, sapmComponent, sapmComponent->newPower, &upList, &downList);
    }

    DLIST_FOR_EACH_ENTRY(sapmComponent, &audioCard->components, struct AudioSapmComponent, list) {
        DListRemove(&sapmComponent->dirty);
        DListHeadInit(&sapmComponent->dirty);
    }

    AudioSapmPowerDownSeqRun(&downList);
    AudioSapmPowerUpSeqRun(&upList);
}

static void ReadInitComponentPowerStatus(struct AudioSapmComponent *sapmComponent)
{
    int32_t ret;
    uint32_t regVal = 0;

    if (sapmComponent == NULL || sapmComponent->codec == NULL) {
        ADM_LOG_ERR("input param sapmComponent is NULL.");
        return;
    }

    if (sapmComponent->reg != AUDIO_NO_SAPM_REG) {
        ret = AudioCodecReadReg(sapmComponent->codec, sapmComponent->reg, &regVal);
        if (ret != HDF_SUCCESS) {
            ADM_LOG_ERR("read reg fail!");
            return;
        }
        regVal &= 1 << sapmComponent->shift;

        if (sapmComponent->invert) {
            regVal = !regVal;
        }

        if (regVal) {
            sapmComponent->power = SAPM_POWER_UP;
        } else {
            sapmComponent->power = SAPM_POWER_DOWN;
        }
    }

    return;
}

static int AudioSapmThread(void *data)
{
    struct AudioCard *audioCard = (struct AudioCard *)data;
    audioCard->time = 0;

    while (true) {
        OsalSleep(SAPM_POLL_TIME);
        AudioSapmTimerCallback(audioCard);
        audioCard->time++;
    }

    return 0;
}

int32_t AudioSapmSleep(struct AudioCard *audioCard)
{
    int32_t ret;
    char *sapmThreadName = NULL;
    struct OsalThreadParam threadCfg;
    OSAL_DECLARE_THREAD(audioSapmThread);

    if (audioCard == NULL) {
        ADM_LOG_ERR("input param audioCard is NULL.");
        return HDF_ERR_INVALID_OBJECT;
    }

    (void)AudioSapmRefreshTime(audioCard, true);
    sapmThreadName = OsalMemCalloc(SAPM_THREAD_NAME);
    if (sapmThreadName == NULL) {
        return HDF_ERR_MALLOC_FAIL;
    }
    if (snprintf_s(sapmThreadName, SAPM_THREAD_NAME, SAPM_THREAD_NAME - 1, "AudioSapmThread%u", g_cardNum) < 0) {
        ADM_LOG_ERR("snprintf_s failed.");
        OsalMemFree(sapmThreadName);
        return HDF_FAILURE;
    }

    ret = OsalThreadCreate(&audioSapmThread, (OsalThreadEntry)AudioSapmThread, (void *)audioCard);
    if (ret != HDF_SUCCESS) {
        ADM_LOG_ERR("create sapm thread fail, ret=%d", ret);
        OsalMemFree(sapmThreadName);
        return HDF_FAILURE;
    }

    (void)memset_s(&threadCfg, sizeof(threadCfg), 0, sizeof(threadCfg));
    threadCfg.name = sapmThreadName;
    threadCfg.priority = OSAL_THREAD_PRI_DEFAULT;
    threadCfg.stackSize = SAPM_STACK_SIZE;
    ret = OsalThreadStart(&audioSapmThread, &threadCfg);
    if (ret != HDF_SUCCESS) {
        ADM_LOG_ERR("start sapm thread fail, ret=%d", ret);
        OsalThreadDestroy(&audioSapmThread);
        OsalMemFree(sapmThreadName);
        return HDF_FAILURE;
    }

    g_cardNum++;
    audioCard->sapmStandbyState = false;
    audioCard->sapmSleepState = false;
    OsalMemFree(sapmThreadName);

    return HDF_SUCCESS;
}

int32_t AudioSapmNewControls(struct AudioCard *audioCard)
{
    struct AudioSapmComponent *sapmComponent = NULL;
    int32_t ret = HDF_SUCCESS;

    if (audioCard == NULL) {
        ADM_LOG_ERR("input param audioCard is NULL.");
        return HDF_ERR_INVALID_OBJECT;
    }

    DLIST_FOR_EACH_ENTRY(sapmComponent, &audioCard->components, struct AudioSapmComponent, list) {
        if (sapmComponent->newCpt) {
            continue;
        }
        if (sapmComponent->kcontrolsNum > 0) {
            sapmComponent->kcontrols = OsalMemCalloc(sizeof(struct AudioKcontrol*) * sapmComponent->kcontrolsNum);
            if (sapmComponent->kcontrols == NULL) {
                ADM_LOG_ERR("malloc kcontrols fail!");
                return HDF_FAILURE;
            }
        }

        switch (sapmComponent->sapmType) {
            case AUDIO_SAPM_ANALOG_SWITCH:
            case AUDIO_SAPM_MIXER:
            case AUDIO_SAPM_MIXER_NAMED_CTRL:
                ret = AudioSapmNewMixerControls(sapmComponent, audioCard);
                break;
            case AUDIO_SAPM_MUX:
            case AUDIO_SAPM_VIRT_MUX:
            case AUDIO_SAPM_VALUE_MUX:
                ret = AudioSapmNewMuxControls(sapmComponent, audioCard);
                break;
            default:
                ret = HDF_SUCCESS;
                break;
        }
        if (ret != HDF_SUCCESS) {
            OsalMemFree(sapmComponent->kcontrols);
            ADM_LOG_ERR("sapm new mixer or mux controls fail!");
            return HDF_FAILURE;
        }

        ReadInitComponentPowerStatus(sapmComponent);
        sapmComponent->newCpt = 1;
        DListInsertTail(&sapmComponent->dirty, &audioCard->sapmDirty);
    }

    AudioSapmPowerComponents(audioCard);

    return HDF_SUCCESS;
}

static int32_t MixerUpdatePowerStatus(const struct AudioKcontrol *kcontrol, uint32_t pathStatus)
{
    struct AudioCard *audioCard = NULL;
    struct AudioSapmpath *path = NULL;

    if (kcontrol == NULL || kcontrol->pri == NULL) {
        ADM_LOG_ERR("Mixer input param kcontrol is NULL.");
        return HDF_ERR_INVALID_OBJECT;
    }
    audioCard = (struct AudioCard *)((volatile uintptr_t)(kcontrol->pri));

    DLIST_FOR_EACH_ENTRY(path, &audioCard->paths, struct AudioSapmpath, list) {
        if (path->kcontrol != kcontrol) {
            continue;
        }
        if (path->sink == NULL || path->source == NULL) {
            ADM_LOG_ERR("get path sink or source fail!");
            return HDF_FAILURE;
        }
        if (path->sink->sapmType != AUDIO_SAPM_MIXER &&
            path->sink->sapmType != AUDIO_SAPM_MIXER_NAMED_CTRL &&
            path->sink->sapmType != AUDIO_SAPM_PGA &&
            path->sink->sapmType != AUDIO_SAPM_SPK &&
            path->sink->sapmType != AUDIO_SAPM_ANALOG_SWITCH) {
            ADM_LOG_DEBUG("no mixer device.");
            return HDF_DEV_ERR_NO_DEVICE;
        }

        path->connect = pathStatus;
        DListInsertTail(&path->source->dirty, &audioCard->sapmDirty);
        DListInsertTail(&path->sink->dirty, &audioCard->sapmDirty);
        break;
    }

    AudioSapmPowerComponents(audioCard);

    return HDF_SUCCESS;
}

static int32_t MuxUpdatePowerStatus(const struct AudioKcontrol *kcontrol, int32_t i, struct AudioEnumKcontrol *enumKtl)
{
    struct AudioCard *audioCard = NULL;
    struct AudioSapmpath *path = NULL;

    if (kcontrol == NULL || kcontrol->pri == NULL) {
        ADM_LOG_ERR("Mux input param kcontrol is NULL.");
        return HDF_ERR_INVALID_OBJECT;
    }
    audioCard = (struct AudioCard *)((volatile uintptr_t)(kcontrol->pri));

    DLIST_FOR_EACH_ENTRY(path, &audioCard->paths, struct AudioSapmpath, list) {
        if (path->kcontrol != kcontrol) {
            continue;
        }
        if (path->name == NULL || enumKtl->texts[i] == NULL) {
            continue;
        }

        if (path->sink->sapmType != AUDIO_SAPM_MUX &&
            path->sink->sapmType != AUDIO_SAPM_VIRT_MUX &&
            path->sink->sapmType != AUDIO_SAPM_VALUE_MUX) {
            ADM_LOG_ERR("no mux device.");
            return HDF_DEV_ERR_NO_DEVICE;
        }

        if (strcmp(path->name, enumKtl->texts[i]) == 0) {
            path->connect = 1;
        } else {
            if (path->connect == 1) {
                path->connect = 0;
            }
        }

        DListInsertTail(&path->source->dirty, &audioCard->sapmDirty);
        DListInsertTail(&path->sink->dirty, &audioCard->sapmDirty);
        break;
    }

    AudioSapmPowerComponents(audioCard);
    return HDF_SUCCESS;
}

int32_t AudioCodecSapmGetCtrlOps(const struct AudioKcontrol *kcontrol, struct AudioCtrlElemValue *elemValue)
{
    if (AudioCodecGetCtrlOps(kcontrol, elemValue) != HDF_SUCCESS) {
        ADM_LOG_ERR("Audio codec sapm get control switch is fail!");
        return HDF_FAILURE;
    }

    return HDF_SUCCESS;
}

/* 1.first user specify old component -- power down; 2.second user specify new component -- power up */
static int32_t AudioSapmSetCtrlOps(const struct AudioKcontrol *kcontrol, const struct AudioCtrlElemValue *elemValue,
    uint32_t *value, uint32_t *pathStatus)
{
    struct AudioMixerControl *mixerCtrl = NULL;
    int32_t iFlag = (kcontrol == NULL) || (kcontrol->privateValue <= 0) || (elemValue == NULL)
        || (value == NULL) || (pathStatus == NULL);
    if (iFlag) {
        ADM_LOG_ERR("input params invalid.");
        return HDF_ERR_INVALID_OBJECT;
    }

    mixerCtrl = (struct AudioMixerControl *)((volatile uintptr_t)kcontrol->privateValue);
    *value = elemValue->value[0];
    if (*value < mixerCtrl->min || *value > mixerCtrl->max) {
        ADM_LOG_ERR("value is invalid.");
        return HDF_ERR_INVALID_OBJECT;
    }

    if (*value) {
        *pathStatus = CONNECT_SINK_AND_SOURCE;
    } else {
        *pathStatus = UNCONNECT_SINK_AND_SOURCE;
    }

    if (mixerCtrl->invert) {
        *value = mixerCtrl->max - *value;
    }

    return HDF_SUCCESS;
}

int32_t AudioCodecSapmSetCtrlOps(const struct AudioKcontrol *kcontrol, const struct AudioCtrlElemValue *elemValue)
{
    uint32_t value;
    uint32_t pathStatus = 0;
    struct CodecDevice *codec = NULL;
    struct AudioMixerControl *mixerCtrl = NULL;
    if ((kcontrol == NULL) || (kcontrol->privateValue <= 0) || (kcontrol->pri == NULL)) {
        ADM_LOG_ERR("input params: kcontrol is NULL.");
        return HDF_ERR_INVALID_OBJECT;
    }
    if (elemValue == NULL) {
        ADM_LOG_ERR("input params: elemValue is NULL.");
        return HDF_ERR_INVALID_OBJECT;
    }

    mixerCtrl = (struct AudioMixerControl *)((volatile uintptr_t)kcontrol->privateValue);
    if (AudioSapmSetCtrlOps(kcontrol, elemValue, &value, &pathStatus) != HDF_SUCCESS) {
        ADM_LOG_ERR("Audio sapm put control switch fail!");
    }
    codec = AudioKcontrolGetCodec(kcontrol);

    if (MixerUpdatePowerStatus(kcontrol, pathStatus) != HDF_SUCCESS) {
        ADM_LOG_ERR("update power status is failure!");
        return HDF_FAILURE;
    }

    mixerCtrl->value = elemValue->value[0];
    if (AudioCodecRegUpdate(codec, mixerCtrl) != HDF_SUCCESS) {
        ADM_LOG_ERR("update reg bits fail!");
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

static int32_t AudioCodecCheckRegIsChange(struct AudioEnumKcontrol *enumCtrl,
    const struct AudioCtrlElemValue *elemValue, uint32_t curValue, bool *change)
{
    uint32_t value;
    uint32_t mask;
    uint32_t oldValue;

    if (enumCtrl == NULL || elemValue == NULL || change == NULL) {
        ADM_LOG_ERR("input para is null!");
        return HDF_FAILURE;
    }

    *change = false;
    if (elemValue->value[0] > enumCtrl->max) {
        ADM_LOG_ERR("elemValue value[0] out of range!");
        return HDF_FAILURE;
    }

    if (enumCtrl->values != NULL) {
        value = enumCtrl->values[elemValue->value[0]] << enumCtrl->shiftLeft;
        mask = enumCtrl->mask << enumCtrl->shiftLeft;
        if (enumCtrl->shiftLeft != enumCtrl->shiftRight) {
            if (elemValue->value[1] > enumCtrl->max) {
                ADM_LOG_ERR("elemValue value[1] out of range!");
                return HDF_FAILURE;
            }
            value |= enumCtrl->values[elemValue->value[1]] << enumCtrl->shiftRight;
            mask |= enumCtrl->mask << enumCtrl->shiftRight;
        }
    } else {
        value = elemValue->value[0] << enumCtrl->shiftLeft;
        mask = enumCtrl->mask << enumCtrl->shiftLeft;
        if (enumCtrl->shiftLeft != enumCtrl->shiftRight) {
            if (elemValue->value[1] > enumCtrl->max) {
                ADM_LOG_ERR("elemValue value[1] out of range!");
                return HDF_FAILURE;
            }
            value |= elemValue->value[1] << enumCtrl->shiftRight;
            mask |= enumCtrl->mask << enumCtrl->shiftRight;
        }
    }

    oldValue = curValue & mask;
    if (oldValue != value) {
        *change = true;
    }
    return HDF_SUCCESS;
}

int32_t AudioCodecSapmSetEnumCtrlOps(const struct AudioKcontrol *kcontrol,
    const struct AudioCtrlElemValue *elemValue)
{
    uint32_t curValue;
    bool change;
    int32_t ret;
    struct CodecDevice *codec = NULL;
    struct AudioEnumKcontrol *enumCtrl = NULL;

    if ((kcontrol == NULL) || (kcontrol->privateValue <= 0) || (elemValue == NULL)) {
        ADM_LOG_ERR("input params: kcontrol is NULL or elemValue is NULL");
        return HDF_ERR_INVALID_OBJECT;
    }
    codec = AudioKcontrolGetCodec(kcontrol);

    enumCtrl = (struct AudioEnumKcontrol *)((volatile uintptr_t)kcontrol->privateValue);
    if (enumCtrl == NULL) {
        ADM_LOG_ERR("privateValue is null");
        return HDF_FAILURE;
    }

    if (AudioCodecReadReg(codec, enumCtrl->reg, &curValue) != HDF_SUCCESS) {
        ADM_LOG_ERR("Device read register is failure!");
        return HDF_FAILURE;
    }

    ret = AudioCodecCheckRegIsChange(enumCtrl, elemValue, curValue, &change);
    if (ret != HDF_SUCCESS) {
        ADM_LOG_ERR("AudioCodecCheckRegIsChange is failure!");
        return HDF_FAILURE;
    }

    if (change) {
        if (MuxUpdatePowerStatus(kcontrol, elemValue->value[0], enumCtrl) != HDF_SUCCESS) {
            ADM_LOG_ERR("update power status is failure!");
            return HDF_FAILURE;
        }

        ret = AudioCodecMuxRegUpdate(codec, enumCtrl, elemValue->value);
        if (ret != HDF_SUCCESS) {
            ADM_LOG_ERR("AudioCodecMuxRegUpdate is failure!");
            return HDF_FAILURE;
        }
    }

    return HDF_SUCCESS;
}

int32_t AudioCodecSapmGetEnumCtrlOps(const struct AudioKcontrol *kcontrol, struct AudioCtrlElemValue *elemValue)
{
    if (AudioCodecGetEnumCtrlOps(kcontrol, elemValue) != HDF_SUCCESS) {
        ADM_LOG_ERR("Audio codec sapm get control switch is fail!");
        return HDF_FAILURE;
    }

    return HDF_SUCCESS;
}

static int32_t AudioSapmRefreshTime(struct AudioCard *audioCard, bool bRefresh)
{
    if (audioCard == NULL) {
        ADM_LOG_ERR("input params is NULL.");
        return HDF_ERR_INVALID_OBJECT;
    }

    if (bRefresh) {
        audioCard->time = 0;
    }
    return HDF_SUCCESS;
}

static int32_t AudioSapmCheckTime(struct AudioCard *audioCard, bool *timeoutStatus)
{
    int32_t ret;

    if (audioCard == NULL || timeoutStatus == NULL) {
        ADM_LOG_ERR("input params is NULL.");
        return HDF_ERR_INVALID_OBJECT;
    }

    ret = AudioSapmRefreshTime(audioCard, false);
    if (ret != HDF_SUCCESS) {
        ADM_LOG_ERR("AudioSapmRefreshTime failed.");
        return HDF_FAILURE;
    }
    *timeoutStatus = audioCard->time > SAPM_SLEEP_TIMES ? true : false;

    return HDF_SUCCESS;
}

int32_t AudioSampPowerUp(const struct AudioCard *card)
{
    struct DListHead upList;
    struct AudioSapmComponent *sapmComponent = NULL;

    if (card == NULL) {
        ADM_LOG_ERR("input params is null.");
        return HDF_ERR_INVALID_OBJECT;
    }

    DListHeadInit(&upList);
    DLIST_FOR_EACH_ENTRY(sapmComponent, &card->components, struct AudioSapmComponent, list) {
        if (sapmComponent == NULL) {
            break;
        }
        if (sapmComponent->power == SAPM_POWER_DOWN) {
            AudioSapmPowerSeqInsert(sapmComponent, &upList, SAPM_POWER_UP);
        }
    }

    AudioSapmPowerUpSeqRun(&upList);
    return HDF_SUCCESS;
}

int32_t AudioSampSetPowerMonitor(struct AudioCard *card, bool powerMonitorState)
{
    if (card == NULL) {
        ADM_LOG_ERR("input params is null.");
        return HDF_ERR_INVALID_OBJECT;
    }

    card->sapmMonitorState = powerMonitorState;
    if (powerMonitorState == false) {
        card->sapmSleepState = false;
        card->sapmStandbyState = false;
        card->sapmStandbyStartTimeFlag = false;
        card->sapmSleepStartTimeFlag = false;
    }
    return HDF_SUCCESS;
}

static void AudioSapmEnterSleep(struct AudioCard *audioCard)
{
    struct DListHead downList;
    struct AudioSapmComponent *sapmComponent = NULL;
    ADM_LOG_INFO("entry!");

    DListHeadInit(&downList);
    if (audioCard == NULL) {
        ADM_LOG_ERR("audioCard is null.");
        return;
    }
    DLIST_FOR_EACH_ENTRY(sapmComponent, &audioCard->components, struct AudioSapmComponent, list) {
        if (sapmComponent == NULL) {
            break;
        }
        if (sapmComponent->power == SAPM_POWER_UP) {
            AudioSapmPowerSeqInsert(sapmComponent, &downList, SAPM_POWER_DOWN);
        }
    }
    AudioSapmPowerDownSeqRun(&downList);
    audioCard->sapmStandbyState = false;
    audioCard->sapmSleepState = true;
}

static bool AudioSapmEnterStandby(struct AudioCard *audioCard)
{
    bool timeoutStatus;
    struct AudioSapmComponent *sapmComponent = NULL;

    if (audioCard == NULL) {
        ADM_LOG_ERR("audioCard is null.");
        return false;
    }

    if (audioCard->sapmStandbyStartTimeFlag == false) {
        if (AudioSapmRefreshTime(audioCard, true) != HDF_SUCCESS) {
            ADM_LOG_ERR("AudioSapmRefreshTime failed.");
            return false;
        }
        audioCard->sapmStandbyStartTimeFlag = true;
    }

    if (audioCard->standbyMode != AUDIO_SAPM_TURN_STANDBY_NOW) {
        if (AudioSapmCheckTime(audioCard, &timeoutStatus) != HDF_SUCCESS) {
            ADM_LOG_ERR("AudioSapmCheckTime failed.");
            return false;
        }

        if (!timeoutStatus) {
            return false;
        }
    }

    if (audioCard->sapmStandbyState == false) {
        DLIST_FOR_EACH_ENTRY(sapmComponent, &audioCard->components, struct AudioSapmComponent, list) {
            if (sapmComponent->PowerClockOp != NULL) {
                    sapmComponent->PowerClockOp(sapmComponent);
                }
        }
        audioCard->sapmStandbyState = true;
    }

    return true;
}

static void AudioSapmTimerCallback(struct AudioCard *audioCard)
{
    bool timeoutStatus;
    bool standbyEntry;

#ifndef __LITEOS__
    ADM_LOG_DEBUG("entry!");
#endif

    if (audioCard == NULL) {
        return;
    }
    if (audioCard->sapmSleepState == true) {
        return;
    }

    if (audioCard->sapmMonitorState == false) {
        return;
    }

    standbyEntry = AudioSapmEnterStandby(audioCard);
    if (!standbyEntry) {
        return;
    }

    if (audioCard->sapmSleepStartTimeFlag == false) {
        if (AudioSapmRefreshTime(audioCard, true) != HDF_SUCCESS) {
            ADM_LOG_ERR("AudioSapmRefreshTime failed.");
            return;
        }
        audioCard->sapmSleepStartTimeFlag = true;
    }

    if (AudioSapmCheckTime(audioCard, &timeoutStatus) != HDF_SUCCESS) {
        ADM_LOG_ERR("AudioSapmCheckTime failed.");
        return;
    }

    if (!timeoutStatus) {
        return;
    }
    AudioSapmEnterSleep(audioCard);
}
