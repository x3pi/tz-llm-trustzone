/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include <securec.h>
#include "osal_mem.h"
#include "osal_io.h"
#include "hdf_device_desc.h"
#include "hdf_log.h"
#include "hdf_touch.h"
#include "event_hub.h"
#include "input_i2c_ops.h"
#include "touch_ft5406.h"

static FT5406TouchData *g_touchData = NULL;

static int32_t ChipInit(ChipDevice *device)
{
    (void)device;
    return HDF_SUCCESS;
}

static int32_t ChipResume(ChipDevice *device)
{
    (void)device;
    return HDF_SUCCESS;
}

static int32_t ChipSuspend(ChipDevice *device)
{
    (void)device;
    return HDF_SUCCESS;
}

static int32_t ChipDetect(ChipDevice *device)
{
    uint8_t regAddr;
    uint8_t regValue[FTS_REG_IDX_MAX] = {0};
    int32_t ret;
    InputI2cClient *i2cClient = &device->driver->i2cClient;
    const int idx0 = 0;
    const int idx1 = 1;
    const int idx2 = 2;

    regAddr = FTS_REG_FW_VER;
    ret = InputI2cRead(i2cClient, &regAddr, 1, &regValue[idx0], 1);
    CHIP_CHECK_RETURN(ret);

    regAddr = FTS_REG_FW_MIN_VER;
    ret = InputI2cRead(i2cClient, &regAddr, 1, &regValue[idx1], 1);
    CHIP_CHECK_RETURN(ret);

    regAddr = FTS_REG_FW_SUB_MIN_VER;
    ret = InputI2cRead(i2cClient, &regAddr, 1, &regValue[idx2], 1);
    CHIP_CHECK_RETURN(ret);

    HDF_LOGI("%s: Firmware version = %d.%d.%d", __func__, regValue[idx0], \
        regValue[idx1], regValue[idx2]);

    (void)ChipInit(device);
    (void)ChipResume(device);
    (void)ChipSuspend(device);

    return HDF_SUCCESS;
}

static int32_t ParsePointData(ChipDevice *device, FrameData *frame, uint8_t pointNum)
{
    int32_t i;
    int32_t ret;
    uint8_t regAddr;
    uint8_t touchId;
    short fingersX;
    short fingersY;
    uint8_t buf[POINT_BUFFER_LEN] = { 0 };
    InputI2cClient *i2cClient = &device->driver->i2cClient;

    for (i = 0; i < pointNum; i++) {
        regAddr = FTS_REG_X_H + (i * FT_POINT_SIZE);
        ret = InputI2cRead(i2cClient, &regAddr, 1, buf, POINT_BUFFER_LEN);
        if (ret != HDF_SUCCESS) {
            HDF_LOGE("Unable to fetch data, error: %d\n", ret);
            return ret;
        }

        touchId = (buf[FT_POINT_NUM_POS]) >> HALF_BYTE_OFFSET;
        if (touchId >= MAX_SUPPORT_POINT) {
            break;
        }

        fingersX = (short) (buf[FT_X_H_POS] & 0x0F) << ONE_BYTE_OFFSET;
        frame->fingers[i].x = fingersX | (short) buf[FT_X_L_POS];
        fingersY = (short) (buf[FT_Y_H_POS] & 0x0F) << ONE_BYTE_OFFSET;
        frame->fingers[i].y = fingersY | (short) buf[FT_Y_L_POS];
        frame->fingers[i].x = device->boardCfg->attr.resolutionX - frame->fingers[i].x - 1;
        frame->fingers[i].y = device->boardCfg->attr.resolutionY - frame->fingers[i].y - 1;
        frame->fingers[i].trackId = touchId;
        frame->fingers[i].status = buf[FT_EVENT_POS] >> SIX_BIT_OFFSET;
        frame->fingers[i].valid = true;
        frame->definedEvent = frame->fingers[i].status;

        if ((frame->fingers[i].status == TOUCH_DOWN || frame->fingers[i].status == TOUCH_CONTACT) && (pointNum == 0)) {
            HDF_LOGE("%s: abnormal event data from driver chip", __func__);
            return HDF_FAILURE;
        }
    }

    return HDF_SUCCESS;
}

static int32_t ChipDataHandle(ChipDevice *device)
{
    uint8_t reg = FTS_REG_STATUS;
    uint8_t touchStatus = 0;
    static uint8_t lastTouchStatus = 0;
    uint8_t pointNum = 0;
    int32_t ret = HDF_SUCCESS;
    InputI2cClient *i2cClient = &device->driver->i2cClient;
    FrameData *frame = &device->driver->frameData;

    ret = InputI2cRead(i2cClient, &reg, 1, &touchStatus, 1);
    CHECK_RETURN_VALUE(ret);

    OsalMutexLock(&device->driver->mutex);
    (void)memset_s(frame, sizeof(FrameData), 0, sizeof(FrameData));

    pointNum = touchStatus & HALF_BYTE_MASK;
    if (pointNum == 0) {
        if (lastTouchStatus == 1) {
            lastTouchStatus = 0;
            frame->realPointNum = 0;
            frame->definedEvent = TOUCH_UP;
            ret = HDF_SUCCESS;
        } else {
            ret = HDF_FAILURE;
        }

        OsalMutexUnlock(&device->driver->mutex);
        return ret;
    }

    if (lastTouchStatus != touchStatus) {
        lastTouchStatus = touchStatus;
    }

    frame->realPointNum = pointNum;
    frame->definedEvent = TOUCH_DOWN;

    ret = ParsePointData(device, frame, pointNum);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: ParsePointData fail", __func__);
        OsalMutexUnlock(&device->driver->mutex);
        return ret;
    }

    OsalMutexUnlock(&device->driver->mutex);
    return ret;
}

