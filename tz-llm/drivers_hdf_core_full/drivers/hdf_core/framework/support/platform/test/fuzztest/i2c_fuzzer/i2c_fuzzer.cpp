/*
 * Copyright (c) 2022-2023 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include "i2c_fuzzer.h"

using namespace std;

namespace {
constexpr int32_t BUS_NUM = 6;
constexpr uint16_t BUF_LEN = 7;
}

struct AllParameters {
    uint16_t addr;
    uint16_t flags;
    uint8_t buf[BUF_LEN];
};

namespace OHOS {
static bool I2cFuzzTest(const uint8_t *data, size_t size)
{
    DevHandle handle = nullptr;
    const struct AllParameters *params = reinterpret_cast<const struct AllParameters *>(data);
    struct I2cMsg msg;

    handle = I2cOpen(BUS_NUM);
    if (handle == nullptr) {
        HDF_LOGE("I2cFuzzTest: handle is nullptr!");
        return false;
    }
    msg.addr = params->addr;
    msg.flags = params->flags;
    msg.len = BUF_LEN;
    msg.buf = nullptr;
    msg.buf = reinterpret_cast<uint8_t *>(malloc(BUF_LEN));
    if (msg.buf == nullptr) {
        HDF_LOGE("I2cFuzzTest: malloc buf fail!");
        return false;
    }
    if (memcpy_s(msg.buf, BUF_LEN, params->buf, BUF_LEN) != EOK) {
        HDF_LOGE("I2cFuzzTest :memcpy buf fail!");
        free(msg.buf);
        return false;
    }
    I2cTransfer(handle, &msg, 1);
    free(msg.buf);
    I2cClose(handle);

    return true;
}
} // namespace OHOS

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    /* Run your code on data */
    if (data == nullptr) {
        HDF_LOGE("LLVMFuzzerTestOneInput: i2c fuzz test data is nullptr!");
        return 0;
    }

    if (size < sizeof(struct AllParameters)) {
        HDF_LOGE("LLVMFuzzerTestOneInput: i2c fuzz test size is small!");
        return 0;
    }
    OHOS::I2cFuzzTest(data, size);
    return 0;
}
