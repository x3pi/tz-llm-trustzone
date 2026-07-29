/*
 * Copyright (c) 2020-2023 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include "device_resource_if.h"
#include "hdf_base.h"
#include "hdf_log.h"
#include "osal_mem.h"
#include "osal_time.h"
#include "sdio_if.h"
#include "sdio_test.h"

#define HDF_LOG_TAG sdio_test_c

#define TEST_DATA_LEN 3
#define TEST_BLOCKSIZE 512
#ifdef CONFIG_IMX8MM_SDIO_TEST
#define TEST_SDIO_BASE_ADDR 0x10000
#define TEST_FUNC_NUM 1
#define TEST_ADDR_OFFSET 0x0D
#define TEST_FIXED_OFFSET 0x09
#else
#define TEST_SDIO_BASE_ADDR 0x100
#define TEST_FUNC_NUM 1
#define TEST_ADDR_OFFSET 0x10
#define TEST_FIXED_OFFSET 0x09
#endif
#define TEST_TIME_OUT 1000
#define TEST_ADDR_ADD 1
#define TEST_FUNC0_ADDR 0xFE

struct SdioTestFunc {
    enum SdioTestCmd type;
    int32_t (*Func)(struct SdioTester *tester);
};

static DevHandle SdioTestGetHandle(struct SdioTester *tester)
{
    if (tester == NULL) {
        HDF_LOGE("SdioTestGetHandle: tester is null!");
        return NULL;
    }
    return SdioOpen((int16_t)(tester->busNum), &(tester->config));
}

static void SdioTestReleaseHandle(DevHandle handle)
{
    if (handle == NULL) {
        HDF_LOGE("SdioTestReleaseHandle: sdio handle is null!");
        return;
    }
    SdioClose(handle);
}

static int32_t TestSdioIncrAddrReadAndWriteOtherBytes(struct SdioTester *tester)
{
    int32_t ret;
    uint8_t data[TEST_DATA_LEN] = {0};
    const uint32_t addr = TEST_SDIO_BASE_ADDR * TEST_FUNC_NUM + TEST_ADDR_OFFSET + TEST_ADDR_ADD;

    ret = SdioReadBytes(tester->handle, &data[0], addr, 1);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("TestSdioIncrAddrReadAndWriteOtherBytes: SdioReadBytes fail! ret = %d!", ret);
        return HDF_FAILURE;
    }
    HDF_LOGE("TestSdioIncrAddrReadAndWriteOtherBytes: read, data[0]:%d\n", data[0]);
    ret = SdioWriteBytes(tester->handle, &data[0], addr, 1);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("TestSdioIncrAddrReadAndWriteOtherBytes: SdioWriteBytes fail! ret = %d!", ret);
        return HDF_FAILURE;
    }
    HDF_LOGD("TestSdioIncrAddrReadAndWriteOtherBytes: write, data[0]:%u!\n", data[0]);
    return HDF_SUCCESS;
}

static int32_t TestSdioIncrAddrReadAndWriteOneByte(struct SdioTester *tester)
{
    int32_t ret;
    uint8_t val;
    uint32_t addr;

    addr = TEST_SDIO_BASE_ADDR * TEST_FUNC_NUM + TEST_ADDR_OFFSET;
    /* read 1 bits */
    ret = SdioReadBytes(tester->handle, &val, addr, 1);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("TestSdioIncrAddrReadAndWriteOneByte: SdioReadBytes fail, ret = %d!", ret);
        return HDF_FAILURE;
    }
    HDF_LOGD("TestSdioIncrAddrReadAndWriteOneByte: read, val:%d!\n", val);
    /* write 1 bits */
    ret = SdioWriteBytes(tester->handle, &val, addr, 1);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("TestSdioIncrAddrReadAndWriteOneByte: SdioWriteBytes fail, ret = %d!", ret);
        return HDF_FAILURE;
    }
    HDF_LOGD("TestSdioIncrAddrReadAndWriteOneByte: write, val:%d!\n", val);
    /* read 1 bits */
    addr++;
    ret = SdioReadBytes(tester->handle, &val, addr, 1);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("TestSdioIncrAddrReadAndWriteOneByte: SdioReadBytes fail, ret = %d!", ret);
        return HDF_FAILURE;
    }
    HDF_LOGD("TestSdioIncrAddrReadAndWriteOneByte: read, val:%u!", val);
    /* read 1 bits */
    addr++;
    ret = SdioReadBytes(tester->handle, &val, addr, 1);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("TestSdioIncrAddrReadAndWriteOneByte: SdioReadBytes fail! ret = %d!", ret);
        return HDF_FAILURE;
    }
    HDF_LOGE("TestSdioIncrAddrReadAndWriteOneByte: read, val:%u!", val);
    return HDF_SUCCESS;
}

