/*
 * i2c_adapter_dummy.c
 *
 * driver for i2c dummy adapter
 *
 * Copyright (c) 2023 Huawei Device Co., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 */
#include <securec.h>
#include <linux/i2c.h>

#include "hdf_device_desc.h"
#include "hdf_log.h"
#include "i2c_core.h"
#include "osal_mem.h"
#include "i2c_msg.h"

#define HDF_LOG_TAG      i2c_dummy
#define HDF_DUMMY_I2C_ID 15

static int32_t EchoI2cMsgs(struct I2cMsg *msgs, int16_t count)
{
    int16_t i;
    uint8_t *dummySendBuf = NULL;
    struct I2cMsg *msg = NULL;
    uint16_t dummySendBufSize;

    for (i = 0; i < count; i++) {
        msg = msgs + i;
        dummySendBufSize = msg->len;
        dummySendBuf = (uint8_t *)OsalMemCalloc(sizeof(uint8_t) * dummySendBufSize);
        if (dummySendBuf == NULL) {
            HDF_LOGE("%s: malloc linux msgs fail!", __func__);
            return HDF_ERR_MALLOC_FAIL;
        }

        (void)memcpy_s(dummySendBuf, dummySendBufSize, msg->buf, msg->len);
        if (msg->flags > 0) {
            (void)memset_s(msg->buf, msg->len, msg->len, 0xff);
        }
        OsalMemFree(dummySendBuf);
    }
    return count;
}

static int32_t DummyI2cTransEcho(struct I2cCntlr *cntlr, struct I2cMsg *msgs, int16_t count)
{
    if (cntlr == NULL || cntlr->priv == NULL) {
        HDF_LOGE("%s: cntlr or priv is null!", __func__);
        return HDF_ERR_INVALID_OBJECT;
    }
    if (msgs == NULL || count <= 0) {
        HDF_LOGE("%s: err params! count:%d", __func__, count);
        return HDF_ERR_INVALID_PARAM;
    }

    return EchoI2cMsgs(msgs, count);
}

static struct I2cMethod g_dummyMethod = {
    .transfer = DummyI2cTransEcho,
};

static int32_t DummyI2cBind(struct HdfDeviceObject *device)
{
    (void)device;
    return HDF_SUCCESS;
}

static int32_t DummyI2cInit(struct HdfDeviceObject *device)
{
    int32_t ret;
    struct I2cCntlr *cntlr = NULL;

    if (device == NULL) {
        return HDF_ERR_INVALID_OBJECT;
    }
    cntlr = (struct I2cCntlr *)OsalMemCalloc(sizeof(*cntlr));
    if (cntlr == NULL) {
        HDF_LOGE("%s: malloc cntlr fail!", __func__);
        return HDF_ERR_MALLOC_FAIL;
    }

    cntlr->busId = HDF_DUMMY_I2C_ID;
    cntlr->priv = device;
    cntlr->ops = &g_dummyMethod;

    ret = I2cCntlrAdd(cntlr);
    if (ret != HDF_SUCCESS) {
        HDF_LOGI("%s: failed to add i2c dummy adapter %d", __func__, cntlr->busId);
        OsalMemFree(cntlr);
        return ret;
    }
    device->priv = cntlr;
    HDF_LOGE("%s: i2c dummy adapter %d add success", __func__, cntlr->busId);
    return HDF_SUCCESS;
}

static void DummyI2cRelease(struct HdfDeviceObject *device)
{
    struct I2cCntlr *cntlr = NULL;
    if (device == NULL) {
        return;
    }

    cntlr = (struct I2cCntlr *)device->priv;
    if (cntlr != NULL) {
        I2cCntlrPut(cntlr);
        I2cCntlrRemove(cntlr);
        OsalMemFree(cntlr);
    }
    device->priv = NULL;
}

struct HdfDriverEntry g_dummyLinuxDriverEntry = {
    .moduleVersion = 1,
    .Bind = DummyI2cBind,
    .Init = DummyI2cInit,
    .Release = DummyI2cRelease,
    .moduleName = "dummy_i2c_adapter",
};
HDF_INIT(g_dummyLinuxDriverEntry);
