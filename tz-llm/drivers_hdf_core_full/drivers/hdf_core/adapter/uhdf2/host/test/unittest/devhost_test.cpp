/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#include <gtest/gtest.h>

#include "devhost_service.h"
#include "devhost_service_stub.h"
#include "devhost_service_proxy.h"
#include "devhost_dump.h"
#include "devhost_dump_reg.h"
#include "device_service_stub.h"
#include "devmgr_service_clnt.h"
#include "devmgr_service_proxy.h"
#include "devsvc_manager_proxy.h"
#include "driver_loader_full.h"
#include "hdf_cstring.h"
#include "hdf_device.h"
#include "hdf_device_node.h"
#include "hdf_remote_service.h"
#include "hdf_log.h"
#include "hdf_sbuf.h"
#include "osal_mem.h"

#define HDF_LOG_TAG   host_test

namespace OHOS {
using namespace testing::ext;

class DevHostTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp();
    void TearDown();
};

void DevHostTest::SetUpTestCase()
{
}

void DevHostTest::TearDownTestCase()
{
}

void DevHostTest::SetUp()
{
}

void DevHostTest::TearDown()
{
}

HWTEST_F(DevHostTest, DevHostDevMgrServiceProxyTest, TestSize.Level1)
{
    struct HdfObject *object = DevmgrServiceProxyCreate();
    ASSERT_TRUE(object != nullptr);
    struct IDevmgrService *instance = reinterpret_cast<struct IDevmgrService *>(object);

    int32_t ret = instance->AttachDeviceHost(instance, 0, nullptr);
    ASSERT_TRUE(ret != HDF_SUCCESS);
    ret = instance->AttachDevice(instance, nullptr);
    ASSERT_TRUE(ret != HDF_SUCCESS);
    ret = instance->LoadDevice(instance, nullptr);
    ASSERT_TRUE(ret != HDF_SUCCESS);

    struct DevmgrServiceProxy *proxy = reinterpret_cast<struct DevmgrServiceProxy *>(object);
    HdfRemoteServiceRecycle(proxy->remote);
    proxy->remote = nullptr;

    ret = instance->AttachDeviceHost(instance, 0, nullptr);
    ASSERT_TRUE(ret != HDF_SUCCESS);
    ret = instance->AttachDevice(instance, nullptr);
    ASSERT_TRUE(ret != HDF_SUCCESS);
    ret = instance->AttachDevice(nullptr, nullptr);
    ASSERT_TRUE(ret != HDF_SUCCESS);
    ret = instance->DetachDevice(instance, 0);
    ASSERT_TRUE(ret != HDF_SUCCESS);
    ret = instance->DetachDevice(nullptr, 0);
    ASSERT_TRUE(ret != HDF_SUCCESS);
    ret = instance->LoadDevice(instance, nullptr);
    ASSERT_TRUE(ret != HDF_SUCCESS);
    ret = instance->LoadDevice(nullptr, nullptr);
    ASSERT_TRUE(ret != HDF_SUCCESS);
    DevmgrServiceProxyRelease(object);
    DevmgrServiceProxyRelease(nullptr);
}

HWTEST_F(DevHostTest, DevHostDevSvcMgrProxyTest, TestSize.Level1)
{
    struct HdfObject *object = DevSvcManagerProxyCreate();
    ASSERT_TRUE(object != nullptr);
    struct IDevSvcManager *instance = reinterpret_cast<struct IDevSvcManager *>(object);
    struct HdfDeviceObject device;
    struct HdfServiceInfo servInfo;
    servInfo.servName = "test";
    servInfo.devClass = DEVICE_CLASS_MAX;
    int32_t ret = instance->AddService(instance, &device, &servInfo);
    ASSERT_TRUE(ret != HDF_SUCCESS);
    ret = instance->AddService(instance, nullptr, &servInfo);
    ASSERT_TRUE(ret != HDF_SUCCESS);
    ret = instance->UpdateService(instance, &device, &servInfo);
    ASSERT_TRUE(ret != HDF_SUCCESS);

    servInfo.servName = nullptr;
    ret = instance->AddService(instance, &device, &servInfo);
    ASSERT_TRUE(ret != HDF_SUCCESS);
    ret = instance->AddService(nullptr, &device, &servInfo);
    ASSERT_TRUE(ret != HDF_SUCCESS);
    ret = instance->UpdateService(instance, &device, &servInfo);
    ASSERT_TRUE(ret != HDF_SUCCESS);
    ret = instance->UpdateService(nullptr, &device, &servInfo);
    ASSERT_TRUE(ret != HDF_SUCCESS);

    struct DevSvcManagerProxy *proxy = reinterpret_cast<struct DevSvcManagerProxy *>(object);
    struct HdfDeathRecipient recipient;
    proxy->recipient.OnRemoteDied(&recipient, nullptr);
    proxy->recipient.OnRemoteDied(nullptr, nullptr);
    HdfRemoteServiceRecycle(proxy->remote);
    proxy->remote = nullptr;
    ret = instance->AddService(instance, &device, &servInfo);
    ASSERT_TRUE(ret != HDF_SUCCESS);
    ret = instance->UpdateService(instance, &device, &servInfo);
    ASSERT_TRUE(ret != HDF_SUCCESS);
    struct HdfObject *service = instance->GetService(instance, "test");
    ASSERT_TRUE(service == nullptr);
    instance->RemoveService(nullptr, "test", nullptr);
    instance->RemoveService(instance, nullptr, nullptr);
    instance->RemoveService(instance, "test", nullptr);
    DevSvcManagerProxyRelease(object);
    DevSvcManagerProxyRelease(nullptr);
}

