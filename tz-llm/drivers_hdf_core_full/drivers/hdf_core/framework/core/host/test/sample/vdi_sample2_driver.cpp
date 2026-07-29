/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include "hdf_base.h"
#include "hdf_load_vdi.h"
#include "hdf_log.h"
#include "vdi_sample2_driver.h"

#define HDF_LOG_TAG vdi_sample2

namespace OHOS {
namespace VDI {
namespace Sample {
namespace V1_0 {
VdiSample::VdiSample(int para)
{
    priData = para;
}

int VdiSample::ServiceA(void)
{
    HDF_LOGI("%{public}s", __func__);
    return HDF_SUCCESS;
}

int VdiSample::ServiceB(IVdiSample *vdi)
{
    VdiSample *sample = reinterpret_cast<VdiSample *>(vdi);
    HDF_LOGI("%{public}s %{public}d", __func__, sample->priData);
    return HDF_SUCCESS;
}

static int SampleAOpen(struct HdfVdiBase *vdiBase)
{
    HDF_LOGI("%{public}s", __func__);
    struct VdiWrapperB *sampleB = reinterpret_cast<struct VdiWrapperB *>(vdiBase);
    sampleB->module = new VdiSample(1);
    return HDF_SUCCESS;
}

static int SampleAClose(struct HdfVdiBase *vdiBase)
{
    HDF_LOGI("%{public}s", __func__);
    struct VdiWrapperB *sampleB = reinterpret_cast<struct VdiWrapperB *>(vdiBase);
    VdiSample *sample = reinterpret_cast<VdiSample *>(sampleB->module);
    delete sample;
    sampleB->module = nullptr;
    return HDF_SUCCESS;
}

static struct VdiWrapperB g_vdiB = {
    .base = {
        .moduleVersion = 1,
        .moduleName = "SampleServiceB",
        .CreateVdiInstance = SampleAOpen,
        .DestoryVdiInstance = SampleAClose,
    },
    .module = nullptr,
};
} // namespace V1_0
} // namespace Sample
} // namespace VDI
} // namespace OHOS

HDF_VDI_INIT(OHOS::VDI::Sample::V1_0::g_vdiB);
