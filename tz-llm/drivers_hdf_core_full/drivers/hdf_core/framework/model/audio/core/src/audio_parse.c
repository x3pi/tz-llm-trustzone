/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include "audio_parse.h"
#include "audio_driver_log.h"

#define HDF_LOG_TAG HDF_AUDIO_KADM

#define HW_INFO "hwInfo"
#define PORT_INFO_LIST_LENGHT 12

enum AudioRegCfgIndex {
    AUDIO_REG_CFG_REG_INDEX = 0,
    AUDIO_REG_CFG_RREG_INDEX,
    AUDIO_REG_CFG_SHIFT_INDEX,
    AUDIO_REG_CFG_RSHIFT_INDEX,
    AUDIO_REG_CFG_MIN_INDEX,
    AUDIO_REG_CFG_MAX_INDEX,
    AUDIO_REG_CFG_MASK_INDEX,
    AUDIO_REG_CFG_INVERT_INDEX,
    AUDIO_REG_CFG_VALUE_INDEX,
    AUDIO_REG_CFG_INDEX_MAX
};

enum AudioEnumRegCfgIndex {
    AUDIO_ENUM_REG_CFG_REG_INDEX = 0,
    AUDIO_ENUM_REG_CFG_RREG_INDEX,
    AUDIO_ENUM_REG_CFG_SHIFT_INDEX,
    AUDIO_ENUM_REG_CFG_RSHIFT_INDEX,
    AUDIO_ENUM_REG_CFG_MAX_INDEX,
    AUDIO_ENUM_REG_CFG_MASK_INDEX,
    AUDIO_ENUM_REG_CFG_TEXTS_INDEX,
    AUDIO_ENUM_REG_CFG_VALUE_INDEX,
    AUDIO_ENUM_REG_CFG_SAPM_INDEX,
    AUDIO_ENUM_REG_CFG_INDEX_MAX
};

enum AudioAddrCfgIndex {
    AUDIO_ADDR_CFG_REG_INDEX = 0,
    AUDIO_ADDR_CFG_VALUE_INDEX,
    AUDIO_ADDR_CFG_INDEX_MAX
};

enum AudioCrtlCfgIndex {
    AUDIO_CTRL_CFG_INDEX_INDEX = 0,
    AUDIO_CTRL_CFG_IFACE_INDEX,
    AUDIO_CTRL_CFG_TYPE_INDEX,
    AUDIO_CTRL_CFG_ENABLE_INDEX,
    AUDIO_CTRL_CFG_INDEX_MAX
};

enum AudioSapmComponentIndex {
    AUDIO_SAPM_COMP_INDEX_TYPE  = 0,
    AUDIO_SAPM_COMP_INDEX_NAME,
    AUDIO_SAPM_COMP_INDEX_REG,
    AUDIO_SAPM_COMP_INDEX_MASK,
    AUDIO_SAPM_COMP_INDEX_SHIFT,
    AUDIO_SAPM_COMP_INDEX_INVERT,
    AUDIO_SAPM_COMP_INDEX_KCTL,
    AUDIO_SAPM_COMP_INDEX_KCTLNUM,
    AUDIO_SAPM_COMP_INDEX_MAX
};


static char *g_audioRegGroupName[AUDIO_GROUP_MAX] = {
    "resetSeqConfig",
    "initSeqConfig",
    "ctrlParamsSeqConfig",
    "ctrlParamsMuxSeqConfig",
    "ctrlSapmParamsSeqConfig",
    "ctrlSapmMuxParamsSeqConfig",
    "daiStartupSeqConfig",
    "daiParamsSeqConfig",
    "daiTriggerSeqConfig",
    "controlsConfig",
    "sapmComponent",
    "sapmConfig"
};

