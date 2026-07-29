/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#ifndef CAMERA_DEVICE_MANAGER_H
#define CAMERA_DEVICE_MANAGER_H

#include <utils/device_resource_if.h>
#include <camera/camera_product.h>
#include "camera_buffer_manager.h"
#include "camera_buffer_manager_adapter.h"
#include "camera_config_parser.h"

#define DEVICE_NAME_SIZE 20
#define DEVICE_DRIVER_MAX_NUM 20
#define CAMERA_COMPONENT_NAME_MAX_LEN 20
#define DRIVER_NAME_SIZE 20
#define DEVICE_NUM 3

#define CHECK_RETURN_RET(ret) do { \
    if ((ret) != 0) { \
        HDF_LOGE("%s: failed, ret = %{public}d, line: %{public}d", __func__, ret, __LINE__); \
        return ret; \
    } \
} while (0)

enum DevicePowerState {
    CAMERA_DEVICE_POWER_DOWN = 0,
    CAMERA_DEVICE_POWER_UP = 1
};

struct UvcQueryCtrl {
    uint32_t id;
    uint32_t minimum;
    uint32_t maximum;
    uint32_t step;
    uint32_t defaultValue;
};

struct Pixel {
    char description[32];
    uint32_t format;
    uint32_t width;
    uint32_t height;
    uint32_t maxWidth;
    uint32_t maxHeight;
    uint32_t minWidth;
    uint32_t minHeight;
    uint32_t sizeImage;
};

struct FPS {
    uint32_t numerator;
    uint32_t denominator;
};

struct CameraControl {
    uint32_t id;
    uint32_t value;
};

struct Rect {
    int32_t left;
    int32_t top;
    uint32_t width;
    uint32_t height;
};

struct Capability {
    char driver[16];
    char card[32];
    char busInfo[32];
    uint32_t capabilities;
};

struct PixelFormat {
    struct FPS fps;
    struct Pixel pixel;
    struct Rect crop;
};

struct CameraCtrlConfig {
    struct PixelFormat pixelFormat;
    struct CameraControl ctrl;
};

struct EnumPixelFormatData {
    int32_t index;
    struct PixelFormat pixelFormat;
};

struct CameraDeviceDriverFactory {
    const char *deviceName;
    void (*releaseFactory)(struct CameraDeviceDriverFactory *factory);
    struct CameraDeviceDriver *(*build)(const char *deviceName);
    void (*release)(struct CameraDeviceDriver *deviceDriver);
};

struct CameraDevice {
    char deviceName[DEVICE_NAME_SIZE];
    struct CameraDeviceDriver *deviceDriver;
};

struct DeviceOps {
    int32_t (*powerUp)(struct CameraDeviceDriver *regDev);
    int32_t (*powerDown)(struct CameraDeviceDriver *regDev);
    int32_t (*setConfig)(struct CameraDeviceDriver *regDev, struct CameraCtrlConfig *config);
    int32_t (*getConfig)(struct CameraDeviceDriver *regDev, struct CameraCtrlConfig *config);
    int32_t (*uvcQueryCtrl)(struct CameraDeviceDriver *regDev, struct UvcQueryCtrl *query);
};

struct SensorDevice {
    char kernelDrvName[DRIVER_NAME_SIZE];
    struct DeviceOps *devOps;
};

struct IspDevice {
    char kernelDrvName[DRIVER_NAME_SIZE];
    struct DeviceOps *devOps;
};

struct VcmDevice {
    char kernelDrvName[DRIVER_NAME_SIZE];
    struct DeviceOps *devOps;
};

struct LensDevice {
    char kernelDrvName[DRIVER_NAME_SIZE];
    struct DeviceOps *devOps;
};

struct FlashDevice {
    char kernelDrvName[DRIVER_NAME_SIZE];
    struct DeviceOps *devOps;
};

struct StreamDevice {
    char kernelDrvName[DRIVER_NAME_SIZE];
    struct StreamOps *streamOps;
    struct BufferQueueImp queueImp;
};

struct UvcDevice {
    char kernelDrvName[DRIVER_NAME_SIZE];
    struct DeviceOps *devOps;
};

struct StreamOps {
    int32_t (*streamSetFormat)(struct CameraCtrlConfig *config, struct StreamDevice *streamDev);
    int32_t (*streamGetFormat)(struct CameraCtrlConfig *config, struct StreamDevice *streamDev);
    int32_t (*streamSetCrop)(struct CameraCtrlConfig *config, struct StreamDevice *streamDev);
    int32_t (*streamGetCrop)(struct CameraCtrlConfig *config, struct StreamDevice *streamDev);
    int32_t (*streamSetFps)(struct CameraCtrlConfig *config, struct StreamDevice *streamDev);
    int32_t (*streamGetFps)(struct CameraCtrlConfig *config, struct StreamDevice *streamDev);
    int32_t (*streamGetAbility)(struct Capability *capability, struct StreamDevice *streamDev);
    int32_t (*streamEnumFormat)(struct PixelFormat *config, struct StreamDevice *streamDev,
        uint32_t index, uint32_t cmd);
    int32_t (*startStreaming)(struct StreamDevice *dev);
    void (*stopStreaming)(struct StreamDevice *streamDev);
    int32_t (*streamQueueInit)(struct StreamDevice *streamDev);
};

struct CameraDeviceDriverManager {
    struct CameraDeviceDriverFactory **deviceFactoryInsts;
    int32_t (*regDeviceDriverFactory)(struct CameraDeviceDriverFactory *factoryInst);
    struct CameraDeviceDriverFactory *(*getDeviceDriverFactoryByName)(const char *name);
};

struct CameraDeviceDriver {
    char name[CAMERA_COMPONENT_NAME_MAX_LEN];
    struct SensorDevice *sensor[DEVICE_NUM];
    struct IspDevice *isp[DEVICE_NUM];
    struct VcmDevice *vcm[DEVICE_NUM];
    struct LensDevice *lens[DEVICE_NUM];
    struct FlashDevice *flash[DEVICE_NUM];
    struct UvcDevice *uvc[DEVICE_NUM];
    struct StreamDevice *stream[DEVICE_NUM];
    int32_t (*init)(struct CameraDeviceDriver *deviceDriver, struct CameraDevice *camDev);
    int32_t (*deinit)(struct CameraDeviceDriver *deviceDriver, struct CameraDevice *camDev);
};

struct CommonDevice {
    int32_t type;
    int32_t permissionId;
    const char *driverName;
    struct CameraDevice *camDev;
    int32_t camId;
    int32_t devId;
    uint32_t ctrlId;
    struct HdfSBuf *reqData;
    struct HdfSBuf *rspData;
    struct CameraDeviceConfig *cameraHcsConfig;
};

struct CameraDevice *CameraDeviceCreate(const char *deviceName, uint32_t len);
int32_t CameraDeviceRelease(struct CameraDevice *camDev);
struct CameraDeviceDriverFactory *CameraDeviceDriverFactoryGetByName(const char *deviceName);
int32_t CameraDeviceDriverFactoryRegister(struct CameraDeviceDriverFactory *obj);
int32_t DeviceDriverManagerDeInit(void);
struct CameraDevice *CameraDeviceGetByName(const char *deviceName);
struct CameraDeviceDriverManager *CameraDeviceDriverManagerGet(void);

#endif  // CAMERA_DEVICE_MANAGER_H