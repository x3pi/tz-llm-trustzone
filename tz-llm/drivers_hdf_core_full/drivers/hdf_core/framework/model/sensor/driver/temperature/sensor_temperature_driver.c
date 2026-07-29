/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include "sensor_temperature_driver.h"
#include <securec.h>
#include "hdf_base.h"
#include "hdf_device_desc.h"
#include "osal_math.h"
#include "osal_mem.h"
#include "sensor_config_controller.h"
#include "sensor_device_manager.h"
#include "sensor_platform_if.h"

#define HDF_LOG_TAG    khdf_sensor_temperature_driver

#define HDF_TEMPERATURE_WORK_QUEUE_NAME    "hdf_temperature_work_queue"

static struct TemperatureDrvData *g_temperatureDrvData = NULL;

static struct TemperatureDrvData *TemperatureGetDrvData(void)
{
    return g_temperatureDrvData;
}

static struct SensorRegCfgGroupNode *g_regCfgGroup[SENSOR_GROUP_MAX] = { NULL };

int32_t TemperatureRegisterChipOps(const struct TemperatureOpsCall *ops)
{
    struct TemperatureDrvData *drvData = TemperatureGetDrvData();

    CHECK_NULL_PTR_RETURN_VALUE(drvData, HDF_ERR_INVALID_PARAM);
    CHECK_NULL_PTR_RETURN_VALUE(ops, HDF_ERR_INVALID_PARAM);

    drvData->ops.Init = ops->Init;
    drvData->ops.ReadData = ops->ReadData;
    return HDF_SUCCESS;
}

static void TemperatureDataWorkEntry(void *arg)
{
    struct TemperatureDrvData *drvData = NULL;

    drvData = (struct TemperatureDrvData *)arg;
    CHECK_NULL_PTR_RETURN(drvData);

    if (drvData->ops.ReadData == NULL) {
        HDF_LOGE("%s: Temperature readdata function NULL", __func__);
        return;
    }
    if (drvData->ops.ReadData(drvData->temperatureCfg) != HDF_SUCCESS) {
        HDF_LOGE("%s: Temperature read data failed", __func__);
    }
}

static void TemperatureTimerEntry(uintptr_t arg)
{
    int64_t interval;
    int32_t ret;
    struct TemperatureDrvData *drvData = (struct TemperatureDrvData *)arg;
    CHECK_NULL_PTR_RETURN(drvData);

    if (!HdfAddWork(&drvData->temperatureWorkQueue, &drvData->temperatureWork)) {
        HDF_LOGE("%s: Temperature add work queue failed", __func__);
    }

    interval = OsalDivS64(drvData->interval, (SENSOR_CONVERT_UNIT * SENSOR_CONVERT_UNIT));
    interval = (interval < SENSOR_TIMER_MIN_TIME) ? SENSOR_TIMER_MIN_TIME : interval;
    ret = OsalTimerSetTimeout(&drvData->temperatureTimer, interval);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: Temperature modify time failed", __func__);
    }
}

static int32_t InitTemperatureData(struct TemperatureDrvData *drvData)
{
    if (HdfWorkQueueInit(&drvData->temperatureWorkQueue, HDF_TEMPERATURE_WORK_QUEUE_NAME) != HDF_SUCCESS) {
        HDF_LOGE("%s: Temperature init work queue failed", __func__);
        return HDF_FAILURE;
    }

    if (HdfWorkInit(&drvData->temperatureWork, TemperatureDataWorkEntry, drvData) != HDF_SUCCESS) {
        HDF_LOGE("%s: Temperature create thread failed", __func__);
        return HDF_FAILURE;
    }

    drvData->interval = SENSOR_TIMER_MIN_TIME;
    drvData->enable = false;
    drvData->detectFlag = false;

    return HDF_SUCCESS;
}

