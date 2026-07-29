/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include "sensor_humidity_driver.h"
#include <securec.h>
#include "hdf_base.h"
#include "hdf_device_desc.h"
#include "osal_math.h"
#include "osal_mem.h"
#include "sensor_config_controller.h"
#include "sensor_device_manager.h"
#include "sensor_platform_if.h"

#define HDF_LOG_TAG    khdf_sensor_humidity_driver

#define HDF_HUMIDITY_WORK_QUEUE_NAME    "hdf_humidity_work_queue"

static struct HumidityDrvData *g_humidityDrvData = NULL;

static struct HumidityDrvData *HumidityGetDrvData(void)
{
    return g_humidityDrvData;
}

static struct SensorRegCfgGroupNode *g_regCfgGroup[SENSOR_GROUP_MAX] = { NULL };

int32_t HumidityRegisterChipOps(const struct HumidityOpsCall *ops)
{
    struct HumidityDrvData *drvData = HumidityGetDrvData();

    CHECK_NULL_PTR_RETURN_VALUE(drvData, HDF_ERR_INVALID_PARAM);
    CHECK_NULL_PTR_RETURN_VALUE(ops, HDF_ERR_INVALID_PARAM);

    drvData->ops.Init = ops->Init;
    drvData->ops.ReadData = ops->ReadData;
    return HDF_SUCCESS;
}

static void HumidityDataWorkEntry(void *arg)
{
    struct HumidityDrvData *drvData = NULL;

    drvData = (struct HumidityDrvData *)arg;
    CHECK_NULL_PTR_RETURN(drvData);

    if (drvData->ops.ReadData == NULL) {
        HDF_LOGE("%s: Humidity readdata function NULL", __func__);
        return;
    }
    if (drvData->ops.ReadData(drvData->humidityCfg) != HDF_SUCCESS) {
        HDF_LOGE("%s: Humidity read data failed", __func__);
    }
}

static void HumidityTimerEntry(uintptr_t arg)
{
    int64_t interval;
    int32_t ret;
    struct HumidityDrvData *drvData = (struct HumidityDrvData *)arg;
    CHECK_NULL_PTR_RETURN(drvData);

    if (!HdfAddWork(&drvData->humidityWorkQueue, &drvData->humidityWork)) {
        HDF_LOGE("%s: Humidity add work queue failed", __func__);
    }

    interval = OsalDivS64(drvData->interval, (SENSOR_CONVERT_UNIT * SENSOR_CONVERT_UNIT));
    interval = (interval < SENSOR_TIMER_MIN_TIME) ? SENSOR_TIMER_MIN_TIME : interval;
    ret = OsalTimerSetTimeout(&drvData->humidityTimer, interval);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: Humidity modify time failed", __func__);
    }
}

static int32_t InitHumidityData(struct HumidityDrvData *drvData)
{
    if (HdfWorkQueueInit(&drvData->humidityWorkQueue, HDF_HUMIDITY_WORK_QUEUE_NAME) != HDF_SUCCESS) {
        HDF_LOGE("%s: Humidity init work queue failed", __func__);
        return HDF_FAILURE;
    }

    if (HdfWorkInit(&drvData->humidityWork, HumidityDataWorkEntry, drvData) != HDF_SUCCESS) {
        HDF_LOGE("%s: Humidity create thread failed", __func__);
        return HDF_FAILURE;
    }

    drvData->interval = SENSOR_TIMER_MIN_TIME;
    drvData->enable = false;
    drvData->detectFlag = false;

    return HDF_SUCCESS;
}

static int32_t SetHumidityEnable(void)
{
    int32_t ret;
    struct HumidityDrvData *drvData = HumidityGetDrvData();

    CHECK_NULL_PTR_RETURN_VALUE(drvData, HDF_ERR_INVALID_PARAM);
    CHECK_NULL_PTR_RETURN_VALUE(drvData->humidityCfg, HDF_ERR_INVALID_PARAM);

    if (drvData->enable) {
        HDF_LOGE("%s: Humidity sensor is enabled", __func__);
        return HDF_SUCCESS;
    }

    ret = SetSensorRegCfgArray(&drvData->humidityCfg->busCfg, drvData->humidityCfg->regCfgGroup[SENSOR_ENABLE_GROUP]);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: Humidity sensor enable config failed", __func__);
        return ret;
    }

    ret = OsalTimerCreate(&drvData->humidityTimer, SENSOR_TIMER_MIN_TIME, HumidityTimerEntry, (uintptr_t)drvData);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: Humidity create timer failed[%d]", __func__, ret);
        return ret;
    }

    ret = OsalTimerStartLoop(&drvData->humidityTimer);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: Humidity start timer failed[%d]", __func__, ret);
        return ret;
    }
    drvData->enable = true;

    return HDF_SUCCESS;
}