static int32_t TestSdioIncrAddrReadAndWriteBytes(struct SdioTester *tester)
{
    int32_t ret;

    SdioClaimHost(tester->handle);
    ret = TestSdioIncrAddrReadAndWriteOneByte(tester);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("TestSdioIncrAddrReadAndWriteBytes: TestSdioIncrAddrReadAndWriteOneByte fail, ret = %d!", ret);
        SdioReleaseHost(tester->handle);
        return HDF_FAILURE;
    }
    ret = TestSdioIncrAddrReadAndWriteOtherBytes(tester);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("TestSdioIncrAddrReadAndWriteBytes: TestSdioIncrAddrReadAndWriteOtherBytes fail, ret = %d!", ret);
        SdioReleaseHost(tester->handle);
        return HDF_FAILURE;
    }
    SdioReleaseHost(tester->handle);

    return ret;
}

static int32_t TestSdioFixedAddrReadAndWriteOtherBytes(struct SdioTester *tester)
{
    int32_t ret;
    uint8_t data[TEST_DATA_LEN] = {0};
    const uint32_t addr = TEST_SDIO_BASE_ADDR * TEST_FUNC_NUM + TEST_FIXED_OFFSET + TEST_ADDR_ADD;

    /* read bits */
    ret = SdioReadBytes(tester->handle, &data[0], addr, 1);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("TestSdioFixedAddrReadAndWriteOtherBytes: SdioReadBytesFromFixedAddr fail, ret = %d!", ret);
        return HDF_FAILURE;
    }
    HDF_LOGD("TestSdioFixedAddrReadAndWriteOtherBytes: read, data[0]:%u, data[1]:%u!\n", data[0], data[1]);
    /* write bits */
    ret = SdioWriteBytes(tester->handle, &data[0], addr, 1);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("TestSdioFixedAddrReadAndWriteOtherBytes: SdioWriteBytesToFixedAddr fail, ret = %d!", ret);
        return HDF_FAILURE;
    }
    HDF_LOGD("TestSdioFixedAddrReadAndWriteOtherBytes: write, data[0]:%u, data[1]:%u!", data[0], data[1]);
    return ret;
}

