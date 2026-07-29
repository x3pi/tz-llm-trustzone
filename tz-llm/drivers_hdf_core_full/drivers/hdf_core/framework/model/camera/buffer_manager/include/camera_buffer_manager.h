/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#ifndef CAMERA_BUFFER_MANAGER_H
#define CAMERA_BUFFER_MANAGER_H

#include <hdf_dlist.h>
#include <osal_atomic.h>
#include <osal_mutex.h>
#include <osal_spinlock.h>
#include <camera/camera_product.h>
#include "camera_buffer_manager_adapter.h"
#include "camera_buffer.h"

/* extern used function */
int32_t BufferQueueFindPlaneByOffset(struct BufferQueue *queue,
    unsigned long off, uint32_t *bufferId, uint32_t *planeId);
void CameraBufferDone(struct CameraBuffer *buffer, enum BufferState state);
void *CameraBufferGetPlaneVaddr(struct CameraBuffer *buffer, uint32_t planeId);

/* init queue */
int32_t BufferQueueInit(struct BufferQueue *queue);
/* request buffers */
int32_t BufferQueueRequest(struct BufferQueue *queue, struct UserCameraReq *userRequest);
/* query buffer */
int32_t BufferQueueQueryBuffer(struct BufferQueue *queue, struct UserCameraBuffer *userBuffer);
/* return buffer */
int32_t BufferQueueReturnBuffer(struct BufferQueue *queue, struct UserCameraBuffer *userBuffer);
/* acquire buffer */
int32_t BufferQueueAcquireBuffer(struct BufferQueue *queue, struct UserCameraBuffer *userBuffer);
/* stream on */
int32_t BufferQueueStreamOn(struct BufferQueue *queue);
/* stream off */
int32_t BufferQueueStreamOff(struct BufferQueue *queue);
/* set curent queue for mmap */
void BufferQueueSetQueueForMmap(struct BufferQueue *queue);
/* get curent queue for mmap */
struct BufferQueue *BufferQueueGetQueueForMmap(void);

#endif  // CAMERA_BUFFER_MANAGER_H