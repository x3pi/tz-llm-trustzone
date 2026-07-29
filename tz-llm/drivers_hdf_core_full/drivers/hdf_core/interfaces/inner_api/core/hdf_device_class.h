/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

/**
 * @addtogroup Core
 * @{
 *
 * @brief Provides Hardware Driver Foundation (HDF) APIs.
 *
 * The HDF implements driver framework capabilities such as driver loading, service management,
 * and driver message model. You can develop drivers based on the HDF.
 *
 * @since 1.0
 */

/**
 * @file hdf_device_class.h
 *
 * @brief Defines device types by service. Service modules can listen for services of the specific device type.
 *
 * @since 1.0
 */

#ifndef HDF_DEVICE_CLASS_H
#define HDF_DEVICE_CLASS_H

/**
 * @brief Enumerates the driver device types.
 */
typedef enum {
    DEVICE_CLASS_DEFAULT  = 0x1 << 0,    /** Default device type */
    DEVICE_CLASS_PLAT     = 0x1 << 1,    /** Platform device */
    DEVICE_CLASS_SENSOR   = 0x1 << 2,    /** Sensor device */
    DEVICE_CLASS_INPUT    = 0x1 << 3,    /** Input device */
    DEVICE_CLASS_DISPLAY  = 0x1 << 4,    /** Display device */
    DEVICE_CLASS_AUDIO    = 0x1 << 5,    /** Audio device */
    DEVICE_CLASS_CAMERA   = 0x1 << 6,    /** Camera device */
    DEVICE_CLASS_USB      = 0x1 << 7,    /** USB device */
    DEVICE_CLASS_USERAUTH = 0x1 << 8,    /** User authentication device */
    DEVICE_CLASS_MAX      = 0x1 << 9,    /** Maximum value of the device class */
} DeviceClass;

#endif /* HDF_DEVICE_CLASS_H */
/** @} */
