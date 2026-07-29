/*
 * Copyright (c) 2022-2023 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include "timer_test.h"
#include "device_resource_if.h"
#include "hdf_io_service_if.h"
#include "hdf_log.h"
#include "osal_mem.h"
#include "osal_thread.h"
#include "osal_time.h"
#include "securec.h"
#include "timer_if.h"

#define HDF_LOG_TAG timer_test_c

static bool g_theard1Flag = false;
static bool g_theard2Flag = false;

struct TimerTestFunc {
    int type;
    int32_t (*Func)(const struct TimerTest *test);
};

static int32_t TimerTestcaseCb(uint32_t number)
{
    static uint16_t num = 0;
    num++;
    if (num >= TIMER_TEST_PERIOD_TIMES) {
        HDF_LOGD("->>>>>>>>>>>TimerTestcaseCb: num exceed max %d", number);
        g_theard2Flag = true;
        num = 0;
    }
    return HDF_SUCCESS;
}

static int32_t TimerTestcaseOnceCb(uint32_t number)
{
    HDF_LOGD("->>>>>>>>>>>TimerTestcaseOnceCb: %d", number);
    g_theard1Flag = true;
    return HDF_SUCCESS;
}

static int32_t TimerSetTest(const struct TimerTest *test)
{
    if (test == NULL || test->handle == NULL) {
        HDF_LOGE("TimerSetTest: test or handle is null!");
        return HDF_ERR_INVALID_OBJECT;
    }

    HwTimerSet(test->handle, test->uSecond, TimerTestcaseCb);
    return HDF_SUCCESS;
}

static int32_t TimerSetOnceTest(const struct TimerTest *test)
{
    if (test == NULL || test->handle == NULL) {
        HDF_LOGE("TimerSetOnceTest: test or handle is null!");
        return HDF_ERR_INVALID_OBJECT;
    }

    HwTimerSetOnce(test->handle, test->uSecond, TimerTestcaseOnceCb);
    return HDF_SUCCESS;
}

static int32_t TimerGetTest(const struct TimerTest *test)
{
    if (test == NULL || test->handle == NULL) {
        HDF_LOGE("TimerGetTest: test or handle is null!");
        return HDF_ERR_INVALID_OBJECT;
    }

    uint32_t uSecond;
    bool isPeriod;

    if (HwTimerGet(test->handle, &uSecond, &isPeriod) != HDF_SUCCESS) {
        HDF_LOGE("TimerGetTest: TimerGet fail!");
        return HDF_FAILURE;
    }

    HDF_LOGD("TimerGetTest: [%u][%d]", uSecond, isPeriod);
    return HDF_SUCCESS;
}

static int32_t TimerStartTest(const struct TimerTest *test)
{
    if (test == NULL || test->handle == NULL) {
        HDF_LOGE("TimerStartTest: test or handle is null!");
        return HDF_ERR_INVALID_OBJECT;
    }

    HwTimerStart(test->handle);
    return HDF_SUCCESS;
}

static int32_t TimerStopTest(const struct TimerTest *test)
{
    if (test == NULL || test->handle == NULL) {
        HDF_LOGE("TimerStopTest: test or handle is null!");
        return HDF_ERR_INVALID_OBJECT;
    }

    HwTimerStop(test->handle);
    return HDF_SUCCESS;
}

static int TimerOnceTestThreadFunc(void *param)
{
    DevHandle handle = (DevHandle)param;

    if (handle == NULL) {
        HDF_LOGE("TimerOnceTestThreadFunc: handle is null!");
        g_theard1Flag = true;
        return HDF_FAILURE;
    }

    if (HwTimerSetOnce(handle, TIMER_TEST_TIME_USECONDS, TimerTestcaseOnceCb) != HDF_SUCCESS) {
        HDF_LOGE("TimerOnceTestThreadFunc: TimerSetOnce fail!");
        g_theard1Flag = true;
        return HDF_FAILURE;
    }
    if (HwTimerStart(handle) != HDF_SUCCESS) {
        HDF_LOGE("TimerOnceTestThreadFunc: HwTimerStart fail!");
        g_theard1Flag = true;
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

static int TimerPeriodTestThreadFunc(void *param)
{
    DevHandle handle = (DevHandle)param;

    if (handle == NULL) {
        HDF_LOGE("TimerPeriodTestThreadFunc: timer test get handle fail!");
        g_theard2Flag = true;
        return HDF_FAILURE;
    }

    if (HwTimerSet(handle, TIMER_TEST_TIME_USECONDS, TimerTestcaseCb) != HDF_SUCCESS) {
        HDF_LOGE("TimerPeriodTestThreadFunc: TimerSet fail!");
        g_theard2Flag = true;
        return HDF_FAILURE;
    }
    if (HwTimerStart(handle) != HDF_SUCCESS) {
        HDF_LOGE("TimerPeriodTestThreadFunc: HwTimerStart fail!");
        g_theard2Flag = true;
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

static int32_t TimerTestStartThread(struct OsalThread *thread1, struct OsalThread *thread2)
{
    int32_t ret;
    uint32_t time = 0;
    struct OsalThreadParam cfg1;
    struct OsalThreadParam cfg2;

    if (memset_s(&cfg1, sizeof(cfg1), 0, sizeof(cfg1)) != EOK ||
        memset_s(&cfg2, sizeof(cfg2), 0, sizeof(cfg2)) != EOK) {
        HDF_LOGE("TimerTestStartThread: memset_s fail!");
        return HDF_ERR_IO;
    }

    cfg1.name = "TimerTestThread-once";
    cfg2.name = "TimerTestThread-period";
    cfg1.priority = cfg2.priority = OSAL_THREAD_PRI_DEFAULT;
    cfg1.stackSize = cfg2.stackSize = TIMER_TEST_STACK_SIZE;

    ret = OsalThreadStart(thread1, &cfg1);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("TimerTestStartThread: testing start timer thread1 fail, ret: %d!", ret);
        return ret;
    }

    ret = OsalThreadStart(thread2, &cfg2);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("TimerTestStartThread: testing start timer thread2 fail, ret: %d!", ret);
    }

    while (g_theard1Flag == false || g_theard2Flag == false) {
        HDF_LOGD("TimerTestStartThread: [%d]waitting testing timer thread finish...", time);
        OsalSleep(TIMER_TEST_WAIT_TIMEOUT);
        time++;
        if (time > TIMER_TEST_WAIT_TIMES) {
            break;
        }
    }
    return ret;
}

static int32_t TimerTestCreateThread(struct OsalThread *thread1, struct OsalThread *thread2,
    DevHandle handle1, DevHandle handle2)
{
    int32_t ret;

    ret = OsalThreadCreate(thread1, (OsalThreadEntry)TimerOnceTestThreadFunc, (void *)handle1);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("TimerTestCreateThread: create test once timer fail, ret: %d!", ret);
        return ret;
    }

    ret = OsalThreadCreate(thread2, (OsalThreadEntry)TimerPeriodTestThreadFunc, (void *)handle2);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("TimerTestCreateThread: create test period timer fail, ret: %d!", ret);
        return ret;
    }

    return HDF_SUCCESS;
}

static void MultiThreadSourceRecycle(struct OsalThread *thread1, struct OsalThread *thread2,
    DevHandle handle1, DevHandle handle2)
{
    if (handle1 != NULL) {
        HwTimerClose(handle1);
        handle1 = NULL;
    }

    if (handle2 != NULL) {
        HwTimerClose(handle2);
        handle2 = NULL;
    }

    if (thread1->realThread != NULL) {
        (void)OsalThreadDestroy(thread1);
    }

    if (thread2->realThread != NULL) {
        (void)OsalThreadDestroy(thread2);
    }

    g_theard1Flag = false;
    g_theard2Flag = false;
}

static int32_t TimerTestMultiThread(const struct TimerTest *test)
{
    int32_t ret;
    struct OsalThread thread1;
    struct OsalThread thread2;
    DevHandle handle1 = NULL;
    DevHandle handle2 = NULL;
    thread1.realThread = NULL;
    thread2.realThread = NULL;

    if (test == NULL) {
        HDF_LOGE("TimerTestMultiThread: timer test is null!");
        return HDF_FAILURE;
    }

    handle1 = HwTimerOpen(TIMER_TEST_TIME_ID_THREAD1);
    if (handle1 == NULL) {
        HDF_LOGE("TimerTestMultiThread: timer test get handle1 fail!");
        MultiThreadSourceRecycle(&thread1, &thread2, handle1, handle2);
        return HDF_FAILURE;
    }

    handle2 = HwTimerOpen(TIMER_TEST_TIME_ID_THREAD2);
    if (handle2 == NULL) {
        HDF_LOGE("TimerTestMultiThread: timer test get handle2 fail!");
        MultiThreadSourceRecycle(&thread1, &thread2, handle1, handle2);
        return HDF_FAILURE;
    }

    ret = TimerTestCreateThread(&thread1, &thread2, handle1, handle2);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("TimerTestMultiThread: timer test create thread fail, ret: %d!", ret);
        MultiThreadSourceRecycle(&thread1, &thread2, handle1, handle2);
        return ret;
    }

    ret = TimerTestStartThread(&thread1, &thread2);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("TimerTestMultiThread: timer test start thread fail, ret: %d!", ret);
        MultiThreadSourceRecycle(&thread1, &thread2, handle1, handle2);
        return ret;
    }

    MultiThreadSourceRecycle(&thread1, &thread2, handle1, handle2);
    return HDF_SUCCESS;
}

static int32_t TimerTestReliability(const struct TimerTest *test)
{
    if (test == NULL || test->handle == NULL) {
        HDF_LOGE("TimerTestReliability: test or handle is null!");
        return HDF_ERR_INVALID_OBJECT;
    }

    HwTimerSet(test->handle, test->uSecond, NULL);
    HwTimerSetOnce(test->handle, test->uSecond, NULL);
    HwTimerStart(NULL);
    return HDF_SUCCESS;
}

static int32_t TimerTestGetConfig(struct TimerTestConfig *config)
{
    int32_t ret;
    struct HdfSBuf *reply = NULL;
    struct HdfIoService *service = NULL;
    const void *buf = NULL;
    uint32_t len;

    if (config == NULL) {
        HDF_LOGE("TimerTestGetConfig: config is null!");
        return HDF_FAILURE;
    }

    service = HdfIoServiceBind("TIMER_TEST");
    if (service == NULL) {
        HDF_LOGE("TimerTestGetConfig: service TIMER_TEST bind fail!");
        return HDF_ERR_NOT_SUPPORT;
    }

    reply = HdfSbufObtain(sizeof(*config) + sizeof(uint64_t));
    if (reply == NULL) {
        HDF_LOGE("TimerTestGetConfig: fail to obtain reply!");
        HdfIoServiceRecycle(service);
        return HDF_ERR_MALLOC_FAIL;
    }

    ret = service->dispatcher->Dispatch(&service->object, 0, NULL, reply);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("TimerTestGetConfig: remote dispatch fail, ret: %d!", ret);
        HdfSbufRecycle(reply);
        HdfIoServiceRecycle(service);
        return ret;
    }

    if (!HdfSbufReadBuffer(reply, &buf, &len)) {
        HDF_LOGE("TimerTestGetConfig: read buf fail!");
        HdfSbufRecycle(reply);
        HdfIoServiceRecycle(service);
        return HDF_ERR_IO;
    }

    if (len != sizeof(*config)) {
        HDF_LOGE("TimerTestGetConfig: config size:%zu, read size:%u!", sizeof(*config), len);
        HdfSbufRecycle(reply);
        HdfIoServiceRecycle(service);
        return HDF_ERR_IO;
    }

    if (memcpy_s(config, sizeof(*config), buf, sizeof(*config)) != EOK) {
        HDF_LOGE("TimerTestGetConfig: memcpy buf fail!");
        HdfSbufRecycle(reply);
        HdfIoServiceRecycle(service);
        return HDF_ERR_IO;
    }

    HdfSbufRecycle(reply);
    HdfIoServiceRecycle(service);
    return HDF_SUCCESS;
}

static struct TimerTest *TimerTestGet(void)
{
    int32_t ret;
    static struct TimerTest tester;
    static bool hasInit = false;

    if (hasInit) {
        return &tester;
    }

    struct TimerTestConfig config;
    ret = TimerTestGetConfig(&config);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("TimerTestGet: read config fail, ret: %d!", ret);
        return NULL;
    }

    tester.number = config.number;
    tester.uSecond = config.uSecond;
    tester.isPeriod = config.isPeriod;
    hasInit = true;
    return &tester;
}

static int32_t TimerIfPerformanceTest(const struct TimerTest *test)
{
#ifdef __LITEOS__
    // liteos the accuracy of the obtained time is too large and inaccurate.
    if (test == NULL) {
        HDF_LOGE("TimerIfPerformanceTest: timer test is null!");
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
#endif

    uint64_t startMs;
    uint64_t endMs;
    uint64_t useTime; // ms
    uint32_t uSecond;
    bool isPeriod;

    if (test == NULL) {
        HDF_LOGE("TimerIfPerformanceTest: timer test is null!");
        return HDF_FAILURE;
    }
    startMs = OsalGetSysTimeMs();
    HwTimerGet(test->handle, &uSecond, &isPeriod);
    endMs = OsalGetSysTimeMs();

    useTime = endMs - startMs;
    HDF_LOGI("TimerIfPerformanceTest: ----->interface performance test:[start - end] < 1ms[%s]\r\n",
        useTime < 1 ? "yes" : "no");
    return HDF_SUCCESS;
}

static struct TimerTestFunc g_timerTestFunc[] = {
    {TIMER_TEST_SET,            TimerSetTest          },
    {TIMER_TEST_SETONCE,        TimerSetOnceTest      },
    {TIMER_TEST_GET,            TimerGetTest          },
    {TIMER_TEST_START,          TimerStartTest        },
    {TIMER_TEST_STOP,           TimerStopTest         },
    {TIMER_MULTI_THREAD_TEST,   TimerTestMultiThread  },
    {TIMER_RELIABILITY_TEST,    TimerTestReliability  },
    {TIMER_IF_PERFORMANCE_TEST, TimerIfPerformanceTest},
};

int32_t TimerTestExecute(int cmd)
{
    uint32_t i;
    int32_t ret = HDF_ERR_NOT_SUPPORT;

    if (cmd > TIMER_TEST_MAX_CMD) {
        HDF_LOGE("TimerTestExecute: invalid cmd:%d!", cmd);
        return HDF_ERR_NOT_SUPPORT;
    }

    struct TimerTest *test = TimerTestGet();
    if (test == NULL) {
        HDF_LOGE("TimerTestExecute: test null cmd %d!", cmd);
        return HDF_ERR_INVALID_OBJECT;
    }

    if (cmd != TIMER_MULTI_THREAD_TEST) {
        test->handle = HwTimerOpen(test->number);
        if (test->handle == NULL) {
            HDF_LOGE("TimerTestExecute: timer test get handle fail!");
            return HDF_FAILURE;
        }
    }

    for (i = 0; i < sizeof(g_timerTestFunc) / sizeof(g_timerTestFunc[0]); i++) {
        if (cmd == g_timerTestFunc[i].type && g_timerTestFunc[i].Func != NULL) {
            ret = g_timerTestFunc[i].Func(test);
            break;
        }
    }

    if (cmd != TIMER_MULTI_THREAD_TEST) {
        HwTimerClose(test->handle);
    }

    return ret;
}
