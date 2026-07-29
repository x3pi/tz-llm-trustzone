/*
 * Copyright (c) 2022-2023 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include "hdf_io_service_if.h"
#include "hdf_log.h"
#include "securec.h"
#include "uart_if.h"

#define HDF_LOG_TAG uart_if_u_c
#define UART_HOST_NAME_LEN 32

static void *UartGetObjGetByBusNum(uint32_t num)
{
    int ret;
    char name[UART_HOST_NAME_LEN + 1] = {0};

    ret = snprintf_s(name, UART_HOST_NAME_LEN + 1, UART_HOST_NAME_LEN, "HDF_PLATFORM_UART_%u", num);
    if (ret < 0) {
        HDF_LOGE("UartGetObjGetByBusNum: snprintf_s fail!");
        return NULL;
    }

    return (void *)HdfIoServiceBind(name);
}

static void UartPutObjByPointer(const void *obj)
{
    if (obj == NULL) {
        HDF_LOGE("UartPutObjByPointer: obj is null!");
        return;
    }
    HdfIoServiceRecycle((struct HdfIoService *)obj);
};

DevHandle UartOpen(uint32_t port)
{
    int32_t ret;
    void *handle = NULL;

    handle = UartGetObjGetByBusNum(port);
    if (handle == NULL) {
        HDF_LOGE("UartOpen: get handle error!");
        return NULL;
    }

    struct HdfIoService *service = (struct HdfIoService *)handle;
    if (service->dispatcher == NULL || service->dispatcher->Dispatch == NULL) {
        HDF_LOGE("UartOpen: service is invalid!");
        UartPutObjByPointer(handle);
        return NULL;
    }
    ret = service->dispatcher->Dispatch(&service->object, UART_IO_REQUEST, NULL, NULL);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("UartOpen: uart host request error, ret: %d!", ret);
        UartPutObjByPointer(handle);
        return NULL;
    }

    return (DevHandle)handle;
}

void UartClose(DevHandle handle)
{
    int32_t ret;

    if (handle == NULL) {
        HDF_LOGE("UartClose: handle is null!");
        return;
    }

    struct HdfIoService *service = (struct HdfIoService *)handle;
    if (service->dispatcher == NULL || service->dispatcher->Dispatch == NULL) {
        HDF_LOGE("UartClose: service is invalid!");
        UartPutObjByPointer(handle);
        return;
    }

    ret = service->dispatcher->Dispatch(&service->object, UART_IO_RELEASE, NULL, NULL);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("UartClose: uart host release error, ret: %d!", ret);
    }
    UartPutObjByPointer(handle);
}

static int32_t UartDispatch(DevHandle handle, int cmd, struct HdfSBuf *data, struct HdfSBuf *reply)
{
    int32_t ret;
    struct HdfIoService *service = (struct HdfIoService *)handle;

    if (service == NULL) {
        HDF_LOGE("UartDispatch: service is null!");
        return HDF_ERR_INVALID_OBJECT;
    }

    if (service->dispatcher == NULL || service->dispatcher->Dispatch == NULL) {
        HDF_LOGE("UartDispatch: dispatcher or dispatch is null!");
        return HDF_ERR_INVALID_PARAM;
    }

    ret = service->dispatcher->Dispatch(&service->object, cmd, data, reply);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("UartDispatch: dispatch fail, ret: %d!", ret);
    }
    return ret;
}

int32_t UartRead(DevHandle handle, uint8_t *data, uint32_t size)
{
    int32_t ret;
    struct HdfSBuf *sizeBuf = NULL;
    struct HdfSBuf *reply = NULL;
    uint32_t tmpLen;
    const void *tmpBuf = NULL;

    if (data == NULL || size == 0) {
        return HDF_ERR_INVALID_PARAM;
    }

    sizeBuf = HdfSbufObtainDefaultSize();
    if (sizeBuf == NULL) {
        HDF_LOGE("UartRead: fail to obtain sizeBuf!");
        return HDF_ERR_MALLOC_FAIL;
    }

    if (!HdfSbufWriteUint32(sizeBuf, size)) {
        HDF_LOGE("UartRead: write read size fail!");
        HdfSbufRecycle(sizeBuf);
        return HDF_ERR_IO;
    }

    reply = HdfSbufObtain(size + sizeof(uint64_t));
    if (reply == NULL) {
        HDF_LOGE("UartRead: fail to obtain reply buf!");
        HdfSbufRecycle(sizeBuf);
        return HDF_ERR_MALLOC_FAIL;
    }

    ret = UartDispatch(handle, UART_IO_READ, sizeBuf, reply);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("UartRead: uart read fail, ret: %d!", ret);
        goto EXIT;
    }

    if (!HdfSbufReadBuffer(reply, &tmpBuf, &tmpLen)) {
        HDF_LOGE("UartRead: sbuf read buffer fail!");
        ret = HDF_ERR_IO;
        goto EXIT;
    }

    if (tmpLen > 0 && memcpy_s(data, size, tmpBuf, tmpLen) != EOK) {
        HDF_LOGE("UartRead: memcpy data fail!");
        ret = HDF_ERR_IO;
        goto EXIT;
    }
    ret = (int32_t)tmpLen; // the return value is the length of the read data. max length 65535

EXIT:
    HdfSbufRecycle(sizeBuf);
    HdfSbufRecycle(reply);
    return ret;
}

int32_t UartWrite(DevHandle handle, uint8_t *data, uint32_t size)
{
    int32_t ret;
    struct HdfSBuf *dataBuf = NULL;

    if (data == NULL || size == 0) {
        HDF_LOGE("UartWrite: data is null or size is invalid!");
        return HDF_ERR_INVALID_PARAM;
    }

    dataBuf = HdfSbufObtainDefaultSize();
    if (dataBuf == NULL) {
        HDF_LOGE("UartWrite: fail to obtain write buf!");
        return HDF_ERR_MALLOC_FAIL;
    }

    if (!HdfSbufWriteBuffer(dataBuf, data, size)) {
        HDF_LOGE("UartWrite: sbuf write buffer fail!");
        HdfSbufRecycle(dataBuf);
        return HDF_ERR_IO;
    }

    ret = UartDispatch(handle, UART_IO_WRITE, dataBuf, NULL);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("UartWrite: uart write fail, ret: %d!", ret);
    }
    HdfSbufRecycle(dataBuf);
    return ret;
}

int32_t UartGetBaud(DevHandle handle, uint32_t *baudRate)
{
    int32_t ret;
    struct HdfSBuf *reply = NULL;

    if (baudRate == NULL) {
        HDF_LOGE("UartGetBaud: baudRate is null!");
        return HDF_ERR_INVALID_PARAM;
    }

    reply = HdfSbufObtainDefaultSize();
    if (reply == NULL) {
        HDF_LOGE("UartGetBaud: fail to obtain reply buf!");
        return HDF_ERR_MALLOC_FAIL;
    }

    ret = UartDispatch(handle, UART_IO_GET_BAUD, NULL, reply);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("UartGetBaud: get uart baud fail, ret: %d!", ret);
        HdfSbufRecycle(reply);
        return ret;
    }

    if (!HdfSbufReadUint32(reply, baudRate)) {
        HDF_LOGE("UartGetBaud: read baudRate fail!");
        ret = HDF_ERR_IO;
    }

    HdfSbufRecycle(reply);
    return ret;
}

int32_t UartSetBaud(DevHandle handle, uint32_t baudRate)
{
    int32_t ret;
    struct HdfSBuf *data = NULL;

    data = HdfSbufObtainDefaultSize();
    if (data == NULL) {
        HDF_LOGE("UartSetBaud: fail to obtain data buf!");
        return HDF_ERR_MALLOC_FAIL;
    }

    if (!HdfSbufWriteUint32(data, baudRate)) {
        HDF_LOGE("UartSetBaud: sbuf write baudRate fail!");
        HdfSbufRecycle(data);
        return HDF_ERR_IO;
    }

    ret = UartDispatch(handle, UART_IO_SET_BAUD, data, NULL);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("UartSetBaud: set uart baud fail, ret: %d!", ret);
    }
    HdfSbufRecycle(data);
    return ret;
}

int32_t UartGetAttribute(DevHandle handle, struct UartAttribute *attribute)
{
    int32_t ret;
    struct HdfSBuf *reply = NULL;
    uint32_t tmpLen;
    const void *tmpBuf = NULL;

    if (attribute == NULL) {
        HDF_LOGE("UartGetAttribute: attribute is null!");
        return HDF_ERR_INVALID_PARAM;
    }

    reply = HdfSbufObtainDefaultSize();
    if (reply == NULL) {
        HDF_LOGE("UartGetAttribute: fail to obtain reply buf!");
        return HDF_ERR_MALLOC_FAIL;
    }

    ret = UartDispatch(handle, UART_IO_GET_ATTRIBUTE, NULL, reply);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("UartGetAttribute: get uart attribute fail, ret: %d!", ret);
        HdfSbufRecycle(reply);
        return HDF_ERR_IO;
    }

    if (!HdfSbufReadBuffer(reply, &tmpBuf, &tmpLen)) {
        HDF_LOGE("UartGetAttribute: sbuf read buffer fail!");
        HdfSbufRecycle(reply);
        return HDF_ERR_IO;
    }

    if (tmpLen != sizeof(*attribute)) {
        HDF_LOGE("UartGetAttribute: reply data len not match, exp:%zu, got:%u!", sizeof(*attribute), tmpLen);
        HdfSbufRecycle(reply);
        return HDF_ERR_IO;
    }

    if (memcpy_s(attribute, sizeof(*attribute), tmpBuf, tmpLen) != EOK) {
        HDF_LOGE("UartGetAttribute: memcpy buf fail!");
        HdfSbufRecycle(reply);
        return HDF_ERR_IO;
    }

    HdfSbufRecycle(reply);
    return HDF_SUCCESS;
}

int32_t UartSetAttribute(DevHandle handle, struct UartAttribute *attribute)
{
    int32_t ret;
    struct HdfSBuf *data = NULL;

    if (attribute == NULL) {
        HDF_LOGE("UartSetAttribute: attribute is null!");
        return HDF_ERR_INVALID_PARAM;
    }

    data = HdfSbufObtainDefaultSize();
    if (data == NULL) {
        HDF_LOGE("UartSetAttribute: fail to obtain data!");
        return HDF_ERR_MALLOC_FAIL;
    }

    if (!HdfSbufWriteBuffer(data, (void *)attribute, sizeof(*attribute))) {
        HDF_LOGE("UartSetAttribute: sbuf write attribute fail!");
        HdfSbufRecycle(data);
        return HDF_ERR_IO;
    }

    ret = UartDispatch(handle, UART_IO_SET_ATTRIBUTE, data, NULL);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("UartSetAttribute: set uart attribute fail, ret: %d!", ret);
    }
    HdfSbufRecycle(data);
    return ret;
}

int32_t UartSetTransMode(DevHandle handle, enum UartTransMode mode)
{
    int32_t ret;
    struct HdfSBuf *data = NULL;

    data = HdfSbufObtainDefaultSize();
    if (data == NULL) {
        HDF_LOGE("UartSetTransMode: fail to obtain data buf!");
        return HDF_ERR_MALLOC_FAIL;
    }

    if (!HdfSbufWriteUint32(data, (uint32_t)mode)) {
        HDF_LOGE("UartSetTransMode: sbuf write mode fail!");
        HdfSbufRecycle(data);
        return HDF_ERR_IO;
    }

    ret = UartDispatch(handle, UART_IO_SET_TRANSMODE, data, NULL);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("UartSetTransMode: set uart trans mode fail, ret: %d!", ret);
    }
    HdfSbufRecycle(data);
    return ret;
}
