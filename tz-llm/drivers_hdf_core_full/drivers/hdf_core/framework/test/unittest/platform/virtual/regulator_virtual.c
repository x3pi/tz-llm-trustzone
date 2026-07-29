/*
 * Copyright (c) 2021-2023 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

 /* hcs topology for example
dev  ---+-> Regulator-1(voltage) -+-> Regulator-2(voltage) -+-> Regulator-3(voltage) -+-> Regulator-4(voltage)
           |                              |
           |                              | -+-> Regulator-5(voltage) -+-> Regulator-6(voltage) -+-> Regulator-7(voltage) -+-> Regulator-8(voltage)
           |                                       |
           |                                       | -+-> Regulator-9
           |
         ---+-> Regulator-10(current)
           |
           |
         ---+-> Regulator-11(current) -+-> Regulator-12(current) -+-> Regulator-14(current)
           |                                   |
           |                                   | -+-> Regulator-13(current)
*/

#include "device_resource_if.h"
#include "hdf_log.h"
#include "osal_mem.h"
#include "regulator/regulator_core.h"

#define HDF_LOG_TAG regulator_virtual
#define VOLTAGE_2500_UV 2500
#define CURRENT_2500_UA 2500

static int32_t VirtualRegulatorEnable(struct RegulatorNode *node)
{
    if (node == NULL) {
        HDF_LOGE("VirtualRegulatorEnable node null\n");
        return HDF_ERR_INVALID_OBJECT;
    }

    node->regulatorInfo.status = REGULATOR_STATUS_ON;
    HDF_LOGD("VirtualRegulatorEnable %s success !\n", node->regulatorInfo.name);
    return HDF_SUCCESS;
}

static int32_t VirtualRegulatorDisable(struct RegulatorNode *node)
{
    if (node == NULL) {
        HDF_LOGE("VirtualRegulatorDisable node null\n");
        return HDF_ERR_INVALID_OBJECT;
    }

    node->regulatorInfo.status = REGULATOR_STATUS_OFF;
    HDF_LOGD("VirtualRegulatorDisable %s success !\n", node->regulatorInfo.name);
    return HDF_SUCCESS;
}

static int32_t VirtualRegulatorSetVoltage(struct RegulatorNode *node, uint32_t minUv, uint32_t maxUv)
{
    if (node == NULL) {
        HDF_LOGE("VirtualRegulatorEnable node null\n");
        return HDF_ERR_INVALID_OBJECT;
    }

    HDF_LOGD("VirtualRegulatorSetVoltage %s [%u, %u] success!\n",
        node->regulatorInfo.name, minUv, maxUv);
    return HDF_SUCCESS;
}

static int32_t VirtualRegulatorGetVoltage(struct RegulatorNode *node, uint32_t *voltage)
{
    if (node == NULL || voltage == NULL) {
        HDF_LOGE("VirtualRegulatorGetVoltage param null\n");
        return HDF_ERR_INVALID_OBJECT;
    }

    *voltage = VOLTAGE_2500_UV;
    HDF_LOGD("VirtualRegulatorGetVoltage get %s %d success !\n", node->regulatorInfo.name, *voltage);
    return HDF_SUCCESS;
}

static int32_t VirtualRegulatorSetCurrent(struct RegulatorNode *node, uint32_t minUa, uint32_t maxUa)
{
    if (node == NULL) {
        HDF_LOGE("VirtualRegulatorSetCurrent node null\n");
        return HDF_ERR_INVALID_OBJECT;
    }

    HDF_LOGD("VirtualRegulatorSetCurrent %s [%d, %d] success!\n",
        node->regulatorInfo.name, minUa, maxUa);
    return HDF_SUCCESS;
}

static int32_t VirtualRegulatorGetCurrent(struct RegulatorNode *node, uint32_t *current)
{
    if (node == NULL || current == NULL) {
        HDF_LOGE("VirtualRegulatorGetCurrent param null\n");
        return HDF_ERR_INVALID_OBJECT;
    }

    *current = CURRENT_2500_UA;
    HDF_LOGD("VirtualRegulatorGetCurrent get %s %u success !\n", node->regulatorInfo.name, *current);
    return HDF_SUCCESS;
}