static int32_t TestSdioFixedAddrReadAndWriteOneByte(struct SdioTester *tester)
{
    int32_t ret;
    uint32_t addr;
    uint8_t val;

    addr = TEST_SDIO_BASE_ADDR * TEST_FUNC_NUM + TEST_FIXED_OFFSET;
    /* read 1 bits */
    ret = SdioReadBytes(tester->handle, &val, addr, 1);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("TestSdioFixedAddrReadAndWriteOneByte: SdioReadBytesFromFixedAddr fail, ret = %d!", ret);
        return HDF_FAILURE;
    }
    HDF_LOGD("TestSdioFixedAddrReadAndWriteOneByte: read, val:%d\n", val);
    /* write 1 bits */
    ret = SdioWriteBytes(tester->handle, &val, addr, 1);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("TestSdioFixedAddrReadAndWriteOneByte: SdioWriteBytesToFixedAddr fail, ret = %d!", ret);
        return HDF_FAILURE;
    }
    HDF_LOGD("TestSdioFixedAddrReadAndWriteOneByte: write, val:%d.", val);
    /* read 1 bits */
    addr++;
    ret = SdioReadBytes(tester->handle, &val, addr, 1);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("TestSdioFixedAddrReadAndWriteOneByte: SdioReadBytesFromFixedAddr fail, ret = %d!", ret);
        return HDF_FAILURE;
    }
    HDF_LOGD("TestSdioFixedAddrReadAndWriteOneByte: read, val:%d.", val);
    /* read 1 bits */
    addr++;
    ret = SdioWriteBytes(tester->handle, &val, addr, 1);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("TestSdioFixedAddrReadAndWriteOneByte: SdioReadBytesFromFixedAddr fail, ret = %d!", ret);
        return HDF_FAILURE;
    }
    HDF_LOGD("TestSdioFixedAddrReadAndWriteOneByte: read, val:%d!", val);

    return ret;
}

static int32_t TestSdioFixedAddrReadAndWriteBytes(struct SdioTester *tester)
{
    int32_t ret;

    SdioClaimHost(tester->handle);
    ret = TestSdioFixedAddrReadAndWriteOtherBytes(tester);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("TestSdioFixedAddrReadAndWriteBytes: TestSdioFixedAddrReadAndWriteOtherBytes fail, ret = %d!", ret);
        SdioReleaseHost(tester->handle);
        return HDF_FAILURE;
    }
    ret = TestSdioFixedAddrReadAndWriteOneByte(tester);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("TestSdioFixedAddrReadAndWriteBytes: TestSdioFixedAddrReadAndWriteOneByte fail, ret = %d!", ret);
        SdioReleaseHost(tester->handle);
        return HDF_FAILURE;
    }
    SdioReleaseHost(tester->handle);

    return ret;
}

static int32_t TestSdioFunc0ReadAndWriteBytes(struct SdioTester *tester)
{
    int32_t ret;
    uint8_t val;

    SdioClaimHost(tester->handle);
    /* read sdio rev */
    ret = SdioReadBytesFromFunc0(tester->handle, &val, TEST_FUNC0_ADDR, 1);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("TestSdioFunc0ReadAndWriteBytes: SdioReadBytesFromFunc0 fail, ret = %d!", ret);
        SdioReleaseHost(tester->handle);
        return HDF_FAILURE;
    }
    HDF_LOGD("TestSdioFunc0ReadAndWriteBytes: Func0 Read, val :%d.", val);

    /* write sdio rev */
    ret = SdioWriteBytesToFunc0(tester->handle, &val, TEST_FUNC0_ADDR, 1);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("TestSdioFunc0ReadAndWriteBytes: SdioWriteBytesToFunc0 fail, ret = %d!", ret);
        SdioReleaseHost(tester->handle);
        return HDF_FAILURE;
    }

    /* read sdio rev again */
    ret = SdioReadBytesFromFunc0(tester->handle, &val, TEST_FUNC0_ADDR, 1);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("TestSdioFunc0ReadAndWriteBytes: SdioReadBytesFromFunc0 fail, ret = %d!", ret);
        SdioReleaseHost(tester->handle);
        return HDF_FAILURE;
    }
    SdioReleaseHost(tester->handle);

    HDF_LOGD("TestSdioFunc0ReadAndWriteBytes: Func0 Read, val:%u!", val);
    return ret;
}

