/*
 * Copyright (c) 2022-2023 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include "can/can_core.h"
#include "can/can_mail.h"
#include "can/can_msg.h"

static int32_t CanCntlrLock(struct CanCntlr *cntlr)
{
    if (cntlr->ops != NULL && cntlr->ops->lock != NULL) {
        return cntlr->ops->lock(cntlr);
    } else {
        return OsalMutexLock(&cntlr->lock);
    }
}

static void CanCntlrUnlock(struct CanCntlr *cntlr)
{
    if (cntlr->ops != NULL && cntlr->ops->unlock != NULL) {
        cntlr->ops->unlock(cntlr);
    } else {
        (void)OsalMutexUnlock(&cntlr->lock);
    }
}

int32_t CanCntlrWriteMsg(struct CanCntlr *cntlr, const struct CanMsg *msg)
{
    int32_t ret;

    if (cntlr == NULL) {
        HDF_LOGE("CanCntlrWriteMsg: cntlr is null!");
        return HDF_ERR_INVALID_OBJECT;
    }
    if (cntlr->ops == NULL || cntlr->ops->sendMsg == NULL) {
        HDF_LOGE("CanCntlrWriteMsg: ops or sendMsg is null!");
        return HDF_ERR_NOT_SUPPORT;
    }

    if ((ret = CanCntlrLock(cntlr)) != HDF_SUCCESS) {
        HDF_LOGE("CanCntlrWriteMsg: can cntlr lock fail!");
        return ret;
    }
    ret = cntlr->ops->sendMsg(cntlr, msg);
    CanCntlrUnlock(cntlr);
    return ret;
}

int32_t CanCntlrSetCfg(struct CanCntlr *cntlr, const struct CanConfig *cfg)
{
    int32_t ret;

    if (cntlr == NULL) {
        HDF_LOGE("CanCntlrSetCfg: cntlr is null!");
        return HDF_ERR_INVALID_OBJECT;
    }
    if (cntlr->ops == NULL || cntlr->ops->setCfg == NULL) {
        HDF_LOGE("CanCntlrSetCfg: ops or setCfg is null!");
        return HDF_ERR_NOT_SUPPORT;
    }

    if ((ret = CanCntlrLock(cntlr)) != HDF_SUCCESS) {
        HDF_LOGE("CanCntlrSetCfg: can cntlr lock fail!");
        return ret;
    }
    ret = cntlr->ops->setCfg(cntlr, cfg);
    CanCntlrUnlock(cntlr);
    return ret;
}

int32_t CanCntlrGetCfg(struct CanCntlr *cntlr, struct CanConfig *cfg)
{
    int32_t ret;

    if (cntlr == NULL) {
        HDF_LOGE("CanCntlrGetCfg: cntlr is null!");
        return HDF_ERR_INVALID_OBJECT;
    }
    if (cntlr->ops == NULL || cntlr->ops->getCfg == NULL) {
        HDF_LOGE("CanCntlrGetCfg: ops or setCfg is null!");
        return HDF_ERR_NOT_SUPPORT;
    }

    if ((ret = CanCntlrLock(cntlr)) != HDF_SUCCESS) {
        HDF_LOGE("CanCntlrGetCfg: can cntlr lock fail!");
        return ret;
    }
    ret = cntlr->ops->getCfg(cntlr, cfg);
    CanCntlrUnlock(cntlr);
    return ret;
}

int32_t CanCntlrGetState(struct CanCntlr *cntlr)
{
    int32_t ret;

    if (cntlr == NULL) {
        HDF_LOGE("CanCntlrGetState: cntlr is null!");
        return HDF_ERR_INVALID_OBJECT;
    }
    if (cntlr->ops == NULL || cntlr->ops->getState == NULL) {
        HDF_LOGE("CanCntlrGetState: ops or setCfg is null!");
        return HDF_ERR_NOT_SUPPORT;
    }

    if ((ret = CanCntlrLock(cntlr)) != HDF_SUCCESS) {
        HDF_LOGE("CanCntlrGetState: can cntlr lock fail!");
        return ret;
    }
    ret = cntlr->ops->getState(cntlr);
    CanCntlrUnlock(cntlr);
    return ret;
}

int32_t CanCntlrAddRxBox(struct CanCntlr *cntlr, struct CanRxBox *rxBox)
{
    if (cntlr == NULL) {
        HDF_LOGE("CanCntlrAddRxBox: cntlr is null!");
        return HDF_ERR_INVALID_OBJECT;
    }
    if (rxBox == NULL) {
        HDF_LOGE("CanCntlrAddRxBox: rxBox is null!");
        return HDF_ERR_INVALID_PARAM;
    }
    (void)OsalMutexLock(&cntlr->rboxListLock);
    DListInsertTail(&rxBox->node, &cntlr->rxBoxList);
    (void)OsalMutexUnlock(&cntlr->rboxListLock);
    return HDF_SUCCESS;
}

int32_t CanCntlrDelRxBox(struct CanCntlr *cntlr, struct CanRxBox *rxBox)
{
    struct CanRxBox *tmp = NULL;
    struct CanRxBox *toRmv = NULL;

    if (cntlr == NULL) {
        HDF_LOGE("CanCntlrDelRxBox: cntlr is null!");
        return HDF_ERR_INVALID_OBJECT;
    }
    if (rxBox == NULL) {
        HDF_LOGE("CanCntlrDelRxBox: rxBox is null!");
        return HDF_ERR_INVALID_PARAM;
    }
    (void)OsalMutexLock(&cntlr->rboxListLock);
    DLIST_FOR_EACH_ENTRY_SAFE(toRmv, tmp, &cntlr->rxBoxList, struct CanRxBox, node) {
        if (toRmv == rxBox) {
            DListRemove(&toRmv->node);
            (void)OsalMutexUnlock(&cntlr->rboxListLock);
            return HDF_SUCCESS;
        }
    }
    (void)OsalMutexUnlock(&cntlr->rboxListLock);
    HDF_LOGE("CanCntlrDelRxBox: del rxBox is not support!");
    return HDF_ERR_NOT_SUPPORT;
}

static int32_t CanCntlrMsgDispatch(struct CanCntlr *cntlr, struct CanMsg *msg)
{
    struct CanRxBox *rxBox = NULL;

    (void)OsalMutexLock(&cntlr->rboxListLock);
    DLIST_FOR_EACH_ENTRY(rxBox, &cntlr->rxBoxList, struct CanRxBox, node) {
        (void)CanRxBoxAddMsg(rxBox, msg);
    }
    (void)OsalMutexUnlock(&cntlr->rboxListLock);
    CanMsgPut(msg);

    return HDF_SUCCESS;
}

int32_t CanCntlrOnNewMsg(struct CanCntlr *cntlr, const struct CanMsg *msg)
{
    struct CanMsg *copy = NULL;

    if (cntlr == NULL) {
        HDF_LOGE("CanCntlrOnNewMsg: cntlr is null!");
        return HDF_ERR_INVALID_OBJECT;
    }

    if (msg == NULL) {
        HDF_LOGE("CanCntlrOnNewMsg: msg is null!");
        return HDF_ERR_INVALID_PARAM;
    }

    copy = CanMsgPoolObtainMsg(cntlr->msgPool);
    if (copy == NULL) {
        HDF_LOGE("CanCntlrOnNewMsg: can msg pool obtain msg fail!");
        return HDF_FAILURE;
    }
    *copy = *msg;

    return CanCntlrMsgDispatch(cntlr, copy); // gona call in thread context later ...
}

int32_t CanCntlrOnNewMsgIrqSafe(struct CanCntlr *cntlr, const struct CanMsg *msg)
{
    if (cntlr == NULL) {
        HDF_LOGE("CanCntlrOnNewMsgIrqSafe: cntlr is null!");
        return HDF_ERR_INVALID_OBJECT;
    }

    if (msg == NULL) {
        HDF_LOGE("CanCntlrOnNewMsgIrqSafe: msg is null!");
        return HDF_ERR_INVALID_PARAM;
    }

    if ((cntlr->threadStatus & CAN_THREAD_RUNNING) == 0 || (cntlr->threadStatus & CAN_THREAD_PENDING) != 0) {
        HDF_LOGE("CanCntlrOnNewMsgIrqSafe: device is busy!");
        return HDF_ERR_DEVICE_BUSY;
    }
    cntlr->irqMsg = msg;
    cntlr->threadStatus |= CAN_THREAD_PENDING;
    (void)OsalSemPost(&cntlr->sem);
    return HDF_SUCCESS;
}
