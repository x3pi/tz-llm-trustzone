/*
 * Copyright (c) 2021-2023 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include "adc_test.h"
#include "adc_if.h"
#include "hdf_base.h"
#include "hdf_io_service_if.h"
#include "hdf_log.h"
#include "osal_mem.h"
#include "securec.h"
#include "osal_thread.h"
#include "osal_time.h"


#define HDF_LOG_TAG adc_test_c

#define TEST_ADC_VAL_NUM           50
#define ADC_TEST_WAIT_TIMES        100
#define ADC_TEST_WAIT_TIMEOUT      20
#define ADC_TEST_STACK_SIZE        (1024 * 64)

static int32_t AdcTestGetConfig(struct AdcTestConfig *config)
{
    int32_t ret;
    struct HdfSBuf *reply = NULL;
    struct HdfIoService *service = NULL;
    const void *buf = NULL;
    uint32_t len;

    HDF_LOGD("AdcTestGetConfig: enter!");
    service = HdfIoServiceBind("ADC_TEST");
    if (service == NULL) {
        HDF_LOGE("AdcTestGetConfig: service is null!");
        return HDF_ERR_NOT_SUPPORT;
    }

    do {
        reply = HdfSbufObtain(sizeof(*config) + sizeof(uint64_t));
        if (reply == NULL) {
            HDF_LOGE("AdcTestGetConfig: fail to obtain reply!");
            ret = HDF_ERR_MALLOC_FAIL;
            break;
        }

        ret = service->dispatcher->Dispatch(&service->object, 0, NULL, reply);
        if (ret != HDF_SUCCESS) {
            HDF_LOGE("AdcTestGetConfig: remote dispatch fail!");
            break;
        }

        if (!HdfSbufReadBuffer(reply, &buf, &len)) {
            HDF_LOGE("AdcTestGetConfig: read buf fail!");
            ret = HDF_ERR_IO;
            break;
        }

        if (len != sizeof(*config)) {
            HDF_LOGE("AdcTestGetConfig: config size:%zu, read size:%u!", sizeof(*config), len);
            ret = HDF_ERR_IO;
            break;
        }

        if (memcpy_s(config, sizeof(*config), buf, sizeof(*config)) != EOK) {
            HDF_LOGE("AdcTestGetConfig: memcpy buf fail!");
            ret = HDF_ERR_IO;
            break;
        }
        HDF_LOGD("AdcTestGetConfig: exit!");
        ret = HDF_SUCCESS;
    } while (0);
    HdfSbufRecycle(reply);
    HdfIoServiceRecycle(service);
    return ret;
}

static struct AdcTester *AdcTesterGet(void)
{
    int32_t ret;
    static struct AdcTester tester;
    static bool hasInit = false;

    HDF_LOGE("AdcTesterGet: enter!");
    if (hasInit) {
        return &tester;
    }
    ret = AdcTestGetConfig(&tester.config);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("AdcTesterGet: read config fail, ret: %d!", ret);
        return NULL;
    }
    tester.handle = AdcOpen(tester.config.devNum);
    if (tester.handle == NULL) {
        HDF_LOGE("AdcTesterGet: open adc device:%u fail!", tester.config.devNum);
        return NULL;
    }
    hasInit = true;
    HDF_LOGI("AdcTesterGet: done!");
    return &tester;
}

static int32_t AdcTestRead(void)
{
    struct AdcTester *tester = NULL;
    uint32_t value[TEST_ADC_VAL_NUM];
    int32_t ret;
    int i;

    HDF_LOGI("AdcTestRead: enter!");
    tester = AdcTesterGet();
    if (tester == NULL) {
        HDF_LOGE("AdcTestRead: get tester fail!");
        return HDF_ERR_INVALID_OBJECT;
    }
    for (i = 0; i < TEST_ADC_VAL_NUM; i++) {
        value[i] = 0;
        ret = AdcRead(tester->handle, tester->config.channel, &value[i]);
        if (ret != HDF_SUCCESS || value[i] >= (1U << tester->config.dataWidth)) {
            HDF_LOGE("AdcTestRead: read value fail, ret: %d!", ret);
            return HDF_ERR_IO;
        }
    }
    HDF_LOGI("AdcTestRead: adc device num is %u!", tester->config.devNum);

    HDF_LOGI("AdcTestRead: done!");
    return HDF_SUCCESS;
}

static int AdcTestThreadFunc(void *param)
{
    struct AdcTester *tester = NULL;
    uint32_t val;
    int i;
    int32_t ret;

    HDF_LOGI("AdcTestThreadFunc: enter!");
    tester = AdcTesterGet();
    if (tester == NULL) {
        HDF_LOGE("AdcTestThreadFunc: get tester fail!");
        *((int32_t *)param) = 1;
        return HDF_ERR_INVALID_OBJECT;
    }

    for (i = 0; i < ADC_TEST_WAIT_TIMES; i++) {
        ret = AdcRead(tester->handle, tester->config.channel, &val);
        if (ret != HDF_SUCCESS) {
            HDF_LOGE("AdcTestThreadFunc: adc read fail, ret: %d!", ret);
            *((int32_t *)param) = 1;
            return HDF_ERR_IO;
        }
    }

    *((int32_t *)param) = 1;
    HDF_LOGI("AdcTestThreadFunc: done!");
    return val;
}

static int32_t AdcTestStartThread(struct OsalThread *thread1, struct OsalThread *thread2,
    const int32_t *count1, const int32_t *count2)
{
    int32_t ret;
    uint32_t time = 0;
    struct OsalThreadParam cfg1;
    struct OsalThreadParam cfg2;

    if (memset_s(&cfg1, sizeof(cfg1), 0, sizeof(cfg1)) != EOK ||
        memset_s(&cfg2, sizeof(cfg2), 0, sizeof(cfg2)) != EOK) {
        HDF_LOGE("AdcTestStartThread: memset_s fail!");
        return HDF_ERR_IO;
    }

    cfg1.name = "AdcTestThread-1";
    cfg2.name = "AdcTestThread-2";
    cfg1.priority = cfg2.priority = OSAL_THREAD_PRI_DEFAULT;
    cfg1.stackSize = cfg2.stackSize = ADC_TEST_STACK_SIZE;

    ret = OsalThreadStart(thread1, &cfg1);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("AdcTestStartThread: start test thread1 fail, ret: %d!", ret);
        return ret;
    }

    ret = OsalThreadStart(thread2, &cfg2);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("AdcTestStartThread: start test thread2 fail, ret: %d!", ret);
    }

    while (*count1 == 0 || *count2 == 0) {
        HDF_LOGD("AdcTestStartThread: waitting testing thread finish...");
        OsalMSleep(ADC_TEST_WAIT_TIMES);
        time++;
        if (time > ADC_TEST_WAIT_TIMEOUT) {
            break;
        }
    }
    return ret;
}

static int32_t AdcTestMultiThread(void)
{
    int32_t ret;
    struct OsalThread thread1;
    struct OsalThread thread2;
    int32_t count1 = 0;
    int32_t count2 = 0;

    ret = OsalThreadCreate(&thread1, (OsalThreadEntry)AdcTestThreadFunc, (void *)&count1);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("AdcTestMultiThread: create test thread1 fail, ret: %d!", ret);
        return ret;
    }

    ret = OsalThreadCreate(&thread2, (OsalThreadEntry)AdcTestThreadFunc, (void *)&count2);
    if (ret != HDF_SUCCESS) {
        (void)OsalThreadDestroy(&thread1);
        HDF_LOGE("AdcTestMultiThread: create test thread2 fail, ret: %d!", ret);
        return ret;
    }

    ret = AdcTestStartThread(&thread1, &thread2, &count1, &count2);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("AdcTestMultiThread: test start thread fail, ret: %d!", ret);
    }

    (void)OsalThreadDestroy(&thread1);
    (void)OsalThreadDestroy(&thread2);
    return ret;
}

static int32_t AdcTestReliability(void)
{
    struct AdcTester *tester = NULL;
    uint32_t val;

    HDF_LOGI("AdcTestReliability: enter!");
    tester = AdcTesterGet();
    if (tester == NULL || tester->handle == NULL) {
        return HDF_ERR_INVALID_OBJECT;
    }
    // invalid handle
    (void)AdcRead(NULL, tester->config.channel, &val);
    // invalid channel
    (void)AdcRead(tester->handle, tester->config.maxChannel + 1, &val);
    // invalid val pointer
    (void)AdcRead(tester->handle, tester->config.channel, NULL);
    HDF_LOGI("AdcTestReliability: done!");
    return HDF_SUCCESS;
}

static int32_t AdcIfPerformanceTest(void)
{
#ifdef __LITEOS__
    // liteos the accuracy of the obtained time is too large and inaccurate.
    return HDF_SUCCESS;
#endif
    struct AdcTester *tester = NULL;
    uint64_t startMs;
    uint64_t endMs;
    uint64_t useTime;    // ms
    uint32_t val;
    int32_t ret;

    tester = AdcTesterGet();
    if (tester == NULL || tester->handle == NULL) {
        HDF_LOGE("AdcIfPerformanceTest: get tester fail!");
        return HDF_ERR_INVALID_OBJECT;
    }

    startMs = OsalGetSysTimeMs();
    ret = AdcRead(tester->handle, tester->config.channel, &val);
    if (ret == HDF_SUCCESS) {
        endMs = OsalGetSysTimeMs();
        useTime = endMs - startMs;
        HDF_LOGI("AdcIfPerformanceTest: ----->interface performance test:[start - end] < 1ms[%{pubilc}s]\r\n",
            useTime < 1 ? "yes" : "no");
        return HDF_SUCCESS;
    }
    return HDF_FAILURE;
}

struct AdcTestEntry {
    int cmd;
    int32_t (*func)(void);
    const char *name;
};

static struct AdcTestEntry g_entry[] = {
    { ADC_TEST_CMD_READ, AdcTestRead, "AdcTestRead" },
    { ADC_TEST_CMD_MULTI_THREAD, AdcTestMultiThread, "AdcTestMultiThread" },
    { ADC_TEST_CMD_RELIABILITY, AdcTestReliability, "AdcTestReliability" },
    { ADC_IF_PERFORMANCE_TEST, AdcIfPerformanceTest, "AdcIfPerformanceTest" },
};

int32_t AdcTestExecute(int cmd)
{
    uint32_t i;
    int32_t ret = HDF_ERR_NOT_SUPPORT;

    if (cmd > ADC_TEST_CMD_MAX) {
        HDF_LOGE("AdcTestExecute: invalid cmd:%d!", cmd);
        ret = HDF_ERR_NOT_SUPPORT;
        HDF_LOGE("[AdcTestExecute][======cmd:%d====ret:%d======]", cmd, ret);
        return ret;
    }

    for (i = 0; i < sizeof(g_entry) / sizeof(g_entry[0]); i++) {
        if (g_entry[i].cmd != cmd || g_entry[i].func == NULL) {
            continue;
        }
        ret = g_entry[i].func();
        break;
    }

    HDF_LOGE("[AdcTestExecute][======cmd:%d====ret:%d======]", cmd, ret);
    return ret;
}
