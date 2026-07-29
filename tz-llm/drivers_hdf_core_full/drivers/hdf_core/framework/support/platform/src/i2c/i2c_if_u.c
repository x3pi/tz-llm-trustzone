/*
 * Copyright (c) 2022-2023 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include "hdf_base.h"
#include "hdf_io_service_if.h"
#include "hdf_log.h"
#include "i2c_if.h"
#include "i2c_service.h"
#include "osal_mem.h"
#include "osal_mutex.h"
#include "securec.h"

#define HDF_LOG_TAG ui2c

#define I2C_SERVICE_NAME   "HDF_PLATFORM_I2C_MANAGER"
#define I2C_BUFF_SIZE      128
#define I2C_MSG_COUNT_MAX  2

struct I2cManagerService {
    struct HdfIoService *ioService;
};

struct I2cHandle {
    DevHandle handle;
    struct I2cManagerService *i2cManager;
    struct OsalMutex lock;
    struct HdfSBuf *data;
    struct HdfSBuf *reply;
};

static void I2cHandleRelease(struct I2cHandle *handle)
{
    OsalMutexDestroy(&handle->lock);
    HdfSbufRecycle(handle->data);
    HdfSbufRecycle(handle->reply);
    handle->data = NULL;
    handle->reply = NULL;
    OsalMemFree(handle);
}

static struct I2cHandle *I2cHandleInstance(struct I2cManagerService *manager)
{
    struct I2cHandle *handle = OsalMemCalloc(sizeof(struct I2cHandle));

    if (handle == NULL) {
        HDF_LOGE("I2cHandleInstance: handle is null!");
        return NULL;
    }

    if (OsalMutexInit(&handle->lock) != HDF_SUCCESS) {
        OsalMemFree(handle);
        HDF_LOGE("I2cHandleInstance: mutex init fail!");
        return NULL;
    }

    handle->data = HdfSbufObtain(I2C_BUFF_SIZE);
    handle->reply = HdfSbufObtain(I2C_BUFF_SIZE);
    if (handle->data == NULL || handle->reply == NULL) {
        HDF_LOGE("I2cHandleInstance: fail to obtain data or reply!");
        I2cHandleRelease(handle);
    }
    handle->i2cManager = manager;
    return handle;
}

static int32_t I2cHandleSbufCheckResize(struct HdfSBuf **sbuf)
{
    struct HdfSBuf *buf = *sbuf;
    int32_t ret = HDF_SUCCESS;

    if (buf != NULL && HdfSbufGetCapacity(buf) <= I2C_BUFF_SIZE) {
        return HDF_SUCCESS;
    }

    HdfSbufRecycle(buf);
    buf = HdfSbufObtain(I2C_BUFF_SIZE);
    if (buf == NULL) {
        HDF_LOGE("I2cHandleSbufCheckResize: fail to obtain buf!");
        ret = HDF_ERR_MALLOC_FAIL;
    }
    *sbuf = buf;
    return ret;
}

static int32_t I2cHandleSbufReset(struct I2cHandle *i2cHandle)
{
    if (I2cHandleSbufCheckResize(&i2cHandle->data) != HDF_SUCCESS ||
        I2cHandleSbufCheckResize(&i2cHandle->reply) != HDF_SUCCESS) {
        HDF_LOGE("I2cHandleSbufReset: sbuf check resize data or reply fail!");
        return HDF_ERR_MALLOC_FAIL;
    }

    HdfSbufFlush(i2cHandle->data);
    HdfSbufFlush(i2cHandle->reply);

    return HDF_SUCCESS;
}

static struct I2cManagerService *I2cManagerGetService(void)
{
    static struct I2cManagerService manager;

    if (manager.ioService != NULL) {
        return &manager;
    }

    manager.ioService = HdfIoServiceBind(I2C_SERVICE_NAME);
    if (manager.ioService == NULL) {
        HDF_LOGE("I2cManagerGetService: fail to get i2c IoService!");
        return NULL;
    }

    return &manager;
}

DevHandle I2cOpen(int16_t number)
{
    struct I2cManagerService *i2cManager = I2cManagerGetService();

    if (i2cManager == NULL) {
        HDF_LOGE("I2cOpen: i2c manager is invalid!");
        return NULL;
    }

    struct I2cHandle *i2cHandle = I2cHandleInstance(i2cManager);
    if (i2cHandle == NULL) {
        HDF_LOGE("I2cOpen: i2cHandle is null!");
        return NULL;
    }

    do {
        if (I2cHandleSbufReset(i2cHandle) != HDF_SUCCESS) {
            HDF_LOGE("I2cOpen: fail to reset sbuf!");
            break;
        }

        if (!HdfSbufWriteUint16(i2cHandle->data, (uint16_t)number)) {
            HDF_LOGE("I2cOpen: write number fail!");
            break;
        }

        int32_t ret = HdfIoServiceDispatch(i2cManager->ioService, I2C_IO_OPEN, i2cHandle->data, i2cHandle->reply);
        if (ret != HDF_SUCCESS) {
            HDF_LOGE("I2cOpen: service call open fail, ret: %d!", ret);
            break;
        }
        uint32_t handle = 0;
        if (!HdfSbufReadUint32(i2cHandle->reply, &handle)) {
            HDF_LOGE("I2cOpen: read handle fail!");
            break;
        }
        DevHandle devHandle = (DevHandle)(uintptr_t)handle;
        i2cHandle->handle = devHandle;
        return (DevHandle)i2cHandle;
    } while (0);

    I2cHandleRelease(i2cHandle);
    return NULL;
}

static inline bool IsI2cManagerValid(struct I2cManagerService *i2cManager)
{
    return i2cManager != NULL && i2cManager->ioService != NULL;
}

static inline bool IsI2cHandleValid(struct I2cHandle *i2cHandle)
{
    return i2cHandle != NULL && i2cHandle->data != NULL && i2cHandle->reply != NULL &&
        IsI2cManagerValid(i2cHandle->i2cManager);
}

void I2cClose(DevHandle handle)
{
    if (handle == NULL) {
        return;
    }
    struct I2cHandle *i2cHandle = (struct I2cHandle *)handle;
    if (!IsI2cHandleValid(i2cHandle)) {
        HDF_LOGE("I2cClose: invalid i2c handle!");
        return;
    }

    if (I2cHandleSbufReset(i2cHandle)) {
        HDF_LOGE("I2cClose: fail to reset sbuf!");
        return;
    }

    if (!HdfSbufWriteUint32(i2cHandle->data, (uint32_t)(uintptr_t)i2cHandle->handle)) {
        HDF_LOGE("I2cClose: failed to write handle!");
        return;
    }

    int32_t ret = HdfIoServiceDispatch(i2cHandle->i2cManager->ioService, I2C_IO_CLOSE, i2cHandle->data, NULL);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("I2cClose: close handle fail, ret: %d!", ret);
    }
    I2cHandleRelease(i2cHandle);
}

static int32_t WriteI2cMsgs(struct HdfSBuf *data, struct I2cMsg *msgs, int16_t count)
{
    int16_t i;
    struct I2cUserMsg userMsgs = {0};

    if (!HdfSbufWriteInt16(data, count)) {
        HDF_LOGE("WriteI2cMsgs: write count fail!");
        return HDF_ERR_IO;
    }

    for (i = 0; i < count; i++) {
        userMsgs.addr = msgs[i].addr;
        userMsgs.len = msgs[i].len;
        userMsgs.flags = msgs[i].flags;
        if (!HdfSbufWriteBuffer(data, &userMsgs, sizeof(struct I2cUserMsg))) {
            HDF_LOGE("WriteI2cMsgs: write userMsgs[%hd] buf fail!", i);
            return HDF_ERR_IO;
        }

        if (msgs[i].len > I2C_BUFF_SIZE ||
            ((msgs[i].flags & I2C_FLAG_READ) && (msgs[i].len + sizeof(uint64_t) > I2C_BUFF_SIZE))) {
            HDF_LOGE("WriteI2cMsgs: msg data size %{public}u out of range!", msgs[i].len);
            return HDF_ERR_OUT_OF_RANGE;
        }

        if (!HdfSbufWriteBuffer(data, (uint8_t *)msgs[i].buf, msgs[i].len)) {
            HDF_LOGE("WriteI2cMsgs: fail to write msg[%hd] buf!", i);
            return HDF_ERR_IO;
        }
    }

    return HDF_SUCCESS;
}

static int32_t I2cMsgReadBack(struct HdfSBuf *data, struct I2cMsg *msg)
{
    uint32_t rLen;
    const void *rBuf = NULL;

    if ((msg->flags & I2C_FLAG_READ) == 0) {
        return HDF_SUCCESS; /* write msg no need to read back */
    }

    if (!HdfSbufReadBuffer(data, &rBuf, &rLen)) {
        HDF_LOGE("I2cMsgReadBack: read rBuf fail!");
        return HDF_ERR_IO;
    }
    if (msg->len != rLen) {
        HDF_LOGW("I2cMsgReadBack: err len:%u, rLen:%u!", msg->len, rLen);
        if (rLen > msg->len) {
            rLen = msg->len;
        }
    }
    if (memcpy_s(msg->buf, msg->len, rBuf, rLen) != EOK) {
        HDF_LOGE("I2cMsgReadBack: memcpy rBuf fail!");
        return HDF_ERR_IO;
    }

    return HDF_SUCCESS;
}

