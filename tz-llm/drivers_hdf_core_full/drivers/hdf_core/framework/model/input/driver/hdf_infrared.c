/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include "hdf_infrared.h"
#include "hdf_device_desc.h"
#include "input-event-codes.h"
#include "osal_mem.h"
#include "gpio_if.h"
#include "hdf_log.h"
#include "hdf_input_device_manager.h"
#include "event_hub.h"

#define AVERAGE_ERROR 20
#define PILOT_CODE 180
#define LOGIC_1 75
#define LOGIC_0 28
#define MAX_DATA_LEN 32
#define MAX_TIME_LIMIT 250

/* {key value, key input event} */
static struct InfraredKey g_infraredKeyTable[] = {
    {0xA2, KEY_CHANNELDOWN},
    {0x62, KEY_CHANNEL},
    {0xE2, KEY_CHANNELUP},
    {0x22, KEY_PREVIOUSSONG},
    {0x02, KEY_NEXTSONG},
    {0xC2, KEY_PLAYPAUSE},
    {0xE0, KEY_VOLUMEDOWN},
    {0xA8, KEY_VOLUMEUP},
    {0x90, KEY_EQUAL},
    {0x68, KEY_0},
    {0x30, KEY_1},
    {0x18, KEY_2},
    {0x7A, KEY_3},
    {0x10, KEY_4},
    {0x38, KEY_5},
    {0x5A, KEY_6},
    {0x42, KEY_7},
    {0x4A, KEY_8},
    {0x52, KEY_9},
    {0x98, BTN_0},
    {0xB0, BTN_1},
};

static uint16_t TimeCounter(uint16_t intGpioNum)
{
    uint16_t counter = 0;
    uint16_t gpioValue = 0;
    int32_t ret = GpioRead(intGpioNum, &gpioValue);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: gpio read failed, ret %d", __func__, ret);
        return HDF_FAILURE;
    }

    while (gpioValue == GPIO_VAL_HIGH) {
        counter++;
        OsalUDelay(20); // delay 20us
        if (counter >= MAX_TIME_LIMIT) {
            return counter;
        }
        ret = GpioRead(intGpioNum, &gpioValue);
        if (ret != HDF_SUCCESS) {
            HDF_LOGE("%s: gpio read failed, ret %d", __func__, ret);
            return HDF_FAILURE;
        }
    }
    return counter;
}

static void RecvDataHandle(InfraredDriver *infraredDrv, uint32_t data)
{
    if (((data & 0xFF00) >> 8) + (data & 0xFF) != 0xFF) { // 8 bit
        return;
    }

    uint32_t i;
    for (i = 0; i < (sizeof(g_infraredKeyTable) / sizeof(g_infraredKeyTable[0])); ++i) {
        if (g_infraredKeyTable[i].value == ((data & 0xFF00) >> 8)) { // 8 bit
            input_report_key(infraredDrv->inputDev, g_infraredKeyTable[i].infraredCode, 1);
            break;
        }
    }
    input_report_key(infraredDrv->inputDev, g_infraredKeyTable[i].infraredCode, 0);
    input_sync(infraredDrv->inputDev);
}

static void EventHandle(InfraredDriver *infraredDrv)
{
    int32_t ret;
    uint8_t dataBit;
    uint16_t dataLen = 0;
    uint16_t recvFlag = 0;
    uint16_t gpioValue = 0;
    uint16_t counter = 0;
    uint32_t recvData = 0;
    while (1) {
        ret = GpioRead(infraredDrv->infraredCfg->gpioNum, &gpioValue);
        if (ret != HDF_SUCCESS) {
            HDF_LOGE("%s: gpio read failed, ret %d", __func__, ret);
            return;
        }

        if (gpioValue == GPIO_VAL_HIGH) {
            counter = TimeCounter(infraredDrv->infraredCfg->gpioNum);
            if (counter >= MAX_TIME_LIMIT) {
                break;
            }
            if ((counter >=  PILOT_CODE - AVERAGE_ERROR) && (counter < MAX_TIME_LIMIT)) {
                recvFlag = 1;
            } else if ((counter >= LOGIC_1 - AVERAGE_ERROR) && (counter < LOGIC_1 + AVERAGE_ERROR)) {
                dataBit = 1;
            } else if ((counter >= LOGIC_0 - AVERAGE_ERROR) && (counter < LOGIC_0 + AVERAGE_ERROR)) {
                dataBit = 0;
            } else {
                recvData = 0;
                recvFlag = 0;
                dataBit = 0;
                counter = 0;
                dataLen = 0;
            }

            if (recvFlag == 1) {
                recvData <<= 1;
                recvData += dataBit;
                if (dataLen >= MAX_DATA_LEN) {
                    RecvDataHandle(infraredDrv, recvData);
                    recvData = 0;
                    recvFlag = 0;
                    dataLen = 0;
                    dataBit = 0;
                    counter = 0;
                    break;
                }
                dataLen++;
            }
        }
    }
}

