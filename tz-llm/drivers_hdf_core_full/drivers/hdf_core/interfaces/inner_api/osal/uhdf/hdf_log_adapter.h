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
 * @addtogroup DriverUtils
 * @{
 *
 * @brief Defines common macros and interfaces of the driver module.
 *
 * This module provides interfaces for log printing, doubly linked list operations, and work queues.
 *
 * @since 1.0
 */

/**
 * @file hdf_log_adapter.h
 *
 * @brief Provides the implementation of hdf_log. HiLog macros are used to implement the functions of macros,
 * such as <b>HDF_LOGV</b>. Service modules do not use this file.
 *
 * @since 1.0
 */

#ifndef HDF_LOG_ADAPTER_H
#define HDF_LOG_ADAPTER_H

#include "hilog/log.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * @brief Defines the log domain of the HDF.
 */
#undef LOG_DOMAIN
#define LOG_DOMAIN 0xD002510

/**
 * @brief Defines the default log label of the HDF.
 */
#ifndef LOG_TAG
#define LOG_TAG HDF
#endif

/**
 * @brief Adapts to printing information of the <b>verbose</b> level of the HiLog module.
 */
#define HDF_LOGV_WRAPPER(fmt, arg...) HILOG_DEBUG(LOG_CORE, fmt, ##arg)

/**
 * @brief Adapts to printing information of the <b>debug</b> level of the HiLog module.
 */
#define HDF_LOGD_WRAPPER(fmt, arg...) HILOG_DEBUG(LOG_CORE, fmt, ##arg)

/**
 * @brief Adapts to printing information of the <b>info</b> level of the HiLog module.
 */
#define HDF_LOGI_WRAPPER(fmt, arg...) HILOG_INFO(LOG_CORE, fmt, ##arg)

/**
 @brief Adapts to printing information of the <b>warning</b> level of the HiLog module.
 */
#define HDF_LOGW_WRAPPER(fmt, arg...) HILOG_WARN(LOG_CORE, fmt, ##arg)

/**
 * @brief Adapts to printing information of the <b>error</b> level of the HiLog module.
 */
#define HDF_LOGE_WRAPPER(fmt, arg...) HILOG_ERROR(LOG_CORE, fmt, ##arg)

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* HDF_LOG_ADAPTER_H */
