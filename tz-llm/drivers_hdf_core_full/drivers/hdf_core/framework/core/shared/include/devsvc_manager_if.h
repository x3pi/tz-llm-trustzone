/*
 * Copyright (c) 2020-2022 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#ifndef DEVSVC_MANAGER_IF_H
#define DEVSVC_MANAGER_IF_H

#include "devsvc_listener_holder.h"
#include "hdf_device_desc.h"
#include "hdf_object.h"
#include "hdf_service_info.h"

struct IDevSvcManager {
    struct HdfObject object;
    int (*StartService)(struct IDevSvcManager *);
    int (*AddService)(struct IDevSvcManager *, struct HdfDeviceObject *, const struct HdfServiceInfo *);
    int (*UpdateService)(struct IDevSvcManager *, struct HdfDeviceObject *, const struct HdfServiceInfo *);
    int (*SubscribeService)(struct IDevSvcManager *, const char *, struct SubscriberCallback);
    int (*UnsubscribeService)(struct IDevSvcManager *, const char *);
    struct HdfObject *(*GetService)(struct IDevSvcManager *, const char *);
    struct HdfDeviceObject *(*GetObject)(struct IDevSvcManager *, const char *);
    void (*RemoveService)(struct IDevSvcManager *, const char *, const struct HdfDeviceObject *);
    int (*RegsterServListener)(struct IDevSvcManager *, struct ServStatListenerHolder *);
    void (*UnregsterServListener)(struct IDevSvcManager *, struct ServStatListenerHolder *);
    void (*ListAllService)(struct IDevSvcManager *, struct HdfSBuf *);
    int (*ListServiceByInterfaceDesc)(struct IDevSvcManager *, const char *, struct HdfSBuf *);
};

#endif /* DEVSVC_MANAGER_IF_H */
