/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include "hdf_device_desc.h"
#include "hdf_log.h"

#define HDF_LOG_TAG uevent_ut_driver

static int32_t HdfUeventDriverDispatch(
    struct HdfDeviceIoClient *client, int cmd, struct HdfSBuf *data, struct HdfSBuf *reply)
{
    (void)client;
    (void)cmd;
    (void)data;
    (void)reply;

    return HDF_SUCCESS;
}

static void HdfUeventDriverRelease(struct HdfDeviceObject *deviceObject)
{
    (void)deviceObject;

    HDF_LOGD("%s enter", __func__);

    return;
}

static int HdfUeventDriverBind(struct HdfDeviceObject *deviceObject)
{
    if (deviceObject == NULL) {
        return HDF_FAILURE;
    }

    HDF_LOGD("%s enter", __func__);
    static struct IDeviceIoService testService = {
        .Dispatch = HdfUeventDriverDispatch,
        .Open = NULL,
        .Release = NULL,
    };

    deviceObject->service = &testService;

    return HDF_SUCCESS;
}

static int HdfUeventDriverInit(struct HdfDeviceObject *deviceObject)
{
    (void)deviceObject;

    HDF_LOGD("%s enter", __func__);

    return HDF_SUCCESS;
}

struct HdfDriverEntry g_ueventDriverEntry = {
    .moduleVersion = 1,
    .moduleName = "uevent_ut_driver",
    .Bind = HdfUeventDriverBind,
    .Init = HdfUeventDriverInit,
    .Release = HdfUeventDriverRelease,
};

HDF_INIT(g_ueventDriverEntry);
