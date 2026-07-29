/*
 * Copyright (c) 2022-2023 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include "can/can_manager.h"
#include "osal_time.h"
#include "securec.h"
#include "can/can_core.h"

#define HDF_LOG_TAG can_manager

#define CAN_NUMBER_MAX 32
#define CAN_MSG_POOL_SIZE_DFT 8
#define CAN_IRQ_STACK_SIZE (1024 * 8)

static struct PlatformManager *g_manager;

static struct PlatformManager *CanManagerGet(void)
{
    int32_t ret;

    if (g_manager == NULL) {
        ret = PlatformManagerCreate("CAN_BUS_MANAGER", &g_manager);
        if (ret != HDF_SUCCESS) {
            HDF_LOGE("CanManagerGet: create can manager fail, ret: %d!", ret);
        }
    }
    return g_manager;
}

static void CanManagerDestroyIfNeed(void)
{
    if (g_manager != NULL && DListIsEmpty(&g_manager->devices)) {
        PlatformManagerDestroy(g_manager);
        g_manager = NULL;
    }
}

static int32_t CanIrqThreadWorker(void *data)
{
    int32_t ret;
    struct CanCntlr *cntlr = (struct CanCntlr *)data;

    if (cntlr == NULL) {
        HDF_LOGE("CanIrqThreadWorker: cntlr is null!");
        return HDF_ERR_INVALID_OBJECT;
    }

    cntlr->threadStatus |= CAN_THREAD_RUNNING;
    while ((cntlr->threadStatus & CAN_THREAD_RUNNING) != 0) {
        /* wait event */
        ret = OsalSemWait(&cntlr->sem, HDF_WAIT_FOREVER);
        if (ret != HDF_SUCCESS) {
            continue;
        }
        if ((cntlr->threadStatus & CAN_THREAD_RUNNING) == 0) {
            break;  // exit thread
        }
        ret = CanCntlrOnNewMsg(cntlr, cntlr->irqMsg);
        if (ret != HDF_SUCCESS) {
            HDF_LOGE("CanIrqThreadWorker: CanCntlrOnNewMsg fail!");
        }
        cntlr->irqMsg = NULL;
        cntlr->threadStatus &= ~CAN_THREAD_PENDING;
    }

    cntlr->threadStatus |= CAN_THREAD_STOPPED;
    return HDF_SUCCESS;
}

static int32_t CanCntlrCreateThread(struct CanCntlr *cntlr)
{
    int32_t ret;
    struct OsalThreadParam config;

    if (memset_s(&config, sizeof(config), 0, sizeof(config)) != EOK) {
        HDF_LOGE("CanCntlrCreateThread: memset_s fail!");
        return HDF_ERR_IO;
    }
    ret = OsalSemInit(&cntlr->sem, 0);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("CanCntlrCreateThread: sem init fail!");
        return ret;
    }

    ret = OsalThreadCreate(&cntlr->thread, (OsalThreadEntry)CanIrqThreadWorker, cntlr);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("CanCntlrCreateThread: thread create fail!");
        (void)OsalSemDestroy(&cntlr->sem);
        return ret;
    }

    config.name = "CanIrqThread";
    config.priority = OSAL_THREAD_PRI_HIGH;
    config.stackSize = CAN_IRQ_STACK_SIZE;
    cntlr->threadStatus = 0;
    ret = OsalThreadStart(&cntlr->thread, &config);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("CanCntlrCreateThread: thread start fail!");
        OsalThreadDestroy(&cntlr->thread);
        (void)OsalSemDestroy(&cntlr->sem);
        return ret;
    }

    return HDF_SUCCESS;
}

static int32_t CanCntlrCheckAndInit(struct CanCntlr *cntlr)
{
    if (cntlr == NULL || cntlr->ops == NULL) {
        return HDF_ERR_INVALID_OBJECT;
    }

    if (cntlr->number < 0 || cntlr->number >= CAN_NUMBER_MAX) {
        HDF_LOGE("CanCntlrCheckAndInit: invlaid can num:%d!", cntlr->number);
        return HDF_ERR_INVALID_OBJECT;
    }

    DListHeadInit(&cntlr->rxBoxList);

    if (CanCntlrCreateThread(cntlr) != HDF_SUCCESS) {
        HDF_LOGE("CanCntlrCheckAndInit: create thread fail!");
        return HDF_FAILURE;
    }
    if (OsalMutexInit(&cntlr->lock) != HDF_SUCCESS) {
        HDF_LOGE("CanCntlrCheckAndInit: init lock fail!");
        return HDF_FAILURE;
    }

    if (OsalMutexInit(&cntlr->rboxListLock) != HDF_SUCCESS) {
        HDF_LOGE("CanCntlrCheckAndInit: init rx box list lock fail!");
        (void)OsalMutexDestroy(&cntlr->lock);
        return HDF_FAILURE;
    }

    if (cntlr->msgPoolSize <= 0) {
        cntlr->msgPoolSize = CAN_MSG_POOL_SIZE_DFT;
    }

    cntlr->msgPool = CanMsgPoolCreate(cntlr->msgPoolSize);
    if (cntlr->msgPool == NULL) {
        HDF_LOGE("CanCntlrCheckAndInit: create can msg pool fail!");
        (void)OsalMutexDestroy(&cntlr->rboxListLock);
        (void)OsalMutexDestroy(&cntlr->lock);
        return HDF_FAILURE;
    }

    return HDF_SUCCESS;
}