HWTEST_F(DevHostTest, DevHostServiceTest, TestSize.Level1)
{
    struct HdfObject *object = DevHostServiceCreate();
    ASSERT_TRUE(object != nullptr);
    struct IDevHostService *hostService = reinterpret_cast<struct IDevHostService *>(object);
    struct DevHostService *devHost = reinterpret_cast<struct DevHostService *>(object);
    devHost->hostName = "test_host";
    ASSERT_TRUE(hostService->PmNotify != nullptr);
    hostService->PmNotify(hostService, POWER_STATE_RESUME);
    hostService->PmNotify(hostService, POWER_STATE_SUSPEND);
    hostService->PmNotify(hostService, POWER_STATE_MAX);
    hostService->PmNotify(nullptr, POWER_STATE_RESUME);
    hostService->StartService(nullptr);
    DevHostServiceRelease(object);
    DevHostServiceRelease(nullptr);
    DevHostServiceConstruct(nullptr);
    DevHostServiceDestruct(nullptr);

    hostService = DevHostServiceNewInstance(0, "sample_host");
    ASSERT_TRUE(hostService != nullptr);
    int32_t ret = DevHostServiceAddDevice(hostService, nullptr);
    ASSERT_TRUE(ret != HDF_SUCCESS);
    ret = DevHostServiceAddDevice(nullptr, nullptr);
    ASSERT_TRUE(ret != HDF_SUCCESS);
    ret = DevHostServiceDelDevice(hostService, 0);
    ASSERT_TRUE(ret == HDF_SUCCESS);
    ret = DevHostServiceDelDevice(nullptr, 0);
    ASSERT_TRUE(ret == HDF_SUCCESS);

    hostService = DevHostServiceNewInstance(0, nullptr);
    ASSERT_TRUE(hostService != nullptr);
    DevHostServiceFreeInstance(hostService);
    DevHostServiceFreeInstance(nullptr);

    struct DevmgrServiceClnt clnt;
    clnt.devMgrSvcIf = nullptr;
    DevmgrServiceClntFreeInstance(&clnt);
    DevmgrServiceClntFreeInstance(nullptr);
}

static int HdfTestSuccBind(struct HdfDeviceObject *deviceObject)
{
    HDF_LOGI("HdfTestSuccBind enter!");
    return HDF_SUCCESS;
}

static int HdfTestFailBind(struct HdfDeviceObject *deviceObject)
{
    HDF_LOGI("HdfTestFailBind enter!");
    return HDF_FAILURE;
}

static int HdfTestInit(struct HdfDeviceObject *deviceObject)
{
    HDF_LOGI("HdfTestInit enter!");
    return HDF_FAILURE;
}