static int32_t TestSdioSetAndGetFuncInfo(struct SdioTester *tester)
{
    int32_t ret;
    SdioCommonInfo info = {0};

    ret = SdioGetCommonInfo(tester->handle, &info, SDIO_FUNC_INFO);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("TestSdioSetAndGetFuncInfo: SdioGetCommonInfo fail, ret = %d!", ret);
        return ret;
    }
    HDF_LOGE("TestSdioSetAndGetFuncInfo: success! Timeout = %u!", info.funcInfo.enTimeout);

    info.funcInfo.enTimeout = TEST_TIME_OUT;
    ret = SdioSetCommonInfo(tester->handle, &info, SDIO_FUNC_INFO);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("TestSdioSetAndGetFuncInfo: SdioSetCommonInfo fail! ret = %d!", ret);
        return ret;
    }

    ret = SdioGetCommonInfo(tester->handle, &info, SDIO_FUNC_INFO);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("TestSdioSetAndGetFuncInfo: SdioGetCommonInfo fail, ret = %d!", ret);
        return ret;
    }
    HDF_LOGD("TestSdioSetAndGetFuncInfo: again success, Timeout = %u!", info.funcInfo.enTimeout);

    return HDF_SUCCESS;
}

static int32_t TestSdioSetAndGetCommonInfo(struct SdioTester *tester)
{
    int32_t ret;

    SdioClaimHost(tester->handle);
    ret = TestSdioSetAndGetFuncInfo(tester);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("TestSdioSetAndGetCommonInfo: TestSdioSetAndGetFuncInfo fail, ret = %d!", ret);
        SdioReleaseHost(tester->handle);
        return HDF_FAILURE;
    }
    SdioReleaseHost(tester->handle);
    return ret;
}

static int32_t TestSdioSetBlockSize(struct SdioTester *tester)
{
    int32_t ret;
    SdioClaimHost(tester->handle);
    ret = SdioSetBlockSize(tester->handle, TEST_BLOCKSIZE);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("TestSdioSetBlockSize: SdioSetBlockSize fail, ret = %d!", ret);
        SdioReleaseHost(tester->handle);
        return HDF_FAILURE;
    }
    SdioReleaseHost(tester->handle);
    return ret;
}

static int32_t TestSdioEnableFunc(struct SdioTester *tester)
{
    int32_t ret;

    SdioClaimHost(tester->handle);
    ret = SdioEnableFunc(tester->handle);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("TestSdioEnableFunc: SdioEnableFunc fail, ret = %d!", ret);
        SdioReleaseHost(tester->handle);
        return HDF_FAILURE;
    }
    SdioReleaseHost(tester->handle);
    return ret;
}

static int32_t TestSdioDisableFunc(struct SdioTester *tester)
{
    int32_t ret;

    SdioClaimHost(tester->handle);
    ret = SdioDisableFunc(tester->handle);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("TestSdioDisableFunc: SdioDisableFunc fail, ret = %d!", ret);
        SdioReleaseHost(tester->handle);
        return HDF_FAILURE;
    }
    SdioReleaseHost(tester->handle);
    return ret;
}

struct SdioTestFunc g_sdioTestFunc[] = {
    { SDIO_DISABLE_FUNC_01, TestSdioDisableFunc },
    { SDIO_ENABLE_FUNC_01, TestSdioEnableFunc },
    { SDIO_SET_BLOCK_SIZE_01, TestSdioSetBlockSize },
    { SDIO_INCR_ADDR_READ_AND_WRITE_BYTES_01, TestSdioIncrAddrReadAndWriteBytes },
    { SDIO_FIXED_ADDR_READ_AND_WRITE_BYTES_01, TestSdioFixedAddrReadAndWriteBytes },
    { SDIO_FUNC0_READ_AND_WRITE_BYTES_01, TestSdioFunc0ReadAndWriteBytes },
    { SDIO_SET_AND_GET_COMMON_INFO_01, TestSdioSetAndGetCommonInfo },
};

