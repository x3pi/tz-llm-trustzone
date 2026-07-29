/*
 * Copyright (c) 2020-2023 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include "uart_core.h"
#include "hdf_log.h"
#include "osal_mem.h"
#include "uart_if.h"

#define HDF_LOG_TAG uart_core_c

int32_t UartHostRequest(struct UartHost *host)
{
    int32_t ret;

    if (host == NULL) {
        HDF_LOGE("UartHostRequest: host is null!");
        return HDF_ERR_INVALID_PARAM;
    }

    if (host->method == NULL || host->method->Init == NULL) {
        HDF_LOGE("UartHostRequest: method or init is null!");
        return HDF_ERR_NOT_SUPPORT;
    }

    if (OsalAtomicIncReturn(&host->atom) > 1) {
        HDF_LOGE("UartHostRequest: uart device is busy!");
        OsalAtomicDec(&host->atom);
        return HDF_ERR_DEVICE_BUSY;
    }

    ret = host->method->Init(host);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("UartHostRequest: host init fail!");
        OsalAtomicDec(&host->atom);
        return ret;
    }

    return HDF_SUCCESS;
}

int32_t UartHostRelease(struct UartHost *host)
{
    int32_t ret;

    if (host == NULL) {
        HDF_LOGE("UartHostRelease: host or method is null!");
        return HDF_ERR_INVALID_PARAM;
    }

    if (host->method == NULL || host->method->Deinit == NULL) {
        HDF_LOGE("UartHostRelease: method or Deinit is null!");
        return HDF_ERR_NOT_SUPPORT;
    }

    ret = host->method->Deinit(host);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("UartHostRelease: host deinit fail!");
        return ret;
    }

    OsalAtomicDec(&host->atom);
    return HDF_SUCCESS;
}

void UartHostDestroy(struct UartHost *host)
{
    if (host == NULL) {
        HDF_LOGE("UartHostDestroy: host is null!");
        return;
    }
    OsalMemFree(host);
}

struct UartHost *UartHostCreate(struct HdfDeviceObject *device)
{
    struct UartHost *host = NULL;

    if (device == NULL) {
        HDF_LOGE("UartHostCreate: device is null!");
        return NULL;
    }

    host = (struct UartHost *)OsalMemCalloc(sizeof(*host));
    if (host == NULL) {
        HDF_LOGE("UartHostCreate: memcalloc error!");
        return NULL;
    }

    host->device = device;
    device->service = &(host->service);
    host->device->service->Dispatch = UartIoDispatch;
    OsalAtomicSet(&host->atom, 0);
    host->priv = NULL;
    host->method = NULL;
    return host;
}
