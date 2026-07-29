/*
 * Copyright (c) 2021-2023 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include "audio_codec_base.h"
#include "i2c_if.h"
#include "audio_driver_log.h"
#include "audio_parse.h"
#include "audio_sapm.h"
#include "osal_time.h"

#define HDF_LOG_TAG HDF_AUDIO_KADM

#define COMM_SHIFT_8BIT     8
#define COMM_MASK_FF        0xFF
#define COMM_WAIT_TIMES     10  // ms

#define I2C_REG_LEN         1
#define I2C_REG_MSGLEN      3
#define I2C_MSG_NUM         2
#define I2C_MSG_BUF_SIZE_1  1
#define I2C_MSG_BUF_SIZE_2  2

static char *g_audioSapmCompNameList[AUDIO_SAPM_COMP_NAME_LIST_MAX] = {
    "ADCL",         "ADCR",         "DACL",         "DACR",         // [0], [1] [2], [3]
    "LPGA",         "RPGA",         "SPKL",         "SPKR",         // [4], [5] [6], [7]
    "MIC",          "LOUT",         "HPL",          "HPR",          // [8], [9] [10], [11]
    "Stereo Mixer", "Line Mix",     "Input Mixer",  "Speaker Mix",  // [12], [13] [14], [15]
    "Input Mux",    "AuxOut Mux",   "SPKL Mux",     "SPKR Mux",     // [16], [17] [18], [19]
    "AUXOUTL",      "AUXOUTR",      "LINEINL",      "LINEINR",      // [20], [21] [22], [23]
    "AUXINL",       "AUXINR",       "I2S Mix",      "AuxI Mix",     // [24], [25] [26], [27]
    "CaptureL Mix", "CaptureR Mix", "Mono1 Mixer",  "Mono2 Mixer",  // [28], [29] [30], [31]
    "DAC1",         "DAC2",         "DAC3",         "DAC4",         // [32], [33] [34], [35]
    "ADC1",         "ADC2",         "ADC3",         "ADC4",         // [36], [37] [38], [39]
    "MIC1",         "MIC2",         "MIC3",         "MIC4",         // [40], [41],[42], [43],
    "SPK1",         "SPK2",         "SPK3",         "SPK4",         // [44], [45],[46], [47],
    "DAC Mix",      "DAC Mux",      "ADC Mix",      "ADC Mux",      // [48], [49],[50], [51],
    "SPKL PGA",     "SPKR PGA",     "HPL PGA",      "HPR PGA",      // [52], [53],[54], [55],
};

static char *g_audioSapmCfgNameList[AUDIO_SAPM_CFG_NAME_LIST_MAX] = {
    "LPGA MIC Switch", "RPGA MIC Switch",                           // [0], [1]
    "Dacl enable", "Dacr enable",                                   // [2], [3]
    "Headphone Playback Switch",    "PCM Playback Switch",          // [4], [5]
    "PCM Capture Switch",           "Mono Playback Switch",         // [6], [7]
    "Phone Capture Switch",         "Mic Switch",                   // [8], [9]
    "Stereo Mic Switch",            "Line HP Swap Switch",          // [10], [11]
    "Surround Playback Switch",     "Center/LFE Playback Switch",   // [12], [13]
    "Capture Source",               "Mic Boost Switch",             // [14], [15]
    "DAC1 Switch",                  "DAC2 Switch",                  // [16], [17]
    "DAC3 Switch",                  "DAC4 Switch",                  // [18], [19]
    "ADC1 Switch",                  "ADC2 Switch",                  // [20], [21]
    "ADC3 Switch",                  "ADC4 Switch",                  // [22], [23]
    "Speaker1 Switch",              "Speaker2 Switch",              // [24], [25]
    "Speaker3 Switch",              "Speaker4 Switch",              // [26], [27]
    "Headphone1 Switch",            "Headphone2 Switch",            // [28], [29]
    "Lineout1 Switch",              "Lineout2 Switch",              // [30], [31]
    "Lineout3 Switch",              "Lineout4 Switch",              // [32], [33]
    "Mixer1 Switch",                "Mixer2 Switch",                // [34], [35]
    "Mixer3 Switch",                "Mixer4 Switch",                // [36], [37]
};

static char *g_audioCodecControlsList[AUDIO_CTRL_LIST_MAX] = {
    "Main Playback Volume",         "Main Capture Volume",          // [0], [1]
    "Playback Mute",                "Capture Mute",                 // [2], [3]
    "Mic Left Gain",                "Mic Right Gain",               // [4], [5]
    "External Codec Enable",        "Internally Codec Enable",      // [6], [7]
    "Render Channel Mode",          "Captrue Channel Mode",         // [8], [9]
    "Headphone Playback Volume",    "PCM Playback Volume",          // [10], [11]
    "PCM Capture Volume",           "Mono Playback Volume",         // [12], [13]
    "Phone Capture Volume",         "Mic Volume",                   // [14], [15]
    "Surround Playback Volume",     "Center/LFE Playback Volume",   // [16], [17]
    "DAC1 Volume",                  "DAC2 Volume",                  // [18], [19]
    "DAC3 Volume",                  "DAC4 Volume",                  // [20], [21]
    "ADC1 Volume",                  "ADC2 Volume",                  // [22], [23]
    "ADC3 Volume",                  "ADC4 Volume",                  // [24], [25]
    "Speaker1 Volume",              "Speaker2 Volume",              // [26], [27]
    "Speaker3 Volume",              "Speaker4 Volume",              // [28], [29]
    "MIC1 Volume",                  "MIC2 Volume",                  // [30], [31]
    "MIC3 Volume",                  "MIC4 Volume",                  // [32], [33]
    "MIC1 Boost Volume",            "MIC2 Boost Volume",            // [34], [35]
    "INA1 Volume",                  "INB1 Volume",                  // [36], [37]
    "INA2 Volume",                  "INB2 Volume",                  // [38], [39]
    "Lineout1 Volume",              "Lineout2 Volume",              // [40], [41]
    "Lineout3 Volume",              "Lineout4 Volume",              // [42], [43]
    "Headphone Volume",             "Receiver Volume",              // [44], [45]
    "EQ1 Switch",                   "EQ2 Switch",                   // [46], [47]
    "DAI1 Filter Mode",             "DAI2 Filter Mode",             // [48], [49]
    "ADC High Pass Filter Switch",  "Playback Deemphasis",          // [50], [51]
    "PGA1 Setting",                 "PGA2 Setting",                 // [52], [53]
    "PGA3 Setting",                 "PGA3 Setting",                 // [54], [55]
    "ADC1 Mute",                    "ADC2 Mute",                    // [56], [57]
    "ADC3 Mute",                    "ADC4 Mute",                    // [58], [59]
};

int32_t CodecGetServiceName(const struct HdfDeviceObject *device, const char **drvCodecName)
{
    const struct DeviceResourceNode *node = NULL;
    struct DeviceResourceIface *drsOps = NULL;
    int32_t ret;

    if (device == NULL) {
        AUDIO_DRIVER_LOG_ERR("input device para is nullptr.");
        return HDF_FAILURE;
    }

    node = device->property;
    if (node == NULL) {
        AUDIO_DRIVER_LOG_ERR("node instance is nullptr.");
        return HDF_FAILURE;
    }
    drsOps = DeviceResourceGetIfaceInstance(HDF_CONFIG_SOURCE);
    if (drsOps == NULL || drsOps->GetString == NULL) {
        AUDIO_DRIVER_LOG_ERR("from resource get drsOps fail!");
        return HDF_FAILURE;
    }

    ret = drsOps->GetString(node, "serviceName", drvCodecName, 0);
    if (ret != HDF_SUCCESS) {
        AUDIO_DRIVER_LOG_ERR("read codecServiceName fail!");
        return ret;
    }

    return HDF_SUCCESS;
}

int32_t CodecGetDaiName(const struct HdfDeviceObject *device, const char **drvDaiName)
{
    const struct DeviceResourceNode *node = NULL;
    struct DeviceResourceIface *drsOps = NULL;
    int32_t ret;

    if (device == NULL) {
        AUDIO_DRIVER_LOG_ERR("input para is NULL.");
        return HDF_FAILURE;
    }

    node = device->property;
    if (node == NULL) {
        AUDIO_DRIVER_LOG_ERR("drs node is NULL.");
        return HDF_FAILURE;
    }
    drsOps = DeviceResourceGetIfaceInstance(HDF_CONFIG_SOURCE);
    if (drsOps == NULL || drsOps->GetString == NULL) {
        AUDIO_DRIVER_LOG_ERR("drs ops failed!");
        return HDF_FAILURE;
    }

    ret = drsOps->GetString(node, "codecDaiName", drvDaiName, 0);
    if (ret != HDF_SUCCESS) {
        AUDIO_DRIVER_LOG_ERR("read codecDaiName fail!");
        return ret;
    }

    return HDF_SUCCESS;
}

int32_t CodecGetConfigInfo(const struct HdfDeviceObject *device, struct CodecData *codecData)
{
    if (device == NULL || codecData == NULL) {
        AUDIO_DRIVER_LOG_ERR("param is null!");
        return HDF_FAILURE;
    }

    if (codecData->regConfig != NULL) {
        ADM_LOG_INFO("g_codecData regConfig has been parsed!");
        return HDF_SUCCESS;
    }

    codecData->regConfig = (struct AudioRegCfgData *)OsalMemCalloc(sizeof(*(codecData->regConfig)));
    if (codecData->regConfig == NULL) {
        ADM_LOG_ERR("malloc AudioRegCfgData fail!");
        return HDF_FAILURE;
    }

    if (AudioGetRegConfig(device, codecData->regConfig) != HDF_SUCCESS) {
        ADM_LOG_ERR("AudioGetRegConfig fail!");
        OsalMemFree(codecData->regConfig);
        codecData->regConfig = NULL;
        return HDF_FAILURE;
    }

    return HDF_SUCCESS;
}

int32_t CodecDaiGetPortConfigInfo(const struct HdfDeviceObject *device, struct DaiData *codecData)
{
    if (device == NULL || codecData == NULL) {
        ADM_LOG_ERR("<device> or <codecData> is a null pointer!");
        return HDF_ERR_INVALID_PARAM;
    }

    return AudioGetPortConfig(device, &codecData->portInfo);
}

static int32_t SapmCtrlToSapmComp(struct AudioSapmComponent *sapmComponents,
    const struct AudioSapmCtrlConfig *sapmCompItem, uint16_t index)
{
    if (sapmComponents == NULL || sapmCompItem == NULL) {
        AUDIO_DRIVER_LOG_ERR("input para is NULL.");
        return HDF_FAILURE;
    }

    sapmComponents[index].componentName =
        g_audioSapmCompNameList[sapmCompItem[index].compNameIndex];
    sapmComponents[index].reg      = sapmCompItem[index].reg;
    sapmComponents[index].sapmType = sapmCompItem[index].sapmType;
    sapmComponents[index].mask     = sapmCompItem[index].mask;
    sapmComponents[index].shift    = sapmCompItem[index].shift;
    sapmComponents[index].invert   = sapmCompItem[index].invert;
    sapmComponents[index].kcontrolsNum = sapmCompItem[index].kcontrolsNum;

    return HDF_SUCCESS;
}

static int32_t CodecSetSapmKcontrolInfo(struct AudioKcontrol *audioSapmControls,
    struct AudioRegCfgGroupNode **regCfgGroup)
{
    uint16_t index;
    struct AudioControlConfig  *sapmCtrlItem = NULL;
    struct AudioMixerControl   *ctlSapmRegCfgItem = NULL;
    struct AudioEnumCtrlConfig *ctlRegEnumCfgItem = NULL;

    if (audioSapmControls == NULL || regCfgGroup == NULL) {
        AUDIO_DRIVER_LOG_ERR("input para is NULL.");
        return HDF_FAILURE;
    }
    if (regCfgGroup[AUDIO_CTRL_SAPM_PATAM_GROUP] == NULL || regCfgGroup[AUDIO_SAPM_CFG_GROUP] == NULL) {
        AUDIO_DRIVER_LOG_ERR("codec config hcs configuration file is no configuration information for sapm");
        return HDF_SUCCESS;
    }

    sapmCtrlItem = regCfgGroup[AUDIO_SAPM_CFG_GROUP]->ctrlCfgItem;
    ctlSapmRegCfgItem = regCfgGroup[AUDIO_CTRL_SAPM_PATAM_GROUP]->regCfgItem;
    if (sapmCtrlItem == NULL || ctlSapmRegCfgItem == NULL) {
        AUDIO_DRIVER_LOG_ERR("sapmCtrlItem, ctlSapmRegCfgItem is NULL.");
        return HDF_FAILURE;
    }

    if (regCfgGroup[AUDIO_CTRL_SAPM_PATAM_MUX_GROUP] != NULL) {
        ctlRegEnumCfgItem = regCfgGroup[AUDIO_CTRL_SAPM_PATAM_MUX_GROUP]->regEnumCfgItem;
        if (ctlRegEnumCfgItem == NULL) {
            AUDIO_DRIVER_LOG_ERR("ctlRegEnumCfgItem is NULL.");
            return HDF_FAILURE;
        }
    }

    for (index = 0; index < regCfgGroup[AUDIO_SAPM_CFG_GROUP]->itemNum; index++) {
        if (sapmCtrlItem[index].type == AUDIO_CONTROL_MIXER) {
            audioSapmControls[index].iface = sapmCtrlItem[index].iface;
            audioSapmControls[index].name = g_audioSapmCfgNameList[sapmCtrlItem[index].arrayIndex];
            audioSapmControls[index].privateValue = (unsigned long)(uintptr_t)(void*)(&ctlSapmRegCfgItem[index]);
            audioSapmControls[index].Info = AudioInfoCtrlOps;
            audioSapmControls[index].Get  = AudioCodecSapmGetCtrlOps;
            audioSapmControls[index].Set  = AudioCodecSapmSetCtrlOps;
        } else if (sapmCtrlItem[index].type == AUDIO_CONTROL_MUX) {
            audioSapmControls[index].iface = sapmCtrlItem[index].iface;
            audioSapmControls[index].name = g_audioSapmCfgNameList[sapmCtrlItem[index].arrayIndex];
            audioSapmControls[index].privateValue = (unsigned long)(uintptr_t)(void*)(&ctlRegEnumCfgItem[index]);
            audioSapmControls[index].Info = AudioInfoEnumCtrlOps;
            audioSapmControls[index].Get  = AudioCodecSapmGetEnumCtrlOps;
            audioSapmControls[index].Set  = AudioCodecSapmSetEnumCtrlOps;
        }
    }

    return HDF_SUCCESS;
}

static int32_t CodecSetSapmConfigInfo(struct CodecData *codeData, struct AudioRegCfgGroupNode **regCfgGroup)
{
    uint16_t index;
    struct AudioSapmCtrlConfig *sapmCompItem = NULL;
    struct AudioKcontrol *audioSapmControls = NULL;
    if (codeData == NULL || regCfgGroup == NULL) {
        return HDF_FAILURE;
    }
    if (regCfgGroup[AUDIO_SAPM_COMP_GROUP] == NULL || regCfgGroup[AUDIO_SAPM_CFG_GROUP] == NULL) {
        AUDIO_DRIVER_LOG_ERR("codec config hcs configuration file is no configuration information for sapm");
        return HDF_SUCCESS;
    }
    sapmCompItem = regCfgGroup[AUDIO_SAPM_COMP_GROUP]->sapmCompItem;
    if (sapmCompItem == NULL) {
        AUDIO_DRIVER_LOG_ERR("sapmCompItem is NULL.");
        return HDF_FAILURE;
    }
    audioSapmControls = (struct AudioKcontrol *)OsalMemCalloc(
        regCfgGroup[AUDIO_SAPM_CFG_GROUP]->itemNum * sizeof(struct AudioKcontrol));
    if (audioSapmControls == NULL) {
        AUDIO_DRIVER_LOG_ERR("OsalMemCalloc failed.");
        return HDF_FAILURE;
    }
    if (CodecSetSapmKcontrolInfo(audioSapmControls, regCfgGroup) != HDF_SUCCESS) {
        OsalMemFree(audioSapmControls);
        return HDF_FAILURE;
    }
    codeData->numSapmComponent = regCfgGroup[AUDIO_SAPM_COMP_GROUP]->itemNum;
    codeData->sapmComponents = (struct AudioSapmComponent *)
        OsalMemCalloc(codeData->numSapmComponent * sizeof(struct AudioSapmComponent));
    if (codeData->sapmComponents == NULL) {
        OsalMemFree(audioSapmControls);
        AUDIO_DRIVER_LOG_ERR("OsalMemCalloc failed.");
        return HDF_FAILURE;
    }
    for (index = 0; index < codeData->numSapmComponent; index++) {
        if (SapmCtrlToSapmComp(codeData->sapmComponents, sapmCompItem, index)) {
            OsalMemFree(audioSapmControls);
            OsalMemFree(codeData->sapmComponents);
            codeData->sapmComponents = NULL;
            return HDF_FAILURE;
        }

        if (sapmCompItem[index].kcontrolsNum) {
            codeData->sapmComponents[index].kcontrolNews =
                &audioSapmControls[sapmCompItem[index].kcontrolNews - 1];
        }
    }
    return HDF_SUCCESS;
}

static int32_t CodecSetKcontrolInfo(struct CodecData *codeData, struct AudioRegCfgGroupNode **regCfgGroup)
{
    uint16_t index = 0;
    uint16_t enumIndex = 0;
    struct AudioControlConfig  *compItem = NULL;
    struct AudioMixerControl   *ctlRegCfgItem = NULL;
    struct AudioEnumCtrlConfig *enumCtlRegCfgItem = NULL;

    if (codeData == NULL || regCfgGroup == NULL || regCfgGroup[AUDIO_CTRL_CFG_GROUP] == NULL ||
        regCfgGroup[AUDIO_CTRL_PATAM_GROUP] == NULL) {
        AUDIO_DRIVER_LOG_ERR("input para is NULL.");
        return HDF_FAILURE;
    }

    compItem = regCfgGroup[AUDIO_CTRL_CFG_GROUP]->ctrlCfgItem;
    ctlRegCfgItem = regCfgGroup[AUDIO_CTRL_PATAM_GROUP]->regCfgItem;
    if (compItem == NULL || ctlRegCfgItem == NULL) {
        AUDIO_DRIVER_LOG_ERR("compItem or ctlRegCfgItem is NULL.");
        return HDF_FAILURE;
    }

    if (regCfgGroup[AUDIO_CTRL_PATAM_MUX_GROUP] != NULL) {
        enumCtlRegCfgItem = regCfgGroup[AUDIO_CTRL_PATAM_MUX_GROUP]->regEnumCfgItem;
        if (enumCtlRegCfgItem == NULL) {
            AUDIO_DRIVER_LOG_ERR("enumCtlRegCfgItem is NULL.");
            return HDF_FAILURE;
        }
    }

    for (index = 0; index < codeData->numControls; index++) {
        if (compItem[index].type == AUDIO_CONTROL_MIXER) {
            codeData->controls[index].iface   = compItem[index].iface;
            codeData->controls[index].name    = g_audioCodecControlsList[compItem[index].arrayIndex];
            codeData->controls[index].Info    = AudioInfoCtrlOps;
            codeData->controls[index].privateValue = (unsigned long)(uintptr_t)(void*)(&ctlRegCfgItem[index]);
            if (compItem[index].enable) {
                codeData->controls[index].Get = AudioCodecGetCtrlOps;
                codeData->controls[index].Set = AudioCodecSetCtrlOps;
            }
        } else if (compItem[index].type == AUDIO_CONTROL_MUX) {
            codeData->controls[index].iface   = compItem[index].iface;
            codeData->controls[index].name    = g_audioCodecControlsList[compItem[index].arrayIndex];
            codeData->controls[index].Info    = AudioInfoEnumCtrlOps;
            codeData->controls[index].privateValue = (unsigned long)(uintptr_t)(void*)(&enumCtlRegCfgItem[enumIndex++]);
            if (compItem[index].enable) {
                codeData->controls[index].Get = AudioCodecGetEnumCtrlOps;
                codeData->controls[index].Set = AudioCodecSetEnumCtrlOps;
            }
        }
    }
    return HDF_SUCCESS;
}

int32_t CodecSetConfigInfoOfControls(struct CodecData *codeData, struct DaiData *daiData)
{
    struct AudioIdInfo   *audioIdInfo = NULL;
    struct AudioRegCfgGroupNode **regCfgGroup = NULL;

    if (codeData == NULL || daiData == NULL || codeData->regConfig == NULL) {
        AUDIO_DRIVER_LOG_ERR("input para is NULL.");
        return HDF_FAILURE;
    }

    audioIdInfo = &(codeData->regConfig->audioIdInfo);
    regCfgGroup = codeData->regConfig->audioRegParams;
    if (audioIdInfo == NULL || regCfgGroup == NULL) {
        AUDIO_DRIVER_LOG_ERR("audioIdInfo or regCfgGroup is NULL.");
        return HDF_FAILURE;
    }
    daiData->regCfgGroup = regCfgGroup;
    codeData->regCfgGroup = regCfgGroup;

    if (regCfgGroup[AUDIO_CTRL_CFG_GROUP] == NULL) {
        AUDIO_DRIVER_LOG_ERR("compItem is NULL.");
        return HDF_FAILURE;
    }

    codeData->numControls = regCfgGroup[AUDIO_CTRL_CFG_GROUP]->itemNum;
    codeData->controls =
        (struct AudioKcontrol *)OsalMemCalloc(codeData->numControls * sizeof(struct AudioKcontrol));
    if (codeData->controls == NULL) {
        AUDIO_DRIVER_LOG_ERR("OsalMemCalloc failed.");
        return HDF_FAILURE;
    }

    if (CodecSetKcontrolInfo(codeData, regCfgGroup) != HDF_SUCCESS) {
        OsalMemFree(codeData->controls);
        codeData->controls = NULL;
        return HDF_FAILURE;
    }
    codeData->virtualAddress = (uintptr_t)OsalIoRemap(audioIdInfo->chipIdRegister, audioIdInfo->chipIdSize);

    if (CodecSetSapmConfigInfo(codeData, regCfgGroup) != HDF_SUCCESS) {
        OsalMemFree(codeData->controls);
        codeData->controls = NULL;
        return HDF_FAILURE;
    }

    return HDF_SUCCESS;
}

int32_t CodecSetCtlFunc(struct CodecData *codeData, enum AudioControlType controlType, const void *getCtrl,
    const void *setCtrl)
{
    uint32_t index;
    struct AudioRegCfgGroupNode **regCfgGroup = NULL;
    struct AudioControlConfig *compItem = NULL;
    if (codeData == NULL || codeData->regConfig == NULL ||
        getCtrl == NULL || setCtrl == NULL) {
        AUDIO_DRIVER_LOG_ERR("input para is NULL.");
        return HDF_FAILURE;
    }
    regCfgGroup = codeData->regConfig->audioRegParams;
    if (regCfgGroup == NULL || regCfgGroup[AUDIO_CTRL_CFG_GROUP] == NULL) {
        AUDIO_DRIVER_LOG_ERR("regCfgGroup or regCfgGroup[AUDIO_CTRL_CFG_GROUP] is NULL.");
        return HDF_FAILURE;
    }

    compItem = regCfgGroup[AUDIO_CTRL_CFG_GROUP]->ctrlCfgItem;
    if (compItem == NULL) {
        AUDIO_DRIVER_LOG_ERR("compItem is NULL.");
        return HDF_FAILURE;
    }

    for (index = 0; index < codeData->numControls; index++) {
        if (compItem[index].type == controlType) {
            if (!compItem[index].enable) {
                codeData->controls[index].Get = getCtrl;
                codeData->controls[index].Set = setCtrl;
            }
        }
    }

    return HDF_SUCCESS;
}

// release I2C object public function
static void CodecI2cRelease(struct I2cMsg *msgs, int16_t msgSize, DevHandle i2cHandle)
{
    if (msgs != NULL) {
        if (msgSize == 0 && msgs->buf != NULL) {
            OsalMemFree(msgs->buf);
            msgs->buf = NULL;
        } else if (msgSize == 1 && msgs[0].buf != NULL) {
            OsalMemFree(msgs[0].buf);
            msgs[0].buf = NULL;
        } else if (msgSize >= I2C_MSG_NUM) {
            if (msgs[0].buf != NULL) {
                OsalMemFree(msgs[0].buf);
                msgs[0].buf = NULL;
            }
            if (msgs[1].buf != NULL) {
                OsalMemFree(msgs[1].buf);
                msgs[1].buf = NULL;
            }
        }
        AUDIO_DRIVER_LOG_DEBUG("OsalMemFree msgBuf success.\n");
    }
    // close i2c device
    if (i2cHandle != NULL) {
        I2cClose(i2cHandle);
        i2cHandle = NULL;
        AUDIO_DRIVER_LOG_DEBUG("I2cClose success.\n");
    }
}

static int32_t CodecI2cMsgFill(struct I2cTransferParam *i2cTransferParam, const struct AudioAddrConfig *regAttr,
    uint16_t rwFlag, uint8_t *regs, struct I2cMsg *msgs)
{
    uint8_t *msgBuf = NULL;

    if (i2cTransferParam == NULL || regAttr == NULL || regs == NULL || msgs == NULL) {
        AUDIO_DRIVER_LOG_ERR("input invalid parameter.");
        return HDF_ERR_INVALID_PARAM;
    }

    if (rwFlag != 0 && rwFlag != I2C_FLAG_READ) {
        AUDIO_DRIVER_LOG_ERR("invalid rwFlag value: %d.", rwFlag);
        return HDF_ERR_INVALID_PARAM;
    }
    regs[0] = regAttr->addr;
    msgs[0].addr = i2cTransferParam->i2cDevAddr;
    msgs[0].flags = 0;
    msgs[0].len = i2cTransferParam->i2cRegDataLen + 1;
    AUDIO_DRIVER_LOG_DEBUG("msgs[0].addr=0x%02x, regs[0]=0x%02x.", msgs[0].addr, regs[0]);

    if (rwFlag == 0) { // write
        // S 11011A2A1 0 A ADDR A MS1 A LS1 A <....> P
        msgBuf = OsalMemCalloc(i2cTransferParam->i2cRegDataLen + 1);
        if (msgBuf == NULL) {
            AUDIO_DRIVER_LOG_ERR("[write]: malloc buf failed!");
            return HDF_ERR_MALLOC_FAIL;
        }
        msgBuf[0] = regs[0];
        if (i2cTransferParam->i2cRegDataLen == I2C_MSG_BUF_SIZE_1) {
            msgBuf[1] = (uint8_t)regAttr->value;
        } else if (i2cTransferParam->i2cRegDataLen == I2C_MSG_BUF_SIZE_2) {
            msgBuf[1] = (regAttr->value >> COMM_SHIFT_8BIT); // High 8 bit
            msgBuf[I2C_MSG_BUF_SIZE_2] = (uint8_t)(regAttr->value & COMM_MASK_FF);    // Low 8 bit
        } else {
            AUDIO_DRIVER_LOG_ERR("i2cRegDataLen is invalid");
            return HDF_FAILURE;
        }
        msgs[0].buf = msgBuf;
    } else {
        // S 11011A2A1 0 A ADDR A Sr 11011A2A1 1 A MS1 A LS1 A <....> NA P
        msgBuf = OsalMemCalloc(i2cTransferParam->i2cRegDataLen);
        if (msgBuf == NULL) {
            AUDIO_DRIVER_LOG_ERR("[read]: malloc buf failed!");
            return HDF_ERR_MALLOC_FAIL;
        }
        msgs[0].len = 1;
        msgs[0].buf = regs;
        msgs[1].addr = i2cTransferParam->i2cDevAddr;
        msgs[1].flags = I2C_FLAG_READ;
        msgs[1].len = i2cTransferParam->i2cRegDataLen;
        msgs[1].buf = msgBuf;
    }

    return HDF_SUCCESS;
}

static int32_t CodecI2cTransfer(struct I2cTransferParam *i2cTransferParam, struct AudioAddrConfig *regAttr,
    uint16_t rwFlag)
{
    int32_t ret;
    DevHandle i2cHandle;
    int16_t transferMsgCount = 1;
    uint8_t regs[I2C_REG_LEN];
    struct I2cMsg msgs[I2C_MSG_NUM];
    (void)memset_s(msgs, sizeof(struct I2cMsg) * I2C_MSG_NUM, 0, sizeof(struct I2cMsg) * I2C_MSG_NUM);

    AUDIO_DRIVER_LOG_DEBUG("entry.\n");
    if (i2cTransferParam == NULL || regAttr == NULL || rwFlag > 1) {
        AUDIO_DRIVER_LOG_ERR("invalid parameter.");
        return HDF_ERR_INVALID_PARAM;
    }
    i2cHandle = I2cOpen(i2cTransferParam->i2cBusNumber);
    if (i2cHandle == NULL) {
        AUDIO_DRIVER_LOG_ERR("open i2cBus:%u failed!", i2cTransferParam->i2cBusNumber);
        return HDF_FAILURE;
    }
    if (rwFlag == I2C_FLAG_READ) {
        transferMsgCount = I2C_MSG_NUM;
    }
    ret = CodecI2cMsgFill(i2cTransferParam, regAttr, rwFlag, regs, msgs);
    if (ret != HDF_SUCCESS) {
        AUDIO_DRIVER_LOG_ERR("CodecI2cMsgFill failed!");
        I2cClose(i2cHandle);
        return HDF_FAILURE;
    }
    ret = I2cTransfer(i2cHandle, msgs, transferMsgCount);
    if (ret != transferMsgCount) {
        AUDIO_DRIVER_LOG_ERR("I2cTransfer err:%d", ret);
        CodecI2cRelease(msgs, transferMsgCount, i2cHandle);
        return HDF_FAILURE;
    }
    if (rwFlag == I2C_FLAG_READ) {
        if (i2cTransferParam->i2cRegDataLen == I2C_MSG_BUF_SIZE_1) {
            regAttr->value = msgs[1].buf[0];
        } else if (i2cTransferParam->i2cRegDataLen == I2C_MSG_BUF_SIZE_2) {
            regAttr->value = (msgs[1].buf[0] << COMM_SHIFT_8BIT) | msgs[1].buf[1]; // result value 16 bit
        } else {
            AUDIO_DRIVER_LOG_ERR("i2cRegDataLen is invalid");
            return HDF_FAILURE;
        }
        AUDIO_DRIVER_LOG_DEBUG("[read]: regAttr->regValue=0x%04x", regAttr->value);
    }

    CodecI2cRelease(msgs, transferMsgCount, i2cHandle);
    return HDF_SUCCESS;
}

int32_t CodecDeviceRegI2cRead(const struct CodecDevice *codec, uint32_t reg, uint32_t *val)
{
    int32_t ret;
    struct AudioAddrConfig regAttr;
    struct I2cTransferParam *i2cTransferParam = NULL;

    if (codec == NULL || codec->devData == NULL || val == NULL) {
        AUDIO_DRIVER_LOG_ERR("input para is NULL.");
        return HDF_ERR_INVALID_OBJECT;
    }

    i2cTransferParam = (struct I2cTransferParam *)codec->devData->privateParam;
    if (i2cTransferParam == NULL) {
        AUDIO_DRIVER_LOG_ERR("codec i2cTransferParam is NULL.");
        return HDF_FAILURE;
    }

    regAttr.addr = (uint8_t)reg;
    regAttr.value = 0;
    ret = CodecI2cTransfer(i2cTransferParam, &regAttr, I2C_FLAG_READ);
    if (ret != HDF_SUCCESS) {
        AUDIO_DRIVER_LOG_ERR("failed.");
        return HDF_FAILURE;
    }
    *val = regAttr.value;
    AUDIO_DRIVER_LOG_DEBUG("success");
    return HDF_SUCCESS;
}

int32_t CodecDeviceRegI2cWrite(const struct CodecDevice *codec, uint32_t reg, uint32_t value)
{
    int32_t ret;
    struct AudioAddrConfig regAttr;
    struct I2cTransferParam *i2cTransferParam = NULL;
    if (codec == NULL || codec->devData == NULL) {
        AUDIO_DRIVER_LOG_ERR("input para is NULL.");
        return HDF_FAILURE;
    }

    i2cTransferParam = (struct I2cTransferParam *)codec->devData->privateParam;
    if (i2cTransferParam == NULL) {
        AUDIO_DRIVER_LOG_ERR("codec i2cTransferParam is NULL.");
        return HDF_FAILURE;
    }

    regAttr.addr = (uint8_t)reg;
    regAttr.value = (uint16_t)value;
    ret = CodecI2cTransfer(i2cTransferParam, &regAttr, 0);
    if (ret != HDF_SUCCESS) {
        AUDIO_DRIVER_LOG_ERR("I2c Transfer failed.");
        return HDF_FAILURE;
    }
    AUDIO_DRIVER_LOG_DEBUG("success");
    return HDF_SUCCESS;
}

int32_t CodecDaiRegI2cRead(const struct DaiDevice *dai, uint32_t reg, uint32_t *value)
{
    int32_t ret;
    struct AudioAddrConfig regAttr;
    struct I2cTransferParam *i2cTransferParam = NULL;

    if (dai == NULL || dai->devData == NULL || value == NULL) {
        AUDIO_DRIVER_LOG_ERR("input para is NULL.");
        return HDF_ERR_INVALID_OBJECT;
    }

    i2cTransferParam = (struct I2cTransferParam *)dai->devData->privateParam;
    if (i2cTransferParam == NULL) {
        AUDIO_DRIVER_LOG_ERR("codec dai i2cTransferParam is NULL.");
        return HDF_FAILURE;
    }

    regAttr.addr = (uint8_t)reg;
    regAttr.value = 0;
    ret = CodecI2cTransfer(i2cTransferParam, &regAttr, I2C_FLAG_READ);
    if (ret != HDF_SUCCESS) {
        AUDIO_DRIVER_LOG_ERR("CodecI2cTransfer failed.");
        return HDF_FAILURE;
    }
    *value = regAttr.value;
    AUDIO_DRIVER_LOG_DEBUG("success");
    return HDF_SUCCESS;
}

int32_t CodecDaiRegI2cWrite(const struct DaiDevice *dai, uint32_t reg, uint32_t value)
{
    int32_t ret;
    struct AudioAddrConfig regAttr;
    struct I2cTransferParam *i2cTransferParam = NULL;
    if (dai == NULL || dai->devData == NULL) {
        AUDIO_DRIVER_LOG_ERR("input para is NULL.");
        return HDF_FAILURE;
    }

    i2cTransferParam = (struct I2cTransferParam *)dai->devData->privateParam;
    if (i2cTransferParam == NULL) {
        AUDIO_DRIVER_LOG_ERR("codec dai i2cTransferParam is NULL.");
        return HDF_FAILURE;
    }

    regAttr.addr = (uint8_t)reg;
    regAttr.value = (uint16_t)value;
    ret = CodecI2cTransfer(i2cTransferParam, &regAttr, 0);
    if (ret != HDF_SUCCESS) {
        AUDIO_DRIVER_LOG_ERR("codec I2c Transfer failed.");
        return HDF_FAILURE;
    }
    AUDIO_DRIVER_LOG_DEBUG("success");
    return HDF_SUCCESS;
}

int32_t CodecDeviceReadReg(const struct CodecDevice *codec, uint32_t reg, uint32_t *val)
{
    unsigned long virtualAddress;
    if (codec == NULL || codec->devData == NULL || val == NULL) {
        AUDIO_DRIVER_LOG_ERR("param val is null.");
        return HDF_FAILURE;
    }
    virtualAddress = codec->devData->virtualAddress;
    *val = OSAL_READL((void *)((uintptr_t)(virtualAddress + reg)));
    return HDF_SUCCESS;
}

int32_t CodecDeviceWriteReg(const struct CodecDevice *codec, uint32_t reg, uint32_t value)
{
    unsigned long virtualAddress;
    if (codec == NULL || codec->devData == NULL) {
        AUDIO_DRIVER_LOG_ERR("param val is null.");
        return HDF_FAILURE;
    }
    virtualAddress = codec->devData->virtualAddress;
    OSAL_WRITEL(value, (void *)((uintptr_t)(virtualAddress + reg)));
    return HDF_SUCCESS;
}

int32_t CodecDeviceInitRegConfig(const struct CodecDevice *device)
{
    int32_t ret;
    uint32_t index;
    struct AudioAddrConfig *initCfg = NULL;
    struct AudioRegCfgGroupNode **regCfgGroup = NULL;

    if (device == NULL || device->devData == NULL || device->devData->Write == NULL) {
        AUDIO_DRIVER_LOG_ERR("param val is null.");
        return HDF_FAILURE;
    }

    regCfgGroup = device->devData->regCfgGroup;
    if (regCfgGroup == NULL || regCfgGroup[AUDIO_INIT_GROUP] == NULL) {
        AUDIO_DRIVER_LOG_ERR("regCfgGroup init group is null.");
        return HDF_FAILURE;
    }

    initCfg = regCfgGroup[AUDIO_INIT_GROUP]->addrCfgItem;
    if (initCfg == NULL) {
        AUDIO_DRIVER_LOG_ERR("initCfg is NULL.");
        return HDF_FAILURE;
    }

    for (index = 0; index < regCfgGroup[AUDIO_INIT_GROUP]->itemNum; index++) {
        ret = device->devData->Write(device, initCfg[index].addr, initCfg[index].value);
        if (ret != HDF_SUCCESS) {
            AUDIO_DRIVER_LOG_ERR("Write err regAddr: 0x%x.\n", initCfg[index].addr);
            return HDF_FAILURE;
        }
        OsalMSleep(COMM_WAIT_TIMES);
    }
    AUDIO_DRIVER_LOG_DEBUG("success.");
    return HDF_SUCCESS;
}

int32_t CodecDaiDeviceStartupRegConfig(const struct DaiDevice *device)
{
    int32_t ret;
    uint16_t index;
    struct AudioMixerControl *startupRegCfgItem = NULL;
    uint16_t regCfgItemCount;
    struct AudioRegCfgGroupNode **regCfgGroup = NULL;

    if (device == NULL || device->devData == NULL) {
        AUDIO_DRIVER_LOG_ERR("param val is null.");
        return HDF_FAILURE;
    }

    regCfgGroup = device->devData->regCfgGroup;
    if (regCfgGroup == NULL || regCfgGroup[AUDIO_DAI_STARTUP_PATAM_GROUP] == NULL ||
        regCfgGroup[AUDIO_DAI_STARTUP_PATAM_GROUP]->regCfgItem == NULL) {
        AUDIO_DEVICE_LOG_ERR("regCfgGroup is NULL.");
        return HDF_FAILURE;
    }
    startupRegCfgItem = regCfgGroup[AUDIO_DAI_STARTUP_PATAM_GROUP]->regCfgItem;
    regCfgItemCount = regCfgGroup[AUDIO_DAI_STARTUP_PATAM_GROUP]->itemNum;

    for (index = 0; index < regCfgItemCount; index++) {
        ret = AudioDaiRegUpdate(device, &startupRegCfgItem[index]);
        if (ret != HDF_SUCCESS) {
            AUDIO_DEVICE_LOG_ERR("CodecDaiRegBitsUpdate fail.");
            return HDF_FAILURE;
        }
    }
    AUDIO_DEVICE_LOG_DEBUG("success.");
    return HDF_SUCCESS;
}

