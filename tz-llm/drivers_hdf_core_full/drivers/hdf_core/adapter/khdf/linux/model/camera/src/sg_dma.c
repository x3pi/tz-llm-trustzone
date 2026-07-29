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
#include "sg_dma.h"

struct SgDmaBuffer {
    struct device *dev;
    void *vaddr;
    struct page **pages;
    struct frame_vector *vec;
    int32_t offset;
    enum dma_data_direction dmaDir;
    struct sg_table sgTable;
    struct sg_table *dmaSgt;
    size_t size;
    uint32_t numPages;
    refcount_t refCount;
    struct VmareaHandler handler;
    struct dma_buf_attachment *dbAttach;
};

static void SgMmapFree(void *bufPriv)
{
    if (bufPriv == NULL) {
        return;
    }
    struct SgDmaBuffer *buf = bufPriv;
    struct sg_table *sgt = &buf->sgTable;
    int32_t i = buf->numPages;

    if (refcount_dec_and_test(&buf->refCount) != 0) {
        dma_unmap_sgtable(buf->dev, sgt, buf->dmaDir, DMA_ATTR_SKIP_CPU_SYNC);
        if (buf->vaddr != NULL) {
            vm_unmap_ram(buf->vaddr, buf->numPages);
        }
        sg_free_table(buf->dmaSgt);
        while (--i >= 0) {
            __free_page(buf->pages[i]);
        }
        kvfree(buf->pages);
        put_device(buf->dev);
        OsalMemFree(buf);
    }
}

static int32_t SgAllocCompacted(struct SgDmaBuffer *buf, gfp_t gfpFlags)
{
    if (buf == NULL) {
        return -EINVAL;
    }
    uint32_t lastPage = 0;
    unsigned long size = buf->size;

    while (size > 0) {
        struct page *pages = NULL;
        int32_t order;
        int32_t i;

        order = get_order(size);
        /* Don't over allocate */
        if ((PAGE_SIZE << order) > size) {
            order--;
        }

        while (pages == NULL) {
            pages = alloc_pages(GFP_KERNEL | __GFP_ZERO | __GFP_NOWARN | gfpFlags, order);
            if (pages != NULL) {
                break;
            }
            if (order != 0) {
                order--;
                continue;
            }
            while (lastPage--) {
                __free_page(buf->pages[lastPage]);
            }
            return -ENOMEM;
        }

        split_page(pages, order);
        for (i = 0; i < (1 << order); i++) {
            buf->pages[lastPage++] = &pages[i];
        }

        size -= PAGE_SIZE << order;
    }

    return 0;
}

static void *SgMmapAlloc(struct BufferQueue *queue, uint32_t planeNum, unsigned long size)
{
    struct BufferQueueImp *queueImp = container_of(queue, struct BufferQueueImp, queue);
    struct SgDmaBuffer *buf = NULL;
    struct sg_table *sgt = NULL;
    int32_t ret;
    int32_t numPages;
    struct device *dev = NULL;

    if (queueImp->allocDev[planeNum] != NULL) {
        dev = queueImp->allocDev[planeNum];
    } else {
        dev = queueImp->dev;
    }

    if (dev == NULL) {
        return ERR_PTR(-EINVAL);
    }
    buf = OsalMemCalloc(sizeof(*buf));
    if (buf == NULL) {
        return ERR_PTR(-ENOMEM);
    }
    buf->vaddr = NULL;
    buf->dmaDir = queueImp->dmaDir;
    buf->offset = 0;
    buf->size = size;
    /* size is already page aligned */
    buf->numPages = size >> PAGE_SHIFT;
    buf->dmaSgt = &buf->sgTable;
    buf->pages = kvmalloc_array(buf->numPages, sizeof(struct page *), GFP_KERNEL | __GFP_ZERO);
    if (buf->pages == NULL) {
        goto FAIL_PAGES_ARRAY_ALLOC;
    }
    ret = SgAllocCompacted(buf, queueImp->gfpFlags);
    if (ret != 0) {
        goto FAIL_PAGES_ALLOC;
    }
    ret = sg_alloc_table_from_pages(buf->dmaSgt, buf->pages, buf->numPages, 0, size, GFP_KERNEL);
    if (ret != 0) {
        goto FAIL_TABLE_ALLOC;
    }
    /* Prevent the device from being released while the buffer is used */
    buf->dev = get_device(dev);
    sgt = &buf->sgTable;
    if (dma_map_sgtable(buf->dev, sgt, buf->dmaDir, DMA_ATTR_SKIP_CPU_SYNC) != 0) {
        goto FAIL_MAP;
    }
    buf->handler.refCount = &buf->refCount;
    buf->handler.free = SgMmapFree;
    buf->handler.arg = buf;
    refcount_set(&buf->refCount, 1);
    return buf;
FAIL_MAP:
    put_device(buf->dev);
    sg_free_table(buf->dmaSgt);
FAIL_TABLE_ALLOC:
    numPages = buf->numPages;
    while (numPages--) {
        __free_page(buf->pages[numPages]);
    }
FAIL_PAGES_ALLOC:
    kvfree(buf->pages);
FAIL_PAGES_ARRAY_ALLOC:
    OsalMemFree(buf);
    return ERR_PTR(-ENOMEM);
}