static int32_t SetHumidityDisable(void)
{
    int32_t ret;
    struct HumidityDrvData *drvData = HumidityGetDrvData();

    CHECK_NULL_PTR_RETURN_VALUE(drvData, HDF_ERR_INVALID_PARAM);
    CHECK_NULL_PTR_RETURN_VALUE(drvData->humidityCfg, HDF_ERR_INVALID_PARAM);

    if (!drvData->enable) {
        HDF_LOGE("%s: Humidity sensor had disable", __func__);
        return HDF_SUCCESS;
    }

    ret = SetSensorRegCfgArray(&drvData->humidityCfg->busCfg, drvData->humidityCfg->regCfgGroup[SENSOR_DISABLE_GROUP]);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: Humidity sensor disable config failed", __func__);
        return ret;
    }

    ret = OsalTimerDelete(&drvData->humidityTimer);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: Humidity delete timer failed", __func__);
        return ret;
    }
    drvData->enable = false;

    return HDF_SUCCESS;
}

static int32_t SetHumidityBatch(int64_t samplingInterval, int64_t interval)
{
    (void)interval;

    struct HumidityDrvData *drvData = NULL;

    drvData = HumidityGetDrvData();
    CHECK_NULL_PTR_RETURN_VALUE(drvData, HDF_ERR_INVALID_PARAM);

    drvData->interval = samplingInterval;

    return HDF_SUCCESS;
}

static int32_t SetHumidityMode(int32_t mode)
{
    if (mode <= SENSOR_WORK_MODE_DEFAULT || mode >= SENSOR_WORK_MODE_MAX) {
        HDF_LOGE("%s: The current mode is not supported", __func__);
        return HDF_FAILURE;
    }

    return HDF_SUCCESS;
}

static int32_t SetHumidityOption(uint32_t option)
{
    (void)option;

    return HDF_SUCCESS;
}

static int32_t DispatchHumidity(struct HdfDeviceIoClient *client,
    int cmd, struct HdfSBuf *data, struct HdfSBuf *reply)
{
    (void)client;
    (void)cmd;
    (void)data;
    (void)reply;

    return HDF_SUCCESS;
}

int32_t HumidityBindDriver(struct HdfDeviceObject *device)
{
    CHECK_NULL_PTR_RETURN_VALUE(device, HDF_ERR_INVALID_PARAM);

    struct HumidityDrvData *drvData = (struct HumidityDrvData *)OsalMemCalloc(sizeof(*drvData));
    if (drvData == NULL) {
        HDF_LOGE("%s: Malloc Humidity drv data fail!", __func__);
        return HDF_ERR_MALLOC_FAIL;
    }

    drvData->ioService.Dispatch = DispatchHumidity;
    drvData->device = device;
    device->service = &drvData->ioService;
    g_humidityDrvData = drvData;

    return HDF_SUCCESS;
}

static int32_t InitHumidityOps(struct SensorCfgData *config, struct SensorDeviceInfo *deviceInfo)
{
    CHECK_NULL_PTR_RETURN_VALUE(config, HDF_ERR_INVALID_PARAM);

    deviceInfo->ops.Enable = SetHumidityEnable;
    deviceInfo->ops.Disable = SetHumidityDisable;
    deviceInfo->ops.SetBatch = SetHumidityBatch;
    deviceInfo->ops.SetMode = SetHumidityMode;
    deviceInfo->ops.SetOption = SetHumidityOption;

    if (memcpy_s(&deviceInfo->sensorInfo, sizeof(deviceInfo->sensorInfo),
        &config->sensorInfo, sizeof(config->sensorInfo)) != EOK) {
        HDF_LOGE("%s: Copy sensor info failed", __func__);
        return HDF_FAILURE;
    }

    return HDF_SUCCESS;
}

