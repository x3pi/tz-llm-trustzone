/*
 * Copyright (c) 2022-2023 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include "can/can_msg.h"
#include "hdf_dlist.h"
#include "hdf_log.h"
#include "osal_atomic.h"
#include "osal_mem.h"
#include "osal_time.h"
#include "securec.h"

#define HDF_LOG_TAG can_msg

#define CAN_MSG_POOL_SIZE_MAX 32

struct CanMsgHolder {
    struct CanMsg cmsg;
    struct HdfSRef ref;
    OsalAtomic available;
};

struct CanMsgPool {
    size_t size;
    struct CanMsgHolder *holders;
};

static struct CanMsgHolder *CanMsgPoolAcquireHolder(struct CanMsgPool *pool)
{
    size_t i;
    struct CanMsgHolder *holder = NULL;

    for (i = 0; i < pool->size; i++) {
        holder = &pool->holders[i];
        if (OsalAtomicRead(&holder->available) < 1) {
            continue;
        }
        if (OsalAtomicDecReturn(&holder->available) != 0) {
            OsalAtomicInc(&holder->available);
            continue;
        }
        (void)memset_s(holder, sizeof(*holder), 0, sizeof(*holder));
        return holder;
    }
    return NULL;
}

static void CanMsgPoolRecycleHolder(struct CanMsgHolder *holder)
{
    if (holder == NULL) {
        HDF_LOGE("CanMsgPoolRecycleHolder: holder is null!");
        return;
    }
    OsalAtomicInc(&holder->available);
}

void CanMsgGet(const struct CanMsg *msg)
{
    struct CanMsgHolder *holder = NULL;

    if (msg == NULL) {
        HDF_LOGE("CanMsgGet: msg is null!");
        return;
    }
    holder = CONTAINER_OF(msg, struct CanMsgHolder, cmsg);
    HdfSRefAcquire(&holder->ref);
}

void CanMsgPut(const struct CanMsg *msg)
{
    struct CanMsgHolder *holder = NULL;

    if (msg == NULL) {
        HDF_LOGE("CanMsgPut: msg is null!");
        return;
    }
    holder = CONTAINER_OF(msg, struct CanMsgHolder, cmsg);
    HdfSRefRelease(&holder->ref);
}

static void CanMsgHolderOnFirstGet(struct HdfSRef *sref)
{
    (void)sref;
}

static void CanMsgHolderOnLastPut(struct HdfSRef *sref)
{
    struct CanMsgHolder *holder = NULL;

    if (sref == NULL) {
        HDF_LOGE("CanMsgHolderOnLastPut: sref is null!");
        return;
    }
    holder = CONTAINER_OF(sref, struct CanMsgHolder, ref);
    CanMsgPoolRecycleHolder(holder);
}

struct IHdfSRefListener g_canMsgExtListener = {
    .OnFirstAcquire = CanMsgHolderOnFirstGet,
    .OnLastRelease = CanMsgHolderOnLastPut,
};

struct CanMsg *CanMsgPoolObtainMsg(struct CanMsgPool *pool)
{
    struct CanMsgHolder *holder = NULL;

    if (pool == NULL) {
        HDF_LOGE("CanMsgPoolObtainMsg: pool is null!");
        return NULL;
    }

    holder = CanMsgPoolAcquireHolder(pool);
    if (holder == NULL) {
        HDF_LOGE("CanMsgPoolObtainMsg: holder is null!");
        return NULL;
    }

    HdfSRefConstruct(&holder->ref, &g_canMsgExtListener);
    CanMsgGet(&holder->cmsg);
    return &holder->cmsg;
}

struct CanMsgPool *CanMsgPoolCreate(size_t size)
{
    size_t i;
    struct CanMsgPool *pool = NULL;

    if (size > CAN_MSG_POOL_SIZE_MAX) {
        HDF_LOGE("CanMsgPoolCreate: invalid pool size %zu", size);
        return NULL;
    }
    pool = OsalMemCalloc(sizeof(*pool) + sizeof(struct CanMsgHolder) * size);
    if (pool == NULL) {
        HDF_LOGE("CanMsgPoolCreate: alloc pool mem fail!");
        return NULL;
    }

    pool->size = size;
    pool->holders = (struct CanMsgHolder *)((uint8_t *)pool + sizeof(*pool));
    for (i = 0; i < size; i++) {
        OsalAtomicSet(&pool->holders[i].available, 1);
    }

    HDF_LOGI("CanMsgPoolCreate: end!");
    return pool;
}

void CanMsgPoolDestroy(struct CanMsgPool *pool)
{
    size_t drain;
    struct CanMsgHolder *holder = NULL;

    if (pool == NULL) {
        HDF_LOGE("CanMsgPoolDestroy: pool is null!");
        return;
    }
    drain = pool->size;
    while (drain > 0) {
        holder = CanMsgPoolAcquireHolder(pool);
        if (holder == NULL) {
            HDF_LOGI("CanMsgPoolDestroy: wait pool, drain = %zu", drain);
            OsalMSleep(200); //wait for 200 ms
            continue;
        }
        drain--;
    };
    HDF_LOGI("CanMsgPoolDestroy: end!");
    OsalMemFree(pool);
}