static int32_t VirtualRegulatorGetStatus(struct RegulatorNode *node, uint32_t *status)
{
    if (node == NULL || status == NULL) {
        HDF_LOGE("VirtualRegulatorGetStatus param null\n");
        return HDF_ERR_INVALID_OBJECT;
    }

    *status = node->regulatorInfo.status;
    HDF_LOGD("VirtualRegulatorGetStatus get %s %d success !\n", node->regulatorInfo.name, *status);
    return HDF_SUCCESS;
}

static struct RegulatorMethod g_method = {
    .enable = VirtualRegulatorEnable,
    .disable = VirtualRegulatorDisable,
    .setVoltage = VirtualRegulatorSetVoltage,
    .getVoltage = VirtualRegulatorGetVoltage,
    .setCurrent = VirtualRegulatorSetCurrent,
    .getCurrent = VirtualRegulatorGetCurrent,
    .getStatus = VirtualRegulatorGetStatus,
};

static int32_t VirtualRegulatorContinueReadHcs(struct RegulatorNode *regNode, const struct DeviceResourceNode *node)
{
    int32_t ret;
    struct DeviceResourceIface *drsOps = NULL;

    HDF_LOGD("VirtualRegulatorContinueReadHcs enter!");

    drsOps = DeviceResourceGetIfaceInstance(HDF_CONFIG_SOURCE);
    if (drsOps == NULL || drsOps->GetString == NULL) {
        HDF_LOGE("VirtualRegulatorContinueReadHcs: invalid drs ops fail!");
        return HDF_FAILURE;
    }

    ret = drsOps->GetUint32(node, "minUv", &regNode->regulatorInfo.constraints.minUv, 0);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("VirtualRegulatorContinueReadHcs: read minUv fail, ret: %d!", ret);
        return ret;
    }

    ret = drsOps->GetUint32(node, "maxUv", &regNode->regulatorInfo.constraints.maxUv, 0);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("VirtualRegulatorContinueReadHcs: read maxUv fail, ret: %d!", ret);
        return ret;
    }

    ret = drsOps->GetUint32(node, "minUa", &regNode->regulatorInfo.constraints.minUa, 0);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("VirtualRegulatorContinueReadHcs: read minUa fail, ret: %d!", ret);
        return ret;
    }

    ret = drsOps->GetUint32(node, "maxUa", &regNode->regulatorInfo.constraints.maxUa, 0);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("VirtualRegulatorContinueReadHcs: read maxUa fail, ret: %d!", ret);
        return ret;
    }

    HDF_LOGD("VirtualRegulatorContinueReadHcs: regulatorInfo:[%s][%d][%d]--[%d][%d]--[%d][%d]!",
        regNode->regulatorInfo.name, regNode->regulatorInfo.constraints.alwaysOn,
        regNode->regulatorInfo.constraints.mode,
        regNode->regulatorInfo.constraints.minUv, regNode->regulatorInfo.constraints.maxUv,
        regNode->regulatorInfo.constraints.minUa, regNode->regulatorInfo.constraints.maxUa);

    return HDF_SUCCESS;
}

