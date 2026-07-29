/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include <cstddef>
#include <cstdint>

#include "hdf_base.h"
#include "hdf_io_service.h"
#include "hdf_log.h"
#include "parcel.h"
#include "ioserviceadapterobtain_fuzzer.h"

namespace OHOS {
constexpr size_t THRESHOLD = 10;

bool IoserviceAdapterObtainFuzzTest(const uint8_t *data, size_t size)
{
    if (data == nullptr) {
        HDF_LOGE("%{public}s: data is nullptr!", __func__);
        return false;
    }

    bool result = false;
    Parcel parcel;
    parcel.WriteBuffer(data, size);
    auto servicename = parcel.ReadCString();
    struct HdfIoService *testServ = HdfIoServiceAdapterObtain(servicename);
    if (testServ != nullptr) {
        result = true;
    }
    HdfIoServiceRecycle(testServ);
    return result;
}
} // namespace OHOS

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    if (size < OHOS::THRESHOLD) {
        return HDF_SUCCESS;
    }

    OHOS::IoserviceAdapterObtainFuzzTest(data, size);
    return HDF_SUCCESS;
}
