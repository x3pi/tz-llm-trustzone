/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#ifndef SENSOR_HUMIDITY_DRIVER_H
#define SENSOR_HUMIDITY_DRIVER_H

#include "hdf_workqueue.h"
#include "osal_timer.h"
#include "sensor_config_parser.h"
#include "sensor_platform_if.h"

#define HUMIDITY_DEFAULT_SAMPLING_1000_MS    1000000000

struct HumidityData {
    int32_t humidity;
};

struct HumidityOpsCall {
    int32_t (*Init)(struct SensorCfgData *data);
    int32_t (*ReadData)(struct SensorCfgData *data);
};

struct HumidityDrvData {
    struct IDeviceIoService ioService;
    struct HdfDeviceObject *device;
    HdfWorkQueue humidityWorkQueue;
    HdfWork humidityWork;
    OsalTimer humidityTimer;
    bool detectFlag;
    bool enable;
    int64_t interval;
    struct SensorCfgData *humidityCfg;
    struct HumidityOpsCall ops;
};

int32_t HumidityRegisterChipOps(const struct HumidityOpsCall *ops);
struct SensorCfgData *HumidityCreateCfgData(const struct DeviceResourceNode *node);
void HumidityReleaseCfgData(struct SensorCfgData *humidityCfg);

#endif /* SENSOR_HUMIDITY_DRIVER_H */
