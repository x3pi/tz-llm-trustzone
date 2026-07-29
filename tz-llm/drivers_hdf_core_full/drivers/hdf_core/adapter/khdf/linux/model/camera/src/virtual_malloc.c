/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include <hdf_log.h>
#include <osal_mem.h>
#include "camera_buffer_manager_adapter.h"
#include "virtual_malloc.h"

struct VmallocBuffer {
    void *vaddr;
    struct frame_vector *vec;
    enum dma_data_direction dmaDir;
    unsigned long size;
    refcount_t refCount;
    struct VmareaHandler handler;
    struct dma_buf *dbuf;
};

static void VmallocMmapFree(void *bufPriv)
{
    struct VmallocBuffer *buf = bufPriv;

    if (refcount_dec_and_test(&buf->refCount) != 0) {
        vfree(buf->vaddr);
        OsalMemFree(buf);
    }
}

static void *VmallocMmapAlloc(struct BufferQueue *queue, uint32_t planeNum, unsigned long size)
{
    struct BufferQueueImp *queueImp = container_of(queue, struct BufferQueueImp, queue);
    struct VmallocBuffer *buf = NULL;

    buf = OsalMemCalloc(sizeof(*buf));
    if (buf == NULL) {
        return ERR_PTR(-ENOMEM);
    }

    buf->size = size;
    buf->vaddr = vmalloc_user(buf->size);
    if (buf->vaddr == NULL) {
        HDF_LOGE("%s: vmalloc of size %{public}d failed", __func__, buf->size);
        OsalMemFree(buf);
        return ERR_PTR(-ENOMEM);
    }

    buf->dmaDir = queueImp->dmaDir;
    buf->handler.refCount = &buf->refCount;
    buf->handler.free = VmallocMmapFree;
    buf->handler.arg = buf;

    refcount_set(&buf->refCount, 1);
    return buf;
}

static void *VmallocAllocUserPtr(struct BufferQueue *queue,
    uint32_t planeNum, unsigned long vaddr, unsigned long size)
{
    struct BufferQueueImp *queueImp = container_of(queue, struct BufferQueueImp, queue);
    struct VmallocBuffer *buf = NULL;
    struct frame_vector *vec = NULL;
    int32_t numPages;
    int32_t offset;
    int32_t i;
    int32_t ret = -ENOMEM;

    buf = OsalMemCalloc(sizeof(*buf));
    if (buf == NULL) {
        return ERR_PTR(-ENOMEM);
    }

    buf->dmaDir = queueImp->dmaDir;
    offset = vaddr & ~PAGE_MASK;
    buf->size = size;
    vec = CreateFrameVec(vaddr, size);
    if (IS_ERR(vec)) {
        ret = PTR_ERR(vec);
        goto USERPTR_FAIL_PFNVEC;
    }
    buf->vec = vec;
    numPages = frame_vector_count(vec);
    if (frame_vector_to_pages(vec) < 0) {
        unsigned long *nums = frame_vector_pfns(vec);
        for (i = 1; i < numPages; i++) {
            if (nums[i - 1] + 1 != nums[i]) {
                goto USERPTR_FAIL_MAP;
            }
        }
        buf->vaddr = (__force void *)ioremap(__pfn_to_phys(nums[0]), size + offset);
    } else {
        buf->vaddr = vm_map_ram(frame_vector_pages(vec), numPages, -1);
    }

    if (buf->vaddr == NULL) {
        goto USERPTR_FAIL_MAP;
    }
    buf->vaddr = static_cast<void *>(static_cast<uint8_t *>(buf->vaddr) + offset);
    return buf;

USERPTR_FAIL_MAP:
    DestroyFrameVec(vec);
USERPTR_FAIL_PFNVEC:
    OsalMemFree(buf);

    return ERR_PTR(ret);
}

static void VmallocFreeUserptr(void *bufPriv)
{
    if (bufPriv == NULL) {
        return;
    }
    struct VmallocBuffer *buf = bufPriv;
    unsigned long vaddr = (unsigned long)buf->vaddr & PAGE_MASK;
    struct page **pages = NULL;

    if (!buf->vec->is_pfns) {
        uint32_t numPages = frame_vector_count(buf->vec);
        pages = frame_vector_pages(buf->vec);
        if (vaddr != 0) {
            vm_unmap_ram((void *)vaddr, numPages);
        }
        if (buf->dmaDir == DMA_FROM_DEVICE || buf->dmaDir == DMA_BIDIRECTIONAL) {
            for (uint32_t i = 0; i < numPages; i++) {
                set_page_dirty_lock(pages[i]);
            }
        }
    } else {
        iounmap((__force void __iomem *)buf->vaddr);
    }
    DestroyFrameVec(buf->vec);
    OsalMemFree(buf);
}

