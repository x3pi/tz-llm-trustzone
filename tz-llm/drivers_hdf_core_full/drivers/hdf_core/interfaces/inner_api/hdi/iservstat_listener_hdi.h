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
 * @file iservstat_listener_hdi.h
 *
 * @brief Defines the data structs and interface types related to service status listening based on C++.
 *
 * @since 1.0
 */
#ifndef HDI_SERVICE_STATUS_LISTENER_INF_H
#define HDI_SERVICE_STATUS_LISTENER_INF_H

#include <refbase.h>
#include <iremote_broker.h>
#include <iremote_stub.h>

#include "hdf_device_class.h"

namespace OHOS {
namespace HDI {
namespace ServiceManager {
namespace V1_0 {
/**
 * @brief Enumerates the service statuses.
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
    std::string serviceName;
    /** Device type */
    uint16_t deviceClass;
    /** Service status */
    uint16_t status;
    /** Service information */
    std::string info;
};

/**
 * @brief Defines the <b>ServStatListener</b> class.
 */
class IServStatListener : public ::OHOS::IRemoteBroker {
public:
    /** HDI interface descriptor, which is used to verify the permission for accessing the HDI interface. */
    DECLARE_INTERFACE_DESCRIPTOR(u"HDI.IServiceStatusListener.V1_0");

    /**
     * @brief Callback of the listener. You need to implement this callback for the service module.
     *
     * @param status Indicates the service status obtained.
     */
    virtual void OnReceive(const ServiceStatus &status) = 0;
};

/**
 * @brief Defines the listener stub class to implement serialization and deserialization of interface parameters.
 * The listener implementation class must inherit this class and implement <b>OnReceive()</b>.
 */
class ServStatListenerStub : public ::OHOS::IRemoteStub<IServStatListener> {
public:
    ServStatListenerStub() = default;
    virtual ~ServStatListenerStub() {}

    /**
     * @brief Distributes messages based on the invoking ID.
     * This API calls the private function <b>ServStatListenerStubOnReceive()</b>.
     *
     * @param code Indicates the distribution command word called by the IPC.
     * @param data Indicates the input parameter, through which the HDF returns
     * the service status information to the service module.
     * @param reply Indicates the data returned by the service module to the HDF.
     * @param option Indicates the call type.
     * @return Returns <b>HDF_SUCCESS</b> if the operation is successful. Otherwise, the operation fails.
     */
    int OnRemoteRequest(uint32_t code,
        ::OHOS::MessageParcel &data, ::OHOS::MessageParcel &reply, ::OHOS::MessageOption &option) override;
private:
    /**
     * @brief Marshals or unmarshals <b>OnReceive()</b> parameters.
     *
     * @param code Indicates the distribution command word called by the IPC.
     * @param data Indicates the input parameter, through which the HDF returns
     * the service status information to the service module.
     * @param reply Indicates the data returned by the service module to the HDF.
     * @param option Indicates the call type.
     * @return Returns <b>HDF_SUCCESS</b> if the operation is successful. Otherwise, the operation fails.
     */
    int32_t ServStatListenerStubOnReceive(::OHOS::MessageParcel& data,
        ::OHOS::MessageParcel& reply, ::OHOS::MessageOption& option);
};
} // namespace V1_0
} // namespace ServiceManager
} // namespace HDI
} // namespace OHOS

#endif /* HDI_SERVICE_STATUS_LISTENER_INF_H */