int32_t AudioFillConfigData(const struct HdfDeviceObject *device, struct AudioConfigData *configData)
{
    const struct DeviceResourceNode *node = NULL;
    struct DeviceResourceIface *drsOps = NULL;

    ADM_LOG_DEBUG("Entry.");
    if (device == NULL || configData == NULL) {
        ADM_LOG_ERR("Input para check error");
        return HDF_FAILURE;
    }

    node = device->property;
    if (node == NULL) {
        ADM_LOG_ERR("drs node is NULL.");
        return HDF_FAILURE;
    }
    drsOps = DeviceResourceGetIfaceInstance(HDF_CONFIG_SOURCE);
    if (drsOps == NULL || drsOps->GetString == NULL) {
        ADM_LOG_ERR("AudioFillConfigData: invalid drs ops fail!");
        return HDF_FAILURE;
    }

    (void)drsOps->GetString(node, "serviceName", &(configData->cardServiceName), 0);
    (void)drsOps->GetString(node, "codecName", &(configData->codecName), 0);
    (void)drsOps->GetString(node, "platformName", &(configData->platformName), 0);
    (void)drsOps->GetString(node, "cpuDaiName", &(configData->cpuDaiName), 0);
    (void)drsOps->GetString(node, "codecDaiName", &(configData->codecDaiName), 0);
    (void)drsOps->GetString(node, "dspName", &(configData->dspName), 0);
    (void)drsOps->GetString(node, "dspDaiName", &(configData->dspDaiName), 0);

    ADM_LOG_INFO("cardServiceName = %s", configData->cardServiceName);
    ADM_LOG_INFO("codecName       = %s, codecDaiName = %s", configData->codecName, configData->codecDaiName);
    ADM_LOG_INFO("platformName    = %s, cpuDaiNamei = %s", configData->platformName, configData->cpuDaiName);
    ADM_LOG_INFO("dspName         = %s, dspDaiName = %s", configData->dspName, configData->dspDaiName);

    return HDF_SUCCESS;
}

static uint32_t GetAudioRegGroupNameIndex(const char *name)
{
    uint32_t index;

    if (name == NULL) {
        return AUDIO_GROUP_MAX;
    }

    for (index = 0; index < AUDIO_GROUP_MAX; ++index) {
        if ((g_audioRegGroupName[index] != NULL) && (strcmp(name, g_audioRegGroupName[index]) == 0)) {
            break;
        }
    }

    return index;
}

static uint32_t* GetRegArray(const struct DeviceResourceIface *parser, const struct DeviceResourceNode *regNode,
    struct AudioRegCfgGroupNode* group, uint32_t indexMax)
{
    int32_t ret;
    int32_t index;
    int32_t num;
    uint32_t *buf = NULL;
    if (group == NULL || parser == NULL || regNode == NULL || indexMax == 0) {
        ADM_LOG_ERR("Input para check error");
        return NULL;
    }

    index = group->groupIndex;
    if (index >= AUDIO_GROUP_MAX) {
        ADM_LOG_ERR("Input indexMax=%d error", index);
        return NULL;
    }

    num = parser->GetElemNum(regNode, g_audioRegGroupName[index]);
    if (num <= 0 || num > AUDIO_CONFIG_MAX_ITEM) {
        ADM_LOG_ERR("parser %s element num failed", g_audioRegGroupName[index]);
        return NULL;
    }

    group->itemNum =  num / indexMax;

    buf = (uint32_t *)OsalMemCalloc(sizeof(uint32_t) * num);
    if (buf == NULL) {
        ADM_LOG_ERR("malloc reg array buf failed!");
        return NULL;
    }

    ret = parser->GetUint32Array(regNode, g_audioRegGroupName[index], buf, num, 0);
    if (ret != HDF_SUCCESS) {
        ADM_LOG_ERR("parser %s reg array failed", g_audioRegGroupName[index]);
        OsalMemFree(buf);
        return NULL;
    }
    return buf;
}

static int32_t ParseAudioRegItem(const struct DeviceResourceIface *parser, const struct DeviceResourceNode *regNode,
    struct AudioRegCfgGroupNode* group)
{
    int32_t step;
    int32_t index;
    int32_t *buf = NULL;
    if (group == NULL || parser == NULL || regNode == NULL) {
        ADM_LOG_ERR("Input para check error");
        return HDF_FAILURE;
    }

