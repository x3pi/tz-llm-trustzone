/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include <securec.h>
#include "camera_buffer_manager_adapter.h"
#include "osal_mem.h"
#include "contig_dma.h"

struct ContigBuffer {
    struct device *dev;
    void *vaddr;
    unsigned long size;
    void *cookie;
    dma_addr_t dmaAddr;
    unsigned long dmaAttrs;
    enum dma_data_direction dmaDir;
    struct sg_table *dmaSgt;
    struct frame_vector *vec;

    /* MMAP related */
    struct VmareaHandler handler;
    refcount_t refCount;
    struct sg_table *sgtBase;

    /* DMABUF related */
    struct dma_buf_attachment *dbAttach;
};

/* scatterlist table functions */
static unsigned long GetContiguousSize(struct sg_table *sgt)
{
    struct scatterlist *slist = NULL;
    dma_addr_t expected = sg_dma_address(sgt->sgl);
    uint32_t i;
    unsigned long size = 0;

    for_each_sgtable_dma_sg(sgt, slist, i) {
        if (sg_dma_address(slist) != expected) {
            break;
        }
        expected += sg_dma_len(slist);
        size += sg_dma_len(slist);
    }
    return size;
}

/* callbacks for MMAP buffers */
static void ContigMmapFree(void *bufPriv)
{
    struct ContigBuffer *buf = bufPriv;
    if (buf == NULL) {
        return;
    }
    if (refcount_dec_and_test(&buf->refCount) == 0) {
        return;
    }

    if (buf->sgtBase != NULL) {
        sg_free_table(buf->sgtBase);
        OsalMemFree(buf->sgtBase);
        buf->sgtBase = NULL;
    }
    dma_free_attrs(buf->dev, buf->size, buf->cookie, buf->dmaAddr, buf->dmaAttrs);
    put_device(buf->dev);
    OsalMemFree(buf);
}

static void *ContigMmapAlloc(struct BufferQueue *queue, uint32_t planeNum, unsigned long size)
{
    struct BufferQueueImp *queueImp = container_of(queue, struct BufferQueueImp, queue);
    struct ContigBuffer *buf = NULL;
    struct device *dev;

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

    buf->dmaAttrs = queueImp->dmaAttrs;
    buf->cookie = dma_alloc_attrs(dev, size, &buf->dmaAddr, GFP_KERNEL | queueImp->gfpFlags, buf->dmaAttrs);
    if (buf->cookie == NULL) {
        OsalMemFree(buf);
        return ERR_PTR(-ENOMEM);
    }

    if ((buf->dmaAttrs & DMA_ATTR_NO_KERNEL_MAPPING) == 0) {
        buf->vaddr = buf->cookie;
    }

    /* Prevent the device from being released while the buffer is used */
    buf->dev = get_device(dev);
    buf->size = size;
    buf->dmaDir = queueImp->dmaDir;

    buf->handler.refCount = &buf->refCount;
    buf->handler.free = ContigMmapFree;
    buf->handler.arg = buf;

    refcount_set(&buf->refCount, 1);

    return buf;
}

static int32_t ContigMmap(void *bufPriv, void *vm)
{
    struct ContigBuffer *buf = bufPriv;
    struct vm_area_struct *vma =  (struct vm_area_struct *)vm;
    int32_t ret;

    if (buf == NULL) {
        return -EINVAL;
    }

    ret = dma_mmap_attrs(buf->dev, vma, buf->cookie, buf->dmaAddr, buf->size, buf->dmaAttrs);
    if (ret != 0) {
        return ret;
    }

    vma->vm_flags |= VM_DONTEXPAND | VM_DONTDUMP;
    vma->vm_private_data = &buf->handler;
    vma->vm_ops = GetVmOps();

    vma->vm_ops->open(vma);

    return 0;
}

