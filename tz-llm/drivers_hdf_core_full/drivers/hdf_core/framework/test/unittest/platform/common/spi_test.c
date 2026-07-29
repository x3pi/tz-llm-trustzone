/*
 * Copyright (c) 2020-2023 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include "spi_test.h"
#include "securec.h"
#include "device_resource_if.h"
#include "hdf_io_service_if.h"
#include "hdf_base.h"
#include "hdf_log.h"
#include "osal_mem.h"
#include "osal_time.h"
#include "spi_if.h"

#define HDF_LOG_TAG spi_test_c

#define SPI_TEST_4BITS  4
#define SPI_TEST_8BITS  8
#define SPI_TEST_16BITS 16

static int32_t SpiTestGetConfig(struct SpiTestConfig *config)
{
    int32_t ret;
    struct HdfSBuf *reply = NULL;
    struct HdfIoService *service = NULL;
    struct SpiTestConfig *cfg = NULL;
    const void *buf = NULL;
    uint32_t len;

    service = HdfIoServiceBind("SPI_TEST");
    if (service == NULL || service->dispatcher == NULL || service->dispatcher->Dispatch == NULL) {
        HDF_LOGE("SpiTestGetConfig: fail to bind service!");
        return HDF_ERR_NOT_SUPPORT;
    }

    do {
        reply = HdfSbufObtainDefaultSize();
        if (reply == NULL) {
            HDF_LOGE("SpiTestGetConfig: fail to obtain reply!");
            ret = HDF_ERR_MALLOC_FAIL;
            break;
        }

        ret = service->dispatcher->Dispatch(&service->object, 0, NULL, reply);
        if (ret != HDF_SUCCESS) {
            HDF_LOGE("SpiTestGetConfig: remote dispatch fail, ret: %d", ret);
            break;
        }

        if (!HdfSbufReadBuffer(reply, (const void **)&cfg, &len)) {
            HDF_LOGE("SpiTestGetConfig: read buf fail!");
            ret = HDF_ERR_IO;
            break;
        }

        if (len != sizeof(*cfg)) {
            HDF_LOGE("SpiTestGetConfig: cfg size:%zu, read size:%u!", sizeof(*cfg), len);
            ret = HDF_ERR_IO;
            break;
        }

        if (memcpy_s(config, sizeof(*config), cfg, sizeof(*cfg)) != EOK) {
            HDF_LOGE("SpiTestGetConfig: memcpy config fail!");
            ret = HDF_ERR_IO;
            break;
        }

        if (!HdfSbufReadBuffer(reply, (const void **)&buf, &len)) {
            HDF_LOGE("SpiTestGetConfig: read buf fail!");
            ret = HDF_ERR_IO;
            break;
        }

        if (len != config->len) {
            HDF_LOGE("SpiTestGetConfig: buffer size:%zu, read size:%u!", config->len, len);
            ret = HDF_ERR_IO;
            break;
        }

        config->wbuf = NULL;
        config->wbuf = (uint8_t *)OsalMemCalloc(config->len);
        if (config->wbuf == NULL) {
            HDF_LOGE("SpiTestGetConfig: malloc wbuf fail!");
            ret = HDF_ERR_MALLOC_FAIL;
            break;
        }

        if (memcpy_s(config->wbuf, config->len, buf, len) != EOK) {
            HDF_LOGE("SpiTestGetConfig: memcpy config fail!");
            ret = HDF_ERR_IO;
            break;
        }
        ret = HDF_SUCCESS;
    } while (0);
    HdfSbufRecycle(reply);
    HdfIoServiceRecycle(service);
    return ret;
}

static struct SpiTester *SpiTesterGet(void)
{
    int32_t ret;
    struct SpiDevInfo info;
    static struct SpiTester tester;

    HDF_LOGE("SpiTesterGet: enter");
    ret = SpiTestGetConfig(&tester.config);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("SpiTesterGet: read config fail, ret: %d!", ret);
        return NULL;
    }

    tester.config.rbuf = (uint8_t *)OsalMemCalloc(tester.config.len);
    if (tester.config.rbuf == NULL) {
        HDF_LOGE("SpiTesterGet: malloc rbuf fail!");
        return NULL;
    }

    info.busNum = tester.config.bus;
    info.csNum = tester.config.cs;

    tester.handle = SpiOpen(&info);
    if (tester.handle == NULL) {
        HDF_LOGE("SpiTesterGet: open spi: %u, cs: %u fail!", tester.config.bus, tester.config.cs);
        return NULL;
    }

    return &tester;
}

static void SpiTesterPut(struct SpiTester *tester)
{
    if (tester == NULL || tester->handle == NULL) {
        HDF_LOGE("SpiTesterPut: tester or handle is null!");
        return;
    }
    SpiClose(tester->handle);
    if (tester->config.rbuf != NULL) {
        OsalMemFree(tester->config.rbuf);
        tester->config.rbuf = NULL;
    }
    if (tester->config.wbuf != NULL) {
        OsalMemFree(tester->config.wbuf);
        tester->config.wbuf = NULL;
    }
    tester->handle = NULL;
}

#define BITS_PER_WORD_DEFAULT    8
#define BITS_PER_WORD_8BITS      8
#define BITS_PER_WORD_10BITS     10
#define MAX_SPEED_HZ             10000000

static struct SpiCfg g_spiCfg = {
    .mode = SPI_CLK_PHASE | SPI_MODE_LOOP,
    .bitsPerWord = BITS_PER_WORD_DEFAULT,
    .maxSpeedHz = MAX_SPEED_HZ,
    .transferMode = SPI_POLLING_TRANSFER,
};

#define SPI_TEST_ONE_BYTE        1
#define SPI_TEST_TWO_BYTE        2

static int32_t SpiCmpMemByBits(uint8_t *wbuf, uint8_t *rbuf, uint32_t len, uint8_t bits)
{
    uint32_t i;
    uint16_t vw;
    uint16_t vr;

    if (bits < SPI_TEST_4BITS) {
        bits = SPI_TEST_4BITS;
    } else if (bits > SPI_TEST_16BITS) {
        bits = SPI_TEST_16BITS;
    }

    for (i = 0; i < len;) {
        if (bits <= SPI_TEST_8BITS) {
            vw = *((uint8_t *)(wbuf + i)) & (~(0xFFFF << bits));
            vr = *((uint8_t *)(rbuf + i)) & (~(0xFFFF << bits));
        } else {
            vw = *((uint16_t *)(wbuf + i)) & (~(0xFFFF << bits));
            vr = *((uint16_t *)(rbuf + i)) & (~(0xFFFF << bits));
        }
        if (vw != vr) {
            HDF_LOGE("SpiCmpMemByBits: compare mem fail(i=%u, vw=%hu, vr=%hu, bits = %hhu, len=%u)",
                i, vw, vr, bits, len);
            return HDF_FAILURE;
        }
        i += (bits <= SPI_TEST_8BITS) ? SPI_TEST_ONE_BYTE : SPI_TEST_TWO_BYTE;
    }
    HDF_LOGD("SpiCmpMemByBits: mem size(%u) compare success!", len);
    return HDF_SUCCESS;
}

static int32_t SpiDoTransferTest(struct SpiTester *tester, struct SpiCfg *cfg, struct SpiMsg *msg)
{
    int32_t ret;

    ret = SpiSetCfg(tester->handle, cfg);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("SpiDoTransferTest: set config fail, ret: %d", ret);
        return ret;
    }

    ret = SpiTransfer(tester->handle, msg, 1);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("SpiDoTransferTest: spi transfer err, ret: %d", ret);
        return ret;
    }

    ret = SpiCmpMemByBits(msg->wbuf, msg->rbuf, msg->len, g_spiCfg.bitsPerWord);
    if (ret != HDF_SUCCESS) {
        return ret;
    }

    ret = SpiWrite(tester->handle, msg->wbuf, msg->len);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("SpiDoTransferTest: spi write err, ret: %d", ret);
        return ret;
    }

    ret = SpiRead(tester->handle, msg->rbuf, msg->len);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("SpiDoTransferTest: spi read err, ret: %d", ret);
        return ret;
    }

    HDF_LOGD("SpiDoTransferTest: success!");
    return HDF_SUCCESS;
}

static int32_t SpiTransferTest(struct SpiTester *tester)
{
    int32_t ret;
    struct SpiMsg msg;

    g_spiCfg.bitsPerWord = BITS_PER_WORD_8BITS;
    g_spiCfg.transferMode = SPI_POLLING_TRANSFER;

    msg.rbuf = tester->config.rbuf;
    msg.wbuf = tester->config.wbuf;
    msg.len = tester->config.len;
    msg.keepCs = 0; // switch off the CS after transfer
    msg.delayUs = 0;
    msg.speed = 0;    // use default speed

    ret = SpiDoTransferTest(tester, &g_spiCfg, &msg);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("SpiTransferTest: fail, bitsPerWord = %u, ret = %d!", g_spiCfg.bitsPerWord, ret);
        return ret;
    }

    g_spiCfg.bitsPerWord = BITS_PER_WORD_10BITS;
    ret = SpiDoTransferTest(tester, &g_spiCfg, &msg);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("SpiTransferTest: fail, bitsPerWord = %u, ret = %d!", g_spiCfg.bitsPerWord, ret);
        return ret;
    }

    return HDF_SUCCESS;
}

#define SPI_TEST_MSG_NUM         3
#define SPI_TEST_MSG_0           0
#define SPI_TEST_MSG_1           1
#define SPI_TEST_MSG_2           2

static int32_t SpiMultiTransferTest(struct SpiTester *tester)
{
    int32_t ret;
    struct SpiMsg msgs[SPI_TEST_MSG_NUM];

    g_spiCfg.bitsPerWord = BITS_PER_WORD_8BITS;
    g_spiCfg.transferMode = SPI_POLLING_TRANSFER;

    msgs[SPI_TEST_MSG_0].rbuf = tester->config.rbuf;
    msgs[SPI_TEST_MSG_0].wbuf = tester->config.wbuf;
    msgs[SPI_TEST_MSG_0].len = tester->config.len;
    msgs[SPI_TEST_MSG_0].delayUs = 0;
    msgs[SPI_TEST_MSG_0].speed = 0;    // use default speed

    msgs[SPI_TEST_MSG_1].wbuf = tester->config.wbuf;
    msgs[SPI_TEST_MSG_1].rbuf = NULL;
    msgs[SPI_TEST_MSG_1].len = tester->config.len;
    msgs[SPI_TEST_MSG_1].speed = 0;

    msgs[SPI_TEST_MSG_2].wbuf = NULL;
    msgs[SPI_TEST_MSG_2].rbuf = tester->config.rbuf;
    msgs[SPI_TEST_MSG_2].len = tester->config.len;
    msgs[SPI_TEST_MSG_2].speed = 0;
    msgs[SPI_TEST_MSG_2].keepCs = 0; // switch off the CS after transfer

    ret = SpiSetCfg(tester->handle, &g_spiCfg);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("SpiMultiTransferTest: set config fail, ret: %d", ret);
        return ret;
    }

    ret = SpiTransfer(tester->handle, (struct SpiMsg *)&msgs, SPI_TEST_MSG_NUM);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("SpiMultiTransferTest: multi transfer test fail, ret: %d", ret);
        return ret;
    }

    return HDF_SUCCESS;
}

#define DMA_TRANSFER_SINGLE_MAX   (1024 * 64 - 1)
#define DMA_TRANSFER_SIZE_TOTAL   (DMA_TRANSFER_SINGLE_MAX * 2 + 65532)
#define DMA_TRANSFER_BUF_SEED     0x5A
#define DMA_ALIGN_SIZE            64
#ifdef CONFIG_IMX8MM_SPI_TEST
#define DMA_TRANSFER_SIZE_TOTAL_NXP (60)
#endif  // CONFIG_IMX8MM_SPI_TEST
static int32_t SpiSetDmaIntMsg(struct SpiMsg *msg, uint32_t len)
{
    uint32_t i;
    uint8_t *wbuf = NULL;
    uint8_t *rbuf = NULL;

    msg->wbuf = msg->rbuf = NULL;

    wbuf = (uint8_t *)OsalMemAllocAlign(DMA_ALIGN_SIZE, len);
    if (wbuf == NULL) {
        return HDF_ERR_MALLOC_FAIL;
    }
    rbuf = (uint8_t *)OsalMemAllocAlign(DMA_ALIGN_SIZE, len);
    if (rbuf == NULL) {
        OsalMemFree(wbuf);
        return HDF_ERR_MALLOC_FAIL;
    }

    wbuf[0] = DMA_TRANSFER_BUF_SEED;
    for (i = 1; i < len; i++) {
        wbuf[i] = wbuf[i - 1] + 1;
        rbuf[i] = 0;
    }
    msg->wbuf = wbuf;
    msg->rbuf = rbuf;
    msg->len = len;
    msg->keepCs = 0,  // switch off the CS after transfer
    msg->delayUs = 0;
    msg->speed = 0;    // using default speed
    return HDF_SUCCESS;
}

static void SpiUnsetDmaIntMsg(struct SpiMsg *msg)
{
    if (msg != NULL) {
        OsalMemFree(msg->wbuf);
        OsalMemFree(msg->rbuf);
        msg->wbuf = NULL;
        msg->rbuf = NULL;
    }
}

static int32_t SpiDmaTransferTest(struct SpiTester *tester)
{
    int32_t ret;
    struct SpiMsg msg;

    g_spiCfg.transferMode = SPI_DMA_TRANSFER;
    g_spiCfg.bitsPerWord = BITS_PER_WORD_8BITS;

    if (tester->config.testDma == 0) {
        HDF_LOGI("SpiDmaTransferTest: testDma not set!");
        return HDF_SUCCESS;
    }

    ret = SpiSetDmaIntMsg(&msg, DMA_TRANSFER_SIZE_TOTAL);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("SpiDmaTransferTest: set dma int msg fail, ret: %d", ret);
        return ret;
    }

    ret = SpiDoTransferTest(tester, &g_spiCfg, &msg);
    if (ret != HDF_SUCCESS) {
        SpiUnsetDmaIntMsg(&msg);
        HDF_LOGE("SpiDmaTransferTest: fail, bitsPerWord = %u, ret = %d!", g_spiCfg.bitsPerWord, ret);
        return ret;
    }

    g_spiCfg.bitsPerWord = BITS_PER_WORD_10BITS;
    ret = SpiDoTransferTest(tester, &g_spiCfg, &msg);
    if (ret != HDF_SUCCESS) {
        SpiUnsetDmaIntMsg(&msg);
        HDF_LOGE("SpiDmaTransferTest: fail, bitsPerWord = %u, ret = %d!", g_spiCfg.bitsPerWord, ret);
        return ret;
    }

    SpiUnsetDmaIntMsg(&msg);
    return HDF_SUCCESS;
}

static int32_t SpiIntTransferTest(struct SpiTester *tester)
{
    int32_t ret;
    struct SpiMsg msg;

    g_spiCfg.transferMode = SPI_INTERRUPT_TRANSFER;
    g_spiCfg.bitsPerWord = BITS_PER_WORD_8BITS;
#ifdef CONFIG_IMX8MM_SPI_TEST
    ret = SpiSetDmaIntMsg(&msg, DMA_TRANSFER_SIZE_TOTAL_NXP);
#else  // CONFIG_IMX8MM_SPI_TEST
    ret = SpiSetDmaIntMsg(&msg, DMA_TRANSFER_SIZE_TOTAL);
#endif  // CONFIG_IMX8MM_SPI_TEST
    if (ret != HDF_SUCCESS) {
        return ret;
    }

    ret = SpiDoTransferTest(tester, &g_spiCfg, &msg);
    if (ret != HDF_SUCCESS) {
        SpiUnsetDmaIntMsg(&msg);
        HDF_LOGE("SpiIntTransferTest: fail, bitsPerWord = %u, ret = %d!", g_spiCfg.bitsPerWord, ret);
        return ret;
    }

    g_spiCfg.bitsPerWord = BITS_PER_WORD_10BITS;
    ret = SpiDoTransferTest(tester, &g_spiCfg, &msg);
    if (ret != HDF_SUCCESS) {
        SpiUnsetDmaIntMsg(&msg);
        HDF_LOGE("SpiIntTransferTest: fail, bitsPerWord = %u, ret = %d!", g_spiCfg.bitsPerWord, ret);
        return ret;
    }

    SpiUnsetDmaIntMsg(&msg);
    return HDF_SUCCESS;
}

static int32_t SpiReliabilityTest(struct SpiTester *tester)
{
    struct SpiCfg cfg = {0};
    struct SpiMsg msg = {0};

    (void)SpiSetCfg(tester->handle, &cfg);
    (void)SpiSetCfg(tester->handle, NULL);
    (void)SpiTransfer(tester->handle, &msg, 1);
    (void)SpiTransfer(tester->handle, NULL, -1);
    (void)SpiWrite(tester->handle, tester->config.wbuf, tester->config.len);
    (void)SpiWrite(tester->handle, NULL, -1);
    (void)SpiRead(tester->handle, tester->config.rbuf, tester->config.len);
    (void)SpiRead(tester->handle, NULL, -1);

    (void)tester;
    HDF_LOGE("SpiReliabilityTest: success!");
    return HDF_SUCCESS;
}

static int32_t SpiTestAll(struct SpiTester *tester)
{
    int32_t total = 0;
    int32_t error = 0;

    if (SpiTransferTest(tester) != HDF_SUCCESS) {
        error++;
    }
    total++;

    if (SpiDmaTransferTest(tester) != HDF_SUCCESS) {
        error++;
    }
    total++;

    if (SpiIntTransferTest(tester) != HDF_SUCCESS) {
        error++;
    }
    total++;

    if (SpiReliabilityTest(tester) != HDF_SUCCESS) {
        error++;
    }
    total++;

    HDF_LOGE("SpiTestAll: spi tester total %d error %d", total, error);
    return HDF_SUCCESS;
}

static int32_t SpiIfPerformanceTest(struct SpiTester *tester)
{
#ifdef __LITEOS__
    // liteos the accuracy of the obtained time is too large and inaccurate.
    if (tester == NULL || tester->handle == NULL) {
        HDF_LOGE("SpiIfPerformanceTest: tester or handle is null!");
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
#endif
    int32_t ret;
    struct SpiCfg cfg = {0};
    uint64_t startMs;
    uint64_t endMs;
    uint64_t useTime;    // ms

    startMs = OsalGetSysTimeMs();
    ret = SpiGetCfg(tester->handle, &cfg);
    endMs = OsalGetSysTimeMs();

    if (ret == HDF_SUCCESS) {
        useTime = endMs - startMs;
        HDF_LOGI("SpiIfPerformanceTest: ----->interface performance test:[start - end] < 1ms[%s]\r\n",
            useTime < 1 ? "yes" : "no");
        return HDF_SUCCESS;
    }
    return HDF_FAILURE;
}

struct SpiTestFunc {
    int cmd;
    int32_t (*func)(struct SpiTester *tester);
    const char *name;
};

static struct SpiTestFunc g_spiTestEntry[] = {
    {SPI_TRANSFER_TEST, SpiTransferTest, "SpiTransferTest"},
    {SPI_MULTI_TRANSFER_TEST, SpiMultiTransferTest, "SpiMultiTransferTest"},
    {SPI_DMA_TRANSFER_TEST, SpiDmaTransferTest, "SpiDmaTransferTest"},
    {SPI_INT_TRANSFER_TEST, SpiIntTransferTest, "SpiIntTransferTest"},
    {SPI_RELIABILITY_TEST, SpiReliabilityTest, "SpiReliabilityTest"},
    {SPI_PERFORMANCE_TEST, SpiIfPerformanceTest, "SpiIfPerformanceTest"},
    {SPI_TEST_ALL, SpiTestAll, "SpiTestAll"},
};

int32_t SpiTestExecute(int cmd)
{
    uint32_t i;
    int32_t ret = HDF_ERR_NOT_SUPPORT;
    struct SpiTester *tester = NULL;

    HDF_LOGE("SpiTestExecute: enter cmd %d!", cmd);
    if (cmd > SPI_TEST_CMD_MAX) {
        HDF_LOGE("SpiTestExecute: invalid cmd: %d!", cmd);
        ret = HDF_ERR_INVALID_OBJECT;
        goto EXIT;
    }

    tester = SpiTesterGet();
    if (tester == NULL || tester->handle == NULL) {
        HDF_LOGE("SpiTestExecute: get tester fail!");
        ret =  HDF_ERR_NOT_SUPPORT;
        goto EXIT;
    }

    for (i = 0; i < sizeof(g_spiTestEntry) / sizeof(g_spiTestEntry[0]); i++) {
        if (g_spiTestEntry[i].cmd != cmd || g_spiTestEntry[i].func == NULL) {
            continue;
        }
        ret = g_spiTestEntry[i].func(tester);
        break;
    }
EXIT:
    HDF_LOGE("[SpiTestExecute][======cmd:%d====ret:%d======]", cmd, ret);
    SpiTesterPut(tester);
    return ret;
}
