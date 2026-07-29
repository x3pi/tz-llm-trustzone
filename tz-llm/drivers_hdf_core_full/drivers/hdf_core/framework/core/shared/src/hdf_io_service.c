/*
 * Copyright (c) 2020-2021 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include "hdf_io_service.h"

struct HdfIoService *HdfIoServiceBind(const char *serviceName)
{
    return HdfIoServiceAdapterObtain(serviceName);
}

void HdfIoServiceRecycle(struct HdfIoService *service)
{
    HdfIoServiceAdapterRecycle(service);
}

struct HdfIoService *HdfIoServicePublish(const char *serviceName, uint32_t mode)
{
    if (HdfIoServiceAdapterPublish != NULL) {
        return HdfIoServiceAdapterPublish(serviceName, mode);
    }

    return NULL;
}

void HdfIoServiceRemove(struct HdfIoService *service)
{
    if (HdfIoServiceAdapterRemove != NULL) {
        HdfIoServiceAdapterRemove(service);
    }
}

int32_t HdfIoServiceDispatch(struct HdfIoService *ioService, int cmdId, struct HdfSBuf *data, struct HdfSBuf *reply)
{
    if (ioService == NULL || ioService->dispatcher == NULL || ioService->dispatcher->Dispatch == NULL) {
        return HDF_ERR_INVALID_OBJECT;
    }

    return ioService->dispatcher->Dispatch(&ioService->object, cmdId, data, reply);
}
