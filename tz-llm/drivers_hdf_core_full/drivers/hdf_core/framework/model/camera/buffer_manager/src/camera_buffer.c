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
#include "camera_buffer_manager_adapter.h"
#include "camera_buffer.h"

#define HDF_LOG_TAG HDF_CAMERA_BUFFER

void CameraBufferSyncForUser(struct CameraBuffer *buffer)
{
    struct BufferQueue *queue = buffer->bufferQueue;

    if ((buffer->flags & BUFFER_DEVICE_SYNCED) == 0) {
        return;
    }

    if ((buffer->flags & BUFFER_NEED_USER_SYNC) != 0) {
        for (uint32_t plane = 0; plane < buffer->numPlanes; ++plane) {
            if (queue->memOps->syncForUser != NULL) {
                queue->memOps->syncForUser(buffer->planes[plane].memPriv);
            }
        }
    }
    buffer->flags &= ~BUFFER_DEVICE_SYNCED;
}

static void CameraBufferFreeMmapPlanes(struct CameraBuffer *buffer)
{
    uint32_t plane;
    struct BufferQueue *queue = buffer->bufferQueue;

    for (plane = 0; plane < buffer->numPlanes; ++plane) {
        if ((queue->memOps->mmapFree != NULL) && (buffer->planes[plane].memPriv != NULL)) {
            queue->memOps->mmapFree(buffer->planes[plane].memPriv);
            buffer->planes[plane].memPriv = NULL;
        }
    }
}

static void CameraBufferFreeUserPtrPlanes(struct CameraBuffer *buffer)
{
    uint32_t plane;
    struct BufferQueue *queue = buffer->bufferQueue;

    for (plane = 0; plane < buffer->numPlanes; ++plane) {
        if ((queue->memOps->freeUserPtr != NULL) && (buffer->planes[plane].memPriv != NULL)) {
            queue->memOps->freeUserPtr(buffer->planes[plane].memPriv);
            buffer->planes[plane].memPriv = NULL;
            buffer->planes[plane].memory.userPtr = 0;
            buffer->planes[plane].length = 0;
        }
    }
}

static void CameraBufferFreeDmaPlane(struct CameraBuffer *buffer, struct BufferPlane *plane)
{
    struct BufferQueue *queue = buffer->bufferQueue;

    if (plane->dmaMapped != 0) {
        if (queue->memOps->unmapDmaBuf != NULL) {
            queue->memOps->unmapDmaBuf(plane->memPriv);
        }
    }
    if (queue->memOps->detachDmaBuf != NULL) {
        queue->memOps->detachDmaBuf(plane->memPriv);
    }

    MemoryAdapterPutDmaBuffer(plane->dmaBuf);
    plane->memPriv = NULL;
    plane->dmaBuf = NULL;
    plane->dmaMapped = 0;
}

static void CameraBufferFreeDmaPlanes(struct CameraBuffer *buffer)
{
    uint32_t planeId;

    for (planeId = 0; planeId < buffer->numPlanes; ++planeId) {
        CameraBufferFreeDmaPlane(buffer, &buffer->planes[planeId]);
    }
}

void CameraBufferFree(struct CameraBuffer *buffer)
{
    struct BufferQueue *queue = buffer->bufferQueue;
    if (queue->memType == MEMTYPE_MMAP) {
        CameraBufferFreeMmapPlanes(buffer);
    } else if (queue->memType == MEMTYPE_DMABUF) {
        CameraBufferFreeDmaPlanes(buffer);
    } else {
        CameraBufferFreeUserPtrPlanes(buffer);
    }
}

static int32_t CameraBufferAllocMmapPlane(struct CameraBuffer *buffer, int32_t planeNum)
{
    struct BufferQueue *queue = buffer->bufferQueue;
    void *memPriv = NULL;
    int32_t ret = HDF_FAILURE;
    unsigned long size = MemoryAdapterPageAlign(buffer->planes[planeNum].length);
    if (size < buffer->planes[planeNum].length) {
        return HDF_FAILURE;
    }

    if (queue->memOps->mmapAlloc != NULL) {
        memPriv = queue->memOps->mmapAlloc(queue, planeNum, size);
    } else {
        memPriv = NULL;
    }

    if (MemoryAdapterIsErrOrNullPtr(memPriv)) {
        if (memPriv != NULL) {
            ret = MemoryAdapterPtrErr(memPriv);
        }
        return ret;
    }
    buffer->planes[planeNum].memPriv = memPriv;
    return HDF_SUCCESS;
}