int32_t InfraredIrqHandle(uint16_t intGpioNum, void *data)
{
    int32_t ret;
    InfraredDriver *driver = (InfraredDriver *)data;
    if (driver == NULL) {
        return HDF_FAILURE;
    }

    ret = GpioDisableIrq(intGpioNum);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: disable irq failed, ret %d", __func__, ret);
    }

    EventHandle(driver);

    ret = GpioEnableIrq(intGpioNum);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: enable irq failed, ret %d", __func__, ret);
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

static int32_t SetupInfraredIrq(InfraredDriver *infraredDrv)
{
    uint16_t intGpioNum = infraredDrv->infraredCfg->gpioNum;
    uint16_t irqFlag = infraredDrv->infraredCfg->irqFlag;
    int32_t ret = GpioSetDir(intGpioNum, GPIO_DIR_IN);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: gpio set dir failed, ret %d", __func__, ret);
        return ret;
    }
    ret = GpioSetIrq(intGpioNum, irqFlag | GPIO_IRQ_USING_THREAD, InfraredIrqHandle, infraredDrv);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: register irq failed, ret %d", __func__, ret);
        return ret;
    }
    ret = GpioEnableIrq(intGpioNum);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: enable irq failed, ret %d", __func__, ret);
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

static int32_t InfraredInit(InfraredDriver *infraredDrv)
{
    int32_t ret = SetupInfraredIrq(infraredDrv);
    CHECK_RETURN_VALUE(ret);
    return HDF_SUCCESS;
}

static InfraredCfg *InfraredConfigInstance(struct HdfDeviceObject *device)
{
    int32_t ret;
    InfraredCfg *infraredCfg = (InfraredCfg *)OsalMemAlloc(sizeof(InfraredCfg));
    if (infraredCfg == NULL) {
        HDF_LOGE("%s: malloc infrared config failed", __func__);
        return NULL;
    }
    ret = memset_s(infraredCfg, sizeof(InfraredCfg), 0, sizeof(InfraredCfg));
    if (ret != 0) {
        HDF_LOGE("%s: memset_s infrared config failed", __func__);
        OsalMemFree(infraredCfg);
        return NULL;
    }
    infraredCfg->hdfInfraredDev = device;

    if (ParseInfraredConfig(device->property, infraredCfg) != HDF_SUCCESS) {
        HDF_LOGE("%s: parse infrared config failed", __func__);
        OsalMemFree(infraredCfg);
        return NULL;
    }
    return infraredCfg;
}

static InfraredDriver *InfraredDriverInstance(InfraredCfg *infraredCfg)
{
    int32_t ret;
    InfraredDriver *infraredDrv = (InfraredDriver *)OsalMemCalloc(sizeof(InfraredDriver));
    if (infraredDrv == NULL) {
        HDF_LOGE("%s: malloc infrared driver failed", __func__);
        return NULL;
    }
    ret = memset_s(infraredDrv, sizeof(InfraredDriver), 0, sizeof(InfraredDriver));
    if (ret != 0) {
        HDF_LOGE("%s: memset infrared driver failed", __func__);
        OsalMemFree(infraredDrv);
        return NULL;
    }

    infraredDrv->devType = infraredCfg->devType;
    infraredDrv->infraredCfg = infraredCfg;

    return infraredDrv;
}

