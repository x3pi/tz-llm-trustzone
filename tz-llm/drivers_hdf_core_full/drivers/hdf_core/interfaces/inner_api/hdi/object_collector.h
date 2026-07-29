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
 * The HDF and the IDL code generated allow system abilities to access HDI driver services.
 *
 * @since 1.0
 */

/**
 * @file object_collector.h
 *
 * @brief Defines an object collector for the hdi-gen tool to manage constructors and objects.
 * When generating a class, the hdi-gen tool defines a static inline object of the <b>ObjectDelegator</b> type
 * and registers the interface name and constructor with the object collector.
 * When the service module driver is initialized, the registered constructor (obtained based on the interface name) is
 * used to create and register the IPC stub object.
 *
 * @since 1.0
 */

#ifndef HDI_OBJECT_COLLECTOR_H
#define HDI_OBJECT_COLLECTOR_H

#include <string>
#include <hdi_base.h>
#include <iremote_object.h>
#include <map>
#include <mutex>
#include <refbase.h>

namespace OHOS {
namespace HDI {
/**
 * @brief Defines the <b>ObjectCollector</b> class for the hdi-gen tool to manage constructors and objects.
 */
class ObjectCollector {
public:
    /** Define the constructor interface type. */
    using Constructor = std::function<sptr<IRemoteObject>(const sptr<HdiBase> &interface)>;
    /** Obtain the object instance managed by the ObjectCollector. */
    static ObjectCollector &GetInstance();
    /** Register a constructor based on the interface name. */
    bool ConstructorRegister(const std::u16string &interfaceName, const Constructor &constructor);
    /** Unregister a constructor based on the interface name. */
    void ConstructorUnRegister(const std::u16string &interfaceName);
    /** Create an object based on the interface name and implementation. */
    sptr<IRemoteObject> NewObject(const sptr<HdiBase> &interface, const std::u16string &interfaceName);
    /** Create or obtain an object based on the interface name and implementation. */
    sptr<IRemoteObject> GetOrNewObject(const sptr<HdiBase> &interface, const std::u16string &interfaceName);
    /** Remove an object based on the interface implementation. */
    bool RemoveObject(const sptr<HdiBase> &interface);

private:
    ObjectCollector() = default;
    /** Create an object based on the interface name and implementation. */
    sptr<IRemoteObject> NewObjectLocked(const sptr<HdiBase> &interface, const std::u16string &interfaceName);
    /** Instance of the object managed by the ObjectCollector. */
    static ObjectCollector *instance_;
    /** Define the mapping between the interface name and the constructor. */
    std::map<const std::u16string, const Constructor> constructorMapper_;
    /** Define the mapping between the interface implementation and the object. */
    std::map<HdiBase *, IRemoteObject *> interfaceObjectCollector_;
    /** Define a mutex to support concurrent access to the ObjectCollector. */
    std::mutex mutex_;
};

/**
 * @brief Defines a <b>ObjectDelegator</b> template, which is used to register and
 * unregister a template object constructor.
 */
template <typename OBJECT, typename INTERFACE>
class ObjectDelegator {
public:
    /** Constructor, which registers a template object constructor through the ObjectCollector. */
    ObjectDelegator();
    /** Destructor, which unregisters a template object constructor through the ObjectCollector. */
    ~ObjectDelegator();

private:
    ObjectDelegator(const ObjectDelegator &) = delete;
    ObjectDelegator(ObjectDelegator &&) = delete;
    ObjectDelegator &operator=(const ObjectDelegator &) = delete;
    ObjectDelegator &operator=(ObjectDelegator &&) = delete;
    std::u16string descriptor_;
};

/**
 * @brief A constructor used to register an object constructor of the <b>ObjectDelegator</b> template
 * through the <b>ObjectCollector</b>.
 *
 * @param OBJECT Indicates the type of the object to create.
 * @param INTERFACE Indicates the HDI interface of the service module.
 */
template <typename OBJECT, typename INTERFACE>
ObjectDelegator<OBJECT, INTERFACE>::ObjectDelegator()
{
    auto creator = [](const sptr<HdiBase> &interface) -> sptr<IRemoteObject> {
        return new OBJECT(static_cast<INTERFACE *>(interface.GetRefPtr()));
    };
    if (ObjectCollector::GetInstance().ConstructorRegister(INTERFACE::GetDescriptor(), creator)) {
        descriptor_ = INTERFACE::GetDescriptor();
    }
}

/**
 * @brief A destructor used to unregister an object constructor of the <b>ObjectDelegator</b> template
 * through the <b>ObjectCollector</b>.
 *
 * @param OBJECT Indicates the type of the object.
 * @param INTERFACE Indicates the HDI interface of the service module.
 */
template <typename OBJECT, typename INTERFACE>
ObjectDelegator<OBJECT, INTERFACE>::~ObjectDelegator()
{
    ObjectCollector::GetInstance().ConstructorUnRegister(descriptor_);
}
} // namespace HDI
} // namespace OHOS

#endif // HDI_OBJECT_COLLECTOR_H