static int32_t SetTemperatureEnable(void)
{
    int32_t ret;
    struct TemperatureDrvData *drvData = TemperatureGetDrvData();

    CHECK_NULL_PTR_RETURN_VALUE(drvData, HDF_ERR_INVALID_PARAM);
    CHECK_NULL_PTR_RETURN_VALUE(drvData->temperatureCfg, HDF_ERR_INVALID_PARAM);

    if (drvData->enable) {
        HDF_LOGE("%s: Temperature sensor is enabled", __func__);
        return HDF_SUCCESS;
    }

    ret = SetSensorRegCfgArray(&drvData->temperatureCfg->busCfg, \
        drvData->temperatureCfg->regCfgGroup[SENSOR_ENABLE_GROUP]);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: Temperature sensor enable config failed", __func__);
        return ret;
    }

    ret = OsalTimerCreate(&drvData->temperatureTimer, SENSOR_TIMER_MIN_TIME, TemperatureTimerEntry, (uintptr_t)drvData);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: Temperature create timer failed[%d]", __func__, ret);
        return ret;
    }

    ret = OsalTimerStartLoop(&drvData->temperatureTimer);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: Temperature start timer failed[%d]", __func__, ret);
        return ret;
    }
    drvData->enable = true;

    return HDF_SUCCESS;
}

static int32_t SetTemperatureDisable(void)
{
    int32_t ret;
    struct TemperatureDrvData *drvData = TemperatureGetDrvData();

    CHECK_NULL_PTR_RETURN_VALUE(drvData, HDF_ERR_INVALID_PARAM);
    CHECK_NULL_PTR_RETURN_VALUE(drvData->temperatureCfg, HDF_ERR_INVALID_PARAM);

    if (!drvData->enable) {
        HDF_LOGE("%s: Temperature sensor had disable", __func__);
        return HDF_SUCCESS;
    }

    ret = SetSensorRegCfgArray(&drvData->temperatureCfg->busCfg, \
        drvData->temperatureCfg->regCfgGroup[SENSOR_DISABLE_GROUP]);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: Temperature sensor disable config failed", __func__);
        return ret;
    }

    ret = OsalTimerDelete(&drvData->temperatureTimer);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: Temperature delete timer failed", __func__);
        return ret;
    }
    drvData->enable = false;

    return HDF_SUCCESS;
}

static int32_t SetTemperatureBatch(int64_t samplingInterval, int64_t interval)
{
    (void)interval;

    struct TemperatureDrvData *drvData = NULL;

    drvData = TemperatureGetDrvData();
    CHECK_NULL_PTR_RETURN_VALUE(drvData, HDF_ERR_INVALID_PARAM);

    drvData->interval = samplingInterval;

    return HDF_SUCCESS;
}

static int32_t SetTemperatureMode(int32_t mode)
{
    if (mode <= SENSOR_WORK_MODE_DEFAULT || mode >= SENSOR_WORK_MODE_MAX) {
        HDF_LOGE("%s: The current mode is not supported", __func__);
        return HDF_FAILURE;
    }

    return HDF_SUCCESS;
}

static int32_t SetTemperatureOption(uint32_t option)
{
    (void)option;

    return HDF_SUCCESS;
}

static int32_t DispatchTemperature(struct HdfDeviceIoClient *client,
    int cmd, struct HdfSBuf *data, struct HdfSBuf *reply)
{
    (void)client;
    (void)cmd;
    (void)data;
    (void)reply;

    return HDF_SUCCESS;
}

int32_t TemperatureBindDriver(struct HdfDeviceObject *device)
{
    CHECK_NULL_PTR_RETURN_VALUE(device, HDF_ERR_INVALID_PARAM);

    struct TemperatureDrvData *drvData = (struct TemperatureDrvData *)OsalMemCalloc(sizeof(*drvData));
    if (drvData == NULL) {
        HDF_LOGE("%s: Malloc Temperature drv data fail!", __func__);
        return HDF_ERR_MALLOC_FAIL;
    }

    drvData->ioService.Dispatch = DispatchTemperature;
    drvData->device = device;
    device->service = &drvData->ioService;
    g_temperatureDrvData = drvData;

    return HDF_SUCCESS;
}

