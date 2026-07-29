/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include <securec.h>
#include <hdf_log.h>
#include "camera_config_parser.h"
#include "camera_utils.h"
#include "camera_stream.h"
#include "camera_sensor.h"
#include "camera_isp.h"
#include "camera_vcm.h"
#include "camera_flash.h"
#include "camera_lens.h"
#include "camera_uvc.h"
#include "camera_common_device.h"

#define HDF_LOG_TAG HDF_CAMERA_COMMON_DEVICE

int32_t CommonDevicePowerOperation(struct CommonDevice *comDev, enum DevicePowerState power)
{
    int32_t ret;
    struct CameraDeviceDriver *deviceDriver = NULL;
    struct DeviceOps *devOps = NULL;

    deviceDriver = GetDeviceDriver(comDev->camDev);
    if (deviceDriver == NULL) {
        HDF_LOGE("%s: deviceDriver ptr is null!", __func__);
        return HDF_FAILURE;
    }
    ret = GetDeviceOps(deviceDriver, comDev->type, comDev->camId, comDev->driverName, &devOps);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: GetDeviceOps failed, ret = %{public}d", __func__, ret);
        return ret;
    }

    switch (power) {
        case CAMERA_DEVICE_POWER_UP:
            if (devOps->powerUp == NULL) {
                HDF_LOGE("%s: powerUp ptr is null!", __func__);
                return HDF_FAILURE;
            } else {
                return devOps->powerUp(deviceDriver);
            }
        case CAMERA_DEVICE_POWER_DOWN:
            if (devOps->powerDown == NULL) {
                HDF_LOGE("%s: powerDown ptr is null!", __func__);
                return HDF_FAILURE;
            } else {
                return devOps->powerDown(deviceDriver);
            }
        default:
            HDF_LOGE("%s: wrong power: %{public}d", __func__, power);
            return HDF_ERR_NOT_SUPPORT;
    }
}

static int32_t CommonDeviceFillSubDeviceConfig(int32_t type, struct CommonDevice *comDev, struct SubDevice *subDev)
{
    subDev->camDev = comDev->camDev;
    subDev->devId = comDev->devId;
    subDev->ctrlId = comDev->ctrlId;
    subDev->reqData = comDev->reqData;
    subDev->rspData = comDev->rspData;
    subDev->cameraHcsConfig = comDev->cameraHcsConfig;

    switch (type) {
        case SENSOR_TYPE:
            subDev->subDevOps = GetSensorDeviceOps();
            break;
        case ISP_TYPE:
            subDev->subDevOps = GetIspDeviceOps();
            break;
        case VCM_TYPE:
            subDev->subDevOps = GetVcmDeviceOps();
            break;
        case LENS_TYPE:
            subDev->subDevOps = GetLensDeviceOps();
            break;
        case FLASH_TYPE:
            subDev->subDevOps = GetFlashDeviceOps();
            break;
        case UVC_TYPE:
            subDev->subDevOps = GetUvcDeviceOps();
            break;
        default:
            HDF_LOGE("%s: wrong type: %{public}d", __func__, type);
            return HDF_ERR_NOT_SUPPORT;
    }
    return HDF_SUCCESS;
}

int32_t CommonDeviceQueryConfig(struct CommonDevice *comDev)
{
    int32_t ret;
    struct SubDevice subDev;

    ret = CameraDeviceGetCtrlConfig(comDev, &comDev->cameraHcsConfig, &comDev->devId);
    CHECK_RETURN_RET(ret);
    if (!HdfSbufReadUint32(comDev->reqData, &comDev->ctrlId)) {
        HDF_LOGE("%s: Read ctrlId failed!", __func__);
        return HDF_FAILURE;
    }
    CommonDeviceFillSubDeviceConfig(comDev->type, comDev, &subDev);
    ret = subDev.subDevOps->queryConfig(subDev);
    CHECK_RETURN_RET(ret);
    return HDF_SUCCESS;
}

int32_t CommonDeviceEnumDevice(struct CommonDevice *comDev)
{
    int32_t ret;
    struct SubDevice subDev;

    ret = CameraDeviceGetCtrlConfig(comDev, &comDev->cameraHcsConfig, &comDev->devId);
    CHECK_RETURN_RET(ret);
    CommonDeviceFillSubDeviceConfig(comDev->type, comDev, &subDev);
    ret = subDev.subDevOps->enumDevice(subDev);
    CHECK_RETURN_RET(ret);
    return HDF_SUCCESS;
}

int32_t CommonDeviceGetConfig(struct CommonDevice *comDev)
{
    int32_t ret;
    struct SubDevice subDev;

    ret = CameraDeviceGetCtrlConfig(comDev, &comDev->cameraHcsConfig, &comDev->devId);
    CHECK_RETURN_RET(ret);
    if (!HdfSbufReadUint32(comDev->reqData, &comDev->ctrlId)) {
        HDF_LOGE("%s: Read ctrlId failed!", __func__);
        return HDF_FAILURE;
    }
    CommonDeviceFillSubDeviceConfig(comDev->type, comDev, &subDev);
    ret = subDev.subDevOps->getConfig(subDev);
    CHECK_RETURN_RET(ret);
    return HDF_SUCCESS;
}

int32_t CommonDeviceSetConfig(struct CommonDevice *comDev)
{
    int32_t ret;
    struct SubDevice subDev;

    ret = CameraDeviceGetCtrlConfig(comDev, &comDev->cameraHcsConfig, &comDev->devId);
    CHECK_RETURN_RET(ret);
    CommonDeviceFillSubDeviceConfig(comDev->type, comDev, &subDev);
    ret = subDev.subDevOps->setConfig(subDev);
    CHECK_RETURN_RET(ret);
    return HDF_SUCCESS;
}
