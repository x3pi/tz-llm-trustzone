/*
 * Copyright (c) 2022-2023 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include "pwm_fuzzer.h"
#include <iostream>
#include "random.h"
#include "securec.h"
#include "hdf_base.h"
#include "hdf_log.h"
#include "pwm_if.h"

using namespace std;

namespace {
constexpr int32_t MIN = 0;
constexpr int32_t MAX = 2;
constexpr int32_t PWM_FUZZ_NUM = 1;
}

struct AllParameters {
    uint32_t descPer;
    uint32_t descDuty;
    uint8_t descPolar;
};

namespace OHOS {
static bool PwmFuzzTest(const uint8_t *data, size_t size)
{
    int32_t number;
    DevHandle handle = nullptr;
    const struct AllParameters *params = reinterpret_cast<const struct AllParameters *>(data);

    number = randNum(MIN, MAX);
    handle = PwmOpen(PWM_FUZZ_NUM);
    if (handle == nullptr) {
        HDF_LOGE("PwmFuzzTest: handle is nullptr!");
        return false;
    }
    switch (static_cast<ApiTestCmd>(number)) {
        case ApiTestCmd::PWM_FUZZ_SET_PERIOD:
            PwmSetPeriod(handle, params->descPer);
            break;
        case ApiTestCmd::PWM_FUZZ_SET_DUTY:
            PwmSetDuty(handle, params->descDuty);
            break;
        case ApiTestCmd::PWM_FUZZ_SET_POLARITY:
            PwmSetPolarity(handle, params->descPolar);
            break;
        default:
            break;
    }
    PwmClose(handle);
    return true;
}
} // namespace OHOS

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    /* Run your code on data */
    if (data == nullptr) {
        HDF_LOGE("LLVMFuzzerTestOneInput: pwm fuzz test data is nullptr!");
        return 0;
    }

    if (size < sizeof(struct AllParameters)) {
        HDF_LOGE("LLVMFuzzerTestOneInput: pwm fuzz test size is small!");
        return 0;
    }
    OHOS::PwmFuzzTest(data, size);
    return 0;
}
