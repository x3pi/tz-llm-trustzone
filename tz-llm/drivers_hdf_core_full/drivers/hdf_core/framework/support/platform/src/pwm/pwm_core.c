/*
 * Copyright (c) 2020-2023 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include "pwm_core.h"
#include "hdf_log.h"
#include "securec.h"

#define HDF_LOG_TAG pwm_core

int32_t PwmDeviceGet(struct PwmDev *pwm)
{
    int32_t ret;

    if (pwm == NULL) {
        HDF_LOGE("PwmDeviceGet: pwm is null!\n");
        return HDF_ERR_INVALID_PARAM;
    }

    (void)OsalSpinLock(&(pwm->lock));
    if (pwm->busy) {
        (void)OsalSpinUnlock(&(pwm->lock));
        HDF_LOGE("PwmDeviceGet: pwm%u is busy!", pwm->num);
        return HDF_ERR_DEVICE_BUSY;
    }
    if (pwm->method != NULL && pwm->method->open != NULL) {
        ret = pwm->method->open(pwm);
        if (ret != HDF_SUCCESS) {
            (void)OsalSpinUnlock(&(pwm->lock));
            HDF_LOGE("PwmDeviceGet: open fail, ret: %d!", ret);
            return HDF_FAILURE;
        }
    }

    pwm->busy = true;
    (void)OsalSpinUnlock(&(pwm->lock));
    return HDF_SUCCESS;
}

int32_t PwmDevicePut(struct PwmDev *pwm)
{
    int32_t ret;

    if (pwm == NULL) {
        HDF_LOGE("PwmDevicePut: pwm is null!\n");
        return HDF_ERR_INVALID_PARAM;
    }

    if (pwm->method != NULL && pwm->method->close != NULL) {
        ret = pwm->method->close(pwm);
        if (ret != HDF_SUCCESS) {
            HDF_LOGE("PwmDevicePut: close fail, ret: %d!", ret);
            return ret;
        }
    }
    (void)OsalSpinLock(&(pwm->lock));
    pwm->busy = false;
    (void)OsalSpinUnlock(&(pwm->lock));
    return HDF_SUCCESS;
}

int32_t PwmDeviceSetConfig(struct PwmDev *pwm, struct PwmConfig *config)
{
    int32_t ret;

    if (pwm == NULL || config == NULL) {
        HDF_LOGE("PwmDeviceSetConfig: pwm or config is null!\n");
        return HDF_ERR_INVALID_PARAM;
    }

    if (memcmp(config, &(pwm->cfg), sizeof(*config)) == 0) {
        HDF_LOGE("PwmDeviceSetConfig: do not need to set config!");
        return HDF_SUCCESS;
    }

    if (pwm->method == NULL || pwm->method->setConfig == NULL) {
        HDF_LOGE("PwmDeviceSetConfig: method or setConfig is null!");
        return HDF_ERR_NOT_SUPPORT;
    }

    ret = pwm->method->setConfig(pwm, config);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("PwmDeviceSetConfig: set config fail, ret: %d!", ret);
        return ret;
    }

    (void)OsalSpinLock(&(pwm->lock));
    pwm->cfg = *config;
    (void)OsalSpinUnlock(&(pwm->lock));
    return HDF_SUCCESS;
}

int32_t PwmDeviceGetConfig(struct PwmDev *pwm, struct PwmConfig *config)
{
    if (pwm == NULL || config == NULL) {
        HDF_LOGE("PwmDeviceGetConfig: pwm or config is null!\n");
        return HDF_ERR_INVALID_PARAM;
    }

    (void)OsalSpinLock(&(pwm->lock));
    *config = pwm->cfg;
    (void)OsalSpinUnlock(&(pwm->lock));
    return HDF_SUCCESS;
}

int32_t PwmSetPriv(struct PwmDev *pwm, void *priv)
{
    if (pwm == NULL) {
        HDF_LOGE("PwmSetPriv: pwm is null!");
        return HDF_ERR_INVALID_PARAM;
    }

    (void)OsalSpinLock(&(pwm->lock));
    pwm->priv = priv;
    (void)OsalSpinUnlock(&(pwm->lock));
    return HDF_SUCCESS;
}

void *PwmGetPriv(const struct PwmDev *pwm)
{
    if (pwm == NULL) {
        HDF_LOGE("PwmGetPriv: pwm is null!");
        return NULL;
    }
    return pwm->priv;
}

static int32_t PwmUserSetConfig(struct PwmDev *pwm, struct HdfSBuf *data)
{
    uint32_t size;
    struct PwmConfig *config = NULL;

    if (data == NULL) {
        HDF_LOGE("PwmUserSetConfig: data is null!");
        return HDF_ERR_INVALID_PARAM;
    }
    if (!HdfSbufReadBuffer(data, (const void **)&config, &size)) {
        HDF_LOGE("PwmUserSetConfig: sbuf read buffer fail!");
        return HDF_ERR_IO;
    }

    if ((config == NULL) || (size != sizeof(struct PwmConfig))) {
        HDF_LOGE("PwmUserSetConfig: read buff error!");
        return HDF_ERR_IO;
    }
    return PwmDeviceSetConfig(pwm, config);
}

static int32_t PwmUserGetConfig(struct PwmDev *pwm, struct HdfSBuf *reply)
{
    int32_t ret;
    struct PwmConfig config;

    if (reply == NULL) {
        HDF_LOGE("PwmUserGetConfig: reply is null!");
        return HDF_ERR_INVALID_PARAM;
    }
    ret = PwmDeviceGetConfig(pwm, &config);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("PwmUserGetConfig: get config fail!");
        return ret;
    }

    if (!HdfSbufWriteBuffer(reply, &config, sizeof(config))) {
        HDF_LOGE("PwmUserGetConfig: sbuf write buffer fail!");
        return HDF_ERR_IO;
    }
    return HDF_SUCCESS;
}

static int32_t PwmIoDispatch(struct HdfDeviceIoClient *client, int cmd,
    struct HdfSBuf *data, struct HdfSBuf *reply)
{
    struct PwmDev *pwm = NULL;

    if (client == NULL || client->device == NULL || client->device->service == NULL) {
        HDF_LOGE("PwmIoDispatch: client info is null!");
        return HDF_ERR_INVALID_OBJECT;
    }

    pwm = (struct PwmDev *)client->device->service;
    switch (cmd) {
        case PWM_IO_GET:
            return PwmDeviceGet(pwm);
        case PWM_IO_PUT:
            return PwmDevicePut(pwm);
        case PWM_IO_SET_CONFIG:
            return PwmUserSetConfig(pwm, data);
        case PWM_IO_GET_CONFIG:
            return PwmUserGetConfig(pwm, reply);
        default:
            HDF_LOGE("PwmIoDispatch: cmd %d is not support!", cmd);
            return HDF_ERR_NOT_SUPPORT;
    }
}

int32_t PwmDeviceAdd(struct HdfDeviceObject *obj, struct PwmDev *pwm)
{
    if (obj == NULL || pwm == NULL) {
        HDF_LOGE("PwmDeviceAdd: obj or pwm is null!");
        return HDF_ERR_INVALID_PARAM;
    }
    if (pwm->method == NULL || pwm->method->setConfig == NULL) {
        HDF_LOGE("PwmDeviceAdd: method or setConfig is null!");
        return HDF_ERR_INVALID_PARAM;
    }
    if (OsalSpinInit(&(pwm->lock)) != HDF_SUCCESS) {
        HDF_LOGE("PwmDeviceAdd: init spinlock fail!");
        return HDF_FAILURE;
    }
    pwm->device = obj;
    obj->service = &(pwm->service);
    pwm->device->service->Dispatch = PwmIoDispatch;
    return HDF_SUCCESS;
}

void PwmDeviceRemove(struct HdfDeviceObject *obj, struct PwmDev *pwm)
{
    if (obj == NULL || pwm == NULL) {
        HDF_LOGE("PwmDeviceRemove: obj or pwm is null!");
        return;
    }
    (void)OsalSpinDestroy(&(pwm->lock));
    pwm->device = NULL;
    obj->service = NULL;
}
