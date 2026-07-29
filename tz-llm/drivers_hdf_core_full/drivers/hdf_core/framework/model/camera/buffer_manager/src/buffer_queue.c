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
#include "camera_buffer_manager.h"
#include "camera_buffer.h"
#include "camera_device_manager.h"
#include "buffer_queue.h"

#define HDF_LOG_TAG HDF_CAMERA_QUEUE

void BufferQueueStop(struct BufferQueue *queue)
{
    uint32_t i;

    if (OsalAtomicRead(&queue->driverOwnCount) != 0) {
        for (i = 0; i < queue->numBuffers; ++i) {
            if (queue->buffers[i]->state == BUFFER_STATE_ACTIVE) {
                CameraBufferDone(queue->buffers[i], BUFFER_STATE_ERROR);
            }
        }
        HDF_LOGI("%s: Expect zero, driverOwnCount = %{public}d", __func__, OsalAtomicRead(&queue->driverOwnCount));
    }

    queue->flags &= ~QUEUE_STATE_STREAMING;
    queue->flags &= ~QUEUE_STATE_STREAMING_CALLED;
    queue->queuedCount = 0;
    queue->flags &= ~QUEUE_STATE_ERROR;

    DListHeadInit(&queue->queuedList);
    DListHeadInit(&queue->doneList);
    OsalAtomicSet(&queue->driverOwnCount, 0);
    wake_up_interruptible(&queue->doneWait);

    for (i = 0; i < queue->numBuffers; ++i) {
        struct CameraBuffer *buffer = queue->buffers[i];
        CameraBufferSyncForUser(buffer);
        buffer->flags &= ~BUFFER_PREPARED;
        buffer->state = BUFFER_STATE_DEQUEUED;
    }
}

