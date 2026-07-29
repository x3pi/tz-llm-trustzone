/*
 * Copyright (c) 2022-2023 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include "rtc_test.h"
#include "hdf_base.h"
#include "hdf_io_service_if.h"
#include "hdf_log.h"
#include "osal_mem.h"
#include "osal_time.h"
#include "rtc_base.h"
#include "rtc_if.h"
#include "securec.h"

#define HDF_LOG_TAG rtc_test_c

#define RTC_TEST_TIME_ZONE 8

static int32_t RtcTestGetConfig(struct RtcTestConfig *config)
{
    int32_t ret;
    struct HdfSBuf *reply = NULL;
    struct HdfIoService *service = NULL;
    const void *buf = NULL;
    uint32_t len;

    service = HdfIoServiceBind("RTC_TEST");
    if (service == NULL) {
        HDF_LOGE("RtcTestGetConfig: service RTC_TEST bind fail!");
        return HDF_ERR_NOT_SUPPORT;
    }

    reply = HdfSbufObtainDefaultSize();
    if (reply == NULL) {
        HDF_LOGE("RtcTestGetConfig: fail to obtain reply!");
        HdfIoServiceRecycle(service);
        return HDF_ERR_MALLOC_FAIL;
    }

    ret = service->dispatcher->Dispatch(&service->object, 0, NULL, reply);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("RtcTestGetConfig: remote dispatch fail, ret: %d!", ret);
        HdfIoServiceRecycle(service);
        HdfSbufRecycle(reply);
        return ret;
    }

    if (!HdfSbufReadBuffer(reply, &buf, &len)) {
        HDF_LOGE("RtcTestGetConfig: read buf fail!");
        HdfIoServiceRecycle(service);
        HdfSbufRecycle(reply);
        return HDF_ERR_IO;
    }

    if (len != sizeof(*config)) {
        HDF_LOGE("RtcTestGetConfig: config size:%zu, read size:%u!", sizeof(*config), len);
        HdfIoServiceRecycle(service);
        HdfSbufRecycle(reply);
        return HDF_ERR_IO;
    }

    if (memcpy_s(config, sizeof(*config), buf, sizeof(*config)) != EOK) {
        HDF_LOGE("RtcTestGetConfig: memcpy buf fail!");
        HdfIoServiceRecycle(service);
        HdfSbufRecycle(reply);
        return HDF_ERR_IO;
    }

    HdfSbufRecycle(reply);
    HdfIoServiceRecycle(service);
    return HDF_SUCCESS;
}

static struct RtcTester *RtcTesterGet(void)
{
    int32_t ret;
    static struct RtcTester tester;

    ret = RtcTestGetConfig(&tester.config);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("RtcTesterGet: read config fail, ret: %d!", ret);
        return NULL;
    }

    tester.handle = RtcOpen();
    if (tester.handle == NULL) {
        HDF_LOGE("RtcTesterGet: open Rtc fail!");
        return NULL;
    }

    return &tester;
}

static void RtcTesterPut(struct RtcTester *tester)
{
    if (tester == NULL || tester->handle == NULL) {
        HDF_LOGE("RtcTesterPut: tester or handle is null!");
        return;
    }

    RtcClose(tester->handle);
    tester->handle = NULL;
}

static int8_t g_rtcIrqCallback = HDF_FAILURE;

static int32_t RtcAlarmACallback(enum RtcAlarmIndex alarmIndex)
{
    if (alarmIndex == RTC_ALARM_INDEX_A) {
        HDF_LOGI("RtcAlarmACallback: alarm a callback success!");
        g_rtcIrqCallback = HDF_SUCCESS;
    } else {
        g_rtcIrqCallback = HDF_FAILURE;
    }
    return 0;
}

static int32_t IsSameRtcTestTime(const struct RtcTime *readTime, const struct RtcTime *writeTime)
{
    if ((readTime->year != writeTime->year) || (readTime->month != writeTime->month) ||
        (readTime->day != writeTime->day) || (readTime->weekday != writeTime->weekday) ||
        (readTime->hour != writeTime->hour) || (readTime->minute != writeTime->minute) ||
        (readTime->second < writeTime->second)) {
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

static int32_t TestReadWriteTime(struct RtcTester *tester)
{
    int32_t ret;
    struct RtcTime readTime = {0};

    ret = RtcWriteTime(tester->handle, &tester->time);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("TestReadWriteTime: write time fail, ret: %d!", ret);
        return ret;
    }

    OsalMSleep(tester->config.writeWaitMillisecond);

    ret = RtcReadTime(tester->handle, &readTime);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("TestReadWriteTime: read time fail, ret: %d!", ret);
        return ret;
    }

    ret = IsSameRtcTestTime(&readTime, &tester->time);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("TestReadWriteTime: different time, ret: %d!", ret);
        return ret;
    }

    return HDF_SUCCESS;
}
static int32_t RtcReadWriteTimeTest(struct RtcTester *tester)
{
    int32_t ret;

    /* 2020-08-08 Saturday 09:08:08 .000 */
    tester->time.year = tester->config.year;
    tester->time.month = tester->config.month;
    tester->time.day = tester->config.day;
    tester->time.hour = tester->config.hour;
    tester->time.minute = tester->config.minute;
    tester->time.second = tester->config.second;
    tester->time.millisecond = 0;
    tester->time.weekday = RtcGetWeekDay(&tester->time);

    ret = TestReadWriteTime(tester);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("RtcReadWriteTimeTest: test ReadWriteTime fail, ret: %d!", ret);
        return ret;
    }

    return HDF_SUCCESS;
}

