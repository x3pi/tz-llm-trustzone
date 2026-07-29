/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include "queryusabledeviceInfo_fuzzer.h"
#include <cstddef>
#include <cstdint>
#include <hdf_base.h>
#include "devmgr_hdi.h"
#include "hdf_log.h"
#include "parcel.h"

namespace OHOS {
constexpr size_t THRESHOLD = 10;

bool QueryUnusableDeviceInfoFuzzTest(const uint8_t *rawData, size_t size)
{
    if (rawData == nullptr) {
        HDF_LOGE("%{public}s: rawData is nullptr!", __func__);
        return false;
    }

    bool result = false;
    Parcel parcel;
    parcel.WriteBuffer(rawData, size);
    struct DeviceInfoList list;
    list.deviceCnt = parcel.ReadUint32();
    struct HDIDeviceManager *devmgr = HDIDeviceManagerGet();
    if (devmgr == nullptr) {
        HDF_LOGE("%{public}s: devmgr is nullptr!", __func__);
        return false;
    }

    int32_t ret = devmgr->QueryUnusableDeviceInfo(devmgr, &list);
    if (ret == HDF_SUCCESS) {
        result = true;
    }
    HDIDeviceManagerRelease(devmgr);

    return result;
}
} // namespace OHOS

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    if (size < OHOS::THRESHOLD) {
        return HDF_SUCCESS;
    }

    /* Run your code on data */
    OHOS::QueryUnusableDeviceInfoFuzzTest(data, size);
    return HDF_SUCCESS;
}
