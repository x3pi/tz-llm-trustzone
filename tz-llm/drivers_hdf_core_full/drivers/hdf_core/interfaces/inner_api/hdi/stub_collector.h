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
 * @file stub_collector.h
 *
 * @brief Defines a stub collector for the hdi-gen tool to manage constructors and objects.
 * The service module registers the constructor with the object collector based on the interface name,
 * and then creates a stub object based on the interface name and registered constructor.
 *
 * @since 1.0
 */
#ifndef HDI_STUB_COLLECTOR_H
#define HDI_STUB_COLLECTOR_H

#include <hdf_remote_service.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * @brief Defines the <b>StubConstructor</b> struct.
 */
struct StubConstructor {
    /** Define the interface type of the stub object constructor. */
    struct HdfRemoteService **(*constructor)(void *);
    /** Define the interface type of the stub object destructor. */
    void (*destructor)(struct HdfRemoteService **);
};

/**
 * @brief Registers a stub constructor.
 *
 * @param ifDesc Indicates the pointer to the interface descriptor of the stub object.
 * @param constructor Indicate the pointer to the constructor of the stub object.
 */
void StubConstructorRegister(const char *ifDesc, struct StubConstructor *constructor);

/**
 * @brief Unregisters a stub constructor.
 *
 * @param ifDesc Indicates the pointer to the interface descriptor of the stub object.
 * @param constructor Indicate the pointer to the constructor of the stub object.
 */
void StubConstructorUnregister(const char *ifDesc, const struct StubConstructor *constructor);

/**
 * @brief Creates a stub object and adds it to the object manager.
 *
 * @param ifDesc Indicates the pointer to the interface descriptor of the stub object.
 * @param servPtr Indicates the pointer to the constructor object.
 * @return Returns the stub object created.
 */
struct HdfRemoteService **StubCollectorGetOrNewObject(const char *ifDesc, void *servPtr);

/**
 * @brief Removes a stub object from the object manager.
 *
 * @param ifDesc Indicates the pointer to the interface descriptor of the stub object.
 * @param servPtr Indicates the pointer to the constructor object.
 */
void StubCollectorRemoveObject(const char *ifDesc, void *servPtr);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // HDI_STUB_COLLECTOR_H