static InputDevice *InputDeviceInstance(InfraredDriver *infraredDrv)
{
    InputDevice *inputDev = (InputDevice *)OsalMemAlloc(sizeof(InputDevice));
    if (inputDev == NULL) {
        HDF_LOGE("%s: malloc input device failed", __func__);
        return NULL;
    }
    (void)memset_s(inputDev, sizeof(InputDevice), 0, sizeof(InputDevice));

    inputDev->pvtData = (void *)infraredDrv;
    inputDev->devType = infraredDrv->devType;
    inputDev->devName = infraredDrv->infraredCfg->infraredName;
    inputDev->hdfDevObj = infraredDrv->infraredCfg->hdfInfraredDev;
    infraredDrv->inputDev = inputDev;

    return inputDev;
}

static int32_t RegisterInfraredDevice(InfraredCfg *infraredCfg)
{
    int32_t ret;
    InfraredDriver *infraredDrv = InfraredDriverInstance(infraredCfg);
    if (infraredDrv == NULL) {
        HDF_LOGE("%s: instance infrared config failed", __func__);
        return HDF_ERR_MALLOC_FAIL;
    }

    ret = InfraredInit(infraredDrv);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: infrared driver init failed, ret %d", __func__, ret);
        OsalMemFree(infraredDrv);
        return HDF_FAILURE;
    }

    InputDevice *inputDev = InputDeviceInstance(infraredDrv);
    if (inputDev == NULL) {
        HDF_LOGE("%s: instance input device failed", __func__);
        OsalMemFree(infraredDrv);
        return HDF_FAILURE;
    }

    ret = RegisterInputDevice(inputDev);
    if (ret != HDF_SUCCESS) {
        OsalMemFree(inputDev->pkgBuf);
        OsalMemFree(inputDev);
        OsalMemFree(infraredDrv);
        HDF_LOGE("%s: RegisterInputDevice failed, ret %d", __func__, ret);
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

static int32_t HdfInfraredDriverInit(struct HdfDeviceObject *device)
{
    int32_t ret;
    HDF_LOGI("%s: enter", __func__);
    if (device == NULL) {
        return HDF_ERR_INVALID_PARAM;
    }

    InfraredCfg *infraredCfg = InfraredConfigInstance(device);
    if (infraredCfg == NULL) {
        HDF_LOGE("%s: instance infrared config failed", __func__);
        return HDF_ERR_MALLOC_FAIL;
    }

    ret = RegisterInfraredDevice(infraredCfg);
    if (ret != HDF_SUCCESS) {
        OsalMemFree(infraredCfg);
        return HDF_FAILURE;
    }
    HDF_LOGI("%s: exit succ!", __func__);
    return HDF_SUCCESS;
}

static int32_t HdfInfraredDispatch(struct HdfDeviceIoClient *client, int cmd, struct HdfSBuf *data,
    struct HdfSBuf *reply)
{
    (void)cmd;
    if (client == NULL || data == NULL || reply == NULL) {
        HDF_LOGE("%s: param is null", __func__);
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

static void HdfInfraredDriverRelease(struct HdfDeviceObject *device)
{
    InfraredDriver *driver = NULL;
    InputDevice *inputDev = NULL;

    if (device == NULL || device->priv == NULL) {
        HDF_LOGE("%s: param is null", __func__);
        return;
    }
    driver = (InfraredDriver *)device->priv;
    inputDev = driver->inputDev;
    if (inputDev != NULL) {
        UnregisterInputDevice(inputDev);
        driver->inputDev = NULL;
    }
    OsalMemFree(driver);
}

static int32_t HdfInfraredDriverBind(struct HdfDeviceObject *device)
{
    if (device == NULL) {
        return HDF_ERR_INVALID_PARAM;
    }
    static struct IDeviceIoService infraredService = {
        .object.objectId = 1,
        .Dispatch = HdfInfraredDispatch,
    };
    device->service = &infraredService;
    return HDF_SUCCESS;
}

struct HdfDriverEntry g_hdfInfraredEntry = {
    .moduleVersion = 1,
    .moduleName = "HDF_INFRARED",
    .Bind = HdfInfraredDriverBind,
    .Init = HdfInfraredDriverInit,
    .Release = HdfInfraredDriverRelease,
};

HDF_INIT(g_hdfInfraredEntry);
