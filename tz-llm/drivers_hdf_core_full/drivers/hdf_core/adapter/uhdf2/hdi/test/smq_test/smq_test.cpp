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

#include <base/hdi_smq.h>
#include <gtest/gtest.h>
#include <idevmgr_hdi.h>
#include <iservmgr_hdi.h>

#include "sample_hdi.h"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <sstream>

#define HDF_LOG_TAG smq_test

namespace {
using namespace testing::ext;
using OHOS::IRemoteObject;
using OHOS::sptr;
using OHOS::HDI::Base::SharedMemQueue;
using OHOS::HDI::Base::SharedMemQueueMeta;
using OHOS::HDI::Base::SmqType;
using OHOS::HDI::DeviceManager::V1_0::IDeviceManager;
using OHOS::HDI::ServiceManager::V1_0::IServiceManager;
static constexpr const char *TEST_SERVICE_HOST_NAME = "sample_host";
static constexpr const char *SERVICE_NAME = "sample_driver_service";
static constexpr const char16_t *TEST_SERVICE_INTERFACE_DESC = u"hdf.test.sampele_service";
static constexpr int SMQ_TEST_QUEUE_SIZE = 10;
static constexpr uint32_t HOST_PID_BUFF_SIZE = 20;
static constexpr uint32_t CMD_RESULT_BUFF_SIZE = 1024;
} // namespace

static std::vector<std::string> Split(const std::string &src, const std::string &limit)
{
    std::vector<std::string> result;
    if (src.empty()) {
        return result;
    }

    if (limit.empty()) {
        result.push_back(src);
        return result;
    }

    size_t begin = 0;
    size_t pos = src.find(limit, begin);
    while (pos != std::string::npos) {
        std::string element = src.substr(begin, pos - begin);
        if (!element.empty()) {
            result.push_back(element);
        }
        begin = pos + limit.size();
        pos = src.find(limit, begin);
    }

    if (begin < src.size()) {
        std::string element = src.substr(begin);
        result.push_back(element);
    }
    return result;
}

static bool QueryPidOfHostName(const std::string &hostName, int &hostPid)
{
    if (hostName.empty()) {
        return false;
    }

    // ps -ef | grep hostName | grep -v "grep" | sed 's/  */ /g' | cut -d ' ' -f 2
    std::ostringstream cmdStr;
    cmdStr << "ps -ef | grep '" << hostName << "' | grep -v 'grep' | ";
    cmdStr << "sed 's/  */ /g' | cut -d ' ' -f 2";
    FILE *res = popen(cmdStr.str().c_str(), "r");
    if (res == nullptr) {
        HDF_LOGE("[%{public}s:%{public}d] failed to popen '%{public}s'", __func__, __LINE__, cmdStr.str().c_str());
        return false;
    }

    HDF_LOGD("[%{public}s:%{public}d] popen '%{public}s'", __func__, __LINE__, cmdStr.str().c_str());
    char resBuf[HOST_PID_BUFF_SIZE] = {0};
    (void)fread(resBuf, HOST_PID_BUFF_SIZE - 1, 1, res);
    pclose(res);

    // if no result, can not convert string to number
    if (strlen(resBuf) == 0) {
        return false;
    }

    hostPid = std::stoi(resBuf);
    return true;
}

static bool QueryOpendFdsByHostPid(int hostPid, std::set<int> &fds)
{
    if (hostPid <= 0) {
        HDF_LOGE("[%{public}s:%{public}d] invalid hostPid", __func__, __LINE__);
        return false;
    }

    std::ostringstream cmdStr;
    cmdStr << "ls /proc/" << hostPid << "/fd | sort -n | xargs";

    FILE *res = popen(cmdStr.str().c_str(), "r");
    if (res == nullptr) {
        HDF_LOGE("[%{public}s:%{public}d] failed to popen '%{public}s'", __func__, __LINE__, cmdStr.str().c_str());
        return false;
    }

    int resFd = fileno(res);
    HDF_LOGD("[%{public}s:%{public}d] popen '%{public}s'", __func__, __LINE__, cmdStr.str().c_str());
    char resBuf[CMD_RESULT_BUFF_SIZE] = {0};
    (void)fread(resBuf, CMD_RESULT_BUFF_SIZE - 1, 1, res);
    pclose(res);

    std::vector<std::string> fdsResult = Split(resBuf, " ");
    for (const auto &fdStr : fdsResult) {
        int fd = std::stoi(fdStr);
        if (fd == resFd) {
            continue;
        }
        fds.insert(fd);
    }
    return true;
}

static bool QueryOpenedFdsByHostName(const std::string &hostName, std::set<int> &fds)
{
    int hostPid = 0;
    if (!QueryPidOfHostName(hostName, hostPid)) {
        return false;
    }

    if (!QueryOpendFdsByHostPid(hostPid, fds)) {
        return false;
    }

    return true;
}

static bool QueryOpenedFdsOfCurrentHost(std::set<int> &fds)
{
    int pid = getpid();
    if (!QueryOpendFdsByHostPid(pid, fds)) {
        return false;
    }
    return true;
}

static void PrintFds(const std::string &info, const std::set<int> &fds)
{
    std::cout << info << ":";
    for (const auto& fd: fds) {
        std::cout << fd << ' ';
    }
    std::cout << std::endl;
}

class SmqTest : public testing::Test {
public:
    static void SetUpTestCase()
    {
        auto devmgr = IDeviceManager::Get();
        if (devmgr != nullptr) {
            HDF_LOGI("%{public}s:%{public}d", __func__, __LINE__);
            devmgr->LoadDevice(SERVICE_NAME);
        }
    }