static int32_t RtcReadWriteMaxTimeTest(struct RtcTester *tester)
{
    int32_t ret;

    tester->time.year = tester->config.year;
    tester->time.month = tester->config.month + 1;
    tester->time.day = RTC_GREAT_MONTH_DAY + 1;
    tester->time.hour = RTC_DAY_HOURS;
    tester->time.minute = RTC_MAX_MINUTE;
    tester->time.second = RTC_MAX_SECOND;
    tester->time.millisecond = RTC_MAX_MS;
    tester->time.weekday = RtcGetWeekDay(&tester->time);

    ret = TestReadWriteTime(tester);
    if (ret == HDF_SUCCESS) {
        HDF_LOGE("RtcReadWriteMaxTimeTest: test fail!");
        return HDF_FAILURE;
    }

    return HDF_SUCCESS;
}

static int32_t RtcReadWriteMinTimeTest(struct RtcTester *tester)
{
    int32_t ret;

    tester->time.year = RTC_BEGIN_YEAR - 1;
    tester->time.month = 0;
    tester->time.day = 0;
    tester->time.hour = 0;
    tester->time.minute = 0;
    tester->time.second = 0;
    tester->time.millisecond = 0;
    tester->time.weekday = RtcGetWeekDay(&tester->time);

    ret = TestReadWriteTime(tester);
    if (ret == HDF_SUCCESS) {
        HDF_LOGE("RtcReadWriteMinTimeTest: test fail!");
        return HDF_FAILURE;
    }

    return HDF_SUCCESS;
}

static int32_t TestReadWriteAlarm(struct RtcTester *tester)
{
    int32_t ret;
    struct RtcTime readTime = {0};

    ret = RtcWriteAlarm(tester->handle, RTC_ALARM_INDEX_A, &tester->time);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("TestReadWriteAlarm: write alarm fail, ret: %d!", ret);
        return ret;
    }

    OsalMSleep(tester->config.writeWaitMillisecond);

    ret = RtcReadAlarm(tester->handle, RTC_ALARM_INDEX_A, &readTime);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("TestReadWriteAlarm: read alarm fail, ret: %d!", ret);
        return ret;
    }

    ret = IsSameRtcTestTime(&readTime, &tester->time);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("TestReadWriteAlarm: different time, ret: %d!", ret);
        return ret;
    }

    return HDF_SUCCESS;
}
static int32_t RtcReadWriteAlarmTimeTest(struct RtcTester *tester)
{
    int32_t ret;

    tester->time.year = tester->config.year;
    tester->time.month = tester->config.month;
    tester->time.day = tester->config.day;
    tester->time.hour = tester->config.hour + RTC_UNIT_DIFF;
    tester->time.minute = tester->config.minute;
    tester->time.second = tester->config.second;
    tester->time.millisecond = 0;
    tester->time.weekday = RtcGetWeekDay(&tester->time);

    ret = TestReadWriteAlarm(tester);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("RtcReadWriteAlarmTimeTest: test fail, ret: %d!", ret);
        return ret;
    }

    return HDF_SUCCESS;
}