    buf = GetRegArray(parser, regNode, group, AUDIO_REG_CFG_INDEX_MAX);
    if (buf == NULL) {
        ADM_LOG_ERR("malloc reg array buf failed!");
        return HDF_FAILURE;
    }

    group->regCfgItem =
        (struct AudioMixerControl*)OsalMemCalloc(group->itemNum * sizeof(*(group->regCfgItem)));
    if (group->regCfgItem == NULL) {
        OsalMemFree(buf);
        ADM_LOG_ERR("malloc audio reg config item failed!");
        return HDF_ERR_MALLOC_FAIL;
    }

    for (index = 0; index < group->itemNum; ++index) {
        step = AUDIO_REG_CFG_INDEX_MAX * index;

        group->regCfgItem[index].reg    = buf[step + AUDIO_REG_CFG_REG_INDEX];
        group->regCfgItem[index].rreg   = buf[step + AUDIO_REG_CFG_RREG_INDEX];
        group->regCfgItem[index].shift  = buf[step + AUDIO_REG_CFG_SHIFT_INDEX];
        group->regCfgItem[index].rshift = buf[step + AUDIO_REG_CFG_RSHIFT_INDEX];
        group->regCfgItem[index].min    = buf[step + AUDIO_REG_CFG_MIN_INDEX];
        group->regCfgItem[index].max    = buf[step + AUDIO_REG_CFG_MAX_INDEX];
        group->regCfgItem[index].mask   = buf[step + AUDIO_REG_CFG_MASK_INDEX];
        group->regCfgItem[index].invert = buf[step + AUDIO_REG_CFG_INVERT_INDEX];
        group->regCfgItem[index].value  = buf[step + AUDIO_REG_CFG_VALUE_INDEX];
    }
    OsalMemFree(buf);

    return HDF_SUCCESS;
}

static int32_t ParseAudioEnumRegItem(const struct DeviceResourceIface *parser, const struct DeviceResourceNode *regNode,
    struct AudioRegCfgGroupNode* group)
{
    int32_t step;
    int32_t index;
    int32_t *buf = NULL;

    if (group == NULL || parser == NULL || regNode == NULL) {
        ADM_LOG_ERR("Input para check error");
        return HDF_FAILURE;
    }

    buf = GetRegArray(parser, regNode, group, AUDIO_ENUM_REG_CFG_INDEX_MAX);
    if (buf == NULL) {
        ADM_LOG_ERR("malloc reg array buf failed!");
        return HDF_FAILURE;
    }

    group->regEnumCfgItem =
        (struct AudioEnumCtrlConfig*)OsalMemCalloc(group->itemNum * sizeof(*(group->regEnumCfgItem)));
    if (group->regEnumCfgItem == NULL) {
        OsalMemFree(buf);
        ADM_LOG_ERR("malloc audio Enum reg config item is failed!");
        return HDF_ERR_MALLOC_FAIL;
    }

    for (index = 0; index < group->itemNum; ++index) {
        step = AUDIO_ENUM_REG_CFG_INDEX_MAX * index;

        group->regEnumCfgItem[index].reg    = buf[step + AUDIO_ENUM_REG_CFG_REG_INDEX];
        group->regEnumCfgItem[index].reg2   = buf[step + AUDIO_ENUM_REG_CFG_RREG_INDEX];
        group->regEnumCfgItem[index].shiftLeft  = buf[step + AUDIO_ENUM_REG_CFG_SHIFT_INDEX];
        group->regEnumCfgItem[index].shiftRight = buf[step + AUDIO_ENUM_REG_CFG_RSHIFT_INDEX];
        group->regEnumCfgItem[index].max    = buf[step + AUDIO_ENUM_REG_CFG_MAX_INDEX];
        group->regEnumCfgItem[index].mask   = buf[step + AUDIO_ENUM_REG_CFG_MASK_INDEX];
        group->regEnumCfgItem[index].texts  = buf[step + AUDIO_ENUM_REG_CFG_TEXTS_INDEX];
        group->regEnumCfgItem[index].values = buf[step + AUDIO_ENUM_REG_CFG_VALUE_INDEX];
        group->regEnumCfgItem[index].sapm   = buf[step + AUDIO_ENUM_REG_CFG_SAPM_INDEX];
    }
    OsalMemFree(buf);

