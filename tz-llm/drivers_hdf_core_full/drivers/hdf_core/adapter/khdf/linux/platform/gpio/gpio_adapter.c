/*
 * gpio_adapter.h
 *
 * gpio driver adapter of linux
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

#include <linux/gpio.h>
#include <linux/gpio/driver.h>
#include <linux/interrupt.h>
#include <linux/list.h>
#include "device_resource_if.h"
#include "gpio/gpio_core.h"
#include "hdf_base.h"
#include "hdf_device_desc.h"
#include "hdf_dlist.h"
#include "hdf_log.h"
#include "osal_mem.h"
#include "osal_mutex.h"

#define HDF_LOG_TAG linux_gpio_adapter

#define LINUX_GPIO_NUM_MAX 0x7FFF

static int32_t LinuxGpioWrite(struct GpioCntlr *cntlr, uint16_t local, uint16_t val)
{
    if (cntlr == NULL) {
        HDF_LOGE("LinuxGpioWrite: cntlr is null!\n");
        return HDF_ERR_INVALID_OBJECT;
    }
    gpio_set_value_cansleep(cntlr->start + local, val);
    return HDF_SUCCESS;
}

static int32_t LinuxGpioRead(struct GpioCntlr *cntlr, uint16_t local, uint16_t *val)
{
    if (cntlr == NULL) {
        HDF_LOGE("LinuxGpioRead: cntlr is null!\n");
        return HDF_ERR_INVALID_OBJECT;
    }
    if (val != NULL) {
        *val = (gpio_get_value_cansleep(cntlr->start + local) == 0) ? GPIO_VAL_LOW : GPIO_VAL_HIGH;
        return HDF_SUCCESS;
    }
    HDF_LOGE("LinuxGpioRead: val is null!\n");
    return HDF_ERR_BSP_PLT_API_ERR;
}

static int32_t LinuxGpioSetDir(struct GpioCntlr *cntlr, uint16_t local, uint16_t dir)
{
    int32_t ret;
    int val;

    if (cntlr == NULL) {
        HDF_LOGE("LinuxGpioSetDir: cntlr is null!\n");
        return HDF_ERR_INVALID_OBJECT;
    }
    switch (dir) {
        case GPIO_DIR_IN:
            ret = gpio_direction_input(cntlr->start + local);
            if (ret < 0) {
                return HDF_ERR_BSP_PLT_API_ERR;
            }
            break;
        case GPIO_DIR_OUT:
            val = gpio_get_value_cansleep(cntlr->start + local);
            if (val < 0) {
                return HDF_ERR_BSP_PLT_API_ERR;
            }
            ret = gpio_direction_output(cntlr->start + local, val);
            if (ret < 0) {
                return HDF_ERR_BSP_PLT_API_ERR;
            }
            break;
        default:
            HDF_LOGE("LinuxGpioSetDir: invalid dir:%d!\n", dir);
            return HDF_ERR_INVALID_PARAM;
    }
    return HDF_SUCCESS;
}

static int32_t LinuxGpioGetDir(struct GpioCntlr *cntlr, uint16_t local, uint16_t *dir)
{
    int dirGet;

    if (cntlr == NULL) {
        HDF_LOGE("LinuxGpioGetDir: cntlr is null!\n");
        return HDF_ERR_INVALID_OBJECT;
    }
    dirGet = gpiod_get_direction(gpio_to_desc(cntlr->start + local));
    if (dirGet < 0) {
        return HDF_ERR_BSP_PLT_API_ERR;
    }
    *dir = (dirGet == GPIOF_DIR_IN) ? GPIO_DIR_IN : GPIO_DIR_OUT;
    return HDF_SUCCESS;
}

static irqreturn_t LinuxGpioIrqDummy(int irq, void *data)
{
    (void)irq;
    (void)data;
    return IRQ_HANDLED;
}

static irqreturn_t LinuxGpioIrqBridge(int irq, void *data)
{
    int gpio = (int)(uintptr_t)data;
    struct GpioCntlr *cntlr = NULL;

    (void)irq;
    cntlr = GpioCntlrGetByGpio(gpio);
    GpioCntlrIrqCallback(cntlr, GpioCntlrGetLocal(cntlr, gpio));
    GpioCntlrPut(cntlr);
    return IRQ_HANDLED;
}

static int32_t LinuxGpioSetIrq(struct GpioCntlr *cntlr, uint16_t local, uint16_t mode)
{
    int ret;
    int irq;
    unsigned long flags = 0;
    uint16_t gpio;

    if (cntlr == NULL) {
        HDF_LOGE("LinuxGpioSetIrq: cntlr is null!\n");
        return HDF_ERR_INVALID_OBJECT;
    }
    gpio = cntlr->start + local;

    irq = gpio_to_irq(gpio);
    if (irq < 0) {
        HDF_LOGE("LinuxGpioSetIrq: gpio(%u) to irq fail:%d!", gpio, irq);
        return HDF_ERR_BSP_PLT_API_ERR;
    }
    flags |= (mode & GPIO_IRQ_TRIGGER_RISING) == 0 ? 0 : IRQF_TRIGGER_RISING;
    flags |= (mode & GPIO_IRQ_TRIGGER_FALLING) == 0 ? 0 : IRQF_TRIGGER_FALLING;
    flags |= (mode & GPIO_IRQ_TRIGGER_HIGH) == 0 ? 0 : IRQF_TRIGGER_HIGH;
    flags |= (mode & GPIO_IRQ_TRIGGER_LOW) == 0 ? 0 : IRQF_TRIGGER_LOW;
    HDF_LOGI("LinuxGpioSetIrq: gona request normal irq:%d(%u)!\n", irq, gpio);
    ret = request_irq(irq, LinuxGpioIrqBridge, flags,
        "LinuxIrqBridge", (void *)(uintptr_t)gpio);
    if (ret != 0) {
        HDF_LOGI("LinuxGpioSetIrq: gona request threaded irq:%d(%u)!\n", irq, gpio);
        flags |= IRQF_ONESHOT;
        ret = request_threaded_irq(irq, LinuxGpioIrqBridge, LinuxGpioIrqDummy, flags,
            "LinuxIrqBridge", (void *)(uintptr_t)gpio);
    }
    if (ret == 0) {
        disable_irq_nosync(irq); // disable on set
    }
    return (ret == 0) ? HDF_SUCCESS : HDF_ERR_BSP_PLT_API_ERR;
}

static int32_t LinuxGpioUnsetIrq(struct GpioCntlr *cntlr, uint16_t local)
{
    int irq;
    uint16_t gpio;

    if (cntlr == NULL) {
        HDF_LOGE("LinuxGpioUnsetIrq: cntlr is null!");
        return HDF_ERR_INVALID_OBJECT;
    }
    gpio = cntlr->start + local;
    irq = gpio_to_irq(gpio);
    if (irq < 0) {
        HDF_LOGE("LinuxGpioUnsetIrq: gpio(%u) to irq fail:%d!", gpio, irq);
        return HDF_ERR_BSP_PLT_API_ERR;
    }
    HDF_LOGI("LinuxGpioUnsetIrq: gona free irq:%d!\n", irq);
    free_irq(irq, (void *)(uintptr_t)gpio);
    return HDF_SUCCESS;
}

static inline int32_t LinuxGpioEnableIrq(struct GpioCntlr *cntlr, uint16_t local)
{
    int irq;
    uint16_t gpio;

    if (cntlr == NULL) {
        HDF_LOGE("LinuxGpioEnableIrq: cntlr is null!\n");
        return HDF_ERR_INVALID_OBJECT;
    }
    gpio = cntlr->start + local;
    irq = gpio_to_irq(gpio);
    if (irq < 0) {
        HDF_LOGE("LinuxGpioEnableIrq: gpio(%u) to irq fail:%d!", gpio, irq);
        return HDF_ERR_BSP_PLT_API_ERR;
    }
    enable_irq(irq);
    return HDF_SUCCESS;
}

static inline int32_t LinuxGpioDisableIrq(struct GpioCntlr *cntlr, uint16_t local)
{
    int irq;
    uint16_t gpio;

    if (cntlr == NULL) {
        HDF_LOGE("LinuxGpioDisableIrq: cntlr is null!\n");
        return HDF_ERR_INVALID_OBJECT;
    }
    gpio = cntlr->start + local;
    irq = gpio_to_irq(gpio);
    if (irq < 0) {
        HDF_LOGE("LinuxGpioDisableIrq: gpio(%u) to irq fail:%d!", gpio, irq);
        return HDF_ERR_BSP_PLT_API_ERR;
    }
    disable_irq_nosync(irq); // nosync default in case used in own irq
    return HDF_SUCCESS;
}

static struct GpioMethod g_method = {
    .write = LinuxGpioWrite,
    .read = LinuxGpioRead,
    .setDir = LinuxGpioSetDir,
    .getDir = LinuxGpioGetDir,
    .setIrq = LinuxGpioSetIrq,
    .unsetIrq = LinuxGpioUnsetIrq,
    .enableIrq = LinuxGpioEnableIrq,
    .disableIrq = LinuxGpioDisableIrq,
};

static int32_t LinuxGpioBind(struct HdfDeviceObject *device)
{
    (void)device;
    return HDF_SUCCESS;
}

static int LinuxGpioMatchProbe(struct gpio_chip *chip, void *data)
{
    int32_t ret;
    struct GpioCntlr *cntlr = NULL;

    (void)data;
    if (chip == NULL) {
        HDF_LOGE("LinuxGpioMatchProbe: chip is null!\n");
        return 0;
    }
    HDF_LOGI("LinuxGpioMatchProbe: find gpio chip(start:%d, count:%u)", chip->base, chip->ngpio);
    if (chip->base >= LINUX_GPIO_NUM_MAX || (chip->base + chip->ngpio) > LINUX_GPIO_NUM_MAX) {
        HDF_LOGW("LinuxGpioMatchProbe: chip(base:%d-num:%u) exceed range!", chip->base, chip->ngpio);
        return 0;
    }

    cntlr = (struct GpioCntlr *)OsalMemCalloc(sizeof(*cntlr));
    if (cntlr == NULL) {
        HDF_LOGE("LinuxGpioMatchProbe: malloc cntlr fail!");
        return HDF_ERR_MALLOC_FAIL;
    }

    cntlr->ops = &g_method;
    cntlr->start = (uint16_t)chip->base;
    cntlr->count = (uint16_t)chip->ngpio;
    ret = GpioCntlrAdd(cntlr);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("LinuxGpioMatchProbe: add gpio controller(start:%d, count:%u) fail:%d!",
            cntlr->start, cntlr->count, ret);
        OsalMemFree(cntlr);
        return ret;
    }

    HDF_LOGI("LinuxGpioMatchProbe: add gpio controller(start:%d, count:%u) succeed!",
        cntlr->start, cntlr->count);
    return 0; // return 0 to continue
}

static int32_t LinuxGpioInit(struct HdfDeviceObject *device)
{
    if (device == NULL) {
        HDF_LOGE("LinuxGpioInit: device is null!");
        return HDF_ERR_INVALID_OBJECT;
    }

    (void)gpiochip_find(device, LinuxGpioMatchProbe);
    HDF_LOGI("LinuxGpioInit: dev service:%s init done!", HdfDeviceGetServiceName(device));
    return HDF_SUCCESS;
}

static int LinuxGpioMatchRelease(struct gpio_chip *chip, void *data)
{
    struct GpioCntlr *cntlr = NULL;

    (void)data;
    if (chip == NULL) {
        HDF_LOGE("LinuxGpioMatchRelease: chip is null!");
        return 0;
    }
    HDF_LOGI("LinuxGpioMatchRelease: find gpio chip(start:%d, count:%u)", chip->base, chip->ngpio);
    if (chip->base >= LINUX_GPIO_NUM_MAX || (chip->base + chip->ngpio) > LINUX_GPIO_NUM_MAX) {
        HDF_LOGW("LinuxGpioMatchRelease: chip(base:%d-num:%u) exceed range!", chip->base, chip->ngpio);
        return 0;
    }

    cntlr = GpioCntlrGetByGpio((uint16_t)chip->base);
    if (cntlr == NULL) {
        HDF_LOGW("LinuxGpioMatchRelease: get cntlr failed for base:%d!", chip->base);
        return 0;
    }
    GpioCntlrPut(cntlr); // !!! be careful to keep the reference count balanced

    HDF_LOGI("LinuxGpioMatchRelease: gona remove gpio controller(start:%d, count:%u)",
        cntlr->start, cntlr->count);
    GpioCntlrRemove(cntlr);
    OsalMemFree(cntlr);
    return 0; // return 0 to continue
}

static void LinuxGpioRelease(struct HdfDeviceObject *device)
{
    if (device == NULL) {
        HDF_LOGE("LinuxGpioRelease: device is null!");
        return;
    }

    (void)gpiochip_find(device, LinuxGpioMatchRelease);
}

struct HdfDriverEntry g_gpioLinuxDriverEntry = {
    .moduleVersion = 1,
    .Bind = LinuxGpioBind,
    .Init = LinuxGpioInit,
    .Release = LinuxGpioRelease,
    .moduleName = "linux_gpio_adapter",
};
HDF_INIT(g_gpioLinuxDriverEntry);