HWTEST_F(DevHostTest, DevHostDeviceNodeTest1, TestSize.Level1)
{
    struct HdfDriver driver;
    struct HdfDriverEntry entry;
    entry.Bind = nullptr;
    driver.entry = &entry;
    struct HdfDeviceInfo deviceInfo;
    deviceInfo.permission = 0;
    deviceInfo.deviceId = 0;
    deviceInfo.policy = SERVICE_POLICY_NONE;
    deviceInfo.svcName = "sample_service";
    deviceInfo.deviceName = "sample_module";

    struct HdfDeviceNode *devNode = HdfDeviceNodeNewInstance(&deviceInfo, &driver);
    ASSERT_TRUE(devNode != nullptr);

    devNode->servStatus = false;
    int32_t ret = HdfDeviceNodeRemoveService(devNode);
    ASSERT_TRUE(ret == HDF_SUCCESS);

    ret = DeviceDriverBind(devNode);
    ASSERT_TRUE(ret == HDF_SUCCESS);
    HdfDeviceNodeFreeInstance(devNode);

    entry.Bind = HdfTestSuccBind;
    deviceInfo.policy = SERVICE_POLICY_PUBLIC;
    devNode = HdfDeviceNodeNewInstance(&deviceInfo, &driver);
    ret = DeviceDriverBind(devNode);
    ASSERT_TRUE(ret == HDF_SUCCESS);
    HdfDeviceNodeFreeInstance(devNode);

    entry.Bind = HdfTestFailBind;
    devNode = HdfDeviceNodeNewInstance(&deviceInfo, &driver);
    ret = DeviceDriverBind(devNode);
    ASSERT_TRUE(ret != HDF_SUCCESS);
    HdfDeviceNodeFreeInstance(devNode);

    entry.Bind = nullptr;
    devNode = HdfDeviceNodeNewInstance(&deviceInfo, &driver);
    ret = DeviceDriverBind(devNode);
    ASSERT_TRUE(ret != HDF_SUCCESS);
    HdfDeviceNodeFreeInstance(devNode);

    entry.Bind = nullptr;
    devNode = HdfDeviceNodeNewInstance(&deviceInfo, &driver);
    devNode->policy = SERVICE_POLICY_CAPACITY;
    ret = DeviceDriverBind(devNode);
    ASSERT_TRUE(ret != HDF_SUCCESS);
    HdfDeviceNodeFreeInstance(devNode);

    deviceInfo.svcName = nullptr;
    devNode = HdfDeviceNodeNewInstance(&deviceInfo, &driver);
    ASSERT_TRUE(devNode == nullptr);
    devNode = HdfDeviceNodeNewInstance(nullptr, &driver);
    ASSERT_TRUE(devNode == nullptr);
}

HWTEST_F(DevHostTest, DevHostDeviceNodeTest2, TestSize.Level1)
{
    struct HdfDriverEntry entry;
    struct HdfDriver driver;
    driver.entry = &entry;

    struct HdfDeviceInfo deviceInfo;
    deviceInfo.deviceId = 0;
    deviceInfo.permission = 0;
    deviceInfo.svcName = "driver_service";
    deviceInfo.deviceName = "driver_module";

    deviceInfo.policy = SERVICE_POLICY_PUBLIC;
    entry.Bind = HdfTestSuccBind;
    entry.Init = HdfTestInit;
    struct HdfDeviceNode *devNode = HdfDeviceNodeNewInstance(&deviceInfo, &driver);
    ASSERT_TRUE(devNode != nullptr);
    HdfDeviceNodeConstruct(devNode);
    struct IDeviceNode *nodeIf = &devNode->super;
    int32_t ret = nodeIf->LaunchNode(devNode);

    entry.Bind = HdfTestFailBind;
    entry.Init = HdfTestInit;
    devNode = HdfDeviceNodeNewInstance(&deviceInfo, &driver);
    ASSERT_TRUE(devNode != nullptr);
    HdfDeviceNodeConstruct(devNode);
    nodeIf = &devNode->super;
    ret = nodeIf->LaunchNode(devNode);

    entry.Init = nullptr;
    devNode = HdfDeviceNodeNewInstance(&deviceInfo, &driver);
    ASSERT_TRUE(devNode != nullptr);
    HdfDeviceNodeConstruct(devNode);
    nodeIf = &devNode->super;
    ret = nodeIf->LaunchNode(devNode);
    ASSERT_TRUE(ret != HDF_SUCCESS);

    devNode->devStatus = DEVNODE_NONE;
    nodeIf->UnlaunchNode(devNode);
}