    return HDF_SUCCESS;
}

static int32_t ParseAudioSapmItem(const struct DeviceResourceIface *parser, const struct DeviceResourceNode *regNode,
    struct AudioRegCfgGroupNode* group)
{
    int32_t step;
    int32_t index;
    uint32_t *buf = NULL;
    if (group == NULL || parser == NULL || regNode == NULL) {
        ADM_LOG_ERR("Input para check error");
        return HDF_FAILURE;
    }

    buf = GetRegArray(parser, regNode, group, AUDIO_SAPM_COMP_INDEX_MAX);
    if (buf == NULL) {
        ADM_LOG_ERR("malloc reg array buf failed!");
        return HDF_FAILURE;
    }

    group->sapmCompItem =
        (struct AudioSapmCtrlConfig*)OsalMemCalloc(group->itemNum * sizeof(*(group->sapmCompItem)));
    if (group->sapmCompItem == NULL) {
        OsalMemFree(buf);
        ADM_LOG_ERR("malloc audio reg config item failed!");
        return HDF_ERR_MALLOC_FAIL;
    }

    for (index = 0; index < group->itemNum; ++index) {
        step = AUDIO_SAPM_COMP_INDEX_MAX * index;
        group->sapmCompItem[index].sapmType       = buf[step + AUDIO_SAPM_COMP_INDEX_TYPE];
        group->sapmCompItem[index].compNameIndex  = buf[step + AUDIO_SAPM_COMP_INDEX_NAME];
        group->sapmCompItem[index].reg            = buf[step + AUDIO_SAPM_COMP_INDEX_REG];
        group->sapmCompItem[index].mask           = buf[step + AUDIO_SAPM_COMP_INDEX_MASK];
        group->sapmCompItem[index].shift          = buf[step + AUDIO_SAPM_COMP_INDEX_SHIFT];
        group->sapmCompItem[index].invert         = buf[step + AUDIO_SAPM_COMP_INDEX_INVERT];
        group->sapmCompItem[index].kcontrolNews   = buf[step + AUDIO_SAPM_COMP_INDEX_KCTL];
        group->sapmCompItem[index].kcontrolsNum   = buf[step + AUDIO_SAPM_COMP_INDEX_KCTLNUM];
    }

    OsalMemFree(buf);
    return HDF_SUCCESS;
}


static int32_t ParseAudioCtrlItem(const struct DeviceResourceIface *parser, const struct DeviceResourceNode *regNode,
    struct AudioRegCfgGroupNode* group)
{
    int32_t step;
    int32_t index;
    uint32_t *buf = NULL;
    if (parser == NULL || regNode == NULL || group == NULL) {
        ADM_LOG_ERR("Input para check error");
        return HDF_FAILURE;
    }

    buf = GetRegArray(parser, regNode, group, AUDIO_CTRL_CFG_INDEX_MAX);
    if (buf == NULL) {
        ADM_LOG_ERR("malloc reg array buf failed!");
        return HDF_FAILURE;
    }

    group->ctrlCfgItem =
        (struct AudioControlConfig*)OsalMemCalloc(group->itemNum * sizeof(*(group->ctrlCfgItem)));
    if (group->ctrlCfgItem == NULL) {
        OsalMemFree(buf);
        ADM_LOG_ERR("malloc audio ctrl config item failed!");
        return HDF_ERR_MALLOC_FAIL;
    }

