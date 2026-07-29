/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include <securec.h>
#include <hdf_log.h>
#include <osal_mem.h>
#include "osal_uaccess.h"
#include "buffer_queue.h"
#include "camera_buffer_manager.h"

#define HDF_LOG_TAG HDF_CAMERA_BUFFER_MANAGER
struct BufferQueue *g_queueForMmap = NULL;

void CameraBufferDone(struct CameraBuffer *buffer, enum BufferState state)
{
    struct BufferQueue *queue = buffer->bufferQueue;
    uint32_t flags;

    if (buffer->state != BUFFER_STATE_ACTIVE) {
        return;
    }

    if (state != BUFFER_STATE_DONE && state != BUFFER_STATE_ERROR && state != BUFFER_STATE_QUEUED) {
        state = BUFFER_STATE_ERROR;
    }
    HDF_LOGI("%s: Done processing on buffer %{public}u!", __func__, buffer->id);

    if (state != BUFFER_STATE_QUEUED) {
        CameraBufferSyncForUser(buffer);
    }
    OsalSpinLockIrqSave(&queue->doneLock, &flags);
    buffer->state = state;
    if (state != BUFFER_STATE_QUEUED) {
        DListInsertTail(&buffer->doneEntry, &queue->doneList);
    }

    OsalAtomicDec(&queue->driverOwnCount);
    OsalSpinUnlockIrqRestore(&queue->doneLock, &flags);
    if (state != BUFFER_STATE_QUEUED) {
        wake_up(&queue->doneWait);
    }
}

int32_t BufferQueueFindPlaneByOffset(struct BufferQueue *queue,
    unsigned long off, uint32_t *bufferId, uint32_t *planeId)
{
    struct CameraBuffer *buffer = NULL;
    uint32_t bufferCnt;
    uint32_t planeCnt;

    if (queue == NULL || bufferId == NULL || planeId == NULL) {
        return HDF_ERR_INVALID_PARAM;
    }

    for (bufferCnt = 0; bufferCnt < queue->numBuffers; ++bufferCnt) {
        buffer = queue->buffers[bufferCnt];
        for (planeCnt = 0; planeCnt < buffer->numPlanes; ++planeCnt) {
            if (buffer->planes[planeCnt].memory.offset == off) {
                *bufferId = bufferCnt;
                *planeId = planeCnt;
                return HDF_SUCCESS;
            }
        }
    }

    return HDF_FAILURE;
}

void *CameraBufferGetPlaneVaddr(struct CameraBuffer *buffer, uint32_t planeId)
{
    struct BufferQueue *queue = buffer->bufferQueue;
    if (planeId >= buffer->numPlanes || buffer->planes[planeId].memPriv == NULL) {
        return NULL;
    }

    if (queue->memOps->getVaddr != NULL) {
        return queue->memOps->getVaddr(buffer->planes[planeId].memPriv);
    }

    return NULL;
}

/* init queue */
int32_t BufferQueueInit(struct BufferQueue *queue)
{
    if (queue == NULL) {
        HDF_LOGE("%s: queue ptr is null", __func__);
        return HDF_ERR_INVALID_PARAM;
    }

    if (queue->queueIsInit == true) {
        return HDF_SUCCESS;
    }

    if (queue->queueOps == NULL || queue->memOps == NULL || queue->ioModes == 0) {
        HDF_LOGE("%s: queueOps/memOps is null or ioModes is 0", __func__);
        return HDF_FAILURE;
    }

    if (queue->queueOps->queueSetup == NULL || queue->queueOps->queueBuffer == NULL) {
        HDF_LOGE("%s: queueSetup or queueBuffer is null", __func__);
        return HDF_FAILURE;
    }

    if (queue->bufferSize == 0) {
        queue->bufferSize = sizeof(struct CameraBuffer);
    }
    queue->memType = MEMTYPE_UNKNOWN;
    DListHeadInit(&queue->queuedList);
    DListHeadInit(&queue->doneList);
    OsalSpinInit(&queue->doneLock);
    OsalMutexInit(&queue->mmapLock);
    init_waitqueue_head(&queue->doneWait);
    MemoryAdapterQueueImpInit(queue);
    queue->queueIsInit = true;

    return HDF_SUCCESS;
}