int32_t CameraBufferAllocMmapPlanes(struct CameraBuffer *buffer)
{
    struct BufferQueue *queue = buffer->bufferQueue;
    int32_t planeNum;
    int32_t ret;

    for (planeNum = 0; planeNum < buffer->numPlanes; ++planeNum) {
        ret = CameraBufferAllocMmapPlane(buffer, planeNum);
        if (ret != HDF_SUCCESS) {
            goto FREE;
        }
    }
    return HDF_SUCCESS;

FREE:
    for (; planeNum > 0; --planeNum) {
        if ((queue->memOps->mmapFree != NULL) && (buffer->planes[planeNum - 1].memPriv != NULL)) {
            queue->memOps->mmapFree(buffer->planes[planeNum - 1].memPriv);
            buffer->planes[planeNum - 1].memPriv = NULL;
        }
    }
    return ret;
}

void CameraBufferSetupOffsets(struct CameraBuffer *buffer)
{
    struct BufferQueue *queue = buffer->bufferQueue;
    uint32_t planeId;
    unsigned long off;

    if (buffer->id != 0) {
        struct CameraBuffer *prev = queue->buffers[buffer->id - 1];
        struct BufferPlane *p = &prev->planes[prev->numPlanes - 1];
        off = MemoryAdapterPageAlign(p->memory.offset + p->length);
    } else {
        off = MemoryAdapterPageAlign(buffer->planes[0].memory.offset);
    }

    for (planeId = 0; planeId < buffer->numPlanes; ++planeId) {
        buffer->planes[planeId].memory.offset = off;
        off += buffer->planes[planeId].length;
        off = MemoryAdapterPageAlign(off);
    }
}

