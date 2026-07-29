/*
 * Copyright (c) 2020-2023 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include "uart_test.h"
#include "hdf_base.h"
#include "hdf_io_service_if.h"
#include "hdf_log.h"
#include "osal_mem.h"
#include "osal_time.h"
#include "securec.h"
#include "uart_if.h"

#define HDF_LOG_TAG uart_test

static int32_t UartTestGetConfig(struct UartTestConfig *config)
{
    int32_t ret;
    struct HdfSBuf *reply = NULL;
    struct HdfIoService *service = NULL;
    const void *buf = NULL;
    uint32_t len;

    service = HdfIoServiceBind("UART_TEST");
    if (service == NULL) {
        HDF_LOGE("UartTestGetConfig: fail to bind service!");
        return HDF_ERR_NOT_SUPPORT;
    }

    do {
        reply = HdfSbufObtainDefaultSize();
        if (reply == NULL) {
            HDF_LOGE("UartTestGetConfig: fail to obtain reply!");
            ret = HDF_ERR_MALLOC_FAIL;
            break;
        }

        ret = service->dispatcher->Dispatch(&service->object, 0, NULL, reply);
        if (ret != HDF_SUCCESS) {
            HDF_LOGE("UartTestGetConfig: remote dispatch fail, ret: %d!", ret);
            break;
        }

        if (!HdfSbufReadUint32(reply, &config->port)) {
            HDF_LOGE("UartTestGetConfig: read port fail!");
            ret = HDF_ERR_IO;
            break;
        }
        if (!HdfSbufReadUint32(reply, &config->len)) {
            HDF_LOGE("UartTestGetConfig: read len fail!");
            ret = HDF_ERR_IO;
            break;
        }

        if (!HdfSbufReadBuffer(reply, (const void **)&buf, &len)) {
            HDF_LOGE("UartTestGetConfig: read buf fail!");
            ret = HDF_ERR_IO;
            break;
        }

        if (len != config->len) {
            HDF_LOGE("UartTestGetConfig: config len:%u, read size:%u!", config->len, len);
            ret = HDF_ERR_IO;
            break;
        }
        config->wbuf = NULL;
        config->wbuf = (uint8_t *)OsalMemCalloc(len);
        if (config->wbuf == NULL) {
            HDF_LOGE("UartTestGetConfig: malloc wbuf fail!");
            ret = HDF_ERR_MALLOC_FAIL;
            break;
        }
        config->rbuf = NULL;
        config->rbuf = (uint8_t *)OsalMemCalloc(len);
        if (config->rbuf == NULL) {
            HDF_LOGE("UartTestGetConfig: malloc rbuf fail!");
            ret = HDF_ERR_MALLOC_FAIL;
            break;
        }

        if (memcpy_s(config->wbuf, config->len, buf, len) != EOK) {
            HDF_LOGE("UartTestGetConfig: Memcpy wbuf fail!");
            ret = HDF_ERR_IO;
            break;
        }

        HDF_LOGD("UartTestGetConfig: done!");
        ret = HDF_SUCCESS;
    } while (0);
    HdfSbufRecycle(reply);
    HdfIoServiceRecycle(service);
    return ret;
}

static inline void UartBufFree(struct UartTestConfig *config)
{
    OsalMemFree(config->wbuf);
    config->wbuf = NULL;
    OsalMemFree(config->rbuf);
    config->rbuf = NULL;
}

static struct UartTester *UartTesterGet(void)
{
    int32_t ret;
    static struct UartTester tester;

    ret = UartTestGetConfig(&tester.config);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("UartTesterGet: read config fail, ret: %d!", ret);
        UartBufFree(&tester.config);
        return NULL;
    }
    tester.handle = UartOpen(tester.config.port);
    if (tester.handle == NULL) {
        HDF_LOGE("UartTesterGet: open uart port:%u fail!", tester.config.port);
        UartBufFree(&tester.config);
        return NULL;
    }
    return &tester;
}

static void UartTesterPut(struct UartTester *tester)
{
    if (tester == NULL || tester->handle == NULL) {
        HDF_LOGE("UartTesterPut: tester or uart handle is null!");
        return;
    }
    UartBufFree(&tester->config);
    UartClose(tester->handle);
    tester->handle = NULL;
}

static int32_t UartWriteTest(struct UartTester *tester)
{
    int32_t ret;

    ret = UartWrite(tester->handle, tester->config.wbuf, tester->config.len);
    HDF_LOGD("UartWriteTest: len is %d", tester->config.len);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("UartWriteTest: write fail!");
        return HDF_FAILURE;
    }
    HDF_LOGD("UartWriteTest: success!");
    return HDF_SUCCESS;
}

static int32_t UartReadTest(struct UartTester *tester)
{
    int32_t ret;

    ret = UartSetTransMode(tester->handle, UART_MODE_RD_NONBLOCK);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("UartReadTest: transmode error, ret: %d!", ret);
        return ret;
    }
    ret = UartRead(tester->handle, tester->config.rbuf, tester->config.len);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("UartReadTest: read fail, ret: %d!", ret);
        return ret;
    }
    HDF_LOGD("UartReadTest: success!");
    return HDF_SUCCESS;
}

#define BAUD_921600 921600
static int32_t UartSetBaudTest(struct UartTester *tester)
{
    int32_t ret;

    ret = UartSetBaud(tester->handle, BAUD_921600);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("UartSetBaudTest: set baud fail, ret: %d!", ret);
        return ret;
    }
    HDF_LOGD("UartSetBaudTest: success!");
    return HDF_SUCCESS;
}

static int32_t UartGetBaudTest(struct UartTester *tester)
{
    int32_t ret;
    uint32_t baud;

    ret = UartGetBaud(tester->handle, &baud);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("UartGetBaudTest: get baud fail, ret: %d!", ret);
        return ret;
    }
    HDF_LOGD("UartGetBaudTest: baud %u success!", baud);
    return HDF_SUCCESS;
}

static int32_t UartSetAttributeTest(struct UartTester *tester)
{
    struct UartAttribute attribute;
    int32_t ret;

    attribute.dataBits = UART_ATTR_DATABIT_7;
    attribute.parity = UART_ATTR_PARITY_NONE;
    attribute.stopBits = UART_ATTR_STOPBIT_1;
    attribute.rts = UART_ATTR_RTS_DIS;
    attribute.cts = UART_ATTR_CTS_DIS;
    attribute.fifoRxEn = UART_ATTR_RX_FIFO_EN;
    attribute.fifoTxEn = UART_ATTR_TX_FIFO_EN;
    ret = UartSetAttribute(tester->handle, &attribute);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("UartSetAttributeTest: set attribute fail, ret: %d!", ret);
        return ret;
    }
    HDF_LOGD("UartSetAttributeTest: success!");
    return HDF_SUCCESS;
}

static int32_t UartGetAttributeTest(struct UartTester *tester)
{
    struct UartAttribute attribute;
    int32_t ret;

    ret = UartGetAttribute(tester->handle, &attribute);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("UartGetAttributeTest: get attribute fail, ret: %d!", ret);
        return ret;
    }
    HDF_LOGD("UartGetAttributeTest: dataBits %u", attribute.dataBits);
    HDF_LOGD("UartGetAttributeTest: parity %u", attribute.parity);
    HDF_LOGD("UartGetAttributeTest: stopBits %u", attribute.stopBits);
    HDF_LOGD("UartGetAttributeTest: rts %u", attribute.rts);
    HDF_LOGD("UartGetAttributeTest: cts %u", attribute.cts);
    HDF_LOGD("UartGetAttributeTest: fifoRxEn %u", attribute.fifoRxEn);
    HDF_LOGD("UartGetAttributeTest: fifoTxEn %u", attribute.fifoTxEn);
    HDF_LOGD("UartGetAttributeTest: success!");
    return HDF_SUCCESS;
}

static int32_t UartSetTransModeTest(struct UartTester *tester)
{
    int32_t ret;

    ret = UartSetTransMode(tester->handle, UART_MODE_RD_NONBLOCK);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("UartSetTransModeTest: set transmode fail, ret: %d!", ret);
        return ret;
    }
    HDF_LOGD("UartSetTransModeTest: success!");
    return HDF_SUCCESS;
}

static int32_t UartReliabilityTest(struct UartTester *tester)
{
    uint32_t baud;
    struct UartAttribute attribute = {0};

    (void)UartSetTransMode(tester->handle, UART_MODE_RD_NONBLOCK);
    (void)UartSetTransMode(tester->handle, -1);
    (void)UartWrite(tester->handle, tester->config.wbuf, tester->config.len);
    (void)UartWrite(tester->handle, NULL, -1);
    (void)UartRead(tester->handle, tester->config.rbuf, tester->config.len);
    (void)UartRead(tester->handle, NULL, -1);
    (void)UartSetBaud(tester->handle, BAUD_921600);
    (void)UartSetBaud(tester->handle, -1);
    (void)UartGetBaud(tester->handle, &baud);
    (void)UartGetBaud(tester->handle, NULL);
    (void)UartSetAttribute(tester->handle, &attribute);
    (void)UartSetAttribute(tester->handle, NULL);
    (void)UartGetAttribute(tester->handle, &attribute);
    (void)UartGetAttribute(tester->handle, NULL);
    HDF_LOGD("UartReliabilityTest: success!");
    return HDF_SUCCESS;
}

static int32_t UartIfPerformanceTest(struct UartTester *tester)
{
#ifdef __LITEOS__
    if (tester == NULL) {
        HDF_LOGE("UartIfPerformanceTest: tester is null!");
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
#endif
    uint32_t baudRate;
    uint64_t startMs;
    uint64_t endMs;
    uint64_t useTime;    // ms

    if (tester == NULL) {
        HDF_LOGE("UartIfPerformanceTest: tester is null!");
        return HDF_FAILURE;
    }
    startMs = OsalGetSysTimeMs();
    UartGetBaud(tester->handle, &baudRate);
    endMs = OsalGetSysTimeMs();

    useTime = endMs - startMs;
    HDF_LOGI("UartIfPerformanceTest: ----->interface performance test:[start - end] < 1ms[%s]\r\n",
        useTime < 1 ? "yes" : "no");
    return HDF_SUCCESS;
}

static int32_t UartMiniBlockWriteTest(struct UartTester *tester)
{
#ifdef __KERNEL__
    uint8_t data;
    int32_t ret;

    ret = UartBlockWrite(tester->handle, &data, sizeof(data));
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("UartMiniBlockWriteTest: uart block write fail, ret: %d!", ret);
        return ret;
    }
#else
    (void)tester;
#endif
    HDF_LOGI("UartMiniBlockWriteTest: all test done!");
    return HDF_SUCCESS;
}

struct UartTestEntry {
    int cmd;
    int32_t (*func)(struct UartTester *tester);
    const char *name;
};

static struct UartTestEntry g_entry[] = {
    { UART_TEST_CMD_WRITE, UartWriteTest, "UartWriteTest" },
    { UART_TEST_CMD_READ, UartReadTest, "UartReadTest" },
    { UART_TEST_CMD_SET_BAUD, UartSetBaudTest, "UartSetBaudTest" },
    { UART_TEST_CMD_GET_BAUD, UartGetBaudTest, "UartGetBaudTest" },
    { UART_TEST_CMD_SET_ATTRIBUTE, UartSetAttributeTest, "UartSetAttributeTest" },
    { UART_TEST_CMD_GET_ATTRIBUTE, UartGetAttributeTest, "UartGetAttributeTest" },
    { UART_TEST_CMD_SET_TRANSMODE, UartSetTransModeTest, "UartSetTransModeTest" },
    { UART_TEST_CMD_RELIABILITY, UartReliabilityTest, "UartReliabilityTest" },
    { UART_TEST_CMD_PERFORMANCE, UartIfPerformanceTest, "UartIfPerformanceTest" },
    { UART_MINI_BLOCK_WRITE_TEST, UartMiniBlockWriteTest, "UartMiniBlockWriteTest" },
};

int32_t UartTestExecute(int cmd)
{
    uint32_t i;
    int32_t ret = HDF_ERR_NOT_SUPPORT;
    struct UartTester *tester = NULL;

    tester = UartTesterGet();
    if (tester == NULL) {
        HDF_LOGE("UartTestExecute: tester is null!");
        return HDF_ERR_INVALID_OBJECT;
    }

    if (cmd > UART_TEST_CMD_MAX) {
        HDF_LOGE("UartTestExecute: invalid cmd:%d!", cmd);
        ret = HDF_ERR_NOT_SUPPORT;
        HDF_LOGE("[UartTestExecute][======cmd:%d====ret:%d======]", cmd, ret);
        UartTesterPut(tester);
        return ret;
    }

    for (i = 0; i < sizeof(g_entry) / sizeof(g_entry[0]); i++) {
        if (g_entry[i].cmd != cmd || g_entry[i].func == NULL) {
            continue;
        }
        ret = g_entry[i].func(tester);
        break;
    }

    HDF_LOGE("[UartTestExecute][======cmd:%d====ret:%d======]", cmd, ret);
    UartTesterPut(tester);
    return ret;
}
