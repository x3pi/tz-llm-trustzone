/*
 * Copyright (c) 2021-2023 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */
#include "regulator_if.h"
#include "hdf_log.h"
#include "regulator_core.h"

DevHandle RegulatorOpen(const char *name)
{
    if (name == NULL) {
        HDF_LOGE("RegulatorOpen: name is null!");
        return NULL;
    }

    return (DevHandle)RegulatorNodeOpen(name);
}

void RegulatorClose(DevHandle handle)
{
    struct RegulatorNode *node = (struct RegulatorNode *)handle;

    if (node == NULL) {
        HDF_LOGE("RegulatorClose: node is null!");
        return;
    }

    if (RegulatorNodeClose(node) != HDF_SUCCESS) {
        HDF_LOGE("RegulatorClose: RegulatorNodeClose fail!");
        return;
    }
}

int32_t RegulatorEnable(DevHandle handle)
{
    struct RegulatorNode *node = (struct RegulatorNode *)handle;

    if (node == NULL) {
        HDF_LOGE("RegulatorEnable: node is null!");
        return HDF_FAILURE;
    }

    int ret = RegulatorNodeEnable(node);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("RegulatorEnable: RegulatorNodeEnable fail!");
        return ret;
    }

    return HDF_SUCCESS;
}

int32_t RegulatorDisable(DevHandle handle)
{
    struct RegulatorNode *node = (struct RegulatorNode *)handle;

    if (node == NULL) {
        HDF_LOGE("RegulatorDisable: node is null!");
        return HDF_FAILURE;
    }

    int ret = RegulatorNodeDisable(node);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("RegulatorDisable: RegulatorNodeDisable fail!");
        return ret;
    }

    return HDF_SUCCESS;
}

int32_t RegulatorForceDisable(DevHandle handle)
{
    struct RegulatorNode *node = (struct RegulatorNode *)handle;

    if (node == NULL) {
        HDF_LOGE("RegulatorForceDisable: node is null!");
        return HDF_FAILURE;
    }

    int ret = RegulatorNodeForceDisable(node);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("RegulatorForceDisable: RegulatorNodeForceDisable fail!");
        return ret;
    }

    return HDF_SUCCESS;
}

int32_t RegulatorSetVoltage(DevHandle handle, uint32_t minUv, uint32_t maxUv)
{
    struct RegulatorNode *node = (struct RegulatorNode *)handle;

    if (node == NULL) {
        HDF_LOGE("RegulatorSetVoltage: node is null!");
        return HDF_FAILURE;
    }

    if (minUv > maxUv) {
        HDF_LOGE("RegulatorSetVoltage: %s Uv [%u, %u] invalid!",
            node->regulatorInfo.name, minUv, maxUv);
        return HDF_FAILURE;
    }

    int ret = RegulatorNodeSetVoltage(node, minUv, maxUv);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("RegulatorSetVoltage: RegulatorNodeSetVoltage fail!");
        return HDF_FAILURE;
    }

    return HDF_SUCCESS;
}

int32_t RegulatorGetVoltage(DevHandle handle, uint32_t *voltage)
{
    struct RegulatorNode *node = (struct RegulatorNode *)handle;

    if (node == NULL || voltage == NULL) {
        HDF_LOGE("RegulatorGetVoltage: param is null!");
        return HDF_FAILURE;
    }

    int ret = RegulatorNodeGetVoltage(node, voltage);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("RegulatorGetVoltage: RegulatorNodeGetVoltage fail!");
        return HDF_FAILURE;
    }

    return HDF_SUCCESS;
}

int32_t RegulatorSetCurrent(DevHandle handle, uint32_t minUa, uint32_t maxUa)
{
    struct RegulatorNode *node = (struct RegulatorNode *)handle;

    if (node == NULL) {
        HDF_LOGE("RegulatorSetCurrent: node is null!");
        return HDF_FAILURE;
    }

    if (minUa > maxUa) {
        HDF_LOGE("RegulatorSetCurrent: %s Ua [%u, %u] invalid!",
            node->regulatorInfo.name, minUa, maxUa);
        return HDF_FAILURE;
    }

    int ret = RegulatorNodeSetCurrent(node, minUa, maxUa);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("RegulatorSetCurrent: RegulatorNodeSetCurrent fail!");
        return HDF_FAILURE;
    }

    return HDF_SUCCESS;
}

int32_t RegulatorGetCurrent(DevHandle handle, uint32_t *regCurrent)
{
    struct RegulatorNode *node = (struct RegulatorNode *)handle;

    if (node == NULL || regCurrent == NULL) {
        HDF_LOGE("RegulatorGetCurrent: node or regCurrent is null!");
        return HDF_FAILURE;
    }

    int ret = RegulatorNodeGetCurrent(node, regCurrent);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("RegulatorGetCurrent: RegulatorNodeGetCurrent fail!");
        return HDF_FAILURE;
    }

    return HDF_SUCCESS;
}

int32_t RegulatorGetStatus(DevHandle handle, uint32_t *status)
{
    struct RegulatorNode *node = (struct RegulatorNode *)handle;

    if (node == NULL || status == NULL) {
        HDF_LOGE("RegulatorGetStatus: node or status is null!");
        return HDF_FAILURE;
    }

    int ret = RegulatorNodeGetStatus(node, status);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("RegulatorGetStatus: RegulatorNodeGetStatus fail!");
        return HDF_FAILURE;
    }

    return HDF_SUCCESS;
}