static int32_t InitHumidityAfterDetected(struct SensorCfgData *config)
{
    struct SensorDeviceInfo deviceInfo;
    CHECK_NULL_PTR_RETURN_VALUE(config, HDF_ERR_INVALID_PARAM);

    if (InitHumidityOps(config, &deviceInfo) != HDF_SUCCESS) {
        HDF_LOGE("%s: Init Humidity ops failed", __func__);
        return HDF_FAILURE;
    }

    if (AddSensorDevice(&deviceInfo) != HDF_SUCCESS) {
        HDF_LOGE("%s: Add Humidity device failed", __func__);
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

struct SensorCfgData *HumidityCreateCfgData(const struct DeviceResourceNode *node)
{
    struct HumidityDrvData *drvData = HumidityGetDrvData();

    if (drvData == NULL || node == NULL) {
        HDF_LOGE("%s: Humidity node pointer NULL", __func__);
        return NULL;
    }

    if (drvData->detectFlag) {
        HDF_LOGE("%s: Humidity sensor have detected", __func__);
        return NULL;
    }

    if (drvData->humidityCfg == NULL) {
        HDF_LOGE("%s: Humidity HumidityCfg pointer NULL", __func__);
        return NULL;
    }

    if (GetSensorBaseConfigData(node, drvData->humidityCfg) != HDF_SUCCESS) {
        HDF_LOGE("%s: Get sensor base config failed", __func__);
        goto BASE_CONFIG_EXIT;
    }

    if ((drvData->humidityCfg->sensorAttr.chipName != NULL) && \
        (DetectSensorDevice(drvData->humidityCfg) != HDF_SUCCESS)) {
        HDF_LOGI("%s: Humidity sensor detect device no exist", __func__);
        drvData->detectFlag = false;
        goto BASE_CONFIG_EXIT;
    } else {
        if (GetSensorBusHandle(&drvData->humidityCfg->busCfg) != HDF_SUCCESS) {
            HDF_LOGE("%s: get sensor bus handle failed", __func__);
            (void)ReleaseSensorBusHandle(&drvData->humidityCfg->busCfg);
            drvData->detectFlag = false;
            goto BASE_CONFIG_EXIT;
        }
    }

    drvData->detectFlag = true;
    if (InitHumidityAfterDetected(drvData->humidityCfg) != HDF_SUCCESS) {
        HDF_LOGI("%s: Humidity sensor detect device no exist", __func__);
        goto INIT_EXIT;
    }
    return drvData->humidityCfg;

INIT_EXIT:
    (void)ReleaseSensorBusHandle(&drvData->humidityCfg->busCfg);
BASE_CONFIG_EXIT:
    drvData->humidityCfg->root = NULL;
    (void)memset_s(&drvData->humidityCfg->sensorInfo,
        sizeof(struct SensorBasicInfo), 0, sizeof(struct SensorBasicInfo));
    (void)memset_s(&drvData->humidityCfg->busCfg, sizeof(struct SensorBusCfg), 0, sizeof(struct SensorBusCfg));
    (void)memset_s(&drvData->humidityCfg->sensorAttr, sizeof(struct SensorAttr), 0, sizeof(struct SensorAttr));

    return drvData->humidityCfg;
}

void HumidityReleaseCfgData(struct SensorCfgData *humidityCfg)
{
    CHECK_NULL_PTR_RETURN(humidityCfg);

    (void)DeleteSensorDevice(&humidityCfg->sensorInfo);
    ReleaseSensorAllRegConfig(humidityCfg);
    (void)ReleaseSensorBusHandle(&humidityCfg->busCfg);
    ReleaseSensorDirectionConfig(humidityCfg);

    humidityCfg->root = NULL;
    (void)memset_s(&humidityCfg->sensorInfo, sizeof(struct SensorBasicInfo), 0, sizeof(struct SensorBasicInfo));
    (void)memset_s(&humidityCfg->busCfg, sizeof(struct SensorBusCfg), 0, sizeof(struct SensorBusCfg));
    (void)memset_s(&humidityCfg->sensorAttr, sizeof(struct SensorAttr), 0, sizeof(struct SensorAttr));
}

int32_t HumidityInitDriver(struct HdfDeviceObject *device)
{
    CHECK_NULL_PTR_RETURN_VALUE(device, HDF_ERR_INVALID_PARAM);
    struct HumidityDrvData *drvData = (struct HumidityDrvData *)device->service;
    CHECK_NULL_PTR_RETURN_VALUE(drvData, HDF_ERR_INVALID_PARAM);

    if (InitHumidityData(drvData) != HDF_SUCCESS) {
        HDF_LOGE("%s: Init Humidity config failed", __func__);
        return HDF_FAILURE;
    }

    drvData->humidityCfg = (struct SensorCfgData *)OsalMemCalloc(sizeof(*drvData->humidityCfg));
    if (drvData->humidityCfg == NULL) {
        HDF_LOGE("%s: Malloc Humidity config data failed", __func__);
        return HDF_FAILURE;
    }

    drvData->humidityCfg->regCfgGroup = &g_regCfgGroup[0];

    HDF_LOGI("%s: Init Humidity driver success", __func__);
    return HDF_SUCCESS;
}

void HumidityReleaseDriver(struct HdfDeviceObject *device)
{
    CHECK_NULL_PTR_RETURN(device);

    struct HumidityDrvData *drvData = (struct HumidityDrvData *)device->service;
    CHECK_NULL_PTR_RETURN(drvData);

    if (drvData->detectFlag && drvData->humidityCfg != NULL) {
        HumidityReleaseCfgData(drvData->humidityCfg);
    }

    OsalMemFree(drvData->humidityCfg);
    drvData->humidityCfg = NULL;

    HdfWorkDestroy(&drvData->humidityWork);
    HdfWorkQueueDestroy(&drvData->humidityWorkQueue);
    OsalMemFree(drvData);
}

struct HdfDriverEntry g_sensorHumidityDevEntry = {
    .moduleVersion = 1,
    .moduleName = "HDF_SENSOR_HUMIDITY",
    .Bind = HumidityBindDriver,
    .Init = HumidityInitDriver,
    .Release = HumidityReleaseDriver,
};

HDF_INIT(g_sensorHumidityDevEntry);