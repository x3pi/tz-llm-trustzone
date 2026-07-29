/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <devhost_dump_reg.h>
#include <hdf_device_desc.h>
#include <hdf_device_object.h>
#include <hdf_log.h>
#include "sample_hdi.h"

#define HDF_LOG_TAG sample_driver

static int32_t SampleServiceDispatch(
    struct HdfDeviceIoClient *client, int cmdId, struct HdfSBuf *data, struct HdfSBuf *reply)
{
    return SampleServiceOnRemoteRequest(client, cmdId, data, reply);
}

static void HdfSampleDriverRelease(struct HdfDeviceObject *deviceObject)
{
    (void)deviceObject;
    return;
}

static int HdfSampleDriverBind(struct HdfDeviceObject *deviceObject)
{
    HDF_LOGE("HdfSampleDriverBind enter!");
    static struct IDeviceIoService testService = {
        .Open = nullptr,
        .Dispatch = SampleServiceDispatch,
        .Release = nullptr,
    };
    int ret = HdfDeviceObjectSetInterfaceDesc(deviceObject, "hdf.test.sampele_service");
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("failed to set interface desc");
        return ret;
    }
    deviceObject->service = &testService;
    return HDF_SUCCESS;
}

static int32_t DevHostSampleDumpService(struct HdfSBuf *data, struct HdfSBuf *reply)
{
    uint32_t argv = 0;

    (void)HdfSbufReadUint32(data, &argv);
    HDF_LOGI("%{public}d", argv);

    const char *str = HdfSbufReadString(data);
    while (str != NULL && argv > 0) {
        HDF_LOGI("%{public}s read:%{public}s", __func__, str);
        str = HdfSbufReadString(data);
        argv--;
    }

    HdfSbufWriteString(reply, "sample_service_dump\n");

    return HDF_SUCCESS;
}

static int HdfSampleDriverInit(struct HdfDeviceObject *deviceObject)
{
    HDF_LOGE("HdfSampleDriverInit enter, new hdi impl");
    if (HdfDeviceObjectSetServInfo(deviceObject, "sample_driver_service") != HDF_SUCCESS) {
        HDF_LOGE("failed to set service info");
    }
    (void)DevHostRegisterDumpService("sample_driver_service", DevHostSampleDumpService);
    return HDF_SUCCESS;
}

static struct HdfDriverEntry g_sampleDriverEntry = {
    .moduleVersion = 1,
    .moduleName = "sample_driver",
    .Bind = HdfSampleDriverBind,
    .Init = HdfSampleDriverInit,
    .Release = HdfSampleDriverRelease,
};

extern "C" {
HDF_INIT(g_sampleDriverEntry);
}