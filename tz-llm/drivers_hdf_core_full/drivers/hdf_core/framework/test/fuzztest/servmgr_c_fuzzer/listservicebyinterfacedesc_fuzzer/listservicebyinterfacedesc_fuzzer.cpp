/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include "listservicebyinterfacedesc_fuzzer.h"
#include <cstddef>
#include <cstdint>
#include "hdf_base.h"
#include "hdf_log.h"
#include "parcel.h"
#include "servmgr_hdi.h"

namespace OHOS {
constexpr size_t THRESHOLD = 10;

bool ListServiceByInterfaceDescFuzzTest(const uint8_t *rawData, size_t size)
{
    if (rawData == nullptr) {
        HDF_LOGE("%{public}s: rawData is nullptr!", __func__);
        return false;
    }

    Parcel parcel;
    parcel.WriteBuffer(rawData, size);
    auto interfaceName = parcel.ReadCString();

    struct HDIServiceManager *servmgr = HDIServiceManagerGet();
    if (servmgr == nullptr) {
        HDF_LOGE("%{public}s: serviceSet is nullptr!", __func__);
        return false;
    }

    struct HdiServiceSet *serviceSet = servmgr->ListServiceByInterfaceDesc(servmgr, interfaceName);
    HDIServiceManagerRelease(servmgr);
    if (serviceSet == nullptr) {
        HDF_LOGE("%{public}s: ListServiceByInterfaceDesc failed!", __func__);
        return false;
    }
    HdiServiceSetRelease(serviceSet);

    return true;
}
} // namespace OHOS

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    if (size < OHOS::THRESHOLD) {
        return HDF_SUCCESS;
    }

    /* Run your code on data */
    OHOS::ListServiceByInterfaceDescFuzzTest(data, size);
    return HDF_SUCCESS;
}