HWTEST_F(DevHostTest, DevHostDeviceNodeTest3, TestSize.Level1)
{
    struct HdfDriverEntry entry;
    entry.Bind = nullptr;
    struct HdfDriver driver;
    driver.entry = &entry;

    struct HdfDeviceInfo deviceInfo;
    deviceInfo.permission = 0;
    deviceInfo.deviceId = 0;
    deviceInfo.svcName = "test_service";
    deviceInfo.deviceName = "test_module";
    struct HdfDeviceNode *devNode = HdfDeviceNodeNewInstance(&deviceInfo, &driver);
    ASSERT_TRUE(devNode != nullptr);
    struct IDeviceNode *nodeIf = &devNode->super;
    int32_t ret = HdfDeviceNodeAddPowerStateListener(devNode, nullptr);
    ASSERT_TRUE(ret == HDF_SUCCESS);
    ret = HdfDeviceNodeAddPowerStateListener(devNode, nullptr);
    ASSERT_TRUE(ret != HDF_SUCCESS);

    HdfDeviceNodeRemovePowerStateListener(nullptr, nullptr);
    HdfDeviceNodeRemovePowerStateListener(devNode, nullptr);
    HdfDeviceNodeRemovePowerStateListener(devNode, nullptr);
    devNode->deviceObject.service = nullptr;
    ret = HdfDeviceNodePublishPublicService(devNode);
    ASSERT_TRUE(ret != HDF_SUCCESS);
    nodeIf->UnlaunchNode(nullptr);
    HdfDeviceNodeFreeInstance(nullptr);
    HdfDeviceNodeConstruct(nullptr);
    HdfDeviceNodeDestruct(nullptr);
    ret = HdfDeviceNodePublishPublicService(nullptr);
    ASSERT_TRUE(ret != HDF_SUCCESS);
    ret = HdfDeviceNodeRemoveService(nullptr);
    ASSERT_TRUE(ret == HDF_SUCCESS);
    ret = nodeIf->LaunchNode(nullptr);
    ASSERT_TRUE(ret != HDF_SUCCESS);
    ret = DeviceDriverBind(nullptr);
    ASSERT_TRUE(ret != HDF_SUCCESS);
}

HWTEST_F(DevHostTest, DevHostDeviceTest, TestSize.Level1)
{
    struct HdfObject *object = HdfDeviceCreate();
    ASSERT_TRUE(object != nullptr);

    struct IHdfDevice *device = reinterpret_cast<struct IHdfDevice *>(object);
    int32_t ret = device->Attach(device, nullptr);
    ASSERT_TRUE(ret != HDF_SUCCESS);
    ret = device->Attach(nullptr, nullptr);
    ASSERT_TRUE(ret != HDF_SUCCESS);

    ret = device->DetachWithDevid(device, 0);
    ASSERT_TRUE(ret != HDF_SUCCESS);

    ret = HdfDeviceDetach(nullptr, nullptr);
    ASSERT_TRUE(ret != HDF_SUCCESS);
    ret = HdfDeviceDetach(device, nullptr);
    ASSERT_TRUE(ret != HDF_SUCCESS);
    HdfDeviceRelease(nullptr);
    HdfDeviceFreeInstance(nullptr);
}

HWTEST_F(DevHostTest, DevHostDriverLoaderFullTest, TestSize.Level1)
{
    struct HdfObject *object = HdfDriverLoaderFullCreate();
    ASSERT_TRUE(object != nullptr);
    struct HdfDriverLoader *loader = reinterpret_cast<struct HdfDriverLoader *>(object);
    char testModule[PATH_MAX] = {0xa};
    testModule[PATH_MAX - 1] = '0';
    struct HdfDriver *driver = loader->super.GetDriver(nullptr);
    ASSERT_TRUE(driver == nullptr);
    driver = loader->super.GetDriver(testModule);
    ASSERT_TRUE(driver == nullptr);
    loader->super.ReclaimDriver(nullptr);
    HdfDriverLoaderFullRelease(object);
    HdfDriverLoaderFullRelease(nullptr);
}

