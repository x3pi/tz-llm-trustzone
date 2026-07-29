/*
 * Copyright (c) 2021-2023 Huawei Device Co., Ltd.
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
 * @file hdf_service_status.h
 *
 * @brief Defines data structs for the service status and service status listener callback.
 *
 * @since 1.0
 */

#ifndef HDF_SERVICE_STATUS_H
#define HDF_SERVICE_STATUS_H

#include "hdf_types.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

struct ServiceStatusListener;

/**
 * @brief Enumerates the service status.
 */
enum ServiceStatusType {
    /** The service is started. */
    SERVIE_STATUS_START,
    /** The service status changes. */
    SERVIE_STATUS_CHANGE,
    /** The service is stopped. */
    SERVIE_STATUS_STOP,
    /** Maximum value of the service status. */
    SERVIE_STATUS_MAX,
};

/**
 * @brief Defines the service status struct.
 * The HDF uses this struct to notify the service module of the service status.
 */
struct ServiceStatus {
    /** Service name */
    const char* serviceName;
    /** Device type */
    uint16_t deviceClass;
    /** Service status */
    uint16_t status;
    /** Service information */
    const char *info;
};

/**
 * @brief Defines the function for service status listening.
 *
 * @param listener Indicates the pointer to the service status listener.
 * @param status Indicates the pointer to the service status obtained.
 */
typedef void (*OnServiceStatusReceived)(struct ServiceStatusListener *listener, struct ServiceStatus *status);

/**
 * @brief Defines the service status listener struct.
 */
struct ServiceStatusListener {
    /** Callback invoked to return the service status */
    OnServiceStatusReceived callback;
    /**
     * Pointer to the private parameter of the service module.
     * The service module can pass this parameter to the callback to convert
     * the HDF service status to the required type.
     */
    void *priv;
};

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* HDF_SERVICE_STATUS_H */