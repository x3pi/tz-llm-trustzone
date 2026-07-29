/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#ifndef SPI_SERVICE_H
#define SPI_SERVICE_H

#include "hdf_base.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SPI_USER_MSG_READ  (0x1 << 1)
#define SPI_USER_MSG_WRITE (0x1 << 2)

#pragma pack(push, 4)
struct SpiUserMsg {
    uint8_t rwFlag;   /**< the msg read write flag */
    uint32_t len;     /**< Length of the read and write buffers. The read buffer and the write
                       * buffer have the same length.
                       */
    uint32_t speed;   /**< Current message transfer speed */
    uint16_t delayUs; /**< Delay (in microseconds) before starting the next transfer.
                       * The value <b>0</b> indicates there is no delay between transfers.
                       */
    uint8_t keepCs;   /**< Whether to keep CS active after current transfer has been
                       * completed. <b>1</b> indicates to keeps CS; <b>0</b> indicates to switch off the CS.
                       */
};
#pragma pack(pop)

#ifdef __cplusplus
}
#endif

#endif /* SPI_SERVICE_H */
