/*
 * Copyright (c) 2022-2023 Huawei Device Co., Ltd.
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
 * @file hdi_support.h
 *
 * @brief Defines APIs for loading and unloading an HDI driver.
 *
 * @since 1.0
 */

#ifndef HDF_HDI_SUPPORT_H
#define HDF_HDI_SUPPORT_H

#include "hdf_base.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */
/**
 * @brief Loads the HDI driver of a service module.
 * This API obtains the driver name based on the interface descriptor and service name,
 * and then loads the device driver to the system.
 *
 * @param desc Indicates the pointer to the HDI descriptor.
 * @param serviceName Indicates the service name of the driver to load.
 * @return Returns the HDI driver object.
 */
void *LoadHdiImpl(const char *desc, const char *serviceName);

/**
 * @brief Unloads the HDI driver of a service module.
 *
 * This API obtains the driver name based on the interface descriptor and service name,
 * and then calls <b>unload()</b> of the driver to unload the driver.
 * @param desc Indicates the pointer to the HDI descriptor.
 * @param serviceName Indicates the service name of the driver to unload.
 * @param impl Indicates the pointer to the HDI driver object to unload.
 */
void UnloadHdiImpl(const char *desc, const char *serviceName, void *impl);
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* HDF_HDI_SUPPORT_H */
