/*
 * Copyright (c) 2022-2023 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include "hdf_io_service_if.h"
#include "hdf_log.h"
#include "adc_if.h"

#define HDF_LOG_TAG adc_if_u_c
#define ADC_SERVICE_NAME "HDF_PLATFORM_ADC_MANAGER"

static void *AdcManagerGetService(void)
{
    static void *manager = NULL;

    if (manager != NULL) {
        return manager;
    }
    manager = (void *)HdfIoServiceBind(ADC_SERVICE_NAME);
    if (manager == NULL) {
        HDF_LOGE("AdcManagerGetService: fail to get adc manager!");
    }
    return manager;
}

DevHandle AdcOpen(uint32_t number)
{
    int32_t ret;
    struct HdfIoService *service = NULL;
    struct HdfSBuf *data = NULL;
    struct HdfSBuf *reply = NULL;
    uint32_t handle;

    service = (struct HdfIoService *)AdcManagerGetService();
    if (service == NULL) {
        HDF_LOGE("AdcOpen: service is null!");
        return NULL;
    }
    data = HdfSbufObtainDefaultSize();
    if (data == NULL) {
        HDF_LOGE("AdcOpen: fail to obtain data!");
        return NULL;
    }
    reply = HdfSbufObtainDefaultSize();
    if (reply == NULL) {
        HDF_LOGE("AdcOpen: fail to obtain reply!");
        HdfSbufRecycle(data);
        return NULL;
    }

    if (!HdfSbufWriteUint32(data, (uint32_t)number)) {
        HDF_LOGE("AdcOpen: write number fail!");
        goto ERR;
    }

    ret = service->dispatcher->Dispatch(&service->object, ADC_IO_OPEN, data, reply);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("AdcOpen: service call ADC_IO_OPEN fail, ret: %d!", ret);
        goto ERR;
    }

    if (!HdfSbufReadUint32(reply, &handle)) {
        HDF_LOGE("AdcOpen: read handle fail!");
        goto ERR;
    }
    HdfSbufRecycle(data);
    HdfSbufRecycle(reply);
    return (DevHandle)(uintptr_t)handle;
ERR:
    HdfSbufRecycle(data);
    HdfSbufRecycle(reply);
    return NULL;
}

void AdcClose(DevHandle handle)
{
    int32_t ret;
    struct HdfIoService *service = NULL;
    struct HdfSBuf *data = NULL;

    service = (struct HdfIoService *)AdcManagerGetService();
    if (service == NULL) {
        HDF_LOGE("AdcClose: service is null!");
        return;
    }

    data = HdfSbufObtainDefaultSize();
    if (data == NULL) {
        HDF_LOGE("AdcClose: fail to obtain data!");
        return;
    }

    if (!HdfSbufWriteUint32(data, (uint32_t)(uintptr_t)handle)) {
        HDF_LOGE("AdcClose: write handle fail!");
        HdfSbufRecycle(data);
        return;
    }

    ret = service->dispatcher->Dispatch(&service->object, ADC_IO_CLOSE, data, NULL);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("AdcClose: close handle fail, ret: %d!", ret);
    }
    HdfSbufRecycle(data);
}

int32_t AdcRead(DevHandle handle, uint32_t channel, uint32_t *val)
{
    int32_t ret;
    struct HdfSBuf *data = NULL;
    struct HdfSBuf *reply = NULL;
    struct HdfIoService *service = NULL;
	
    service = (struct HdfIoService *)AdcManagerGetService();
    if (service == NULL) {
        HDF_LOGE("AdcRead: service is null!");
        return HDF_PAL_ERR_DEV_CREATE;
    }

    data = HdfSbufObtainDefaultSize();
    if (data == NULL) {
        HDF_LOGE("AdcRead: fail to obtain data!");
        return HDF_ERR_MALLOC_FAIL;
    }

    reply = HdfSbufObtainDefaultSize();
    if (reply == NULL) {
        HdfSbufRecycle(data);
        HDF_LOGE("AdcRead: fail to obtain reply!");
        return HDF_ERR_MALLOC_FAIL;
    }

    if (!HdfSbufWriteUint32(data, (uint32_t)(uintptr_t)handle)) {
        HDF_LOGE("AdcRead: write handle fail!");
        ret = HDF_ERR_IO;
        goto EXIT;
    }
    if (!HdfSbufWriteUint32(data, (uint32_t)channel)) {
        HDF_LOGE("AdcRead: write channel fail!");
        ret = HDF_ERR_IO;
        goto EXIT;
    }
    ret = service->dispatcher->Dispatch(&service->object, ADC_IO_READ, data, reply);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("AdcRead: service call ADC_IO_READ fail, ret: %d!", ret);
        goto EXIT;
    }

    if (!HdfSbufReadUint32(reply, val)) {
        HDF_LOGE("AdcRead: read val fail!");
        ret = HDF_ERR_IO;
        goto EXIT;
    }

    goto EXIT;
EXIT:
    HdfSbufRecycle(data);
    HdfSbufRecycle(reply);
    return ret;
}
