/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */
#ifndef HDF_SERVICE_STATUS_INNER_H
#define HDF_SERVICE_STATUS_INNER_H

#include "hdf_service_status.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

struct HdfSBuf;

enum ServiceStatusListenerCmd {
    SERVIE_STATUS_LISTENER_NOTIFY,
    SERVIE_STATUS_LISTENER_MAX,
};

int ServiceStatusMarshalling(struct ServiceStatus *status, struct HdfSBuf *buf);
int ServiceStatusUnMarshalling(struct ServiceStatus *status, struct HdfSBuf *buf);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* HDF_SERVICE_STATUS_INNER_H */