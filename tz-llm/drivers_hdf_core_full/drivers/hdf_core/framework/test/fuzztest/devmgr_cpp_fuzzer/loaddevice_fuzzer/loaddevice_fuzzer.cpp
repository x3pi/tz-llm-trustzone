/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include "loaddevice_fuzzer.h"
#include <cstddef>
#include <cstdint>
#include <hdf_base.h>
#include "hdf_log.h"
#include "idevmgr_hdi.h"
#include "parcel.h"

using namespace OHOS::HDI::DeviceManager::V1_0;

namespace OHOS {
constexpr size_t THRESHOLD = 10;

sptr<IDeviceManager> g_deviceManager = IDeviceManager::Get();

bool LoadDeviceFuzzTest(const uint8_t *data, size_t size)
{
    if (data == nullptr) {
        HDF_LOGE("%{public}s: data is nullptr!", __func__);
        return false;
    }

    bool result = false;
    Parcel parcel;
    parcel.WriteBuffer(data, size);
    auto servicename = parcel.ReadString();
    if (g_deviceManager != nullptr && g_deviceManager->LoadDevice(servicename) == HDF_SUCCESS) {
        result = true;
    }
    return result;
}
} // namespace OHOS

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    if (size < OHOS::THRESHOLD) {
        return HDF_SUCCESS;
    }

    OHOS::LoadDeviceFuzzTest(data, size);
    return HDF_SUCCESS;
}