HWTEST_F(DevHostTest, DevHostDeviceServiceStubTest, TestSize.Level1)
{
    struct HdfObject *object = DeviceServiceStubCreate();
    ASSERT_TRUE(object != nullptr);
    struct IDeviceNode *deviceIf = reinterpret_cast<struct IDeviceNode *>(object);
    struct DeviceServiceStub service;
    struct HdfRemoteService remote;
    struct HdfDeviceNode *devNode = reinterpret_cast<struct HdfDeviceNode *>(&service);
    devNode->servName = nullptr;
    int32_t ret = deviceIf->PublishService(devNode);
    ASSERT_TRUE(ret != HDF_SUCCESS);
    service.remote = &remote;
    devNode->servName = HdfStringCopy("test");
    ret = deviceIf->PublishService(devNode);
    ASSERT_TRUE(ret != HDF_SUCCESS);

    service.remote = nullptr;
    devNode->policy = SERVICE_POLICY_NONE;
    ret = deviceIf->PublishService(devNode);
    ASSERT_TRUE(ret != HDF_SUCCESS);

    devNode->policy = SERVICE_POLICY_PUBLIC;
    devNode->deviceObject.deviceClass = DEVICE_CLASS_MAX;
    devNode->interfaceDesc = nullptr;
    ret = deviceIf->PublishService(devNode);
    ASSERT_TRUE(ret != HDF_SUCCESS);

    ret = deviceIf->RemoveService(nullptr);
    ASSERT_TRUE(ret != HDF_SUCCESS);

    OsalMemFree(devNode->servName);

    DeviceServiceStubRelease(object);
    DeviceServiceStubRelease(nullptr);

    DevHostServiceStubRelease(nullptr);
}

static int32_t DevHostTestDumpHostFunc(struct HdfSBuf *data, struct HdfSBuf *reply)
{
    uint32_t argv = 0;

    (void)HdfSbufReadUint32(data, &argv);
    HDF_LOGI("%{public}d", argv);

    const char *value = HdfSbufReadString(data);
    while (value != NULL && argv > 0) {
        HDF_LOGI("%{public}s", value);
        value = HdfSbufReadString(data);
        argv--;
    }

    HdfSbufWriteString(reply, "test_host_dump\n");

    return HDF_SUCCESS;
}

static int32_t DevHostTestDumpServiceFunc(struct HdfSBuf *data, struct HdfSBuf *reply)
{
    uint32_t argv = 0;
    (void)HdfSbufReadUint32(data, &argv);
    HDF_LOGI("%{public}d", argv);
    const char *value = HdfSbufReadString(data);
    while (value != NULL && argv > 0) {
        HDF_LOGI("%{public}s", value);
        value = HdfSbufReadString(data);
        argv--;
    }

    HdfSbufWriteString(reply, "test_service_dump\n");

    return HDF_SUCCESS;
}

HWTEST_F(DevHostTest, DevHostDumpTest001, TestSize.Level1)
{
    DevHostDumpInit();
    HdfSBuf * testSBufData = HdfSbufTypedObtain(SBUF_RAW);
    HdfSBuf * testSBufReply = HdfSbufTypedObtain(SBUF_RAW);

    // test DevHostDump NULL inpput
    HDF_LOGI("test DevHostDump NULL inpput BEGIN");
    DevHostDump(nullptr, nullptr);
    ASSERT_TRUE(HdfSbufReadString(testSBufReply) == NULL);
    DevHostDump(testSBufData, nullptr);
    ASSERT_TRUE(HdfSbufReadString(testSBufReply) == NULL);
    DevHostDump(nullptr, testSBufReply);
    ASSERT_TRUE(HdfSbufReadString(testSBufReply) == NULL);
    HDF_LOGI("test DevHostDump NULL inpput END");

    // test DevHostDump null option
    HDF_LOGI("test DevHostDump null option BEGIN");
    DevHostDump(testSBufData, testSBufReply);
    ASSERT_TRUE(HdfSbufReadString(testSBufReply) == NULL);
    HDF_LOGI("test DevHostDump null option END");

    // test DevHostDump invalid parameter brunch
    HDF_LOGI("test DevHostDump invalid parameter brunch BEGIN");
    HdfSbufWriteString(testSBufData, "dumpNothing");
    DevHostDump(testSBufData, testSBufReply);
    ASSERT_TRUE(HdfSbufReadString(testSBufReply) == NULL);
    HDF_LOGI("test DevHostDump invalid parameter brunch END");

    DevHostDumpDeInit();
}

