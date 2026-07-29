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
 * @file iservmgr_hdi.h
 *
 * @brief Defines the data structs and interface types related to service management based on the C++ language.
 *
 * @since 1.0
 */

#ifndef HDI_ISERVICE_MANAGER_INF_H
#define HDI_ISERVICE_MANAGER_INF_H

#include <vector>
#include <hdi_base.h>
#include <iremote_broker.h>
#include <refbase.h>

#include "iservstat_listener_hdi.h"

namespace OHOS {
namespace HDI {
namespace ServiceManager {
namespace V1_0 {
/**
 * @brief Defines the HDI service information struct.
 */

struct HdiServiceInfo {
    /** HDI service name */
    std::string serviceName;
    /** Device type */
    uint16_t devClass;
    /** Device ID */
    uint32_t devId;
};

/**
 * @brief Defines the HDI APIs for service management. Developers using C++ can use the APIs to obtain
 * a service or service set and register a service status listener.
 */
class IServiceManager : public HdiBase {
public:
    /** HDI interface descriptor, which is used to verify the permission for accessing the HDI interface. */
    DECLARE_HDI_DESCRIPTOR(u"HDI.IServiceManager.V1_0");

    /**
     * @brief Obtains a service manager object.
     *
     * @return Returns the service manager object obtained.
     */
    static ::OHOS::sptr<IServiceManager> Get();

    /**
     * @brief Obtain an HDI service.
     *
     * @param serviceName Indicates the pointer to the HDI service name to obtain.
     * @return Returns the HDI service obtained.
     */
    virtual ::OHOS::sptr<IRemoteObject> GetService(const char *serviceName) = 0;

    /**
     * @brief Obtains information about all loaded HDI services.
     *
     * @param serviceInfos Indicates information about all loaded services.
     * @return Returns <b>HDF_SUCCESS</b> if the operation is successful; otherwise, the operation fails.
     */
    virtual int32_t ListAllService(std::vector<HdiServiceInfo> &serviceInfos) = 0;

    /**
     * @brief Registers a listener for observing the service status.
     *
     * @param listener Indicates the listener object to register.
     * @param deviceClass Indicates the service type of the device to be observed.
     * @return Returns <b>HDF_SUCCESS</b> if the operation is successful; otherwise, the operation fails.
     */

    virtual int32_t RegisterServiceStatusListener(::OHOS::sptr<IServStatListener> listener, uint16_t deviceClass) = 0;
    /**
     * @brief Unregisters a service status listener.
     *
     * @param listener Indicates the listener object to unregister.
     * @return Returns <b>HDF_SUCCESS</b> if the operation is successful; otherwise, the operation fails.
     */
    virtual int32_t UnregisterServiceStatusListener(::OHOS::sptr<IServStatListener> listener) = 0;

    /**
     * @brief Queries HDI services based on the specified interface descriptor.
     *
     * @param serviceNames Indicates the service names obtained.
     * @param interfaceDesc Indicates the interface descriptor.
     * @return Returns <b>HDF_SUCCESS</b> if the operation is successful; otherwise, the operation fails.
     */
    virtual int32_t ListServiceByInterfaceDesc(std::vector<std::string> &serviceNames, const char *interfaceDesc) = 0;
};
} // namespace V1_0
} // namespace ServiceManager
} // namespace HDI
} // namespace OHOS

#endif /* HDI_ISERVICE_MANAGER_INF_H */