/* callbacks for USERPTR buffers */
static void *ContigAllocUserPtr(struct BufferQueue *queue,
    uint32_t planeNum, unsigned long vaddr, unsigned long size)
{
    struct BufferQueueImp *queueImp = container_of(queue, struct BufferQueueImp, queue);
    struct ContigBuffer *buf = NULL;
    struct frame_vector *vec = NULL;
    struct device *dev = NULL;
    uint32_t offset;
    int32_t numPages;
    int32_t i;
    int32_t ret;
    struct sg_table *sgt = NULL;
    unsigned long contigSize;
    unsigned long dmaAlign = dma_get_cache_alignment();

    if (queueImp->allocDev[planeNum] != NULL) {
        dev = queueImp->allocDev[planeNum];
    } else {
        dev = queueImp->dev;
    }
    /* Only cache aligned DMA transfers are reliable */
    if (IS_ALIGNED(vaddr | size, dmaAlign) == 0 || size == 0 || dev == NULL) {
        return ERR_PTR(-EINVAL);
    }
    buf = OsalMemCalloc(sizeof(*buf));
    if (buf == NULL) {
        return ERR_PTR(-ENOMEM);
    }
    buf->dev = dev;
    buf->dmaDir = queueImp->dmaDir;
    offset = lower_32_bits(offset_in_page(vaddr));
    vec = CreateFrameVec(vaddr, size);
    if (IS_ERR(vec)) {
        ret = PTR_ERR(vec);
        goto FAIL_BUF;
    }
    buf->vec = vec;
    numPages = frame_vector_count(vec);
    ret = frame_vector_to_pages(vec);
    if (ret < 0) {
        unsigned long *nums = frame_vector_pfns(vec);
        for (i = 1; i < numPages; i++) {
            if (nums[i - 1] + 1 != nums[i]) {
                goto FAIL_PFNVEC;
            }
        }
        buf->dmaAddr = dma_map_resource(buf->dev, __pfn_to_phys(nums[0]), size, buf->dmaDir, 0);
        if (dma_mapping_error(buf->dev, buf->dmaAddr) != 0) {
            ret = -ENOMEM;
            goto FAIL_PFNVEC;
        }
        goto OUT;
    }
    sgt = OsalMemCalloc(sizeof(*sgt));
    if (sgt == NULL) {
        ret = -ENOMEM;
        goto FAIL_PFNVEC;
    }
    ret = sg_alloc_table_from_pages(sgt, frame_vector_pages(vec), numPages, offset, size, GFP_KERNEL);
    if (ret != 0) {
        goto FAIL_SGT;
    }
    if (dma_map_sgtable(buf->dev, sgt, buf->dmaDir, DMA_ATTR_SKIP_CPU_SYNC) != 0) {
        ret = -EIO;
        goto FAIL_SGT_INIT;
    }
    contigSize = GetContiguousSize(sgt);
    if (contigSize < size) {
        ret = -EFAULT;
        goto FAIL_MAP_SG;
    }
    buf->dmaAddr = sg_dma_address(sgt->sgl);
    buf->dmaSgt = sgt;
OUT:
    buf->size = size;
    return buf;
FAIL_MAP_SG:
    dma_unmap_sgtable(buf->dev, sgt, buf->dmaDir, DMA_ATTR_SKIP_CPU_SYNC);
FAIL_SGT_INIT:
    sg_free_table(sgt);
FAIL_SGT:
    OsalMemFree(sgt);
FAIL_PFNVEC:
    DestroyFrameVec(vec);
FAIL_BUF:
    OsalMemFree(buf);
    return ERR_PTR(ret);
}

static void ContigFreeUserPtr(void *bufPriv)
{
    if (bufPriv == NULL) {
        return;
    }
    struct ContigBuffer *buf = bufPriv;
    struct sg_table *sgt = buf->dmaSgt;
    struct page **pages = NULL;

    if (sgt != NULL) {
        dma_unmap_sgtable(buf->dev, sgt, buf->dmaDir, DMA_ATTR_SKIP_CPU_SYNC);
        pages = frame_vector_pages(buf->vec);
        /* sgt should exist only if vector contains pages... */
        if (IS_ERR(pages)) {
            return;
        }
        if (buf->dmaDir == DMA_FROM_DEVICE || buf->dmaDir == DMA_BIDIRECTIONAL) {
            int32_t frameVecCnt = frame_vector_count(buf->vec);
            for (int32_t i = 0; i < frameVecCnt; i++) {
                set_page_dirty_lock(pages[i]);
            }
        }
        sg_free_table(sgt);
        OsalMemFree(sgt);
    } else {
        dma_unmap_resource(buf->dev, buf->dmaAddr, buf->size, buf->dmaDir, 0);
    }
    DestroyFrameVec(buf->vec);
    OsalMemFree(buf);
}

/* callbacks for DMABUF buffers */
static int ContigMapDmaBuf(void *bufPriv)
{
    if (bufPriv == NULL) {
        return -EINVAL;
    }
    struct ContigBuffer *buf = bufPriv;
    struct sg_table *sgt = NULL;
    unsigned long contigSize;

    if (buf->dbAttach == NULL) {
        return -EINVAL;
    }

    if (buf->dmaSgt != NULL) {
        return 0;
    }

    /* get the associated scatterlist for this buffer */
    sgt = dma_buf_map_attachment(buf->dbAttach, buf->dmaDir);
    if (IS_ERR(sgt)) {
        return -EINVAL;
    }

    /* checking if dmabuf is big enough to store contiguous chunk */
    contigSize = GetContiguousSize(sgt);
    if (contigSize < buf->size) {
        dma_buf_unmap_attachment(buf->dbAttach, sgt, buf->dmaDir);
        return -EFAULT;
    }

    buf->dmaAddr = sg_dma_address(sgt->sgl);
    buf->dmaSgt = sgt;
    buf->vaddr = NULL;

    return 0;
}