    for (index = 0; index < group->itemNum; ++index) {
        step = AUDIO_CTRL_CFG_INDEX_MAX * index;

        group->ctrlCfgItem[index].arrayIndex = buf[step + AUDIO_CTRL_CFG_INDEX_INDEX];
        group->ctrlCfgItem[index].iface      = buf[step + AUDIO_CTRL_CFG_IFACE_INDEX];
        group->ctrlCfgItem[index].type       = buf[step + AUDIO_CTRL_CFG_TYPE_INDEX];
        group->ctrlCfgItem[index].enable     = buf[step + AUDIO_CTRL_CFG_ENABLE_INDEX];
    }
    OsalMemFree(buf);
    return HDF_SUCCESS;
}

static int32_t ParseAudioAddrItem(const struct DeviceResourceIface *parser, const struct DeviceResourceNode *regNode,
    struct AudioRegCfgGroupNode* group)
{
    int32_t step;
    int32_t index;
    uint32_t *buf = NULL;

    if (parser == NULL || regNode == NULL || group == NULL) {
        ADM_LOG_ERR("Input para check error.");
        return HDF_FAILURE;
    }

    buf = GetRegArray(parser, regNode, group, AUDIO_ADDR_CFG_INDEX_MAX);
    if (buf == NULL) {
        ADM_LOG_ERR("malloc reg array buf failed!");
        return HDF_FAILURE;
    }

    group->addrCfgItem = (struct AudioAddrConfig*)OsalMemCalloc(group->itemNum * sizeof(*(group->addrCfgItem)));
    if (group->addrCfgItem == NULL) {
        OsalMemFree(buf);
        ADM_LOG_ERR("malloc audio addr config item failed!");
        return HDF_ERR_MALLOC_FAIL;
    }

    for (index = 0; index < group->itemNum; ++index) {
        step = AUDIO_ADDR_CFG_INDEX_MAX * index;
        group->addrCfgItem[index].addr  = buf[step + AUDIO_ADDR_CFG_REG_INDEX];
        group->addrCfgItem[index].value = buf[step + AUDIO_ADDR_CFG_VALUE_INDEX];
    }
    OsalMemFree(buf);
    return HDF_SUCCESS;
}

