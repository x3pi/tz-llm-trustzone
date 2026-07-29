/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#ifndef CAMERA_CONFIG_PARSER_H
#define CAMERA_CONFIG_PARSER_H

#include <utils/device_resource_if.h>
#include <utils/hdf_base.h>

#define BUFFER_TYPE_MAX_NUM 3
#define FORMAT_TYPE_MAX_NUM 23
#define CAMERA_MAX_NUM 16
#define CAMERA_DEVICE_MAX_NUM 4
#define CTRL_INFO_COUNT 5
#define CTRL_ID_INDEX 0
#define CTRL_MAX_INDEX 1
#define CTRL_MIN_INDEX 2
#define CTRL_STEP_INDEX 3
#define CTRL_DEF_INDEX 4
#define CTRL_MAX_NUM 21
#define CTRL_VALUE_MAX_NUM CTRL_MAX_NUM * CTRL_INFO_COUNT
#define DEVICE_SUPPORT 1
#define DEVICE_NOT_SUPPORT 0

struct CtrlCapInfo {
    uint32_t ctrlId;
    uint32_t max;
    uint32_t min;
    uint32_t step;
    uint32_t def;
};

struct SensorDeviceConfig {
    const char *name;
    uint8_t id;
    uint8_t exposure;
    uint8_t mirror;
    uint8_t gain;
    uint32_t ctrlValueNum;
    uint32_t ctrlValue[CTRL_VALUE_MAX_NUM];
    struct CtrlCapInfo ctrlCap[CTRL_MAX_NUM];
};

struct IspDeviceConfig {
    const char *name;
    uint8_t id;
    uint8_t brightness;
    uint8_t contrast;
    uint8_t saturation;
    uint8_t hue;
    uint8_t sharpness;
    uint8_t gain;
    uint8_t gamma;
    uint8_t whiteBalance;
    uint32_t ctrlValueNum;
    uint32_t ctrlValue[CTRL_VALUE_MAX_NUM];
    struct CtrlCapInfo ctrlCap[CTRL_MAX_NUM];
};

struct LensDeviceConfig {
    const char *name;
    uint8_t id;
    uint8_t aperture;
    uint32_t ctrlValueNum;
    uint32_t ctrlValue[CTRL_VALUE_MAX_NUM];
    struct CtrlCapInfo ctrlCap[CTRL_MAX_NUM];
};

struct VcmDeviceConfig {
    const char *name;
    uint8_t id;
    uint8_t focus;
    uint8_t autoFocus;
    uint8_t zoom;
    uint32_t zoomMaxNum;
    uint32_t ctrlValueNum;
    uint32_t ctrlValue[CTRL_VALUE_MAX_NUM];
    struct CtrlCapInfo ctrlCap[CTRL_MAX_NUM];
};

struct FlashDeviceConfig {
    const char *name;
    uint8_t id;
    uint8_t flashMode;
    uint8_t flashIntensity;
    uint32_t ctrlValueNum;
    uint32_t ctrlValue[CTRL_VALUE_MAX_NUM];
    struct CtrlCapInfo ctrlCap[CTRL_MAX_NUM];
};

struct StreamDeviceConfig {
    const char *name;
    uint8_t id;
    uint32_t heightMaxNum;
    uint32_t widthMaxNum;
    uint32_t frameRateMaxNum;
    uint8_t bufferCount;
    uint32_t bufferTypeNum;
    uint32_t bufferType[BUFFER_TYPE_MAX_NUM];
    uint32_t formatTypeNum;
    uint32_t formatType[FORMAT_TYPE_MAX_NUM];
};

struct CameraSensorConfig {
    uint8_t mode;
    uint16_t sensorNum;
    struct SensorDeviceConfig sensor[CAMERA_DEVICE_MAX_NUM];
};

struct CameraIspConfig {
    uint8_t mode;
    uint16_t ispNum;
    struct IspDeviceConfig isp[CAMERA_DEVICE_MAX_NUM];
};

struct CameraLensConfig {
    uint8_t mode;
    uint16_t lensNum;
    struct LensDeviceConfig lens[CAMERA_DEVICE_MAX_NUM];
};

struct CameraVcmConfig {
    uint8_t mode;
    uint16_t vcmNum;
    struct VcmDeviceConfig vcm[CAMERA_DEVICE_MAX_NUM];
};

struct CameraFlashConfig {
    uint8_t mode;
    uint16_t flashNum;
    struct FlashDeviceConfig flash[CAMERA_DEVICE_MAX_NUM];
};

struct CameraStreamConfig {
    uint8_t mode;
    uint16_t streamNum;
    struct StreamDeviceConfig stream[CAMERA_DEVICE_MAX_NUM];
};

struct CameraDeviceConfig {
    const char *deviceName;
    struct CameraSensorConfig sensor;
    struct CameraIspConfig isp;
    struct CameraLensConfig lens;
    struct CameraVcmConfig vcm;
    struct CameraFlashConfig flash;
    struct CameraStreamConfig stream;
};

struct CameraConfigRoot {
    uint8_t uvcMode;
    uint16_t deviceNum;
    struct CameraDeviceConfig deviceConfig[CAMERA_MAX_NUM];
};

struct CameraConfigRoot *HdfCameraGetConfigRoot(void);
int32_t HdfParseCameraConfig(const struct DeviceResourceNode *node);

#endif  // CAMERA_CONFIG_PARSER_H
