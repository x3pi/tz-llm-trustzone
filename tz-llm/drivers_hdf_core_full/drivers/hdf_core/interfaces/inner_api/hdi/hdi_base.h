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
 * @file hdi_base.h
 *
 * @brief Defines the HDI base class for managing HDI objects.
 *
 * @since 1.0
 */

#ifndef HDI_BASE_INTERFACE_H
#define HDI_BASE_INTERFACE_H

#include <string>
#ifdef __LITEOS__
#include <memory>
#else
#include <refbase.h>
#endif

namespace OHOS {
namespace HDI {
/**
 * @brief Defines the HDI base class for managing HDI objects.
 */
#ifdef __LITEOS__
class HdiBase : public std::enable_shared_from_this<HdiBase> {
#else
class HdiBase : virtual public OHOS::RefBase {
#endif
public:
    HdiBase() = default;
    virtual ~HdiBase() = default;
};

/**
 * @brief Defines an HDI interface descriptor macro.
 *
 * @param DESCRIPTOR Indicates the interface descriptor.
 */
#define DECLARE_HDI_DESCRIPTOR(DESCRIPTOR)                             \
    const static inline std::u16string metaDescriptor_ = {DESCRIPTOR}; \
    const static inline std::u16string &GetDescriptor()                \
    {                                                                  \
        return metaDescriptor_;                                        \
    }
} // namespace HDI
} // namespace OHOS

#endif // HDI_BASE_INTERFACE_H