static void *VmallocGetVaddr(void *bufPriv)
{
    if (bufPriv == NULL) {
        return ERR_PTR(-EINVAL);
    }
    struct VmallocBuffer *buf = bufPriv;

    if (buf->vaddr == NULL) {
        HDF_LOGE("%s: Address of an unallocated plane requested or cannot map user pointer", __func__);
        return NULL;
    }

    return buf->vaddr;
}

static uint32_t VmallocNumUsers(void *bufPriv)
{
    if (bufPriv == NULL) {
        return 0;
    }
    struct VmallocBuffer *buf = bufPriv;
    return refcount_read(&buf->refCount);
}

static int32_t VmallocMmap(void *bufPriv, void *vm)
{
    struct VmallocBuffer *buf = bufPriv;
    struct vm_area_struct *vma =  (struct vm_area_struct *)vm;
    int32_t ret;
    if (buf == NULL) {
        HDF_LOGE("%s: No memory to map", __func__);
        return -EINVAL;
    }
    ret = remap_vmalloc_range(vma, buf->vaddr, 0);
    if (ret != 0) {
        HDF_LOGE("%s: Remapping vmalloc memory, error: %{public}d", __func__, ret);
        return ret;
    }
    vma->vm_flags |= VM_DONTEXPAND;
    vma->vm_private_data = &buf->handler;
    vma->vm_ops = GetVmOps();
    vma->vm_ops->open(vma);
    return 0;
}

static int32_t VmallocMapDmaBuf(void *memPriv)
{
    struct VmallocBuffer *buf = memPriv;
    if (buf == NULL) {
        HDF_LOGE("%s: No memory to map", __func__);
        return -EINVAL;
    }
    buf->vaddr = dma_buf_vmap(buf->dbuf);

    return buf->vaddr != NULL ? 0 : -EFAULT;
}

static void VmallocUnmapDmaBuf(void *memPriv)
{
    if (memPriv == NULL) {
        return;
    }
    struct VmallocBuffer *buf = memPriv;

    dma_buf_vunmap(buf->dbuf, buf->vaddr);
    buf->vaddr = NULL;
}

static void VmallocDetachDmaBuf(void *memPriv)
{
    if (memPriv == NULL) {
        return;
    }
    struct VmallocBuffer *buf = memPriv;

    if (buf->vaddr != NULL) {
        dma_buf_vunmap(buf->dbuf, buf->vaddr);
    }

    OsalMemFree(buf);
}

static void *VmallocAttachDmaBuf(struct BufferQueue *queue, uint32_t planeNum, void *dmaBuf, unsigned long size)
{
    struct BufferQueueImp *queueImp = container_of(queue, struct BufferQueueImp, queue);
    struct VmallocBuffer *buf = NULL;
    struct device *dev = NULL;
    struct dma_buf *dbuf = (struct dma_buf *)dmaBuf;

    if (queueImp->allocDev[planeNum] != NULL) {
        dev = queueImp->allocDev[planeNum];
    } else {
        dev = queueImp->dev;
    }
    if (dev == NULL || dbuf == NULL) {
        return ERR_PTR(-EINVAL);
    }
    if (dbuf->size < size) {
        return ERR_PTR(-EFAULT);
    }

    buf = OsalMemCalloc(sizeof(*buf));
    if (buf == NULL) {
        return ERR_PTR(-ENOMEM);
    }
    buf->dbuf = dbuf;
    buf->dmaDir = queueImp->dmaDir;
    buf->size = size;

    return buf;
}

struct MemOps g_virtualMallocOps = {
    .mmapAlloc      = VmallocMmapAlloc,
    .mmapFree       = VmallocMmapFree,
    .allocUserPtr   = VmallocAllocUserPtr,
    .freeUserPtr    = VmallocFreeUserptr,
    .mapDmaBuf      = VmallocMapDmaBuf,
    .unmapDmaBuf    = VmallocUnmapDmaBuf,
    .attachDmaBuf   = VmallocAttachDmaBuf,
    .detachDmaBuf   = VmallocDetachDmaBuf,
    .getVaddr       = VmallocGetVaddr,
    .mmap           = VmallocMmap,
    .numUsers       = VmallocNumUsers,
};

struct MemOps *GetVirtualMallocOps(void)
{
    return &g_virtualMallocOps;
}