static int32_t UpdateFirmware(ChipDevice *device)
{
    return HDF_SUCCESS;
}

static void SetAbility(ChipDevice *device)
{
    device->driver->inputDev->abilitySet.devProp[0] = SET_BIT(INPUT_PROP_DIRECT);
    device->driver->inputDev->abilitySet.eventType[0] = SET_BIT(EV_SYN) |
        SET_BIT(EV_KEY) | SET_BIT(EV_ABS);
    device->driver->inputDev->abilitySet.absCode[0] = SET_BIT(ABS_X) | SET_BIT(ABS_Y);
    device->driver->inputDev->abilitySet.absCode[1] = SET_BIT(ABS_MT_POSITION_X) |
        SET_BIT(ABS_MT_POSITION_Y) | SET_BIT(ABS_MT_TRACKING_ID);
    device->driver->inputDev->abilitySet.keyCode[KEY_CODE_4TH] = SET_BIT(KEY_UP) | SET_BIT(KEY_DOWN);
    device->driver->inputDev->attrSet.axisInfo[ABS_X].min = 0;
    device->driver->inputDev->attrSet.axisInfo[ABS_X].max = device->boardCfg->attr.resolutionX - 1;
    device->driver->inputDev->attrSet.axisInfo[ABS_X].range = 0;
    device->driver->inputDev->attrSet.axisInfo[ABS_Y].min = 0;
    device->driver->inputDev->attrSet.axisInfo[ABS_Y].max = device->boardCfg->attr.resolutionY - 1;
    device->driver->inputDev->attrSet.axisInfo[ABS_Y].range = 0;
    device->driver->inputDev->attrSet.axisInfo[ABS_MT_POSITION_X].min = 0;
    device->driver->inputDev->attrSet.axisInfo[ABS_MT_POSITION_X].max = device->boardCfg->attr.resolutionX - 1;
    device->driver->inputDev->attrSet.axisInfo[ABS_MT_POSITION_X].range = 0;
    device->driver->inputDev->attrSet.axisInfo[ABS_MT_POSITION_Y].min = 0;
    device->driver->inputDev->attrSet.axisInfo[ABS_MT_POSITION_Y].max = device->boardCfg->attr.resolutionY - 1;
    device->driver->inputDev->attrSet.axisInfo[ABS_MT_POSITION_Y].range = 0;
    device->driver->inputDev->attrSet.axisInfo[ABS_MT_TRACKING_ID].max = MAX_SUPPORT_POINT;
}

static struct TouchChipOps g_ft5406ChipOps = {
    .Init = ChipInit,
    .Detect = ChipDetect,
    .Resume = ChipResume,
    .Suspend = ChipSuspend,
    .DataHandle = ChipDataHandle,
    .UpdateFirmware = UpdateFirmware,
    .SetAbility = SetAbility,
};

static void ChipTimer(struct timer_list *t)
{
    FT5406TouchData *touchData = from_timer(touchData, t, timer);

    schedule_work(&touchData->work_poll);
    mod_timer(&touchData->timer, jiffies + msecs_to_jiffies(POLL_INTERVAL_MS));
}

static void ChipWorkPoll(struct work_struct *work)
{
    FT5406TouchData *touchData = container_of(work, FT5406TouchData, work_poll);
    TouchDriver *driver = (TouchDriver *)touchData->data;
    InputDevice *dev = driver->inputDev;
    FrameData *frame = &driver->frameData;
    int32_t ret;
    int32_t i;

    ret = ChipDataHandle(driver->device);
    if (ret == HDF_SUCCESS) {
        OsalMutexLock(&driver->mutex);
        for (i = 0; i < MAX_FINGERS_NUM; i++) {
            if (frame->fingers[i].valid) {
                input_report_abs(dev, ABS_MT_POSITION_X, frame->fingers[i].x);
                input_report_abs(dev, ABS_MT_POSITION_Y, frame->fingers[i].y);
                input_report_abs(dev, ABS_MT_TRACKING_ID, frame->fingers[i].trackId);
                input_mt_sync(dev);
            }
        }

        if ((frame->definedEvent == TOUCH_DOWN) || (frame->definedEvent == TOUCH_CONTACT)) {
            input_report_key(dev, BTN_TOUCH, 1); // BTN_TOUCH DOWN
        } else {
            input_report_key(dev, BTN_TOUCH, 0); // BTN_TOUCH UP
        }
        OsalMutexUnlock(&driver->mutex);
        input_sync(dev);
    }
}

