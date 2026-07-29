/*
 * Copyright (c) 2020-2023 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include "gpio/gpio_core.h"
#ifdef __LITEOS_M__
#include "los_interrupt.h"
#endif
#include "osal_mem.h"
#include "platform_core.h"

#define HDF_LOG_TAG gpio_core

#define GPIO_IRQ_STACK_SIZE        10000

static inline void GpioInfoLock(struct GpioInfo *ginfo)
{
#ifndef __LITEOS_M__
    (void)OsalSpinLockIrqSave(&ginfo->spin, &ginfo->irqSave);
#else
    ginfo->irqSave = LOS_IntLock();
#endif
}

static inline void GpioInfoUnlock(struct GpioInfo *ginfo)
{
#ifndef __LITEOS_M__
    (void)OsalSpinUnlockIrqRestore(&ginfo->spin, &ginfo->irqSave);
#else
    LOS_IntRestore(ginfo->irqSave);
#endif
}

int32_t GpioCntlrWrite(struct GpioCntlr *cntlr, uint16_t local, uint16_t val)
{
    if (cntlr == NULL) {
        HDF_LOGE("GpioCntlrWrite: cntlr is null!");
        return HDF_ERR_INVALID_OBJECT;
    }
    if (cntlr->ops == NULL || cntlr->ops->write == NULL) {
        HDF_LOGE("GpioCntlrWrite: ops or write is null!");
        return HDF_ERR_NOT_SUPPORT;
    }

    return cntlr->ops->write(cntlr, local, val);
}

int32_t GpioCntlrRead(struct GpioCntlr *cntlr, uint16_t local, uint16_t *val)
{
    if (cntlr == NULL) {
        HDF_LOGE("GpioCntlrRead: cntlr is null!");
        return HDF_ERR_INVALID_OBJECT;
    }
    if (cntlr->ops == NULL || cntlr->ops->read == NULL) {
        HDF_LOGE("GpioCntlrRead: ops or read is null!");
        return HDF_ERR_NOT_SUPPORT;
    }
    if (val == NULL) {
        HDF_LOGE("GpioCntlrRead: val is null!");
        return HDF_ERR_INVALID_PARAM;
    }

    return cntlr->ops->read(cntlr, local, val);
}

int32_t GpioCntlrSetDir(struct GpioCntlr *cntlr, uint16_t local, uint16_t dir)
{
    if (cntlr == NULL) {
        HDF_LOGE("GpioCntlrSetDir: cntlr is null!");
        return HDF_ERR_INVALID_OBJECT;
    }
    if (cntlr->ops == NULL || cntlr->ops->setDir == NULL) {
        HDF_LOGE("GpioCntlrSetDir: ops or setDir is null!");
        return HDF_ERR_NOT_SUPPORT;
    }

    return cntlr->ops->setDir(cntlr, local, dir);
}

int32_t GpioCntlrGetDir(struct GpioCntlr *cntlr, uint16_t local, uint16_t *dir)
{
    if (cntlr == NULL) {
        HDF_LOGE("GpioCntlrGetDir: cntlr is null!");
        return HDF_ERR_INVALID_OBJECT;
    }
    if (cntlr->ops == NULL || cntlr->ops->getDir == NULL) {
        HDF_LOGE("GpioCntlrGetDir: ops or getDir is null!");
        return HDF_ERR_NOT_SUPPORT;
    }
    if (dir == NULL) {
        HDF_LOGE("GpioCntlrGetDir: dir is null!");
        return HDF_ERR_INVALID_PARAM;
    }

    return cntlr->ops->getDir(cntlr, local, dir);
}

int32_t GpioCntlrToIrq(struct GpioCntlr *cntlr, uint16_t local, uint16_t *irq)
{
    if (cntlr == NULL) {
        HDF_LOGE("GpioCntlrToIrq: cntlr is null!");
        return HDF_ERR_INVALID_OBJECT;
    }
    if (cntlr->ops == NULL || cntlr->ops->toIrq == NULL) {
        HDF_LOGE("GpioCntlrToIrq: ops or toIrq is null!");
        return HDF_ERR_NOT_SUPPORT;
    }

    return cntlr->ops->toIrq(cntlr, local, irq);
}

void GpioCntlrIrqCallback(struct GpioCntlr *cntlr, uint16_t local)
{
    struct GpioInfo *ginfo = NULL;
    struct GpioIrqRecord *irqRecord = NULL;

    if (cntlr == NULL || cntlr->ginfos == NULL || local >= cntlr->count) {
        HDF_LOGW("GpioCntlrIrqCallback: invalid cntlr(ginfos) or loal num:%hu!", local);
        return;
    }
    ginfo = &cntlr->ginfos[local];
    if (ginfo == NULL) {
        HDF_LOGW("GpioCntlrIrqCallback: ginfo null(start:%hu, local:%hu)", cntlr->start, local);
        return;
    }

    GpioInfoLock(ginfo);
    irqRecord = ginfo->irqRecord;

    if (irqRecord == NULL) {
        HDF_LOGW("GpioCntlrIrqCallback: irq not set (start:%hu, local:%hu)", cntlr->start, local);
        GpioInfoUnlock(ginfo);
        return;
    }
    GpioIrqRecordTrigger(irqRecord);
    GpioInfoUnlock(ginfo);
}

static int32_t GpioCntlrIrqThreadHandler(void *data)
{
    int32_t ret;
    uint32_t irqSave;
    struct GpioIrqRecord *irqRecord = (struct GpioIrqRecord *)data;

    if (irqRecord == NULL) {
        HDF_LOGI("GpioCntlrIrqThreadHandler: irqRecord is null!");
        return HDF_FAILURE;
    }
    while (true) {
        ret = OsalSemWait(&irqRecord->sem, HDF_WAIT_FOREVER);
        if (irqRecord->removed) {
            break;
        }
        if (ret != HDF_SUCCESS) {
            continue;
        }
        if (irqRecord->btmFunc != NULL) {
            (void)irqRecord->btmFunc(irqRecord->global, irqRecord->irqData);
        }
    }
    (void)OsalSpinLockIrqSave(&irqRecord->spin, &irqSave);
    (void)OsalSpinUnlockIrqRestore(&irqRecord->spin, &irqSave); // last post done
    HDF_LOGI("GpioCntlrIrqThreadHandler: gpio(%hu) thread exit!", irqRecord->global);
    (void)OsalSemDestroy(&irqRecord->sem);
    (void)OsalSpinDestroy(&irqRecord->spin);
    (void)OsalThreadDestroy(&irqRecord->thread);
    OsalMemFree(irqRecord);
    return HDF_SUCCESS;
}

static int32_t GpioCntlrSetIrqInner(struct GpioInfo *ginfo, struct GpioIrqRecord *irqRecord)
{
    int32_t ret;
    uint16_t local = GpioInfoToLocal(ginfo);
    struct GpioCntlr *cntlr = ginfo->cntlr;

    if (cntlr == NULL) {
        HDF_LOGE("GpioCntlrSetIrqInner: cntlr is null!");
        return HDF_ERR_INVALID_PARAM;
    }
    GpioInfoLock(ginfo);

    if (ginfo->irqRecord != NULL) {
        GpioInfoUnlock(ginfo);
        HDF_LOGE("GpioCntlrSetIrqInner: gpio(%hu+%hu) irq already set!", cntlr->start, local);
        return HDF_ERR_NOT_SUPPORT;
    }

    ginfo->irqRecord = irqRecord;
    GpioInfoUnlock(ginfo);
    ret = cntlr->ops->setIrq(cntlr, local, irqRecord->mode);
    if (ret != HDF_SUCCESS) {
        GpioInfoLock(ginfo);
        HDF_LOGE("GpioCntlrSetIrqInner: gpio fail to setIrq!");
        ginfo->irqRecord = NULL;
        GpioInfoUnlock(ginfo);
    }
    return ret;
}

static int32_t GpioIrqRecordCreate(struct GpioInfo *ginfo, uint16_t mode, GpioIrqFunc func, void *arg,
    struct GpioIrqRecord **new)
{
    int32_t ret;
    struct GpioIrqRecord *irqRecord = NULL;
    struct OsalThreadParam cfg;

    irqRecord = (struct GpioIrqRecord *)OsalMemCalloc(sizeof(*irqRecord));
    if (irqRecord == NULL) {
        HDF_LOGE("GpioIrqRecordCreate: fail to calloc for irqRecord!");
        return HDF_ERR_MALLOC_FAIL;
    }
    irqRecord->removed = false;
    irqRecord->mode = mode;
    irqRecord->irqFunc = ((mode & GPIO_IRQ_USING_THREAD) == 0) ? func : NULL;
    irqRecord->btmFunc = ((mode & GPIO_IRQ_USING_THREAD) != 0) ? func : NULL;
    irqRecord->irqData = arg;
    irqRecord->global = GpioInfoToGlobal(ginfo);
    if (irqRecord->btmFunc != NULL) {
        ret = OsalThreadCreate(&irqRecord->thread, GpioCntlrIrqThreadHandler, irqRecord);
        if (ret != HDF_SUCCESS) {
            OsalMemFree(irqRecord);
            HDF_LOGE("GpioIrqRecordCreate: fail to create irq thread, ret: %d!", ret);
            return ret;
        }
        (void)OsalSpinInit(&irqRecord->spin);
        (void)OsalSemInit(&irqRecord->sem, 0);
        cfg.name = (char *)ginfo->name;
        cfg.priority = OSAL_THREAD_PRI_HIGHEST;
        cfg.stackSize = GPIO_IRQ_STACK_SIZE;
        ret = OsalThreadStart(&irqRecord->thread, &cfg);
        if (ret != HDF_SUCCESS) {
            (void)OsalSemDestroy(&irqRecord->sem);
            (void)OsalSpinDestroy(&irqRecord->spin);
            (void)OsalThreadDestroy(&irqRecord->thread);
            OsalMemFree(irqRecord);
            HDF_LOGE("GpioIrqRecordCreate: fail to start irq thread, ret: %d!", ret);
            return ret;
        }
        HDF_LOGI("GpioIrqRecordCreate: gpio(%hu) thread started!", GpioInfoToGlobal(ginfo));
    }

    *new = irqRecord;
    return HDF_SUCCESS;
}

int32_t GpioCntlrSetIrq(struct GpioCntlr *cntlr, uint16_t local, uint16_t mode, GpioIrqFunc func, void *arg)
{
    int32_t ret;
    struct GpioInfo *ginfo = NULL;
    struct GpioIrqRecord *irqRecord = NULL;

    if (cntlr == NULL || cntlr->ginfos == NULL) {
        HDF_LOGE("GpioCntlrSetIrq: cntlr or ginfos is null!");
        return HDF_ERR_INVALID_OBJECT;
    }
    if (local >= cntlr->count || func == NULL) {
        HDF_LOGE("GpioCntlrSetIrq: local is invalid or func is null!");
        return HDF_ERR_INVALID_PARAM;
    }
    if (cntlr->ops == NULL || cntlr->ops->setIrq == NULL) {
        HDF_LOGE("GpioCntlrSetIrq: ops or setIrq is null!");
        return HDF_ERR_NOT_SUPPORT;
    }

    ginfo = &cntlr->ginfos[local];
    ret = GpioIrqRecordCreate(ginfo, mode, func, arg, &irqRecord);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("GpioCntlrSetIrq: fail to create irq record, ret: %d!", ret);
        return ret;
    }

    ret = GpioCntlrSetIrqInner(ginfo, irqRecord);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("GpioCntlrSetIrq: fail to set irq inner, ret: %d!", ret);
        GpioIrqRecordDestroy(irqRecord);
    }
    return ret;
}

int32_t GpioCntlrUnsetIrq(struct GpioCntlr *cntlr, uint16_t local, void *arg)
{
    int32_t ret;
    struct GpioInfo *ginfo = NULL;
    struct GpioIrqRecord *irqRecord = NULL;

    if (cntlr == NULL || cntlr->ginfos == NULL) {
        HDF_LOGE("GpioCntlrUnsetIrq: cntlr or ginfos is null!");
        return HDF_ERR_INVALID_OBJECT;
    }
    if (local >= cntlr->count) {
        HDF_LOGE("GpioCntlrUnsetIrq: local is invalid!");
        return HDF_ERR_INVALID_PARAM;
    }
    if (cntlr->ops == NULL || cntlr->ops->unsetIrq == NULL) {
        HDF_LOGE("GpioCntlrUnsetIrq: ops or unsetIrq is null!");
        return HDF_ERR_NOT_SUPPORT;
    }

    ginfo = &cntlr->ginfos[local];
    GpioInfoLock(ginfo);
    if (ginfo->irqRecord == NULL) {
        GpioInfoUnlock(ginfo);
        HDF_LOGE("GpioCntlrUnsetIrq: gpio(%hu+%hu) irq not set yet!", cntlr->start, local);
        return HDF_ERR_NOT_SUPPORT;
    }
    irqRecord = ginfo->irqRecord;
    if (arg != irqRecord->irqData) {
        GpioInfoUnlock(ginfo);
        HDF_LOGE("GpioCntlrUnsetIrq: gpio(%hu+%hu) arg not match!", cntlr->start, local);
        return HDF_ERR_INVALID_PARAM;
    }
    ret = cntlr->ops->unsetIrq(cntlr, local);
    if (ret == HDF_SUCCESS) {
        ginfo->irqRecord = NULL;
    }
    GpioInfoUnlock(ginfo);

    if (ret == HDF_SUCCESS) {
        GpioIrqRecordDestroy(irqRecord);
    }
    return ret;
}

int32_t GpioCntlrEnableIrq(struct GpioCntlr *cntlr, uint16_t local)
{
    if (cntlr == NULL) {
        HDF_LOGE("GpioCntlrEnableIrq: cntlr is null!");
        return HDF_ERR_INVALID_OBJECT;
    }
    if (cntlr->ops == NULL || cntlr->ops->enableIrq == NULL) {
        HDF_LOGE("GpioCntlrEnableIrq: ops or enableIrq is null!");
        return HDF_ERR_NOT_SUPPORT;
    }

    return cntlr->ops->enableIrq(cntlr, local);
}

int32_t GpioCntlrDisableIrq(struct GpioCntlr *cntlr, uint16_t local)
{
    if (cntlr == NULL) {
        HDF_LOGE("GpioCntlrDisableIrq: cntlr is null!");
        return HDF_ERR_INVALID_OBJECT;
    }
    if (cntlr->ops == NULL || cntlr->ops->disableIrq == NULL) {
        HDF_LOGE("GpioCntlrDisableIrq: ops or disableIrq is null!");
        return HDF_ERR_NOT_SUPPORT;
    }

    return cntlr->ops->disableIrq(cntlr, local);
}

int32_t GpioCntlrGetNumByGpioName(struct GpioCntlr *cntlr, const char *gpioName)
{
    uint16_t index;

    if (cntlr == NULL) {
        HDF_LOGE("GpioCntlrGetNumByGpioName: cntlr is null!");
        return HDF_ERR_INVALID_OBJECT;
    }

    if (gpioName == NULL || strlen(gpioName) > GPIO_NAME_LEN) {
        HDF_LOGE("GpioCntlrGetNumByGpioName: gpioName is null or gpioName len out of range!");
        return HDF_ERR_INVALID_OBJECT;
    }

    for (index = 0; index < cntlr->count; index++) {
        if (strcmp(cntlr->ginfos[index].name, gpioName) == 0) {
            HDF_LOGD("GpioCntlrGetNumByGpioName: gpioName:%s get gpio number: %d success!", gpioName,
                (int32_t)(cntlr->start + index));
            return (int32_t)(cntlr->start + index);
        }
    }
    HDF_LOGE("GpioCntlrGetNumByGpioName: gpioName:%s fail to get gpio number!", gpioName);
    return HDF_FAILURE;
}
