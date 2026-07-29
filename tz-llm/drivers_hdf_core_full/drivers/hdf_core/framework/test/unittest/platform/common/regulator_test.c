/*
 * Copyright (c) 2021-2023 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include "regulator_test.h"
#include "device_resource_if.h"
#include "hdf_log.h"
#include "osal_thread.h"
#include "osal_time.h"
#include "securec.h"
#include "regulator_if.h"
#if defined(CONFIG_DRIVERS_HDF_PLATFORM_REGULATOR)
#include "virtual/regulator_linux_voltage_virtual_driver.h"
#include "virtual/regulator_linux_current_virtual_driver.h"
#endif

#define HDF_LOG_TAG regulator_test_c

struct RegulatorTestFunc {
    enum RegulatorTestCmd type;
    int32_t (*Func)(struct RegulatorTest *test);
};

enum RegulatorVoltage {
    VOLTAGE_50_UV = 50,
    VOLTAGE_250_UV = 250,
    VOLTAGE_2500_UV = 2500,
};

enum RegulatorCurrent {
    CURRENT_50_UA = 50,
    CURRENT_250_UA = 250,
};

static int32_t RegulatorEnableTest(struct RegulatorTest *test)
{
    if (test == NULL || test->handle == NULL) {
        HDF_LOGE("RegulatorEnableTest: test or handle is null!");
        return HDF_ERR_INVALID_OBJECT;
    }

    RegulatorEnable(test->handle);
    return HDF_SUCCESS;
}

static int32_t RegulatorDisableTest(struct RegulatorTest *test)
{
    if (test == NULL || test->handle == NULL) {
        HDF_LOGE("RegulatorDisableTest: test or handle is null!");
        return HDF_ERR_INVALID_OBJECT;
    }

    RegulatorDisable(test->handle);
    return HDF_SUCCESS;
}

static int32_t RegulatorForceDisableTest(struct RegulatorTest *test)
{
    if (test == NULL || test->handle == NULL) {
        HDF_LOGE("RegulatorForceDisableTest: test or handle is null!");
        return HDF_ERR_INVALID_OBJECT;
    }

    RegulatorForceDisable(test->handle);
    return HDF_SUCCESS;
}

static int32_t RegulatorSetVoltageTest(struct RegulatorTest *test)
{
    if (test == NULL || test->handle == NULL) {
        HDF_LOGE("RegulatorSetVoltageTest: test or handle is null!");
        return HDF_ERR_INVALID_OBJECT;
    }

    if (test->mode != REGULATOR_CHANGE_VOLTAGE) {
        return HDF_SUCCESS;
    }

    if (RegulatorSetVoltage(test->handle, test->minUv, test->maxUv) != HDF_SUCCESS) {
        HDF_LOGE("RegulatorSetVoltageTest: [%d, %d] test fail", test->maxUv, test->minUv);
        return HDF_FAILURE;
    }

    return HDF_SUCCESS;
}

static int32_t RegulatorGetVoltageTest(struct RegulatorTest *test)
{
    if (test == NULL || test->handle == NULL) {
        HDF_LOGE("RegulatorGetVoltageTest: test or handle is null!");
        return HDF_ERR_INVALID_OBJECT;
    }

    if (test->mode != REGULATOR_CHANGE_VOLTAGE) {
        return HDF_SUCCESS;
    }

    if (RegulatorGetVoltage(test->handle, &test->uv) != HDF_SUCCESS) {
        HDF_LOGE("RegulatorGetVoltageTest: test fail!");
        return HDF_FAILURE;
    }

    return HDF_SUCCESS;
}

static int32_t RegulatorSetCurrentTest(struct RegulatorTest *test)
{
    if (test == NULL || test->handle == NULL) {
        HDF_LOGE("RegulatorSetCurrentTest: test or handle is null!");
        return HDF_ERR_INVALID_OBJECT;
    }

    if (test->mode != REGULATOR_CHANGE_CURRENT) {
        return HDF_SUCCESS;
    }

    if (RegulatorSetCurrent(test->handle, test->minUa, test->maxUa) != HDF_SUCCESS) {
        HDF_LOGE("RegulatorSetCurrentTest: [%d, %d] test fail!", test->minUa, test->maxUa);
        return HDF_FAILURE;
    }

    return HDF_SUCCESS;
}

static int32_t RegulatorGetCurrentTest(struct RegulatorTest *test)
{
    if (test == NULL || test->handle == NULL) {
        HDF_LOGE("RegulatorGetCurrentTest: test or handle is null!");
        return HDF_ERR_INVALID_OBJECT;
    }

    if (test->mode != REGULATOR_CHANGE_CURRENT) {
        return HDF_SUCCESS;
    }

    if (RegulatorGetCurrent(test->handle, &test->ua) != HDF_SUCCESS) {
        HDF_LOGE("RegulatorGetCurrentTest: test fail!");
        return HDF_FAILURE;
    }

    return HDF_SUCCESS;
}

static int32_t RegulatorGetStatusTest(struct RegulatorTest *test)
{
    if (test == NULL || test->handle == NULL) {
        HDF_LOGE("RegulatorGetStatusTest: test or handle is null!");
        return HDF_ERR_INVALID_OBJECT;
    }

    if (RegulatorGetStatus(test->handle, &test->status) != HDF_SUCCESS) {
        HDF_LOGE("RegulatorGetStatusTest: test fail!");
        return HDF_FAILURE;
    }

    if ((test->status != REGULATOR_STATUS_ON) && (test->status != REGULATOR_STATUS_OFF)) {
        HDF_LOGE("RegulatorGetStatusTest: regulator status invalid %d!", test->status);
        return HDF_FAILURE;
    }

    return HDF_SUCCESS;
}

static int RegulatorTestThreadFunc(void *param)
{
    DevHandle handle = RegulatorOpen("regulator_virtual_1");
    if (handle == NULL) {
        HDF_LOGE("RegulatorTestThreadFunc: regulator test get handle fail!");
        *((int32_t *)param) = 1;
        return HDF_FAILURE;
    }

    if (RegulatorSetVoltage(handle, VOLTAGE_250_UV, VOLTAGE_2500_UV) != HDF_SUCCESS) {
        HDF_LOGE("RegulatorTestThreadFunc: test fail!");
        RegulatorClose(handle);
        *((int32_t *)param) = 1;
        return HDF_FAILURE;
    }

    RegulatorClose(handle);
    *((int32_t *)param) = 1;
    return HDF_SUCCESS;
}

static int32_t RegulatorTestStartThread(struct OsalThread *thread1, struct OsalThread *thread2,
    const int32_t *count1, const int32_t *count2)
{
    int32_t ret;
    uint32_t time = 0;
    struct OsalThreadParam cfg1;
    struct OsalThreadParam cfg2;

    if (memset_s(&cfg1, sizeof(cfg1), 0, sizeof(cfg1)) != EOK ||
        memset_s(&cfg2, sizeof(cfg2), 0, sizeof(cfg2)) != EOK) {
        HDF_LOGE("RegulatorTestStartThread: memset_s fail!");
        return HDF_ERR_IO;
    }
    cfg1.name = "RegulatorTestThread-1";
    cfg2.name = "RegulatorTestThread-2";
    cfg1.priority = cfg2.priority = OSAL_THREAD_PRI_DEFAULT;
    cfg1.stackSize = cfg2.stackSize = REGULATOR_TEST_STACK_SIZE;

    ret = OsalThreadStart(thread1, &cfg1);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("RegulatorTestStartThread: start test thread1 fail, ret: %d!", ret);
        return ret;
    }

    ret = OsalThreadStart(thread2, &cfg2);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("RegulatorTestStartThread: start test thread2 fail, ret: %d!", ret);
    }

    while (*count1 == 0 || *count2 == 0) {
        HDF_LOGE("RegulatorTestStartThread: waitting testing Regulator thread finish...");
        OsalMSleep(REGULATOR_TEST_WAIT_TIMES);
        time++;
        if (time > REGULATOR_TEST_WAIT_TIMEOUT) {
            break;
        }
    }
    return ret;
}

static int32_t RegulatorTestMultiThread(struct RegulatorTest *test)
{
    int32_t ret;
    struct OsalThread thread1;
    struct OsalThread thread2;
    int32_t count1 = 0;
    int32_t count2 = 0;

    (void)test;
    ret = OsalThreadCreate(&thread1, (OsalThreadEntry)RegulatorTestThreadFunc, (void *)&count1);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("RegulatorTestMultiThread: create test thread1 fail, ret: %d!", ret);
        return ret;
    }

    ret = OsalThreadCreate(&thread2, (OsalThreadEntry)RegulatorTestThreadFunc, (void *)&count2);
    if (ret != HDF_SUCCESS) {
        (void)OsalThreadDestroy(&thread1);
        HDF_LOGE("RegulatorTestMultiThread: create test thread2 fail, ret: %d!", ret);
        return ret;
    }

    ret = RegulatorTestStartThread(&thread1, &thread2, &count1, &count2);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("RegulatorTestMultiThread: test start thread fail, ret: %d!", ret);
    }

    (void)OsalThreadDestroy(&thread1);
    (void)OsalThreadDestroy(&thread2);
    return ret;
}

static int32_t RegulatorTestReliability(struct RegulatorTest *test)
{
    HDF_LOGD("RegulatorTestReliability: test for Regulator ...");
    if (test == NULL || test->handle == NULL) {
        HDF_LOGE("RegulatorTestReliability: test or handle is null!");
        return HDF_ERR_INVALID_OBJECT;
    }

    RegulatorSetVoltage(test->handle, VOLTAGE_250_UV, VOLTAGE_50_UV);
    RegulatorSetCurrent(test->handle, CURRENT_250_UA, CURRENT_50_UA);
    RegulatorGetCurrent(test->handle, NULL);
    return HDF_SUCCESS;
}

static struct RegulatorTestFunc g_regulatorTestFunc[] = {
    {REGULATOR_ENABLE_TEST, RegulatorEnableTest},
    {REGULATOR_DISABLE_TEST, RegulatorDisableTest},
    {REGULATOR_FORCE_DISABLE_TEST, RegulatorForceDisableTest},
    {REGULATOR_SET_VOLTAGE_TEST, RegulatorSetVoltageTest},
    {REGULATOR_GET_VOLTAGE_TEST, RegulatorGetVoltageTest},
    {REGULATOR_SET_CURRENT_TEST, RegulatorSetCurrentTest},
    {REGULATOR_GET_CURRENT_TEST, RegulatorGetCurrentTest},
    {REGULATOR_GET_STATUS_TEST, RegulatorGetStatusTest},
    {REGULATOR_MULTI_THREAD_TEST, RegulatorTestMultiThread},
    {REGULATOR_RELIABILITY_TEST, RegulatorTestReliability},
};

static int32_t RegulatorTestEntry(struct RegulatorTest *test, int32_t cmd)
{
    int32_t i;
    int32_t ret = HDF_ERR_NOT_SUPPORT;

    if (test == NULL || test->name == NULL) {
        HDF_LOGE("RegulatorTestEntry: test or name is null!");
        return HDF_ERR_INVALID_OBJECT;
    }

    if (cmd != REGULATOR_MULTI_THREAD_TEST) {
        test->handle = RegulatorOpen(test->name);
        if (test->handle == NULL) {
            HDF_LOGE("RegulatorTestEntry: regulator test get handle fail!");
            return HDF_FAILURE;
        }
    }

    for (i = 0; i < sizeof(g_regulatorTestFunc) / sizeof(g_regulatorTestFunc[0]); i++) {
        if (cmd == g_regulatorTestFunc[i].type && g_regulatorTestFunc[i].Func != NULL) {
            ret = g_regulatorTestFunc[i].Func(test);
            HDF_LOGD("RegulatorTestEntry: cmd %d ret %d!", cmd, ret);
            break;
        }
    }

    if (cmd != REGULATOR_MULTI_THREAD_TEST) {
        RegulatorClose(test->handle);
    }
    return ret;
}

static int32_t RegulatorTestBind(struct HdfDeviceObject *device)
{
    static struct RegulatorTest test;

    if (device != NULL) {
        device->service = &test.service;
    } else {
        HDF_LOGE("RegulatorTestBind: device is null!");
    }

    HDF_LOGI("RegulatorTestBind: success!\r\n");
    return HDF_SUCCESS;
}

static int32_t RegulatorTestInitFromHcs(struct RegulatorTest *test, const struct DeviceResourceNode *node)
{
    int32_t ret;
    struct DeviceResourceIface *face = NULL;

    face = DeviceResourceGetIfaceInstance(HDF_CONFIG_SOURCE);
    if (face == NULL) {
        HDF_LOGE("RegulatorTestInitFromHcs: face is null!");
        return HDF_FAILURE;
    }
    if (face->GetUint32 == NULL || face->GetUint32Array == NULL) {
        HDF_LOGE("RegulatorTestInitFromHcs: GetUint32 or GetUint32Array not support");
        return HDF_ERR_NOT_SUPPORT;
    }

    ret = face->GetString(node, "name", &(test->name), "ERROR");
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("RegulatorTestInitFromHcs: read name fail!");
        return HDF_FAILURE;
    }

    ret = face->GetUint8(node, "mode", &test->mode, 0);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("RegulatorTestInitFromHcs: read mode fail!");
        return HDF_FAILURE;
    }

    ret = face->GetUint32(node, "minUv", &test->minUv, 0);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("RegulatorTestInitFromHcs: read minUv fail!");
        return HDF_FAILURE;
    }

    ret = face->GetUint32(node, "maxUv", &test->maxUv, 0);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("RegulatorTestInitFromHcs: read maxUv fail!");
        return HDF_FAILURE;
    }

    ret = face->GetUint32(node, "minUa", &test->minUa, 0);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("RegulatorTestInitFromHcs: read minUa fail!");
        return HDF_FAILURE;
    }

    ret = face->GetUint32(node, "maxUa", &test->maxUa, 0);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("RegulatorTestInitFromHcs: read maxUa fail!");
        return HDF_FAILURE;
    }

    HDF_LOGI("RegulatorTestInitFromHcs: regulator test init:[%s][%d]--[%d][%d]--[%d][%d]!",
        test->name, test->mode, test->minUv, test->maxUv, test->minUa, test->maxUa);

    return HDF_SUCCESS;
}

static int32_t RegulatorTestInit(struct HdfDeviceObject *device)
{
    struct RegulatorTest *test = NULL;

    HDF_LOGI("RegulatorTestInit: enter!\r\n");

#if defined(CONFIG_DRIVERS_HDF_PLATFORM_REGULATOR)
    VirtualVoltageRegulatorAdapterInit();
    VirtualCurrentRegulatorAdapterInit();
#endif

    if (device == NULL || device->service == NULL || device->property == NULL) {
        HDF_LOGE("RegulatorTestInit: invalid parameter!");
        return HDF_ERR_INVALID_PARAM;
    }
    test = (struct RegulatorTest *)device->service;

    if (RegulatorTestInitFromHcs(test, device->property) != HDF_SUCCESS) {
        HDF_LOGE("RegulatorTestInit: RegulatorTestInitFromHcs fail!");
        return HDF_FAILURE;
    }

    test->TestEntry = RegulatorTestEntry;
    HDF_LOGI("RegulatorTestInit: success!");

    return HDF_SUCCESS;
}

static void RegulatorTestRelease(struct HdfDeviceObject *device)
{
    (void)device;
}

struct HdfDriverEntry g_regulatorTestEntry = {
    .moduleVersion = 1,
    .Bind = RegulatorTestBind,
    .Init = RegulatorTestInit,
    .Release = RegulatorTestRelease,
    .moduleName = "PLATFORM_REGULATOR_TEST",
};
HDF_INIT(g_regulatorTestEntry);
