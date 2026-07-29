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
 * @file servstat_listener_hdi.h
 *
 * @brief Defines the data structs and interface types related to service status listening based on C.
 *
 * @since 1.0
 */

#ifndef HDI_SERVICE_STATUS_LISTENER_INF_H
#define HDI_SERVICE_STATUS_LISTENER_INF_H

#include "hdf_types.h"
#include "hdf_service_status.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */
/**
 * @brief Creates an HDI service status listener object. You can set the function for receiving the service status
 * and use <b>RegisterServiceStatusListener()</b> to register the listener object.
 *
 * @return Returns the pointer to the listener object created.
 */

struct ServiceStatusListener *HdiServiceStatusListenerNewInstance(void);
/**
 * @brief Releases an HDI service status listener object.
 *
 * @param listener Indicates the pointer to the listener object to release.
 */
void HdiServiceStatusListenerFree(struct ServiceStatusListener *listener);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* HDI_SERVICE_STATUS_LISTENER_INF_H */
