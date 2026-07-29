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
 * @file idevmgr_hdi.h
 *
 * @brief Defines the data structs and interface types related to device management based on the C++ language.
 *
 * @since 1.0
 */
#ifndef HDI_DEVICE_MANAGER_HDI_INF_H
#define HDI_DEVICE_MANAGER_HDI_INF_H

#include <vector>
#include <hdi_base.h>

namespace OHOS {
namespace HDI {
namespace DeviceManager {
namespace V1_0 {
/**
 * @brief Defines the device information struct.
 */
struct DevInfo {
    /** Device ID */
    uint32_t devId;
    /** Service name */
    std::string servName;
    /** Device name */
    std::string deviceName;
};

/**
 * @brief Defines the device linked list struct.
 */
struct HdiDevHostInfo {
    /** Host name */
    std::string hostName;
    /** Host ID */
    uint32_t hostId;
    /** List of devices managed by the host */
    std::vector<DevInfo> devInfo;
};

/**
 * @brief Defines the HDI APIs for device management. Developers using C++ can use these APIs to
 * obtain device information and load or unload a device.
 */
class IDeviceManager : public HdiBase {
public:
    /** HDI interface descriptor, which is used to verify the permission for accessing the HDI interface. */
    DECLARE_HDI_DESCRIPTOR(u"HDI.IDeviceManager.V1_0");
    IDeviceManager() = default;
    virtual ~IDeviceManager() = default;

    /**
     * @brief Obtains a device manager object.
     *
     * @return Returns the device manager object obtained.
     */
    static ::OHOS::sptr<IDeviceManager> Get();

    /**
     * @brief Loads a device driver.
     *
     * @param serviceName Indicates the service name of the device to load.
     * @return Returns <b>HDF_SUCCESS</b> if the operation is successful; otherwise, the operation fails.
     */
    virtual int32_t LoadDevice(const std::string &serviceName) = 0;

    /**
     * @brief Unloads a device driver.
     *
     * @param serviceName Indicates the service name of the device to unload.
     * @return Returns <b>HDF_SUCCESS</b> if the operation is successful; otherwise, the operation fails.
     */
    virtual int32_t UnloadDevice(const std::string &serviceName) = 0;

    /**
     * @brief Obtains information about all loaded devices.
     *
     * @param deviceInfos Indicates information about all loaded devices.
     * @return Returns <b>HDF_SUCCESS</b> if the operation is successful; otherwise, the operation fails.
     */
    virtual int32_t ListAllDevice(std::vector<HdiDevHostInfo> &deviceInfos) = 0;
};
} // namespace V1_0
} // namespace DeviceManager
} // namespace HDI
} // namespace OHOS

#endif /* HDI_DEVICE_MANAGER_HDI_INF_H */
