/*
 * Copyright (c) 2020-2023 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include "pwm_test.h"
#include "device_resource_if.h"
#include "hdf_io_service_if.h"
#include "hdf_log.h"
#include "osal_mem.h"
#include "osal_time.h"
#include "securec.h"

#define HDF_LOG_TAG           pwm_test
#define SEQ_OUTPUT_DELAY      100 /* Delay time of sequential output, unit: ms */
#define OUTPUT_WAVES_DELAY    1 /* Delay time of waves output, unit: second */
#define TEST_WAVES_NUMBER     10 /* The number of waves for test. */

static int32_t PwmTesterGetConfig(struct PwmTestConfig *config)
{
    int32_t ret;
    struct HdfSBuf *reply = NULL;
    struct HdfIoService *service = NULL;
    const void *buf = NULL;
    uint32_t len;

    service = HdfIoServiceBind("PWM_TEST");
    if ((service == NULL) || (service->dispatcher == NULL) || (service->dispatcher->Dispatch == NULL)) {
        HDF_LOGE("PwmTesterGetConfig: service bind fail!\n");
        return HDF_ERR_NOT_SUPPORT;
    }

    reply = HdfSbufObtain(sizeof(*config) + sizeof(uint64_t));
    if (reply == NULL) {
        HDF_LOGE("PwmTesterGetConfig: fail to obtain reply!");
        HdfIoServiceRecycle(service);
        return HDF_ERR_MALLOC_FAIL;
    }

    ret = service->dispatcher->Dispatch(&service->object, 0, NULL, reply);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("PwmTesterGetConfig: remote dispatch fail, ret: %d!", ret);
        HdfIoServiceRecycle(service);
        HdfSbufRecycle(reply);
        return ret;
    }

    if (!HdfSbufReadBuffer(reply, &buf, &len)) {
        HDF_LOGE("PwmTesterGetConfig: read buf fail!");
        HdfIoServiceRecycle(service);
        HdfSbufRecycle(reply);
        return HDF_ERR_IO;
    }

    if (len != sizeof(*config)) {
        HDF_LOGE("PwmTesterGetConfig: config size:%zu, read size:%u!", sizeof(*config), len);
        HdfIoServiceRecycle(service);
        HdfSbufRecycle(reply);
        return HDF_ERR_IO;
    }

    if (memcpy_s(config, sizeof(*config), buf, sizeof(*config)) != EOK) {
        HDF_LOGE("PwmTesterGetConfig: memcpy buf fail!");
        HdfIoServiceRecycle(service);
        HdfSbufRecycle(reply);
        return HDF_ERR_IO;
    }

    HdfIoServiceRecycle(service);
    HdfSbufRecycle(reply);
    return HDF_SUCCESS;
}

static struct PwmTester *PwmTesterGet(void)
{
    int32_t ret;
    static struct PwmTester tester;

    OsalMSleep(SEQ_OUTPUT_DELAY);
    ret = PwmTesterGetConfig(&tester.config);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("PwmTesterGet: read config fail, ret: %d!", ret);
        return NULL;
    }

    tester.handle = PwmOpen(tester.config.num);
    if (tester.handle == NULL) {
        HDF_LOGE("PwmTesterGet: open pwm device:%u fail!", tester.config.num);
        return NULL;
    }

    return &tester;
}

static void PwmTesterPut(struct PwmTester *tester)
{
    if (tester == NULL) {
        HDF_LOGE("PwmTesterPut: tester is null!");
        return;
    }
    PwmClose(tester->handle);
    tester->handle = NULL;
    OsalMSleep(SEQ_OUTPUT_DELAY);
}

static int32_t PwmSetGetConfigTest(struct PwmTester *tester)
{
    int32_t ret;
    struct PwmConfig cfg = {0};
    uint32_t number;

    number = tester->config.cfg.number;
    tester->config.cfg.number = ((number > 0) ? 0 : TEST_WAVES_NUMBER);
    HDF_LOGI("PwmSetGetConfigTest: set number %u!", tester->config.cfg.number);
    ret = PwmSetConfig(tester->handle, &(tester->config.cfg));
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("PwmSetGetConfigTest: [PwmSetConfig] fail, ret: %d!", ret);
        return ret;
    }

    OsalSleep(OUTPUT_WAVES_DELAY);
    tester->config.cfg.number = number;
    HDF_LOGI("PwmSetGetConfigTest: Set number %u.", tester->config.cfg.number);
    ret = PwmSetConfig(tester->handle, &(tester->config.cfg));
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("PwmSetGetConfigTest: [PwmSetConfig] fail, ret: %d!", ret);
        return ret;
    }

    ret = PwmGetConfig(tester->handle, &cfg);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("PwmSetGetConfigTest: [PwmGetConfig] fail, ret: %d!", ret);
        return ret;
    }

    if (memcmp(&cfg, &(tester->config.cfg), sizeof(cfg)) != 0) {
        HDF_LOGE("PwmSetGetConfigTest: [memcmp_s] fail!");
        return HDF_FAILURE;
    }

    return HDF_SUCCESS;
}

