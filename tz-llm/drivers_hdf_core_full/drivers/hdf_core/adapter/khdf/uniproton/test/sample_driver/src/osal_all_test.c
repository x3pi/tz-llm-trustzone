/*
 * Copyright (c) 2022-2023 Huawei Device Co., Ltd. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this list of
 *    conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice, this list
 *    of conditions and the following disclaimer in the documentation and/or other materials
 *    provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its contributors may be used
 *    to endorse or promote products derived from this software without specific prior written
 *    permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "hdf_base.h"
#include "osal_test_case_def.h"
#include "osal_mem.h"
#include "osal_mutex.h"
#include "osal_spinlock.h"
#ifndef __USER__
#include "osal_test_type.h"
#endif
#include <stdio.h>
#include "osal_thread.h"
#include "osal_time.h"
#include "osal_timer.h"
#include "securec.h"
#include "osal_all_test.h"

#define IRQ_NUM_TEST 33
#define HDF_LOG_TAG osal_test

#define OSAL_TEST_TIME_DEFAULT 100

bool g_testEndFlag = false;
uint32_t g_osalTestCases[OSAL_TEST_CASE_CNT];

OsalTimespec g_hdfTestBegin;
OsalTimespec g_hdfTestEnd;
static int32_t g_waitMutexTime = 3100;
static bool g_threadTest1Flag = true;
OSAL_DECLARE_THREAD(g_thread1);
OSAL_DECLARE_THREAD(g_thread2);
OSAL_DECLARE_THREAD(g_thread);
struct OsalMutex g_mutexTest;
OSAL_DECLARE_SPINLOCK(g_spinTest);

#define HDF_THREAD_TEST_SLEEP_S 1
#define HDF_THREAD_TEST_SLEEP_US 600
#define HDF_THREAD_TEST_SLEEP_MS 300
#define HDF_THREAD_TEST_MUX_CNT 20
static bool g_thread1RunFlag;
static int32_t g_test1Para = 120;
static int32_t g_test2Para = 123;
#define TIME_RANGE 200000
#define NUMERATOR 15
#define DENOMINATOR 100
static int g_hdfTestCount = 0;
static int g_hdfTestSuccess = 0;

static bool OsalCheckTime(OsalTimespec *time, uint32_t ms)
{
    uint64_t t1 = time->sec * HDF_KILO_UNIT * HDF_KILO_UNIT + time->usec;
    uint64_t t2 = (uint64_t)ms * HDF_KILO_UNIT;
    uint64_t diff = (t1 < t2) ? (t2 - t1) : (t1 - t2);
    uint64_t timeRange = t2 / DENOMINATOR * NUMERATOR;
    timeRange = (timeRange < TIME_RANGE) ? TIME_RANGE : timeRange;

    return diff < timeRange;
}

static int ThreadTest1(void *arg)
{
    static int cnt = 0;

    HDF_LOGI("[OSAL_UT_TEST]%s test thread para end", __func__);
    (void)arg;

    g_thread1RunFlag = true;

    while (g_threadTest1Flag) {
        OsalSleep(HDF_THREAD_TEST_SLEEP_S);
        HDF_LOGI("%s %d", __func__, cnt);
        cnt++;
        if (cnt > HDF_THREAD_TEST_MUX_CNT) {
            g_waitMutexTime = HDF_WAIT_FOREVER;
        }
        int ret = OsalMutexTimedLock(&g_mutexTest, g_waitMutexTime);
        UT_TEST_CHECK_RET(ret == HDF_FAILURE, OSAL_MUTEX_LOCK_TIMEOUT);

        OsalMSleep(HDF_THREAD_TEST_SLEEP_MS);
        if (ret == HDF_SUCCESS) {
            ret = OsalMutexUnlock(&g_mutexTest);
            UT_TEST_CHECK_RET(ret != HDF_SUCCESS, OSAL_MUTEX_UNLOCK);
        }
        OsalMSleep(HDF_THREAD_TEST_SLEEP_US);

        ret = OsalSpinLock(&g_spinTest);
        UT_TEST_CHECK_RET(ret != HDF_SUCCESS, OSAL_SPIN_LOCK);
        ret = OsalSpinUnlock(&g_spinTest);
        UT_TEST_CHECK_RET(ret != HDF_SUCCESS, OSAL_SPIN_UNLOCK);

        OsalMSleep(HDF_THREAD_TEST_SLEEP_MS);
        if (cnt % HDF_THREAD_TEST_MUX_CNT == 0) {
            HDF_LOGI("%s ", __func__);
        }
        if (g_testEndFlag) {
            break;
        }
    }
    HDF_LOGI("%s thread return\n", __func__);
    return 0;
}

static bool g_thread2RunFlag;
static bool g_threadTest2Flag = true;
static int ThreadTest2(void *arg)
{
    static int cnt = 0;
    OsalTimespec hdfTs1 = { 0, 0 };
    OsalTimespec hdfTs2 = { 0, 0 };
    OsalTimespec hdfTsDiff = { 0, 0 };

    HDF_LOGI("[OSAL_UT_TEST]%s test thread para end", __func__);
    if (arg != NULL) {
        int32_t para = *(int32_t *)arg;
        UT_TEST_CHECK_RET(para != g_test2Para, OSAL_THREAD_PARA_CHECK);
    } else {
        UT_TEST_CHECK_RET(true, OSAL_THREAD_PARA_CHECK);
    }

    g_thread2RunFlag = true;

    while (g_threadTest2Flag) {
        OsalSleep(HDF_THREAD_TEST_SLEEP_S);
        OsalGetTime(&hdfTs1);
        HDF_LOGI("%s %d", __func__, cnt);

        cnt++;
        int ret = OsalMutexTimedLock(&g_mutexTest, g_waitMutexTime);
        UT_TEST_CHECK_RET(ret == HDF_FAILURE, OSAL_MUTEX_LOCK_TIMEOUT);

        OsalMSleep(HDF_THREAD_TEST_SLEEP_MS);
        if (ret == HDF_SUCCESS) {
            ret = OsalMutexUnlock(&g_mutexTest);
            UT_TEST_CHECK_RET(ret != HDF_SUCCESS, OSAL_MUTEX_UNLOCK);
        }

        OsalMSleep(HDF_THREAD_TEST_SLEEP_US);

        ret = OsalSpinLock(&g_spinTest);
        UT_TEST_CHECK_RET(ret != HDF_SUCCESS, OSAL_SPIN_LOCK);
        ret = OsalSpinUnlock(&g_spinTest);
        UT_TEST_CHECK_RET(ret != HDF_SUCCESS, OSAL_SPIN_UNLOCK);

        OsalMSleep(HDF_THREAD_TEST_SLEEP_MS);
        OsalGetTime(&hdfTs2);
        OsalDiffTime(&hdfTs1, &hdfTs2, &hdfTsDiff);
        if (cnt % HDF_THREAD_TEST_MUX_CNT == 0) {
            HDF_LOGI("%s %us %uus", __func__, (uint32_t)hdfTsDiff.sec, (uint32_t)hdfTsDiff.usec);
        }
        if (g_testEndFlag) {
            break;
        }
    }
    HDF_LOGI("%s thread return\n", __func__);
    return 0;
}

#define HDF_DBG_CNT_CTRL 10
#ifndef __USER__
OSAL_DECLARE_TIMER(g_testTimerLoop1);
OSAL_DECLARE_TIMER(g_testTimerLoop2);
OSAL_DECLARE_TIMER(g_testTimerOnce);
#define HDF_TIMER1_PERIOD 630
#define HDF_TIMER2_PERIOD 1350
static int32_t g_timerPeriod1 = HDF_TIMER1_PERIOD;
static int32_t g_timerPeriod2 = HDF_TIMER2_PERIOD;
static int32_t g_timerPeriod3 = 10300;
static int32_t g_timerPeriod1Modify = 1250;
static int32_t g_timerPeriod2Modify = 750;
#define HDF_TIMER_TEST_MODIFY 8
#define HDF_TEST_TIMER_PARA 1
#define HDF_TEST_TIMER_MODIFY 2
#define HDF_TEST_TIMER_END 3

static bool g_timerLoop1RunFlag;
static int g_timer1Cnt = 0;
static void TimerLoopTest1(int id, uintptr_t arg)
{
    int32_t para;
    OsalTimespec hdfTs1 = { 0, 0 };
    static OsalTimespec hdfTs2 = { 0, 0 };
    OsalTimespec hdfTsDiff = { 0, 0 };
    static int index = HDF_TEST_TIMER_PARA;

    if (g_timer1Cnt == 0) {
        OsalGetTime(&hdfTs2);
    }

    para = *(int32_t *)arg;
    if ((g_timer1Cnt >= 1) && (g_timer1Cnt != HDF_TIMER_TEST_MODIFY + 1)) {
        OsalGetTime(&hdfTs1);
        OsalDiffTime(&hdfTs2, &hdfTs1, &hdfTsDiff);
        HDF_LOGI("%s %d %d %d %d %d",
            __func__,
            g_timer1Cnt,
            para,
            (int32_t)hdfTsDiff.sec,
            (int32_t)hdfTsDiff.usec,
            g_timerPeriod1);
        UT_TEST_CHECK_RET(!OsalCheckTime(&hdfTsDiff, g_timerPeriod1), OSAL_TIMER_PERIOD_CHECK);
        UT_TEST_CHECK_RET(g_timerPeriod1 != para, OSAL_TIMER_PARA_CHECK);
        if (index == HDF_TEST_TIMER_PARA) {
            HDF_LOGI("[OSAL_UT_TEST]%s test timer para end", __func__);
            index = HDF_TEST_TIMER_END;
        }
        if (index == HDF_TEST_TIMER_MODIFY) {
            HDF_LOGI("[OSAL_UT_TEST]%s test timer modify function end", __func__);
            index = HDF_TEST_TIMER_END;
        }
    }

    if (g_timer1Cnt == HDF_TIMER_TEST_MODIFY) {
        g_timerPeriod1 = g_timerPeriod1Modify;
        int32_t ret = OsalTimerSetTimeout(&g_testTimerLoop1, g_timerPeriod1);
        UT_TEST_CHECK_RET(ret != HDF_SUCCESS, OSAL_TIMER_MODIFY_CHECK);
        index = HDF_TEST_TIMER_MODIFY;
    }

    OsalGetTime(&hdfTs2);

    g_timer1Cnt++;
    g_timerLoop1RunFlag = true;
}

static bool g_timerLoop2RunFlag;
static int g_timer2Cnt = 0;
static void TimerLoopTest2(int id, uintptr_t arg)
{
    int32_t para;
    OsalTimespec hdfTs1 = { 0, 0 };
    static OsalTimespec hdfTs2 = { 0, 0 };
    OsalTimespec hdfTsDiff = { 0, 0 };
    static int index = HDF_TEST_TIMER_PARA;

    if (g_timer2Cnt == 0) {
        OsalGetTime(&hdfTs2);
    }

    para = *(int32_t *)arg;
    if ((g_timer2Cnt >= 1) && (g_timer2Cnt != HDF_TIMER_TEST_MODIFY + 1)) {
        OsalGetTime(&hdfTs1);
        OsalDiffTime(&hdfTs2, &hdfTs1, &hdfTsDiff);
        HDF_LOGI("%s %d %d %d %d %d",
            __func__,
            g_timer2Cnt,
            para,
            (int32_t)hdfTsDiff.sec,
            (int32_t)hdfTsDiff.usec,
            g_timerPeriod2);

        UT_TEST_CHECK_RET(!OsalCheckTime(&hdfTsDiff, g_timerPeriod2), OSAL_TIMER_PERIOD_CHECK);
        UT_TEST_CHECK_RET(g_timerPeriod2 != para, OSAL_TIMER_PARA_CHECK);
        if (index == HDF_TEST_TIMER_PARA) {
            HDF_LOGI("[OSAL_UT_TEST]%s test timer para end", __func__);
            index = HDF_TEST_TIMER_END;
        }
        if (index == HDF_TEST_TIMER_MODIFY) {
            HDF_LOGI("[OSAL_UT_TEST]%s test timer modify function end", __func__);
            index = HDF_TEST_TIMER_END;
        }
    }

    if (g_timer2Cnt == HDF_TIMER_TEST_MODIFY) {
        g_timerPeriod2 = g_timerPeriod2Modify;
        HDF_LOGI("[OSAL_UT_TEST]%s modify timer", __func__);
        int32_t ret = OsalTimerSetTimeout(&g_testTimerLoop2, g_timerPeriod2);
        UT_TEST_CHECK_RET(ret != HDF_SUCCESS, OSAL_TIMER_MODIFY_CHECK);
        index = HDF_TEST_TIMER_MODIFY;
    }
    OsalGetTime(&hdfTs2);
    g_timer2Cnt++;
    g_timerLoop2RunFlag = true;
}

static int g_timerOnceRunFlag;
static void TimerOnceTest(int id, uintptr_t arg)
{
    int32_t para;
    para = *(int32_t *)arg;

    HDF_LOGI("%s %d", __func__, para);
    UT_TEST_CHECK_RET(para != g_timerPeriod3, OSAL_TIMER_PARA_CHECK);
    g_timerOnceRunFlag++;
}

static void OsaTimerTest(void)
{
    HDF_LOGI("entering OsaTimerTest.\r\n");
    int32_t ret;

    g_hdfTestCount++;
    HDF_LOGI("[OSAL_UT_TEST]%s start", __func__);
    g_timerPeriod1 = HDF_TIMER1_PERIOD;
    g_timerPeriod2 = HDF_TIMER2_PERIOD;

    ret = OsalTimerCreate(&g_testTimerLoop1, g_timerPeriod1, TimerLoopTest1, (uintptr_t)&g_timerPeriod1);
    UT_TEST_CHECK_RET(ret != HDF_SUCCESS, OSAL_TIMER_CREATE_LOOP);
    OsalTimerStartLoop(&g_testTimerLoop1);

    ret = OsalTimerCreate(&g_testTimerLoop2, g_timerPeriod2, TimerLoopTest2, (uintptr_t)&g_timerPeriod2);
    UT_TEST_CHECK_RET(ret != HDF_SUCCESS, OSAL_TIMER_CREATE_LOOP);
    OsalTimerStartLoop(&g_testTimerLoop2);

    ret = OsalTimerCreate(&g_testTimerOnce, g_timerPeriod3, TimerOnceTest, (uintptr_t)&g_timerPeriod3);
    UT_TEST_CHECK_RET(ret != HDF_SUCCESS, OSAL_TIMER_CREATE_ONCE);
    OsalTimerStartOnce(&g_testTimerOnce);

    g_hdfTestSuccess++;
    HDF_LOGI("[OSAL_UT_TEST]%s end", __func__);
    HDF_LOGI("finished OsaTimerTest.\r\n");
}

#define HDF_ONCE_TIMER_DEL_TIME 10
static void OsaTimerTestStop(void)
{
    int32_t ret;

    HDF_LOGI("[OSAL_UT_TEST]%s start", __func__);
    ret = OsalTimerDelete(&g_testTimerLoop2);
    UT_TEST_CHECK_RET(ret != HDF_SUCCESS, OSAL_TIMER_STOP_CHECK);
    g_timerLoop2RunFlag = false;

    ret = OsalTimerDelete(&g_testTimerOnce);
    UT_TEST_CHECK_RET(ret != HDF_SUCCESS, OSAL_TIMER_STOP_CHECK);

    ret = OsalTimerCreate(&g_testTimerOnce, g_timerPeriod3, TimerOnceTest, (uintptr_t)&g_timerPeriod3);
    UT_TEST_CHECK_RET(ret != HDF_SUCCESS, OSAL_TIMER_CREATE_ONCE);

    OsalTimerStartOnce(&g_testTimerOnce);
    HDF_LOGI("[OSAL_UT_TEST]%s OsalTimerStartOnce", __func__);
    OsalMSleep(HDF_ONCE_TIMER_DEL_TIME);
    ret = OsalTimerDelete(&g_testTimerOnce);
    UT_TEST_CHECK_RET(ret != HDF_SUCCESS, OSAL_TIMER_STOP_CHECK);

    HDF_LOGI("[OSAL_UT_TEST]%s end", __func__);
}

#define THREAD_TEST_TIMER_RUN 20
#define THREAD_TEST_TIMER_STOP 25

static void OsaCheckRun(int cnt)
{
    OsalTimespec diffTime = { 0, 0 };

    if (cnt == THREAD_TEST_TIMER_RUN) {
        UT_TEST_CHECK_RET(g_timerOnceRunFlag == 0, OSAL_TIMER_RUN_CHECK);
        UT_TEST_CHECK_RET(g_timerLoop2RunFlag == false, OSAL_TIMER_RUN_CHECK);
        UT_TEST_CHECK_RET(g_timerLoop1RunFlag == false, OSAL_TIMER_RUN_CHECK);
        HDF_LOGI("[OSAL_UT_TEST]%s timer run end", __func__);
    }

    if (cnt == THREAD_TEST_TIMER_STOP) {
        UT_TEST_CHECK_RET(g_timerOnceRunFlag != 1, OSAL_TIMER_STOP_CHECK);
        UT_TEST_CHECK_RET(g_timerLoop2RunFlag != false, OSAL_TIMER_STOP_CHECK);
        HDF_LOGI("[OSAL_UT_TEST]%s timer stop end", __func__);
    }
    if (cnt == THREAD_TEST_TIMER_STOP) {
        OsalGetTime(&g_hdfTestEnd);
        OsalDiffTime(&g_hdfTestBegin, &g_hdfTestEnd, &diffTime);
        HDF_LOGI("[OSAL_UT_TEST]%s **** All case test end, use %ds****", __func__, (uint32_t)diffTime.sec);
        HDF_LOGI("[OSAL_UT_TEST]%s ***************************", __func__);
    }
}

#define THREAD_TEST_STOP_TIMER 25
#endif

#define THREAD_TEST_DBG_CTRL 200
#define THREAD_TEST_MUX_BEGIN 3
#define THREAD_TEST_MUX_END 5
#define THREAD_TEST_SLEEP_MS 1
static int g_thread3RunFlag;
static int32_t g_threadTestFlag = true;
static int ThreadTest(void *arg)
{
    static int cnt = 0;
    HDF_STATUS ret;
    HDF_LOGI("in threadTest %s %d", __func__, __LINE__);

    g_thread3RunFlag = true;
    (void)arg;

    while (g_threadTestFlag) {
        OsalSleep(THREAD_TEST_SLEEP_MS);

        if (cnt < THREAD_TEST_DBG_CTRL && cnt % HDF_DBG_CNT_CTRL == 0) {
            HDF_LOGI("in threadTest %d", cnt);
        }
        cnt++;

        if (cnt == THREAD_TEST_MUX_BEGIN) {
            HDF_LOGI("%s mutex Lock", __func__);
            ret = OsalMutexTimedLock(&g_mutexTest, g_waitMutexTime);
            UT_TEST_CHECK_RET(ret != HDF_SUCCESS, OSAL_MUTEX_STRESS_TEST);
        }
        if (cnt == THREAD_TEST_MUX_END) {
            ret = OsalMutexUnlock(&g_mutexTest);
            UT_TEST_CHECK_RET(ret != HDF_SUCCESS, OSAL_MUTEX_STRESS_TEST);
            HDF_LOGI("%s mutex unLock", __func__);
        }
#ifndef __USER__
        if (cnt == THREAD_TEST_STOP_TIMER) {
            OsaTimerTestStop();
        }
        OsaCheckRun(cnt);
#endif
        if (g_testEndFlag) {
            break;
        }
    }
    HDF_LOGI("%s thread return\n", __func__);
    return 0;
}

#define HDF_TEST_STACK_SIZE 8000
static void OsaThreadTest1(void)
{
    HDF_LOGI("entering OsaThreadTest1.\r\n");
    struct OsalThreadParam threadCfg;
    static int para = 120;
    int32_t ret;
    g_hdfTestCount++;

    (void)memset_s(&threadCfg, sizeof(threadCfg), 0, sizeof(threadCfg));
    threadCfg.name = "hdf_test0";
    threadCfg.priority = OSAL_THREAD_PRI_HIGHEST;
    threadCfg.stackSize = HDF_TEST_STACK_SIZE;
    ret = OsalThreadCreate(&g_thread, (OsalThreadEntry)ThreadTest, (void *)&para);
    UT_TEST_CHECK_RET(ret != HDF_SUCCESS, OSAL_THREAD_CREATE);
    ret = OsalThreadStart(&g_thread, &threadCfg);
    UT_TEST_CHECK_RET(ret != HDF_SUCCESS, OSAL_THREAD_CREATE);
    g_hdfTestSuccess++;
    HDF_LOGI("finished OsaThreadTest1.\r\n");
}

static void OsaThreadTest(void)
{
    HDF_LOGI("entering OsaThreadTest.\r\n");
    struct OsalThreadParam threadCfg;
    int ret;
    g_hdfTestCount++;
    HDF_LOGI("[OSAL_UT_TEST]%s start", __func__);
    ret = OsalMutexInit(&g_mutexTest);
    UT_TEST_CHECK_RET(ret != HDF_SUCCESS, OSAL_MUTEX_CREATE);
    ret = OsalSpinInit(&g_spinTest);
    UT_TEST_CHECK_RET(ret != HDF_SUCCESS, OSAL_SPIN_CREATE);

    (void)memset_s(&threadCfg, sizeof(threadCfg), 0, sizeof(threadCfg));
    threadCfg.name = "hdf_test1";
    threadCfg.priority = OSAL_THREAD_PRI_HIGH;
    threadCfg.stackSize = HDF_TEST_STACK_SIZE;
    ret = OsalThreadCreate(&g_thread1, (OsalThreadEntry)ThreadTest1, (void *)&g_test1Para);
    UT_TEST_CHECK_RET(ret != HDF_SUCCESS, OSAL_THREAD_CREATE);
    ret = OsalThreadStart(&g_thread1, &threadCfg);
    UT_TEST_CHECK_RET(ret != HDF_SUCCESS, OSAL_THREAD_CREATE);

    OsalMSleep(HDF_THREAD_TEST_SLEEP_S);

    (void)memset_s(&threadCfg, sizeof(threadCfg), 0, sizeof(threadCfg));
    threadCfg.name = "hdf_test2";
    threadCfg.priority = OSAL_THREAD_PRI_DEFAULT;
    threadCfg.stackSize = HDF_TEST_STACK_SIZE;
    ret = OsalThreadCreate(&g_thread2, (OsalThreadEntry)ThreadTest2, (void *)&g_test2Para);
    UT_TEST_CHECK_RET(ret != HDF_SUCCESS, OSAL_THREAD_CREATE);
    ret = OsalThreadStart(&g_thread2, &threadCfg);
    UT_TEST_CHECK_RET(ret != HDF_SUCCESS, OSAL_THREAD_CREATE);

    OsaThreadTest1();

    g_hdfTestSuccess++;
    HDF_LOGI("[OSAL_UT_TEST]%s end", __func__);
    HDF_LOGI("finished OsaThreadTest.\r\n");
}

#define TIME_TEST_SLEEP_S 5
#define TIME_TEST_SLEEP_MS 200
#define TIME_TEST_SLEEP_US 200
#define TIME_TEST_MS_RANGE_H 210
#define TIME_TEST_MS_RANGE_L 190
#define TIME_TEST_US_RANGE_H 1000
#define TIME_TEST_US_RANGE_L 200
static void OsaTimeTest(void)
{
    HDF_LOGI("entering OsaTimeTest.\r\n");
    g_hdfTestCount++;
    int32_t ret;
    OsalTimespec hdfTs = { 0, 0 };
    OsalTimespec hdfTs2 = { 0, 0 };
    OsalTimespec hdfTsDiff = { 0, 0 };

    OsalGetTime(&hdfTs);
    OsalSleep(TIME_TEST_SLEEP_S);
    ret = OsalGetTime(&hdfTs2);
    UT_TEST_CHECK_RET(ret != HDF_SUCCESS, OSAL_TIME_GETTIME);
    ret = OsalDiffTime(&hdfTs, &hdfTs2, &hdfTsDiff);
    HDF_LOGI("%s %us %uus", __func__, (uint32_t)hdfTsDiff.sec, (uint32_t)hdfTsDiff.usec);
    UT_TEST_CHECK_RET(ret != HDF_SUCCESS, OSAL_TIME_DIFFTIME);
    UT_TEST_CHECK_RET(!OsalCheckTime(&hdfTsDiff, TIME_TEST_SLEEP_S * HDF_KILO_UNIT), OSAL_TIME_DIFFTIME);

    OsalGetTime(&hdfTs);
    OsalMSleep(TIME_TEST_SLEEP_MS);
    ret = OsalGetTime(&hdfTs2);
    UT_TEST_CHECK_RET(ret != HDF_SUCCESS, OSAL_TIME_GETTIME);
    ret = OsalDiffTime(&hdfTs, &hdfTs2, &hdfTsDiff);
    HDF_LOGI("%s %us %uus", __func__, (uint32_t)hdfTsDiff.sec, (uint32_t)hdfTsDiff.usec);
    UT_TEST_CHECK_RET(ret != HDF_SUCCESS, OSAL_TIME_DIFFTIME);
    UT_TEST_CHECK_RET(!OsalCheckTime(&hdfTsDiff, TIME_TEST_SLEEP_MS), OSAL_TIME_DIFFTIME);
    g_hdfTestSuccess++;
    HDF_LOGI("finished OsaTimeTest.\r\n");
}
enum {
    MEM_ALIGN_TEST_0 = 1,
    MEM_ALIGN_TEST_1 = 2,
    MEM_ALIGN_TEST_2 = 3,
    MEM_ALIGN_TEST_3 = 4,
    MEM_ALIGN_TEST_4 = 7,
    MEM_ALIGN_TEST_5 = 8,
    MEM_ALIGN_TEST_6 = 16,
};
#define MALLOC_TEST_CASE1_SIZE 0x100
#define MALLOC_TEST_CASE2_SIZE 0x1FF
#define MALLOC_TEST_CNT 1000
static void OsaMemoryTest(void)
{
    void *buf = NULL;
    int i = 0;
    g_hdfTestCount++;
    HDF_LOGI("[OSAL_UT_TEST]%s start", __func__);
    buf = OsalMemCalloc(MALLOC_TEST_CASE1_SIZE);
    UT_TEST_CHECK_RET(buf == NULL, OSAL_MALLOC_BIG);
    OsalMemFree(buf);

    buf = OsalMemCalloc(MALLOC_TEST_CASE2_SIZE);
    UT_TEST_CHECK_RET(buf == NULL, OSAL_MALLOC_SMALL);
    OsalMemFree(buf);

    buf = OsalMemAllocAlign(MEM_ALIGN_TEST_0, MALLOC_TEST_CASE1_SIZE);
    UT_TEST_CHECK_RET(buf != NULL, OSAL_MALLOC_BIG);
    OsalMemFree(buf);
    buf = OsalMemAllocAlign(MEM_ALIGN_TEST_1, MALLOC_TEST_CASE1_SIZE);
    UT_TEST_CHECK_RET(buf != NULL, OSAL_MALLOC_BIG);
    OsalMemFree(buf);
    buf = OsalMemAllocAlign(MEM_ALIGN_TEST_2, MALLOC_TEST_CASE1_SIZE);
    UT_TEST_CHECK_RET(buf != NULL, OSAL_MALLOC_BIG);
    OsalMemFree(buf);
    buf = OsalMemAllocAlign(MEM_ALIGN_TEST_3, MALLOC_TEST_CASE1_SIZE);
    if (sizeof(void *) == MEM_ALIGN_TEST_3) {
        UT_TEST_CHECK_RET(buf == NULL, OSAL_MALLOC_BIG);
        UT_TEST_CHECK_RET(((int)buf % MEM_ALIGN_TEST_3) != 0, OSAL_MALLOC_BIG);
    } else {
        UT_TEST_CHECK_RET(buf != NULL, OSAL_MALLOC_BIG);
    }
    OsalMemFree(buf);
    buf = OsalMemAllocAlign(MEM_ALIGN_TEST_4, MALLOC_TEST_CASE1_SIZE);
    UT_TEST_CHECK_RET(buf != NULL, OSAL_MALLOC_BIG);
    OsalMemFree(buf);
    buf = OsalMemAllocAlign(MEM_ALIGN_TEST_5, MALLOC_TEST_CASE1_SIZE);
    UT_TEST_CHECK_RET(buf == NULL, OSAL_MALLOC_BIG);
    UT_TEST_CHECK_RET(((int)buf % MEM_ALIGN_TEST_5) != 0, OSAL_MALLOC_BIG);
    OsalMemFree(buf);
    buf = OsalMemAllocAlign(MEM_ALIGN_TEST_6, MALLOC_TEST_CASE1_SIZE);
    UT_TEST_CHECK_RET(buf == NULL, OSAL_MALLOC_BIG);
    UT_TEST_CHECK_RET(((int)buf % MEM_ALIGN_TEST_6) != 0, OSAL_MALLOC_BIG);
    OsalMemFree(buf);

    for (; i < MALLOC_TEST_CNT; i++) {
        buf = OsalMemCalloc(MALLOC_TEST_CASE1_SIZE);
        UT_TEST_CHECK_RET(buf == NULL, OSAL_MALLOC_BIG_STRESS);
        OsalMemFree(buf);

        buf = OsalMemCalloc(MALLOC_TEST_CASE2_SIZE);
        UT_TEST_CHECK_RET(buf == NULL, OSAL_MALLOC_SMALL_STRESS);
        OsalMemFree(buf);
    }
    g_hdfTestSuccess++;
    HDF_LOGI("[OSAL_UT_TEST]%s end", __func__);
}

static void OsaMutexTest(void)
{
    HDF_LOGI("entering OsaMutexTest.\r\n");
    HDF_STATUS ret;
    struct OsalMutex mutex;

    g_hdfTestCount++;
    HDF_LOGI("[OSAL_UT_TEST]%s start", __func__);

    (void)memset_s(&mutex, sizeof(mutex), 0, sizeof(mutex));
    ret = OsalMutexInit(&mutex);
    UT_TEST_CHECK_RET(ret != HDF_SUCCESS, OSAL_MUTEX_CREATE);
    ret = OsalMutexTimedLock(&mutex, OSAL_TEST_TIME_DEFAULT);
    UT_TEST_CHECK_RET(ret != HDF_SUCCESS, OSAL_MUTEX_LOCK_TIMEOUT);
    ret = OsalMutexUnlock(&mutex);
    UT_TEST_CHECK_RET(ret != HDF_SUCCESS, OSAL_MUTEX_UNLOCK);
    ret = OsalMutexLock(&mutex);
    UT_TEST_CHECK_RET(ret != HDF_SUCCESS, OSAL_MUTEX_LOCK_FOREVER);
    ret = OsalMutexUnlock(&mutex);
    UT_TEST_CHECK_RET(ret != HDF_SUCCESS, OSAL_MUTEX_UNLOCK);
    ret = OsalMutexDestroy(&mutex);
    UT_TEST_CHECK_RET(ret != HDF_SUCCESS, OSAL_MUTEX_DESTROY);

    HDF_LOGI("[OSAL_UT_TEST]%s end", __func__);

    g_hdfTestSuccess++;
    HDF_LOGI("finished OsaMutexTest.\r\n");
}

static void OsaSpinTest(void)
{
    HDF_LOGI("entering OsaSpinTest.\r\n");
    int32_t ret;
#ifndef __USER__
    uint32_t flag = 0;
#endif
    g_hdfTestCount++;
    OSAL_DECLARE_SPINLOCK(spin);

    HDF_LOGI("[OSAL_UT_TEST]%s start", __func__);
    (void)memset_s(&spin, sizeof(spin), 0, sizeof(spin));
    ret = OsalSpinInit(&spin);
    UT_TEST_CHECK_RET(ret != HDF_SUCCESS, OSAL_SPIN_CREATE);

    ret = OsalSpinLock(&spin);
    UT_TEST_CHECK_RET(ret != HDF_SUCCESS, OSAL_SPIN_LOCK);
    ret = OsalSpinUnlock(&spin);
    UT_TEST_CHECK_RET(ret != HDF_SUCCESS, OSAL_SPIN_UNLOCK);
#ifndef __USER__
    ret = OsalSpinLockIrq(&spin);
    UT_TEST_CHECK_RET(ret != HDF_SUCCESS, OSAL_SPIN_LOCK_IRQ);
    ret = OsalSpinUnlockIrq(&spin);
    UT_TEST_CHECK_RET(ret != HDF_SUCCESS, OSAL_SPIN_UNLOCK_IRQ);

    ret = OsalSpinLockIrqSave(&spin, &flag);
    UT_TEST_CHECK_RET(ret != HDF_SUCCESS, OSAL_SPIN_LOCK_IRQ_SAVE);
    ret = OsalSpinUnlockIrqRestore(&spin, &flag);
    UT_TEST_CHECK_RET(ret != HDF_SUCCESS, OSAL_SPIN_UNLOCK_IRQ_RESTORE);
#endif
    ret = OsalSpinDestroy(&spin);
    UT_TEST_CHECK_RET(ret != HDF_SUCCESS, OSAL_SPIN_DESTROY);

    g_hdfTestSuccess++;
    HDF_LOGI("[OSAL_UT_TEST]%s end", __func__);
    HDF_LOGI("entering OsaSpinTest.\r\n");
}

static void OsaCheckThreadRun(void)
{
    UT_TEST_CHECK_RET(g_thread1RunFlag == 0, OSAL_THREAD_RUN_CHECK);
    UT_TEST_CHECK_RET(g_thread1RunFlag == 0, OSAL_THREAD_RUN_CHECK);
    UT_TEST_CHECK_RET(g_thread1RunFlag == 0, OSAL_THREAD_RUN_CHECK);
    HDF_LOGI("[OSAL_UT_TEST]%s end", __func__);
}

#define HDF_MAIN_SLEEP_S 2
int OsaTestBegin(void)
{
    (void)memset_s(g_osalTestCases, sizeof(g_osalTestCases), 0, sizeof(g_osalTestCases));
    g_testEndFlag = false;
#ifndef __USER__
    g_timer1Cnt = 0;
    g_timer2Cnt = 0;
#endif
    OsalGetTime(&g_hdfTestBegin);
    printf("OsaTestBegin start!!!\n");

    OsaTimeTest();
    OsaMutexTest();
    OsaSpinTest();
    OsaTimerTest();
    OsaThreadTest();
    OsaMemoryTest();
    HDF_LOGI("%s ", __func__);
    OsalSleep(HDF_MAIN_SLEEP_S);
    HDF_LOGI("%s", __func__);
    OsaCheckThreadRun();
    return 0;
}

int OsaTestEnd(void)
{
#ifndef __USER__
    OsalTimerDelete(&g_testTimerLoop1);
    OsalTimerDelete(&g_testTimerLoop2);
    OsalTimerDelete(&g_testTimerOnce);
#endif
    g_testEndFlag = true;
    OsalThreadDestroy(&g_thread1);
    OsalThreadDestroy(&g_thread);
    OsalThreadDestroy(&g_thread2);
    HDF_LOGI("%s", __func__);

    return 0;
}

int OsaTestALLResult(void)
{
    int index;
    for (index = 0; index < OSAL_TEST_CASE_CNT; index++) {
        if (g_osalTestCases[index] != 0) {
            HDF_LOGE("%s %d %d", __func__, g_osalTestCases[index], index);
            return 1;
        }
    }
    HDF_LOGI("[OSAL_UT_TEST] TestCase: %d Successful: %d Failed: %d",
        g_hdfTestCount,
        g_hdfTestSuccess,
        g_hdfTestCount - g_hdfTestSuccess);
    return 0;
}
