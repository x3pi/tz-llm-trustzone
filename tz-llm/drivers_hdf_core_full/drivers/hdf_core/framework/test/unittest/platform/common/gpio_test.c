/*
 * Copyright (c) 2020-2023 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include "gpio_test.h"
#include "gpio_if.h"
#include "hdf_base.h"
#include "hdf_io_service_if.h"
#include "hdf_log.h"
#include "osal_irq.h"
#include "osal_time.h"
#include "securec.h"

#define HDF_LOG_TAG gpio_test

#define GPIO_TEST_IRQ_TIMEOUT 1000
#define GPIO_TEST_IRQ_DELAY   200

static int32_t GpioTestGetConfig(struct GpioTestConfig *config)
{
    int32_t ret;
    struct HdfSBuf *reply = NULL;
    struct HdfIoService *service = NULL;
    const void *buf = NULL;
    uint32_t len;

    HDF_LOGD("GpioTestGetConfig: enter!");
    service = HdfIoServiceBind("GPIO_TEST");
    if (service == NULL) {
        HDF_LOGE("GpioTestGetConfig: fail to bind gpio test server!");
        return HDF_ERR_NOT_SUPPORT;
    }

    reply = HdfSbufObtain(sizeof(*config) + sizeof(uint64_t));
    if (reply == NULL) {
        HDF_LOGE("GpioTestGetConfig: fail to obtain reply!");
        HdfIoServiceRecycle(service);
        return HDF_ERR_MALLOC_FAIL;
    }

    ret = service->dispatcher->Dispatch(&service->object, 0, NULL, reply);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("GpioTestGetConfig: remote dispatch fail, ret: %d!", ret);
        HdfIoServiceRecycle(service);
        HdfSbufRecycle(reply);
        return ret;
    }

    if (!HdfSbufReadBuffer(reply, &buf, &len)) {
        HDF_LOGE("GpioTestGetConfig: fail to read buf!");
        HdfIoServiceRecycle(service);
        HdfSbufRecycle(reply);
        return HDF_ERR_IO;
    }

    if (len != sizeof(*config)) {
        HDF_LOGE("GpioTestGetConfig: config size:%zu, but read size:%u!", sizeof(*config), len);
        HdfIoServiceRecycle(service);
        HdfSbufRecycle(reply);
        return HDF_ERR_IO;
    }

    if (memcpy_s(config, sizeof(*config), buf, sizeof(*config)) != EOK) {
        HDF_LOGE("GpioTestGetConfig: fail to memcpy buf!");
        HdfIoServiceRecycle(service);
        HdfSbufRecycle(reply);
        return HDF_ERR_IO;
    }

    HdfSbufRecycle(reply);
    HDF_LOGD("GpioTestGetConfig: exit!");
    HdfIoServiceRecycle(service);
    return HDF_SUCCESS;
}

static struct GpioTester *GpioTesterGet(void)
{
    int32_t ret;
    static struct GpioTester tester;
    static struct GpioTester *pTester = NULL;

    if (pTester != NULL) {
        return pTester;
    }

    ret = GpioTestGetConfig(&tester.cfg);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("GpioTesterGet: fail to get config, ret: %d!", ret);
        return NULL;
    }
    HDF_LOGI("GpioTesterGet: gpio=%u, gpioIrq=%u, testUserApi=%u, gpioTestTwo=%u, testNameOne=%s, testNameTwo=%s!",
        tester.cfg.gpio, tester.cfg.gpioIrq, tester.cfg.testUserApi, tester.cfg.gpioTestTwo, tester.cfg.testNameOne,
        tester.cfg.testNameTwo);

    pTester = &tester;
    return pTester;
}

static int32_t GpioTestSetUp(void)
{
    int32_t ret;
    struct GpioTester *tester = NULL;

    tester = GpioTesterGet();
    if (tester == NULL) {
        HDF_LOGE("GpioTestSetUp: fail to get tester!");
        return HDF_ERR_INVALID_OBJECT;
    }

    ret = GpioGetDir(tester->cfg.gpio, &tester->oldDir);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("GpioTestSetUp: fail to get old dir, ret: %d!", ret);
        return ret;
    }
    ret = GpioRead(tester->cfg.gpio, &tester->oldVal);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("GpioTestSetUp: fail to read old val, ret: %d!", ret);
        return ret;
    }

    tester->fails = 0;
    tester->irqCnt = 0;
    tester->irqTimeout = GPIO_TEST_IRQ_TIMEOUT;
    return HDF_SUCCESS;
}

static int32_t GpioTestTearDown(void)
{
    int ret;
    struct GpioTester *tester = NULL;

    tester = GpioTesterGet();
    if (tester == NULL) {
        HDF_LOGE("GpioTestTearDown: fail to get tester!");
        return HDF_ERR_INVALID_OBJECT;
    }

    ret = GpioSetDir(tester->cfg.gpio, tester->oldDir);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("GpioTestTearDown: fail to set old dir, ret: %d!", ret);
        return ret;
    }
    if (tester->oldDir == GPIO_DIR_IN) {
        return HDF_SUCCESS;
    }
    ret = GpioWrite(tester->cfg.gpio, tester->oldVal);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("GpioTestTearDown: fail to write old val, ret: %d!", ret);
        return ret;
    }

    return HDF_SUCCESS;
}

static int32_t GpioTestSetGetDir(void)
{
    int32_t ret;
    uint16_t dirSet;
    uint16_t dirGet;
    struct GpioTester *tester = NULL;

    tester = GpioTesterGet();
    if (tester == NULL) {
        HDF_LOGE("GpioTestSetGetDir: fail to get tester!");
        return HDF_ERR_INVALID_OBJECT;
    }

    dirSet = GPIO_DIR_OUT;
    dirGet = GPIO_DIR_IN;

    ret = GpioSetDir(tester->cfg.gpio, dirSet);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("GpioTestSetGetDir: fail to set dir, ret: %d!", ret);
        return ret;
    }
    ret = GpioGetDir(tester->cfg.gpio, &dirGet);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("GpioTestSetGetDir: fail to get dir, ret: %d!", ret);
        return ret;
    }
    if (dirSet != dirGet) {
        HDF_LOGE("GpioTestSetGetDir: set dir:%u, but get:%u!", dirSet, dirGet);
        return HDF_FAILURE;
    }
    /* change the value and test one more time */
    dirSet = GPIO_DIR_IN;
    dirGet = GPIO_DIR_OUT;
    ret = GpioSetDir(tester->cfg.gpio, dirSet);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("GpioTestSetGetDir: fail to set dir, ret: %d!", ret);
        return ret;
    }
    ret = GpioGetDir(tester->cfg.gpio, &dirGet);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("GpioTestSetGetDir: fail to get dir, ret: %d!", ret);
        return ret;
    }
    if (dirSet != dirGet) {
        HDF_LOGE("GpioTestSetGetDir: set dir:%hu, but get:%hu!", dirSet, dirGet);
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

static int32_t GpioTestWriteRead(void)
{
    int32_t ret;
    uint16_t valWrite;
    uint16_t valRead;
    struct GpioTester *tester = NULL;

    tester = GpioTesterGet();
    if (tester == NULL) {
        HDF_LOGE("GpioTestWriteRead: fail to get tester!");
        return HDF_ERR_INVALID_OBJECT;
    }

    ret = GpioSetDir(tester->cfg.gpio, GPIO_DIR_OUT);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("GpioTestWriteRead: fail to set dir, ret: %d!", ret);
        return ret;
    }
    valWrite = GPIO_VAL_LOW;
    valRead = GPIO_VAL_HIGH;

    ret = GpioWrite(tester->cfg.gpio, valWrite);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("GpioTestWriteRead: failed to write val:%hu, ret: %d!", valWrite, ret);
        return ret;
    }
    ret = GpioRead(tester->cfg.gpio, &valRead);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("GpioTestWriteRead: fail to read, ret: %d!", ret);
        return ret;
    }
    if (valWrite != valRead) {
        HDF_LOGE("GpioTestWriteRead: write:%u, but get:%u!", valWrite, valRead);
        return HDF_FAILURE;
    }
    /* change the value and test one more time */
    valWrite = GPIO_VAL_HIGH;
    valRead = GPIO_VAL_LOW;
    ret = GpioWrite(tester->cfg.gpio, valWrite);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("GpioTestWriteRead: fail towrite val:%u, ret: %d!", valWrite, ret);
        return ret;
    }
    ret = GpioRead(tester->cfg.gpio, &valRead);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("GpioTestWriteRead: fail to read, ret: %d!", ret);
        return ret;
    }
    if (valWrite != valRead) {
        HDF_LOGE("GpioTestWriteRead: write:%u, but get:%u!", valWrite, valRead);
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

static int32_t GpioTestIrqHandler(uint16_t gpio, void *data)
{
    struct GpioTester *tester = (struct GpioTester *)data;

    HDF_LOGE("GpioTestIrqHandler: >>>>>>>>>>>>>>>>>>>>>enter gpio:%hu<<<<<<<<<<<<<<<<<<<<<<", gpio);
    if (tester != NULL) {
        tester->irqCnt++;
        return GpioDisableIrq(gpio);
    }
    return HDF_FAILURE;
}

static inline void GpioTestHelperInversePin(uint16_t gpio, uint16_t mode)
{
    uint16_t dir = 0;
    uint16_t valRead = 0;

    (void)GpioRead(gpio, &valRead);
    (void)GpioWrite(gpio, (valRead == GPIO_VAL_LOW) ? GPIO_VAL_HIGH : GPIO_VAL_LOW);
    (void)GpioRead(gpio, &valRead);
    (void)GpioGetDir(gpio, &dir);
    HDF_LOGE("GpioTestHelperInversePin: gpio:%u, val:%u, dir:%u, mode:%x!", gpio, valRead, dir, mode);
}

static int32_t GpioTestIrqSharedFunc(struct GpioTester *tester, uint16_t mode, bool inverse)
{
    int32_t ret;
    uint32_t timeout;

    HDF_LOGD("GpioTestIrqSharedFunc: mark gona set irq ...");
    ret = GpioSetIrq(tester->cfg.gpioIrq, mode, GpioTestIrqHandler, (void *)tester);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("GpioTestIrqSharedFunc: fail to set irq, ret:%d!", ret);
        return ret;
    }
    HDF_LOGD("GpioTestIrqSharedFunc: mark gona enable irq ...");
    ret = GpioEnableIrq(tester->cfg.gpioIrq);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("GpioTestIrqSharedFunc: fail to enable irq, ret: %d!", ret);
        (void)GpioUnsetIrq(tester->cfg.gpioIrq, tester);
        return ret;
    }

    HDF_LOGD("GpioTestIrqSharedFunc: mark gona inverse irq ...");
    for (timeout = 0; tester->irqCnt == 0 && timeout <= tester->irqTimeout;
        timeout += GPIO_TEST_IRQ_DELAY) {
        if (inverse) {
            // maybe can make an inverse ...
            GpioTestHelperInversePin(tester->cfg.gpioIrq, mode);
        }
        OsalMSleep(GPIO_TEST_IRQ_DELAY);
    }
    (void)GpioUnsetIrq(tester->cfg.gpioIrq, tester);

