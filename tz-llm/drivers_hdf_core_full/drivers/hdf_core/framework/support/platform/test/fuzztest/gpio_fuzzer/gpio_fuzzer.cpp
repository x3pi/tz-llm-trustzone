/*
 * Copyright (c) 2022-2023 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include "gpio_fuzzer.h"
#include <iostream>
#include "random.h"
#include "securec.h"
#include "gpio_if.h"
#include "hdf_base.h"
#include "hdf_log.h"

using namespace std;

namespace {
constexpr int32_t MIN = 0;
constexpr int32_t MAX = 2;
constexpr uint16_t GPIO_TEST_NUM = 3;
}

struct AllParameters {
    uint16_t descVal;
    uint16_t descDir;
    uint16_t descMode;
};

static int32_t GpioTestIrqHandler(uint16_t gpio, void *data)
{
    (void)gpio;
    (void)data;
    return 0;
}

namespace OHOS {
static bool GpioFuzzTest(const uint8_t *data, size_t size)
{
    int32_t number;
    const struct AllParameters *params = reinterpret_cast<const struct AllParameters *>(data);

    number = randNum(MIN, MAX);
    switch (static_cast<ApiTestCmd>(number)) {
        case ApiTestCmd::GPIO_FUZZ_WRITE:
            GpioWrite(GPIO_TEST_NUM, params->descVal);
            break;
        case ApiTestCmd::GPIO_FUZZ_SET_DIR:
            GpioSetDir(GPIO_TEST_NUM, params->descDir);
            break;
        case ApiTestCmd::GPIO_FUZZ_SET_IRQ:
            GpioSetIrq(GPIO_TEST_NUM, params->descMode, GpioTestIrqHandler, &data);
            break;
        default:
            break;
    }
    return true;
}
} // namespace OHOS

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    /* Run your code on data */
    if (data == nullptr) {
        HDF_LOGE("LLVMFuzzerTestOneInput: gpio fuzz test data is nullptr!");
        return 0;
    }

    if (size < sizeof(struct AllParameters)) {
        HDF_LOGE("LLVMFuzzerTestOneInput: gpio fuzz test size is small!");
        return 0;
    }

    OHOS::GpioFuzzTest(data, size);
    return 0;
}