static int32_t RtcReadWriteAlarmMaxTimeTest(struct RtcTester *tester)
{
    int32_t ret;

    tester->time.year = tester->config.maxYear;
    tester->time.month = RTC_MAX_MONTH + 1;
    tester->time.day = RTC_GREAT_MONTH_DAY + 1;
    tester->time.hour = RTC_DAY_HOURS;
    tester->time.minute = RTC_MAX_MINUTE;
    tester->time.second = RTC_MAX_SECOND;
    tester->time.millisecond = RTC_MAX_MS;
    tester->time.weekday = RtcGetWeekDay(&tester->time);

    ret = TestReadWriteAlarm(tester);
    if (ret == HDF_SUCCESS) {
        HDF_LOGE("RtcReadWriteAlarmMaxTimeTest: test fail!");
        return HDF_FAILURE;
    }

    return HDF_SUCCESS;
}

static int32_t RtcReadWriteAlarmMinTimeTest(struct RtcTester *tester)
{
    int32_t ret;

    tester->time.year = RTC_BEGIN_YEAR - 1;
    tester->time.month = 0;
    tester->time.day = 0;
    tester->time.hour = 0;
    tester->time.minute = 0;
    tester->time.second = 0;
    tester->time.millisecond = 0;
    tester->time.weekday = RtcGetWeekDay(&tester->time);

    ret = TestReadWriteAlarm(tester);
    if (ret == HDF_SUCCESS) {
        HDF_LOGE("RtcReadWriteAlarmMinTimeTest: test fail!");
        return HDF_FAILURE;
    }

    return HDF_SUCCESS;
}

static int32_t RtcAlarmEnableTest(struct RtcTester *tester)
{
    int32_t ret;

    /* 2020-08-08 Saturday 08:09:08 .000 */
    tester->time.year = tester->config.year;
    tester->time.month = tester->config.month;
    tester->time.day = tester->config.day;
    tester->time.hour = tester->config.hour;
    tester->time.minute = tester->config.minute + RTC_UNIT_DIFF;
    tester->time.second = tester->config.second;
    tester->time.millisecond = 0;
    tester->time.weekday = RtcGetWeekDay(&tester->time);

    ret = TestReadWriteAlarm(tester);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("RtcAlarmEnableTest: read write alarm fail, ret: %d!", ret);
        return ret;
    }

    ret = RtcAlarmInterruptEnable(tester->handle, RTC_ALARM_INDEX_A, 1);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("RtcAlarmEnableTest: alarm interrupt enable fail, ret: %d!", ret);
        return ret;
    }

    return HDF_SUCCESS;
}

static int32_t RtcAlarmIrqAttachConfig(struct RtcTester *tester)
{
    int32_t ret;
    uint32_t freq;

    ret = RtcRegisterAlarmCallback(tester->handle, RTC_ALARM_INDEX_A, RtcAlarmACallback);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("RtcAlarmIrqAttachConfig: register alarm callback fail, ret: %d!", ret);
        return ret;
    }

    freq = tester->config.frequency;
    ret = RtcSetFreq(tester->handle, freq);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("RtcAlarmIrqAttachConfig: set freq fail, ret: %d!", ret);
        return ret;
    }

    ret = RtcAlarmInterruptEnable(tester->handle, RTC_ALARM_INDEX_A, 1);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("RtcAlarmIrqAttachConfig: alarm interrupt enable fail, ret: %d!", ret);
        return ret;
    }

    return HDF_SUCCESS;
}

