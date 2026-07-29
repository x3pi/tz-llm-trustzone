/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#ifndef CAMERA_BUFFER_MANAGER_CONTIG_DMA_H
#define CAMERA_BUFFER_MANAGER_CONTIG_DMA_H

#include <linux/scatterlist.h>
#include "camera_buffer_manager_adapter.h"
#include "camera_buffer.h"

struct MemOps *GetDmaContigOps(void);

static inline struct dma_addr_t *ContigDmaGetAddr(struct CameraBuffer *buffer, uint32_t planeId)
{
    struct BufferQueue *queue = buffer->bufferQueue;
    struct dma_addr_t *addr = NULL;

    if (planeId >= buffer->numPlanes || buffer->planes[planeId].memPriv == NULL) {
        return NULL;
    }

    if (queue->memOps->getCookie != NULL) {
        addr = queue->memOps->getCookie(buffer->planes[planeId].memPriv);
    }

    return addr;
}

#endif /* CAMERA_BUFFER_MANAGER_CONTIG_DMA_H */