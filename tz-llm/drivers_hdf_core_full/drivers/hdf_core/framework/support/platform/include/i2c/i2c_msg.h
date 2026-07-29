/*
 * i2c_msg.h
 *
 * i2c message utils
 *
 * Copyright (c) 2023 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */
#ifndef HDF_I2C_MSG_H
#define HDF_I2C_MSG_H

#include "i2c_core.h"

int32_t I2cMsgsRebuildFromSbuf(struct HdfSBuf *data, struct I2cMsg **msgs, int16_t *count);

void I2cMsgsFree(struct I2cMsg *msgs, int16_t count);

int32_t I2cMsgsWriteToSbuf(struct I2cMsg *msgs, int16_t count, struct HdfSBuf *reply);

#endif // HDF_I2C_MSG_H
