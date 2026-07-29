/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#ifndef HDF_DEVICE_MANAGER_INTERFACE_CODE
#define HDF_DEVICE_MANAGER_INTERFACE_CODE

/* SAID: 5100 */
namespace OHOS {
namespace HDI {
namespace HdfDeviceManager {
enum class HdfDeviceManagerInterfaceCode {
    DEVSVC_MANAGER_ADD_SERVICE = 1,
    DEVSVC_MANAGER_UPDATE_SERVICE = 2,
    DEVSVC_MANAGER_GET_SERVICE = 3,
    DEVSVC_MANAGER_REGISTER_SVCLISTENER = 4,
    DEVSVC_MANAGER_UNREGISTER_SVCLISTENER = 5,
    DEVSVC_MANAGER_LIST_ALL_SERVICE = 6,
    DEVSVC_MANAGER_LIST_SERVICE = 7,
    DEVSVC_MANAGER_REMOVE_SERVICE = 8,
    DEVSVC_MANAGER_LIST_SERVICE_BY_INTERFACEDESC = 9,
};
}  // namespace HdfDeviceManager
}  // namespace HDI
}  // namespace OHOS
#endif // HDF_DEVICE_MANAGER_INTERFACE_CODE