static int32_t BufferQueueCheckMmapOps(struct BufferQueue *queue)
{
    if (((queue->ioModes & MEMTYPE_MMAP) == 0) || (queue->memOps->mmapAlloc == NULL) ||
        (queue->memOps->mmapFree == NULL) || (queue->memOps->mmap == NULL)) {
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

static int32_t BufferQueueCheckUserPtrOps(struct BufferQueue *queue)
{
    if (((queue->ioModes & MEMTYPE_USERPTR) == 0) || (queue->memOps->allocUserPtr == NULL) ||
        (queue->memOps->freeUserPtr == NULL)) {
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

static int32_t BufferQueueCheckDmaBufOps(struct BufferQueue *queue)
{
    if (((queue->ioModes & MEMTYPE_DMABUF) == 0) || (queue->memOps->attachDmaBuf == NULL) ||
        (queue->memOps->detachDmaBuf == NULL) || (queue->memOps->mapDmaBuf == NULL) ||
        (queue->memOps->unmapDmaBuf == NULL)) {
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

int32_t BufferQueueCheckMemOps(struct BufferQueue *queue, enum CameraMemType memType)
{
    if (memType != MEMTYPE_MMAP && memType != MEMTYPE_USERPTR && memType != MEMTYPE_DMABUF) {
        HDF_LOGE("%s: memType is fault!", __func__);
        return HDF_ERR_INVALID_PARAM;
    }
    if (memType == MEMTYPE_MMAP && BufferQueueCheckMmapOps(queue) != HDF_SUCCESS) {
        HDF_LOGE("%s: MMAP for current setup unsupported!", __func__);
        return HDF_ERR_NOT_SUPPORT;
    }
    if (memType == MEMTYPE_USERPTR && BufferQueueCheckUserPtrOps(queue) != HDF_SUCCESS) {
        HDF_LOGE("%s: USERPTR for current setup unsupported!", __func__);
        return HDF_ERR_NOT_SUPPORT;
    }
    if (memType == MEMTYPE_DMABUF && BufferQueueCheckDmaBufOps(queue) != HDF_SUCCESS) {
        HDF_LOGE("%s: DMABUF for current setup unsupported!", __func__);
        return HDF_ERR_NOT_SUPPORT;
    }
    return HDF_SUCCESS;
}

static int32_t BufferQueueFree(struct BufferQueue *queue, uint32_t count)
{
    uint32_t i;
    struct CameraBuffer *buffer = NULL;

    if (queue->numBuffers < count) {
        HDF_LOGE("%s: count out of range!", __func__);
        return HDF_ERR_INVALID_PARAM;
    }

    for (i = queue->numBuffers - count; i < queue->numBuffers; ++i) {
        buffer = queue->buffers[i];
        if (buffer == NULL) {
            continue;
        }
        if (buffer->state == BUFFER_STATE_PREPARING) {
            HDF_LOGE("%s: Can free buffer. Buffer is preparing!", __func__);
            return HDF_FAILURE;
        }
        CameraBufferFree(buffer);
        OsalMemFree(buffer);
        queue->buffers[i] = NULL;
    }

    queue->numBuffers -= count;
    if (queue->numBuffers == 0) {
        queue->memType = MEMTYPE_UNKNOWN;
        DListHeadInit(&queue->queuedList);
    }
    return HDF_SUCCESS;
}

int32_t BufferQueueReleaseBuffers(struct BufferQueue *queue, struct UserCameraReq *userRequest)
{
    int32_t ret = 0;

    userRequest->capabilities = queue->ioModes;
    if ((queue->flags & QUEUE_STATE_STREAMING) != 0) {
        HDF_LOGE("%s: Queue is streaming", __func__);
        return HDF_FAILURE;
    }
    if (((queue->flags & QUEUE_STATE_WAITING_DEQUEUE) != 0) && userRequest->count != 0) {
        HDF_LOGE("%s: Queue is waiting dequeue", __func__);
        return HDF_FAILURE;
    }

    if (userRequest->count == 0 || queue->numBuffers != 0 ||
        (queue->memType != MEMTYPE_UNKNOWN && queue->memType != userRequest->memType)) {
        OsalMutexLock(&queue->mmapLock);
        BufferQueueStop(queue);
        ret = BufferQueueFree(queue, queue->numBuffers);
        OsalMutexUnlock(&queue->mmapLock);
    }
    return ret;
}

static int32_t BufferQueueAllocBuffers(struct BufferQueue *queue, uint32_t memType,
    uint32_t numBuffers, uint32_t numPlanes, const uint32_t planeSizes[])
{
    uint32_t bufferId;
    uint32_t planeId;
    struct CameraBuffer *buffer = NULL;
    int32_t ret;

    numBuffers = numBuffers < (MAX_FRAME - queue->numBuffers) ? numBuffers : (MAX_FRAME - queue->numBuffers);
    for (bufferId = 0; bufferId < numBuffers; ++bufferId) {
        buffer = OsalMemCalloc(queue->bufferSize);
        if (buffer == NULL) {
            HDF_LOGE("%s: Memory alloc failed", __func__);
            break;
        }
        buffer->state = BUFFER_STATE_DEQUEUED;
        buffer->bufferQueue = queue;
        buffer->numPlanes = numPlanes;
        buffer->id = queue->numBuffers + bufferId;
        buffer->memType = memType;
        if (queue->memType != MEMTYPE_DMABUF) {
            buffer->flags |= BUFFER_NEED_USER_SYNC;
            buffer->flags |= BUFFER_NEED_DEVICE_SYNC;
        }
        for (planeId = 0; planeId < numPlanes; ++planeId) {
            buffer->planes[planeId].length = planeSizes[planeId];
            buffer->planes[planeId].minLength = planeSizes[planeId];
        }
        queue->buffers[buffer->id] = buffer;
        if (memType == MEMTYPE_MMAP) {
            ret = CameraBufferAllocMmapPlanes(buffer);
            if (ret != HDF_SUCCESS) {
                HDF_LOGE("%s: Mmap alloc failed", __func__);
                queue->buffers[buffer->id] = NULL;
                OsalMemFree(buffer);
                break;
            }
            CameraBufferSetupOffsets(buffer);
        }
    }
    HDF_LOGI("%s: allocated %{public}d buffers and %{public}d planes", __func__, bufferId, planeId);

    return bufferId;
}

int32_t BufferQueueRequestBuffers(struct BufferQueue *queue, struct UserCameraReq *userRequest,
    uint32_t numBuffers, uint32_t numPlanes, uint32_t planeSizes[])
{
    int32_t ret;
    uint32_t allocatedBuffers;

    allocatedBuffers = BufferQueueAllocBuffers(queue, userRequest->memType, numBuffers, numPlanes, planeSizes);
    ret = allocatedBuffers < queue->minBuffersNeeded ? HDF_FAILURE : HDF_SUCCESS;
    if (allocatedBuffers == 0) {
        HDF_LOGE("%s: allocatedBuffers wrong", __func__);
        return HDF_FAILURE;
    }
    if (ret == HDF_SUCCESS && allocatedBuffers < numBuffers) {
        numBuffers = allocatedBuffers;
        numPlanes = 0;
        ret = queue->queueOps->queueSetup != NULL ? queue->queueOps->queueSetup(queue, &numBuffers,
            &numPlanes, planeSizes) : HDF_SUCCESS;
        if (ret == HDF_SUCCESS && allocatedBuffers < numBuffers) {
            ret = HDF_FAILURE;
        }
    }

    OsalMutexLock(&queue->mmapLock);
    queue->numBuffers = allocatedBuffers;
    if (ret != HDF_SUCCESS) {
        BufferQueueFree(queue, queue->numBuffers);
        OsalMutexUnlock(&queue->mmapLock);
        return ret;
    }
    OsalMutexUnlock(&queue->mmapLock);
    userRequest->count = allocatedBuffers;
    queue->flags |= QUEUE_STATE_WAITING_BUFFERS;
    return HDF_SUCCESS;
}

int32_t BufferQueueStart(struct BufferQueue *queue)
{
    struct CameraBuffer *buffer = NULL;
    int32_t ret;

    struct BufferQueueImp *queueImp = container_of(queue, struct BufferQueueImp, queue);
    struct StreamDevice *streamDev = container_of(queueImp, struct StreamDevice, queueImp);

    DLIST_FOR_EACH_ENTRY(buffer, &queue->queuedList, struct CameraBuffer, queueEntry) {
        CameraBufferEnqueue(buffer);
    }
    queue->flags |= QUEUE_STATE_STREAMING_CALLED;
    if (streamDev->streamOps->startStreaming == NULL) {
        return HDF_FAILURE;
    }

    ret = streamDev->streamOps->startStreaming(streamDev);
    if (ret == HDF_SUCCESS) {
        return ret;
    }
    queue->flags &= ~QUEUE_STATE_STREAMING_CALLED;

    if (OsalAtomicRead(&queue->driverOwnCount) != 0) {
        for (uint32_t i = 0; i < queue->numBuffers; ++i) {
            buffer = queue->buffers[i];
            if (buffer->state == BUFFER_STATE_ACTIVE) {
                CameraBufferDone(buffer, BUFFER_STATE_QUEUED);
            }
        }
        HDF_LOGW("%s: driver count must be zero: %{public}d", __func__, OsalAtomicRead(&queue->driverOwnCount));
    }
    if (!DListIsEmpty(&queue->doneList)) {
        HDF_LOGW("%s: doneList is not empty", __func__);
    }
    return ret;
}

int32_t BufferQueuePrepare(struct BufferQueue *queue, struct UserCameraBuffer *userBuffer)
{
    struct CameraBuffer *buffer = NULL;
    int32_t ret;

    if (userBuffer->id >= queue->numBuffers) {
        HDF_LOGE("%s: buffer index out of range", __func__);
        return HDF_ERR_INVALID_PARAM;
    }
    if (queue->buffers[userBuffer->id] == NULL) {
        HDF_LOGE("%s: buffer is NULL", __func__);
        return HDF_FAILURE;
    }
    if (userBuffer->memType != queue->memType) {
        HDF_LOGE("%s: invalid memory type: userBuffer memType = %{public}u, queue memType = %{public}u",
            __func__, userBuffer->memType, queue->memType);
        return HDF_FAILURE;
    }

    buffer = queue->buffers[userBuffer->id];
    ret = CameraBufferCheckPlanes(buffer, userBuffer);
    if (ret != HDF_SUCCESS) {
        return ret;
    }
    if (buffer->state != BUFFER_STATE_DEQUEUED) {
        HDF_LOGE("%s: buffer is not in dequeued state", __func__);
        return HDF_FAILURE;
    }

    if ((buffer->flags & BUFFER_PREPARED) == 0) {
        CameraBufferSetCacheSync(queue, buffer);
        ret = CameraBufferCheckPlaneLength(buffer, userBuffer);
        if (ret != HDF_SUCCESS) {
            return ret;
        }
    }
    return ret;
}
