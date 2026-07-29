/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#ifndef CAMERA_QUEUE_H
#define CAMERA_QUEUE_H

#include <hdf_dlist.h>
#include <osal_atomic.h>
#include <osal_mutex.h>
#include <osal_spinlock.h>
#include <camera/camera_product.h>
#include "camera_buffer.h"

#define MAX_FRAME   32  /* max buffer count per queue */

struct BufferQueue {
    uint32_t ioModes;
    uint32_t flags;
    struct BufferQueueOps *queueOps;
    uint32_t bufferSize;
    uint32_t minBuffersNeeded;
    struct OsalMutex mmapLock;
    uint32_t memType;
    struct CameraBuffer *buffers[MAX_FRAME];
    uint32_t numBuffers;
    struct DListHead queuedList;
    uint32_t queuedCount;
    OsalAtomic driverOwnCount;
    struct DListHead doneList;
    OsalSpinlock doneLock;
    wait_queue_head_t doneWait;
    uint32_t queueIsInit;
    struct MemOps *memOps;
};

struct MemOps {
    void *(*mmapAlloc)(struct BufferQueue *queue, uint32_t planeNum, unsigned long size);
    void (*mmapFree)(void *bufPriv);
    void *(*getDmaBuf)(void *bufPriv, uint32_t flags);
    void *(*allocUserPtr)(struct BufferQueue *queue, uint32_t planeNum, unsigned long vaddr, unsigned long size);
    void (*freeUserPtr)(void *bufPriv);
    void (*syncForDevice)(void *bufPriv);
    void (*syncForUser)(void *bufPriv);
    void *(*attachDmaBuf)(struct BufferQueue *queue, uint32_t planeNum, void *dmaBuf, unsigned long size);
    void (*detachDmaBuf)(void *bufPriv);
    int32_t (*mapDmaBuf)(void *bufPriv);
    void (*unmapDmaBuf)(void *bufPriv);
    void *(*getVaddr)(void *bufPriv);
    void *(*getCookie)(void *bufPriv);
    uint32_t (*numUsers)(void *bufPriv);
    int32_t (*mmap)(void *bufPriv, void *vm);
};

struct BufferQueueOps {
    int32_t (*queueSetup)(struct BufferQueue *queue, uint32_t *bufferCount, uint32_t *planeCount, uint32_t sizes[]);
    void (*queueBuffer)(struct BufferQueue *queue, struct CameraBuffer *buffer);
};

/* BufferQueue flags */
enum QueueState {
    QUEUE_STATE_STREAMING = (1 << 0),               /**< set bit: queue is streaming */
    QUEUE_STATE_WAITING_DEQUEUE = (1 << 1),         /**< set bit: queue is waiting buffer dequeue */
    QUEUE_STATE_STREAMING_CALLED = (1 << 2),        /**< set bit: start streaming is called */
    QUEUE_STATE_ERROR = (1 << 3),                   /**< set bit: error happened */
    QUEUE_STATE_WAITING_BUFFERS = (1 << 4),         /**< set bit: queue is waiting for buffers */
    QUEUE_STATE_LAST_BUFFER_DEQUEUED = (1 << 5),    /**< set bit: last buffer has been dequeued */
    QUEUE_STATE_ALLOW_CACHE_HINTS = (1 << 6),       /**< set bit:  queue allow cache hints*/
    QUEUE_STATE_BIDIRECTIONAL = (1 << 7),           /**< set bit:  queue is bidirectional*/
};

void BufferQueueStop(struct BufferQueue *queue);
int32_t BufferQueueCheckMemOps(struct BufferQueue *queue, enum CameraMemType memType);
int32_t BufferQueueReleaseBuffers(struct BufferQueue *queue, struct UserCameraReq *userRequest);
int32_t BufferQueueRequestBuffers(struct BufferQueue *queue, struct UserCameraReq *userRequest,
    uint32_t numBuffers, uint32_t numPlanes, uint32_t planeSizes[]);
int32_t BufferQueueStart(struct BufferQueue *queue);
int32_t BufferQueuePrepare(struct BufferQueue *queue, struct UserCameraBuffer *userBuffer);


#endif  // CAMERA_QUEUE_H