    static void TearDownTestCase()
    {
        auto devmgr = IDeviceManager::Get();
        if (devmgr != nullptr) {
            HDF_LOGI("%{public}s:%{public}d", __func__, __LINE__);
            devmgr->UnloadDevice(SERVICE_NAME);
        }
    }

    void SetUp() {};
    void TearDown() {};
};

HWTEST_F(SmqTest, SmqTest001, TestSize.Level1)
{
    std::set<int> fds1;
    EXPECT_TRUE(QueryOpenedFdsOfCurrentHost(fds1));
    std::cout << "File descriptor opened by the current host at the current time:\n";
    PrintFds("current host:", fds1);
    {
        std::unique_ptr<SharedMemQueue<int32_t>> smq =
            std::make_unique<SharedMemQueue<int32_t>>(SMQ_TEST_QUEUE_SIZE, SmqType::SYNCED_SMQ);
        ASSERT_TRUE(smq->IsGood());
        ASSERT_NE(smq->GetMeta(), nullptr);

        OHOS::MessageParcel data;
        ASSERT_TRUE(smq->GetMeta()->Marshalling(data));
        std::shared_ptr<SharedMemQueueMeta<int32_t>> replyMeta = SharedMemQueueMeta<int32_t>::UnMarshalling(data);
        ASSERT_NE(replyMeta, nullptr);
        std::shared_ptr<SharedMemQueue<int32_t>> smq2 = std::make_shared<SharedMemQueue<int32_t>>(*replyMeta);
        ASSERT_TRUE(smq2->IsGood());
    }

    std::set<int> fds2;
    EXPECT_TRUE(QueryOpenedFdsOfCurrentHost(fds2));
    std::cout << "File descriptor opened by the current host at the current time:\n";
    PrintFds("current host:", fds2);

    std::set<int> unclosedFds;
    std::set_difference(fds2.begin(), fds2.end(), fds1.begin(), fds1.end(),
        std::inserter(unclosedFds, unclosedFds.begin()));
    EXPECT_TRUE(unclosedFds.empty());
    if (!unclosedFds.empty()) {
        PrintFds("unclosed fds:", unclosedFds);
    }
}

HWTEST_F(SmqTest, SmqTest002, TestSize.Level1)
{
    // query fds of current host
    std::set<int> hostFds1;
    EXPECT_TRUE(QueryOpenedFdsOfCurrentHost(hostFds1));
    std::cout << "File descriptor opened by the current host at the current time:\n";
    PrintFds("current host:", hostFds1);

    // query fds of sample_host
    std::set<int> servFds1;
    EXPECT_TRUE(QueryOpenedFdsByHostName(TEST_SERVICE_HOST_NAME, servFds1));
    std::cout << "File descriptor opened by the sample_host at the current time:\n";
    PrintFds("sample_host:", servFds1);

    {
        auto servmgr = IServiceManager::Get();
        ASSERT_TRUE(servmgr != nullptr);

        auto sampleService = servmgr->GetService(SERVICE_NAME);
        ASSERT_TRUE(sampleService != nullptr);

        OHOS::MessageParcel data;
        OHOS::MessageParcel reply;
        OHOS::MessageOption option;
        std::unique_ptr<SharedMemQueue<SampleSmqElement>> smq =
            std::make_unique<SharedMemQueue<SampleSmqElement>>(SMQ_TEST_QUEUE_SIZE, SmqType::SYNCED_SMQ);
        ASSERT_TRUE(smq->IsGood());

        bool ret = data.WriteInterfaceToken(TEST_SERVICE_INTERFACE_DESC);
        ASSERT_EQ(ret, true);
        ret = smq->GetMeta()->Marshalling(data);
        ASSERT_TRUE(ret);
        data.WriteUint32(1);

        int status = sampleService->SendRequest(SAMPLE_TRANS_SMQ, data, reply, option);
        ASSERT_EQ(status, 0);
    }

    // query fds of current host
    std::set<int> hostFds2;
    EXPECT_TRUE(QueryOpenedFdsOfCurrentHost(hostFds2));
    std::cout << "File descriptor opened by the current host at the current time:\n";
    PrintFds("current host:", hostFds2);

    // query fds of sample_host
    std::set<int> servFds2;
    EXPECT_TRUE(QueryOpenedFdsByHostName(TEST_SERVICE_HOST_NAME, servFds2));
    std::cout << "File descriptor opened by the sample_host at the current time:\n";
    PrintFds("sample_host:", servFds2);

    // the fds that host didn't to close
    std::set<int> hostUnclosedFds;
    std::set_difference(hostFds2.begin(), hostFds2.end(), hostFds1.begin(), hostFds1.end(),
        std::inserter(hostUnclosedFds, hostUnclosedFds.begin()));
    EXPECT_TRUE(hostUnclosedFds.empty());
    if (!hostUnclosedFds.empty()) {
        PrintFds("current host unclosed fds:", hostUnclosedFds);
    }

    // the fds that sample_host didn't to close
    std::set<int> servHostUnclosedFds;
    std::set_difference(servFds2.begin(), servFds2.end(), servFds1.begin(), servFds1.end(),
        std::inserter(servHostUnclosedFds, servHostUnclosedFds.begin()));
    EXPECT_TRUE(servHostUnclosedFds.empty());
    if (!servHostUnclosedFds.empty()) {
        PrintFds("sample_host unclosed fds:", servHostUnclosedFds);
    }
}