/*
 * Copyright (c) 2022-2023 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include "watchdog_fuzzer.h"

using namespace std;

namespace OHOS {
static bool WatchdogFuzzTest(const uint8_t *data, size_t size)
{
    DevHandle handle = nullptr;

    if (data == nullptr) {
        HDF_LOGE("WatchdogFuzzTest: data is nullptr!");
        return false;
    }
    if (WatchdogOpen(0, &handle) != HDF_SUCCESS) {
        HDF_LOGE("WatchdogFuzzTest: open watchdog fail!");
        return false;
    }
    WatchdogSetTimeout(handle, *(reinterpret_cast<const uint32_t *>(data)));
    WatchdogClose(handle);
    return true;
}
} // namespace OHOS

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    /* Run your code on data */
    OHOS::WatchdogFuzzTest(data, size);
    return 0;
}
