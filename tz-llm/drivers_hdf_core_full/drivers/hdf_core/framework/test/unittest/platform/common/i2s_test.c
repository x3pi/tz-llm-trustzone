/*
 * Copyright (c) 2021-2023 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include "i2s_test.h"
#include <unistd.h>
#include "device_resource_if.h"
#include "hdf_base.h"
#include "hdf_log.h"
#include "i2s_if.h"
#include "osal_file.h"
#include "osal_mem.h"
#include "osal_test_type.h"
#include "osal_time.h"

#define HDF_LOG_TAG i2s_test_c

#define TEST_READ_FILE_PATH_NAME "/nfs/i2s.wav"
#define TEST_WRITE_FILE_PATH_NAME "/nfs/i2s1.wav"
#define I2S_DATA_BUF_SIZE               0x1000

struct I2sTestFunc {
    enum I2sTestCmd type;
    int32_t (*Func)(struct I2sTest *test);
};

static int32_t I2sSetCfgTest(struct I2sTest *test)
{
    if (test == NULL) {
        HDF_LOGE("I2sSetCfgTest: test is null!");
        return HDF_ERR_INVALID_OBJECT;
    }

    HDF_LOGD("I2sSetCfgTest: sampleRate[%u], type[%u], channelMode[%u], samplePrecision[%u], \
        channelIfMode[%u], mclk[%u], bclk[%u], writeChannel[%u], i2slFsSel[%u]",
        test->sampleRate, test->type, test->channelMode, test->samplePrecision,
        test->channelIfMode, test->mclk, test->bclk, test->writeChannel, test->i2slFsSel);

    struct I2sCfg cfg;
    cfg.sampleRate = test->sampleRate;
    cfg.type = test->type;
    cfg.channelMode = test->channelMode;
    cfg.samplePrecision = test->samplePrecision;
    cfg.channelIfMode = test->channelIfMode;
    cfg.mclk = test->mclk;
    cfg.bclk = test->bclk;
    cfg.writeChannel = test->writeChannel;
    cfg.i2slFsSel = test->i2slFsSel;
    cfg.width = I2S_WORDWIDTH_16BIT;

    I2sSetCfg(test->handle, &cfg);
    return HDF_SUCCESS;
}

static int32_t I2sGetCfgTest(struct I2sTest *test)
{
    if (test == NULL) {
        HDF_LOGE("I2sGetCfgTest: test is null!");
        return HDF_ERR_INVALID_OBJECT;
    }

    struct I2sCfg cfg;
    I2sGetCfg(test->handle, &cfg);

    HDF_LOGD("I2sGetCfgTest: sampleRate[%u], type[%u], channelMode[%u], samplePrecision[%u], \
        channelIfMode[%u], mclk[%u], bclk[%u], writeChannel[%u], i2slFsSel[%u]",
        test->sampleRate, test->type, test->channelMode, test->samplePrecision,
        test->channelIfMode, test->mclk, test->bclk, test->writeChannel, test->i2slFsSel);
    return HDF_SUCCESS;
}

static int32_t I2sOpenTest(struct I2sTest *test)
{
    if (test == NULL) {
        HDF_LOGE("I2sOpenTest: test is null!");
        return HDF_ERR_INVALID_OBJECT;
    }

    return HDF_SUCCESS;
}

static int32_t I2sCloseTest(struct I2sTest *test)
{
    if (test == NULL) {
        HDF_LOGE("I2sCloseTest: test is null!");
        return HDF_ERR_INVALID_OBJECT;
    }

    return HDF_SUCCESS;
}

static int32_t I2sEnableTest(struct I2sTest *test)
{
    if (test == NULL || test->handle == NULL || test->wbuf == NULL) {
        HDF_LOGE("I2sEnableTest: test or handle or wbuf is null!");
        return HDF_ERR_INVALID_OBJECT;
    }

    I2sEnable(test->handle);
    return HDF_SUCCESS;
}

static int32_t I2sDisableTest(struct I2sTest *test)
{
    if (test == NULL || test->handle == NULL || test->wbuf == NULL) {
        HDF_LOGE("I2sDisableTest: test or handle or wbuf is null!");
        return HDF_ERR_INVALID_OBJECT;
    }

    I2sDisable(test->handle);
    return HDF_SUCCESS;
}

#define I2S_WRITE_BUFF_SIZE 0x2000
static int32_t I2sPlayTest(struct I2sTest *test)
{
    if (test == NULL || test->handle == NULL || test->wbuf == NULL) {
        HDF_LOGE("I2sPlayTest: test or handle or wbuf is null!");
        return HDF_ERR_INVALID_OBJECT;
    }

    OsalFile file;
    int32_t size = OsalFileOpen(&file, TEST_WRITE_FILE_PATH_NAME, O_CREAT | OSAL_O_RDWR, OSAL_S_IREAD);
    if (size < 0) {
        printf("[I2sPlayTest] OsalFileOpen ret[%d] error!\n", size);
        return HDF_FAILURE;
    }

    uint32_t readBuff = I2S_WRITE_BUFF_SIZE;
    do {
        size = OsalFileRead(&file, test->wbuf, readBuff);
        printf("[I2sPlayTest] read file size[%d]", size);
        if (size > 0) {
            uint32_t wlen = 0;
            int ret = I2sWrite(test->handle, test->wbuf, size, &wlen);
            if (ret != HDF_SUCCESS) {
                HDF_LOGE("I2sPlayTest: I2sPlayTest error!");
                return HDF_FAILURE;
            }
            printf("[I2sPlayTest] [%d] I2sPlayTest wlen[%u]\n", ret, wlen);
        }
    } while (size > 0);

    OsalFileClose(&file);
    return HDF_SUCCESS;
}

#define READ_TEST_SLEEP        2
#define READ_TEST_TIMES        1000
#define READ_TEST_FILE_SIZE    (0x4000 * 10000)

static int32_t I2sRecordTest(struct I2sTest *test)
{
    if (test == NULL || test->handle == NULL || test->rbuf == NULL) {
        HDF_LOGE("I2sRecordTest: test or handle or rbuf is null!");
        return HDF_ERR_INVALID_OBJECT;
    }

    OsalFile file;
    int32_t ret = OsalFileOpen(&file, TEST_READ_FILE_PATH_NAME, O_CREAT | OSAL_O_RDWR, OSAL_S_IWRITE);
    if (ret < 0) {
        HDF_LOGE("[I2sRecordTest] OsalFileOpen ret[%d] error!\n", ret);
        return HDF_FAILURE;
    }

    int i = 0;
    uint32_t totalLen = 0;
    while ((i <= READ_TEST_TIMES) && (totalLen <= READ_TEST_FILE_SIZE)) {
        test->len = I2S_DATA_BUF_SIZE;
        (void)memset_s(test->rbuf, I2S_DATA_BUF_SIZE, 0xee, I2S_DATA_BUF_SIZE);
        if (I2sRead(test->handle, test->rbuf, test->len, &test->len) != HDF_SUCCESS) {
            HDF_LOGE("I2sRecordTest: I2sRecordTest error!\n");
            return HDF_FAILURE;
        }
        if (test->len == 0) {
            HDF_LOGD("I2sRecordTest: not available data!\n");
        } else {
            totalLen += test->len;
            ret = OsalFileWrite(&file, test->rbuf, test->len);
            if (ret < -1) {
                HDF_LOGE("I2sRecordTest: write file error!\n");
                OsalFileClose(&file);
                return HDF_FAILURE;
            }
        }

        i++;
    }

    OsalFileClose(&file);
    return HDF_SUCCESS;
}

static int32_t I2sReadTest(struct I2sTest *test)
{
    if (test == NULL || test->handle == NULL || test->rbuf == NULL  || test->wbuf == NULL) {
        HDF_LOGE("I2sReadTest: test or handle or rbuf or wbuf is null!");
        return HDF_ERR_INVALID_OBJECT;
    }

    if (I2sRead(test->handle, test->rbuf, test->len, &test->len) != HDF_SUCCESS) {
        HDF_LOGE("I2sReadTest: I2sRead error!\n");
        return HDF_FAILURE;
    }

    if (test->len > I2S_DATA_BUF_SIZE) {
        HDF_LOGE("I2sReadTest: I2sRead read data too large!\n");
        return HDF_FAILURE;
    }

    if (memcpy_s(test->wbuf, I2S_DATA_BUF_SIZE, test->rbuf, test->len) != EOK) {
        HDF_LOGE("I2sReadTest: memcpy buf fail!");
        return HDF_ERR_IO;
    }

    return HDF_SUCCESS;
}

static int32_t I2sWriteTest(struct I2sTest *test)
{
    if (test == NULL || test->handle == NULL || test->wbuf == NULL) {
        HDF_LOGE("I2sWriteTest: test or handle or wbuf is null!");
        return HDF_ERR_INVALID_OBJECT;
    }

    if (I2sWrite(test->handle, test->wbuf, test->len, &test->len) != HDF_SUCCESS) {
        HDF_LOGE("I2sWriteTest: I2sWriteTest error!\n");
        return HDF_FAILURE;
    }

    return HDF_SUCCESS;
}


static int32_t I2sWriteStartTest(struct I2sTest *test)
{
    if (test == NULL || test->handle == NULL) {
        HDF_LOGE("I2sWriteStartTest: test or handle is null!");
        return HDF_ERR_INVALID_OBJECT;
    }

    I2sStartWrite(test->handle);
    return HDF_SUCCESS;
}

static int32_t I2sReadStartTest(struct I2sTest *test)
{
    if (test == NULL || test->handle == NULL) {
        HDF_LOGE("I2sReadStartTest: test or handle is null!");
        return HDF_ERR_INVALID_OBJECT;
    }
    
    I2sStartRead(test->handle);
    if (test->rbuf != NULL) {
        HDF_LOGI("I2sReadStartTest: rbuf[0] = [%u]\n", test->rbuf[0]);
    }

    return HDF_SUCCESS;
}

static int32_t I2sWriteStopTest(struct I2sTest *test)
{
    if (test == NULL || test->handle == NULL) {
        HDF_LOGE("I2sWriteStopTest: test or handle is null!");
        return HDF_ERR_INVALID_OBJECT;
    }

    I2sStopWrite(test->handle);
    return HDF_SUCCESS;
}

static int32_t I2sReadStopTest(struct I2sTest *test)
{
    if (test == NULL || test->handle == NULL) {
        HDF_LOGE("I2sReadStopTest: test or handle is null!");
        return HDF_ERR_INVALID_OBJECT;
    }
    
    I2sStopRead(test->handle);
    if (test->rbuf != NULL) {
        HDF_LOGI("I2sReadStopTest: rbuf[0] = [%u]\n", test->rbuf[0]);
    }

    return HDF_SUCCESS;
}

static int32_t I2sReliabilityTest(struct I2sTest *test)
{
    if (test == NULL || test->handle == NULL) {
        HDF_LOGE("I2sReliabilityTest: test or handle is null!");
        return HDF_ERR_INVALID_OBJECT;
    }

    (void)I2sSetCfg(test->handle, NULL);
    (void)I2sReadTest(NULL);

    (void)test;
    HDF_LOGD("I2sReliabilityTest: success!");
    return HDF_SUCCESS;
}

static struct I2sTestFunc g_i2sTestFunc[] = {
    {I2S_SET_CFG_TEST, I2sSetCfgTest},
    {I2S_GET_CFG_TEST, I2sGetCfgTest},
    {I2S_OPEN_TEST, I2sOpenTest},
    {I2S_CLOSE_TEST, I2sCloseTest},
    {I2S_ENABLE_TEST, I2sEnableTest},
    {I2S_DISABLE_TEST, I2sDisableTest},
    {I2S_WRITE_START_TEST, I2sWriteStartTest},
    {I2S_READ_START_TEST, I2sReadStartTest},
    {I2S_WRITE_TEST, I2sWriteTest},
    {I2S_READ_TEST, I2sReadTest},
    {I2S_WRITE_STOP_TEST, I2sWriteStopTest},
    {I2S_READ_STOP_TEST, I2sReadStopTest},
    {I2S_RELIABILITY_TEST, I2sReliabilityTest},
    {I2S_RECORD_TEST, I2sRecordTest},
    {I2S_PLAY_TEST, I2sPlayTest},
};

static int32_t I2sTestEntry(struct I2sTest *test, int32_t cmd)
{
    int32_t i;
    int32_t ret = HDF_ERR_NOT_SUPPORT;

    HDF_LOGE("I2s test-- -- -- -- -- -->I2sTestEntry: enter cmd %d", cmd);

    if (test == NULL) {
        HDF_LOGE("I2sTestEntry: test is null, cmd %d!", cmd);
        return HDF_ERR_INVALID_OBJECT;
    }

    test->handle = I2sOpen(0);
    if (test->handle == NULL) {
        HDF_LOGE("I2sTestEntry: i2s test get handle fail!");
        return HDF_FAILURE;
    }

    for (i = 0; i < sizeof(g_i2sTestFunc) / sizeof(g_i2sTestFunc[0]); i++) {
        if (cmd == g_i2sTestFunc[i].type && g_i2sTestFunc[i].Func != NULL) {
            ret = g_i2sTestFunc[i].Func(test);
            HDF_LOGE("I2sTestEntry: cmd %d ret %d", cmd, ret);
            break;
        }
    }

    I2sClose(test->handle);
    return ret;
}

static int32_t I2sTestBind(struct HdfDeviceObject *device)
{
    static struct I2sTest test;

    if (device != NULL) {
        device->service = &test.service;
    } else {
        HDF_LOGE("I2sTestBind: device is null!");
    }
    return HDF_SUCCESS;
}

static int32_t I2sTestInitBuf(struct I2sTest *test)
{
    if (test == NULL) {
        HDF_LOGE("I2sTestInitBuf: test is null!");
        return HDF_FAILURE;
    }

    test->len = I2S_DATA_BUF_SIZE;
    test->wbuf = (uint8_t *)OsalMemCalloc(test->len);
    if (test->wbuf == NULL) {
        HDF_LOGE("I2sTestInitBuf: wbuf OsalMemCalloc error!\n");
        return HDF_ERR_MALLOC_FAIL;
    }

    test->rbuf = (uint8_t *)OsalMemCalloc(test->len);
    if (test->rbuf == NULL) {
        HDF_LOGE("I2sTestInitBuf: rbuf OsalMemCalloc error!\n");
        OsalMemFree(test->wbuf);
        return HDF_ERR_MALLOC_FAIL;
    }

    return HDF_SUCCESS;
}

static int32_t I2sTestInitCodecFromHcs(struct I2sTest *test, const struct DeviceResourceNode *node)
{
    struct DeviceResourceIface *face = NULL;

    face = DeviceResourceGetIfaceInstance(HDF_CONFIG_SOURCE);
    if (face == NULL) {
        HDF_LOGE("I2sTestInitCodecFromHcs: face is null!");
        return HDF_FAILURE;
    }

    int32_t ret = face->GetUint8(node, "writeChannel", &test->writeChannel, 0);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("I2sTestInitCodecFromHcs: read writeChannel fail!");
        return HDF_FAILURE;
    }
    ret = face->GetUint8(node, "i2slFsSel", &test->i2slFsSel, 0);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("I2sTestInitCodecFromHcs: read i2slFsSel fail!");
        return HDF_FAILURE;
    }

    ret = face->GetUint8(node, "channelMode", &test->channelMode, 0);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("I2sTestInitCodecFromHcs: read channelMode fail!");
        return HDF_FAILURE;
    }
    ret = face->GetUint8(node, "channelIfMode", &test->channelIfMode, 0);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("I2sTestInitCodecFromHcs: read channelIfMode fail!");
        return HDF_FAILURE;
    }

    return HDF_SUCCESS;
}

static int32_t I2sTestInitFromHcs(struct I2sTest *test, const struct DeviceResourceNode *node)
{
    int32_t ret;
    struct DeviceResourceIface *face = NULL;

    face = DeviceResourceGetIfaceInstance(HDF_CONFIG_SOURCE);
    if (face == NULL) {
        HDF_LOGE("I2sTestInitFromHcs: face is null!");
        return HDF_FAILURE;
    }
    if (face->GetUint32 == NULL || face->GetUint32Array == NULL) {
        HDF_LOGE("I2sTestInitFromHcs: GetUint32 or GetUint32Array not support!");
        return HDF_ERR_NOT_SUPPORT;
    }
    ret = face->GetUint8(node, "sampleRate", &test->sampleRate, 0);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("I2sTestInitFromHcs: read sampleRate fail!");
        return HDF_FAILURE;
    }
    ret = face->GetUint8(node, "type", &test->type, 0);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("I2sTestInitFromHcs: read type fail!");
        return HDF_FAILURE;
    }
    ret = face->GetUint8(node, "samplePrecision", &test->samplePrecision, 0);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("I2sTestInitFromHcs: read samplePrecision fail!");
        return HDF_FAILURE;
    }
    ret = face->GetUint32(node, "MCLK", &test->mclk, 0);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("I2sTestInitFromHcs: read MCLK fail!");
        return HDF_FAILURE;
    }
    ret = face->GetUint32(node, "BCLK", &test->bclk, 0);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("I2sTestInitFromHcs: read BCLK fail!");
        return HDF_FAILURE;
    }

    if (I2sTestInitCodecFromHcs (test, node) != HDF_SUCCESS) {
        HDF_LOGE("I2sTestInitFromHcs: init codec from hcs fail!");
        return HDF_FAILURE;
    }

    if (I2sTestInitBuf (test) != HDF_SUCCESS) {
        HDF_LOGE("I2sTestInitFromHcs: init buf fail!");
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

static int32_t I2sTestInit(struct HdfDeviceObject *device)
{
    struct I2sTest *test = NULL;

    if (device == NULL || device->service == NULL || device->property == NULL) {
        HDF_LOGE("I2sTestInit: invalid parameter!");
        return HDF_ERR_INVALID_PARAM;
    }
    test = (struct I2sTest *)device->service;
    if (I2sTestInitFromHcs(test, device->property) != HDF_SUCCESS) {
        HDF_LOGE("I2sTestInit: I2sTestInitFromHcs fail!");
        return HDF_FAILURE;
    }

    HDF_LOGD("I2sTestInit: success!");
    test->TestEntry = I2sTestEntry;
    return HDF_SUCCESS;
}

static void I2sTestRelease(struct HdfDeviceObject *device)
{
    struct I2sTest *test = NULL;

    if (device == NULL) {
        HDF_LOGE("I2sTestRelease: device is null!");
        return;
    }
    test = (struct I2sTest *)device->service;
    if (test == NULL) {
        HDF_LOGE("I2sTestRelease: test is null!");
        return;
    }
    if (test->wbuf != NULL) {
        OsalMemFree(test->wbuf);
    }
    if (test->rbuf != NULL) {
        OsalMemFree(test->rbuf);
    }
}

struct HdfDriverEntry g_i2sTestEntry = {
    .moduleVersion = 1,
    .Bind = I2sTestBind,
    .Init = I2sTestInit,
    .Release = I2sTestRelease,
    .moduleName = "PLATFORM_I2S_TEST",
};
HDF_INIT(g_i2sTestEntry);
