/*
 * Copyright (c) 2021-2023 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * @addtogroup DriverHdi
 * @{
 *
 * @brief Provides APIs for a system ability to obtain hardware device interface (HDI) services,
 * load or unload a device, and listen for service status, and capabilities for the hdi-gen tool to
 * automatically generate code in the interface description language (IDL).
 *
 * The HDF and the IDL code generated allow system abilities to access the HDI driver service.
 *
 * @since 1.0
 */

/**
 * @file servmgr_hdi.h
 *
 * @brief Defines the data structs and interface types related to service management based on C.
 *
 * @since 1.0
 */

#ifndef HDI_SERVICE_MANAGER_INF_H
#define HDI_SERVICE_MANAGER_INF_H

#include "hdf_remote_service.h"
#include "servstat_listener_hdi.h"
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * @brief Defines a set of HDI services for system abilities.
 */
struct HdiServiceSet {
    /** HDI service names */
    const char **serviceNames;
    /** Number of HDI services. */
    uint32_t count;
};

/**
 * @brief Defines the HDI APIs for service management. Developers using C can use the APIs to obtain a service or
 * service set and register a service status listener.
 */
struct HDIServiceManager {
    /**
     * @brief Obtain an HDI service.
     *
     * @param self Indicates the pointer to the service manager object.
     * @param serviceName Indicates the pointer to the HDI service name to obtain.
     * @return Returns the HDI service name obtained.
     */
    struct HdfRemoteService *(*GetService)(struct HDIServiceManager *self, const char *serviceName);

    /**
     * @brief Registers a listener for observing the service status.
     *
     * @param self Indicates the pointer to the service manager object.
     * @param listener Indicates the listener object to register.
     * @param deviceClass Indicates the service type of the device to be observed.
     * @return Returns <b>HDF_SUCCESS</b> if the operation is successful; otherwise, the operation fails.
     */
    int32_t (*RegisterServiceStatusListener)(
        struct HDIServiceManager *self, struct ServiceStatusListener *listener, uint16_t deviceClass);

    /**
     * @brief Unregisters a service status listener.
     *
     * @param self Indicates the pointer to the service manager object.
     * @param listener Indicates the pointer to the listener object to unregister.
     * @return Returns <b>HDF_SUCCESS</b> if the operation is successful; otherwise, the operation fails.
     */
    int32_t (*UnregisterServiceStatusListener)(struct HDIServiceManager *self, struct ServiceStatusListener *listener);

    /**
     * @brief Obtains the HDI service set based on the interface descriptor.
     *
     * @param interfaceName Indicates the pointer to the interface descriptor.
     * @return Returns the service names obtained.
     */
    struct HdiServiceSet *(*ListServiceByInterfaceDesc)(struct HDIServiceManager *self, const char *interfaceName);
};

/**
 * @brief Obtain the <b>HDIServiceManager</b> object.
 *
 * @return Returns the <b>HDIServiceManager</b> object obtained.
 */
struct HDIServiceManager *HDIServiceManagerGet(void);

/**
 * @brief Releases an <b>HDIServiceManager</b> object.
 * @param servmgr Indicates the <b>HDIServiceManager</b> object to release.
 */
void HDIServiceManagerRelease(struct HDIServiceManager *servmgr);

/**
 * @brief Releases an <b>HdiServiceSet</b> object.
 * @return Returns <b>0</b> if the operation is successful; returns a negative number otherwise.
 */
int32_t HdiServiceSetRelease(struct HdiServiceSet *serviceSet);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* HDI_SERVICE_MANAGER_INF_H */
