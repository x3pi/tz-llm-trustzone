/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#ifndef CAMERA_UTILS_H
#define CAMERA_UTILS_H

#include <utils/hdf_base.h>
#include <utils/hdf_sbuf.h>
#include <core/hdf_device_desc.h>
#include "camera_device_manager.h"

#define ID_MAX_NUM 10

bool FindAvailableId(uint32_t *id);
int32_t AddPermissionId(int32_t id, int32_t permissionId);
int32_t RemovePermissionId(int32_t id);
int32_t GetCameraId(const char *str, int length, int *id);
struct CameraDeviceDriver *GetDeviceDriver(const struct CameraDevice *cameraDev);
int32_t CheckPermission(int permissionId);
int32_t CheckCameraDevice(const char *deviceName, int type);
struct SensorDevice *GetSensorDevice(const char *kernelDrvName, struct CameraDeviceDriver *regDev);
struct IspDevice *GetIspDevice(const char *kernelDrvName, struct CameraDeviceDriver *regDev);
struct LensDevice *GetLensDevice(const char *kernelDrvName, struct CameraDeviceDriver *regDev);
struct FlashDevice *GetFlashDevice(const char *kernelDrvName, struct CameraDeviceDriver *regDev);
struct VcmDevice *GetVcmDevice(const char *kernelDrvName, struct CameraDeviceDriver *regDev);
struct UvcDevice *GetUvcDevice(const char *kernelDrvName, struct CameraDeviceDriver *regDev);
struct StreamDevice *GetStreamDevice(const char *kernelDrvName, struct CameraDeviceDriver *regDev);
int32_t GetDeviceNum(const char *driverName, int camId, int type);
int32_t GetStreamNum(const char *driverName, struct CameraDeviceDriver *deviceDriver);
int32_t GetUvcNum(const char *driverName, struct CameraDeviceDriver *deviceDriver);
int32_t CheckFrameRate(int camId, const char *driverName, int type, struct CameraCtrlConfig *ctrlConfig);
int32_t CameraGetDeviceInfo(int type, struct HdfSBuf *reqData,
    const char **driverName, int32_t *camId, struct CameraDevice **camDev);
int32_t CameraGetStreamDevInfo(int type, struct HdfSBuf *reqData, struct CameraDevice **camDev);
int32_t CameraGetNames(int type, struct HdfSBuf *reqData, const char **deviceName, const char **driverName);
int32_t GetDeviceOps(struct CameraDeviceDriver *deviceDriver,
    int type, int32_t camId, const char *driverName, struct DeviceOps **devOps);
int32_t CameraDeviceGetCtrlConfig(struct CommonDevice *comDev,
    struct CameraDeviceConfig **deviceConfig, int32_t *deviceId);
int32_t CameraGetDevice(struct HdfSBuf *reqData, struct CommonDevice *comDev);
int32_t CameraDispatchCommonInfo(const struct HdfDeviceIoClient *client,
    struct HdfSBuf *reqData, struct HdfSBuf *rspData, struct CommonDevice *comDev);

#endif  // CAMERA_UTILS_H