int32_t CameraBufferCheckPlanes(struct CameraBuffer *buffer, const struct UserCameraBuffer *userBuffer)
{
    if (userBuffer->planes == NULL) {
        HDF_LOGE("%s: user buffer's plane is null!", __func__);
        return HDF_FAILURE;
    }
    if (userBuffer->planeCount < buffer->numPlanes || userBuffer->planeCount > MAX_PLANES) {
        HDF_LOGE("%s: incorrect plane count!", __func__);
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

void CameraBufferToUserBuffer(struct CameraBuffer *buffer, struct UserCameraBuffer *userBuffer)
{
    uint32_t plane;

    userBuffer->id = buffer->id;
    userBuffer->memType = buffer->memType;
    userBuffer->field = buffer->field;
    userBuffer->timeStamp = buffer->timeStamp;
    userBuffer->sequence = buffer->sequence;
    userBuffer->planeCount = buffer->numPlanes;

    for (plane = 0; plane < buffer->numPlanes; ++plane) {
        struct UserCameraPlane *dst = &userBuffer->planes[plane];
        struct BufferPlane *src = &buffer->planes[plane];
        dst->bytesUsed = src->bytesUsed;
        dst->length = src->length;
        if (buffer->memType == MEMTYPE_MMAP) {
            dst->memory.offset = src->memory.offset;
        } else if (buffer->memType == MEMTYPE_USERPTR) {
            dst->memory.userPtr = src->memory.userPtr;
        } else if (buffer->memType == MEMTYPE_DMABUF) {
            dst->memory.fd = src->memory.fd;
        }
        dst->dataOffset = src->dataOffset;
    }

    switch (buffer->state) {
        case BUFFER_STATE_QUEUED:
        case BUFFER_STATE_ACTIVE:
            userBuffer->flags |= USER_BUFFER_QUEUED;
            break;
        case BUFFER_STATE_ERROR:
            userBuffer->flags |= USER_BUFFER_ERROR;
            break;
        case BUFFER_STATE_DONE:
            userBuffer->flags |= USER_BUFFER_DONE;
            break;
        default:
            break;
    }
}

void CameraBufferEnqueue(struct CameraBuffer *buffer)
{
    struct BufferQueue *queue = buffer->bufferQueue;

    buffer->state = BUFFER_STATE_ACTIVE;
    OsalAtomicInc(&queue->driverOwnCount);
    if (queue->queueOps->queueBuffer != NULL) {
        queue->queueOps->queueBuffer(queue, buffer);
    }
    return;
}

void CameraBufferQueueBuffer(struct CameraBuffer *buffer)
{
    struct BufferQueue *queue = buffer->bufferQueue;

    DListInsertTail(&buffer->queueEntry, &queue->queuedList);
    queue->queuedCount++;
    queue->flags &= ~QUEUE_STATE_WAITING_BUFFERS;
    buffer->state = BUFFER_STATE_QUEUED;
    if ((queue->flags & QUEUE_STATE_STREAMING_CALLED) != 0) {
        CameraBufferEnqueue(buffer);
    }
    return;
}

void CameraBufferSetCacheSync(struct BufferQueue *queue, struct CameraBuffer *buffer)
{
    if (queue->memType == MEMTYPE_DMABUF) {
        buffer->flags &= ~BUFFER_NEED_DEVICE_SYNC;
        buffer->flags &= ~BUFFER_NEED_USER_SYNC;
        return;
    }
    buffer->flags |= BUFFER_NEED_DEVICE_SYNC;
    buffer->flags |= BUFFER_NEED_USER_SYNC;
    return;
}

int32_t CameraBufferCheckPlaneLength(struct CameraBuffer *buffer, const struct UserCameraBuffer *userBuffer)
{
    uint32_t length;
    uint32_t planeId;

    for (planeId = 0; planeId < buffer->numPlanes; ++planeId) {
        if (buffer->memType == MEMTYPE_USERPTR || buffer->memType == MEMTYPE_DMABUF) {
            length = userBuffer->planes[planeId].length;
        } else {
            length = buffer->planes[planeId].length;
        }
        uint32_t bytesUsed = userBuffer->planes[planeId].bytesUsed != 0 ?
            userBuffer->planes[planeId].bytesUsed : length;
        if (bytesUsed > length) {
            return HDF_ERR_INVALID_PARAM;
        }
        if (userBuffer->planes[planeId].dataOffset > 0 && userBuffer->planes[planeId].dataOffset >= bytesUsed) {
            return HDF_ERR_INVALID_PARAM;
        }
    }
    return HDF_SUCCESS;
}

static int32_t CameraBufferPrepareMmap(struct CameraBuffer *buffer, struct BufferPlane planes[])
{
    uint32_t planeNum;

    for (planeNum = 0; planeNum < buffer->numPlanes; ++planeNum) {
        buffer->planes[planeNum].bytesUsed = planes[planeNum].bytesUsed;
        buffer->planes[planeNum].dataOffset = planes[planeNum].dataOffset;
    }
    return HDF_SUCCESS;
}

static int32_t CameraBufferPrepareUserPtrPlane(struct CameraBuffer *buffer,
    uint32_t planeNum, unsigned long userPtr, uint32_t length)
{
    void *memPriv = NULL;
    struct BufferQueue *queue = buffer->bufferQueue;
    if (buffer->planes[planeNum].memPriv != NULL) {
        if (queue->memOps->freeUserPtr != NULL) {
            queue->memOps->freeUserPtr(buffer->planes[planeNum].memPriv);
        } else {
            HDF_LOGW("%s: no freeUserPtr function!", __func__);
        }
    }

    buffer->planes[planeNum].memPriv = NULL;
    buffer->planes[planeNum].bytesUsed = 0;
    buffer->planes[planeNum].length = 0;
    buffer->planes[planeNum].memory.userPtr = 0;
    buffer->planes[planeNum].dataOffset = 0;
    if (queue->memOps->allocUserPtr != NULL) {
        memPriv = queue->memOps->allocUserPtr(queue, planeNum, userPtr, length);
    } else {
        memPriv = NULL;
    }

    if (MemoryAdapterIsErrPtr(memPriv)) {
        int32_t ret = MemoryAdapterPtrErr(memPriv);
        return ret;
    }
    buffer->planes[planeNum].memPriv = memPriv;
    return HDF_SUCCESS;
}

static int32_t CameraBufferPrepareUserPtr(struct CameraBuffer *buffer, struct BufferPlane planes[])
{
    uint32_t planeNum;
    int32_t ret;

    for (planeNum = 0; planeNum < buffer->numPlanes; ++planeNum) {
        if ((buffer->planes[planeNum].memory.userPtr != 0) &&
            (buffer->planes[planeNum].memory.userPtr == planes[planeNum].memory.userPtr) &&
            (buffer->planes[planeNum].length == planes[planeNum].length)) {
            continue;
        }
        if (planes[planeNum].length < buffer->planes[planeNum].minLength) {
            goto FREE;
        }
        ret = CameraBufferPrepareUserPtrPlane(buffer, planeNum,
            planes[planeNum].memory.userPtr, planes[planeNum].length);
        if (ret != HDF_SUCCESS) {
            goto FREE;
        }
    }
    return HDF_SUCCESS;

FREE:
    CameraBufferFreeUserPtrPlanes(buffer);
    HDF_LOGE("%s: user ptr mem prepare failed", __func__);
    return HDF_FAILURE;
}

static int32_t CameraBufferAttachDmaPlane(struct CameraBuffer *buffer, uint32_t planeNum, struct BufferPlane planes[])
{
    void *memPriv = NULL;
    struct BufferQueue *queue = buffer->bufferQueue;
    void *dmaBuf = MemoryAdapterGetDmaBuffer(planes[planeNum].memory.fd);
    if (MemoryAdapterIsErrOrNullPtr(dmaBuf)) {
        HDF_LOGE("%s: dmaBuf ptr error!", __func__);
        return HDF_FAILURE;
    }
    if (planes[planeNum].length == 0) {
        planes[planeNum].length = MemoryAdapterDmaBufSize(dmaBuf);
    }
    if (planes[planeNum].length < buffer->planes[planeNum].minLength) {
        MemoryAdapterPutDmaBuffer(dmaBuf);
        HDF_LOGE("%s: not enough dmabuf length!", __func__);
        return HDF_FAILURE;
    }
    if (dmaBuf == buffer->planes[planeNum].dmaBuf && buffer->planes[planeNum].length == planes[planeNum].length) {
        MemoryAdapterPutDmaBuffer(dmaBuf);
        return HDF_SUCCESS;
    }
    CameraBufferFreeDmaPlane(buffer, &buffer->planes[planeNum]);
    buffer->planes[planeNum].bytesUsed = 0;
    buffer->planes[planeNum].length = 0;
    buffer->planes[planeNum].memory.fd = 0;
    buffer->planes[planeNum].dataOffset = 0;

    if (queue->memOps->attachDmaBuf != NULL) {
        memPriv = queue->memOps->attachDmaBuf(queue, planeNum, dmaBuf, planes[planeNum].length);
    }

    if (MemoryAdapterIsErrPtr(memPriv)) {
        int32_t ret = MemoryAdapterPtrErr(memPriv);
        MemoryAdapterPutDmaBuffer(dmaBuf);
        return ret;
    }
    buffer->planes[planeNum].dmaBuf = dmaBuf;
    buffer->planes[planeNum].memPriv = memPriv;
    return HDF_SUCCESS;
}

static int32_t CameraBufferAttachDmaBuffer(struct CameraBuffer *buffer, struct BufferPlane planes[])
{
    uint32_t planeNum;

    for (planeNum = 0; planeNum < buffer->numPlanes; ++planeNum) {
        int32_t ret = CameraBufferAttachDmaPlane(buffer, planeNum, planes);
        if (ret != HDF_SUCCESS) {
            return ret;
        }
    }
    return HDF_SUCCESS;
}

static int32_t CameraBufferMapDmaBuffer(struct CameraBuffer *buffer, struct BufferPlane planes[])
{
    int32_t planeNum;
    int32_t ret;
    struct BufferQueue *queue = buffer->bufferQueue;

    for (planeNum = 0; planeNum < buffer->numPlanes; ++planeNum) {
        if (buffer->planes[planeNum].dmaMapped != 0) {
            continue;
        }
        if (queue->memOps->mapDmaBuf == NULL) {
            HDF_LOGE("%s: mapDmaBuf function is NULL!", __func__);
            return HDF_FAILURE;
        }
        ret = queue->memOps->mapDmaBuf(buffer->planes[planeNum].memPriv);
        if (ret != HDF_SUCCESS) {
            HDF_LOGE("%s: mapDmaBuf failed!", __func__);
            return ret;
        }
        buffer->planes[planeNum].dmaMapped = 1;

        buffer->planes[planeNum].bytesUsed = planes[planeNum].bytesUsed;
        buffer->planes[planeNum].length = planes[planeNum].length;
        buffer->planes[planeNum].memory.fd = planes[planeNum].memory.fd;
        buffer->planes[planeNum].dataOffset = planes[planeNum].dataOffset;
    }

    return HDF_SUCCESS;
}

static int32_t CameraBufferPrepareDma(struct CameraBuffer *buffer, struct BufferPlane planes[])
{
    int32_t ret;

    ret = CameraBufferAttachDmaBuffer(buffer, planes);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: dma buffer attach failed", __func__);
        goto FREE;
    }
    ret = CameraBufferMapDmaBuffer(buffer, planes);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: dma buffer map failed", __func__);
        goto FREE;
    }
    return HDF_SUCCESS;

FREE:
    CameraBufferFreeDmaPlanes(buffer);
    return ret;
}