static void ContigUnmapDmaBuf(void *bufPriv)
{
    if (bufPriv == NULL) {
        return;
    }
    struct ContigBuffer *buf = bufPriv;
    struct sg_table *sgt = buf->dmaSgt;

    if (buf->dbAttach == NULL) {
        return;
    }

    if (sgt == NULL) {
        return;
    }

    if (buf->vaddr != NULL) {
        dma_buf_vunmap(buf->dbAttach->dmabuf, buf->vaddr);
        buf->vaddr = NULL;
    }
    dma_buf_unmap_attachment(buf->dbAttach, sgt, buf->dmaDir);

    buf->dmaAddr = 0;
    buf->dmaSgt = NULL;
}

static void *ContigAttachDmaBuf(struct BufferQueue *queue, uint32_t planeNum, void *dmaBuf, unsigned long size)
{
    struct BufferQueueImp *queueImp = container_of(queue, struct BufferQueueImp, queue);
    struct ContigBuffer *buf = NULL;
    struct dma_buf_attachment *dba = NULL;
    struct device *dev = NULL;
    struct dma_buf *dbuf = (struct dma_buf *)dmaBuf;

    if (queueImp->allocDev[planeNum] != NULL) {
        dev = queueImp->allocDev[planeNum];
    } else {
        dev = queueImp->dev;
    }
    if (dbuf == NULL || dev == NULL) {
        return ERR_PTR(-EINVAL);
    }
    if (dbuf->size < size) {
        return ERR_PTR(-EFAULT);
    }

    buf = OsalMemCalloc(sizeof(*buf));
    if (buf == NULL) {
        return ERR_PTR(-ENOMEM);
    }
    if (memset_s(buf, sizeof(struct ContigBuffer), 0, sizeof(struct ContigBuffer)) != EOK) {
        HDF_LOGE("ContigAttachDmaBuf: [memcpy_s] fail!");
        return;
    }
    buf->dev = dev;
    /* create attachment for the dmabuf with the user device */
    dba = dma_buf_attach(dbuf, buf->dev);
    if (IS_ERR(dba)) {
        OsalMemFree(buf);
        return dba;
    }

    buf->dmaDir = queueImp->dmaDir;
    buf->size = size;
    buf->dbAttach = dba;

    return buf;
}


static void ContigDetachDmaBuf(void *bufPriv)
{
    struct ContigBuffer *buf = bufPriv;
    if (buf == NULL) {
        return;
    }

    /* if vb2 works correctly you should never detach mapped buffer */
    if (buf->dmaAddr != 0) {
        ContigUnmapDmaBuf(buf);
    }

    /* detach this attachment */
    dma_buf_detach(buf->dbAttach->dmabuf, buf->dbAttach);
    OsalMemFree(buf);
}

/* callbacks for all buffers */
static void *ContigGetCookie(void *bufPriv)
{
    struct ContigBuffer *buf = bufPriv;
    if (buf == NULL) {
        return ERR_PTR(-EINVAL);
    }

    return &buf->dmaAddr;
}

static void *ContigGetVaddr(void *bufPriv)
{
    struct ContigBuffer *buf = bufPriv;
    if (buf == NULL) {
        return ERR_PTR(-EINVAL);
    }

    if ((buf->vaddr == NULL) && (buf->dbAttach != NULL)) {
        buf->vaddr = dma_buf_vmap(buf->dbAttach->dmabuf);
    }

    return buf->vaddr;
}

static unsigned int ContigNumUsers(void *bufPriv)
{
    struct ContigBuffer *buf = bufPriv;
    if (buf == NULL) {
        return 0;
    }

    return refcount_read(&buf->refCount);
}

static void ContigPrepareMem(void *bufPriv)
{
    if (bufPriv == NULL) {
        return;
    }
    struct ContigBuffer *buf = bufPriv;
    struct sg_table *sgt = buf->dmaSgt;

    if (sgt == NULL) {
        return;
    }

    dma_sync_sgtable_for_device(buf->dev, sgt, buf->dmaDir);
}

static void ContigFinishMem(void *bufPriv)
{
    if (bufPriv == NULL) {
        return;
    }
    struct ContigBuffer *buf = bufPriv;
    struct sg_table *sgt = buf->dmaSgt;

    if (sgt == NULL) {
        return;
    }

    dma_sync_sgtable_for_cpu(buf->dev, sgt, buf->dmaDir);
}

struct MemOps g_dmaContigOps = {
    .mmapAlloc      = ContigMmapAlloc,
    .mmapFree       = ContigMmapFree,
    .mmap           = ContigMmap,
    .allocUserPtr   = ContigAllocUserPtr,
    .freeUserPtr    = ContigFreeUserPtr,
    .mapDmaBuf      = ContigMapDmaBuf,
    .unmapDmaBuf    = ContigUnmapDmaBuf,
    .attachDmaBuf   = ContigAttachDmaBuf,
    .detachDmaBuf   = ContigDetachDmaBuf,
    .getCookie      = ContigGetCookie,
    .getVaddr       = ContigGetVaddr,
    .numUsers       = ContigNumUsers,
    .syncForDevice  = ContigPrepareMem,
    .syncForUser    = ContigFinishMem,
};

struct MemOps *GetDmaContigOps(void)
{
    return &g_dmaContigOps;
}