static int32_t RtcAlarmIrqTest(struct RtcTester *tester)
{
    int32_t ret;

    /* set time 2020-08-08 Saturday 08:08:08 .000 */
    tester->time.year = tester->config.year;
    tester->time.month = tester->config.month;
    tester->time.day = tester->config.day;
    tester->time.hour = tester->config.hour;
    tester->time.minute = tester->config.minute;
    tester->time.second = tester->config.second;
    tester->time.millisecond = 0;
    tester->time.weekday = RtcGetWeekDay(&tester->time);

    ret = RtcAlarmIrqAttachConfig(tester);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("RtcAlarmIrqTest: alarm irq attach config fail, ret: %d!", ret);
        return ret;
    }

    ret = RtcWriteTime(tester->handle, &tester->time);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("RtcAlarmIrqTest: write time fail, ret: %d!", ret);
        return ret;
    }

    /* set alarm time 2020-08-08 Saturday 08:08:09 .000 */
    tester->time.second = tester->config.second + 1;
    ret = RtcWriteAlarm(tester->handle, RTC_ALARM_INDEX_A, &tester->time);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("RtcAlarmIrqTest: write alarm fail, ret: %d!", ret);
        return ret;
    }

    OsalSleep(tester->config.writeWaitMillisecond);
    if (g_rtcIrqCallback == HDF_FAILURE) {
        HDF_LOGE("RtcAlarmIrqTest: rtc irq call back fail!");
        return HDF_FAILURE;
    }
    g_rtcIrqCallback = HDF_FAILURE;
    return HDF_SUCCESS;
}

static int32_t RtcRegisterCallbackTest(struct RtcTester *tester)
{
    int32_t ret;

    ret = RtcRegisterAlarmCallback(tester->handle, RTC_ALARM_INDEX_A, RtcAlarmACallback);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("RtcRegisterCallbackTest: register alarm callback fail, ret: %d!", ret);
        return ret;
    }

    return HDF_SUCCESS;
}

static int32_t RtcRegisterNullCallbackTest(struct RtcTester *tester)
{
    int32_t ret;

    ret = RtcRegisterAlarmCallback(tester->handle, RTC_ALARM_INDEX_A, NULL);
    if (ret == HDF_SUCCESS) {
        HDF_LOGE("RtcRegisterNullCallbackTest: register alarm callback fail!");
        return HDF_FAILURE;
    }

    return HDF_SUCCESS;
}

static int32_t RtcSetNormalFreqTest(struct RtcTester *tester)
{
    int32_t ret;

    ret = RtcSetFreq(tester->handle, tester->config.frequency);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("RtcSetNormalFreqTest: set normal frequency fail, ret: %d!", ret);
        return ret;
    }

    return HDF_SUCCESS;
}

static int32_t RtcSetMaxFreqTest(struct RtcTester *tester)
{
    int32_t ret;

    ret = RtcSetFreq(tester->handle, tester->config.frequency * RTC_TIME_UNIT);
    if (ret == HDF_SUCCESS) {
        HDF_LOGE("RtcSetMaxFreqTest: set max frequency test fail!");
        return HDF_FAILURE;
    }

    return HDF_SUCCESS;
}

static int32_t RtcSetMinFreqTest(struct RtcTester *tester)
{
    int32_t ret;

    ret = RtcSetFreq(tester->handle, 0);
    if (ret == HDF_SUCCESS) {
        HDF_LOGE("RtcSetMinFreqTest: set min frequency test fail!");
        return HDF_FAILURE;
    }

    ret = RtcSetFreq(tester->handle, tester->config.frequency);

    return ret;
}

static int32_t RtcReadWriteUserRegTest(struct RtcTester *tester)
{
    int32_t ret;
    uint8_t value;

    value = tester->config.userValue;
    ret = RtcWriteReg(tester->handle, 0, value);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("RtcReadWriteUserRegTest: write reg fail, ret: %d!", ret);
        return ret;
    }

    ret = RtcReadReg(tester->handle, 0, &value);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("RtcReadWriteUserRegTest: read reg fail, ret :%d!", ret);
        return ret;
    }

    if (value != tester->config.userValue) {
        HDF_LOGE("RtcReadWriteUserRegTest: value is not same. value :%hhu, userValue :%u!", value,
            tester->config.userValue);
        return HDF_FAILURE;
    }

    return HDF_SUCCESS;
}