/* request buffers */
int32_t BufferQueueRequest(struct BufferQueue *queue, struct UserCameraReq *userRequest)
{
    uint32_t numBuffers;
    uint32_t numPlanes = 0;
    uint32_t planeSizes[MAX_PLANES] = { 0 };
    uint32_t i;
    int32_t ret;

    ret = BufferQueueCheckMemOps(queue, userRequest->memType);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: BufferQueueCheckMemOps failed, ret = %{public}d", __func__, ret);
        return HDF_FAILURE;
    }
    if (BufferQueueReleaseBuffers(queue, userRequest) != HDF_SUCCESS) {
        return HDF_FAILURE;
    }
    if (userRequest->count == 0) {
        return HDF_SUCCESS;
    }
    if (queue->minBuffersNeeded > MAX_FRAME) {
        HDF_LOGW("%s: Buffer needed is too many", __func__);
    }

    numBuffers = userRequest->count > queue->minBuffersNeeded ? userRequest->count : queue->minBuffersNeeded;
    numBuffers = numBuffers > MAX_FRAME ? MAX_FRAME : numBuffers;
    queue->memType = userRequest->memType;
    ret = queue->queueOps->queueSetup != NULL ? queue->queueOps->queueSetup(queue, &numBuffers,
        &numPlanes, planeSizes) : HDF_SUCCESS;
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: queue setup failed", __func__);
        return ret;
    }
    if (numPlanes == 0) {
        HDF_LOGE("%s: numPlanes cannot be zero", __func__);
        return HDF_FAILURE;
    }
    for (i = 0; i < numPlanes; ++i) {
        if (planeSizes[i] == 0) {
            return HDF_FAILURE;
        }
    }

    ret = BufferQueueRequestBuffers(queue, userRequest, numBuffers, numPlanes, planeSizes);
    if (ret != HDF_SUCCESS) {
        return ret;
    }

    return HDF_SUCCESS;
}

/* query buffer */
int32_t BufferQueueQueryBuffer(struct BufferQueue *queue, struct UserCameraBuffer *userBuffer)
{
    struct CameraBuffer *buffer = NULL;
    int32_t ret;

    if (userBuffer->id >= queue->numBuffers) {
        HDF_LOGE("%s: user buffer id cannot >= queue numBuffers", __func__);
        return HDF_FAILURE;
    }

    buffer = queue->buffers[userBuffer->id];
    ret = CameraBufferCheckPlanes(buffer, userBuffer);
    if (ret == HDF_SUCCESS) {
        CameraBufferToUserBuffer(buffer, userBuffer);
    }

    return ret;
}

/* return buffer */
int32_t BufferQueueReturnBuffer(struct BufferQueue *queue, struct UserCameraBuffer *userBuffer)
{
    if (userBuffer == NULL) {
        return HDF_ERR_INVALID_PARAM;
    }
    struct CameraBuffer *buffer = NULL;
    int32_t ret;
    ret = BufferQueuePrepare(queue, userBuffer);
    if (ret != HDF_SUCCESS) {
        return ret;
    }
    if ((queue->flags & QUEUE_STATE_ERROR) != 0) {
        HDF_LOGE("%s: fatal error occurred on queue", __func__);
        return HDF_FAILURE;
    }
    buffer = queue->buffers[userBuffer->id];
    if (buffer->state != BUFFER_STATE_DEQUEUED) {
        HDF_LOGE("%s: invalid buffer state", __func__);
        return HDF_FAILURE;
    }
    if ((buffer->flags & BUFFER_PREPARED) == 0) {
        ret = CameraBufferFromUserBuffer(buffer, userBuffer);
        if (ret != HDF_SUCCESS) {
            HDF_LOGE("%s: Buffer prepare failed", __func__);
            return ret;
        }
    }
    CameraBufferQueueBuffer(buffer);
    CameraBufferToUserBuffer(buffer, userBuffer);

    if ((queue->flags & QUEUE_STATE_STREAMING) != 0 && (queue->flags & QUEUE_STATE_STREAMING_CALLED) == 0 &&
        (queue->queuedCount >= queue->minBuffersNeeded)) {
        ret = BufferQueueStart(queue);
        if (ret != HDF_SUCCESS) {
            HDF_LOGE("%s: BufferQueueStart failed", __func__);
            return ret;
        }
    }
    return HDF_SUCCESS;
}

