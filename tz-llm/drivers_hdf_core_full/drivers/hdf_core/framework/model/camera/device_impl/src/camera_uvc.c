/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include <securec.h>
#include <hdf_log.h>
#include "camera_buffer_manager.h"
#include "camera_config_parser.h"
#include "camera_event.h"
#include "camera_utils.h"
#include "camera_uvc.h"

#define HDF_LOG_TAG HDF_CAMERA_UVC

static int32_t CameraCmdUvcCheckDeviceOps(struct CameraDeviceDriver *deviceDriver, int32_t devId)
{
    if (deviceDriver == NULL) {
        HDF_LOGE("%s: deviceDriver ptr is null!", __func__);
        return HDF_FAILURE;
    }
    if (deviceDriver->uvc[devId] == NULL) {
        HDF_LOGE("%s: uvc dev is null!", __func__);
        return HDF_FAILURE;
    }
    if (deviceDriver->uvc[devId]->devOps == NULL) {
        HDF_LOGE("%s: uvc dev ops is null!", __func__);
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

int32_t CameraCmdUvcQueryConfig(struct SubDevice subDev)
{
    int32_t ret;
    bool isFailed = false;
    int32_t devId = subDev.devId;
    uint32_t ctrlId = subDev.ctrlId;
    struct CameraDevice *camDev = subDev.camDev;
    struct HdfSBuf *rspData = subDev.rspData;
    struct UvcQueryCtrl queryctrl;
    struct CameraDeviceDriver *deviceDriver = GetDeviceDriver(camDev);

    ret = CameraCmdUvcCheckDeviceOps(deviceDriver, devId);
    CHECK_RETURN_RET(ret);
    queryctrl.id = ctrlId;
    deviceDriver->uvc[devId]->devOps->uvcQueryCtrl(deviceDriver, &queryctrl);
    HDF_LOGD("%s: queryctrl = %{public}d | %{public}d| %{public}d |%{public}d| %{public}d \n", __func__,
        queryctrl.id, queryctrl.maximum, queryctrl.minimum, queryctrl.step, queryctrl.defaultValue);
    isFailed |= !HdfSbufWriteUint32(rspData, queryctrl.id);
    isFailed |= !HdfSbufWriteUint32(rspData, queryctrl.maximum);
    isFailed |= !HdfSbufWriteUint32(rspData, queryctrl.minimum);
    isFailed |= !HdfSbufWriteUint32(rspData, queryctrl.step);
    isFailed |= !HdfSbufWriteUint32(rspData, queryctrl.defaultValue);
    CHECK_RETURN_RET(isFailed);
    return HDF_SUCCESS;
}

int32_t CameraCmdUvcSetConfig(struct SubDevice subDev)
{
    int32_t ret;
    int32_t isFailed = 0;
    int32_t devId = subDev.devId;
    struct HdfSBuf *reqData = subDev.reqData;
    struct CameraCtrlConfig ctrlConfig = {};
    struct CameraDeviceDriver *deviceDriver = GetDeviceDriver(subDev.camDev);

    ret = CameraCmdUvcCheckDeviceOps(deviceDriver, devId);
    CHECK_RETURN_RET(ret);
    isFailed |= !HdfSbufReadUint32(reqData, &ctrlConfig.ctrl.id);
    isFailed |= !HdfSbufReadUint32(reqData, &ctrlConfig.ctrl.value);
    CHECK_RETURN_RET(isFailed);
    return deviceDriver->uvc[devId]->devOps->setConfig(deviceDriver, &ctrlConfig);
}

int32_t CameraCmdUvcGetConfig(struct SubDevice subDev)
{
    int32_t ret;
    int32_t devId = subDev.devId;
    struct HdfSBuf *rspData = subDev.rspData;
    struct CameraCtrlConfig ctrlConfig = {};
    struct CameraDeviceDriver *deviceDriver = GetDeviceDriver(subDev.camDev);

    ret = CameraCmdUvcCheckDeviceOps(deviceDriver, devId);
    CHECK_RETURN_RET(ret);
    ctrlConfig.ctrl.id = subDev.ctrlId;
    ret = deviceDriver->uvc[devId]->devOps->getConfig(deviceDriver, &ctrlConfig);
    if (!HdfSbufWriteUint32(rspData, ctrlConfig.ctrl.value)) {
        HDF_LOGE("%s: Write ctrl value failed!", __func__);
        return HDF_FAILURE;
    }
    return ret;
}

int32_t CameraCmdUvcEnumDevice(struct SubDevice subDev)
{
    int32_t ret;
    bool isFailed = false;
    int32_t devId = subDev.devId;
    struct CameraDevice *camDev = subDev.camDev;
    struct HdfSBuf *rspData = subDev.rspData;
    struct CameraDeviceDriver *deviceDriver = GetDeviceDriver(camDev);

    ret = CameraCmdUvcCheckDeviceOps(deviceDriver, devId);
    CHECK_RETURN_RET(ret);
    isFailed |= !HdfSbufWriteString(rspData, deviceDriver->uvc[devId]->kernelDrvName);
    CHECK_RETURN_RET(isFailed);
    return HDF_SUCCESS;
}

struct DeviceConfigOps g_uvcDeviceOps = {
    .getConfig = &CameraCmdUvcGetConfig,
    .setConfig = &CameraCmdUvcSetConfig,
    .enumDevice = &CameraCmdUvcEnumDevice,
    .queryConfig = &CameraCmdUvcQueryConfig,
};

struct DeviceConfigOps *GetUvcDeviceOps(void)
{
    return &g_uvcDeviceOps;
}