static int32_t RtcReadWriteMaxUserIndexTest(struct RtcTester *tester)
{
    int32_t ret;
    uint8_t value;

    value = tester->config.userValue;
    ret = RtcWriteReg(tester->handle, tester->config.userMaxIndex, value);
    if (ret != HDF_SUCCESS) {
        HDF_LOGI("RtcReadWriteMaxUserIndexTest: write reg max index test success!");
        return HDF_SUCCESS;
    }

    ret = RtcReadReg(tester->handle, tester->config.userMaxIndex, &value);
    if (ret != HDF_SUCCESS) {
        HDF_LOGI("RtcReadWriteMaxUserIndexTest: read reg max index test success!");
        return HDF_SUCCESS;
    }

    HDF_LOGE("RtcReadWriteMaxUserIndexTest: read write reg max index test fail!");
    return HDF_FAILURE;
}

static int32_t RtcTestSample(struct RtcTester *tester)
{
    int32_t ret;
    uint32_t freq;
    struct RtcTime readTime = {0};

    ret = RtcRegisterAlarmCallback(tester->handle, RTC_ALARM_INDEX_A, RtcAlarmACallback);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("RtcTestSample: register alarm callback fail, ret: %d!", ret);
        return ret;
    }

    freq = tester->config.frequency;
    ret = RtcSetFreq(tester->handle, freq);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("RtcTestSample: set frequency fail, ret: %d!", ret);
        return ret;
    }

    ret = RtcAlarmInterruptEnable(tester->handle, RTC_ALARM_INDEX_A, 1);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("RtcTestSample: alarm interrupt enable fail, ret: %d!", ret);
        return ret;
    }

    tester->time.year = tester->config.year;
    tester->time.month = RTC_JANUARY;
    tester->time.day = tester->config.day;
    tester->time.hour = 0;
    tester->time.minute = 0;
    tester->time.second = 0;
    tester->time.millisecond = 0;

    ret = RtcWriteTime(tester->handle, &tester->time);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("RtcTestSample: write time fail, ret: %d!", ret);
        return ret;
    }

    tester->time.second = tester->config.second;
    ret = RtcWriteAlarm(tester->handle, RTC_ALARM_INDEX_A, &tester->time);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("RtcTestSample: write alarm fail, ret: %d!", ret);
        return ret;
    }

    OsalMSleep(tester->config.writeWaitMillisecond);

    ret = RtcReadAlarm(tester->handle, RTC_ALARM_INDEX_A, &readTime);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("RtcTestSample: read alarm fail, ret: %d!", ret);
        return ret;
    }

    ret = RtcReadTime(tester->handle, &tester->time);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("RtcTestSample: read time fail, ret: %d!", ret);
        return ret;
    }

    return HDF_SUCCESS;
}

static int32_t RtcReadWriteReliability(struct RtcTester *tester)
{
    uint32_t i;

    for (i = 0; i < tester->config.time; i++) {
        (void)RtcReadWriteTimeTest(tester);
        (void)RtcReadWriteAlarmTimeTest(tester);
#ifdef __LITEOS__
        (void)RtcSetNormalFreqTest(tester);
#endif
    }

    return HDF_SUCCESS;
}

static int32_t RtcMiniSetGetTimeZoneTest(struct RtcTester *tester)
{
#ifdef __KERNEL__
    int32_t timeZone = RTC_TEST_TIME_ZONE;
    int32_t ret;

    (void)tester;
    ret = RtcSetTimeZone(timeZone);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("RtcMiniSetGetTimeZoneTest: rtc set time zone fail, ret: %d!", ret);
        return ret;
    }

    HDF_LOGI("RtcMiniSetGetTimeZoneTest: rtc set time zone test done, then test rtc get time zone!");

    ret = RtcGetTimeZone(&timeZone);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("RtcMiniSetGetTimeZoneTest: rtc get time zone fail, ret: %d!", ret);
        return ret;
    }
#else
    (void)tester;
#endif
    HDF_LOGI("RtcMiniSetGetTimeZoneTest: all test done!");
    return HDF_SUCCESS;
}

struct RtcTestEntry {
    int cmd;
    int32_t (*func)(struct RtcTester *tester);
    const char *name;
};

