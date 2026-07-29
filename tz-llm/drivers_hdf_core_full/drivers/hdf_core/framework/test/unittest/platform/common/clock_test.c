/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include "clock_test.h"
#include "clock_if.h"
#include "hdf_base.h"
#include "hdf_io_service_if.h"
#include "hdf_log.h"
#include "osal_mem.h"
#include "securec.h"
#include "osal_thread.h"
#include "osal_time.h"

#define DEFAULT_RATE                 10000
#define CLOCK_TEST_STACK_SIZE        (1024 * 64)
#define CLOCK_TEST_SLEEP_TIME        100
#define CLOCK_TEST_WAIT_TIMEOUT      20
#define HDF_LOG_TAG clock_test_c
DevHandle parent = NULL;

static int32_t ClockTestGetConfig(struct ClockTestConfig *config)
{
    int32_t ret;
    struct HdfSBuf *reply = NULL;
    struct HdfIoService *service = NULL;
    const void *buf = NULL;
    uint32_t len;

    HDF_LOGD("ClockTestGetConfig: enter!");
    service = HdfIoServiceBind("CLOCK_TEST");
    if (service == NULL) {
        HDF_LOGE("ClockTestGetConfig: service is null!");
        return HDF_ERR_NOT_SUPPORT;
    }

    do {
        reply = HdfSbufObtain(sizeof(*config) + sizeof(uint64_t));
        if (reply == NULL) {
            HDF_LOGE("ClockTestGetConfig: fail to obtain reply!");
            ret = HDF_ERR_MALLOC_FAIL;
            break;
        }

        ret = service->dispatcher->Dispatch(&service->object, 0, NULL, reply);
        if (ret != HDF_SUCCESS) {
            HDF_LOGE("ClockTestGetConfig: remote dispatch fail!");
            break;
        }

        if (!HdfSbufReadBuffer(reply, &buf, &len)) {
            HDF_LOGE("ClockTestGetConfig: read buf fail!");
            ret = HDF_ERR_IO;
            break;
        }

        if (len != sizeof(*config)) {
            HDF_LOGE("ClockTestGetConfig: config size:%zu, read size:%u!", sizeof(*config), len);
            ret = HDF_ERR_IO;
            break;
        }

        if (memcpy_s(config, sizeof(*config), buf, sizeof(*config)) != EOK) {
            HDF_LOGE("ClockTestGetConfig: memcpy buf fail!");
            ret = HDF_ERR_IO;
            break;
        }
        HDF_LOGD("ClockTestGetConfig: exit!");
        ret = HDF_SUCCESS;
    } while (0);
    HdfSbufRecycle(reply);
    HdfIoServiceRecycle(service);
    return ret;
}

static struct ClockTester *ClockTesterGet(void)
{
    int32_t ret;
    static struct ClockTester tester;
    static bool hasInit = false;

    HDF_LOGE("ClockTesterGet: enter!");
    if (hasInit) {
        return &tester;
    }
    ret = ClockTestGetConfig(&tester.config);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("ClockTesterGet: read config fail, ret: %d!", ret);
        return NULL;
    }
    tester.handle = ClockOpen(tester.config.deviceIndex);
    if (tester.handle == NULL) {
        HDF_LOGE("ClockTesterGet: open clock device:%u fail!", tester.config.deviceIndex);
        return NULL;
    }
    hasInit = true;
    HDF_LOGI("ClockTesterGet: done!");
    return &tester;
}

static int32_t ClockTestEnable(void)
{
    struct ClockTester *tester = NULL;
    int32_t ret;

    HDF_LOGI("ClockTestEnable: enter!");
    tester = ClockTesterGet();
    if (tester == NULL) {
        HDF_LOGE("ClockTestEnable: get tester fail!");
        return HDF_ERR_INVALID_OBJECT;
    }

    ret = ClockEnable(tester->handle);

    HDF_LOGI("ClockTestEnable: clock device num is %u!", tester->config.deviceIndex);
    HDF_LOGI("ClockTestEnable: done!");
    return ret;
}

static int ClockTestThreadFunc(void *param)
{
    struct ClockTester *tester = NULL;
    int32_t ret;

    HDF_LOGI("ClockTestThreadFunc: enter!");
    tester = ClockTesterGet();
    if (tester == NULL) {
        HDF_LOGE("ClockTestThreadFunc: get tester fail!");
        *((int32_t *)param) = 1;
        return HDF_ERR_INVALID_OBJECT;
    }

    ret = ClockEnable(tester->handle);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("ClockTestThreadFunc: clock read fail, ret: %d!", ret);
        *((int32_t *)param) = 1;
        return HDF_ERR_IO;
    }

    *((int32_t *)param) = 1;
    HDF_LOGI("ClockTestThreadFunc: done!");
    return HDF_SUCCESS;
}

