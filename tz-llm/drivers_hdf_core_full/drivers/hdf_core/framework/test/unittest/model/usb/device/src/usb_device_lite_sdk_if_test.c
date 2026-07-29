/*
 * Copyright (c) 2021-2022 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include "usb_device_lite_cdcacm_test.h"

#define HDF_LOG_TAG usb_device_sdk_test

struct AcmDevice *g_acmDevice = NULL;
extern struct UsbFnDeviceDesc g_acmFnDevice;

struct AcmDevice *UsbGetAcmDevice(void)
{
    return g_acmDevice;
}

int32_t UsbFnDviceTestCreate(void)
{
    dprintf("%s: start\n", __func__);
    RemoveUsbDevice();
    g_acmDevice = SetUpAcmDevice();
    if (g_acmDevice == NULL || g_acmDevice->fnDev == NULL) {
        HDF_LOGE("%s: UsbFnDviceTestCreate fail", __func__);
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

int32_t UsbFnDviceTestCreate002(void)
{
    struct UsbFnDevice *fnDev = NULL;
    struct UsbFnDescriptorData descData;

    descData.type = USBFN_DESC_DATA_TYPE_DESC;
    descData.descriptor = NULL;
    fnDev = (struct UsbFnDevice *)UsbFnCreateDevice("100e0000.hidwc3_0", &descData);
    if (fnDev != NULL) {
        HDF_LOGE("%s: UsbFnDviceTestCreate success!!", __func__);
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

int32_t UsbFnDviceTestCreate003(void)
{
    struct UsbFnDevice *fnDev = NULL;
    struct UsbFnDescriptorData descData;

    descData.type = USBFN_DESC_DATA_TYPE_PROP;
    descData.property = NULL;
    fnDev = (struct UsbFnDevice *)UsbFnCreateDevice("100e0000.hidwc3_0", &descData);
    if (fnDev != NULL) {
        HDF_LOGE("%s: UsbFnDviceTestCreate success!!", __func__);
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

int32_t UsbFnDviceTestCreate004(void)
{
    struct UsbFnDevice *fnDev = NULL;
    struct UsbFnDescriptorData descData;

    descData.type = USBFN_DESC_DATA_TYPE_DESC;
    descData.descriptor = &g_acmFnDevice;
    fnDev = (struct UsbFnDevice *)UsbFnCreateDevice("100e0000.hidwc3_0", &descData);
    if (fnDev != NULL) {
        HDF_LOGE("%s: UsbFnDviceTestCreate success!!", __func__);
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

int32_t UsbFnDviceTestCreate005(void)
{
    struct UsbFnDevice *fnDev = NULL;
    struct UsbFnDescriptorData descData;

    descData.type = USBFN_DESC_DATA_TYPE_DESC;
    descData.descriptor = &g_acmFnDevice;
    fnDev = (struct UsbFnDevice *)UsbFnCreateDevice("100e0000.hidwc3_1", &descData);
    if (fnDev != NULL) {
        HDF_LOGE("%s: UsbFnDviceTestCreate success!!", __func__);
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

int32_t UsbFnDviceTestCreate006(void)
{
    struct UsbFnDevice *fnDev = NULL;
    struct UsbFnDescriptorData descData;
    const char *udcName = NULL;

    descData.type = USBFN_DESC_DATA_TYPE_DESC;
    descData.descriptor = &g_acmFnDevice;
    fnDev = (struct UsbFnDevice *)UsbFnCreateDevice(udcName, &descData);
    if (fnDev != NULL) {
        HDF_LOGE("%s: UsbFnDviceTestCreate success!!", __func__);
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

int32_t UsbFnDviceTestStatus(void)
{
    if (g_acmDevice == NULL || g_acmDevice->fnDev == NULL) {
        HDF_LOGE("%s: fnDev is invail", __func__);
        return HDF_FAILURE;
    }

    UsbFnDeviceState devState;
    int32_t ret = UsbFnGetDeviceState(g_acmDevice->fnDev, &devState);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: get status error", __func__);
        return HDF_FAILURE;
    }
    if (!(devState >= USBFN_STATE_BIND && devState <= USBFN_STATE_RESUME)) {
        HDF_LOGE("%s: device status error", __func__);
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

int32_t UsbFnDviceTestStatus002(void)
{
    UsbFnDeviceState devState;
    int32_t ret = UsbFnGetDeviceState(NULL, &devState);
    if (ret == HDF_SUCCESS) {
        HDF_LOGE("%s: get status success!!", __func__);
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

int32_t UsbFnDviceTestStatus003(void)
{
    UsbFnDeviceState devState;
    if (g_acmDevice == NULL || g_acmDevice->fnDev == NULL) {
        HDF_LOGE("%s: fnDev is invail", __func__);
        return HDF_FAILURE;
    }
    for (int32_t i = 0; i < TEST_TIMES; i++) {
        int32_t ret = UsbFnGetDeviceState(g_acmDevice->fnDev, &devState);
        if (ret != HDF_SUCCESS) {
            HDF_LOGE("%s: get status error", __func__);
            return HDF_FAILURE;
        }
        if (!(devState >= USBFN_STATE_BIND && devState <= USBFN_STATE_RESUME)) {
            HDF_LOGE("%s: device status error", __func__);
            return HDF_FAILURE;
        }
    }
    return HDF_SUCCESS;
}

int32_t UsbFnDviceTestStatus004(void)
{
    int32_t ret = UsbFnGetDeviceState(NULL, NULL);
    if (ret == HDF_SUCCESS) {
        HDF_LOGE("%s: get status success!!", __func__);
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

int32_t UsbFnDviceTestStatus005(void)
{
    if (g_acmDevice == NULL || g_acmDevice->fnDev == NULL) {
        HDF_LOGE("%s: fnDev is invail", __func__);
        return HDF_FAILURE;
    }

    UsbFnDeviceState *devState = NULL;
    int32_t ret = UsbFnGetDeviceState(g_acmDevice->fnDev, devState);
    if (ret == HDF_SUCCESS) {
        HDF_LOGE("%s: get status success!!", __func__);
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

int32_t UsbFnDviceTestGetDevice(void)
{
    const struct UsbFnDevice *device = NULL;
    const char *udcName = "100e0000.hidwc3_0";
    device = UsbFnGetDevice(udcName);
    if (device == NULL) {
        HDF_LOGE("%s: get device failed", __func__);
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

int32_t UsbFnDviceTestGetDevice002(void)
{
    const struct UsbFnDevice *device = NULL;
    const char *udcName = "100e0000.hidwc3_1";
    device = UsbFnGetDevice(udcName);
    if (device != NULL) {
        HDF_LOGE("%s: get device success!!", __func__);
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

int32_t UsbFnDviceTestGetDevice003(void)
{
    const struct UsbFnDevice *device = NULL;
    const char *udcName = NULL;
    device = UsbFnGetDevice(udcName);
    if (device != NULL) {
        HDF_LOGE("%s: get device success!!", __func__);
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

int32_t UsbFnDviceTestGetDevice004(void)
{
    const struct UsbFnDevice *device = NULL;
    const char *udcName = "100e0000.hidwc3_0 ";
    device = UsbFnGetDevice(udcName);
    if (device != NULL) {
        HDF_LOGE("%s: get device success!!", __func__);
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

int32_t UsbFnDviceTestGetDevice005(void)
{
    const struct UsbFnDevice *device = NULL;
    const char *udcName = "100e0000.hidwc3_0\0100e0000.hidwc3_0";
    device = UsbFnGetDevice(udcName);
    if (device != NULL) {
        HDF_LOGE("%s: get device success!!", __func__);
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

int32_t UsbFnDviceTestGetDevice006(void)
{
    const struct UsbFnDevice *device = NULL;
    const char *udcName = " 100e0000.hidwc3_0";
    device = UsbFnGetDevice(udcName);
    if (device != NULL) {
        HDF_LOGE("%s: get device success!!", __func__);
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

int32_t UsbFnDviceTestGetInterface(void)
{
    if (g_acmDevice == NULL || g_acmDevice->fnDev == NULL) {
        HDF_LOGE("%s: fnDev is invail", __func__);
        return HDF_FAILURE;
    }

    struct UsbFnInterface *fnInterface = (struct UsbFnInterface *)UsbFnGetInterface(g_acmDevice->fnDev, 0);
    if (fnInterface == NULL) {
        HDF_LOGE("%s: get interface failed", __func__);
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

int32_t UsbFnDviceTestGetInterface002(void)
{
    if (g_acmDevice == NULL || g_acmDevice->fnDev == NULL) {
        HDF_LOGE("%s: fnDev is invail", __func__);
        return HDF_FAILURE;
    }

    struct UsbFnInterface *fnInterface = (struct UsbFnInterface *)UsbFnGetInterface(g_acmDevice->fnDev, 1);
    if (fnInterface == NULL) {
        HDF_LOGE("%s: get interface fail", __func__);
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

int32_t UsbFnDviceTestGetInterface003(void)
{
    if (g_acmDevice == NULL || g_acmDevice->fnDev == NULL) {
        HDF_LOGE("%s: fnDev is invail", __func__);
        dprintf("%s, %d\n", __func__, __LINE__);
        return HDF_FAILURE;
    }

    struct UsbFnInterface *fnInterface = (struct UsbFnInterface *)UsbFnGetInterface(g_acmDevice->fnDev, 0xA);
    if (fnInterface != NULL) {
        HDF_LOGE("%s: get interface success!!", __func__);
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

int32_t UsbFnDviceTestGetInterface004(void)
{
    if (g_acmDevice == NULL || g_acmDevice->fnDev == NULL) {
        HDF_LOGE("%s: fnDev is invail", __func__);
        return HDF_FAILURE;
    }

    struct UsbFnInterface *fnInterface = (struct UsbFnInterface *)UsbFnGetInterface(g_acmDevice->fnDev, 0x20);
    if (fnInterface != NULL) {
        HDF_LOGE("%s: get interface success!!", __func__);
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

int32_t UsbFnDviceTestGetInterface005(void)
{
    struct UsbFnDevice *fnDevice = NULL;
    struct UsbFnInterface *fnInterface = (struct UsbFnInterface *)UsbFnGetInterface(fnDevice, 0);
    if (fnInterface != NULL) {
        HDF_LOGE("%s: get interface success!!", __func__);
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

int32_t UsbFnDviceTestGetInterface006(void)
{
    if (g_acmDevice == NULL || g_acmDevice->fnDev == NULL) {
        HDF_LOGE("%s: fnDev is invail", __func__);
        return HDF_FAILURE;
    }

    struct UsbFnInterface *fnInterface = NULL;
    for (int32_t idCount = 0; idCount < 0x2; idCount++) {
        fnInterface = (struct UsbFnInterface *)UsbFnGetInterface(g_acmDevice->fnDev, idCount);
        if (fnInterface == NULL) {
            HDF_LOGE("%s: get interface failed", __func__);
            return HDF_FAILURE;
        }
    }
    return HDF_SUCCESS;
}

int32_t UsbFnDviceTestGetPipeInfo(void)
{
    if (g_acmDevice == NULL || g_acmDevice->dataIface.fn == NULL) {
        HDF_LOGE("%s: dataIface.fn is invail", __func__);
        return HDF_FAILURE;
    }

    struct UsbFnPipeInfo info;
    int32_t ret = UsbFnGetInterfacePipeInfo(g_acmDevice->dataIface.fn, 0, &info);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: get pipe info error", __func__);
        return HDF_FAILURE;
    }
    if (info.id != 0) {
        HDF_LOGE("%s: get pipe id error", __func__);
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

int32_t UsbFnDviceTestGetPipeInfo002(void)
{
    if (g_acmDevice == NULL || g_acmDevice->dataIface.fn == NULL) {
        HDF_LOGE("%s: dataIface.fn is invail", __func__);
        return HDF_FAILURE;
    }

    struct UsbFnPipeInfo info;
    int32_t ret = UsbFnGetInterfacePipeInfo(g_acmDevice->dataIface.fn, 1, &info);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: get pipe info error", __func__);
        return HDF_FAILURE;
    }
    if (info.id != 1) {
        HDF_LOGE("%s: get pipe id error", __func__);
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

int32_t UsbFnDviceTestGetPipeInfo003(void)
{
    if (g_acmDevice == NULL || g_acmDevice->dataIface.fn == NULL) {
        HDF_LOGE("%s: dataIface.fn is invail", __func__);
        return HDF_FAILURE;
    }

    struct UsbFnPipeInfo info;
    int32_t ret = UsbFnGetInterfacePipeInfo(g_acmDevice->dataIface.fn, 0xF, &info);
    if (ret == HDF_SUCCESS) {
        HDF_LOGE("%s: get pipe info success!!", __func__);
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

int32_t UsbFnDviceTestGetPipeInfo004(void)
{
    if (g_acmDevice == NULL || g_acmDevice->dataIface.fn == NULL) {
        HDF_LOGE("%s: dataIface.fn is invail", __func__);
        return HDF_FAILURE;
    }

    struct UsbFnPipeInfo *info = NULL;
    int32_t ret = UsbFnGetInterfacePipeInfo(g_acmDevice->dataIface.fn, 0, info);
    if (ret == HDF_SUCCESS) {
        HDF_LOGE("%s: get pipe info success!!", __func__);
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

int32_t UsbFnDviceTestGetPipeInfo005(void)
{
    struct UsbFnPipeInfo info;
    struct UsbFnInterface *fn = NULL;
    int32_t ret = UsbFnGetInterfacePipeInfo(fn, 0, &info);
    if (ret == HDF_SUCCESS) {
        HDF_LOGE("%s: get pipe info success!!", __func__);
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

int32_t UsbFnDviceTestGetPipeInfo006(void)
{
    if (g_acmDevice == NULL || g_acmDevice->ctrlIface.fn == NULL) {
        HDF_LOGE("%s: dataIface.fn is invail", __func__);
        return HDF_FAILURE;
    }

    struct UsbFnPipeInfo info;
    int32_t ret = UsbFnGetInterfacePipeInfo(g_acmDevice->ctrlIface.fn, 0, &info);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: get pipe info error", __func__);
        return HDF_FAILURE;
    }
    if (info.id != 0) {
        HDF_LOGE("%s: get pipe id error", __func__);
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

static int32_t PropCallBack(const struct UsbFnInterface *intf, const char *name, const char *value)
{
    if (intf == NULL || name == NULL || value == NULL) {
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

int32_t UsbFnDviceTestRegistProp(void)
{
    if (g_acmDevice == NULL || g_acmDevice->ctrlIface.fn == NULL) {
        HDF_LOGE("%s: ctrlIface.handle is invail", __func__);
        return HDF_FAILURE;
    }

    struct UsbFnRegistInfo info = {"name_test", "test_value", PropCallBack, PropCallBack};
    int32_t ret = UsbFnRegistInterfaceProp(g_acmDevice->ctrlIface.fn, &info);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: Regist Prop error", __func__);
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

int32_t UsbFnDviceTestRegistProp002(void)
{
    if (g_acmDevice == NULL || g_acmDevice->ctrlIface.fn == NULL) {
        HDF_LOGE("%s: ctrlIface.handle is invail", __func__);
        return HDF_FAILURE;
    }

    struct UsbFnRegistInfo info = {"name_test", "test_value", PropCallBack, PropCallBack};
    int32_t ret = UsbFnRegistInterfaceProp(g_acmDevice->ctrlIface.fn, &info);
    if (ret == HDF_SUCCESS) {
        HDF_LOGE("%s: Regist Prop success!!", __func__);
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

int32_t UsbFnDviceTestRegistProp003(void)
{
    if (g_acmDevice == NULL || g_acmDevice->ctrlIface.fn == NULL) {
        HDF_LOGE("%s: ctrlIface.handle is invail", __func__);
        return HDF_FAILURE;
    }

    struct UsbFnRegistInfo info = {NULL, "test_value", PropCallBack, PropCallBack};
    int32_t ret = UsbFnRegistInterfaceProp(g_acmDevice->ctrlIface.fn, &info);
    if (ret == HDF_SUCCESS) {
        HDF_LOGE("%s: Regist Prop success!!", __func__);
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

int32_t UsbFnDviceTestRegistProp004(void)
{
    if (g_acmDevice == NULL || g_acmDevice->ctrlIface.fn == NULL) {
        HDF_LOGE("%s: ctrlIface.handle is invail", __func__);
        return HDF_FAILURE;
    }

    struct UsbFnRegistInfo info = {"name_test4", "test_value", NULL, PropCallBack};
    int32_t ret = UsbFnRegistInterfaceProp(g_acmDevice->ctrlIface.fn, &info);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: Regist Prop error", __func__);
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

int32_t UsbFnDviceTestRegistProp005(void)
{
    struct UsbFnInterface *fn = NULL;
    struct UsbFnRegistInfo info = {"name_test5", "test_value", PropCallBack, PropCallBack};
    int32_t ret = UsbFnRegistInterfaceProp(fn, &info);
    if (ret == HDF_SUCCESS) {
        HDF_LOGE("%s: Regist Prop success!!", __func__);
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

int32_t UsbFnDviceTestRegistProp006(void)
{
    if (g_acmDevice == NULL || g_acmDevice->ctrlIface.fn == NULL) {
        HDF_LOGE("%s: ctrlIface.handle is invail", __func__);
        return HDF_FAILURE;
    }

    struct UsbFnRegistInfo *info = NULL;
    int32_t ret = UsbFnRegistInterfaceProp(g_acmDevice->ctrlIface.fn, info);
    if (ret == HDF_SUCCESS) {
        HDF_LOGE("%s: Regist Prop success!!", __func__);
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

int32_t UsbFnDviceTestRegistProp007(void)
{
    if (g_acmDevice == NULL || g_acmDevice->dataIface.fn == NULL) {
        HDF_LOGE("%s: ctrlIface.handle is invail", __func__);
        return HDF_FAILURE;
    }

    struct UsbFnRegistInfo info = {"name_test", "test_value", PropCallBack, PropCallBack};
    int32_t ret = UsbFnRegistInterfaceProp(g_acmDevice->dataIface.fn, &info);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: Regist Prop error", __func__);
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

int32_t UsbFnDviceTestGetProp(void)
{
    if (g_acmDevice == NULL || g_acmDevice->ctrlIface.fn == NULL) {
        HDF_LOGE("%s: ctrlIface.fn is invail", __func__);
        return HDF_FAILURE;
    }

    char buffer[BUFFER_LEN] = {0};
    char *buff = buffer;
    int32_t ret = UsbFnGetInterfaceProp(g_acmDevice->ctrlIface.fn, "name_test", buff);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: Get Prop error", __func__);
        return HDF_FAILURE;
    }
    if (strcmp(buffer, "test_value") != 0) {
        HDF_LOGE("%s: Get Prop value error", __func__);
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

int32_t UsbFnDviceTestGetProp002(void)
{
    if (g_acmDevice == NULL || g_acmDevice->ctrlIface.fn == NULL) {
        HDF_LOGE("%s: ctrlIface.fn is invail", __func__);
        return HDF_FAILURE;
    }

    char buffer[BUFFER_LEN] = {0};
    char *buff = buffer;
    int32_t ret = UsbFnGetInterfaceProp(g_acmDevice->ctrlIface.fn, "unknown", buff);
    if (ret == HDF_SUCCESS) {
        HDF_LOGE("%s: Get Prop success!!", __func__);
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

int32_t UsbFnDviceTestGetProp003(void)
{
    if (g_acmDevice == NULL || g_acmDevice->ctrlIface.fn == NULL) {
        HDF_LOGE("%s: ctrlIface.fn is invail", __func__);
        return HDF_FAILURE;
    }

    char buffer[BUFFER_LEN] = {0};
    char *buff = buffer;
    int32_t ret = UsbFnGetInterfaceProp(g_acmDevice->ctrlIface.fn, "idProduct", buff);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: Get Prop error", __func__);
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

int32_t UsbFnDviceTestGetProp004(void)
{
    char buffer[BUFFER_LEN] = {0};
    char *buff = buffer;
    struct UsbFnInterface *fn = NULL;
    int32_t ret = UsbFnGetInterfaceProp(fn, "idProduct", buff);
    if (ret == HDF_SUCCESS) {
        HDF_LOGE("%s: Get Prop success!!", __func__);
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

int32_t UsbFnDviceTestGetProp005(void)
{
    if (g_acmDevice == NULL || g_acmDevice->ctrlIface.fn == NULL) {
        HDF_LOGE("%s: ctrlIface.fn is invail", __func__);
        return HDF_FAILURE;
    }

    char buffer[BUFFER_LEN] = {0};
    char *buff = buffer;
    int32_t ret = UsbFnGetInterfaceProp(g_acmDevice->ctrlIface.fn, "idVendor", buff);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: Get Prop error", __func__);
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

int32_t UsbFnDviceTestGetProp006(void)
{
    if (g_acmDevice == NULL || g_acmDevice->ctrlIface.fn == NULL) {
        HDF_LOGE("%s: ctrlIface.fn is invail", __func__);
        return HDF_FAILURE;
    }

    int32_t ret = UsbFnGetInterfaceProp(g_acmDevice->ctrlIface.fn, "name_test", NULL);
    if (ret == HDF_SUCCESS) {
        HDF_LOGE("%s: Get Prop success!!", __func__);
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

int32_t UsbFnDviceTestGetProp007(void)
{
    if (g_acmDevice == NULL || g_acmDevice->dataIface.fn == NULL) {
        HDF_LOGE("%s: ctrlIface.fn is invail", __func__);
        return HDF_FAILURE;
    }

    char buffer[BUFFER_LEN] = {0};
    char *buff = buffer;
    int32_t ret = UsbFnGetInterfaceProp(g_acmDevice->dataIface.fn, "name_test", buff);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: Get Prop error", __func__);
        return HDF_FAILURE;
    }
    if (strcmp(buffer, "test_value") != 0) {
        HDF_LOGE("%s: Get Prop value error", __func__);
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

int32_t UsbFnDviceTestGetProp008(void)
{
    if (g_acmDevice == NULL || g_acmDevice->dataIface.fn == NULL) {
        HDF_LOGE("%s: ctrlIface.fn is invail", __func__);
        return HDF_FAILURE;
    }

    char buffer[BUFFER_LEN] = {0};
    char *buff = buffer;
    int32_t ret = UsbFnGetInterfaceProp(g_acmDevice->dataIface.fn, "idVendor", buff);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: Get Prop error", __func__);
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

int32_t UsbFnDviceTestSetProp(void)
{
    if (g_acmDevice == NULL || g_acmDevice->ctrlIface.fn == NULL) {
        HDF_LOGE("%s: ctrlIface.fn is invail", __func__);
        return HDF_FAILURE;
    }

    int32_t ret = UsbFnSetInterfaceProp(g_acmDevice->ctrlIface.fn, "name_test", "hello");
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: Set Prop error", __func__);
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

int32_t UsbFnDviceTestSetProp002(void)
{
    if (g_acmDevice == NULL || g_acmDevice->ctrlIface.fn == NULL) {
        HDF_LOGE("%s: ctrlIface.fn is invail", __func__);
        return HDF_FAILURE;
    }

    int32_t ret = UsbFnSetInterfaceProp(g_acmDevice->ctrlIface.fn, "unknown", "hello");
    if (ret == HDF_SUCCESS) {
        HDF_LOGE("%s: Set Prop success!!", __func__);
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

int32_t UsbFnDviceTestSetProp003(void)
{
    struct UsbFnInterface *fn = NULL;
    int32_t ret = UsbFnSetInterfaceProp(fn, "name_test", "hellotest");
    if (ret == HDF_SUCCESS) {
        HDF_LOGE("%s: Set Prop success!!", __func__);
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

int32_t UsbFnDviceTestSetProp004(void)
{
    if (g_acmDevice == NULL || g_acmDevice->ctrlIface.fn == NULL) {
        HDF_LOGE("%s: ctrlIface.fn is invail", __func__);
        return HDF_FAILURE;
    }

    const char *propName = NULL;
    int32_t ret = UsbFnSetInterfaceProp(g_acmDevice->ctrlIface.fn, propName, "hellotest");
    if (ret == HDF_SUCCESS) {
        HDF_LOGE("%s: Set Prop success!!", __func__);
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

int32_t UsbFnDviceTestSetProp005(void)
{
    if (g_acmDevice == NULL || g_acmDevice->ctrlIface.fn == NULL) {
        HDF_LOGE("%s: ctrlIface.fn is invail", __func__);
        return HDF_FAILURE;
    }

    int32_t ret = UsbFnSetInterfaceProp(g_acmDevice->ctrlIface.fn, "idVendor", "0x625");
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: Set Prop error!!", __func__);
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

int32_t UsbFnDviceTestSetProp006(void)
{
    if (g_acmDevice == NULL || g_acmDevice->ctrlIface.fn == NULL) {
        HDF_LOGE("%s: ctrlIface.fn is invail", __func__);
        return HDF_FAILURE;
    }

    int32_t ret = UsbFnSetInterfaceProp(g_acmDevice->ctrlIface.fn, "bLength", "0x14");
    if (ret == HDF_SUCCESS) {
        HDF_LOGE("%s: Set Prop success!!", __func__);
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

int32_t UsbFnDviceTestSetProp007(void)
{
    if (g_acmDevice == NULL || g_acmDevice->dataIface.fn == NULL) {
        HDF_LOGE("%s: ctrlIface.fn is invail", __func__);
        return HDF_FAILURE;
    }

    int32_t ret = UsbFnSetInterfaceProp(g_acmDevice->dataIface.fn, "name_test", "hello");
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: Set Prop error", __func__);
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

int32_t UsbFnDviceTestAllocCtrlRequest(void)
{
    if (g_acmDevice == NULL || g_acmDevice->dataIface.handle == NULL) {
        HDF_LOGE("%s: dataIface.handle is invail", __func__);
        return HDF_FAILURE;
    }

    struct UsbFnRequest *req =
        UsbFnAllocCtrlRequest(g_acmDevice->dataIface.handle, g_acmDevice->dataOutPipe.maxPacketSize);
    if (req == NULL) {
        HDF_LOGE("%s: alloc req fail", __func__);
        return HDF_FAILURE;
    }

    int32_t ret = UsbFnFreeRequest(req);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: free Request error", __func__);
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

int32_t UsbFnDviceTestAllocCtrlRequest002(void)
{
    if (g_acmDevice == NULL || g_acmDevice->ctrlIface.handle == NULL) {
        HDF_LOGE("%s: dataIface.handle is invail", __func__);
        return HDF_FAILURE;
    }

    struct UsbFnRequest *req = UsbFnAllocCtrlRequest(
        g_acmDevice->ctrlIface.handle, sizeof(struct UsbCdcLineCoding) + sizeof(struct UsbCdcLineCoding));
    if (req == NULL) {
        HDF_LOGE("%s: alloc req fail", __func__);
        return HDF_FAILURE;
    }

    int32_t ret = UsbFnFreeRequest(req);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: free Request error", __func__);
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

int32_t UsbFnDviceTestAllocCtrlRequest003(void)
{
    if (g_acmDevice == NULL || g_acmDevice->ctrlIface.handle == NULL) {
        HDF_LOGE("%s: dataIface.handle is invail", __func__);
        return HDF_FAILURE;
    }

    struct UsbFnRequest *req = UsbFnAllocCtrlRequest(g_acmDevice->ctrlIface.handle, 0);
    if (req == NULL) {
        HDF_LOGE("%s: alloc req failed", __func__);
        return HDF_FAILURE;
    }

    int32_t ret = UsbFnFreeRequest(req);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: FreeRequest failed", __func__);
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

int32_t UsbFnDviceTestAllocCtrlRequest004(void)
{
    if (g_acmDevice == NULL || g_acmDevice->ctrlIface.handle == NULL) {
        HDF_LOGE("%s: dataIface.handle is invail", __func__);
        return HDF_FAILURE;
    }

    // 0x801: Represents the length of the requested data, testing greater than 2048.
    struct UsbFnRequest *req = UsbFnAllocCtrlRequest(g_acmDevice->ctrlIface.handle, 0x801);
    if (req == NULL) {
        HDF_LOGE("%s: alloc req failed", __func__);
        return HDF_FAILURE;
    }

    int32_t ret = UsbFnFreeRequest(req);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: free Request error", __func__);
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

int32_t UsbFnDviceTestAllocCtrlRequest005(void)
{
    if (g_acmDevice == NULL) {
        HDF_LOGE("%s: dataIface.handle is invail", __func__);
        return HDF_FAILURE;
    }

    UsbFnInterfaceHandle handle = NULL;
    struct UsbFnRequest *req = UsbFnAllocCtrlRequest(handle, g_acmDevice->notifyPipe.maxPacketSize);
    if (req == NULL) {
        HDF_LOGE("%s: alloc req failed", __func__);
        return HDF_FAILURE;
    }

    int32_t ret = UsbFnFreeRequest(req);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: free Request error", __func__);
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

int32_t UsbFnDviceTestAllocCtrlRequest006(void)
{
    if (g_acmDevice == NULL || g_acmDevice->ctrlIface.handle == NULL) {
        HDF_LOGE("%s: dataIface.handle is invail", __func__);
        return HDF_FAILURE;
    }

    // 0x800: Represents the length of the requested data, testing equal to 2048.
    struct UsbFnRequest *req = UsbFnAllocCtrlRequest(g_acmDevice->ctrlIface.handle, 0x800);
    if (req == NULL) {
        HDF_LOGE("%s: alloc req fail", __func__);
        return HDF_FAILURE;
    }

    int32_t ret = UsbFnFreeRequest(req);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: free Request error", __func__);
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

int32_t UsbFnDviceTestAllocCtrlRequest007(void)
{
    if (g_acmDevice == NULL || g_acmDevice->dataIface.handle == NULL) {
        HDF_LOGE("%s: dataIface.handle is invail", __func__);
        return HDF_FAILURE;
    }

    struct UsbFnRequest *req = UsbFnAllocCtrlRequest(g_acmDevice->dataIface.handle, 0);
    if (req == NULL) {
        HDF_LOGE("%s: alloc req failed", __func__);
        return HDF_FAILURE;
    }

    int32_t ret = UsbFnFreeRequest(req);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: free Request error", __func__);
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

int32_t UsbFnDviceTestAllocCtrlRequest008(void)
{
    UsbFnInterfaceHandle handle = NULL;
    struct UsbFnRequest *req = UsbFnAllocCtrlRequest(handle, 0);
    if (req == NULL) {
        HDF_LOGE("%s: alloc req failed", __func__);
        return HDF_FAILURE;
    }

    int32_t ret = UsbFnFreeRequest(req);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: free Request error", __func__);
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

int32_t UsbFnDviceTestAllocRequest(void)
{
    if (g_acmDevice == NULL || g_acmDevice->ctrlIface.handle == NULL) {
        HDF_LOGE("%s: ctrlIface.handle is invail", __func__);
        return HDF_FAILURE;
    }

    struct UsbFnRequest *req =
        UsbFnAllocRequest(g_acmDevice->ctrlIface.handle, g_acmDevice->notifyPipe.id, sizeof(struct UsbCdcNotification));
    if (req == NULL) {
        HDF_LOGE("%s: alloc req fail", __func__);
        return HDF_FAILURE;
    }

    int32_t ret = UsbFnFreeRequest(req);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: free Request error", __func__);
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

int32_t UsbFnDviceTestAllocRequest002(void)
{
    if (g_acmDevice == NULL || g_acmDevice->ctrlIface.handle == NULL) {
        HDF_LOGE("%s: ctrlIface.handle is invail", __func__);
        return HDF_FAILURE;
    }

    struct UsbFnRequest *req = UsbFnAllocRequest(g_acmDevice->ctrlIface.handle, g_acmDevice->notifyPipe.id, 0);
    if (req == NULL) {
        HDF_LOGE("%s: alloc req failed", __func__);
        return HDF_FAILURE;
    }
    int32_t ret = UsbFnFreeRequest(req);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: free Request error", __func__);
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

int32_t UsbFnDviceTestAllocRequest003(void)
{
    if (g_acmDevice == NULL || g_acmDevice->ctrlIface.handle == NULL) {
        HDF_LOGE("%s: ctrlIface.handle is invail", __func__);
        return HDF_FAILURE;
    }

    struct UsbFnRequest *req = UsbFnAllocRequest(g_acmDevice->ctrlIface.handle, g_acmDevice->notifyPipe.id, 0x800);
    if (req == NULL) {
        HDF_LOGE("%s: alloc req fail", __func__);
        return HDF_FAILURE;
    }
    int32_t ret = UsbFnFreeRequest(req);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: free Request error", __func__);
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

int32_t UsbFnDviceTestAllocRequest004(void)
{
    if (g_acmDevice == NULL) {
        HDF_LOGE("%s: dataIface.handle is invail", __func__);
        return HDF_FAILURE;
    }

    UsbFnInterfaceHandle handle = NULL;
    struct UsbFnRequest *req = UsbFnAllocRequest(handle, g_acmDevice->notifyPipe.id, 0x800);
    if (req == NULL) {
        HDF_LOGE("%s: alloc req failed", __func__);
        return HDF_FAILURE;
    }

    int32_t ret = UsbFnFreeRequest(req);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: free Request error", __func__);
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

int32_t UsbFnDviceTestAllocRequest005(void)
{
    if (g_acmDevice == NULL || g_acmDevice->ctrlIface.handle == NULL) {
        HDF_LOGE("%s: ctrlIface.handle is invail", __func__);
        return HDF_FAILURE;
    }

    struct UsbFnRequest *req =
        UsbFnAllocRequest(g_acmDevice->ctrlIface.handle, REQUEST_ALLOC_PIPE, REQUEST_ALLOC_LENGTH);
    if (req == NULL) {
        HDF_LOGE("%s: alloc req failed", __func__);
        return HDF_FAILURE;
    }

    int32_t ret = UsbFnFreeRequest(req);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: free Request error", __func__);
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

int32_t UsbFnDviceTestAllocRequest006(void)
{
    if (g_acmDevice == NULL || g_acmDevice->ctrlIface.handle == NULL) {
        HDF_LOGE("%s: ctrlIface.handle is invail", __func__);
        return HDF_FAILURE;
    }

    struct UsbFnRequest *req = UsbFnAllocRequest(g_acmDevice->ctrlIface.handle, g_acmDevice->notifyPipe.id, 0x801);
    if (req == NULL) {
        HDF_LOGE("%s: alloc req failed", __func__);
        return HDF_FAILURE;
    }

    int32_t ret = UsbFnFreeRequest(req);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: free Request error", __func__);
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

int32_t UsbFnDviceTestAllocRequest007(void)
{
    if (g_acmDevice == NULL || g_acmDevice->dataIface.handle == NULL) {
        HDF_LOGE("%s: ctrlIface.handle is invail", __func__);
        return HDF_FAILURE;
    }

    struct UsbFnRequest *req = UsbFnAllocRequest(g_acmDevice->dataIface.handle, g_acmDevice->dataOutPipe.id, 0);
    if (req == NULL) {
        HDF_LOGE("%s: alloc req failed", __func__);
        return HDF_FAILURE;
    }

    int32_t ret = UsbFnFreeRequest(req);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: free Request error", __func__);
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

int32_t UsbFnDviceTestAllocRequest008(void)
{
    if (g_acmDevice == NULL || g_acmDevice->dataIface.handle == NULL) {
        HDF_LOGE("%s: ctrlIface.handle is invail", __func__);
        return HDF_FAILURE;
    }

    uint32_t length = g_acmDevice->dataOutPipe.maxPacketSize;
    struct UsbFnRequest *req = UsbFnAllocRequest(g_acmDevice->dataIface.handle, REQUEST_ALLOC_PIPE, length);
    if (req == NULL) {
        HDF_LOGE("%s: alloc req failed", __func__);
        return HDF_FAILURE;
    }

    int32_t ret = UsbFnFreeRequest(req);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: free Request error", __func__);
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

int32_t UsbFnDviceTestAllocRequest009(void)
{
    if (g_acmDevice == NULL || g_acmDevice->dataIface.handle == NULL) {
        HDF_LOGE("%s: ctrlIface.handle is invail", __func__);
        return HDF_FAILURE;
    }

    struct UsbFnRequest *req = UsbFnAllocRequest(
        g_acmDevice->dataIface.handle, g_acmDevice->dataOutPipe.id, g_acmDevice->dataOutPipe.maxPacketSize);
    if (req == NULL) {
        HDF_LOGE("%s: alloc req failed", __func__);
        return HDF_FAILURE;
    }

    int32_t ret = UsbFnFreeRequest(req);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: free Request error", __func__);
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

int32_t UsbFnDviceTestFreeRequest(void)
{
    if (g_acmDevice == NULL || g_acmDevice->ctrlIface.handle == NULL) {
        HDF_LOGE("%s: ctrlIface.handle is invail", __func__);
        return HDF_FAILURE;
    }

    struct UsbFnRequest *req = UsbFnAllocRequest(
        g_acmDevice->ctrlIface.handle, g_acmDevice->notifyPipe.id, g_acmDevice->notifyPipe.maxPacketSize);
    if (req == NULL) {
        HDF_LOGE("%s: alloc req fail", __func__);
        return HDF_FAILURE;
    }

    int32_t ret = UsbFnFreeRequest(req);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: free Request error", __func__);
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

int32_t UsbFnDviceTestFreeRequest002(void)
{
    if (g_acmDevice == NULL || g_acmDevice->dataIface.handle == NULL) {
        HDF_LOGE("%s: ctrlIface.handle is invail", __func__);
        return HDF_FAILURE;
    }

    struct UsbFnRequest *req = UsbFnAllocRequest(
        g_acmDevice->dataIface.handle, g_acmDevice->dataInPipe.id, g_acmDevice->dataInPipe.maxPacketSize);
    if (req == NULL) {
        HDF_LOGE("%s: alloc req fail", __func__);
        return HDF_FAILURE;
    }

    int32_t ret = UsbFnFreeRequest(req);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: free Request error", __func__);
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

int32_t UsbFnDviceTestFreeRequest003(void)
{
    if (g_acmDevice == NULL || g_acmDevice->dataIface.handle == NULL) {
        HDF_LOGE("%s: ctrlIface.handle is invail", __func__);
        return HDF_FAILURE;
    }

    struct UsbFnRequest *req = UsbFnAllocRequest(
        g_acmDevice->dataIface.handle, g_acmDevice->dataOutPipe.id, g_acmDevice->dataOutPipe.maxPacketSize);
    if (req == NULL) {
        HDF_LOGE("%s: alloc req fail", __func__);
        return HDF_FAILURE;
    }

    int32_t ret = UsbFnFreeRequest(req);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: free Request error", __func__);
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

int32_t UsbFnDviceTestFreeRequest004(void)
{
    if (g_acmDevice == NULL || g_acmDevice->ctrlIface.handle == NULL) {
        HDF_LOGE("%s: ctrlIface.handle is invail", __func__);
        return HDF_FAILURE;
    }

    struct UsbFnRequest *req =
        UsbFnAllocCtrlRequest(g_acmDevice->ctrlIface.handle, g_acmDevice->notifyPipe.maxPacketSize);
    if (req == NULL) {
        HDF_LOGE("%s: alloc req fail", __func__);
        return HDF_FAILURE;
    }

    int32_t ret = UsbFnFreeRequest(req);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: free Request error", __func__);
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

int32_t UsbFnDviceTestFreeRequest005(void)
{
    struct UsbFnRequest *req = NULL;
    if (g_acmDevice == NULL || g_acmDevice->ctrlIface.handle == NULL) {
        HDF_LOGE("%s: ctrlIface.handle is invail", __func__);
        return HDF_FAILURE;
    }
    for (int32_t i = 0; i < TEST_TIMES; i++) {
        req = UsbFnAllocCtrlRequest(g_acmDevice->ctrlIface.handle, g_acmDevice->notifyPipe.maxPacketSize);
        if (req == NULL) {
            HDF_LOGE("%s: alloc req fail", __func__);
            return HDF_FAILURE;
        }
        int32_t ret = UsbFnFreeRequest(req);
        if (ret != HDF_SUCCESS) {
            HDF_LOGE("%s: free Request error", __func__);
            return HDF_FAILURE;
        }
    }
    return HDF_SUCCESS;
}

int32_t UsbFnDviceTestFreeRequest006(void)
{
    struct UsbFnRequest *req = NULL;
    int32_t ret = UsbFnFreeRequest(req);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: free Request failed", __func__);
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

int32_t UsbFnDviceTestGetRequestStatus(void)
{
    if (g_acmDevice == NULL || g_acmDevice->ctrlIface.handle == NULL) {
        HDF_LOGE("%s: ctrlIface.handle is invail", __func__);
        return HDF_FAILURE;
    }

    struct UsbFnRequest *notifyReq =
        UsbFnAllocRequest(g_acmDevice->ctrlIface.handle, g_acmDevice->notifyPipe.id, sizeof(struct UsbCdcNotification));
    if (notifyReq == NULL) {
        HDF_LOGE("%s: alloc req fail", __func__);
        return HDF_FAILURE;
    }

    UsbRequestStatus status = 0;
    int32_t ret = UsbFnGetRequestStatus(notifyReq, &status);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: get status error", __func__);
        return HDF_FAILURE;
    }
    if (!(status >= USB_REQUEST_COMPLETED && status <= USB_REQUEST_OVERFLOW)) {
        HDF_LOGE("%s: device status error", __func__);
        return HDF_FAILURE;
    }
    ret = UsbFnFreeRequest(notifyReq);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: free Request error", __func__);
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

int32_t UsbFnDviceTestGetRequestStatus002(void)
{
    if (g_acmDevice == NULL || g_acmDevice->ctrlIface.handle == NULL) {
        HDF_LOGE("%s: ctrlIface.handle is invail", __func__);
        return HDF_FAILURE;
    }

    struct UsbFnRequest *notifyReq =
        UsbFnAllocCtrlRequest(g_acmDevice->ctrlIface.handle, sizeof(struct UsbCdcNotification));
    if (notifyReq == NULL) {
        HDF_LOGE("%s: alloc req fail", __func__);
        return HDF_FAILURE;
    }

    UsbRequestStatus status = 0;
    int32_t ret = UsbFnGetRequestStatus(notifyReq, &status);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: get status error", __func__);
        dprintf("%s: get status error", __func__);
        return HDF_FAILURE;
    }
    if (!(status >= USB_REQUEST_COMPLETED && status <= USB_REQUEST_OVERFLOW)) {
        HDF_LOGE("%s: device status error", __func__);
        return HDF_FAILURE;
    }
    ret = UsbFnFreeRequest(notifyReq);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: free Request error", __func__);
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

int32_t UsbFnDviceTestGetRequestStatus003(void)
{
    if (g_acmDevice == NULL || g_acmDevice->dataIface.handle == NULL) {
        HDF_LOGE("%s: ctrlIface.handle is invail", __func__);
        return HDF_FAILURE;
    }

    struct UsbFnRequest *req = UsbFnAllocRequest(
        g_acmDevice->dataIface.handle, g_acmDevice->dataOutPipe.id, g_acmDevice->dataOutPipe.maxPacketSize);
    if (req == NULL) {
        HDF_LOGE("%s: alloc req fail", __func__);
        return HDF_FAILURE;
    }

    int32_t ret = UsbFnGetRequestStatus(req, NULL);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: get status failed", __func__);
        ret = UsbFnFreeRequest(req);
        if (ret != HDF_SUCCESS) {
            HDF_LOGE("%s: free Request error", __func__);
            return HDF_FAILURE;
        }
    }

    return HDF_SUCCESS;
}

int32_t UsbFnDviceTestGetRequestStatus004(void)
{
    if (g_acmDevice == NULL || g_acmDevice->dataIface.handle == NULL) {
        HDF_LOGE("%s: ctrlIface.handle is invail", __func__);
        return HDF_FAILURE;
    }

    struct UsbFnRequest *notifyReq = UsbFnAllocRequest(
        g_acmDevice->dataIface.handle, g_acmDevice->dataOutPipe.id, g_acmDevice->dataOutPipe.maxPacketSize);
    if (notifyReq == NULL) {
        HDF_LOGE("%s: alloc req fail", __func__);
        return HDF_FAILURE;
    }

    UsbRequestStatus status = 0;
    int32_t ret = UsbFnSubmitRequestAsync(notifyReq);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: get status error", __func__);
        return HDF_FAILURE;
    }
    ret = UsbFnGetRequestStatus(notifyReq, &status);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: get status error", __func__);
        return HDF_FAILURE;
    }
    if (!(status >= USB_REQUEST_COMPLETED && status <= USB_REQUEST_OVERFLOW)) {
        HDF_LOGE("%s: device status error", __func__);
        return HDF_FAILURE;
    }
    ret = UsbFnFreeRequest(notifyReq);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: free Request error", __func__);
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

int32_t UsbFnDviceTestGetRequestStatus005(void)
{
    UsbRequestStatus status = 0;
    struct UsbFnRequest *notifyReq = NULL;
    int32_t ret = UsbFnGetRequestStatus(notifyReq, &status);
    if (ret == HDF_SUCCESS) {
        HDF_LOGE("%s: get status success!!", __func__);
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

int32_t UsbFnDviceTestGetRequestStatus006(void)
{
    UsbRequestStatus *status = NULL;
    struct UsbFnRequest *notifyReq = NULL;
    int32_t ret = UsbFnGetRequestStatus(notifyReq, status);
    if (ret == HDF_SUCCESS) {
        HDF_LOGE("%s: get status success!!", __func__);
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

int32_t UsbFnDviceTestStopReceEvent(void)
{
    if (g_acmDevice == NULL || g_acmDevice->ctrlIface.fn == NULL) {
        HDF_LOGE("%s: ctrlIface.handle is invail", __func__);
        return HDF_FAILURE;
    }

    int32_t ret = UsbFnStopRecvInterfaceEvent(g_acmDevice->ctrlIface.fn);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: stop receive event error", __func__);
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

int32_t UsbFnDviceTestStopReceEvent002(void)
{
    struct UsbFnInterface *fn = NULL;
    int32_t ret = UsbFnStopRecvInterfaceEvent(fn);
    if (ret == HDF_SUCCESS) {
        HDF_LOGE("%s: stop receive event success!!", __func__);
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

int32_t UsbFnDviceTestStopReceEvent003(void)
{
    if (g_acmDevice == NULL || g_acmDevice->ctrlIface.fn == NULL) {
        HDF_LOGE("%s: ctrlIface.handle is invail", __func__);
        return HDF_FAILURE;
    }

    int32_t ret = UsbFnStopRecvInterfaceEvent(g_acmDevice->ctrlIface.fn);
    if (ret == HDF_SUCCESS) {
        HDF_LOGE("%s: stop receive event success!!", __func__);
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

int32_t UsbFnDviceTestStopReceEvent004(void)
{
    if (g_acmDevice == NULL || g_acmDevice->dataIface.fn == NULL) {
        HDF_LOGE("%s: ctrlIface.handle is invail", __func__);
        return HDF_FAILURE;
    }

    int32_t ret = UsbFnStopRecvInterfaceEvent(g_acmDevice->dataIface.fn);
    if (ret == HDF_SUCCESS) {
        HDF_LOGE("%s: stop receive event success!!", __func__);
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

static void EventCallBack(struct UsbFnEvent *event)
{
    (void)event;
}

int32_t UsbFnDviceTestStartReceEvent(void)
{
    if (g_acmDevice == NULL || g_acmDevice->ctrlIface.fn == NULL) {
        HDF_LOGE("%s: ctrlIface.handle is invail", __func__);
        return HDF_FAILURE;
    }

    // 0xff: Represents the type of event to handle and can receive all events.
    int32_t ret = UsbFnStartRecvInterfaceEvent(g_acmDevice->ctrlIface.fn, 0xff, NULL, g_acmDevice);
    if (ret == HDF_SUCCESS) {
        HDF_LOGE("%s: start receive event success!!", __func__);
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

int32_t UsbFnDviceTestStartReceEvent002(void)
{
    if (g_acmDevice == NULL || g_acmDevice->ctrlIface.fn == NULL) {
        HDF_LOGE("%s: ctrlIface.handle is invail", __func__);
        return HDF_FAILURE;
    }

    int32_t ret = UsbFnStartRecvInterfaceEvent(g_acmDevice->ctrlIface.fn, 0x0, EventCallBack, g_acmDevice);
    if (ret == HDF_SUCCESS) {
        HDF_LOGE("%s: start receive event success!!", __func__);
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

int32_t UsbFnDviceTestStartReceEvent003(void)
{
    if (g_acmDevice == NULL) {
        HDF_LOGE("%s: dataIface.handle is invail", __func__);
        return HDF_FAILURE;
    }

    struct UsbFnInterface *fn = NULL;
    int32_t ret = UsbFnStartRecvInterfaceEvent(fn, 0xff, EventCallBack, g_acmDevice);
    if (ret == HDF_SUCCESS) {
        HDF_LOGE("%s: start receive event success!!", __func__);
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

int32_t UsbFnDviceTestStartReceEvent004(void)
{
    if (g_acmDevice == NULL || g_acmDevice->ctrlIface.fn == NULL) {
        HDF_LOGE("%s: ctrlIface.handle is invail", __func__);
        return HDF_FAILURE;
    }

    int32_t ret = UsbFnStartRecvInterfaceEvent(g_acmDevice->ctrlIface.fn, 0xff, EventCallBack, g_acmDevice);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: start receive event error", __func__);
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

int32_t UsbFnDviceTestStartReceEvent005(void)
{
    if (g_acmDevice == NULL || g_acmDevice->ctrlIface.fn == NULL) {
        HDF_LOGE("%s: ctrlIface.handle is invail", __func__);
        return HDF_FAILURE;
    }
    for (int32_t i = 0; i < TEST_TIMES; i++) {
        int32_t ret = UsbFnStopRecvInterfaceEvent(g_acmDevice->ctrlIface.fn);
        if (ret != HDF_SUCCESS) {
            HDF_LOGE("%s: stop receive event error", __func__);
            return HDF_FAILURE;
        }
        ret = UsbFnStartRecvInterfaceEvent(g_acmDevice->ctrlIface.fn, 0xff, EventCallBack, g_acmDevice);
        if (ret != HDF_SUCCESS) {
            HDF_LOGE("%s: stop receive event error", __func__);
            return HDF_FAILURE;
        }
    }
    return HDF_SUCCESS;
}

int32_t UsbFnDviceTestStartReceEvent006(void)
{
    if (g_acmDevice == NULL || g_acmDevice->ctrlIface.fn == NULL) {
        HDF_LOGE("%s: ctrlIface.handle is invail", __func__);
        return HDF_FAILURE;
    }

    int32_t ret = UsbFnStartRecvInterfaceEvent(g_acmDevice->ctrlIface.fn, 0xff, EventCallBack, g_acmDevice);
    if (ret == HDF_SUCCESS) {
        HDF_LOGE("%s: start receive event success!!", __func__);
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

int32_t UsbFnDviceTestStartReceEvent007(void)
{
    if (g_acmDevice == NULL || g_acmDevice->dataIface.fn == NULL) {
        HDF_LOGE("%s: ctrlIface.handle is invail", __func__);
        return HDF_FAILURE;
    }

    int32_t ret = UsbFnStartRecvInterfaceEvent(g_acmDevice->dataIface.fn, 0xff, EventCallBack, g_acmDevice);
    if (ret == HDF_SUCCESS) {
        HDF_LOGE("%s: start receive event success!!", __func__);
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

int32_t UsbFnDviceTestOpenInterface(void)
{
    if (g_acmDevice == NULL || g_acmDevice->ctrlIface.fn == NULL) {
        HDF_LOGE("%s: ctrlIface.fn is invail", __func__);
        return HDF_FAILURE;
    }
    if (g_acmDevice->ctrlIface.handle == NULL) {
        HDF_LOGE("%s: ctrlIface.handle is invail", __func__);
        return HDF_FAILURE;
    }

    UsbFnInterfaceHandle handle = UsbFnOpenInterface(g_acmDevice->ctrlIface.fn);
    if (handle == NULL) {
        HDF_LOGE("%s: open interface failed", __func__);
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

int32_t UsbFnDviceTestOpenInterface002(void)
{
    if (g_acmDevice == NULL || g_acmDevice->dataIface.fn == NULL) {
        HDF_LOGE("%s: ctrlIface.handle is invail", __func__);
        return HDF_FAILURE;
    }
    if (g_acmDevice->dataIface.handle == NULL) {
        HDF_LOGE("%s: ctrlIface.handle is invail", __func__);
        return HDF_FAILURE;
    }

    UsbFnInterfaceHandle handle = UsbFnOpenInterface(g_acmDevice->dataIface.fn);
    if (handle != NULL) {
        HDF_LOGE("%s: open interface success!!", __func__);
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

int32_t UsbFnDviceTestOpenInterface003(void)
{
    if (g_acmDevice == NULL || g_acmDevice->ctrlIface.fn == NULL) {
        HDF_LOGE("%s: ctrlIface.fn is invail", __func__);
        return HDF_FAILURE;
    }
    if (g_acmDevice->ctrlIface.handle == NULL) {
        HDF_LOGE("%s: ctrlIface.handle is invail", __func__);
        return HDF_FAILURE;
    }

    int32_t ret = UsbFnCloseInterface(g_acmDevice->ctrlIface.handle);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: close interface failed", __func__);
        return HDF_FAILURE;
    }
    g_acmDevice->ctrlIface.handle = NULL;
    UsbFnInterfaceHandle handle = UsbFnOpenInterface(g_acmDevice->ctrlIface.fn);
    if (handle == NULL) {
        HDF_LOGE("%s: open interface failed", __func__);
        return HDF_FAILURE;
    }
    g_acmDevice->ctrlIface.handle = handle;
    return HDF_SUCCESS;
}

int32_t UsbFnDviceTestOpenInterface004(void)
{
    if (g_acmDevice == NULL || g_acmDevice->dataIface.fn == NULL) {
        HDF_LOGE("%s: dataIface.handle is invail", __func__);
        return HDF_FAILURE;
    }
    if (g_acmDevice->dataIface.handle == NULL) {
        HDF_LOGE("%s: dataIface.handle is invail", __func__);
        return HDF_FAILURE;
    }

    int32_t ret = UsbFnCloseInterface(g_acmDevice->dataIface.handle);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: close interface failed", __func__);
        return HDF_FAILURE;
    }
    g_acmDevice->dataIface.handle = NULL;
    UsbFnInterfaceHandle handle = UsbFnOpenInterface(g_acmDevice->dataIface.fn);
    if (handle == NULL) {
        HDF_LOGE("%s: open interface failed", __func__);
        return HDF_FAILURE;
    }
    g_acmDevice->dataIface.handle = handle;
    return HDF_SUCCESS;
}

int32_t UsbFnDviceTestOpenInterface005(void)
{
    struct UsbFnInterface *fn = NULL;
    UsbFnInterfaceHandle handle = UsbFnOpenInterface(fn);
    if (handle != NULL) {
        HDF_LOGE("%s: open interface success!!", __func__);
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

int32_t UsbFnDviceTestCloseInterface(void)
{
    if (g_acmDevice == NULL || g_acmDevice->ctrlIface.handle == NULL) {
        HDF_LOGE("%s: ctrlIface.handle is invail", __func__);
        return HDF_FAILURE;
    }

    int32_t ret = UsbFnCloseInterface(g_acmDevice->ctrlIface.handle);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: close interface error", __func__);
        return HDF_FAILURE;
    }
    g_acmDevice->ctrlIface.handle = NULL;
    return HDF_SUCCESS;
}

int32_t UsbFnDviceTestCloseInterface002(void)
{
    if (g_acmDevice == NULL || g_acmDevice->dataIface.handle == NULL) {
        HDF_LOGE("%s: ctrlIface.handle is invail", __func__);
        return HDF_FAILURE;
    }

    int32_t ret = UsbFnCloseInterface(g_acmDevice->dataIface.handle);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: close interface error", __func__);
        return HDF_FAILURE;
    }
    g_acmDevice->dataIface.handle = NULL;
    return HDF_SUCCESS;
}

int32_t UsbFnDviceTestCloseInterface003(void)
{
    if (g_acmDevice == NULL) {
        HDF_LOGE("%s: dataIface.handle is invail", __func__);
        return HDF_FAILURE;
    }

    int32_t ret = UsbFnCloseInterface(g_acmDevice->ctrlIface.handle);
    if (ret == HDF_SUCCESS) {
        HDF_LOGE("%s: close interface success!!", __func__);
        return HDF_FAILURE;
    }
    g_acmDevice->ctrlIface.handle = NULL;
    return HDF_SUCCESS;
}

int32_t UsbFnDviceTestCloseInterface004(void)
{
    if (g_acmDevice == NULL) {
        HDF_LOGE("%s: dataIface.handle is invail", __func__);
        return HDF_FAILURE;
    }

    int32_t ret = UsbFnCloseInterface(g_acmDevice->dataIface.handle);
    if (ret == HDF_SUCCESS) {
        HDF_LOGE("%s: close interface success!!", __func__);
        return HDF_FAILURE;
    }
    g_acmDevice->dataIface.handle = NULL;
    return HDF_SUCCESS;
}

int32_t UsbFnDviceTestRemove(void)
{
    struct UsbFnDevice *fnDev = NULL;
    int32_t ret = UsbFnRemoveDevice(fnDev);
    if (ret == HDF_SUCCESS) {
        HDF_LOGE("%s: UsbFnRemoveDevice success!!", __func__);
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

int32_t UsbFnDviceTestRemove002(void)
{
    if (g_acmDevice == NULL || g_acmDevice->fnDev == NULL) {
        HDF_LOGE("%s: fndev is null", __func__);
        return HDF_FAILURE;
    }

    ReleaseAcmDevice(g_acmDevice);
    int32_t ret = UsbFnRemoveDevice(g_acmDevice->fnDev);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: UsbFnRemoveDevice failed, ret = %d", __func__, ret);
        return HDF_FAILURE;
    }
    OsalMemFree(g_acmDevice);
    return HDF_SUCCESS;
}
