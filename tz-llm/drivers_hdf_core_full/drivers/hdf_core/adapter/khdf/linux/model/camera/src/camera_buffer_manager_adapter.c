/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include "camera_buffer_manager_adapter.h"

struct frame_vector *CreateFrameVec(unsigned long start, unsigned long length)
{
    int32_t ret;
    unsigned long first;
    unsigned long last;
    unsigned long num;
    struct frame_vector *vec = NULL;
    uint32_t flags = FOLL_FORCE | FOLL_WRITE;

    first = start >> PAGE_SHIFT;
    if (start == 0 || length == 0) {
        return ERR_PTR(-EINVAL);
    }
    last = (start + length - 1) >> PAGE_SHIFT;
    num = last - first + 1;
    vec = frame_vector_create(num);
    if (vec == NULL) {
        return ERR_PTR(-ENOMEM);
    }

    ret = get_vaddr_frames(start & PAGE_MASK, num, flags, vec);
    if (ret < 0) {
        goto OUT_DESTROY;
    }

    /* We accept only complete set of PFNs */
    if (ret != num) {
        ret = -EFAULT;
        goto OUT_RELEASE;
    }
    return vec;
OUT_RELEASE:
    put_vaddr_frames(vec);
OUT_DESTROY:
    frame_vector_destroy(vec);
    return ERR_PTR(ret);
}

void DestroyFrameVec(struct frame_vector *vec)
{
    put_vaddr_frames(vec);
    frame_vector_destroy(vec);
}

static void CommonVmOpen(struct vm_area_struct *vma)
{
    if (vma == NULL) {
        return;
    }
    struct VmareaHandler *handler = vma->vm_private_data;

    refcount_inc(handler->refCount);
}

static void CommonVmClose(struct vm_area_struct *vma)
{
    if (vma == NULL) {
        return;
    }
    struct VmareaHandler *handler = vma->vm_private_data;

    handler->free(handler->arg);
}

const struct vm_operations_struct g_commonVmOps = {
    .open = CommonVmOpen,
    .close = CommonVmClose,
};

const struct vm_operations_struct *GetVmOps(void)
{
    return &g_commonVmOps;
}

void MemoryAdapterQueueImpInit(struct BufferQueue *queue)
{
    if (queue == NULL) {
        return;
    }
    struct BufferQueueImp *queueImp = CONTAINER_OF(queue, struct BufferQueueImp, queue);
    
    queueImp->dmaDir = DMA_FROM_DEVICE;
}

void MemoryAdapterDriverMutexLock(struct BufferQueue *queue)
{
    if (queue == NULL) {
        return;
    }
    struct BufferQueueImp *queueImp = CONTAINER_OF(queue, struct BufferQueueImp, queue);

    mutex_lock(queueImp->lock);
}

void MemoryAdapterDriverMutexUnLock(struct BufferQueue *queue)
{
    if (queue == NULL) {
        return;
    }
    struct BufferQueueImp *queueImp = CONTAINER_OF(queue, struct BufferQueueImp, queue);

    mutex_unlock(queueImp->lock);
}

bool MemoryAdapterIsErrOrNullPtr(void *ptr)
{
    return IS_ERR_OR_NULL(ptr);
}

bool MemoryAdapterIsErrPtr(void *ptr)
{
    return IS_ERR(ptr);
}

int32_t MemoryAdapterPtrErr(void *ptr)
{
    return PTR_ERR(ptr);
}

unsigned long MemoryAdapterPageAlign(unsigned long size)
{
    return PAGE_ALIGN(size);
}

void *MemoryAdapterGetDmaBuffer(uint32_t fd)
{
    return (void *)dma_buf_get(fd);
}

void MemoryAdapterPutDmaBuffer(void *dmaBuf)
{
    dma_buf_put((struct dma_buf *)dmaBuf);
}

uint32_t MemoryAdapterDmaBufSize(void *dmaBuf)
{
    if (dmaBuf == NULL) {
        return 0;
    }
    return ((struct dma_buf *)dmaBuf)->size;
}