static int32_t CameraBufferPrepare(struct BufferQueue *queue, struct CameraBuffer *buffer, struct BufferPlane planes[])
{
    int32_t ret;

    switch (queue->memType) {
        case MEMTYPE_MMAP:
            ret = CameraBufferPrepareMmap(buffer, planes);
            break;
        case MEMTYPE_USERPTR:
            ret = CameraBufferPrepareUserPtr(buffer, planes);
            break;
        case MEMTYPE_DMABUF:
            ret = CameraBufferPrepareDma(buffer, planes);
            break;
        default:
            HDF_LOGE("%s: invalid queue type, type=%{public}u", __func__, queue->memType);
            ret = HDF_FAILURE;
            break;
    }
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: buffer prepare is wrong", __func__);
        return ret;
    }
    return HDF_SUCCESS;
}

static int32_t CameraBufferGetPlanesFromUserBuffer(struct CameraBuffer *buffer,
    struct BufferPlane *planes, const struct UserCameraBuffer *userBuffer)
{
    uint32_t planeId;

    for (planeId = 0; planeId < buffer->numPlanes; ++planeId) {
        switch (buffer->memType) {
            case MEMTYPE_USERPTR:
                planes[planeId].memory.userPtr = userBuffer->planes[planeId].memory.userPtr;
                planes[planeId].length = userBuffer->planes[planeId].length;
                break;
            case MEMTYPE_DMABUF:
                planes[planeId].memory.fd = userBuffer->planes[planeId].memory.fd;
                planes[planeId].length = userBuffer->planes[planeId].length;
                break;
            default:
                break;
        }
        planes[planeId].bytesUsed = userBuffer->planes[planeId].bytesUsed;
        planes[planeId].dataOffset = userBuffer->planes[planeId].dataOffset;
    }

