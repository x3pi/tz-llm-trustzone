/*
 * Copyright (c) 2022-2023 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include "pwm_if.h"
#include "hdf_log.h"
#include "hdf_io_service_if.h"
#include "securec.h"

#define HDF_LOG_TAG pwm_if_u_c
#define PWM_NAME_LEN 32

static void *PwmGetDevByNum(uint32_t num)
{
    int32_t ret;
    char name[PWM_NAME_LEN + 1] = {0};
    void *pwm = NULL;

    ret = snprintf_s(name, PWM_NAME_LEN + 1, PWM_NAME_LEN, "HDF_PLATFORM_PWM_%u", num);
    if (ret < 0) {
        HDF_LOGE("PwmGetDevByNum: snprintf_s fail!");
        return NULL;
    }

    pwm = (void *)HdfIoServiceBind(name);
    if (pwm == NULL) {
        HDF_LOGE("PwmGetDevByNum: HdfIoServiceBind fail!");
        return NULL;
    }

    return pwm;
}

static void PwmPutObjByPointer(const void *obj)
{
    if (obj == NULL) {
        HDF_LOGE("PwmPutObjByPointer: obj is null!");
        return;
    }

    HdfIoServiceRecycle((struct HdfIoService *)obj);
}

DevHandle PwmOpen(uint32_t num)
{
    int32_t ret;
    void *pwm = PwmGetDevByNum(num);

    if (pwm == NULL) {
        HDF_LOGE("PwmOpen: pwm is null!");
        return NULL;
    }

    struct HdfIoService *service = (struct HdfIoService *)pwm;
    if (service->dispatcher == NULL || service->dispatcher->Dispatch == NULL) {
        HDF_LOGE("PwmOpen: service is invalid!");
        PwmPutObjByPointer(pwm);
        return NULL;
    }

    ret = service->dispatcher->Dispatch(&service->object, PWM_IO_GET, NULL, NULL);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("PwmOpen: PwmDeviceGet error, ret: %d!", ret);
        PwmPutObjByPointer(pwm);
        return NULL;
    }

    return (DevHandle)pwm;
}

void PwmClose(DevHandle handle)
{
    int32_t ret;
    struct HdfIoService *service = NULL;

    if (handle == NULL) {
        HDF_LOGE("PwmClose: dev is null!");
        return;
    }

    service = (struct HdfIoService *)handle;
    if (service->dispatcher == NULL || service->dispatcher->Dispatch == NULL) {
        HDF_LOGE("PwmClose: service is invalid!");
        PwmPutObjByPointer(handle);
        return;
    }

    ret = service->dispatcher->Dispatch(&service->object, PWM_IO_PUT, NULL, NULL);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("PwmClose: PwmDevicePut error, ret: %d!", ret);
    }

    PwmPutObjByPointer(handle);
}

int32_t PwmSetConfig(DevHandle handle, struct PwmConfig *config)
{
    int32_t ret;
    struct HdfSBuf *buf = NULL;
    struct HdfIoService *service = NULL;

    if (handle == NULL || config == NULL) {
        HDF_LOGE("PwmSetConfig: handle or config is null!");
        return HDF_ERR_INVALID_OBJECT;
    }

    service = (struct HdfIoService *)handle;
    if (service->dispatcher == NULL || service->dispatcher->Dispatch == NULL) {
        HDF_LOGE("PwmSetConfig: service is invalid!");
        return HDF_ERR_INVALID_OBJECT;
    }

    buf = HdfSbufObtainDefaultSize();
    if (buf == NULL) {
        HDF_LOGE("PwmSetConfig: fail to obtain buf!");
        return HDF_ERR_MALLOC_FAIL;
    }
    if (!HdfSbufWriteBuffer(buf, config, sizeof(struct PwmConfig))) {
        HDF_LOGE("PwmSetConfig: sbuf write cfg fail!");
        HdfSbufRecycle(buf);
        return HDF_ERR_IO;
    }

    ret = service->dispatcher->Dispatch(&service->object, PWM_IO_SET_CONFIG, buf, NULL);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("PwmSetConfig: service PWM_IO_SET_CONFIG error, ret: %d!", ret);
    }

    HdfSbufRecycle(buf);
    return ret;
}

int32_t PwmGetConfig(DevHandle handle, struct PwmConfig *config)
{
    int32_t ret;
    struct HdfSBuf *reply = NULL;
    struct HdfIoService *service = NULL;
    const void *rBuf = NULL;
    uint32_t rLen;

    if (handle == NULL || config == NULL) {
        HDF_LOGE("PwmGetConfig: handle or config is null!");
        return HDF_ERR_INVALID_OBJECT;
    }

    service = (struct HdfIoService *)handle;
    if (service->dispatcher == NULL || service->dispatcher->Dispatch == NULL) {
        HDF_LOGE("PwmGetConfig: service is invalid!");
        return HDF_ERR_INVALID_OBJECT;
    }

    reply = HdfSbufObtainDefaultSize();
    if (reply == NULL) {
        HDF_LOGE("PwmGetConfig: fail to obtain reply!");
        return HDF_ERR_MALLOC_FAIL;
    }

    ret = service->dispatcher->Dispatch(&service->object, PWM_IO_GET_CONFIG, NULL, reply);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("PwmGetConfig: service PWM_IO_GET_CONFIG error, ret: %d!", ret);
        HdfSbufRecycle(reply);
        return ret;
    }

    if (!HdfSbufReadBuffer(reply, &rBuf, &rLen)) {
        HDF_LOGE("PwmGetConfig: sbuf read buffer fail!");
        HdfSbufRecycle(reply);
        return HDF_ERR_IO;
    }
    if (rLen != sizeof(struct PwmConfig)) {
        HDF_LOGE("PwmGetConfig: sbuf read buffer len error %u != %zu", rLen, sizeof(struct PwmConfig));
        HdfSbufRecycle(reply);
        return HDF_ERR_IO;
    }
    if (memcpy_s(config, sizeof(struct PwmConfig), rBuf, rLen) != EOK) {
        HDF_LOGE("PwmGetConfig: memcpy rBuf fail!");
        HdfSbufRecycle(reply);
        return HDF_ERR_IO;
    }

    HdfSbufRecycle(reply);
    return HDF_SUCCESS;
}

enum PwmSetConfigType {
    PWM_SET_CONFIG_PERIOD = 1,
    PWM_SET_CONFIG_DUTY,
    PWM_SET_CONFIG_POLARITY,
    PWM_SET_CONFIG_STATUS,
};

static int32_t PwmConfigTransSet(DevHandle handle, enum PwmSetConfigType type, struct PwmConfig *config)
{
    struct PwmConfig nowCfg;
    uint32_t curValue;
    int32_t ret;

    if (PwmGetConfig(handle, &nowCfg) != HDF_SUCCESS) {
        HDF_LOGE("PwmConfigTransSet: PwmGetConfig fail!");
        return HDF_FAILURE;
    }

    switch (type) {
        case PWM_SET_CONFIG_PERIOD:
            curValue = nowCfg.period;
            nowCfg.period = config->period;
            break;
        case PWM_SET_CONFIG_DUTY:
            curValue = nowCfg.duty;
            nowCfg.duty = config->duty;
            break;
        case PWM_SET_CONFIG_POLARITY:
            curValue = nowCfg.polarity;
            nowCfg.polarity = config->polarity;
            break;
        case PWM_SET_CONFIG_STATUS:
            curValue = nowCfg.status;
            nowCfg.status = config->status;
            break;
        default:
            HDF_LOGE("PwmConfigTransSet: type %d is not support!", type);
            return HDF_ERR_NOT_SUPPORT;
    }

    ret = PwmSetConfig(handle, &nowCfg);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("PwmConfigTransSet: set [%d] cfg fail, org val[%u]!", type, curValue);
        return HDF_FAILURE;
    }

    return HDF_SUCCESS;
}
int32_t PwmSetPeriod(DevHandle handle, uint32_t period)
{
    struct PwmConfig config;

    if (handle == NULL) {
        HDF_LOGE("PwmSetPeriod: handle is null!");
        return HDF_ERR_INVALID_OBJECT;
    }

    if (memset_s(&config, sizeof(struct PwmConfig), 0, sizeof(struct PwmConfig)) != EOK) {
        HDF_LOGE("PwmSetPeriod: memset_s fail!");
        return HDF_ERR_IO;
    }
    config.period = period;
    if (PwmConfigTransSet(handle, PWM_SET_CONFIG_PERIOD, &config) != HDF_SUCCESS) {
        HDF_LOGE("PwmSetPeriod: PwmConfigTransSet fail!");
        return HDF_FAILURE;
    }

    return HDF_SUCCESS;
}

int32_t PwmSetDuty(DevHandle handle, uint32_t duty)
{
    struct PwmConfig config;

    if (handle == NULL) {
        HDF_LOGE("PwmSetDuty: handle is null!");
        return HDF_ERR_INVALID_OBJECT;
    }
    if (memset_s(&config, sizeof(struct PwmConfig), 0, sizeof(struct PwmConfig)) != EOK) {
        HDF_LOGE("PwmSetDuty: memset_s fail!");
        return HDF_ERR_IO;
    }
    config.duty = duty;
    if (PwmConfigTransSet(handle, PWM_SET_CONFIG_DUTY, &config) != HDF_SUCCESS) {
        HDF_LOGE("PwmSetDuty: PwmConfigTransSet fail!");
        return HDF_FAILURE;
    }

    return HDF_SUCCESS;
}

int32_t PwmSetPolarity(DevHandle handle, uint8_t polarity)
{
    struct PwmConfig config;

    if (handle == NULL) {
        HDF_LOGE("PwmSetPolarity: handle is null!");
        return HDF_ERR_INVALID_OBJECT;
    }

    if (memset_s(&config, sizeof(struct PwmConfig), 0, sizeof(struct PwmConfig)) != EOK) {
        HDF_LOGE("PwmSetPolarity: memset_s fail!");
        return HDF_ERR_IO;
    }
    config.polarity = polarity;
    if (PwmConfigTransSet(handle, PWM_SET_CONFIG_POLARITY, &config) != HDF_SUCCESS) {
        HDF_LOGE("PwmSetPolarity: PwmConfigTransSet fail!");
        return HDF_FAILURE;
    }

    return HDF_SUCCESS;
}

int32_t PwmEnable(DevHandle handle)
{
    struct PwmConfig config;

    if (handle == NULL) {
        HDF_LOGE("PwmEnable: handle is null!");
        return HDF_ERR_INVALID_OBJECT;
    }

    if (memset_s(&config, sizeof(struct PwmConfig), 0, sizeof(struct PwmConfig)) != EOK) {
        HDF_LOGE("PwmEnable: memset_s fail!");
        return HDF_ERR_IO;
    }
    config.status = PWM_ENABLE_STATUS;
    if (PwmConfigTransSet(handle, PWM_SET_CONFIG_STATUS, &config) != HDF_SUCCESS) {
        HDF_LOGE("PwmEnable: PwmConfigTransSet fail!");
        return HDF_FAILURE;
    }

    return HDF_SUCCESS;
}

int32_t PwmDisable(DevHandle handle)
{
    struct PwmConfig config;

    if (handle == NULL) {
        HDF_LOGE("PwmDisable: handle is null!");
        return HDF_ERR_INVALID_OBJECT;
    }

    if (memset_s(&config, sizeof(struct PwmConfig), 0, sizeof(struct PwmConfig)) != EOK) {
        return HDF_ERR_IO;
    }
    config.status = PWM_DISABLE_STATUS;
    if (PwmConfigTransSet(handle, PWM_SET_CONFIG_STATUS, &config) != HDF_SUCCESS) {
        HDF_LOGE("PwmDisable: PwmConfigTransSet fail!");
        return HDF_FAILURE;
    }

    return HDF_SUCCESS;
}