static void CanCntlrDestroyThread(struct CanCntlr *cntlr)
{
    int t = 0;

    cntlr->threadStatus &= ~CAN_THREAD_RUNNING;
    cntlr->irqMsg = NULL;
    (void)OsalSemPost(&cntlr->sem);
    while ((cntlr->threadStatus & CAN_THREAD_STOPPED) == 0) {
        OsalMSleep(1);
        if (t++ > THREAD_EXIT_TIMEOUT) {
            break;
        }
    }
    (void)OsalThreadDestroy(&cntlr->thread);
    (void)OsalSemDestroy(&cntlr->sem);
}

static void CanCntlrDeInit(struct CanCntlr *cntlr)
{
    HDF_LOGD("CanCntlrDeInit: enter!");
    CanCntlrDestroyThread(cntlr);
    CanMsgPoolDestroy(cntlr->msgPool);
    cntlr->msgPool = NULL;
    (void)OsalMutexDestroy(&cntlr->rboxListLock);
    (void)OsalMutexDestroy(&cntlr->lock);
    HDF_LOGD("CanCntlrDeInit: exit!");
}

int32_t CanCntlrAdd(struct CanCntlr *cntlr)
{
    int32_t ret;

    ret = CanCntlrCheckAndInit(cntlr);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("CanCntlrAdd: can cntlr check and init fail!");
        return ret;
    }

    cntlr->device.number = cntlr->number;
    ret = PlatformDeviceSetName(&cntlr->device, "CAN%d", cntlr->number);
    if (ret != HDF_SUCCESS) {
        CanCntlrDeInit(cntlr);
        HDF_LOGE("CanCntlrAdd: set name for platform device fail!");
        return ret;
    }

    cntlr->device.manager = CanManagerGet();
    if (cntlr->device.manager == NULL) {
        PlatformDeviceClearName(&cntlr->device);
        CanCntlrDeInit(cntlr);
        HDF_LOGE("CanCntlrAdd: get can manager fail!");
        return HDF_PLT_ERR_DEV_GET;
    }

    if ((ret = PlatformDeviceAdd(&cntlr->device)) != HDF_SUCCESS) {
        PlatformDeviceClearName(&cntlr->device);
        CanCntlrDeInit(cntlr);
        HDF_LOGE("CanCntlrAdd: add platform device fail!");
        return ret;
    }

    HDF_LOGI("CanCntlrAdd: add controller %d success!", cntlr->number);
    return HDF_SUCCESS;
}

int32_t CanCntlrDel(struct CanCntlr *cntlr)
{
    if (cntlr == NULL) {
        HDF_LOGE("CanCntlrDel: cntlr is null!");
        return HDF_ERR_INVALID_OBJECT;
    }

    PlatformDeviceDel(&cntlr->device);
    PlatformDeviceClearName(&cntlr->device);

    CanCntlrDeInit(cntlr);

    CanManagerDestroyIfNeed();
    HDF_LOGI("CanCntlrDel: del controller %d success!", cntlr->number);
    return HDF_SUCCESS;
}

static struct CanCntlr *CanCntlrFromPlatformDevice(const struct PlatformDevice *pdevice)
{
    return CONTAINER_OF(pdevice, struct CanCntlr, device);
}

struct CanCntlr *CanCntlrGetByName(const char *name)
{
    static struct PlatformManager *manager = NULL;
    struct PlatformDevice *pdevice = NULL;

    manager = CanManagerGet();
    if (manager == NULL) {
        HDF_LOGE("CanCntlrGetByName: get can manager fail!");
        return NULL;
    }

    pdevice = PlatformManagerGetDeviceByName(manager, name);
    if (pdevice == NULL) {
        HDF_LOGE("CanCntlrGetByName: get platform device fail!");
        return NULL;
    }

    return CanCntlrFromPlatformDevice(pdevice);
}

struct CanCntlr *CanCntlrGetByNumber(int32_t number)
{
    static struct PlatformManager *manager = NULL;
    struct PlatformDevice *pdevice = NULL;

    manager = CanManagerGet();
    if (manager == NULL) {
        HDF_LOGE("CanCntlrGetByNumber: get can manager fail!");
        return NULL;
    }

    pdevice = PlatformManagerGetDeviceByNumber(manager, number);
    if (pdevice == NULL) {
        HDF_LOGE("CanCntlrGetByNumber: get platform device fail!");
        return NULL;
    }

    return CanCntlrFromPlatformDevice(pdevice);
}

void CanCntlrPut(struct CanCntlr *cntlr)
{
    if (cntlr != NULL) {
        PlatformDevicePut(&cntlr->device);
    }
}