static int32_t VirtualRegulatorReadHcs(struct RegulatorNode *regNode, const struct DeviceResourceNode *node)
{
    int32_t ret;
    struct DeviceResourceIface *drsOps = NULL;

    HDF_LOGD("VirtualRegulatorReadHcs enter:");

    drsOps = DeviceResourceGetIfaceInstance(HDF_CONFIG_SOURCE);
    if (drsOps == NULL || drsOps->GetString == NULL) {
        HDF_LOGE("VirtualRegulatorReadHcs: invalid drs ops fail!");
        return HDF_FAILURE;
    }

    ret = drsOps->GetString(node, "name", &(regNode->regulatorInfo.name), "ERROR");
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("VirtualRegulatorReadHcs: read name fail, ret: %d!", ret);
        return ret;
    }
    if (regNode->regulatorInfo.name != NULL) {
        HDF_LOGD("VirtualRegulatorReadHcs:name[%s]", regNode->regulatorInfo.name);
    } else {
        HDF_LOGE("VirtualRegulatorReadHcs:name is null!");
        return HDF_FAILURE;
    }

    ret = drsOps->GetString(node, "parentName", &(regNode->regulatorInfo.parentName), "ERROR");
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("VirtualRegulatorReadHcs: read parentName fail, ret: %d!", ret);
        return ret;
    }
    if (regNode->regulatorInfo.parentName != NULL) {
        HDF_LOGD("VirtualRegulatorReadHcs:parentName[%s]", regNode->regulatorInfo.parentName);
    }

    regNode->regulatorInfo.constraints.alwaysOn = drsOps->GetBool(node, "alwaysOn");
    HDF_LOGD("VirtualRegulatorReadHcs:alwaysOn[%d]", regNode->regulatorInfo.constraints.alwaysOn);

    ret = drsOps->GetUint8(node, "mode", &regNode->regulatorInfo.constraints.mode, 0);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("VirtualRegulatorReadHcs: read mode fail, ret: %d!", ret);
        return ret;
    }

    if (VirtualRegulatorContinueReadHcs(regNode, node) != HDF_SUCCESS) {
        return HDF_FAILURE;
    }

    return HDF_SUCCESS;
}

static int32_t VirtualRegulatorParseAndInit(struct HdfDeviceObject *device, const struct DeviceResourceNode *node)
{
    int32_t ret;
    struct RegulatorNode *regNode = NULL;
    (void)device;

    regNode = (struct RegulatorNode *)OsalMemCalloc(sizeof(*regNode));
    if (regNode == NULL) {
        HDF_LOGE("VirtualRegulatorParseAndInit: malloc node fail!");
        return HDF_ERR_MALLOC_FAIL;
    }

    HDF_LOGD("VirtualRegulatorParseAndInit");

    ret = VirtualRegulatorReadHcs(regNode, node);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("VirtualRegulatorParseAndInit: read drs fail, ret: %d!", ret);
        OsalMemFree(regNode);
        regNode = NULL;
        return ret;
    }

    regNode->priv = (void *)node;
    regNode->ops = &g_method;

    ret = RegulatorNodeAdd(regNode);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("VirtualRegulatorParseAndInit: add regulator controller fail, ret: %d!", ret);
        OsalMemFree(regNode);
        regNode = NULL;
        return ret;
    }
    return HDF_SUCCESS;
}

static int32_t VirtualRegulatorInit(struct HdfDeviceObject *device)
{
    int32_t ret;
    const struct DeviceResourceNode *childNode = NULL;

    if (device == NULL || device->property == NULL) {
        HDF_LOGE("VirtualRegulatorInit: device or property is null!");
        return HDF_ERR_INVALID_OBJECT;
    }

    DEV_RES_NODE_FOR_EACH_CHILD_NODE(device->property, childNode) {
        ret = VirtualRegulatorParseAndInit(device, childNode);
        if (ret != HDF_SUCCESS) {
            HDF_LOGE("VirtualRegulatorInit: VirtualRegulatorParseAndInit fail, ret: %d!", ret);
            return ret;
        }
    }
    HDF_LOGI("VirtualRegulatorInit: success!");
    return HDF_SUCCESS;
}

static void VirtualRegulatorRelease(struct HdfDeviceObject *device)
{
    HDF_LOGI("VirtualRegulatorRelease: enter!");

    if (device == NULL || device->property == NULL) {
        HDF_LOGE("VirtualRegulatorRelease: device or property is null!");
        return;
    }

    RegulatorNodeRemoveAll();
}

struct HdfDriverEntry g_regulatorDriverEntry = {
    .moduleVersion = 1,
    .moduleName = "virtual_regulator_driver",
    .Init = VirtualRegulatorInit,
    .Release = VirtualRegulatorRelease,
};
HDF_INIT(g_regulatorDriverEntry);