static int32_t SgMmap(void *bufPriv, void *vm)
{
    struct SgDmaBuffer *buf = bufPriv;
    struct vm_area_struct *vma =  (struct vm_area_struct *)vm;
    int32_t err;

    if (buf == NULL) {
        HDF_LOGE("%s: buf is NULL!!!!!!", __func__);
        return -EINVAL;
    }

    err = vm_map_pages(vma, buf->pages, buf->numPages);
    if (err != 0) {
        HDF_LOGE("%s: vm_map_pages err! err = %{public}d", __func__, err);
        return err;
    }
    vma->vm_private_data = &buf->handler;
    vma->vm_ops = GetVmOps();
    vma->vm_ops->open(vma);
    return 0;
}

static void *SgAllocUserPtr(struct BufferQueue *queue,
    uint32_t planeNum, unsigned long vaddr, unsigned long size)
{
    struct BufferQueueImp *queueImp = container_of(queue, struct BufferQueueImp, queue);
    struct SgDmaBuffer *buf = NULL;
    struct sg_table *sgt = NULL;
    struct frame_vector *vec = NULL;
    struct device *dev = NULL;

    if (queueImp->allocDev[planeNum] != NULL) {
        dev = queueImp->allocDev[planeNum];
    } else {
        dev = queueImp->dev;
    }

    if (dev == NULL) {
        return ERR_PTR(-EINVAL);
    }
    buf = OsalMemAlloc(sizeof(*buf));
    if (buf == NULL) {
        return ERR_PTR(-ENOMEM);
    }
    buf->vaddr = NULL;
    buf->dev = dev;
    buf->dmaDir = queueImp->dmaDir;
    buf->offset = vaddr & ~PAGE_MASK;
    buf->size = size;
    buf->dmaSgt = &buf->sgTable;
    buf->dbAttach = NULL;
    vec = CreateFrameVec(vaddr, size);
    if (IS_ERR(vec)) {
        goto USERPTR_FAIL_PFNVEC;
    }
    buf->vec = vec;
    buf->pages = frame_vector_pages(vec);
    if (IS_ERR(buf->pages)) {
        goto USERPTR_FAIL_SGTABLE;
    }
    buf->numPages = frame_vector_count(vec);
    if (sg_alloc_table_from_pages(buf->dmaSgt, buf->pages, buf->numPages, buf->offset, size, 0) != 0) {
        goto USERPTR_FAIL_SGTABLE;
    }
    sgt = &buf->sgTable;
    if (dma_map_sgtable(buf->dev, sgt, buf->dmaDir, DMA_ATTR_SKIP_CPU_SYNC) != 0) {
        goto USERPTR_FAIL_MAP;
    }
    return buf;

USERPTR_FAIL_MAP:
    sg_free_table(&buf->sgTable);
USERPTR_FAIL_SGTABLE:
    DestroyFrameVec(vec);
USERPTR_FAIL_PFNVEC:
    OsalMemFree(buf);
    return ERR_PTR(-ENOMEM);
}

static void SgFreeUserPtr(void *bufPriv)
{
    if (bufPriv == NULL) {
        return;
    }
    struct SgDmaBuffer *buf = bufPriv;
    struct sg_table *sgt = &buf->sgTable;
    int32_t i = buf->numPages;

    dma_unmap_sgtable(buf->dev, sgt, buf->dmaDir, DMA_ATTR_SKIP_CPU_SYNC);
    if (buf->vaddr != NULL) {
        vm_unmap_ram(buf->vaddr, buf->numPages);
    }
    sg_free_table(buf->dmaSgt);
    if (buf->dmaDir == DMA_FROM_DEVICE || buf->dmaDir == DMA_BIDIRECTIONAL) {
        while (--i >= 0) {
            set_page_dirty_lock(buf->pages[i]);
        }
    }
    DestroyFrameVec(buf->vec);
    OsalMemFree(buf);
}

static int SgMapDmaBuf(void *memPriv)
{
    struct SgDmaBuffer *buf = memPriv;
    struct sg_table *sgt = NULL;

    if (buf == NULL) {
        return -EINVAL;
    }
    if (WARN_ON(!buf->dbAttach)) {
        HDF_LOGE("%s: trying to pin a non attached buffer", __func__);
        return -EINVAL;
    }

    if (WARN_ON(buf->dmaSgt)) {
        HDF_LOGE("%s: dmabuf buffer is already pinned", __func__);
        return 0;
    }

    /* get the associated scatterlist for this buffer */
    sgt = dma_buf_map_attachment(buf->dbAttach, buf->dmaDir);
    if (IS_ERR(sgt)) {
        HDF_LOGE("%s: Error getting dmabuf scatterlist", __func__);
        return -EINVAL;
    }

    buf->dmaSgt = sgt;
    buf->vaddr = NULL;

    return 0;
}