static int32_t ReadI2cMsgs(struct HdfSBuf *reply, struct I2cMsg *msgs, int16_t count)
{
    for (int16_t i = 0; i < count; i++) {
        if ((msgs[i].flags & I2C_FLAG_READ) == 0) {
            continue; /* write msg no need to read back */
        }

        int32_t ret = I2cMsgReadBack(reply, &msgs[i]);
        if (ret != HDF_SUCCESS) {
            HDF_LOGE("ReadI2cMsgs: i2c msg read back fail!");
            return ret;
        }
    }
    return HDF_SUCCESS;
}

// user data format:handle---count---count data records of I2cUserMsg;
static int32_t I2cServiceTransfer(DevHandle handle, struct I2cMsg *msgs, int16_t count)
{
    int32_t ret = 0;
    struct I2cHandle *i2cHandle = (struct I2cHandle *)handle;

    if (!IsI2cHandleValid(i2cHandle)) {
        HDF_LOGE("I2cServiceTransfer: invalid i2c handle!");
        return HDF_ERR_INVALID_OBJECT;
    }
    OsalMutexLock(&i2cHandle->lock);

    if (I2cHandleSbufReset(i2cHandle)) {
        OsalMutexUnlock(&i2cHandle->lock);
        HDF_LOGE("I2cServiceTransfer: fail to reset sbuf!");
        return HDF_ERR_MALLOC_FAIL;
    }

    do {
        if (!HdfSbufWriteUint32(i2cHandle->data, (uint32_t)(uintptr_t)i2cHandle->handle)) {
            HDF_LOGE("I2cServiceTransfer: write handle fail!");
            break;
        }
        if (WriteI2cMsgs(i2cHandle->data, msgs, count) != HDF_SUCCESS) {
            HDF_LOGE("I2cServiceTransfer: fail to write msgs!");
            break;
        }

        struct HdfIoService *ioService = i2cHandle->i2cManager->ioService;
        ret = HdfIoServiceDispatch(ioService, I2C_IO_TRANSFER, i2cHandle->data, i2cHandle->reply);
        if (ret != HDF_SUCCESS) {
            HDF_LOGE("I2cServiceTransfer: fail to send service call, ret: %d!", ret);
            break;
        }

        if (ReadI2cMsgs(i2cHandle->reply, msgs, count) != HDF_SUCCESS) {
            break;
        }

        ret = count;
    } while (0);

    OsalMutexUnlock(&i2cHandle->lock);
    return ret;
}

int32_t I2cTransfer(DevHandle handle, struct I2cMsg *msgs, int16_t count)
{
    if (handle == NULL) {
        HDF_LOGE("I2cTransfer: handle is null!");
        return HDF_ERR_INVALID_OBJECT;
    }

    if (msgs == NULL || count <= 0) {
        HDF_LOGE("I2cTransfer: msg is null or count is invalid!");
        return HDF_ERR_INVALID_PARAM;
    }

    if (count > I2C_MSG_COUNT_MAX) {
        HDF_LOGE("I2cTransfer: too many i2c msg!");
        return HDF_ERR_OUT_OF_RANGE;
    }

    return I2cServiceTransfer(handle, msgs, count);
}
