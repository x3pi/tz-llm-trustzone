/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "base/buffer_util.h"
#include <unistd.h>
#include "hdf_log.h"
#include "osal_mem.h"
#include "securec.h"

#define HDF_LOG_TAG buffer_util

BufferHandle *AllocateNativeBufferHandle(uint32_t reserveFds, uint32_t reserveInts)
{
    if (reserveFds > MAX_RESERVE_FDS || reserveInts > MAX_RESERVE_INTS) {
        HDF_LOGE("%{public}s: illegal reserveFds or reserveInts", __func__);
        return NULL;
    }

    size_t handleSize = sizeof(BufferHandle) + (sizeof(int32_t) * (reserveFds + reserveInts));
    BufferHandle *handle = (BufferHandle *)(OsalMemCalloc(handleSize));
    if (handle != NULL) {
        handle->fd = -1;
        handle->reserveFds = reserveFds;
        handle->reserveInts = reserveInts;
        for (uint32_t i = 0; i < reserveFds; i++) {
            handle->reserve[i] = -1;
        }
    } else {
        HDF_LOGE("BufferHandle malloc %zu failed", handleSize);
    }
    return handle;
}

BufferHandle *CloneNativeBufferHandle(const BufferHandle *other)
{
    if (other == NULL) {
        HDF_LOGW("%{public}s handle is NULL", __func__);
        return NULL;
    }

    BufferHandle *handle = AllocateNativeBufferHandle(other->reserveFds, other->reserveInts);
    if (handle == NULL) {
        HDF_LOGW("%{public}s AllocateBufferHandle failed, handle is NULL", __func__);
        return NULL;
    }

    if (other->fd == -1) {
        handle->fd = other->fd;
    } else {
        handle->fd = dup(other->fd);
        if (handle->fd == -1) {
            HDF_LOGE("CloneBufferHandle dup failed");
            FreeNativeBufferHandle(handle);
            return NULL;
        }
    }
    handle->width = other->width;
    handle->stride = other->stride;
    handle->height = other->height;
    handle->size = other->size;
    handle->format = other->format;
    handle->usage = other->usage;
    handle->phyAddr = other->phyAddr;

    for (uint32_t i = 0; i < handle->reserveFds; i++) {
        handle->reserve[i] = dup(other->reserve[i]);
        if (handle->reserve[i] == -1) {
            HDF_LOGE("CloneBufferHandle dup reserveFds failed");
            FreeNativeBufferHandle(handle);
            return NULL;
        }
    }

    if (other->reserveInts == 0) {
        return handle;
    }

    if (memcpy_s(&handle->reserve[handle->reserveFds], sizeof(int32_t) * handle->reserveInts,
                 &other->reserve[other->reserveFds], sizeof(int32_t) * other->reserveInts) != EOK) {
        HDF_LOGE("CloneBufferHandle memcpy_s failed");
        FreeNativeBufferHandle(handle);
        return NULL;
    }
    return handle;
}

void FreeNativeBufferHandle(BufferHandle *handle)
{
    if (handle == NULL) {
        return;
    }

    if (handle->fd != -1) {
        close(handle->fd);
        handle->fd = -1;
    }

    for (uint32_t i = 0; i < handle->reserveFds; i++) {
        if (handle->reserve[i] != -1) {
            close(handle->reserve[i]);
            handle->reserve[i] = -1;
        }
    }
    OsalMemFree(handle);
}