static void SgUnmapDmaBuf(void *memPriv)
{
    if (memPriv == NULL) {
        return;
    }
    struct SgDmaBuffer *buf = memPriv;
    struct sg_table *sgt = buf->dmaSgt;

    if (WARN_ON(!buf->dbAttach)) {
        HDF_LOGE("%s: trying to unpin a not attached buffer", __func__);
        return;
    }

    if (WARN_ON(!sgt)) {
        HDF_LOGE("%s: dmabuf buffer is already unpinned", __func__);
        return;
    }

    if (buf->vaddr != NULL) {
        dma_buf_vunmap(buf->dbAttach->dmabuf, buf->vaddr);
        buf->vaddr = NULL;
    }
    dma_buf_unmap_attachment(buf->dbAttach, sgt, buf->dmaDir);

    buf->dmaSgt = NULL;
}

static void SgDetachDmaBuf(void *memPriv)
{
    if (memPriv == NULL) {
        return;
    }

    struct SgDmaBuffer *buf = memPriv;
    /* if vb2 works correctly you should never detach mapped buffer */
    if (WARN_ON(buf->dmaSgt)) {
        SgUnmapDmaBuf(buf);
    }

    /* detach this attachment */
    dma_buf_detach(buf->dbAttach->dmabuf, buf->dbAttach);
    kfree(buf);
}

static void *SgAttachDmaBuf(struct BufferQueue *queue, uint32_t planeNum, void *dmaBuf, unsigned long size)
{
    struct BufferQueueImp *queueImp = container_of(queue, struct BufferQueueImp, queue);
    struct SgDmaBuffer *buf = NULL;
    struct dma_buf_attachment *dba = NULL;
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

    buf = (struct SgDmaBuffer *)OsalMemCalloc(sizeof(*buf));
    if (buf == NULL) {
        return ERR_PTR(-ENOMEM);
    }

    buf->dev = dev;
    /* create attachment for the dmabuf with the user device */
    dba = dma_buf_attach(dbuf, buf->dev);
    if (IS_ERR(dba)) {
        HDF_LOGE("%s: failed to attach dmabuf", __func__);
        OsalMemFree(buf);
        return dba;
    }

    buf->dmaDir = queueImp->dmaDir;
    buf->size = size;
    buf->dbAttach = dba;

    return buf;
}

static void *SgGetCookie(void *bufPriv)
{
    struct SgDmaBuffer *buf = bufPriv;
    if (buf == NULL) {
        return ERR_PTR(-EINVAL);
    }
    return buf->dmaSgt;
}

static void *SgGetVaddr(void *bufPriv)
{
    struct SgDmaBuffer *buf = bufPriv;
    if (buf == NULL) {
        return ERR_PTR(-EINVAL);
    }

    if (buf->vaddr == NULL) {
        if (buf->dbAttach != NULL) {
            buf->vaddr = dma_buf_vmap(buf->dbAttach->dmabuf);
        } else {
            buf->vaddr = vm_map_ram(buf->pages, buf->numPages, -1);
        }
    }

    /* add offset in case userptr is not page-aligned */
    return buf->vaddr != NULL ? static_cast<void *>(static_cast<uint8_t *>(buf->vaddr) + buf->offset) : NULL;
}

static unsigned int SgNumUsers(void *bufPriv)
{
    struct SgDmaBuffer *buf = bufPriv;
    if (buf == NULL) {
        return 0;
    }

    return refcount_read(&buf->refCount);
}

static void SgPrepareMem(void *bufPriv)
{
    if (bufPriv == NULL) {
        return;
    }
    struct SgDmaBuffer *buf = bufPriv;
    struct sg_table *sgt = buf->dmaSgt;

    dma_sync_sgtable_for_device(buf->dev, sgt, buf->dmaDir);
}

static void SgFinishMem(void *bufPriv)
{
    if (bufPriv == NULL) {
        return;
    }
    struct SgDmaBuffer *buf = bufPriv;
    struct sg_table *sgt = buf->dmaSgt;

    dma_sync_sgtable_for_cpu(buf->dev, sgt, buf->dmaDir);
}

struct MemOps g_sgDmaOps = {
    .mmapAlloc      = SgMmapAlloc,
    .mmapFree       = SgMmapFree,
    .mmap           = SgMmap,
    .allocUserPtr   = SgAllocUserPtr,
    .freeUserPtr    = SgFreeUserPtr,
    .mapDmaBuf      = SgMapDmaBuf,
    .unmapDmaBuf    = SgUnmapDmaBuf,
    .attachDmaBuf   = SgAttachDmaBuf,
    .detachDmaBuf   = SgDetachDmaBuf,
    .getCookie      = SgGetCookie,
    .getVaddr       = SgGetVaddr,
    .numUsers       = SgNumUsers,
    .syncForDevice  = SgPrepareMem,
    .syncForUser    = SgFinishMem,
};

struct MemOps *GetSgDmaOps(void)
{
    return &g_sgDmaOps;
}
