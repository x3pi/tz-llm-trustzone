/*
 * Copyright (c) 2020-2023 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include "hdf_log.h"
#include "osal_mem.h"
#include "uart_core.h"
#include "uart_if.h"

#define HDF_LOG_TAG uart_servie_c
#define UART_RBUF_MALLOC_SIZE_MAX  65535

static int32_t UartIoRead(struct UartHost *host, struct HdfSBuf *data, struct HdfSBuf *reply)
{
    uint32_t len;
    int32_t ret;
    uint8_t *buf = NULL;

    if (!HdfSbufReadUint32(data, &len)) {
        HDF_LOGE("UartIoRead: sbuf read data len fail!");
        return HDF_ERR_IO;
    }

    if (len == 0 || len >= UART_RBUF_MALLOC_SIZE_MAX) {
        HDF_LOGE("UartIoRead: invalid buf len:%u!", len);
        return HDF_ERR_INVALID_PARAM;
    }

    buf = (uint8_t *)OsalMemCalloc(len);
    if (buf == NULL) {
        HDF_LOGE("UartIoRead: OsalMemCalloc error!");
        return HDF_ERR_MALLOC_FAIL;
    }

    ret = UartHostRead(host, buf, len);
    if (ret < 0) {
        HDF_LOGE("UartIoRead: Uart host read fail, ret: %d!", ret);
        OsalMemFree(buf);
        return HDF_ERR_IO;
    }

    if (!HdfSbufWriteBuffer(reply, (ret == 0) ? NULL : buf, ret)) {
        HDF_LOGE("UartIoRead: sbuf write buffer fail!");
        OsalMemFree(buf);
        return HDF_ERR_IO;
    }

    OsalMemFree(buf);
    return HDF_SUCCESS;
}

static int32_t UartIoWrite(struct UartHost *host, struct HdfSBuf *data)
{
    uint32_t size;
    uint8_t *buf = NULL;

    if (!HdfSbufReadBuffer(data, (const void **)&buf, &size)) {
        HDF_LOGE("UartIoWrite: sbuf read buffer fail!");
        return HDF_ERR_IO;
    }
    return UartHostWrite(host, buf, size);
}

static int32_t UartIoGetBaud(struct UartHost *host, struct HdfSBuf *reply)
{
    int32_t ret;
    uint32_t baudRate;

    ret = UartHostGetBaud(host, &baudRate);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("UartIoGetBaud: uart get baud fail, ret: %d!", ret);
        return ret;
    }
    if (!HdfSbufWriteUint32(reply, baudRate)) {
        HDF_LOGE("UartIoGetBaud: sbuf write buffer fail!");
        return HDF_ERR_IO;
    }
    return HDF_SUCCESS;
}

static int32_t UartIoSetBaud(struct UartHost *host, struct HdfSBuf *data)
{
    uint32_t baudRate;

    if (!HdfSbufReadUint32(data, &baudRate)) {
        HDF_LOGE("UartIoSetBaud: sbuf read baudRate fail!");
        return HDF_ERR_IO;
    }
    return UartHostSetBaud(host, baudRate);
}

static int32_t UartIoGetAttribute(struct UartHost *host, struct HdfSBuf *reply)
{
    int32_t ret;
    struct UartAttribute attribute;

    ret = UartHostGetAttribute(host, &attribute);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("UartIoGetAttribute: uart get attribute fail, ret: %d!", ret);
        return ret;
    }
    if (!HdfSbufWriteBuffer(reply, &attribute, sizeof(attribute))) {
        HDF_LOGE("UartIoGetAttribute: sbuf write attribute fail!");
        return HDF_ERR_IO;
    }
    return HDF_SUCCESS;
}

static int32_t UartIoSetAttribute(struct UartHost *host, struct HdfSBuf *data)
{
    uint32_t size;
    struct UartAttribute *attribute = NULL;

    if (!HdfSbufReadBuffer(data, (const void **)&attribute, &size)) {
        HDF_LOGE("UartIoSetAttribute: sbuf read buffer fail!");
        return HDF_ERR_IO;
    }

    if (size != sizeof(*attribute)) {
        HDF_LOGE("UartIoSetAttribute: sbuf read size not match, exp:%zu, got:%u!", sizeof(*attribute), size);
        return HDF_ERR_IO;
    }

    return UartHostSetAttribute(host, attribute);
}

static int32_t UartIoSetTransMode(struct UartHost *host, struct HdfSBuf *data)
{
    uint32_t mode;

    if (!HdfSbufReadUint32(data, &mode)) {
        HDF_LOGE("UartIoSetTransMode: sbuf read mode fail!");
        return HDF_ERR_IO;
    }
    return UartHostSetTransMode(host, mode);
}

int32_t UartIoDispatch(struct HdfDeviceIoClient *client, int cmd, struct HdfSBuf *data, struct HdfSBuf *reply)
{
    struct UartHost *host = NULL;

    if (client == NULL) {
        HDF_LOGE("UartIoDispatch: client is null!");
        return HDF_ERR_INVALID_OBJECT;
    }

    if (client->device == NULL) {
        HDF_LOGE("UartIoDispatch: client->device is null!");
        return HDF_ERR_INVALID_OBJECT;
    }

    if (client->device->service == NULL) {
        HDF_LOGE("UartIoDispatch: client->device->service is null!");
        return HDF_ERR_INVALID_OBJECT;
    }

    host = (struct UartHost *)client->device->service;
    switch (cmd) {
        case UART_IO_REQUEST:
            return UartHostRequest(host);
        case UART_IO_RELEASE:
            return UartHostRelease(host);
        case UART_IO_READ:
            return UartIoRead(host, data, reply);
        case UART_IO_WRITE:
            return UartIoWrite(host, data);
        case UART_IO_GET_BAUD:
            return UartIoGetBaud(host, reply);
        case UART_IO_SET_BAUD:
            return UartIoSetBaud(host, data);
        case UART_IO_GET_ATTRIBUTE:
            return UartIoGetAttribute(host, reply);
        case UART_IO_SET_ATTRIBUTE:
            return UartIoSetAttribute(host, data);
        case UART_IO_SET_TRANSMODE:
            return UartIoSetTransMode(host, data);
        default:
            HDF_LOGE("UartIoDispatch: cmd %d is not support!", cmd);
            return HDF_ERR_NOT_SUPPORT;
    }
}
