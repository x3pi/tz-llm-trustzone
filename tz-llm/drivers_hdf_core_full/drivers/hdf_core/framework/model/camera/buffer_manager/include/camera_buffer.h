/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#ifndef CAMERA_BUFFER_H
#define CAMERA_BUFFER_H

#include <hdf_dlist.h>
#include <osal_atomic.h>
#include <osal_mutex.h>
#include <osal_spinlock.h>
#include <camera/camera_product.h>

#define MAX_PLANES  8   /* max plane count per buffer */

enum BufferState {
    BUFFER_STATE_DEQUEUED,
    BUFFER_STATE_PREPARING,
    BUFFER_STATE_QUEUED,
    BUFFER_STATE_ACTIVE,
    BUFFER_STATE_DONE,
    BUFFER_STATE_ERROR,
};

struct BufferPlane {
    void *memPriv;
    void *dmaBuf;
    uint32_t dmaMapped;
    uint32_t bytesUsed;
    uint32_t length;
    uint32_t minLength;
    union {
        uint32_t offset;
        unsigned long userPtr;
        uint32_t fd;
    } memory;
    uint32_t dataOffset;
};

struct CameraBuffer {
    struct BufferQueue *bufferQueue;
    uint32_t id;
    uint32_t memType;
    uint64_t timeStamp;
    enum BufferState state;
    uint32_t flags;
    uint32_t numPlanes;
    struct BufferPlane planes[MAX_PLANES];
    struct DListHead queueEntry;
    struct DListHead doneEntry;
    uint32_t field;
    uint32_t sequence;
};

/* CameraBuffer flags */
enum BufferFlags {
    BUFFER_DEVICE_SYNCED = (1 << 0),
    BUFFER_NEED_USER_SYNC = (1 << 1),
    BUFFER_NEED_DEVICE_SYNC = (1 << 2),
    BUFFER_PREPARED = (1 << 3),
};

void CameraBufferSyncForUser(struct CameraBuffer *buffer);
void CameraBufferFree(struct CameraBuffer *buffer);
int32_t CameraBufferAllocMmapPlanes(struct CameraBuffer *buffer);
void CameraBufferSetupOffsets(struct CameraBuffer *buffer);
int32_t CameraBufferCheckPlanes(struct CameraBuffer *buffer, const struct UserCameraBuffer *userBuffer);
void CameraBufferToUserBuffer(struct CameraBuffer *buffer, struct UserCameraBuffer *userBuffer);
void CameraBufferEnqueue(struct CameraBuffer *buffer);
void CameraBufferQueueBuffer(struct CameraBuffer *buffer);
void CameraBufferSetCacheSync(struct BufferQueue *queue, struct CameraBuffer *buffer);
int32_t CameraBufferCheckPlaneLength(struct CameraBuffer *buffer, const struct UserCameraBuffer *userBuffer);
int32_t CameraBufferFromUserBuffer(struct CameraBuffer *buffer, const struct UserCameraBuffer *userBuffer);

#endif  // CAMERA_BUFFER_H