static int32_t PwmSetPeriodTest(struct PwmTester *tester)
{
    int32_t ret;
    struct PwmConfig cfg = {0};
    uint32_t period;

    period = tester->config.cfg.period + tester->originCfg.period;
    ret = PwmSetPeriod(tester->handle, period);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("PwmSetPeriodTest: [PwmSetPeriod] fail, ret: %d!", ret);
        return ret;
    }

    ret = PwmGetConfig(tester->handle, &cfg);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("PwmSetPeriodTest: [PwmGetConfig] fail, ret: %d!", ret);
        return ret;
    }

    if (cfg.period != period) {
        HDF_LOGE("PwmSetPeriodTest: fail: cfg.period:%u period:%u!", cfg.period, period);
        return HDF_FAILURE;
    }

    return HDF_SUCCESS;
}

static int32_t PwmSetDutyTest(struct PwmTester *tester)
{
    int32_t ret;
    struct PwmConfig cfg = {0};
    uint32_t duty;

    duty = tester->config.cfg.duty+ tester->originCfg.duty;
    ret = PwmSetDuty(tester->handle, duty);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("PwmSetDutyTest: [PwmSetDuty] fail, ret: %d!", ret);
        return ret;
    }

    ret = PwmGetConfig(tester->handle, &cfg);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("PwmSetDutyTest: [PwmGetConfig] fail, ret: %d!", ret);
        return ret;
    }

    if (cfg.duty != duty) {
        HDF_LOGE("PwmSetDutyTest: fail!");
        return HDF_FAILURE;
    }

    return HDF_SUCCESS;
}

static int32_t PwmSetPolarityTest(struct PwmTester *tester)
{
    int32_t ret;
    struct PwmConfig cfg = {0};

    tester->config.cfg.polarity = PWM_NORMAL_POLARITY;
    HDF_LOGI("PwmSetPolarityTest: Test [PwmSetPolarity] polarity %u!", tester->config.cfg.polarity);
    ret = PwmSetPolarity(tester->handle, tester->config.cfg.polarity);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("PwmSetPolarityTest: [PwmSetPolarity] fail, ret: %d!", ret);
        return ret;
    }

    tester->config.cfg.polarity = PWM_INVERTED_POLARITY;
    HDF_LOGI("PwmSetPolarityTest: Test [PwmSetPolarity] polarity %u!", tester->config.cfg.polarity);
    ret = PwmSetPolarity(tester->handle, tester->config.cfg.polarity);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("PwmSetPolarityTest: [PwmSetPolarity] fail, ret: %d!", ret);
        return ret;
    }

    ret = PwmGetConfig(tester->handle, &cfg);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("PwmSetPolarityTest: [PwmGetConfig] fail, ret: %d!", ret);
        return ret;
    }

    if (cfg.polarity != tester->config.cfg.polarity) {
        HDF_LOGE("PwmSetPolarityTest: fail!");
        return HDF_FAILURE;
    }

    return HDF_SUCCESS;
}

static int32_t PwmEnableTest(struct PwmTester *tester)
{
    int32_t ret;
    struct PwmConfig cfg = {0};

    ret = PwmDisable(tester->handle);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("PwmEnableTest: [PwmDisable] fail, ret: %d!", ret);
        return ret;
    }

    HDF_LOGI("PwmEnableTest: Test [PwmEnable] enable!");
    ret = PwmEnable(tester->handle);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("PwmEnableTest: [PwmEnable] fail, ret: %d!", ret);
        return ret;
    }

    ret = PwmGetConfig(tester->handle, &cfg);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("PwmEnableTest: [PwmGetConfig] fail, ret: %d!", ret);
        return ret;
    }

    if (cfg.status == PWM_DISABLE_STATUS) {
        HDF_LOGE("PwmEnableTest: fail!");
        return HDF_FAILURE;
    }

    return HDF_SUCCESS;
}

static int32_t PwmDisableTest(struct PwmTester *tester)
{
    int32_t ret;
    struct PwmConfig cfg = {0};

    ret = PwmEnable(tester->handle);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("PwmDisableTest: [PwmEnable] fail, ret: %d!", ret);
        return ret;
    }

    HDF_LOGI("PwmDisableTest: Test [PwmDisable] disable!");
    ret = PwmDisable(tester->handle);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("PwmDisableTest: [PwmDisable] fail, ret: %d!", ret);
        return ret;
    }

    ret = PwmGetConfig(tester->handle, &cfg);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("PwmDisableTest: [PwmGetConfig] fail, ret: %d!", ret);
        return ret;
    }

    if (cfg.status == PWM_ENABLE_STATUS) {
        HDF_LOGE("PwmDisableTest: fail!");
        return HDF_FAILURE;
    }

    return HDF_SUCCESS;
}

