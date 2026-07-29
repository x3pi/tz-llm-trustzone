/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#ifndef CAN_MSG_H
#define CAN_MSG_H

#include "can_if.h"
#include "hdf_sref.h"
#include "platform_queue.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

struct CanMsgPool;

void CanMsgGet(const struct CanMsg *msg);

void CanMsgPut(const struct CanMsg *msg);

struct CanMsg *CanMsgPoolObtainMsg(struct CanMsgPool *pool);

struct CanMsgPool *CanMsgPoolCreate(size_t size);

void CanMsgPoolDestroy(struct CanMsgPool *pool);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
