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
 * @file devmgr_hdi.h
 *
 * @brief Defines the data structs and interface types related to device management based on the C language.
 *
 * @since 1.0
 */

#ifndef HDI_DEVICE_MANAGER_INF_H
#define HDI_DEVICE_MANAGER_INF_H

#include "hdf_base.h"
#include "hdf_dlist.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

struct HdfRemoteService;

/**
 * @brief Enumerates the types of device services.
 */
enum HDF_DEVICE_TYPE {
    /** Local service */
    HDF_LOCAL_SERVICE,
    /** Remote service */
    HDF_REMOTE_SERVICE,
};

/**
 * @brief Defines the device information struct.
 */
struct DeviceInfoNode {
    /** Device service name */
    char *svcName;
    /** Device type */
    enum HDF_DEVICE_TYPE deviceType;
    /** Device information node, which is used to add device information to the device linked list */
    struct DListHead node;
};

/**
 * @brief Defines the device linked list struct.
 */
struct DeviceInfoList {
    /** Number of devices in the linked list */
    uint32_t deviceCnt;
    /** Device information linked list */
    struct DListHead list;
};

/**
 * @brief Defines the HDI APIs for device management.
 * Developers using the C language can use these APIs to obtain device information and load or unload a device.
 */
struct HDIDeviceManager {
    /** Remote service object */
    struct HdfRemoteService *remote;

    /**
     * @brief Releases the device linked list obtained.
     * You can use this API to release the device linked list obtained by using <b>QueryUsableDeviceInfo</b>
     * or <b>QueryUnusableDeviceInfo()</b>.
     *
     * @param self Indicates the pointer to the device manager object.
     * @param list Indicates the pointer to the the device linked list to release.
     */
    void (*FreeQueryDeviceList)(struct HDIDeviceManager *self, struct DeviceInfoList *list);

    /**
     * @brief Queries information about available devices.
     * Use <b>FreeQueryDeviceList()</b> to release the device linked list after the device information is used.
     *
     * @param self Indicates the pointer to the device manager object.
     * @param list Indicates the pointer to the device linked list, which contains the device information.
     * @return Returns <b>HDF_SUCCESS</b> if the operation is successful; otherwise, the operation fails.
     */
    int32_t (*QueryUsableDeviceInfo)(struct HDIDeviceManager *self, struct DeviceInfoList *list);

    /**
     * @brief Queries information about unavailable devices.
     * Use <b>FreeQueryDeviceList()</b> to release the device linked list after the device information is used.
     *
     * @param self Indicates the pointer to the device manager object.
     * @param list Indicates the pointer to the device linked list, which contains the device information.
     * @return Returns <b>HDF_SUCCESS</b> if the operation is successful; otherwise, the operation fails.
     */
    int32_t (*QueryUnusableDeviceInfo)(struct HDIDeviceManager *self, struct DeviceInfoList *list);

    /**
     * @brief Loads a device driver.
     *
     * @param self Indicates the pointer to the device manager object.
     * @param serviceName Indicates the pointer to the service name of the device to load.
     * @return Returns <b>HDF_SUCCESS</b> if the operation is successful; otherwise, the operation fails.
     */
    int32_t (*LoadDevice)(struct HDIDeviceManager *self, const char *serviceName);

    /**
     * @brief Unloads a device driver.
     *
     * @param self Indicates the pointer to the device manager object.
     * @param serviceName Indicates the pointer to the service name of the device to unload.
     * @return Returns <b>HDF_SUCCESS</b> if the operation is successful; otherwise, the operation fails.
     */
    int32_t (*UnloadDevice)(struct HDIDeviceManager *self, const char *serviceName);
};

/**
 * @brief Obtains an HDI device manager object.
 *
 * @return Returns the device manager object obtained.
 */
struct HDIDeviceManager *HDIDeviceManagerGet(void);

/**
 * @brief Release an HDI device management object.
 *
 * @param devmgr Indicates the pointer to the device manager object to release.
 */
void HDIDeviceManagerRelease(struct HDIDeviceManager *devmgr);
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* HDI_DEVICE_MANAGER_INF_H */