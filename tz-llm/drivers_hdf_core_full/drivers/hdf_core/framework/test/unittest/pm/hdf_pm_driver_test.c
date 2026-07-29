/*
 * Copyright (c) 2021-2022 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include "hdf_pm_driver_test.h"
#include "devsvc_manager.h"
#include "devsvc_manager_clnt.h"
#include "hdf_device_desc.h"
#include "hdf_device_node.h"
#include "hdf_log.h"
#include "hdf_pm.h"
#include "hdf_power_manager.h"
#include "hdf_task_queue.h"
#include "osal_time.h"
#include "power_state_token.h"

#define HDF_LOG_TAG pm_driver_test

#define PM_TEST_COUNT_ONE 1
#define PM_TEST_COUNT_TWO 2
#define PM_TEST_COUNT_TEN 10
#define PM_TEST_COUNT_HUNDRED 100
#define PM_TEST_COUNT_THOUSAND 1000

#define PM_WAIT_TIME 10
#define PM_WAIT_TIME_OUT 100
#define PM_WAIT_LOAD_TIME 30

#define CHECK_VALUE(index, cnt) \
    (g_pmTestType[(index)].resumeCnt == (cnt) && g_pmTestType[(index)].suspendCnt == (cnt))

#define WAIT_TEST_END(value, idx) \
    while ((value) == false && (idx) < PM_WAIT_TIME_OUT) { \
        OsalMSleep(PM_WAIT_TIME); \
        (idx)++; \
    } \

enum {
    HDF_TEST_DRIVER,
    SAMPLE_TEST_DRIVER,
    PM_TEST_DRIVER,
};

struct PmDriverPmListener {
    struct IPowerEventListener powerListener;
    void *p;
};
typedef int32_t (*TestFunc)(void);

struct TestCaseType {
    uint32_t cmd;
    TestFunc testFunc;
};
static int32_t HdfPmTestBegin(void);
static int32_t HdfPmTestOneDriverOnce(void);
static int32_t HdfPmTestOneDriverTwice(void);
static int32_t HdfPmTestOneDriverTen(void);
static int32_t HdfPmTestOneDriverHundred(void);
static int32_t HdfPmTestOneDriverThousand(void);
static int32_t HdfPmTestTwoDriverOnce(void);
static int32_t HdfPmTestTwoDriverTwice(void);
static int32_t HdfPmTestTwoDriverTen(void);
static int32_t HdfPmTestTwoDriverHundred(void);
static int32_t HdfPmTestTwoDriverThousand(void);
static int32_t HdfPmTestThreeDriverOnce(void);
static int32_t HdfPmTestThreeDriverTwice(void);
static int32_t HdfPmTestThreeDriverTen(void);
static int32_t HdfPmTestThreeDriverHundred(void);
static int32_t HdfPmTestThreeDriverThousand(void);
static int32_t HdfPmTestThreeDriverSeqHundred(void);
static int32_t HdfPmTestThreeDriverHundredWithSync(void);
static int32_t HdfPmTestEnd(void);

static const struct TestCaseType g_testCases[] = {
    { HDF_PM_TEST_BEGEN, HdfPmTestBegin },
    { HDF_PM_TEST_ONE_DRIVER_ONCE, HdfPmTestOneDriverOnce },
    { HDF_PM_TEST_ONE_DRIVER_TWICE, HdfPmTestOneDriverTwice },
    { HDF_PM_TEST_ONE_DRIVER_TEN, HdfPmTestOneDriverTen },
    { HDF_PM_TEST_ONE_DRIVER_HUNDRED, HdfPmTestOneDriverHundred },
    { HDF_PM_TEST_ONE_DRIVER_THOUSAND, HdfPmTestOneDriverThousand },
    { HDF_PM_TEST_TWO_DRIVER_ONCE, HdfPmTestTwoDriverOnce },
    { HDF_PM_TEST_TWO_DRIVER_TWICE, HdfPmTestTwoDriverTwice },
    { HDF_PM_TEST_TWO_DRIVER_TEN, HdfPmTestTwoDriverTen },
    { HDF_PM_TEST_TWO_DRIVER_HUNDRED, HdfPmTestTwoDriverHundred },
    { HDF_PM_TEST_TWO_DRIVER_THOUSAND, HdfPmTestTwoDriverThousand },
    { HDF_PM_TEST_THREE_DRIVER_ONCE, HdfPmTestThreeDriverOnce },
    { HDF_PM_TEST_THREE_DRIVER_TWICE, HdfPmTestThreeDriverTwice },
    { HDF_PM_TEST_THREE_DRIVER_TEN, HdfPmTestThreeDriverTen },
    { HDF_PM_TEST_THREE_DRIVER_HUNDRED, HdfPmTestThreeDriverHundred },
    { HDF_PM_TEST_THREE_DRIVER_THOUSAND, HdfPmTestThreeDriverThousand },
    { HDF_PM_TEST_THREE_DRIVER_SEQ_HUNDRED, HdfPmTestThreeDriverSeqHundred },
    { HDF_PM_TEST_THREE_DRIVER_HUNDRED_WITH_SYNC, HdfPmTestThreeDriverHundredWithSync },
    { HDF_PM_TEST_END, HdfPmTestEnd },
};

static const char *g_serviceName[] = { "khdf_ut", "sample_service", "pm_ut_service" };

struct PmTestType {
    const char *serviceName;
    struct HdfDeviceObject *obj;
    struct PmDriverPmListener listener;
    const struct IPowerEventListener *listenerBak;
    uint32_t resumeCnt;
    uint32_t suspendCnt;
};

static struct PmTestType g_pmTestType[PM_TEST_DRIVER + 1];

static bool loopTest = false;

static int HdfPmHdfTestDozeResume(struct HdfDeviceObject *deviceObject)
{
    (void)deviceObject;
    HDF_LOGI("%s called", __func__);
    return HDF_SUCCESS;
}

static int HdfPmHdfTestDozeSuspend(struct HdfDeviceObject *deviceObject)
{
    (void)deviceObject;
    HDF_LOGI("%s called", __func__);
    return HDF_SUCCESS;
}

static int HdfPmHdfTestResume(struct HdfDeviceObject *deviceObject)
{
    (void)deviceObject;
    if (loopTest == false) {
        HDF_LOGI("%s called", __func__);
    }
    
    g_pmTestType[HDF_TEST_DRIVER].resumeCnt++;
    return HDF_SUCCESS;
}

static int HdfPmHdfTestSuspend(struct HdfDeviceObject *deviceObject)
{
    (void)deviceObject;
    if (loopTest == false) {
        HDF_LOGI("%s called", __func__);
    }
    g_pmTestType[HDF_TEST_DRIVER].suspendCnt++;
    return HDF_SUCCESS;
}

static int HdfPmSampleDozeResume(struct HdfDeviceObject *deviceObject)
{
    (void)deviceObject;
    HDF_LOGI("%s called", __func__);
    return HDF_SUCCESS;
}

static int HdfPmSampleDozeSuspend(struct HdfDeviceObject *deviceObject)
{
    (void)deviceObject;
    HDF_LOGI("%s called", __func__);
    return HDF_SUCCESS;
}

static int HdfPmSampleResume(struct HdfDeviceObject *deviceObject)
{
    (void)deviceObject;
    if (loopTest == false) {
        HDF_LOGI("%s called", __func__);
    }
    g_pmTestType[SAMPLE_TEST_DRIVER].resumeCnt++;
    return HDF_SUCCESS;
}

static int HdfPmSampleSuspend(struct HdfDeviceObject *deviceObject)
{
    (void)deviceObject;
    if (loopTest == false) {
        HDF_LOGI("%s called", __func__);
    }
    g_pmTestType[SAMPLE_TEST_DRIVER].suspendCnt++;
    return HDF_SUCCESS;
}

static int HdfPmTestDozeResume(struct HdfDeviceObject *deviceObject)
{
    (void)deviceObject;
    HDF_LOGI("%s called", __func__);
    return HDF_SUCCESS;
}

static int HdfPmTestDozeSuspend(struct HdfDeviceObject *deviceObject)
{
    (void)deviceObject;
    HDF_LOGI("%s called", __func__);
    return HDF_SUCCESS;
}

static int HdfPmTestResume(struct HdfDeviceObject *deviceObject)
{
    (void)deviceObject;
    if (loopTest == false) {
        HDF_LOGI("%s called", __func__);
    }
    g_pmTestType[PM_TEST_DRIVER].resumeCnt++;
    return HDF_SUCCESS;
}

static int HdfPmTestSuspend(struct HdfDeviceObject *deviceObject)
{
    (void)deviceObject;
    if (loopTest == false) {
        HDF_LOGI("%s called", __func__);
    }
    g_pmTestType[PM_TEST_DRIVER].suspendCnt++;
    return HDF_SUCCESS;
}

static void HdfPmSetListeners(void)
{
    g_pmTestType[HDF_TEST_DRIVER].listener.powerListener.DozeResume = HdfPmHdfTestDozeResume;
    g_pmTestType[HDF_TEST_DRIVER].listener.powerListener.DozeSuspend = HdfPmHdfTestDozeSuspend;
    g_pmTestType[HDF_TEST_DRIVER].listener.powerListener.Resume = HdfPmHdfTestResume;
    g_pmTestType[HDF_TEST_DRIVER].listener.powerListener.Suspend = HdfPmHdfTestSuspend;

    g_pmTestType[SAMPLE_TEST_DRIVER].listener.powerListener.DozeResume = HdfPmSampleDozeResume;
    g_pmTestType[SAMPLE_TEST_DRIVER].listener.powerListener.DozeSuspend = HdfPmSampleDozeSuspend;
    g_pmTestType[SAMPLE_TEST_DRIVER].listener.powerListener.Resume = HdfPmSampleResume;
    g_pmTestType[SAMPLE_TEST_DRIVER].listener.powerListener.Suspend = HdfPmSampleSuspend;

    g_pmTestType[PM_TEST_DRIVER].listener.powerListener.DozeResume = HdfPmTestDozeResume;
    g_pmTestType[PM_TEST_DRIVER].listener.powerListener.DozeSuspend = HdfPmTestDozeSuspend;
    g_pmTestType[PM_TEST_DRIVER].listener.powerListener.Resume = HdfPmTestResume;
    g_pmTestType[PM_TEST_DRIVER].listener.powerListener.Suspend = HdfPmTestSuspend;
}

static void HdfPmClearTestCnt(void)
{
    uint32_t index;

    for (index = 0; index <= PM_TEST_DRIVER; index++) {
        g_pmTestType[index].resumeCnt = 0;
        g_pmTestType[index].suspendCnt = 0;
    }
}

static void HdfPmRegisterTestListener(int32_t index)
{
    struct SubscriberCallback callback = {NULL};
    struct HdfDeviceNode *devNode = NULL;

    g_pmTestType[index].serviceName = g_serviceName[index];
    g_pmTestType[index].obj = DevSvcManagerClntGetDeviceObject(g_pmTestType[index].serviceName);
    if (g_pmTestType[index].obj == NULL) {
        DevSvcManagerClntSubscribeService(g_pmTestType[index].serviceName, callback);
        OsalMSleep(PM_WAIT_LOAD_TIME);
        g_pmTestType[index].obj = DevSvcManagerClntGetDeviceObject(g_pmTestType[index].serviceName);
    }

    if (g_pmTestType[index].obj) {
        devNode = (struct HdfDeviceNode *)HDF_SLIST_CONTAINER_OF(
            struct HdfDeviceObject, g_pmTestType[index].obj, struct HdfDeviceNode, deviceObject);
        if ((devNode->powerToken != NULL) && (devNode->powerToken->listener != NULL)) {
            g_pmTestType[index].listenerBak = devNode->powerToken->listener;
            HdfPmUnregisterPowerListener(g_pmTestType[index].obj, g_pmTestType[index].listenerBak);
        } else {
            g_pmTestType[index].listenerBak = NULL;
        }

        HdfPmRegisterPowerListener(g_pmTestType[index].obj, &g_pmTestType[index].listener.powerListener);
    }
    HdfPmSetMode(g_pmTestType[index].obj, HDF_POWER_DYNAMIC_CTRL);
}

static void HdfPmBakListener(int32_t index)
{
    HdfPmUnregisterPowerListener(g_pmTestType[index].obj, &g_pmTestType[index].listener.powerListener);
    if (g_pmTestType[index].listenerBak != NULL) {
        HdfPmRegisterPowerListener(g_pmTestType[index].obj, g_pmTestType[index].listenerBak);
        HdfPmSetMode(g_pmTestType[index].obj, HDF_POWER_SYS_CTRL);
    }
}

static void HdfPmTestAcquire(uint32_t index)
{
    HdfPmAcquireDeviceAsync(g_pmTestType[index].obj);
}

static void HdfPmTestRelease(uint32_t index)
{
    HdfPmReleaseDeviceAsync(g_pmTestType[index].obj);
}

static void HdfPmTestAcquireSync(uint32_t index)
{
    HdfPmAcquireDevice(g_pmTestType[index].obj);
}

static void HdfPmTestReleaseSync(uint32_t index)
{
    HdfPmReleaseDevice(g_pmTestType[index].obj);
}

static int32_t HdfPmTestBegin(void)
{
    uint32_t index;

    HdfPmTaskQueueInit(NULL);
    HdfPmSetListeners();
    HdfPmClearTestCnt();

    for (index = 0; index <= PM_TEST_DRIVER; index++) {
        HdfPmRegisterTestListener(index);
    }
    return HDF_SUCCESS;
}

static int32_t HdfPmTestEnd(void)
{
    uint32_t index;

    for (index = 0; index <= PM_TEST_DRIVER; index++) {
        HdfPmBakListener(index);
    }

    HdfPowerManagerExit();
    HdfPmTaskQueueInit(NULL);
    loopTest = false;

    return HDF_SUCCESS;
}

static int32_t HdfPmTestOneDriver(const uint32_t times)
{
    uint32_t index;
    uint32_t waitTime = 0;
    uint64_t beginTimes = OsalGetSysTimeMs();
    uint32_t expendTimes;

    HdfPmClearTestCnt();

    for (index = 0; index < times; index++) {
        HdfPmTestAcquire(PM_TEST_DRIVER);
        HdfPmTestRelease(PM_TEST_DRIVER);
    }

    WAIT_TEST_END(CHECK_VALUE(PM_TEST_DRIVER, times), waitTime);

    HDF_LOGI("%s %d %d", __func__, g_pmTestType[PM_TEST_DRIVER].resumeCnt, g_pmTestType[PM_TEST_DRIVER].suspendCnt);

    expendTimes = OsalGetSysTimeMs() - beginTimes;
    HDF_LOGI("%s test expend times:%u ms", __func__, expendTimes);

    return CHECK_VALUE(PM_TEST_DRIVER, times) ? HDF_SUCCESS : HDF_FAILURE;
}

static int32_t HdfPmTestTwoDriver(const uint32_t times)
{
    uint32_t index;
    uint32_t waitTime = 0;
    uint64_t beginTimes = OsalGetSysTimeMs();
    uint32_t expendTimes;

    HdfPmClearTestCnt();

    for (index = 0; index < times; index++) {
        HdfPmTestAcquire(PM_TEST_DRIVER);
        HdfPmTestAcquire(HDF_TEST_DRIVER);
        HdfPmTestRelease(PM_TEST_DRIVER);
        HdfPmTestRelease(HDF_TEST_DRIVER);
    }

    WAIT_TEST_END(CHECK_VALUE(PM_TEST_DRIVER, times) && CHECK_VALUE(HDF_TEST_DRIVER, times), waitTime);

    HDF_LOGI("%s %d %d", __func__, g_pmTestType[PM_TEST_DRIVER].resumeCnt, g_pmTestType[PM_TEST_DRIVER].suspendCnt);
    HDF_LOGI("%s %d %d", __func__, g_pmTestType[HDF_TEST_DRIVER].resumeCnt, g_pmTestType[HDF_TEST_DRIVER].suspendCnt);

    expendTimes = OsalGetSysTimeMs() - beginTimes;
    HDF_LOGI("%s test expend times:%d ms", __func__, expendTimes);

    return (CHECK_VALUE(PM_TEST_DRIVER, times) && CHECK_VALUE(HDF_TEST_DRIVER, times)) ? HDF_SUCCESS : HDF_FAILURE;
}

static int32_t HdfPmTestThreeDriver(const uint32_t times, bool sync)
{
    uint32_t index;
    uint32_t total = times;
    uint32_t waitTime = 0;
    uint64_t beginTimes = OsalGetSysTimeMs();
    uint32_t expendTimes;

    HdfPmClearTestCnt();

    if (sync) {
        total += PM_TEST_COUNT_ONE;
        HdfPmTestAcquireSync(PM_TEST_DRIVER);
        HdfPmTestAcquireSync(HDF_TEST_DRIVER);
        HdfPmTestAcquireSync(SAMPLE_TEST_DRIVER);
        HdfPmTestReleaseSync(PM_TEST_DRIVER);
        HdfPmTestReleaseSync(HDF_TEST_DRIVER);
        HdfPmTestReleaseSync(SAMPLE_TEST_DRIVER);
    }

    for (index = 0; index < times; index++) {
        HdfPmTestAcquire(PM_TEST_DRIVER);
        HdfPmTestAcquire(HDF_TEST_DRIVER);
        HdfPmTestAcquire(SAMPLE_TEST_DRIVER);

        HdfPmTestRelease(PM_TEST_DRIVER);
        HdfPmTestRelease(HDF_TEST_DRIVER);
        HdfPmTestRelease(SAMPLE_TEST_DRIVER);
    }

    WAIT_TEST_END(CHECK_VALUE(PM_TEST_DRIVER, total) &&
        CHECK_VALUE(HDF_TEST_DRIVER, total) &&
        CHECK_VALUE(SAMPLE_TEST_DRIVER, total), waitTime);

    HDF_LOGI("%s %d %d", __func__, g_pmTestType[PM_TEST_DRIVER].resumeCnt, g_pmTestType[PM_TEST_DRIVER].suspendCnt);
    HDF_LOGI("%s %d %d", __func__, g_pmTestType[HDF_TEST_DRIVER].resumeCnt, g_pmTestType[HDF_TEST_DRIVER].suspendCnt);
    HDF_LOGI("%s %d %d", __func__,
        g_pmTestType[SAMPLE_TEST_DRIVER].resumeCnt, g_pmTestType[SAMPLE_TEST_DRIVER].suspendCnt);

    expendTimes = OsalGetSysTimeMs() - beginTimes;
    HDF_LOGI("%s test expend times:%d ms", __func__, expendTimes);

    return (CHECK_VALUE(PM_TEST_DRIVER, total) && CHECK_VALUE(HDF_TEST_DRIVER, total) &&
        CHECK_VALUE(SAMPLE_TEST_DRIVER, total)) ? HDF_SUCCESS : HDF_FAILURE;
}

static int32_t HdfPmTestThreeDriverSeqHundred(void)
{
    uint32_t index;
    uint32_t waitTime = 0;
    uint64_t beginTimes = OsalGetSysTimeMs();
    uint32_t expendTimes;

    HdfPmClearTestCnt();

    for (index = 0; index < PM_TEST_COUNT_TWO; index++) {
        HdfPmTestAcquire(PM_TEST_DRIVER);
        HdfPmTestAcquire(HDF_TEST_DRIVER);
        HdfPmTestAcquire(SAMPLE_TEST_DRIVER);
    }

    for (index = 0; index < PM_TEST_COUNT_TWO; index++) {
        HdfPmTestRelease(PM_TEST_DRIVER);
        HdfPmTestRelease(HDF_TEST_DRIVER);
        HdfPmTestRelease(SAMPLE_TEST_DRIVER);
    }

    WAIT_TEST_END(CHECK_VALUE(PM_TEST_DRIVER, PM_TEST_COUNT_ONE) &&
        CHECK_VALUE(HDF_TEST_DRIVER, PM_TEST_COUNT_ONE) &&
        CHECK_VALUE(SAMPLE_TEST_DRIVER, PM_TEST_COUNT_ONE), waitTime);

    HDF_LOGI("%s %d %d", __func__, g_pmTestType[PM_TEST_DRIVER].resumeCnt, g_pmTestType[PM_TEST_DRIVER].suspendCnt);
    HDF_LOGI("%s %d %d", __func__, g_pmTestType[HDF_TEST_DRIVER].resumeCnt, g_pmTestType[HDF_TEST_DRIVER].suspendCnt);
    HDF_LOGI("%s %d %d", __func__,
        g_pmTestType[SAMPLE_TEST_DRIVER].resumeCnt, g_pmTestType[SAMPLE_TEST_DRIVER].suspendCnt);

    expendTimes = OsalGetSysTimeMs() - beginTimes;
    HDF_LOGI("%s test expend times:%u ms", __func__, expendTimes);

    return (CHECK_VALUE(PM_TEST_DRIVER, PM_TEST_COUNT_ONE) && CHECK_VALUE(HDF_TEST_DRIVER, PM_TEST_COUNT_ONE) &&
        CHECK_VALUE(SAMPLE_TEST_DRIVER, PM_TEST_COUNT_ONE)) ? HDF_SUCCESS : HDF_FAILURE;
}

static int32_t HdfPmTestOneDriverOnce(void)
{
    loopTest = false;
    return HdfPmTestOneDriver(PM_TEST_COUNT_ONE);
}

static int32_t HdfPmTestOneDriverTwice(void)
{
    loopTest = false;
    return HdfPmTestOneDriver(PM_TEST_COUNT_TWO);
}

static int32_t HdfPmTestOneDriverTen(void)
{
    loopTest = true;
    return HdfPmTestOneDriver(PM_TEST_COUNT_TEN);
}

static int32_t HdfPmTestOneDriverHundred(void)
{
    loopTest = true;
    return HdfPmTestOneDriver(PM_TEST_COUNT_HUNDRED);
}

static int32_t HdfPmTestOneDriverThousand(void)
{
    loopTest = true;
    return HdfPmTestOneDriver(PM_TEST_COUNT_THOUSAND);
}

static int32_t HdfPmTestTwoDriverOnce(void)
{
    loopTest = false;
    return HdfPmTestTwoDriver(PM_TEST_COUNT_ONE);
}

static int32_t HdfPmTestTwoDriverTwice(void)
{
    loopTest = false;
    return HdfPmTestTwoDriver(PM_TEST_COUNT_TWO);
}

static int32_t HdfPmTestTwoDriverTen(void)
{
    loopTest = true;
    return HdfPmTestTwoDriver(PM_TEST_COUNT_TEN);
}

static int32_t HdfPmTestTwoDriverHundred(void)
{
    loopTest = true;
    return HdfPmTestTwoDriver(PM_TEST_COUNT_HUNDRED);
}

static int32_t HdfPmTestTwoDriverThousand(void)
{
    loopTest = true;
    return HdfPmTestTwoDriver(PM_TEST_COUNT_THOUSAND);
}

static int32_t HdfPmTestThreeDriverOnce(void)
{
    loopTest = false;
    return HdfPmTestThreeDriver(PM_TEST_COUNT_ONE, false);
}

static int32_t HdfPmTestThreeDriverTwice(void)
{
    loopTest = false;
    return HdfPmTestThreeDriver(PM_TEST_COUNT_TWO, false);
}

static int32_t HdfPmTestThreeDriverTen(void)
{
    loopTest = true;
    return HdfPmTestThreeDriver(PM_TEST_COUNT_TEN, false);
}

static int32_t HdfPmTestThreeDriverHundred(void)
{
    loopTest = true;
    return HdfPmTestThreeDriver(PM_TEST_COUNT_HUNDRED, false);
}

static int32_t HdfPmTestThreeDriverThousand(void)
{
    loopTest = true;
    return HdfPmTestThreeDriver(PM_TEST_COUNT_THOUSAND, false);
}

static int32_t HdfPmTestThreeDriverHundredWithSync(void)
{
    loopTest = true;
    return HdfPmTestThreeDriver(PM_TEST_COUNT_HUNDRED, true);
}

static void HdfPmDriverRelease(struct HdfDeviceObject *deviceObject)
{
    (void)deviceObject;
    return;
}

static int32_t HdfPmDriverDispatch(
    struct HdfDeviceIoClient *client, int cmdId, struct HdfSBuf *data, struct HdfSBuf *reply)
{
    int32_t ret = HDF_FAILURE;
    uint32_t index;

    (void)client;
    (void)data;
    (void)reply;
    HDF_LOGI("%s %d enter!", __func__, cmdId);

    for (index = 0; index <= HDF_PM_TEST_END; index++) {
        if (cmdId == g_testCases[index].cmd) {
            ret = g_testCases[cmdId].testFunc();
            break;
        }
    }
    
    return ret;
}

static int HdfPmDriverBind(struct HdfDeviceObject *deviceObject)
{
    HDF_LOGI("%s enter", __func__);
    if (deviceObject == NULL) {
        return HDF_FAILURE;
    }
    static struct IDeviceIoService testService = {
        .Dispatch = HdfPmDriverDispatch,
        .Open = NULL,
        .Release = NULL,
    };
    deviceObject->service = &testService;
    return HDF_SUCCESS;
}

static int HdfPmDriverInit(struct HdfDeviceObject *deviceObject)
{
    static struct PmDriverPmListener pmListener = {0};

    HDF_LOGI("%s enter!", __func__);
    if (deviceObject == NULL) {
        HDF_LOGE("%s ptr is null!", __func__);
        return HDF_FAILURE;
    }
    HDF_LOGD("%s Init success", __func__);

    pmListener.powerListener.DozeResume = HdfPmTestDozeResume;
    pmListener.powerListener.DozeSuspend = HdfPmTestDozeSuspend;
    pmListener.powerListener.Resume = HdfPmTestResume;
    pmListener.powerListener.Suspend = HdfPmTestSuspend;

    int ret = HdfPmRegisterPowerListener(deviceObject, &pmListener.powerListener);
    HDF_LOGI("%s register power listener, ret = %d", __func__, ret);

    return HDF_SUCCESS;
}

struct HdfDriverEntry g_pmDriverEntry = {
    .moduleVersion = 1,
    .moduleName = "pm_test_driver",
    .Bind = HdfPmDriverBind,
    .Init = HdfPmDriverInit,
    .Release = HdfPmDriverRelease,
};

HDF_INIT(g_pmDriverEntry);