static int32_t SdioTestEntry(struct SdioTester *tester, int32_t cmd)
{
    int32_t i;
    int32_t ret = HDF_SUCCESS;
    bool isFind = false;

    if (tester == NULL) {
        HDF_LOGE("SdioTestEntry: tester is null!");
        return HDF_ERR_INVALID_OBJECT;
    }
    tester->handle = SdioTestGetHandle(tester);
    if (tester->handle == NULL) {
        HDF_LOGE("SdioTestEntry: sdio test get handle fail!");
        return HDF_FAILURE;
    }
    for (i = 0; i < sizeof(g_sdioTestFunc) / sizeof(g_sdioTestFunc[0]); i++) {
        if (cmd == g_sdioTestFunc[i].type && g_sdioTestFunc[i].Func != NULL) {
            ret = g_sdioTestFunc[i].Func(tester);
            isFind = true;
            break;
        }
    }
    if (!isFind) {
        ret = HDF_ERR_NOT_SUPPORT;
        HDF_LOGE("SdioTestEntry: cmd %d is not support!", cmd);
    }
    SdioTestReleaseHandle(tester->handle);
    return ret;
}

static int32_t SdioTestFillConfig(struct SdioTester *tester, const struct DeviceResourceNode *node)
{
    int32_t ret;
    struct DeviceResourceIface *drsOps = NULL;

    drsOps = DeviceResourceGetIfaceInstance(HDF_CONFIG_SOURCE);
    if (drsOps == NULL || drsOps->GetUint32 == NULL) {
        HDF_LOGE("SdioTestFillConfig: invalid drs ops fail!");
        return HDF_FAILURE;
    }

    ret = drsOps->GetUint32(node, "busNum", &(tester->busNum), 0);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("SdioTestFillConfig: fill bus num fail, ret: %d", ret);
        return ret;
    }

    ret = drsOps->GetUint32(node, "funcNum", &(tester->config.funcNr), 0);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("SdioTestFillConfig: fill funcNum fail, ret: %d", ret);
        return ret;
    }
    ret = drsOps->GetUint16(node, "vendorId", &(tester->config.vendorId), 0);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("SdioTestFillConfig: fill vendorId fail, ret: %d", ret);
        return ret;
    }
    ret = drsOps->GetUint16(node, "deviceId", &(tester->config.deviceId), 0);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("SdioTestFillConfig: fill deviceId fail, ret: %d", ret);
        return ret;
    }

    HDF_LOGE("SdioTestFillConfig: busNum:%u, funcNum:%u, vendorId:0x%x, deviceId:0x%x!",
        tester->busNum, tester->config.funcNr, tester->config.vendorId, tester->config.deviceId);
    return HDF_SUCCESS;
}

static int32_t SdioTestBind(struct HdfDeviceObject *device)
{
    static struct SdioTester tester;

    if (device == NULL) {
        HDF_LOGE("SdioTestBind: device is null!");
        return HDF_ERR_IO;
    }

    device->service = &tester.service;
    HDF_LOGE("SdioTestBind: Sdio_TEST service init success!");
    return HDF_SUCCESS;
}

static int32_t SdioTestInit(struct HdfDeviceObject *device)
{
    struct SdioTester *tester = NULL;
    int32_t ret;

    if (device == NULL || device->service == NULL || device->property == NULL) {
        HDF_LOGE("SdioTestInit: invalid parameter!");
        return HDF_ERR_INVALID_PARAM;
    }

    tester = (struct SdioTester *)device->service;
    ret = SdioTestFillConfig(tester, device->property);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("SdioTestInit: read config fail, ret: %d!", ret);
        return ret;
    }
    tester->TestEntry = SdioTestEntry;
    HDF_LOGE("SdioTestInit: success!");
    return HDF_SUCCESS;
}

static void SdioTestRelease(struct HdfDeviceObject *device)
{
    if (device != NULL) {
        device->service = NULL;
    }
}

struct HdfDriverEntry g_sdioTestEntry = {
    .moduleVersion = 1,
    .Bind = SdioTestBind,
    .Init = SdioTestInit,
    .Release = SdioTestRelease,
    .moduleName = "PLATFORM_SDIO_TEST",
};
HDF_INIT(g_sdioTestEntry);
