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

#include <mutex>

#include <hdf_base.h>
#include <hdf_log.h>
#include <iproxy_broker.h>
#include <iremote_stub.h>
#include <iservice_registry.h>
#include <object_collector.h>
#include <string_ex.h>

#include "hdf_device_manager_interface_code.h"
#include "iservmgr_hdi.h"

#define HDF_LOG_TAG hdi_servmgr_client

using OHOS::HDI::HdfDeviceManager::HdfDeviceManagerInterfaceCode;

namespace OHOS {
namespace HDI {
namespace ServiceManager {
namespace V1_0 {
constexpr int DEVICE_SERVICE_MANAGER_SA_ID = 5100;
std::mutex g_remoteMutex;

class ServiceManagerProxy : public IProxyBroker<IServiceManager> {
public:
    explicit ServiceManagerProxy(const sptr<IRemoteObject> &impl) : IProxyBroker<IServiceManager>(impl) {}
    ~ServiceManagerProxy() {}

    sptr<IRemoteObject> GetService(const char *serviceName) override;
    int32_t ListAllService(std::vector<HdiServiceInfo> &serviceInfos) override;
    int32_t RegisterServiceStatusListener(sptr<IServStatListener> listener, uint16_t deviceClass) override;
    int32_t UnregisterServiceStatusListener(sptr<IServStatListener> listener) override;
    int32_t ListServiceByInterfaceDesc(std::vector<std::string> &serviceNames, const char *interfaceDesc) override;

private:
    static inline BrokerDelegator<ServiceManagerProxy> delegator_;
};

sptr<IServiceManager> IServiceManager::Get()
{
    auto saManager = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    if (saManager == nullptr) {
        HDF_LOGE("failed to get sa manager");
        return nullptr;
    }

    std::unique_lock<std::mutex> lock(g_remoteMutex);
    sptr<IRemoteObject> remote = saManager->GetSystemAbility(DEVICE_SERVICE_MANAGER_SA_ID);
    if (remote != nullptr) {
        return new ServiceManagerProxy(remote);
    }

    HDF_LOGE("failed to get sa hdf service manager");
    return nullptr;
}

int32_t ServiceManagerProxy::RegisterServiceStatusListener(
    ::OHOS::sptr<IServStatListener> listener, uint16_t deviceClass)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (!data.WriteInterfaceToken(GetDescriptor()) || !data.WriteUint16(deviceClass) ||
        !data.WriteRemoteObject(listener->AsObject())) {
        return HDF_FAILURE;
    }

    std::unique_lock<std::mutex> lock(g_remoteMutex);
    int status = Remote()->SendRequest(
        static_cast<uint32_t>(HdfDeviceManagerInterfaceCode::DEVSVC_MANAGER_REGISTER_SVCLISTENER), data, reply, option);
    lock.unlock();
    if (status) {
        HDF_LOGE("failed to register servstat listener, %{public}d", status);
    }
    return status;
}

int32_t ServiceManagerProxy::UnregisterServiceStatusListener(::OHOS::sptr<IServStatListener> listener)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (!data.WriteInterfaceToken(GetDescriptor()) || !data.WriteRemoteObject(listener->AsObject())) {
        return HDF_FAILURE;
    }