static int32_t ClockTestStartThread(struct OsalThread *thread1, struct OsalThread *thread2, const int32_t *count1,
                                    const int32_t *count2)
{
    int32_t ret;
    uint32_t time = 0;
    struct OsalThreadParam cfg1;
    struct OsalThreadParam cfg2;

    if (memset_s(&cfg1, sizeof(cfg1), 0, sizeof(cfg1)) != EOK ||
        memset_s(&cfg2, sizeof(cfg2), 0, sizeof(cfg2)) != EOK) {
        HDF_LOGE("ClockTestStartThread: memset_s fail!");
        return HDF_ERR_IO;
    }

    cfg1.name = "ClockTestThread-1";
    cfg2.name = "ClockTestThread-2";
    cfg1.priority = cfg2.priority = OSAL_THREAD_PRI_DEFAULT;
    cfg1.stackSize = cfg2.stackSize = CLOCK_TEST_STACK_SIZE;

    ret = OsalThreadStart(thread1, &cfg1);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("ClockTestStartThread: start test thread1 fail, ret: %d!", ret);
        return ret;
    }

    ret = OsalThreadStart(thread2, &cfg2);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("ClockTestStartThread: start test thread2 fail, ret: %d!", ret);
    }

    while (*count1 == 0 || *count2 == 0) {
        HDF_LOGD("ClockTestStartThread: waitting testing thread finish...");
        OsalMSleep(CLOCK_TEST_SLEEP_TIME);
        time++;
        if (time > CLOCK_TEST_WAIT_TIMEOUT) {
            break;
        }
    }
    return ret;
}

static int32_t ClockTestDisable(void)
{
    struct ClockTester *tester = NULL;
    int32_t ret;

    HDF_LOGI("ClockTestDisable: enter!");
    tester = ClockTesterGet();
    if (tester == NULL) {
        HDF_LOGE("ClockTestDisable: get tester fail!");
        return HDF_ERR_INVALID_OBJECT;
    }

    ret = ClockDisable(tester->handle);

    HDF_LOGI("ClockTestDisable: done!");
    return ret;
}

static int32_t ClockTestSetrate(void)
{
    struct ClockTester *tester = NULL;
    int32_t ret;

    HDF_LOGI("ClockTestSetrate: enter!");
    tester = ClockTesterGet();
    if (tester == NULL) {
        HDF_LOGE("ClockTestSetrate: get tester fail!");
        return HDF_ERR_INVALID_OBJECT;
    }

    ret = ClockSetRate(tester->handle, DEFAULT_RATE);

    HDF_LOGI("ClockTestSetrate: done!");
    return ret;
}

static int32_t ClockTestGetrate(void)
{
    struct ClockTester *tester = NULL;
    int32_t ret;
    uint32_t rate;

    HDF_LOGI("ClockTestGetrate: enter!");
    tester = ClockTesterGet();
    if (tester == NULL) {
        HDF_LOGE("ClockTestGetrate: get tester fail!");
        return HDF_ERR_INVALID_OBJECT;
    }

    ret = ClockGetRate(tester->handle, &rate);

    HDF_LOGI("ClockTestGetrate: done!");
    return ret;
}

static int32_t ClockTestGetparent(void)
{
    struct ClockTester *tester = NULL;

    HDF_LOGI("ClockTestGetparent: enter!");
    tester = ClockTesterGet();
    if (tester == NULL) {
        HDF_LOGE("ClockTestGetparent: get tester fail!");
        return HDF_ERR_INVALID_OBJECT;
    }

    parent = ClockGetParent(tester->handle);

    HDF_LOGI("ClockTestGetparent: done!");
    return HDF_SUCCESS;
}

static int32_t ClockTestSetparent(void)
{
    struct ClockTester *tester = NULL;
    int32_t ret;

    HDF_LOGI("ClockTestSetparent: enter!");
    tester = ClockTesterGet();
    if (tester == NULL) {
        HDF_LOGE("ClockTestSetparent: get tester fail!");
        return HDF_ERR_INVALID_OBJECT;
    }

    ret = ClockSetParent(tester->handle, parent);

    HDF_LOGI("ClockTestSetparent: done!");
    return ret;
}