HWTEST_F(DevHostTest, DevHostDumpTest002, TestSize.Level1)
{
    DevHostDumpInit();
    HdfSBuf * testSBufData = HdfSbufTypedObtain(SBUF_RAW);
    HdfSBuf * testSBufReply = HdfSbufTypedObtain(SBUF_RAW);

    // test DevHostDump option dumpHost without dumpHostFunc
    HDF_LOGI("test DevHostDump option dumpHost without dumpHostFunc BEGIN");
    HdfSbufFlush(testSBufData);
    HdfSbufFlush(testSBufReply);
    HdfSbufWriteString(testSBufData, "dumpHost");
    DevHostDump(testSBufData, testSBufReply);
    ASSERT_TRUE(strcmp(HdfSbufReadString(testSBufReply), "The host does not register dump function\n") == 0);
    HdfSbufFlush(testSBufData);
    HdfSbufFlush(testSBufReply);
    HDF_LOGI("test DevHostDump option dumpHost without dumpHostFunc END");

    // test DevHostRegisterDumpHost NULL input
    HDF_LOGI("test DevHostRegisterDumpHost NULL input BEGIN");
    int32_t ret = DevHostRegisterDumpHost(NULL);
    ASSERT_TRUE(HDF_FAILURE == ret);
    HDF_LOGI("test DevHostRegisterDumpHost NULL input END");

    // test DevHostRegisterDumpHost normal brunch
    HDF_LOGI("test DevHostRegisterDumpHost normal brunch BEGIN");
    ret = DevHostRegisterDumpHost(DevHostTestDumpHostFunc);
    ASSERT_TRUE(HDF_SUCCESS == ret);
    HDF_LOGI("test DevHostRegisterDumpHost normal brunch END");

    // test DevHostDump option dumpHost with dumpHostFunc
    HDF_LOGI("test DevHostDump option dumpHost with dumpHostFunc BEGIN");
    HdfSbufWriteString(testSBufData, "dumpHost");
    DevHostDump(testSBufData, testSBufReply);
    ASSERT_TRUE(strcmp(HdfSbufReadString(testSBufReply), "test_host_dump\n") == 0);
    HdfSbufFlush(testSBufData);
    HdfSbufFlush(testSBufReply);
    HDF_LOGI("test DevHostDump option dumpHost with dumpHostFunc END");
}

HWTEST_F(DevHostTest, DevHostDumpServiceTest, TestSize.Level1)
{
    HdfSBuf * testSBufData = HdfSbufTypedObtain(SBUF_RAW);
    HdfSBuf * testSBufReply = HdfSbufTypedObtain(SBUF_RAW);
    DevHostDumpInit();
    // test DevHostRegisterDumpService NULL input
    HDF_LOGI("test DevHostRegisterDumpService NULL input BEGIN");
    int32_t ret = DevHostRegisterDumpService(nullptr, DevHostTestDumpServiceFunc);
    ASSERT_TRUE(HDF_FAILURE == ret);
    HDF_LOGI("test DevHostRegisterDumpService NULL input END");

    // test DevHostRegisterDumpService with sample_driver_service
    HDF_LOGI("test DevHostRegisterDumpService with sample_driver_service BEGIN");
    ret = DevHostRegisterDumpService("sample_driver_service", DevHostTestDumpServiceFunc);
    ASSERT_TRUE(HDF_SUCCESS == ret);
    HDF_LOGI("test DevHostRegisterDumpService with sample_driver_service END");

    // test DevHostRegisterDumpService with sample_driver_service Redundantly
    HDF_LOGI("test DevHostRegisterDumpService with sample_driver_service Redundantly BEGIN");
    ret = DevHostRegisterDumpService("sample_driver_service", DevHostTestDumpServiceFunc);
    ASSERT_TRUE(HDF_FAILURE == ret);
    HDF_LOGI("test DevHostRegisterDumpService with sample_driver_service Redundantly END");

    // test DevDumpHost with option dumpService and wrong service name
    HDF_LOGI("test DevDumpHost with option dumpService and wrong service name BEGIN");
    HdfSbufWriteString(testSBufData, "dumpService");
    HdfSbufWriteString(testSBufData, "no_driver_service");
    DevHostDump(testSBufData, testSBufReply);
    ASSERT_TRUE(strcmp(HdfSbufReadString(testSBufReply), "The service does not register dump function\n") == 0);
    HdfSbufFlush(testSBufData);
    HdfSbufFlush(testSBufReply);
    HDF_LOGI("test DevDumpHost with option dumpService and wrong service name END");

    // test DevDumpHost with option dumpService and correct service name
    HDF_LOGI("test DevDumpHost with option dumpService and correct service name BEGIN");
    HdfSbufWriteString(testSBufData, "dumpService");
    HdfSbufWriteString(testSBufData, "sample_driver_service");
    DevHostDump(testSBufData, testSBufReply);
    ASSERT_TRUE(strcmp(HdfSbufReadString(testSBufReply), "test_service_dump\n") == 0);
    HdfSbufFlush(testSBufData);
    HdfSbufFlush(testSBufReply);
    DevHostDumpDeInit();
    HDF_LOGI("test DevDumpHost with option dumpService and correct service name END");
}
} // namespace OHOS
