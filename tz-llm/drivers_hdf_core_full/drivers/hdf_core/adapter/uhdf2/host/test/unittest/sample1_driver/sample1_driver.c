/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#include "devhost_dump_reg.h"
#include "hdf_base.h"
#include "hdf_device_desc.h"
#include "hdf_device_object.h"
#include "hdf_log.h"
#include "power_state_token.h"

#define HDF_LOG_TAG sample1_driver

struct HdfSample1Service {
    struct IDeviceIoService ioservice;
};

struct SampleDriverPmListener {
    struct IPowerEventListener powerListener;
    void *p;
};

static int HdfSampleDozeResume(struct HdfDeviceObject *deviceObject)
{
    (void)deviceObject;
    HDF_LOGI("%{public}s:called", __func__);
    return HDF_SUCCESS;
}

static int HdfSampleDozeSuspend(struct HdfDeviceObject *deviceObject)
{
    (void)deviceObject;
    HDF_LOGI("%{public}s:called", __func__);
    return HDF_SUCCESS;
}

static int HdfSampleResume(struct HdfDeviceObject *deviceObject)
{
    (void)deviceObject;
    HDF_LOGI("%{public}s:called", __func__);
    return HDF_SUCCESS;
}

static int HdfSampleSuspend(struct HdfDeviceObject *deviceObject)
{
    (void)deviceObject;
    HDF_LOGI("%{public}s:called", __func__);
    return HDF_SUCCESS;
}

static struct SampleDriverPmListener *GetPmListenerInstance(void)
{
    static struct SampleDriverPmListener pmListener;
    static bool init = false;
    if (!init) {
        init = true;
        pmListener.powerListener.DozeResume = HdfSampleDozeResume;
        pmListener.powerListener.DozeSuspend = HdfSampleDozeSuspend;
        pmListener.powerListener.Resume = HdfSampleResume;
        pmListener.powerListener.Suspend = HdfSampleSuspend;
    }

    return &pmListener;
}

static void HdfSample1DriverRelease(struct HdfDeviceObject *deviceObject)
{
    HDF_LOGI("HdfSample1DriverRelease enter!");
    struct SampleDriverPmListener *pmListener = GetPmListenerInstance();
    HdfPmUnregisterPowerListener(deviceObject, &pmListener->powerListener);

    return;
}

static int HdfSample1DriverBind(struct HdfDeviceObject *deviceObject)
{
    HDF_LOGI("HdfSample1DriverBind enter!");
    if (deviceObject == NULL) {
        return HDF_FAILURE;
    }
    int ret = HdfDeviceObjectSetInterfaceDesc(deviceObject, "hdf.test.sampele_service");
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("failed to set interface desc");
    }
    static struct HdfSample1Service sample1Driver;
    deviceObject->service = &sample1Driver.ioservice;
    return HDF_SUCCESS;
}

static int32_t DevHostSample1DumpHost(struct HdfSBuf *data, struct HdfSBuf *reply)
{
    uint32_t argv = 0;

    (void)HdfSbufReadUint32(data, &argv);
    HDF_LOGI("%{public}d", argv);

    const char *value = HdfSbufReadString(data);
    while (value != NULL && argv > 0) {
        HDF_LOGI("%{public}s", value);
        value = HdfSbufReadString(data);
        argv--;
    }

    HdfSbufWriteString(reply, "sample_host_dump\n");

    return HDF_SUCCESS;
}

static int32_t DevHostSample1DumpService(struct HdfSBuf *data, struct HdfSBuf *reply)
{
    uint32_t argv = 0;

    (void)HdfSbufReadUint32(data, &argv);
    HDF_LOGI("%{public}d", argv);

    const char *para = HdfSbufReadString(data);
    while (para != NULL && argv > 0) {
        HDF_LOGI("%{public}s", para);
        para = HdfSbufReadString(data);
        argv--;
    }

    HdfSbufWriteString(reply, "sample1_service_dump\n");

    return HDF_SUCCESS;
}

static int HdfSample1DriverInit(struct HdfDeviceObject *deviceObject)
{
    HDF_LOGI("HdfSample1DriverInit enter!");
    if (HdfDeviceObjectSetServInfo(deviceObject, "sample1_driver_service") != HDF_SUCCESS) {
        HDF_LOGE("failed to set service info");
    }
    struct SampleDriverPmListener *pmListener = GetPmListenerInstance();
    int ret = HdfPmRegisterPowerListener(deviceObject, &pmListener->powerListener);
    HDF_LOGI("%s:register power listener, ret = %{public}d", __func__, ret);
    (void)DevHostRegisterDumpService("sample1_driver_service", DevHostSample1DumpService);
    (void)DevHostRegisterDumpHost(DevHostSample1DumpHost);
    return HDF_SUCCESS;
}

static struct HdfDriverEntry g_sample1DriverEntry = {
    .moduleVersion = 1,
    .moduleName = "sample1_driver",
    .Bind = HdfSample1DriverBind,
    .Init = HdfSample1DriverInit,
    .Release = HdfSample1DriverRelease,
};

HDF_INIT(g_sample1DriverEntry);