    std::unique_lock<std::mutex> lock(g_remoteMutex);
    int status = Remote()->SendRequest(
        static_cast<uint32_t>(HdfDeviceManagerInterfaceCode::DEVSVC_MANAGER_UNREGISTER_SVCLISTENER), data, reply,
        option);
    lock.unlock();
    if (status) {
        HDF_LOGE("failed to unregister servstat listener, %{public}d", status);
    }
    return status;
}

sptr<IRemoteObject> ServiceManagerProxy::GetService(const char *serviceName)
{
    MessageParcel data;
    MessageParcel reply;
    if (!data.WriteInterfaceToken(GetDescriptor()) || !data.WriteCString(serviceName)) {
        return nullptr;
    }

    MessageOption option;
    std::unique_lock<std::mutex> lock(g_remoteMutex);
    int status = Remote()->SendRequest(
        static_cast<uint32_t>(HdfDeviceManagerInterfaceCode::DEVSVC_MANAGER_GET_SERVICE), data, reply, option);
    lock.unlock();
    if (status) {
        HDF_LOGE("get hdi service %{public}s failed, %{public}d", serviceName, status);
        return nullptr;
    }
    HDF_LOGD("get hdi service %{public}s success ", serviceName);
    return reply.ReadRemoteObject();
}

static void HdfDevMgrDbgFillServiceInfo(std::vector<HdiServiceInfo> &serviceInfos, MessageParcel &reply)
{
    while (true) {
        HdiServiceInfo info;
        const char *servName = reply.ReadCString();
        if (servName == nullptr) {
            break;
        }
        info.serviceName = servName;
        info.devClass = reply.ReadUint16();
        info.devId = reply.ReadUint32();
        serviceInfos.push_back(info);
    }
    return;
}

int32_t ServiceManagerProxy::ListAllService(std::vector<HdiServiceInfo> &serviceInfos)
{
    MessageParcel data;
    MessageParcel reply;
    if (!data.WriteInterfaceToken(GetDescriptor())) {
        return HDF_FAILURE;
    }

    MessageOption option;
    std::unique_lock<std::mutex> lock(g_remoteMutex);
    int status = Remote()->SendRequest(
        static_cast<uint32_t>(HdfDeviceManagerInterfaceCode::DEVSVC_MANAGER_LIST_ALL_SERVICE), data, reply, option);
    lock.unlock();
    if (status != HDF_SUCCESS) {
        HDF_LOGE("list all service info failed, %{public}d", status);
        return status;
    } else {
        HdfDevMgrDbgFillServiceInfo(serviceInfos, reply);
    }
    HDF_LOGD("get all service info success");
    return status;
}

int32_t ServiceManagerProxy::ListServiceByInterfaceDesc(
    std::vector<std::string> &serviceNames, const char *interfaceDesc)
{
    MessageParcel data;
    MessageParcel reply;
    if (interfaceDesc == nullptr || strlen(interfaceDesc) == 0) {
        HDF_LOGE("%{public}s: invalid parameter, interfaceDesc is null or empty", __func__);
        return HDF_ERR_INVALID_PARAM;
    }
    if (!data.WriteInterfaceToken(GetDescriptor()) || !data.WriteCString(interfaceDesc)) {
        return HDF_FAILURE;
    }

    MessageOption option;
    std::unique_lock<std::mutex> lock(g_remoteMutex);
    int status = Remote()->SendRequest(
        static_cast<uint32_t>(HdfDeviceManagerInterfaceCode::DEVSVC_MANAGER_LIST_SERVICE_BY_INTERFACEDESC), data, reply,
        option);
    lock.unlock();
    if (status != HDF_SUCCESS) {
        HDF_LOGE("get hdi service collection by %{public}s failed, %{public}d", interfaceDesc, status);
        return status;
    }

    uint32_t serviceNum = 0;
    if (!reply.ReadUint32(serviceNum)) {
        HDF_LOGE("failed to read number of service");
        return HDF_ERR_INVALID_PARAM;
    }

    if (serviceNum > serviceNames.max_size()) {
        HDF_LOGE("invalid len of serviceNames");
        return HDF_ERR_INVALID_PARAM;
    }

    for (uint32_t i = 0; i < serviceNum; i++) {
        if (reply.GetReadableBytes() == 0) {
            HDF_LOGE("no enough data to read");
            return HDF_ERR_INVALID_PARAM;
        }

        const char *serviceName = reply.ReadCString();
        if (serviceName == NULL) {
            break;
        }
        serviceNames.push_back(serviceName);
    }

    HDF_LOGD("get hdi service collection by %{public}s successfully", interfaceDesc);
    return status;
}
} // namespace V1_0
} // namespace ServiceManager
} // namespace HDI
} // namespace OHOS