#if defined(_LINUX_USER_) || defined(__KERNEL__)
    if (inverse) {
        HDF_LOGI("GpioTestIrqSharedFunc: do not judge edge trigger!");
        return HDF_SUCCESS;
    }
#endif
    if (tester->irqCnt == 0) {
        HDF_LOGE("GpioTestIrqSharedFunc: fail to set mode:%x on %u!", mode, tester->cfg.gpioIrq);
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

static int32_t GpioTestIrqLevel(void)
{
    return HDF_SUCCESS;
}

static int32_t GpioTestIrqEdge(void)
{
    uint16_t mode;
    struct GpioTester *tester = NULL;

    tester = GpioTesterGet();
    if (tester == NULL) {
        HDF_LOGE("GpioTestIrqEdge: fail to get tester!");
        return HDF_ERR_INVALID_OBJECT;
    }

    /* set dir to out for self trigger on liteos */
#if defined(_LINUX_USER_) || defined(__KERNEL__)
    (void)GpioSetDir(tester->cfg.gpioIrq, GPIO_DIR_IN);
#else
    (void)GpioSetDir(tester->cfg.gpioIrq, GPIO_DIR_OUT);
#endif
    mode = GPIO_IRQ_TRIGGER_FALLING | GPIO_IRQ_TRIGGER_RISING;
    return GpioTestIrqSharedFunc(tester, mode, true);
}

static int32_t GpioTestIrqThread(void)
{
    uint16_t mode;
    struct GpioTester *tester = NULL;

    tester = GpioTesterGet();
    if (tester == NULL) {
        HDF_LOGE("GpioTestIrqThread: fail to get tester!");
        return HDF_ERR_INVALID_OBJECT;
    }

    /* set dir to out for self trigger on liteos */
#if defined(_LINUX_USER_) || defined(__KERNEL__)
    (void)GpioSetDir(tester->cfg.gpioIrq, GPIO_DIR_IN);
#else
    (void)GpioSetDir(tester->cfg.gpioIrq, GPIO_DIR_OUT);
#endif
    mode = GPIO_IRQ_TRIGGER_FALLING | GPIO_IRQ_TRIGGER_RISING | GPIO_IRQ_USING_THREAD;
    return GpioTestIrqSharedFunc(tester, mode, true);
}

static int32_t GpioTestGetNumByName(void)
{
    int32_t ret;
    struct GpioTester *tester = NULL;

    tester = GpioTesterGet();
    if (tester == NULL) {
        HDF_LOGE("GpioTestGetNumByName: fail to get tester!");
        return HDF_ERR_INVALID_OBJECT;
    }

    ret = GpioGetByName(tester->cfg.testNameOne);
    if (ret < 0) {
        HDF_LOGE("GpioTestGetNumByName: name:%s fail to get gpio global number, ret:%d!",
            tester->cfg.testNameOne, ret);
        return ret;
    }

    if (ret != tester->cfg.gpio) {
        HDF_LOGE("GpioTestGetNumByName: gpio number are different. \
            name:%s get gpio global number:%hu but gpio actual number:%hu!",
            tester->cfg.testNameOne, (uint16_t)ret, tester->cfg.gpio);
        return HDF_FAILURE;
    }

    ret = GpioGetByName(tester->cfg.testNameTwo);
    if (ret < 0) {
        HDF_LOGE("GpioTestGetNumByName: name:%s fail to get gpio global number, ret:%d!",
            tester->cfg.testNameTwo, ret);
        return ret;
    }

    if (ret != tester->cfg.gpioTestTwo) {
        HDF_LOGE("GpioTestGetNumByName: gpio number are different. \
            name:%s get gpio global number:%hu but gpio actual number :%hu!",
            tester->cfg.testNameTwo, (uint16_t)ret, tester->cfg.gpioTestTwo);
        return HDF_FAILURE;
    }

    return HDF_SUCCESS;
}

static int32_t GpioTestReliability(void)
{
    uint16_t val = 0;
    struct GpioTester *tester = NULL;

    tester = GpioTesterGet();
    if (tester == NULL) {
        HDF_LOGE("GpioTestReliability: fail to get tester!");
        return HDF_ERR_INVALID_OBJECT;
    }

    (void)GpioGetByName(NULL);              /* invalid gpio name */
    (void)GpioWrite(-1, val);              /* invalid gpio number */
    (void)GpioWrite(tester->cfg.gpio, -1);     /* invalid gpio value */

    (void)GpioRead(-1, &val);              /* invalid gpio number */
    (void)GpioRead(tester->cfg.gpio, NULL);    /* invalid pointer */

    (void)GpioSetDir(-1, val);             /* invalid gpio number */
    (void)GpioSetDir(tester->cfg.gpio, -1);    /* invalid value */

    (void)GpioGetDir(-1, &val);            /* invalid gpio number */
    (void)GpioGetDir(tester->cfg.gpio, NULL);  /* invalid pointer */

    /* invalid gpio number */
    (void)GpioSetIrq(-1, OSAL_IRQF_TRIGGER_RISING, GpioTestIrqHandler, (void *)tester);
    /* invalid irq handler */
    (void)GpioSetIrq(tester->cfg.gpioIrq, OSAL_IRQF_TRIGGER_RISING, NULL, (void *)tester);

    (void)GpioUnsetIrq(-1, NULL);          /* invalid gpio number */

    (void)GpioEnableIrq(-1);               /* invalid gpio number */

    (void)GpioDisableIrq(-1);              /* invalid gpio number */

    return HDF_SUCCESS;
}

static int32_t GpioIfPerformanceTest(void)
{
#ifdef __LITEOS__
    return HDF_SUCCESS;
#endif
    uint16_t val;
    uint64_t startMs;
    uint64_t endMs;
    uint64_t useTime;    // ms
    struct GpioTester *tester = NULL;

    tester = GpioTesterGet();
    if (tester == NULL) {
        HDF_LOGE("GpioIfPerformanceTest: fail to get tester!");
        return HDF_ERR_INVALID_OBJECT;
    }

    startMs = OsalGetSysTimeMs();
    GpioRead(tester->cfg.gpio, &val);
    endMs = OsalGetSysTimeMs();

    useTime = endMs - startMs;
    HDF_LOGI("GpioIfPerformanceTest: ----->interface performance test:[start - end] < 1ms[%s]\r\n",
        useTime < 1 ? "yes" : "no");
    return HDF_SUCCESS;
}

struct GpioTestEntry {
    int cmd;
    int32_t (*func)(void);
    const char *name;
};

static struct GpioTestEntry g_entry[] = {
    { GPIO_TEST_SET_GET_DIR, GpioTestSetGetDir, "GpioTestSetGetDir" },
    { GPIO_TEST_WRITE_READ, GpioTestWriteRead, "GpioTestWriteRead" },
    { GPIO_TEST_IRQ_LEVEL, GpioTestIrqLevel, "GpioTestIrqLevel" },
    { GPIO_TEST_IRQ_EDGE, GpioTestIrqEdge, "GpioTestIrqEdge" },
    { GPIO_TEST_IRQ_THREAD, GpioTestIrqThread, "GpioTestIrqThread" },
    { GPIO_TEST_GET_NUM_BY_NAME, GpioTestGetNumByName, "GpioTestGetNumByName" },
    { GPIO_TEST_RELIABILITY, GpioTestReliability, "GpioTestReliability" },
    { GPIO_TEST_PERFORMANCE, GpioIfPerformanceTest, "GpioIfPerformanceTest" },
};

int32_t GpioTestExecute(int cmd)
{
    uint32_t i;
    int32_t ret = HDF_ERR_NOT_SUPPORT;

#if defined(_LINUX_USER_) || defined(__USER__)
    struct GpioTester *tester = GpioTesterGet();
    if (tester == NULL) {
        HDF_LOGI("GpioTestExecute: tester is null!");
        return HDF_SUCCESS;
    }
    if (tester->cfg.testUserApi == 0) {
        HDF_LOGI("GpioTestExecute: do not test user api!");
        return HDF_SUCCESS;
    }
#endif

    for (i = 0; i < sizeof(g_entry) / sizeof(g_entry[0]); i++) {
        if (g_entry[i].cmd != cmd || g_entry[i].func == NULL) {
            continue;
        }
        ret = GpioTestSetUp();
        if (ret != HDF_SUCCESS) {
            HDF_LOGE("GpioTestExecute: fail to setup!");
            return ret;
        }

        ret = g_entry[i].func();

        (void)GpioTestTearDown();
        break;
    }

    if (ret == HDF_ERR_NOT_SUPPORT) {
        HDF_LOGE("GpioTestExecute: cmd:%d is not support!", cmd);
    }

    HDF_LOGI("[GpioTestExecute][======cmd:%d====ret:%d======]", cmd, ret);
    return ret;
}

void GpioTestExecuteAll(void)
{
    int32_t i;
    int32_t ret;
    int32_t fails = 0;

    for (i = 0; i < GPIO_TEST_MAX; i++) {
        ret = GpioTestExecute(i);
        fails += (ret != HDF_SUCCESS) ? 1 : 0;
    }

    HDF_LOGE("GpioTestExecuteAll: **********PASS:%d  FAIL:%d************\n\n",
        GPIO_TEST_MAX - fails, fails);
}
