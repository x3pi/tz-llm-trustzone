/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include "clock_if.h"
#include "clock_core.h"
#include "hdf_log.h"
#include "osal_mem.h"
#include "securec.h"
#define HDF_LOG_TAG clock_if_c

DevHandle ClockOpen(uint32_t number)
{
    if (ClockDeviceOpen(number) == NULL) {
        HDF_LOGE("ClockOpen: get device %u fail!", number);
        return NULL;
    }
    return (DevHandle)(number + CLOCK_HANDLE_SHIFT);
}

int32_t ClockClose(DevHandle handle)
{
    int32_t ret;
    uint32_t number = 0;
    struct ClockDevice *device = NULL;

    number = (uint32_t)handle - CLOCK_HANDLE_SHIFT;
    if (number >= CLOCK_DEVICES_MAX) {
        HDF_LOGE("ClockClose: invalid number!");
        return HDF_ERR_INVALID_PARAM;
    }

    device = ClockDeviceGet(number);
    if (device == NULL) {
        HDF_LOGE("ClockClose: get clock device fail!");
        return HDF_ERR_INVALID_PARAM;
    }

    ret = ClockDeviceClose(device);
    return ret;
}

int32_t ClockEnable(DevHandle handle)
{
    int32_t ret;
    uint32_t number = 0;
    struct ClockDevice *device = NULL;

    number = (uint32_t)handle - CLOCK_HANDLE_SHIFT;
    if (number >= CLOCK_DEVICES_MAX) {
        HDF_LOGE("ClockEnable: invalid number!");
        return HDF_ERR_INVALID_PARAM;
    }

    device = ClockDeviceGet(number);
    if (device == NULL) {
        HDF_LOGE("ClockEnable: get clock device fail!");
        return HDF_ERR_INVALID_PARAM;
    }

    ret = ClockDeviceEnable(device);
    return ret;
}

int32_t ClockDisable(DevHandle handle)
{
    int32_t ret;
    uint32_t number = 0;
    struct ClockDevice *device = NULL;

    number = (uint32_t)handle - CLOCK_HANDLE_SHIFT;
    if (number >= CLOCK_DEVICES_MAX) {
        HDF_LOGE("ClockDisable: invalid number!");
        return HDF_ERR_INVALID_PARAM;
    }

    device = ClockDeviceGet(number);
    if (device == NULL) {
        HDF_LOGE("ClockDisable: get clock device fail!");
        return HDF_ERR_INVALID_PARAM;
    }

    ret = ClockDeviceDisable(device);
    return ret;
}

int32_t ClockSetRate(DevHandle handle, uint32_t rate)
{
    int32_t ret;
    uint32_t number = 0;
    struct ClockDevice *device = NULL;

    number = (uint32_t)handle - CLOCK_HANDLE_SHIFT;
    if (number >= CLOCK_DEVICES_MAX) {
        HDF_LOGE("ClockSetRate: invalid number!");
        return HDF_ERR_INVALID_PARAM;
    }

    device = ClockDeviceGet(number);
    if (device == NULL) {
        HDF_LOGE("ClockSetRate: get clock device fail!");
        return HDF_ERR_INVALID_PARAM;
    }

    return ClockDeviceSetRate(device, rate);
}

int32_t ClockGetRate(DevHandle handle, uint32_t *rate)
{
    int32_t ret;
    uint32_t number = 0;
    struct ClockDevice *device = NULL;

    number = (uint32_t)handle - CLOCK_HANDLE_SHIFT;
    if (number >= CLOCK_DEVICES_MAX) {
        HDF_LOGE("ClockGetRate: invalid number!");
        return HDF_ERR_INVALID_PARAM;
    }

    device = ClockDeviceGet(number);
    if (device == NULL) {
        HDF_LOGE("ClockGetRate: get clock device fail!");
        return HDF_ERR_INVALID_PARAM;
    }

    ret = ClockDeviceGetRate(device, rate);
    return ret;
}

DevHandle ClockGetParent(DevHandle handle)
{
    int32_t ret;
    uint32_t number = 0;
    struct ClockDevice *device = NULL;
    struct ClockDevice *parent = NULL;

    number = (uint32_t)handle - CLOCK_HANDLE_SHIFT;
    if (number >= CLOCK_DEVICES_MAX) {
        HDF_LOGE("ClockGetParent: invalid number!");
        return HDF_ERR_INVALID_PARAM;
    }

    device = ClockDeviceGet(number);
    if (device == NULL) {
        HDF_LOGE("ClockGetParent: get clock device fail!");
        return HDF_ERR_INVALID_PARAM;
    }

    parent = ClockDeviceGetParent(device);
    if (parent == NULL) {
        HDF_LOGE("ClockGetParent: get clock parent fail!");
        return HDF_ERR_INVALID_PARAM;
    }

    return (DevHandle)(parent->deviceIndex + CLOCK_HANDLE_SHIFT);
}

int32_t ClockSetParent(DevHandle handle, DevHandle parent)
{
    uint32_t number = 0;
    uint32_t number_p = 0;
    struct ClockDevice *device = NULL;
    struct ClockDevice *pdevices = NULL;
    int32_t ret;

    number = (uint32_t)handle - CLOCK_HANDLE_SHIFT;
    if (number >= CLOCK_DEVICES_MAX) {
        HDF_LOGE("ClockSetParent: invalid number!");
        return HDF_ERR_INVALID_PARAM;
    }

    number_p = (uint32_t)parent - CLOCK_HANDLE_SHIFT;
    if (number_p >= CLOCK_DEVICES_MAX) {
        HDF_LOGE("ClockSetParent: invalid number_p!");
        return HDF_ERR_INVALID_PARAM;
    }

    device = ClockDeviceGet(number);
    if (device == NULL) {
        HDF_LOGE("ClockSetParent: get clock device fail!");
        return HDF_ERR_INVALID_PARAM;
    }

    pdevices = ClockDeviceGet(number_p);
    if (parent == NULL) {
        HDF_LOGE("ClockManagerSetParent: get clock pdevices fail!");
        return HDF_ERR_INVALID_PARAM;
    }

    ret = ClockDeviceSetParent(device, pdevices);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("ClockManagerSetParent: number_p =%u , number =%u ", number_p, number);
    }

    return ret;
}