static int32_t ClockTestMultiThread(void)
{
    int32_t ret;
    struct OsalThread thread1;
    struct OsalThread thread2;
    int32_t count1 = 0;
    int32_t count2 = 0;

    ret = OsalThreadCreate(&thread1, (OsalThreadEntry)ClockTestThreadFunc, (void *)&count1);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("ClockTestMultiThread: create test thread1 fail, ret: %d!", ret);
        return ret;
    }

    ret = OsalThreadCreate(&thread2, (OsalThreadEntry)ClockTestThreadFunc, (void *)&count2);
    if (ret != HDF_SUCCESS) {
        (void)OsalThreadDestroy(&thread1);
        HDF_LOGE("ClockTestMultiThread: create test thread2 fail, ret: %d!", ret);
        return ret;
    }

    ret = ClockTestStartThread(&thread1, &thread2, &count1, &count2);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("ClockTestStartThread: test start thread fail, ret: %d!", ret);
    }

    (void)OsalThreadDestroy(&thread1);
    (void)OsalThreadDestroy(&thread2);
    return HDF_SUCCESS;
}

static int32_t ClockTestReliability(void)
{
    uint32_t rate;
    HDF_LOGI("ClockTestReliability: enter!");
    // invalid handle
    ClockEnable(NULL);
    // invalid handle
    ClockDisable(NULL);
    // invalid handle
    ClockSetRate(NULL, DEFAULT_RATE);
    // invalid handle
    ClockGetRate(NULL, &rate);
    // invalid handle
    ClockGetParent(NULL);
    // invalid handle
    ClockSetParent(NULL, NULL);
    HDF_LOGI("ClockTestReliability: done!");
    return HDF_SUCCESS;
}

static int32_t ClockIfPerformanceTest(void)
{
#ifdef __LITEOS__
    // liteos the accuracy of the obtained time is too large and inaccurate.
    return HDF_SUCCESS;
#endif
    struct ClockTester *tester = NULL;
    uint64_t startMs;
    uint64_t endMs;
    uint64_t useTime; // ms
    int32_t ret;

    tester = ClockTesterGet();
    if (tester == NULL || tester->handle == NULL) {
        HDF_LOGE("ClockIfPerformanceTest: get tester fail!");
        return HDF_ERR_INVALID_OBJECT;
    }

    startMs = OsalGetSysTimeMs();
    ret = ClockEnable(tester->handle);
    if (ret == HDF_SUCCESS) {
        endMs = OsalGetSysTimeMs();
        useTime = endMs - startMs;
        HDF_LOGI("ClockIfPerformanceTest: ----->interface performance test:[start - end] < 1ms[%s]\r\n",
                 useTime < 1 ? "yes" : "no");
        return HDF_SUCCESS;
    }
    return HDF_SUCCESS;
}

struct ClockTestEntry {
    int cmd;
    int32_t (*func)(void);
    const char *name;
};

static struct ClockTestEntry g_entry[] = {
    {CLOCK_TEST_CMD_ENABLE, ClockTestEnable, "ClockTestEnable"},
    {CLOCK_TEST_CMD_DISABLE, ClockTestDisable, "ClockTestDisable"},
    {CLOCK_TEST_CMD_SET_RATE, ClockTestSetrate, "ClockTestSetrate"},
    {CLOCK_TEST_CMD_GET_RATE, ClockTestGetrate, "ClockTestGetrate"},
    {CLOCK_TEST_CMD_GET_PARENT, ClockTestGetparent, "ClockTestGetparent"},
    {CLOCK_TEST_CMD_SET_PARENT, ClockTestSetparent, "ClockTestSetparent"},
    {CLOCK_TEST_CMD_MULTI_THREAD, ClockTestMultiThread, "ClockTestMultiThread"},
    {CLOCK_TEST_CMD_RELIABILITY, ClockTestReliability, "ClockTestReliability"},
    {CLOCK_IF_PERFORMANCE_TEST, ClockIfPerformanceTest, "ClockIfPerformanceTest"},
};

int32_t ClockTestExecute(int cmd)
{
    uint32_t i;
    int32_t ret = HDF_ERR_NOT_SUPPORT;

    if (cmd > CLOCK_TEST_CMD_MAX) {
        HDF_LOGE("ClockTestExecute: invalid cmd:%d!", cmd);
        ret = HDF_ERR_NOT_SUPPORT;
        HDF_LOGE("[ClockTestExecute][======cmd:%d====ret:%d======]", cmd, ret);
        return ret;
    }

    for (i = 0; i < sizeof(g_entry) / sizeof(g_entry[0]); i++) {
        if (g_entry[i].cmd != cmd || g_entry[i].func == NULL) {
            continue;
        }
        ret = g_entry[i].func();
        break;
    }

    HDF_LOGE("[ClockTestExecute][======cmd:%d====ret:%d======]", cmd, ret);
    return ret;
}
