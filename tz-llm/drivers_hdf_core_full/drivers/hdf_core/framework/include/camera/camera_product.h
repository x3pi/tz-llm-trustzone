/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#ifndef CAMERA_PRODUCT_H
#define CAMERA_PRODUCT_H

#include <utils/hdf_base.h>

#define CAMERA_MAX_CMD_ID    23

enum CameraDeviceType {
    SENSOR_TYPE,
    ISP_TYPE,
    VCM_TYPE,
    LENS_TYPE,
    FLASH_TYPE,
    UVC_TYPE,
    STREAM_TYPE,
};

enum CameraQueryMemeryFlags {
    RELEASE_FLAG,
    ALLOC_FLAG,
};

typedef enum {
    CAMERA_UVC_EVENT,
} CameraReceiveEventType;

enum CameraMethodCmd {
    CMD_OPEN_CAMERA,
    CMD_CLOSE_CAMERA,
    CMD_POWER_UP,
    CMD_POWER_DOWN,
    CMD_QUERY_CONFIG,
    CMD_GET_CONFIG,
    CMD_SET_CONFIG,
    CMD_GET_FMT,
    CMD_SET_FMT,
    CMD_GET_CROP,
    CMD_SET_CROP,
    CMD_GET_FPS,
    CMD_SET_FPS,
    CMD_ENUM_FMT,
    CMD_ENUM_DEVICES,
    CMD_GET_ABILITY,
    CMD_QUEUE_INIT,
    CMD_REQ_MEMORY,
    CMD_QUERY_MEMORY,
    CMD_STREAM_QUEUE,
    CMD_STREAM_DEQUEUE,
    CMD_STREAM_ON,
    CMD_STREAM_OFF,
};

enum CameraPixFmt {
    CAMERA_PIX_FMT_UNDIFINED = 0,

    CAMERA_PIX_FMT_YUV420 = 1,
    CAMERA_PIX_FMT_YUV422 = 2,
    CAMERA_PIX_FMT_YUV444 = 3,

    CAMERA_PIX_FMT_RGB565 = 100,
    CAMERA_PIX_FMT_RGB444 = 101,
    CAMERA_PIX_FMT_RGB555 = 102,
    CAMERA_PIX_FMT_RGB32 = 103,

    CAMERA_PIX_FMT_SRGGB8 = 200,
    CAMERA_PIX_FMT_SRGGB10 = 201,
    CAMERA_PIX_FMT_SRGGB12 = 202,
    CAMERA_PIX_FMT_SBGGR8 = 203,
    CAMERA_PIX_FMT_SBGGR10 = 204,
    CAMERA_PIX_FMT_SBGGR12 = 205,

    CAMERA_PIX_FMT_JPEG = 300,
    CAMERA_PIX_FMT_MJPEG = 301,
    CAMERA_PIX_FMT_MPEG = 302,
    CAMERA_PIX_FMT_MPEG1 = 303,
    CAMERA_PIX_FMT_MPEG2 = 304,
    CAMERA_PIX_FMT_MPEG4 = 305,

    CAMERA_PIX_FMT_H264 = 400,
    CAMERA_PIX_FMT_H264_NO_SC = 401,
    CAMERA_PIX_FMT_H264_MVC = 402,
};

enum CameraCmdEnumTypeFmt {
    CAMERA_CMD_ENUM_FMT,
    CAMERA_CMD_ENUM_FRAMESIZES,
    CAMERA_CMD_ENUM_FRAMEINTERVALS,
};

enum CameraPermissionId {
    CAMERA_MASTER = 1,
    CAMERA_SLAVE0,
    CAMERA_SLAVE1,
    CAMERA_SLAVE2,
    CAMERA_SLAVE3,
    CAMERA_SLAVE4,
    CAMERA_SLAVE5,
    CAMERA_SLAVE6,
    CAMERA_SLAVE7,
    CAMERA_SLAVE8,
};

enum CmdGetConfigValue {
    CAMERA_CMD_GET_EXPOSURE,                  /* SENSOR exposure time query */
    CAMERA_CMD_GET_WHITE_BALANCE_MODE,        /* White balance mode query */
    CAMERA_CMD_GET_WHITE_BALANCE,             /* White balance value query */

    CAMERA_CMD_GET_BRIGHTNESS,                /* Brightness query */
    CAMERA_CMD_GET_CONTRAST,                  /* Contrast query */
    CAMERA_CMD_GET_SATURATION,                /* Saturation query */
    CAMERA_CMD_GET_HUE,                       /* Hue query */
    CAMERA_CMD_GET_SHARPNESS,                 /* Sharpness query */
    CAMERA_CMD_GET_GAIN,                      /* Gain query */
    CAMERA_CMD_GET_GAMMA,                     /* Gamma query */
    CAMERA_CMD_GET_HFLIP,                     /* Horizontal mirror query */
    CAMERA_CMD_GET_VFLIP,                     /* Vertical mirror query */

    CAMERA_CMD_GET_FOCUS_MODE,                /* Focus mode query */
    CAMERA_CMD_GET_FOCUS_ABSOLUTE,            /* Query of absolute value of focusing position */
    CAMERA_CMD_GET_ZOOM_ABSOLUTE,             /* Zoom position absolute value query */
    CAMERA_CMD_GET_ZOOM_CONTINUOUS,           /* Continuous zoom query */
    CAMERA_CMD_GET_IRIS_ABSOLUTE,             /* Aperture absolute value query */
    CAMERA_CMD_GET_IRIS_RELATIVE,             /* Aperture relative value query */