static int32_t InitTemperatureOps(struct SensorCfgData *config, struct SensorDeviceInfo *deviceInfo)
{
    CHECK_NULL_PTR_RETURN_VALUE(config, HDF_ERR_INVALID_PARAM);

    deviceInfo->ops.Enable = SetTemperatureEnable;
    deviceInfo->ops.Disable = SetTemperatureDisable;
    deviceInfo->ops.SetBatch = SetTemperatureBatch;
    deviceInfo->ops.SetMode = SetTemperatureMode;
    deviceInfo->ops.SetOption = SetTemperatureOption;

    if (memcpy_s(&deviceInfo->sensorInfo, sizeof(deviceInfo->sensorInfo),
        &config->sensorInfo, sizeof(config->sensorInfo)) != EOK) {
        HDF_LOGE("%s: Copy sensor info failed", __func__);
        return HDF_FAILURE;
    }

    return HDF_SUCCESS;
}

static int32_t InitTemperatureAfterDetected(struct SensorCfgData *config)
{
    struct SensorDeviceInfo deviceInfo;
    CHECK_NULL_PTR_RETURN_VALUE(config, HDF_ERR_INVALID_PARAM);

    if (InitTemperatureOps(config, &deviceInfo) != HDF_SUCCESS) {
        HDF_LOGE("%s: Init Temperature ops failed", __func__);
        return HDF_FAILURE;
    }

    if (AddSensorDevice(&deviceInfo) != HDF_SUCCESS) {
        HDF_LOGE("%s: Add Temperature device failed", __func__);
        return HDF_FAILURE;
    }

    if (ParseSensorRegConfig(config) != HDF_SUCCESS) {
        HDF_LOGE("%s: Parse sensor register failed", __func__);
        (void)DeleteSensorDevice(&config->sensorInfo);
        ReleaseSensorAllRegConfig(config);
        ReleaseSensorDirectionConfig(config);
        return HDF_FAILURE;
    }

    return HDF_SUCCESS;
}

struct SensorCfgData *TemperatureCreateCfgData(const struct DeviceResourceNode *node)
{
    struct TemperatureDrvData *drvData = TemperatureGetDrvData();

    if (drvData == NULL || node == NULL) {
        HDF_LOGE("%s: Temperature node pointer NULL", __func__);
        return NULL;
    }

    if (drvData->detectFlag) {
        HDF_LOGE("%s: Temperature sensor have detected", __func__);
        return NULL;
    }

    if (drvData->temperatureCfg == NULL) {
        HDF_LOGE("%s: Temperature TemperatureCfg pointer NULL", __func__);
        return NULL;
    }

    if (GetSensorBaseConfigData(node, drvData->temperatureCfg) != HDF_SUCCESS) {
        HDF_LOGE("%s: Get sensor base config failed", __func__);
        goto BASE_CONFIG_EXIT;
    }

    if ((drvData->temperatureCfg->sensorAttr.chipName != NULL) && \
        (DetectSensorDevice(drvData->temperatureCfg) != HDF_SUCCESS)) {
        HDF_LOGI("%s: Temperature sensor detect device no exist", __func__);
        drvData->detectFlag = false;
        goto BASE_CONFIG_EXIT;
    } else {
        if (GetSensorBusHandle(&drvData->temperatureCfg->busCfg) != HDF_SUCCESS) {
            HDF_LOGE("%s: get sensor bus handle failed", __func__);
            (void)ReleaseSensorBusHandle(&drvData->temperatureCfg->busCfg);
            drvData->detectFlag = false;
            goto BASE_CONFIG_EXIT;
        }
    }

    drvData->detectFlag = true;
    if (InitTemperatureAfterDetected(drvData->temperatureCfg) != HDF_SUCCESS) {
        HDF_LOGI("%s: Temperature sensor detect device no exist", __func__);
        goto INIT_EXIT;
    }

