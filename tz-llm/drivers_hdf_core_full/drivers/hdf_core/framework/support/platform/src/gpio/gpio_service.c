/*
 * Copyright (c) 2020-2023 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include "gpio/gpio_service.h"
#include "gpio/gpio_core.h"
#include "gpio_if.h"
#include "hdf_device_desc.h"
#include "hdf_device_object.h"
#include "platform_core.h"
#include "platform_listener_common.h"
#include "securec.h"

#define HDF_LOG_TAG gpio_service

static int32_t GpioServiceIoRead(struct HdfSBuf *data, struct HdfSBuf *reply)
{
    int32_t ret;
    uint16_t gpio;
    uint16_t value;

    if (data == NULL || reply == NULL) {
        HDF_LOGE("GpioServiceIoRead: data or reply is null!");
        return HDF_ERR_INVALID_PARAM;
    }

    if (!HdfSbufReadUint16(data, &gpio)) {
        HDF_LOGE("GpioServiceIoRead: fail to read gpio number!");
        return HDF_ERR_IO;
    }

    ret = GpioRead(gpio, &value);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("GpioServiceIoRead: fail to read gpio, ret: %d!", ret);
        return ret;
    }

    if (!HdfSbufWriteUint16(reply, value)) {
        HDF_LOGE("GpioServiceIoRead: fail to write gpio value, ret: %d!", ret);
        return ret;
    }

    return ret;
}

static int32_t GpioServiceIoWrite(struct HdfSBuf *data, struct HdfSBuf *reply)
{
    int32_t ret;
    uint16_t gpio;
    uint16_t value;

    (void)reply;
    if (data == NULL) {
        HDF_LOGE("GpioServiceIoWrite: data is null!");
    }

    if (!HdfSbufReadUint16(data, &gpio)) {
        HDF_LOGE("GpioServiceIoWrite: fail to read gpio number!");
        return HDF_ERR_IO;
    }

    if (!HdfSbufReadUint16(data, &value)) {
        HDF_LOGE("GpioServiceIoWrite: fail to read gpio value!");
        return HDF_ERR_IO;
    }

    ret = GpioWrite(gpio, value);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("GpioServiceIoWrite: fail to write gpio value, ret: %d!", ret);
        return ret;
    }

    return ret;
}

static int32_t GpioServiceIoGetDir(struct HdfSBuf *data, struct HdfSBuf *reply)
{
    int32_t ret;
    uint16_t gpio;
    uint16_t dir;

    if (data == NULL || reply == NULL) {
        HDF_LOGE("GpioServiceIoGetDir: data or reply is null!");
        return HDF_ERR_INVALID_PARAM;
    }

    if (!HdfSbufReadUint16(data, &gpio)) {
        HDF_LOGE("GpioServiceIoGetDir: fail to read gpio number!");
        return HDF_ERR_IO;
    }

    ret = GpioGetDir(gpio, &dir);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("GpioServiceIoGetDir: fail to get gpio dir, ret: %d!", ret);
        return ret;
    }

    if (!HdfSbufWriteUint16(reply, dir)) {
        HDF_LOGE("GpioServiceIoGetDir: fail to write gpio dir, ret: %d!", ret);
        return ret;
    }

    return ret;
}

static int32_t GpioServiceIoSetDir(struct HdfSBuf *data, struct HdfSBuf *reply)
{
    int32_t ret;
    uint16_t gpio;
    uint16_t dir;

    (void)reply;
    if (data == NULL) {
        HDF_LOGE("GpioServiceIoSetDir: data is null!");
        return HDF_ERR_INVALID_PARAM;
    }

    if (!HdfSbufReadUint16(data, &gpio)) {
        HDF_LOGE("GpioServiceIoSetDir: fail to read gpio number!");
        return HDF_ERR_IO;
    }

    if (!HdfSbufReadUint16(data, &dir)) {
        HDF_LOGE("GpioServiceIoSetDir: fail to read gpio dir!");
        return HDF_ERR_IO;
    }

    ret = GpioSetDir(gpio, dir);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("GpioServiceIoSetDir: fail to set gpio dir, ret: %d!", ret);
        return ret;
    }

    return ret;
}

static void GpioServiceUpdate(uint16_t gpio)
{
    int32_t ret;
    uint32_t id;
    struct HdfSBuf *data = NULL;
    struct PlatformManager *gpioMgr = NULL;

    gpioMgr = GpioManagerGet();
    if (gpioMgr == NULL || gpioMgr->device.hdfDev == NULL) {
        HDF_LOGE("GpioServiceUpdate: fail to get gpio manager!");
        return;
    }

    id = PLATFORM_LISTENER_EVENT_GPIO_INT_NOTIFY;
    data = HdfSbufObtainDefaultSize();
    if (data == NULL) {
        HDF_LOGE("GpioServiceUpdate: fail to obtain data!");
        return;
    }
    if (!HdfSbufWriteUint16(data, gpio)) {
        HDF_LOGE("GpioServiceUpdate: fail to write gpio number!");
        HdfSbufRecycle(data);
        return;
    }
    ret = HdfDeviceSendEvent(gpioMgr->device.hdfDev, id, data);
    HdfSbufRecycle(data);
    HDF_LOGD("GpioServiceUpdate: set service info done, ret = %d, id = %d!", ret, id);
}

static int32_t GpioServiceIrqFunc(uint16_t gpio, void *data)
{
    (void)data;
    HDF_LOGD("GpioServiceIrqFunc: %d", gpio);
    GpioServiceUpdate(gpio);
    return HDF_SUCCESS;
}

static int32_t GpioServiceIoSetIrq(struct HdfSBuf *data, struct HdfSBuf *reply)
{
    uint16_t gpio;
    uint16_t mode;
    int32_t ret;

    (void)reply;
    if (data == NULL) {
        HDF_LOGE("GpioServiceIoSetIrq: data is null!");
        return HDF_ERR_INVALID_PARAM;
    }

    if (!HdfSbufReadUint16(data, &gpio)) {
        HDF_LOGE("GpioServiceIoSetIrq: fail to read gpio number!");
        return HDF_ERR_IO;
    }

    if (!HdfSbufReadUint16(data, &mode)) {
        HDF_LOGE("GpioServiceIoSetIrq: fail to read gpio mode!");
        return HDF_ERR_IO;
    }

    ret = GpioSetIrq(gpio, mode, GpioServiceIrqFunc, NULL);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("GpioServiceIoSetIrq: fail to set gpio irq, ret: %d!", ret);
        return ret;
    }
    return HDF_SUCCESS;
}

static int32_t GpioServiceIoUnsetIrq(struct HdfSBuf *data, struct HdfSBuf *reply)
{
    uint16_t gpio;

    (void)reply;
    if (data == NULL) {
        HDF_LOGE("GpioServiceIoUnsetIrq: data is null!");
        return HDF_ERR_INVALID_PARAM;
    }

    if (!HdfSbufReadUint16(data, &gpio)) {
        HDF_LOGE("GpioServiceIoUnsetIrq: fail to read gpio number!");
        return HDF_ERR_IO;
    }

    if (GpioUnsetIrq(gpio, NULL) != HDF_SUCCESS) {
        HDF_LOGE("GpioServiceIoUnsetIrq: fail to unset gpio irq!");
        return HDF_ERR_IO;
    }

    return HDF_SUCCESS;
}

static int32_t GpioServiceIoEnableIrq(struct HdfSBuf *data, struct HdfSBuf *reply)
{
    int32_t ret;
    uint16_t gpio;

    (void)reply;
    if (data == NULL) {
        HDF_LOGE("GpioServiceIoEnableIrq: data is null!");
        return HDF_ERR_INVALID_PARAM;
    }

    if (!HdfSbufReadUint16(data, &gpio)) {
        HDF_LOGE("GpioServiceIoEnableIrq: fail to read gpio number!");
        return HDF_ERR_IO;
    }

    ret = GpioEnableIrq(gpio);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("GpioServiceIoEnableIrq: fail to enable gpio irq, ret: %d!", ret);
        return ret;
    }

    return ret;
}

static int32_t GpioServiceIoDisableIrq(struct HdfSBuf *data, struct HdfSBuf *reply)
{
    int32_t ret;
    uint16_t gpio;

    (void)reply;
    if (data == NULL) {
        HDF_LOGE("GpioServiceIoDisableIrq: data is null!");
        return HDF_ERR_INVALID_PARAM;
    }

    if (!HdfSbufReadUint16(data, &gpio)) {
        HDF_LOGE("GpioServiceIoDisableIrq: fail to read gpio number!");
        return HDF_ERR_IO;
    }

    ret = GpioDisableIrq(gpio);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("GpioServiceIoDisableIrq: fail to disable gpio irq, ret: %d!", ret);
        return ret;
    }

    return ret;
}

static int32_t GpioServiceIoGetNumByName(struct HdfSBuf *data, struct HdfSBuf *reply)
{
    int32_t ret;
    const char *gpioNameData = NULL;

    if (data == NULL || reply == NULL) {
        HDF_LOGE("GpioServiceIoGetNumByName: data or reply is null!");
        return HDF_ERR_INVALID_PARAM;
    }

    gpioNameData = HdfSbufReadString(data);
    if (gpioNameData == NULL) {
        HDF_LOGE("GpioServiceIoGetNumByName: gpioNameData is null!");
        return HDF_ERR_IO;
    }

    ret = GpioGetByName(gpioNameData);
    if (ret < 0) {
        HDF_LOGE("GpioServiceIoGetNumByName: fail to get gpio global number by gpioName!");
        return ret;
    }

    if (!HdfSbufWriteInt32(reply, ret)) {
        HDF_LOGE("GpioServiceIoGetNumByName: fail to write gpio global number!");
        return HDF_ERR_IO;
    }

    return HDF_SUCCESS;
}

static int32_t GpioServiceDispatch(
    struct HdfDeviceIoClient *client, int cmd, struct HdfSBuf *data, struct HdfSBuf *reply)
{
    int32_t ret;

    (void)client;
    switch (cmd) {
        case GPIO_IO_READ:
            return GpioServiceIoRead(data, reply);
        case GPIO_IO_WRITE:
            return GpioServiceIoWrite(data, reply);
        case GPIO_IO_GETDIR:
            return GpioServiceIoGetDir(data, reply);
        case GPIO_IO_SETDIR:
            return GpioServiceIoSetDir(data, reply);
        case GPIO_IO_SETIRQ:
            return GpioServiceIoSetIrq(data, reply);
        case GPIO_IO_UNSETIRQ:
            return GpioServiceIoUnsetIrq(data, reply);
        case GPIO_IO_ENABLEIRQ:
            return GpioServiceIoEnableIrq(data, reply);
        case GPIO_IO_DISABLEIRQ:
            return GpioServiceIoDisableIrq(data, reply);
        case GPIO_IO_GET_NUM_BY_NAME:
            return GpioServiceIoGetNumByName(data, reply);
        default:
            HDF_LOGE("GpioServiceDispatch: cmd %d is not support!", cmd);
            ret = HDF_ERR_NOT_SUPPORT;
            break;
    }
    return ret;
}

static int32_t GpioServiceBind(struct HdfDeviceObject *device)
{
    int32_t ret;
    struct PlatformManager *gpioMgr = NULL;

    HDF_LOGI("GpioServiceBind: enter!");
    if (device == NULL) {
        HDF_LOGE("GpioServiceBind: device is null!");
        return HDF_ERR_INVALID_OBJECT;
    }

    gpioMgr = GpioManagerGet();
    if (gpioMgr == NULL) {
        HDF_LOGE("GpioServiceBind: fail to get gpio manager!");
        return HDF_PLT_ERR_DEV_GET;
    }

    ret = PlatformDeviceCreateService(&gpioMgr->device, GpioServiceDispatch);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("GpioServiceBind: fail to create gpio service, ret: %d!", ret);
        return ret;
    }

    ret = PlatformDeviceBind(&gpioMgr->device, device);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("GpioServiceBind: fail to bind gpio device, ret: %d!", ret);
        (void)PlatformDeviceDestroyService(&gpioMgr->device);
        return ret;
    }

    HDF_LOGI("GpioServiceBind: success!");
    return HDF_SUCCESS;
}

static int32_t GpioServiceInit(struct HdfDeviceObject *device)
{
    (void)device;
    return HDF_SUCCESS;
}

static void GpioServiceRelease(struct HdfDeviceObject *device)
{
    struct PlatformManager *gpioMgr = NULL;

    HDF_LOGI("GpioServiceRelease: enter!");
    if (device == NULL) {
        HDF_LOGI("GpioServiceRelease: device is null!");
        return;
    }

    gpioMgr = GpioManagerGet();
    if (gpioMgr == NULL) {
        HDF_LOGE("GpioServiceRelease: fail to get gpio manager!");
        return;
    }

    (void)PlatformDeviceUnbind(&gpioMgr->device, device);
    (void)PlatformDeviceDestroyService(&gpioMgr->device);
    HDF_LOGI("GpioServiceRelease: done!");
}

struct HdfDriverEntry g_gpioServiceEntry = {
    .moduleVersion = 1,
    .Bind = GpioServiceBind,
    .Init = GpioServiceInit,
    .Release = GpioServiceRelease,
    .moduleName = "HDF_PLATFORM_GPIO_MANAGER",
};
HDF_INIT(g_gpioServiceEntry);