bool HdfSbufWriteNativeBufferHandle(struct HdfSBuf *data, const BufferHandle *handle)
{
    if (data == NULL) {
        HDF_LOGE("%{public}s: invalid sbuf", __func__);
        return false;
    }

    if (handle == NULL) {
        HDF_LOGE("%{public}s: write invalid handle", __func__);
        return false;
    }

    if (!HdfSbufWriteUint32(data, handle->reserveFds) || !HdfSbufWriteUint32(data, handle->reserveInts) ||
        !HdfSbufWriteInt32(data, handle->width) || !HdfSbufWriteInt32(data, handle->stride) ||
        !HdfSbufWriteInt32(data, handle->height) || !HdfSbufWriteInt32(data, handle->size) ||
        !HdfSbufWriteInt32(data, handle->format) || !HdfSbufWriteUint64(data, handle->usage)) {
        HDF_LOGE("%{public}s a lot failed", __func__);
        return false;
    }

    bool validFd = (handle->fd >= 0);
    if (!HdfSbufWriteInt8(data, validFd ? 1 : 0)) {
        HDF_LOGE("%{public}s: failed to write valid flag of fd", __func__);
        return false;
    }
    if (validFd && !HdfSbufWriteFileDescriptor(data, handle->fd)) {
        HDF_LOGE("%{public}s: failed to write fd", __func__);
        return false;
    }

    for (uint32_t i = 0; i < handle->reserveFds; i++) {
        if (!HdfSbufWriteFileDescriptor(data, handle->reserve[i])) {
            HDF_LOGE("%{public}s: failed to write reserved fd value", __func__);
            return false;
        }
    }
    for (uint32_t j = 0; j < handle->reserveInts; j++) {
        if (!HdfSbufWriteInt32(data, handle->reserve[handle->reserveFds + j])) {
            HDF_LOGE("%{public}s: failed to write reserved integer value", __func__);
            return false;
        }
    }
    return true;
}

static bool ReadReserveData(struct HdfSBuf *data, BufferHandle *handle)
{
    for (uint32_t i = 0; i < handle->reserveFds; i++) {
        handle->reserve[i] = HdfSbufReadFileDescriptor(data);
        if (handle->reserve[i] == -1) {
            HDF_LOGE("%{public}s: failed to read reserved fd value", __func__);
            return false;
        }
    }
    for (uint32_t j = 0; j < handle->reserveInts; j++) {
        if (!HdfSbufReadInt32(data, &(handle->reserve[handle->reserveFds + j]))) {
            HDF_LOGE("%{public}s: failed to read reserved integer value", __func__);
            return false;
        }
    }
    return true;
}

BufferHandle *HdfSbufReadNativeBufferHandle(struct HdfSBuf *data)
{
    if (data == NULL) {
        HDF_LOGE("%{public}s: invalid sbuf", __func__);
        return NULL;
    }

    uint32_t reserveFds = 0;
    uint32_t reserveInts = 0;
    if (!HdfSbufReadUint32(data, &reserveFds) || !HdfSbufReadUint32(data, &reserveInts)) {
        HDF_LOGE("%{public}s: failed to read reserveFds or reserveInts", __func__);
        return NULL;
    }

    BufferHandle *handle = AllocateNativeBufferHandle(reserveFds, reserveInts);
    if (handle == NULL) {
        HDF_LOGE("%{public}s: failed to malloc BufferHandle", __func__);
        return NULL;
    }

    if (!HdfSbufReadInt32(data, &handle->width) || !HdfSbufReadInt32(data, &handle->stride) ||
        !HdfSbufReadInt32(data, &handle->height) || !HdfSbufReadInt32(data, &handle->size) ||
        !HdfSbufReadInt32(data, &handle->format) || !HdfSbufReadUint64(data, &handle->usage)) {
        HDF_LOGE("%{public}s: failed to read a lot", __func__);
        FreeNativeBufferHandle(handle);
        return NULL;
    }

    bool validFd = false;
    if (!HdfSbufReadInt8(data, (int8_t *)&validFd)) {
        HDF_LOGE("%{public}s: failed to read valid flag of fd", __func__);
        FreeNativeBufferHandle(handle);
        return NULL;
    }
    if (validFd) {
        handle->fd = HdfSbufReadFileDescriptor(data);
        if (handle->fd == -1) {
            HDF_LOGE("%{public}s: failed to read fd", __func__);
            FreeNativeBufferHandle(handle);
            return NULL;
        }
    }

    if (!ReadReserveData(data, handle)) {
        FreeNativeBufferHandle(handle);
        return NULL;
    }
    return handle;
}