static void FreeChipConfig(TouchChipCfg *config)
{
    if (config->pwrSeq.pwrOn.buf != NULL) {
        OsalMemFree(config->pwrSeq.pwrOn.buf);
    }

    if (config->pwrSeq.pwrOff.buf != NULL) {
        OsalMemFree(config->pwrSeq.pwrOff.buf);
    }

    if (config->pwrSeq.resume.buf != NULL) {
        OsalMemFree(config->pwrSeq.resume.buf);
    }

    if (config->pwrSeq.suspend.buf != NULL) {
        OsalMemFree(config->pwrSeq.suspend.buf);
    }

    OsalMemFree(config);
}

static TouchChipCfg *ChipConfigInstance(struct HdfDeviceObject *device)
{
    TouchChipCfg *chipCfg = (TouchChipCfg *)OsalMemAlloc(sizeof(TouchChipCfg));
    if (chipCfg == NULL) {
        HDF_LOGE("%s: instance chip config failed", __func__);
        return NULL;
    }
    (void)memset_s(chipCfg, sizeof(TouchChipCfg), 0, sizeof(TouchChipCfg));

    if (ParseTouchChipConfig(device->property, chipCfg) != HDF_SUCCESS) {
        HDF_LOGE("%s: parse chip config failed", __func__);
        OsalMemFree(chipCfg);
        chipCfg = NULL;
    }
    return chipCfg;
}

static ChipDevice *ChipDeviceInstance(void)
{
    ChipDevice *chipDev = (ChipDevice *)OsalMemAlloc(sizeof(ChipDevice));
    if (chipDev == NULL) {
        HDF_LOGE("%s: instance chip device failed", __func__);
        return NULL;
    }
    (void)memset_s(chipDev, sizeof(ChipDevice), 0, sizeof(ChipDevice));
    return chipDev;
}

static int32_t HdfFocalChipInit(struct HdfDeviceObject *device)
{
    TouchChipCfg *chipCfg = NULL;
    ChipDevice *chipDev = NULL;

    if (device == NULL) {
        HDF_LOGE("%s: device is NULL", __func__);
        return HDF_ERR_INVALID_PARAM;
    }

    chipCfg = ChipConfigInstance(device);
    if (chipCfg == NULL) {
        HDF_LOGE("%s: ChipConfigInstance is NULL", __func__);
        return HDF_ERR_MALLOC_FAIL;
    }

    chipDev = ChipDeviceInstance();
    if (chipDev == NULL) {
        HDF_LOGE("%s: ChipDeviceInstance fail", __func__);
        FreeChipConfig(chipCfg);
        return HDF_FAILURE;
    }

    chipDev->chipCfg = chipCfg;
    chipDev->ops = &g_ft5406ChipOps;
    chipDev->chipName = chipCfg->chipName;
    chipDev->vendorName = chipCfg->vendorName;
    device->priv = (void *)chipDev;

    if (RegisterTouchChipDevice(chipDev) != HDF_SUCCESS) {
        HDF_LOGE("%s: RegisterTouchChipDevice fail", __func__);
        OsalMemFree(chipDev);
        FreeChipConfig(chipCfg);
        return HDF_FAILURE;
    }

    g_touchData = (FT5406TouchData *)OsalMemAlloc(sizeof(FT5406TouchData));
    if (g_touchData == NULL) {
        HDF_LOGE("OsalMemAlloc touchData failed!");
        OsalMemFree(chipDev);
        FreeChipConfig(chipCfg);
        return HDF_FAILURE;
    }

    (void)memset_s(g_touchData, sizeof(FT5406TouchData), 0, sizeof(FT5406TouchData));

    if (chipDev->driver != NULL) {
        g_touchData->data = chipDev->driver;
    }

    INIT_WORK(&g_touchData->work_poll, ChipWorkPoll);
    timer_setup(&g_touchData->timer, ChipTimer, 0);
    g_touchData->timer.expires = jiffies + msecs_to_jiffies(POLL_INTERVAL_MS);
    add_timer(&g_touchData->timer);

    HDF_LOGI("%s: exit succ, chipName = %s", __func__, chipCfg->chipName);
    return HDF_SUCCESS;
}

static void HdfFocalChipRelease(struct HdfDeviceObject *device)
{
    HDF_LOGI("HdfFocalChipRelease enter.");

    if (device == NULL || device->priv == NULL || g_touchData == NULL) {
        HDF_LOGE("%s: param is null", __func__);
        return;
    }

    del_timer_sync(&g_touchData->timer);
    cancel_work_sync(&g_touchData->work_poll);

    OsalMemFree(g_touchData);
    g_touchData = NULL;

    return;
}

struct HdfDriverEntry g_touchFocalChipEntry = {
    .moduleVersion = 1,
    .moduleName = "HDF_TOUCH_FT5406",
    .Init = HdfFocalChipInit,
    .Release = HdfFocalChipRelease,
};

HDF_INIT(g_touchFocalChipEntry);