static int32_t WaitForDoneBuffer(struct BufferQueue *queue, uint32_t blocking)
{
    while (true) {
        if ((queue->flags & QUEUE_STATE_WAITING_DEQUEUE) != 0 || (queue->flags & QUEUE_STATE_STREAMING) == 0 ||
            (queue->flags & QUEUE_STATE_ERROR) != 0 || (queue->flags & QUEUE_STATE_LAST_BUFFER_DEQUEUED) != 0) {
            return HDF_FAILURE;
        }

        if (!DListIsEmpty(&queue->doneList)) {
            break;
        }

        if (blocking == 0) {
            return HDF_FAILURE;
        }
        queue->flags |= QUEUE_STATE_WAITING_DEQUEUE;

        MemoryAdapterDriverMutexLock(queue);
        int32_t ret = wait_event_interruptible(queue->doneWait, !DListIsEmpty(&queue->doneList) ||
            (queue->flags & QUEUE_STATE_STREAMING) == 0 || (queue->flags & QUEUE_STATE_ERROR) != 0);
        MemoryAdapterDriverMutexUnLock(queue);

        queue->flags &= ~QUEUE_STATE_WAITING_DEQUEUE;
        if (ret != 0) {
            HDF_LOGE("%s: wait function failed", __func__);
            return HDF_FAILURE;
        }
    }

    return HDF_SUCCESS;
}

static int32_t GetDoneBuffer(struct BufferQueue *queue,
    struct CameraBuffer **buffer, struct UserCameraBuffer *userBuffer)
{
    uint32_t flags;
    int32_t ret;

    ret = WaitForDoneBuffer(queue, userBuffer->flags & USER_BUFFER_BLOCKING);
    if (ret == HDF_FAILURE) {
        return ret;
    }
    OsalSpinLockIrqSave(&queue->doneLock, &flags);
    *buffer = DLIST_FIRST_ENTRY(&queue->doneList, struct CameraBuffer, doneEntry);
    ret = CameraBufferCheckPlanes(*buffer, userBuffer);
    if (ret == HDF_SUCCESS) {
        DListRemove(&(*buffer)->doneEntry);
    }
    OsalSpinUnlockIrqRestore(&queue->doneLock, &flags);

    return ret;
}

/* acquire buffer */
int32_t BufferQueueAcquireBuffer(struct BufferQueue *queue, struct UserCameraBuffer *userBuffer)
{
    struct CameraBuffer *buffer = NULL;
    int32_t ret;

    if (userBuffer == NULL) {
        return HDF_FAILURE;
    }
    ret = GetDoneBuffer(queue, &buffer, userBuffer);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: get buffer from doneList failed", __func__);
        return ret;
    }
    switch (buffer->state) {
        case BUFFER_STATE_DONE:
            HDF_LOGE("%s: return done buffer", __func__);
            break;
        case BUFFER_STATE_ERROR:
            HDF_LOGE("%s: return done buffer with error", __func__);
            break;
        default:
            HDF_LOGE("%s: invalid buffer state", __func__);
            return HDF_FAILURE;
    }
    buffer->flags &= ~BUFFER_PREPARED;

    CameraBufferToUserBuffer(buffer, userBuffer);
    DListRemove(&buffer->queueEntry);
    queue->queuedCount--;
    buffer->state = BUFFER_STATE_DEQUEUED;
    userBuffer->flags |= USER_BUFFER_DONE;

    return ret;
}

/* stream on */
int32_t BufferQueueStreamOn(struct BufferQueue *queue)
{
    if ((queue->flags & QUEUE_STATE_STREAMING) != 0 || (queue->numBuffers) == 0 ||
        (queue->numBuffers < queue->minBuffersNeeded)) {
        return HDF_FAILURE;
    }

    if (queue->queuedCount >= queue->minBuffersNeeded) {
        int32_t ret = BufferQueueStart(queue);
        if (ret != HDF_SUCCESS) {
            HDF_LOGE("%s: BufferQueueStart failed", __func__);
            return ret;
        }
    }
    queue->flags |= QUEUE_STATE_STREAMING;
    return HDF_SUCCESS;
}

/* stream off */
int32_t BufferQueueStreamOff(struct BufferQueue *queue)
{
    BufferQueueStop(queue);

    queue->flags |= QUEUE_STATE_WAITING_BUFFERS;
    queue->flags &= ~QUEUE_STATE_LAST_BUFFER_DEQUEUED;
    HDF_LOGD("%s: stream off success", __func__);

    return HDF_SUCCESS;
}

void BufferQueueSetQueueForMmap(struct BufferQueue *queue)
{
    g_queueForMmap = queue;
}

struct BufferQueue *BufferQueueGetQueueForMmap(void)
{
    return g_queueForMmap;
}
