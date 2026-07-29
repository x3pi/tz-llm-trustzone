/*
 * Copyright (c) 2022-2023 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include <iostream>
#include "random.h"
#include "securec.h"
#include "hdf_base.h"
#include "hdf_log.h"
#include "spi_if.h"
#include "spi_fuzzer.h"

using namespace std;

namespace {
constexpr int32_t MIN = 0;
constexpr int32_t MAX = 2;
constexpr int32_t BUS_TEST_NUM = 0;
constexpr int32_t CS_TEST_NUM = 0;
constexpr uint32_t SPI_BUF_SIZE = 8;
}

struct AllParameters {
    uint32_t descSpeed;
    uint16_t descDelay;
    uint8_t descKeep;
    uint32_t desMaxSpeedHz;
    uint16_t desMode;
    uint8_t desTransferMode;
    uint8_t desBitsPerWord;
    uint8_t buf[SPI_BUF_SIZE];
};

namespace OHOS {
static void SpiFuzzDoTest(DevHandle handle, struct SpiDevInfo *info, struct SpiMsg *msg, struct SpiCfg *cfg)
{
    int32_t number;

    number = randNum(MIN, MAX);
    switch (static_cast<ApiNumber>(number)) {
        case ApiNumber::SPI_FUZZ_TRANSFER:
            SpiTransfer(handle, msg, SPI_BUF_SIZE);
            break;
        case ApiNumber::SPI_FUZZ_WRITE:
            SpiWrite(handle, msg->wbuf, SPI_BUF_SIZE);
            break;
        case ApiNumber::SPI_FUZZ_SETCFG:
            SpiSetCfg(handle, cfg);
            break;
        default:
            break;
    }
}

static bool SpiFuzzTest(const uint8_t *data, size_t size)
{
    struct SpiMsg msg;
    struct SpiCfg cfg;
    DevHandle handle = nullptr;
    struct SpiDevInfo info;
    const struct AllParameters *params = reinterpret_cast<const struct AllParameters *>(data);

    info.busNum = BUS_TEST_NUM;
    info.csNum = CS_TEST_NUM;
    msg.speed = params->descSpeed;
    msg.delayUs = params->descDelay;
    msg.keepCs = params->descKeep;
    msg.len = SPI_BUF_SIZE;
    msg.rbuf = reinterpret_cast<uint8_t *>(malloc(SPI_BUF_SIZE));
    if (msg.rbuf == nullptr) {
        HDF_LOGE("SpiFuzzTest: malloc rbuf fail!");
        return false;
    }
    msg.wbuf = reinterpret_cast<uint8_t *>(malloc(SPI_BUF_SIZE));
    if (msg.wbuf == nullptr) {
        HDF_LOGE("SpiFuzzTest: malloc wbuf fail!");
        free(msg.rbuf);
        return false;
    }
    if (memcpy_s(reinterpret_cast<void *>(msg.wbuf), SPI_BUF_SIZE, params->buf, SPI_BUF_SIZE) != EOK) {
        free(msg.rbuf);
        free(msg.wbuf);
        HDF_LOGE("SpiFuzzTest: memcpy buf fail!");
        return false;
    }
    cfg.maxSpeedHz = params->desMaxSpeedHz;
    cfg.mode = params->desMode;
    cfg.transferMode = params->desTransferMode;
    cfg.bitsPerWord = params->desBitsPerWord;
    handle = SpiOpen(&info);
    if (handle == nullptr) {
        HDF_LOGE("SpiFuzzTest: handle is nullptr!");
        return false;
    }
    SpiFuzzDoTest(handle, &info, &msg, &cfg);
    free(msg.rbuf);
    free(msg.wbuf);
    SpiClose(handle);
    return true;
}
} // namespace OHOS

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    /* Run your code on data */
    if (data == nullptr) {
        HDF_LOGE("LLVMFuzzerTestOneInput: spi fuzz test data is nullptr!");
        return 0;
    }

    if (size < sizeof(struct AllParameters)) {
        HDF_LOGE("LLVMFuzzerTestOneInput: spi fuzz test size is small!");
        return 0;
    }
    OHOS::SpiFuzzTest(data, size);
    return 0;
}
