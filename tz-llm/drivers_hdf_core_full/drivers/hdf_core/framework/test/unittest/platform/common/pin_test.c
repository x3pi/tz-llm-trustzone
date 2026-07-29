/*
 * Copyright (c) 2021-2023 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include "pin_test.h"
#include "hdf_base.h"
#include "hdf_io_service_if.h"
#include "hdf_log.h"
#include "osal_mem.h"
#include "osal_time.h"
#include "pin_if.h"
#include "securec.h"

#define HDF_LOG_TAG pin_test_c

#define PIN_FUNC_NAME_LENGTH       30

static struct PinCfgs {
    enum PinPullType pullTypeNum;
    uint32_t strengthNum;
    const char *funcName;
} g_oldPinCfg;

struct PinTestEntry {
    int cmd;
    int32_t (*func)(void);
    const char *name;
};

static int32_t PinTestGetTestConfig(struct PinTestConfig *config)
{
    int32_t ret;
    struct HdfSBuf *reply = NULL;
    struct HdfIoService *service = NULL;
    const void *buf = NULL;
    uint32_t len;

    HDF_LOGD("PinTestGetTestConfig: enter!");
    service = HdfIoServiceBind("PIN_TEST");
    if (service == NULL) {
        return HDF_ERR_NOT_SUPPORT;
    }

    reply = HdfSbufObtain(sizeof(*config) + sizeof(uint64_t));
    if (reply == NULL) {
        HDF_LOGE("PinTestGetTestConfig: fail to obtain reply!");
        HdfIoServiceRecycle(service);
        return HDF_ERR_MALLOC_FAIL;
    }

    ret = service->dispatcher->Dispatch(&service->object, 0, NULL, reply);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("PinTestGetTestConfig: remote dispatch fail, ret: %d!", ret);
        HdfIoServiceRecycle(service);
        HdfSbufRecycle(reply);
        return ret;
    }

    if (!HdfSbufReadBuffer(reply, &buf, &len)) {
        HDF_LOGE("PinTestGetTestConfig: read buf fail!");
        HdfIoServiceRecycle(service);
        HdfSbufRecycle(reply);
        return HDF_ERR_IO;
    }

    if (len != sizeof(*config)) {
        HDF_LOGE("PinTestGetTestConfig: config size:%zu, read size:%u!", sizeof(*config), len);
        HdfIoServiceRecycle(service);
        HdfSbufRecycle(reply);
        return HDF_ERR_IO;
    }

    if (memcpy_s(config, sizeof(*config), buf, sizeof(*config)) != EOK) {
        HDF_LOGE("PinTestGetTestConfig: memcpy buf fail!");
        HdfIoServiceRecycle(service);
        HdfSbufRecycle(reply);
        return HDF_ERR_IO;
    }
    config->pinName = config->pinNameBuf;
    HdfSbufRecycle(reply);
    HdfIoServiceRecycle(service);
    HDF_LOGD("PinTestGetTestConfig: done!");
    return HDF_SUCCESS;
}

static struct PinTester *PinTesterGet(void)
{
    int32_t ret;
    static struct PinTester tester;
    static bool hasInit = false;

    HDF_LOGI("PinTesterGet: enter!");
    if (hasInit) {
        return &tester;
    }
    ret = PinTestGetTestConfig(&tester.config);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("PinTesterGet: read config fail, ret: %d!", ret);
        return NULL;
    }

    tester.handle = PinGet(tester.config.pinName);
    if (tester.handle == NULL) {
        HDF_LOGE("PinTesterGet: open pin:%s fail!", tester.config.pinName);
        return NULL;
    }

    hasInit = true;
    HDF_LOGD("PinTesterGet: done!");
    return &tester;
}

static int32_t PinSetGetPullTest(void)
{
    struct PinTester *tester = NULL;
    int32_t ret;
    enum PinPullType getPullTypeNum;
    getPullTypeNum = 0;

    tester = PinTesterGet();
    if (tester == NULL) {
        HDF_LOGE("PinSetGetPullTest: get tester fail!");
        return HDF_ERR_INVALID_OBJECT;
    }
    ret = PinSetPull(tester->handle, tester->config.PullTypeNum);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("PinSetGetPullTest: pin set pull fail, ret: %d, pinName:%s!", ret, tester->config.pinName);
        return HDF_FAILURE;
    }
    HDF_LOGI("PinSetGetPullTest: pin set pull success, PullTypeNum:%d!", tester->config.PullTypeNum);
    ret = PinGetPull(tester->handle, &getPullTypeNum);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("PinSetGetPullTest: pin get pull fail, ret: %d, pinName:%s!", ret, tester->config.pinName);
        return HDF_FAILURE;
    }
    if (tester->config.PullTypeNum != getPullTypeNum) {
        HDF_LOGE("PinSetGetPullTest: pin set pull:%d, but pin get pull:%u!",
            tester->config.PullTypeNum, getPullTypeNum);
        return HDF_FAILURE;
    }
    HDF_LOGD("PinSetGetPullTest: done!");
    return HDF_SUCCESS;
}

static int32_t PinSetGetStrengthTest(void)
{
    struct PinTester *tester = NULL;
    int32_t ret;
    uint32_t getStrengthNum;

    getStrengthNum = 0;
    tester = PinTesterGet();
    if (tester == NULL) {
        HDF_LOGE("PinSetGetStrengthTest: get tester fail!");
        return HDF_ERR_INVALID_OBJECT;
    }
    ret = PinSetStrength(tester->handle, tester->config.strengthNum);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("PinSetGetStrengthTest: pin set strength fail, ret: %d, pinName:%s!", ret, tester->config.pinName);
        return HDF_FAILURE;
    }
    HDF_LOGI("PinSetGetStrengthTest: pin set strength success!,strengthNum:%d!", tester->config.strengthNum);
    ret = PinGetStrength(tester->handle, &getStrengthNum);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("PinSetGetStrengthTest: pin get pull fail, pinName:%s!", tester->config.pinName);
        return HDF_FAILURE;
    }
    if (tester->config.strengthNum != getStrengthNum) {
        HDF_LOGE("PinSetGetStrengthTest: pin set strength:%d, but pin get strength:%d!",
            tester->config.strengthNum, getStrengthNum);
    }
    HDF_LOGD("PinSetGetStrengthTest: done!");
    return HDF_SUCCESS;
}

static int32_t PinSetGetFuncTest(void)
{
    struct PinTester *tester = NULL;
    int32_t ret;

    tester = PinTesterGet();
    if (tester == NULL) {
        HDF_LOGE("PinSetGetFuncTest: get tester fail!");
        return HDF_ERR_INVALID_OBJECT;
    }
    ret = PinSetFunc(tester->handle, (const char *)tester->config.funcNameBuf);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("PinSetGetFuncTest: pin set function fail, ret: %d, pinName:%s, functionName:%s!",
            ret, tester->config.pinName, tester->config.funcNameBuf);
        return HDF_FAILURE;
    }
    HDF_LOGI("PinSetGetFuncTest: pin set function success, pinName:%s, functionName:%s!",
        tester->config.pinName, tester->config.funcNameBuf);
    ret = PinGetFunc(tester->handle, &tester->config.funcName);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("PinSetGetFuncTest: pin get function fail, ret: %d, pinName:%s, functionName:%s!",
            ret, tester->config.pinName, tester->config.funcName);
        return HDF_FAILURE;
    }
    HDF_LOGI("PinSetGetFuncTest: pin get function success, pinName:%s, functionName:%s",
        tester->config.pinName, tester->config.funcName);
    if (strcmp((const char *)tester->config.funcNameBuf, tester->config.funcName) != 0) {
        HDF_LOGE("PinSetGetFuncTest: pin set function:%s, but pin get function:%s!",
            tester->config.funcNameBuf, tester->config.funcName);
    }
    HDF_LOGD("PinSetGetFuncTest: done!");
    return HDF_SUCCESS;
}

static int32_t PinTestSetUpAll(void)
{
    int32_t ret;
    struct PinTester *tester = NULL;
    struct PinTestConfig *cfg = NULL;

    HDF_LOGD("PinTestSetUpAll: enter!");
    tester = PinTesterGet();
    if (tester == NULL) {
        HDF_LOGE("PinTestSetUpAll: get tester fail!");
        return HDF_ERR_INVALID_OBJECT;
    }
    tester->total = PIN_TEST_CMD_MAX;
    tester->fails = 0;

    cfg = &tester->config;
    HDF_LOGD("PinTestSetUpAll: test on pinName:%s, PullTypeNum:%d, strengthNum:%d!",
        cfg->pinName, cfg->PullTypeNum, cfg->strengthNum);
    ret = PinGetPull(tester->handle, &g_oldPinCfg.pullTypeNum);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("PinTestSetUpAll: get pullTypeNum fail, ret: %d!", ret);
        return ret;
    }
    ret = PinGetStrength(tester->handle, &g_oldPinCfg.strengthNum);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("PinTestSetUpAll: get strengthNum fail, ret: %d!", ret);
        return ret;
    }
    ret = PinGetFunc(tester->handle, &g_oldPinCfg.funcName);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("PinTestSetUpAll: get funcName fail, ret: %d!", ret);
        return ret;
    }
    HDF_LOGI("PinTestSetUpAll: old funcName:%s!", g_oldPinCfg.funcName);
    HDF_LOGD("PinTestSetUpAll: exit!");
    return HDF_SUCCESS;
}

static int32_t PinTestTearDownAll(void)
{
    int32_t ret;
    struct PinTester *tester = NULL;

    HDF_LOGD("PinTestTearDownAll: enter!");
    tester = PinTesterGet();
    if (tester == NULL) {
        HDF_LOGE("PinTestTearDownAll: get tester fail!");
        return HDF_ERR_INVALID_OBJECT;
    }
    ret = PinSetPull(tester->handle, g_oldPinCfg.pullTypeNum);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("PinTestTearDownAll: set pullTypeNum fail, ret: %d!", ret);
        return ret;
    }
    ret = PinSetStrength(tester->handle, g_oldPinCfg.strengthNum);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("PinTestTearDownAll: set strengthNum fail, ret: %d!", ret);
        return ret;
    }
    ret = PinSetFunc(tester->handle, g_oldPinCfg.funcName);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("PinTestTearDownAll: set funcName fail, ret: %d!", ret);
        return ret;
    }

    HDF_LOGD("PinTestTearDownAll: exit!");
    return HDF_SUCCESS;
}

int32_t PinTestSetUpSingle(void)
{
    return HDF_SUCCESS;
}

int32_t PinTestTearDownSingle(void)
{
    return HDF_SUCCESS;
}

static int32_t PinTestReliability(void)
{
    struct PinTester *tester = NULL;

    HDF_LOGI("PinTestReliability: enter!");
    tester = PinTesterGet();
    if (tester == NULL || tester->handle == NULL) {
        return HDF_ERR_INVALID_OBJECT;
    }
    HDF_LOGD("PinTestReliability: test dfr for PinSetPull ...");
    // invalid handle
    (void)PinSetPull(NULL, tester->config.PullTypeNum);
    (void)PinGetPull(NULL, &tester->config.PullTypeNum);
    (void)PinSetStrength(NULL, tester->config.strengthNum);
    (void)PinGetStrength(NULL, &tester->config.strengthNum);
    (void)PinSetFunc(NULL, (const char *)tester->config.funcNameBuf);
    (void)PinGetFunc(NULL, &tester->config.funcName);
    // invalid strengthNum
    (void)PinSetStrength(tester->handle, -1);
    (void)PinGetStrength(tester->handle, NULL);
    // invalid FuncName
    (void)PinSetFunc(tester->handle, NULL);
    HDF_LOGD("PinTestReliability: test done. all complete!");

    return HDF_SUCCESS;
}

static struct PinTestEntry g_entry[] = {
    { PIN_TEST_CMD_SETGETPULL, PinSetGetPullTest, "PinSetGetPullTest" },
    { PIN_TEST_CMD_SETGETSTRENGTH, PinSetGetStrengthTest, "PinSetGetStrengthTest" },
    { PIN_TEST_CMD_SETGETFUNC, PinSetGetFuncTest, "PinSetGetFuncTest" },
    { PIN_TEST_CMD_RELIABILITY, PinTestReliability, "PinTestReliability" },
    { PIN_TEST_CMD_SETUP_ALL, PinTestSetUpAll, "PinTestSetUpAll" },
    { PIN_TEST_CMD_TEARDOWN_ALL, PinTestTearDownAll, "PinTestTearDownAll" },
};

int32_t PinTestExecute(int cmd)
{
    uint32_t i;
    int32_t ret = HDF_ERR_NOT_SUPPORT;

    if (cmd > PIN_TEST_CMD_MAX) {
        HDF_LOGE("PinTestExecute: invalid cmd:%d!", cmd);
        ret = HDF_ERR_NOT_SUPPORT;
        HDF_LOGE("[PinTestExecute][======cmd:%d====ret:%d======]", cmd, ret);
        return ret;
    }

    for (i = 0; i < sizeof(g_entry) / sizeof(g_entry[0]); i++) {
        if (g_entry[i].cmd != cmd || g_entry[i].func == NULL) {
            continue;
        }
        ret = g_entry[i].func();
        break;
    }

    HDF_LOGE("[PinTestExecute][======cmd:%d====ret:%d======]", cmd, ret);
    return ret;
}

void PinTestExecuteAll(void)
{
    int32_t i;
    int32_t ret;
    int32_t fails = 0;

    /* setup env for all test cases */
    ret = PinTestExecute(PIN_TEST_CMD_SETUP_ALL);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("PinTestExecuteAll: PinTestExecute SETUP fail!");
    }

    for (i = 0; i < PIN_TEST_CMD_SETUP_ALL; i++) {
        ret = PinTestExecute(i);
        fails += (ret != HDF_SUCCESS) ? 1 : 0;
    }

    /* teardown env for all test cases */
    ret = PinTestExecute(PIN_TEST_CMD_TEARDOWN_ALL);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("PinTestExecuteAll: PinTestExecute TEARDOWN fail!");
    }

    HDF_LOGI("PinTestExecuteAll:: **********PASS:%d  FAIL:%d************\n\n",
        PIN_TEST_CMD_RELIABILITY + 1 - fails, fails);
}
