/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#ifndef CAMERA_BUFFER_MANAGER_SG_DMA_H
#define CAMERA_BUFFER_MANAGER_SG_DMA_H

#include <linux/scatterlist.h>
#include <linux/slab.h>
#include "camera_buffer_manager_adapter.h"
#include "camera_buffer.h"

struct MemOps *GetSgDmaOps(void);

static inline struct sg_table *DmaSgPlaneDesc(struct CameraBuffer *buffer, uint32_t planeId)
{
    struct BufferQueue *queue = buffer->bufferQueue;
	
    if (planeId >= buffer->numPlanes || buffer->planes[planeId].memPriv == NULL) {
        return NULL;
    }

    if (queue->memOps->getCookie != NULL) {
        return (struct sg_table *)queue->memOps->getCookie(buffer->planes[planeId].memPriv);
    }

    return NULL;
}

#endif /* CAMERA_BUFFER_MANAGER_SG_DMA_H */