static int32_t ParseAudioRegGroup(const struct DeviceResourceIface *parser,
    const struct DeviceResourceNode *regCfgNode, struct AudioRegCfgGroupNode **groupNode, uint32_t index)
{
    int32_t ret = HDF_FAILURE;
    struct AudioRegCfgGroupNode *group = NULL;

    if (parser == NULL || regCfgNode == NULL || groupNode == NULL) {
        ADM_LOG_ERR("Input para check error");
        return HDF_FAILURE;
    }

    group = (struct AudioRegCfgGroupNode*)OsalMemCalloc(sizeof(*group));
    if (group == NULL) {
        ADM_LOG_ERR("malloc audio reg config group failed");
        return HDF_ERR_MALLOC_FAIL;
    }
    *groupNode = group;
    (*groupNode)->groupIndex = index;

    switch (index) {
        case AUDIO_CTRL_CFG_GROUP:
        case AUDIO_SAPM_CFG_GROUP:
            ret = ParseAudioCtrlItem(parser, regCfgNode, group);
            break;
        case AUDIO_RSET_GROUP:
        case AUDIO_INIT_GROUP:
            ret = ParseAudioAddrItem(parser, regCfgNode, group);
            break;
        case AUDIO_DAI_PATAM_GROUP:
        case AUDIO_DAI_TRIGGER_GROUP:
        case AUDIO_CTRL_PATAM_GROUP:
        case AUDIO_CTRL_SAPM_PATAM_GROUP:
        case AUDIO_DAI_STARTUP_PATAM_GROUP:
            ret = ParseAudioRegItem(parser, regCfgNode, group);
            break;
        case AUDIO_CTRL_PATAM_MUX_GROUP:
        case AUDIO_CTRL_SAPM_PATAM_MUX_GROUP:
            ret = ParseAudioEnumRegItem(parser, regCfgNode, group);
            break;
        case AUDIO_SAPM_COMP_GROUP:
            ret = ParseAudioSapmItem(parser, regCfgNode, group);
            break;
        default:
            ADM_LOG_ERR("parse audio config index = %u not found!", index);
            return HDF_FAILURE;
    }

    if (ret != HDF_SUCCESS) {
        ADM_LOG_ERR("parse audio config item failed!");
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

static void ReleaseAudioAllRegConfig(struct AudioRegCfgData *config)
{
    int32_t index;

    if (config == NULL) {
        return;
    }

    for (index = 0; index < AUDIO_GROUP_MAX; ++index) {
        if (config->audioRegParams[index] != NULL) {
            if (config->audioRegParams[index]->regCfgItem != NULL) {
                OsalMemFree(config->audioRegParams[index]->regCfgItem);
                config->audioRegParams[index]->regCfgItem = NULL;
            }
            OsalMemFree(config->audioRegParams[index]);
            config->audioRegParams[index] = NULL;
        }
    }
}

static int32_t ParseAudioAttr(const struct DeviceResourceIface *parser, const struct DeviceResourceNode *attrNode,
    struct AudioIdInfo *config)
{
    int32_t ret;
    ret = parser->GetString(attrNode, "chipName", &config->chipName, NULL);
    if (ret != HDF_SUCCESS) {
        ADM_LOG_ERR("parser chipName reg audioIdInfo failed!");
        return HDF_SUCCESS;
    }

    ret = parser->GetUint32(attrNode, "chipIdRegister", &config->chipIdRegister, 0);
    if (ret != HDF_SUCCESS) {
        ADM_LOG_ERR("parser chipIdRegister reg audioIdInfo failed!");
        return HDF_SUCCESS;
    }

    ret = parser->GetUint32(attrNode, "chipIdSize", &config->chipIdSize, 0);
    if (ret != HDF_SUCCESS) {
        ADM_LOG_ERR("parser chipIdSize reg audioIdInfo failed!");
        return HDF_SUCCESS;
    }
    return ret;
}

static int32_t AudioSetPortInfoConfig(const uint64_t *buf, struct AudioPcmStream *info)
{
    int32_t ret;

    ret = memcpy_s(info, sizeof(struct AudioPcmStream), buf, sizeof(uint64_t) * PORT_INFO_LIST_LENGHT);
    if (ret != 0) {
        ADM_LOG_ERR("memcpy_s error ret = %d!", ret);
        return HDF_FAILURE;
    }

    return HDF_SUCCESS;
}

static int32_t AudioSetPortInfoConfigStub(const uint64_t *buf, struct AudioPortInfo *configData)
{
    switch (buf[0]) {  /* Playback/Captrue */
        case PORT_OUT: /* Playback */
            return AudioSetPortInfoConfig(buf, &configData->render);
        case PORT_IN: /* Captrue */
            return AudioSetPortInfoConfig(buf, &configData->capture);
        default:
            ADM_LOG_ERR("portDirection = %llu element num failed", buf[0]);
            return HDF_FAILURE;
    }
}

static int32_t AudioGetPortInfoConfig(struct DeviceResourceIface *drsOps, const struct HdfDeviceObject *device,
    struct AudioPortInfo *configData)
{
    uint32_t num;
    uint32_t ret;
    uint64_t *buf = NULL;

    num = drsOps->GetElemNum(device->property, HW_INFO);
    if (num == 0 || num > AUDIO_CONFIG_MAX_ITEM) {
        ADM_LOG_ERR("parser %s element num failed", HW_INFO);
        return HDF_FAILURE;
    }

    buf = (uint64_t *)OsalMemCalloc(sizeof(uint64_t) * num);
    if (buf == NULL) {
        ADM_LOG_ERR("malloc reg array buf failed!");
        return HDF_FAILURE;
    }

    ret = drsOps->GetUint64Array(device->property, HW_INFO, buf, num, 0);
    if (ret != HDF_SUCCESS) {
        OsalMemFree(buf);
        ADM_LOG_ERR("GetChildNode: Read portCfgNode fail!");
        return HDF_FAILURE;
    }

    (void)memset_s(configData, sizeof(struct AudioPortInfo), 0, sizeof(struct AudioPortInfo));

    switch (num) {
        case PORT_INFO_LIST_LENGHT:
            AudioSetPortInfoConfigStub(buf, configData);
            break;
        case PORT_INFO_LIST_LENGHT + PORT_INFO_LIST_LENGHT:
            AudioSetPortInfoConfigStub(buf, configData);
            AudioSetPortInfoConfigStub(buf + PORT_INFO_LIST_LENGHT, configData);
            break;
        default:
            OsalMemFree(buf);
            ADM_LOG_ERR("configData->portDirection num is not matched! num = %d", num);
            return HDF_FAILURE;
    }

    OsalMemFree(buf);
    return HDF_SUCCESS;
}

int32_t AudioGetPortConfig(const struct HdfDeviceObject *device, struct AudioPortInfo *configData)
{
    uint32_t ret;
    struct DeviceResourceIface *drsOps = NULL;

    if (device == NULL || device->property == NULL || configData == NULL) {
        ADM_LOG_ERR("Input para check error");
        return HDF_FAILURE;
    }

    drsOps = DeviceResourceGetIfaceInstance(HDF_CONFIG_SOURCE);
    if (drsOps == NULL || drsOps->GetString == NULL) {
        ADM_LOG_ERR("AudioGetPortConfig: invalid drs ops fail!");
        return HDF_FAILURE;
    }

    ret = AudioGetPortInfoConfig(drsOps, device, configData);
    if (ret != HDF_SUCCESS) {
        ADM_LOG_ERR("parser chipIdRegister reg audioIdInfo failed!");
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

int32_t AudioGetRegConfig(const struct HdfDeviceObject *device, struct AudioRegCfgData *configData)
{
    uint16_t index;
    const struct DeviceResourceNode *regCfgNode = NULL;
    const struct DeviceResourceAttr *regAttr = NULL;
    const struct DeviceResourceNode *idNode = NULL;
    struct DeviceResourceIface *drsOps = NULL;

    if (device == NULL || device->property == NULL || configData == NULL) {
        ADM_LOG_ERR("Input para check error");
        return HDF_FAILURE;
    }

    drsOps = DeviceResourceGetIfaceInstance(HDF_CONFIG_SOURCE);
    if (drsOps == NULL || drsOps->GetString == NULL) {
        ADM_LOG_ERR("AudioFillConfigData: invalid drs ops fail!");
        return HDF_FAILURE;
    }

    idNode = drsOps->GetChildNode(device->property, "idInfo");
    if (idNode != NULL) {
        if (ParseAudioAttr(drsOps, idNode, &configData->audioIdInfo) != HDF_SUCCESS) {
            ADM_LOG_ERR("audio reg node attr is null");
            return HDF_FAILURE;
        }
    }

    regCfgNode = drsOps->GetChildNode(device->property, "regConfig");
    if (regCfgNode == NULL) {
        ADM_LOG_ERR("AudioGetRegConfig: Read audioRegConfig fail!");
        return HDF_FAILURE;
    }

    DEV_RES_NODE_FOR_EACH_ATTR(regCfgNode, regAttr) {
        if (regAttr == NULL || regAttr->name == NULL) {
            ADM_LOG_ERR("audio reg node attr is null");
            return HDF_FAILURE;
        }

        index = GetAudioRegGroupNameIndex(regAttr->name);
        if (index >= AUDIO_GROUP_MAX) {
            continue;
        }

        if (ParseAudioRegGroup(drsOps, regCfgNode, &configData->audioRegParams[index], index) != HDF_SUCCESS) {
            ADM_LOG_ERR("parse audio register group failed");
            ReleaseAudioAllRegConfig(configData);
            return HDF_FAILURE;
        }
    }
    return HDF_SUCCESS;
}