    return drvData->temperatureCfg;

INIT_EXIT:
    (void)ReleaseSensorBusHandle(&drvData->temperatureCfg->busCfg);
BASE_CONFIG_EXIT:
    drvData->temperatureCfg->root = NULL;
    (void)memset_s(&drvData->temperatureCfg->sensorInfo,
        sizeof(struct SensorBasicInfo), 0, sizeof(struct SensorBasicInfo));
    (void)memset_s(&drvData->temperatureCfg->busCfg, sizeof(struct SensorBusCfg), 0, sizeof(struct SensorBusCfg));
    (void)memset_s(&drvData->temperatureCfg->sensorAttr, sizeof(struct SensorAttr), 0, sizeof(struct SensorAttr));

    return drvData->temperatureCfg;
}

void TemperatureReleaseCfgData(struct SensorCfgData *temperatureCfg)
{
    CHECK_NULL_PTR_RETURN(temperatureCfg);

    (void)DeleteSensorDevice(&temperatureCfg->sensorInfo);
    ReleaseSensorAllRegConfig(temperatureCfg);
    (void)ReleaseSensorBusHandle(&temperatureCfg->busCfg);
    ReleaseSensorDirectionConfig(temperatureCfg);

    temperatureCfg->root = NULL;
    (void)memset_s(&temperatureCfg->sensorInfo, sizeof(struct SensorBasicInfo), 0, sizeof(struct SensorBasicInfo));
    (void)memset_s(&temperatureCfg->busCfg, sizeof(struct SensorBusCfg), 0, sizeof(struct SensorBusCfg));
    (void)memset_s(&temperatureCfg->sensorAttr, sizeof(struct SensorAttr), 0, sizeof(struct SensorAttr));
}

int32_t TemperatureInitDriver(struct HdfDeviceObject *device)
{
    CHECK_NULL_PTR_RETURN_VALUE(device, HDF_ERR_INVALID_PARAM);
    struct TemperatureDrvData *drvData = (struct TemperatureDrvData *)device->service;
    CHECK_NULL_PTR_RETURN_VALUE(drvData, HDF_ERR_INVALID_PARAM);

    if (InitTemperatureData(drvData) != HDF_SUCCESS) {
        HDF_LOGE("%s: Init Temperature config failed", __func__);
        return HDF_FAILURE;
    }

    drvData->temperatureCfg = (struct SensorCfgData *)OsalMemCalloc(sizeof(*drvData->temperatureCfg));
    if (drvData->temperatureCfg == NULL) {
        HDF_LOGE("%s: Malloc Temperature config data failed", __func__);
        return HDF_FAILURE;
    }

    drvData->temperatureCfg->regCfgGroup = &g_regCfgGroup[0];

    HDF_LOGI("%s: Init Temperature driver success", __func__);
    return HDF_SUCCESS;
}

void TemperatureReleaseDriver(struct HdfDeviceObject *device)
{
    CHECK_NULL_PTR_RETURN(device);

    struct TemperatureDrvData *drvData = (struct TemperatureDrvData *)device->service;
    CHECK_NULL_PTR_RETURN(drvData);

    if (drvData->detectFlag && drvData->temperatureCfg != NULL) {
        TemperatureReleaseCfgData(drvData->temperatureCfg);
    }

    OsalMemFree(drvData->temperatureCfg);
    drvData->temperatureCfg = NULL;

    HdfWorkDestroy(&drvData->temperatureWork);
    HdfWorkQueueDestroy(&drvData->temperatureWorkQueue);
    OsalMemFree(drvData);
}

struct HdfDriverEntry g_sensorTemperatureDevEntry = {
    .moduleVersion = 1,
    .moduleName = "HDF_SENSOR_TEMPERATURE",
    .Bind = TemperatureBindDriver,
    .Init = TemperatureInitDriver,
    .Release = TemperatureReleaseDriver,
};

HDF_INIT(g_sensorTemperatureDevEntry);
