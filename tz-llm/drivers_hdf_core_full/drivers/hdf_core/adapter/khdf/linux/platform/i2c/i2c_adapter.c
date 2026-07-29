/*
 * i2c_adapter.h
 *
 * i2c driver adapter of linux
 *
 * Copyright (c) 2020-2023 Huawei Device Co., Ltd.
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

#include <linux/i2c.h>
#include "hdf_device_desc.h"
#include "hdf_log.h"
#include "i2c_core.h"
#include "i2c_msg.h"
#include "osal_mem.h"

#define HDF_LOG_TAG i2c_linux_adapter

static struct i2c_msg *CreateLinuxI2cMsgs(struct I2cMsg *msgs, int16_t count)
{
    int16_t i;
    struct i2c_msg *linuxMsgs = NULL;

    linuxMsgs = (struct i2c_msg *)OsalMemCalloc(sizeof(*linuxMsgs) * count);
    if (linuxMsgs == NULL) {
        HDF_LOGE("CreateLinuxI2cMsgs: malloc linux msgs fail!");
        return NULL;
    }

    for (i = 0; i < count; i++) {
        linuxMsgs[i].addr = msgs[i].addr;
        linuxMsgs[i].buf = msgs[i].buf;
        linuxMsgs[i].len = msgs[i].len;
        linuxMsgs[i].flags = msgs[i].flags;
    }
    return linuxMsgs;
}

static inline void FreeLinuxI2cMsgs(struct i2c_msg *msgs, int16_t count)
{
    (void)count;
    OsalMemFree(msgs);
}

static int32_t LinuxI2cTransfer(struct I2cCntlr *cntlr, struct I2cMsg *msgs, int16_t count)
{
    int32_t ret;
    struct i2c_msg *linuxMsgs = NULL;

    if (cntlr == NULL || cntlr->priv == NULL) {
        HDF_LOGE("LinuxI2cTransfer: cntlr or priv is null!");
        return HDF_ERR_INVALID_OBJECT;
    }
    if (msgs == NULL || count <= 0) {
        HDF_LOGE("LinuxI2cTransfer: err params! count:%d!", count);
        return HDF_ERR_INVALID_PARAM;
    }

    linuxMsgs = CreateLinuxI2cMsgs(msgs, count);
    if (linuxMsgs == NULL) {
        HDF_LOGE("LinuxI2cTransfer: linuxMsgs is null!");
        return HDF_ERR_MALLOC_FAIL;
    }

    ret = i2c_transfer((struct i2c_adapter *)cntlr->priv, linuxMsgs, count);
    FreeLinuxI2cMsgs(linuxMsgs, count);
    return ret;
}

static struct I2cMethod g_method = {
    .transfer = LinuxI2cTransfer,
};

static int32_t LinuxI2cBind(struct HdfDeviceObject *device)
{
    (void)device;
    return HDF_SUCCESS;
}

static int LinuxI2cRemove(struct device *dev, void *data)
{
    struct I2cCntlr *cntlr = NULL;
    struct i2c_adapter *adapter = NULL;

    HDF_LOGI("LinuxI2cRemove: enter!");
    (void)data;

    if (dev == NULL) {
        HDF_LOGE("LinuxI2cRemove: dev is null!");
        return HDF_ERR_INVALID_OBJECT;
    }

    if (dev->type != &i2c_adapter_type) {
        return HDF_SUCCESS; // continue remove
    }

    adapter = to_i2c_adapter(dev);
    cntlr = I2cCntlrGet(adapter->nr);
    if (cntlr != NULL) {
        I2cCntlrPut(cntlr);
        I2cCntlrRemove(cntlr);
        i2c_put_adapter(adapter);
        OsalMemFree(cntlr);
    }
    return HDF_SUCCESS;
}

static int LinuxI2cProbe(struct device *dev, void *data)
{
    int32_t ret;
    struct I2cCntlr *cntlr = NULL;
    struct i2c_adapter *adapter = NULL;

    (void)data;

    if (dev == NULL) {
        HDF_LOGE("LinuxI2cProbe: dev is null!");
        return HDF_ERR_INVALID_OBJECT;
    }

    if (dev->type != &i2c_adapter_type) {
        return HDF_SUCCESS; // continue probe
    }

    HDF_LOGI("LinuxI2cProbe: enter!");
    adapter = to_i2c_adapter(dev);
    cntlr = (struct I2cCntlr *)OsalMemCalloc(sizeof(*cntlr));
    if (cntlr == NULL) {
        HDF_LOGE("LinuxI2cProbe: malloc cntlr fail!");
        i2c_put_adapter(adapter);
        return HDF_ERR_MALLOC_FAIL;
    }

    cntlr->busId = adapter->nr;
    cntlr->priv = adapter;
    cntlr->ops = &g_method;
    ret = I2cCntlrAdd(cntlr);
    if (ret != HDF_SUCCESS) {
        i2c_put_adapter(adapter);
        OsalMemFree(cntlr);
        cntlr = NULL;
        HDF_LOGE("LinuxI2cProbe: add controller fail, ret: %d!", ret);
        return ret;
    }
    HDF_LOGI("LinuxI2cProbe: i2c adapter %d add success!", cntlr->busId);
    return HDF_SUCCESS;
}

static int32_t LinuxI2cInit(struct HdfDeviceObject *device)
{
    int32_t ret;

    HDF_LOGI("LinuxI2cInit: enter!");
    if (device == NULL) {
        HDF_LOGE("LinuxI2cInit: device is null!");
        return HDF_ERR_INVALID_OBJECT;
    }

    ret = i2c_for_each_dev(NULL, LinuxI2cProbe);
    HDF_LOGI("LinuxI2cInit: done!");
    return ret;
}

static void LinuxI2cRelease(struct HdfDeviceObject *device)
{
    HDF_LOGI("LinuxI2cRelease: enter");
    if (device == NULL) {
        HDF_LOGE("LinuxI2cRelease: device is null!");
        return;
    }

    (void)i2c_for_each_dev(NULL, LinuxI2cRemove);
}

struct HdfDriverEntry g_i2cLinuxDriverEntry = {
    .moduleVersion = 1,
    .Bind = LinuxI2cBind,
    .Init = LinuxI2cInit,
    .Release = LinuxI2cRelease,
    .moduleName = "linux_i2c_adapter",
};
HDF_INIT(g_i2cLinuxDriverEntry);