// add test case entry
static struct RtcTestEntry g_entry[] = {
    {RTC_TEST_CMD_WR_TIME,                    RtcReadWriteTimeTest,         "RtcReadWriteTimeTest"        },
    {RTC_TEST_CMD_WR_MAX_TIME,                RtcReadWriteMaxTimeTest,      "RtcReadWriteMaxTimeTest"     },
    {RTC_TEST_CMD_WR_MIN_TIME,                RtcReadWriteMinTimeTest,      "RtcReadWriteMinTimeTest"     },
    {RTC_TEST_CMD_WR_ALARM_TIME,              RtcReadWriteAlarmTimeTest,    "RtcReadWriteAlarmTimeTest"   },
    {RTC_TEST_CMD_WR_ALARM_MAX_TIME,          RtcReadWriteAlarmMaxTimeTest, "RtcReadWriteAlarmMaxTimeTest"},
    {RTC_TEST_CMD_WR_ALARM_MIN_TIME,          RtcReadWriteAlarmMinTimeTest, "RtcReadWriteAlarmMinTimeTest"},
    {RTC_TEST_CMD_RTC_ALARM_ENABLE,           RtcAlarmEnableTest,           "RtcAlarmEnableTest"          },
    {RTC_TEST_CMD_RTC_ALARM_IRQ,              RtcAlarmIrqTest,              "RtcAlarmIrqTest"             },
    {RTC_TEST_CMD_RTC_REGISTER_CALLBACK,      RtcRegisterCallbackTest,      "RtcRegisterCallbackTest"     },
    {RTC_TEST_CMD_RTC_REGISTER_CALLBACK_NULL, RtcRegisterNullCallbackTest,  "RtcRegisterNullCallbackTest" },
    {RTC_TEST_CMD_RTC_WR_FREQ,                RtcSetNormalFreqTest,         "RtcSetNormalFreqTest"        },
    {RTC_TEST_CMD_RTC_WR_MAX_FREQ,            RtcSetMaxFreqTest,            "RtcSetMaxFreqTest"           },
    {RTC_TEST_CMD_RTC_WR_MIN_FREQ,            RtcSetMinFreqTest,            "RtcSetMinFreqTest"           },
    {RTC_TEST_CMD_RTC_WR_USER_REG,            RtcReadWriteUserRegTest,      "RtcReadWriteUserRegTest"     },
    {RTC_TEST_CMD_RTC_WR_USER_REG_MAX_INDEX,  RtcReadWriteMaxUserIndexTest, "RtcReadWriteMaxUserIndexTest"},
    {RTC_TEST_CMD_RTC_FUNCTION_TEST,          RtcTestSample,                "RtcTestSample"               },
    {RTC_TEST_CMD_RTC_WR_RELIABILITY,         RtcReadWriteReliability,      "RtcReadWriteReliability"     },
    {RTC_MINI_SET_GET_TIME_ZONE_TEST,         RtcMiniSetGetTimeZoneTest,    "RtcMiniSetGetTimeZoneTest"   },
};

int32_t RtcTestExecute(int cmd)
{
    uint32_t i;
    struct RtcTester *tester = NULL;
    int32_t ret = HDF_ERR_NOT_SUPPORT;

    tester = RtcTesterGet();
    if (tester == NULL) {
        HDF_LOGE("RtcTestExecute: tester is null!");
        return HDF_ERR_INVALID_OBJECT;
    }

    if (cmd > RTC_TEST_CMD_MAX) {
        HDF_LOGE("RtcTestExecute: invalid cmd: %d!", cmd);
        ret = HDF_ERR_NOT_SUPPORT;
        goto EXIT;
    }

    for (i = 0; i < sizeof(g_entry) / sizeof(g_entry[0]); i++) {
        if (g_entry[i].cmd != cmd || g_entry[i].func == NULL) {
            continue;
        }
        ret = g_entry[i].func(tester);
        break;
    }

EXIT:
    HDF_LOGE("[RtcTestExecute][======cmd:%d====ret:%d======]", cmd, ret);
    RtcTesterPut(tester);
    return ret;
}
