/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#ifndef CAMERA_BUFFER_MANAGER_ADAPTER_H
#define CAMERA_BUFFER_MANAGER_ADAPTER_H

#include <hdf_dlist.h>
#include <osal_atomic.h>
#include <osal_mutex.h>
#include <osal_spinlock.h>
#include <linux/wait.h>
#include <linux/dma-buf.h>
#include "buffer_queue.h"

/* struct BufferQueue is the first variable in struct BufferQueueImp */
struct BufferQueueImp {
    struct BufferQueue queue;
    struct device *dev;
    struct device *allocDev[MAX_PLANES];
    gfp_t gfpFlags;
    enum dma_data_direction dmaDir;
    struct mutex *lock;
    unsigned long dmaAttrs;
};

struct VmareaHandler {
    refcount_t *refCount;
    void (*free)(void *arg);
    void *arg;
};

/* interface used in the memory operations */
struct frame_vector *CreateFrameVec(unsigned long start, unsigned long length);
void DestroyFrameVec(struct frame_vector *vec);
const struct vm_operations_struct *GetVmOps(void);

/* interface for kernel function */
void MemoryAdapterQueueImpInit(struct BufferQueue *queue);
void MemoryAdapterDriverMutexLock(struct BufferQueue *queue);
void MemoryAdapterDriverMutexUnLock(struct BufferQueue *queue);
bool MemoryAdapterIsErrOrNullPtr(void *ptr);
bool MemoryAdapterIsErrPtr(void *ptr);
int32_t MemoryAdapterPtrErr(void *ptr);
unsigned long MemoryAdapterPageAlign(unsigned long size);
void *MemoryAdapterGetDmaBuffer(uint32_t fd);
void MemoryAdapterPutDmaBuffer(void *dmaBuf);
uint32_t MemoryAdapterDmaBufSize(void *dmaBuf);

#endif /* CAMERA_BUFFER_MANAGER_ADAPTER_H */