    return HDF_SUCCESS;
}

static void CameraBufferSyncForDevice(struct CameraBuffer *buffer)
{
    struct BufferQueue *queue = buffer->bufferQueue;
    if ((buffer->flags & BUFFER_DEVICE_SYNCED) != 0) {
        return;
    }

    if ((buffer->flags & BUFFER_NEED_DEVICE_SYNC) != 0) {
        for (uint32_t plane = 0; plane < buffer->numPlanes; ++plane) {
            if (queue->memOps->syncForDevice != NULL) {
                queue->memOps->syncForDevice(buffer->planes[plane].memPriv);
            }
        }
    }
    buffer->flags |= BUFFER_DEVICE_SYNCED;
}

int32_t CameraBufferFromUserBuffer(struct CameraBuffer *buffer, const struct UserCameraBuffer *userBuffer)
{
    struct BufferPlane planes[MAX_PLANES];
    struct BufferQueue *queue = buffer->bufferQueue;
    enum BufferState origState = buffer->state;
    int32_t ret;

    if ((queue->flags & QUEUE_STATE_ERROR) != 0) {
        return HDF_FAILURE;
    }
    if ((buffer->flags & BUFFER_PREPARED) != 0) {
        return HDF_SUCCESS;
    }

    buffer->state = BUFFER_STATE_PREPARING;
    (void)memset_s(planes, sizeof(planes), 0, sizeof(planes));
    ret = CameraBufferGetPlanesFromUserBuffer(buffer, planes, userBuffer);
    if (ret != HDF_SUCCESS) {
        return ret;
    }
    ret = CameraBufferPrepare(queue, buffer, planes);
    if (ret != HDF_SUCCESS) {
        return ret;
    }
    CameraBufferSyncForDevice(buffer);
    buffer->flags |= BUFFER_PREPARED;
    buffer->state = origState;
    return HDF_SUCCESS;
}