#define TEST_PERIOD 2147483647
#define TEST_DUTY 2147483647
#define TEST_POLARITY 10
static int32_t PwmReliabilityTest(struct PwmTester *tester)
{
    struct PwmConfig cfg = {0};

    (void)PwmSetConfig(tester->handle, &(tester->config.cfg));
    (void)PwmSetConfig(tester->handle, NULL);
    (void)PwmSetConfig(NULL, &(tester->config.cfg));
    (void)PwmGetConfig(tester->handle, &cfg);
    (void)PwmGetConfig(tester->handle, NULL);
    (void)PwmGetConfig(NULL, &cfg);

    (void)PwmSetPeriod(tester->handle, 0);
    (void)PwmSetPeriod(tester->handle, TEST_PERIOD);
    (void)PwmSetPeriod(NULL, TEST_PERIOD);

    (void)PwmSetDuty(tester->handle, 0);
    (void)PwmSetDuty(tester->handle, TEST_DUTY);
    (void)PwmSetDuty(NULL, TEST_DUTY);

    (void)PwmSetPolarity(tester->handle, 0);
    (void)PwmSetPolarity(tester->handle, TEST_POLARITY);
    (void)PwmSetPolarity(NULL, TEST_POLARITY);

    (void)PwmEnable(tester->handle);
    (void)PwmEnable(NULL);

    (void)PwmDisable(tester->handle);
    (void)PwmDisable(NULL);
    HDF_LOGI("PwmReliabilityTest: success!");
    return HDF_SUCCESS;
}

static int32_t PwmIfPerformanceTest(struct PwmTester *tester)
{
#ifdef __LITEOS__
    // liteos the accuracy of the obtained time is too large and inaccurate.
    if (tester == NULL) {
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
#endif

    struct PwmConfig cfg = {0};
    uint64_t startMs;
    uint64_t endMs;
    uint64_t useTime;    // ms

    startMs = OsalGetSysTimeMs();
    PwmGetConfig(tester->handle, &cfg);
    endMs = OsalGetSysTimeMs();

    useTime = endMs - startMs;
    HDF_LOGI("PwmIfPerformanceTest: ----->interface performance test:[start - end] < 1ms[%s]\r\n",
        useTime < 1 ? "yes" : "no");
    return HDF_SUCCESS;
}

struct PwmTestEntry {
    int cmd;
    int32_t (*func)(struct PwmTester *tester);
};

static struct PwmTestEntry g_entry[] = {
    { PWM_SET_PERIOD_TEST, PwmSetPeriodTest },
    { PWM_SET_DUTY_TEST, PwmSetDutyTest },
    { PWM_SET_POLARITY_TEST, PwmSetPolarityTest },
    { PWM_ENABLE_TEST, PwmEnableTest },
    { PWM_DISABLE_TEST, PwmDisableTest },
    { PWM_SET_GET_CONFIG_TEST, PwmSetGetConfigTest },
    { PWM_RELIABILITY_TEST, PwmReliabilityTest },
    { PWM_IF_PERFORMANCE_TEST, PwmIfPerformanceTest },
};

int32_t PwmTestExecute(int cmd)
{
    uint32_t i;
    int32_t ret;
    struct PwmTester *tester = NULL;

    if (cmd > PWM_TEST_CMD_MAX) {
        HDF_LOGE("PwmTestExecute: invalid cmd:%d!", cmd);
        return HDF_ERR_NOT_SUPPORT;
    }

    tester = PwmTesterGet();
    if (tester == NULL) {
        HDF_LOGE("PwmTestExecute: get tester fail!");
        return HDF_ERR_INVALID_OBJECT;
    }

    // At first test case.
    if (cmd == PWM_SET_PERIOD_TEST) {
        ret = PwmGetConfig(tester->handle, &(tester->originCfg));
        if (ret != HDF_SUCCESS) {
            HDF_LOGE("PwmTestExecute: [PwmGetConfig] fail, ret: %d!", ret);
            HDF_LOGI("[PwmTestExecute][======cmd:%d====ret:%d======]", cmd, ret);
            PwmTesterPut(tester);
            return ret;
        }
    }

    for (i = 0; i < sizeof(g_entry) / sizeof(g_entry[0]); i++) {
        if (g_entry[i].cmd != cmd || g_entry[i].func == NULL) {
            continue;
        }
        ret = g_entry[i].func(tester);
        break;
    }

    // At last test case.
    if (cmd == PWM_DISABLE_TEST) {
        PwmSetConfig(tester->handle, &(tester->originCfg));
    }

    HDF_LOGI("[PwmTestExecute][======cmd:%d====ret:%d======]", cmd, ret);
    PwmTesterPut(tester);
    return ret;
}