    CAMERA_CMD_GET_FLASH_FAULT,               /* Flash error query */
    CAMERA_CMD_GET_FLASH_READY,               /* Flash ready query */
};

enum CmdSetConfigValue {
    CAMERA_CMD_SET_EXPOSURE,                  /* Sensor exposure time setting */
    CAMERA_CMD_SET_WHITE_BALANCE_MODE,        /* White balance mode setting */
    CAMERA_CMD_SET_WHITE_BALANCE,             /* White balance value setting */

    CAMERA_CMD_SET_BRIGHTNESS,                /* Brightness settings */
    CAMERA_CMD_SET_CONTRAST,                  /* Contrast settings */
    CAMERA_CMD_SET_SATURATION,                /* Saturation settings */
    CAMERA_CMD_SET_HUE,                       /* Hue settings */
    CAMERA_CMD_SET_SHARPNESS,                 /* Sharpness settings */

    CAMERA_CMD_SET_GAIN,                      /* Gain value setting */
    CAMERA_CMD_SET_GAMMA,                     /* Gamma value setting */
    CAMERA_CMD_SET_HFLIP,                     /* Horizontal mirror settings */
    CAMERA_CMD_SET_VFLIP,                     /* Vertical mirror settings */

    CAMERA_CMD_SET_FOCUS_MODE,                /* Focus mode setting */
    CAMERA_CMD_SET_FOCUS_ABSOLUTE,            /* Focus position absolute value setting */
    CAMERA_CMD_SET_FOCUS_RELATIVE,            /* Setting of relative value of focusing position */
    CAMERA_CMD_SET_ZOOM_ABSOLUTE,             /* Zoom position absolute value setting */
    CAMERA_CMD_SET_ZOOM_RELATIVE,             /* Relative value setting of zoom position */
    CAMERA_CMD_SET_ZOOM_CONTINUOUS,           /* Continuous zoom setting */
    CAMERA_CMD_SET_IRIS_ABSOLUTE,             /* Aperture absolute value setting */
    CAMERA_CMD_SET_IRIS_RELATIVE,             /* Aperture relative value setting */

    CAMERA_CMD_SET_FLASH_STROBE,              /* Flash settings setting */
    CAMERA_CMD_SET_FLASH_INTENSITY,           /* Flash intensity setting */
};

enum CameraExposureAutoType {
    CAMERA_EXPOSURE_AUTO = 0,
    CAMERA_EXPOSURE_MANUAL,
    CAMERA_EXPOSURE_SHUTTER_PRIORITY,
    CAMERA_EXPOSURE_APERTURE_PRIORITY,
};

enum CameraWhitleBalanceAutoType {
    CAMERA_WHITE_BALANCE_AUTO = 0,
    CAMERA_WHITE_BALANCE_MANUAL,
};

enum CameraFocusAutoType {
    CAMERA_FOCUS_AUTO = 0,
    CAMERA_FOCUS_MANUAL,
};

enum CameraFlashMode {
    CAMERA_FLASH_STROBE = 0,
    CAMERA_FLASH_STROBE_STOP,
};

enum CameraIrisAbsolute {
    CAMERA_IRIS_F1 = 0,
    CAMERA_IRIS_F1P2,
    CAMERA_IRIS_F1P4,
    CAMERA_IRIS_F2,
    CAMERA_IRIS_F2P8,
    CAMERA_IRIS_F4,
    CAMERA_IRIS_F5P6,
    CAMERA_IRIS_F8,
    CAMERA_IRIS_F11,
    CAMERA_IRIS_F16,
    CAMERA_IRIS_F22,
    CAMERA_IRIS_F32,
    CAMERA_IRIS_F44,
    CAMERA_IRIS_F64,
};

enum CameraZoomContinuous {
    CAMERA_ZOOM_WIDE = 0,
    CAMERA_ZOOM_TELE,
    CAMERA_ZOOM_STOP,
};

struct UserCameraBuffer {
    uint32_t id;
    uint32_t flags;
    uint32_t field;
    uint64_t timeStamp;
    uint32_t sequence;
    uint32_t memType;
    struct UserCameraPlane *planes;
    uint32_t planeCount;
};

struct UserCameraPlane {
    uint32_t bytesUsed;
    uint32_t length;
    union {
        uint32_t offset;
        uint64_t userPtr;
        uint32_t fd;
    } memory;
    unsigned long vaddr;
    uint32_t dataOffset;
};

struct UserCameraReq {
    uint32_t count;
    uint32_t memType;
    uint32_t capabilities;
};

enum CameraMemType {
    MEMTYPE_UNKNOWN = 0,
    MEMTYPE_MMAP = (1 << 0),
    MEMTYPE_USERPTR = (1 << 1),
    MEMTYPE_DMABUF = (1 << 2),
};

/* UserCameraBuffer flags */
enum UserBufferFlags {
    USER_BUFFER_QUEUED = (1 << 0),
    USER_BUFFER_DONE = (1 << 1),
    USER_BUFFER_ERROR = (1 << 2),
    USER_BUFFER_LAST = (1 << 3),
    USER_BUFFER_BLOCKING = (1 << 4),
};

#endif  // CAMERA_PRODUCT_H