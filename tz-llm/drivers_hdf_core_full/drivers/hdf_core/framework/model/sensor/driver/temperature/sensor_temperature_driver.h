/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#ifndef SENSOR_TEMPERATURE_DRIVER_H
#define SENSOR_TEMPERATURE_DRIVER_H

#include "hdf_workqueue.h"
#include "osal_timer.h"
#include "sensor_config_parser.h"
#include "sensor_platform_if.h"

#define TEMPERATURE_DEFAULT_SAMPLING_1000_MS    1000000000

struct TemperatureData {
    int32_t temperature;
};

struct TemperatureOpsCall {
    int32_t (*Init)(struct SensorCfgData *data);
    int32_t (*ReadData)(struct SensorCfgData *data);
};

struct TemperatureDrvData {
    struct IDeviceIoService ioService;
    struct HdfDeviceObject *device;
    HdfWorkQueue temperatureWorkQueue;
    HdfWork temperatureWork;
    OsalTimer temperatureTimer;
    bool detectFlag;
    bool enable;
    int64_t interval;
    struct SensorCfgData *temperatureCfg;
    struct TemperatureOpsCall ops;
};

int32_t TemperatureRegisterChipOps(const struct TemperatureOpsCall *ops);
struct SensorCfgData *TemperatureCreateCfgData(const struct DeviceResourceNode *node);
void TemperatureReleaseCfgData(struct SensorCfgData *temperatureCfg);

#endif /* SENSOR_TEMPERATURE_DRIVER_H */
