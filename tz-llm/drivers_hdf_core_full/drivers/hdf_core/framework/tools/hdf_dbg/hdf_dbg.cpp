/*
 * Copyright (c) 2022-2023 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include <algorithm>
#include <hdf_io_service.h>
#include <idevmgr_hdi.h>
#include <iostream>
#include <iservmgr_hdi.h>
#include <osal_time.h>
#include <string>
#include <string_ex.h>
#include <vector>

#define HDF_LOG_TAG hdf_dbg

static constexpr uint32_t DATA_SIZE = 5000;
static constexpr uint32_t FUNC_IDX = 1;
static constexpr uint32_t SERVER_NAME_IDX = 3;
static constexpr uint32_t INTERFACE_DESC_IDX = 4;
static constexpr uint32_t CMD_ID_IDX = 5;
static constexpr uint32_t PARA_CNT_IDX = 6;
static constexpr uint32_t PARA_MULTIPLE = 2;
static constexpr uint32_t WAIT_TIME = 100;
static constexpr uint32_t HELP_INFO_PARA_CNT = 2;
static constexpr uint32_t GET_INFO_FUNC_NUMS = 5;
static constexpr int32_t DBG_HDI_PARA_MIN_LEN = 7;
static constexpr int32_t DBG_HDI_SERVICE_LOAD_IDX = 2;
static constexpr int32_t QUERY_INFO_PARA_CNT = 3;
static constexpr int32_t ALIGN_SIZE = 30;
static constexpr int32_t PARAM_IN_OUT_SIZE = 2;
static constexpr int32_t PARAM_IN_SIZE = 1;
static constexpr int32_t PARAM_IN_IDX = 0;
static constexpr int32_t PARAM_OUT_IDX = 1;
static constexpr const char *HELP_COMMENT =
    " hdf_dbg menu:  \n"
    " hdf_dbg -h   :display help information\n"
    " hdf_dbg -q   :query all service and device information\n"
    " hdf_dbg -q 0 :query service information of kernel space\n"
    " hdf_dbg -q 1 :query service information of user space\n"
    " hdf_dbg -q 2 :query device information of kernel space\n"
    " hdf_dbg -q 3 :query device information use space\n"
    " hdf_dbg -d   :debug hdi interface\n"
    "   detailed usage:\n"
    "   debug hdi interface, parameterType can be int or string now, for example:\n"
    "     hdf_dbg -d loadFlag serviceName interfaceToken cmd parameterInCount parameterInType parameterInValue\n"
    "     hdf_dbg -d loadFlag serviceName interfaceToken cmd parameterInCount,parameterOutCount parameterInType "
    "parameterInValue parameterOutType\n"
    "     detailed examples:\n"
    "       hdf_dbg -d 1 sample_driver_service hdf.test.sampele_service 1 2 int 100 int 200\n"
    "       hdf_dbg -d 1 sample_driver_service hdf.test.sampele_service 1 2,1 int 100 int 200 int\n"
    "       hdf_dbg -d 0 sample_driver_service hdf.test.sampele_service 7 1 string foo\n";

using GetInfoFunc = void (*)();
static void GetAllServiceUserSpace();
static void GetAllServiceKernelSpace();
static void GetAllDeviceUserSpace();
static void GetAllDeviceKernelSpace();
static void GetAllInformation();

GetInfoFunc g_getInfoFuncs[GET_INFO_FUNC_NUMS] = {
    GetAllServiceKernelSpace,
    GetAllServiceUserSpace,
    GetAllDeviceKernelSpace,
    GetAllDeviceUserSpace,
    GetAllInformation,
};

using OHOS::MessageOption;
using OHOS::MessageParcel;
using OHOS::HDI::DeviceManager::V1_0::HdiDevHostInfo;
using OHOS::HDI::DeviceManager::V1_0::IDeviceManager;
using OHOS::HDI::ServiceManager::V1_0::HdiServiceInfo;
using OHOS::HDI::ServiceManager::V1_0::IServiceManager;
using std::cout;
using std::endl;

class HdfDbg {
public:
    bool loadDevice;
    std::string serviceName;
    std::u16string descriptor;
    int32_t cmd;
    int32_t inParam;
    int32_t outParam;
    std::vector<std::vector<std::string>> inParamVec;
    std::vector<std::string> outParamVec;
};

static void PrintHelp()
{
    cout << HELP_COMMENT;
}

static int StrToInt(std::string tempstr)
{
    int32_t num = GET_INFO_FUNC_NUMS;
    if (std::all_of(tempstr.begin(), tempstr.end(), ::isdigit)) {
        num = std::stoi(tempstr);
    }
    return num;
}

static void SetPadAlign(std::string &name, char padChar, int32_t size)
{
    int32_t padCnt = size - static_cast<int32_t>(name.length());

    padCnt = (padCnt <= 0) ? 0 : padCnt;
    name.append(padCnt, padChar);
}

static void PrintAllServiceInfoUser(std::vector<HdiServiceInfo> &serviceInfos)
{
    uint32_t cnt = 0;
    std::string titleName = "serviceName";
    SetPadAlign(titleName, ' ', ALIGN_SIZE);

    cout << "display service info in user space, format:" << endl;
    cout << titleName << ":devClass" << "\t:devId" << endl;

    for (auto &info : serviceInfos) {
        SetPadAlign(info.serviceName, ' ', ALIGN_SIZE);
        cout << info.serviceName << ":0x" << std::hex << info.devClass << "\t:0x" << info.devId << endl;
        cnt++;
    }

    cout << "total " << std::dec << cnt << " services in user space" << endl;
}

static void PrintAllServiceInfoKernel(struct HdfSBuf *data, bool flag)
{
    uint32_t cnt = 0;
    std::string titleName = "serviceName";

    SetPadAlign(titleName, ' ', ALIGN_SIZE);
    cout << "display service info in kernel space, format:" << endl;
    cout << titleName << ":devClass" << "\t:devId" << endl;

    while (flag) {
        const char *servName = HdfSbufReadString(data);
        if (servName == nullptr) {
            break;
        }

        uint16_t devClass;
        if (!HdfSbufReadUint16(data, &devClass)) {
            return;
        }

        uint32_t devId;
        if (!HdfSbufReadUint32(data, &devId)) {
            return;
        }

        std::string serviceName = servName;
        SetPadAlign(serviceName, ' ', ALIGN_SIZE);
        cout << serviceName << ":0x" << std::hex << devClass << "\t:0x" << devId << endl;
        cnt++;
    }

    cout << "total " << std::dec << cnt << " services in kernel space" << endl;
}

static void PrintALLDeviceInfoUser(std::vector<HdiDevHostInfo> &deviceInfos)
{
    cout << "display device info in user space, format:" << endl;
    std::string titleHostName = "hostName";
    SetPadAlign(titleHostName, ' ', ALIGN_SIZE);

    cout << titleHostName << ":hostId" << endl;

    std::string titleDevName = "deviceName";
    std::string titleSrvName = ":serviceName";
    SetPadAlign(titleDevName, ' ', ALIGN_SIZE);
    SetPadAlign(titleSrvName, ' ', ALIGN_SIZE);

    cout << "\t" << titleDevName << ":deviceId  \t" << titleSrvName << endl;
    uint32_t hostCnt = 0;
    uint32_t devNodeCnt = 0;

    for (auto &info : deviceInfos) {
        SetPadAlign(info.hostName, ' ', ALIGN_SIZE);
        cout << info.hostName << ":0x" << std::hex << info.hostId << endl;
        for (auto &dev : info.devInfo) {
            SetPadAlign(dev.deviceName, ' ', ALIGN_SIZE);
            SetPadAlign(dev.servName, ' ', ALIGN_SIZE);
            cout << "\t" << dev.deviceName << ":0x" << std::hex << dev.devId << "\t:" << dev.servName << endl;
            devNodeCnt++;
        }
        hostCnt++;
    }

    cout << "total " << std::dec << hostCnt << " hosts, " << devNodeCnt << " devNodes in user space" << endl;
}

static int32_t PrintOneHostInfoKernel(struct HdfSBuf *data, uint32_t &devNodeCnt)
{
    const char *hostName = HdfSbufReadString(data);
    if (hostName == nullptr) {
        return HDF_FAILURE;
    }

    uint32_t hostId;
    if (!HdfSbufReadUint32(data, &hostId)) {
        cout << "PrintOneHostInfoKernel HdfSbufReadUint32 hostId failed" << endl;
        return HDF_FAILURE;
    }

    std::string hostNameStr = hostName;
    SetPadAlign(hostNameStr, ' ', ALIGN_SIZE);
    cout << hostNameStr << ":0x" << std::hex << hostId << endl;

    uint32_t devCnt;
    if (!HdfSbufReadUint32(data, &devCnt)) {
        cout << "PrintOneHostInfoKernel HdfSbufReadUint32 devCnt failed" << endl;
        return HDF_FAILURE;
    }

    for (uint32_t i = 0; i < devCnt; i++) {
        const char *str = HdfSbufReadString(data);
        std::string deviceName = (str == nullptr) ? "" : str;
        SetPadAlign(deviceName, ' ', ALIGN_SIZE);

        uint32_t devId;
        if (!HdfSbufReadUint32(data, &devId)) {
            cout << "PrintOneHostInfoKernel HdfSbufReadUint32 devId failed" << endl;
            return HDF_FAILURE;
        }

        str = HdfSbufReadString(data);
        std::string servName = (str == nullptr) ? "" : str;
        SetPadAlign(servName, ' ', ALIGN_SIZE);
        cout << "\t" << deviceName << ":0x" << std::hex << devId << "\t:" << servName << endl;
    }
    devNodeCnt += devCnt;

    return HDF_SUCCESS;
}
static void PrintAllDeviceInfoKernel(struct HdfSBuf *data, bool flag)
{
    uint32_t hostCnt = 0;
    uint32_t devNodeCnt = 0;

    std::string titleHostName = "hostName";
    SetPadAlign(titleHostName, ' ', ALIGN_SIZE);
    cout << "display device info in kernel space, format:" << endl;
    cout << titleHostName << ":hostId" << endl;

    std::string titleDevName = "deviceName";
    std::string titleSrvName = "serviceName";
    SetPadAlign(titleDevName, ' ', ALIGN_SIZE);
    SetPadAlign(titleSrvName, ' ', ALIGN_SIZE);
    cout << "\t" << titleDevName << ":deviceId  \t:" << titleSrvName << endl;

    while (flag) {
        if (PrintOneHostInfoKernel(data, devNodeCnt) == HDF_FAILURE) {
            break;
        }
        hostCnt++;
    }

    cout << "total " << std::dec << hostCnt << " hosts, " << devNodeCnt << " devNodes in kernel space" << endl;
}

static bool ParseParameterCount(int argc, char **argv, HdfDbg &info)
{
    std::string paramCount = argv[PARA_CNT_IDX];
    std::vector<std::string> result;
    OHOS::SplitStr(paramCount, ",", result);
    info.inParam = 0;
    info.outParam = 0;
    if (result.size() == PARAM_IN_SIZE) {
        info.inParam = StrToInt(result[PARAM_IN_IDX]);
    } else if (result.size() == PARAM_IN_OUT_SIZE) {
        info.inParam = StrToInt(result[PARAM_IN_IDX]);
        info.outParam = StrToInt(result[PARAM_OUT_IDX]);
    } else {
        cout << "parameter count parse failed, input: " << paramCount <<
            " it should be paramIn,paramOut or paramIn" << endl;
        return false;
    }
    if ((info.inParam * PARA_MULTIPLE + info.outParam) != (argc - PARA_CNT_IDX - 1)) {
        cout << "parameter count error, please check your input and output parameters" << endl;
        return false;
    }
    return true;
}

static bool ParseParameterIn(int argc, char **argv, HdfDbg &info)
{
    int32_t paraTypeIdx = PARA_CNT_IDX + 1;
    for (int32_t i = 0; i < info.inParam; i++) {
        int32_t paraValueIdx = paraTypeIdx + 1;
        if (strcmp(argv[paraTypeIdx], "string") != 0 && strcmp(argv[paraTypeIdx], "int") != 0) {
            cout << "parameterType not support:" << argv[paraTypeIdx] << endl;
            return false;
        }
        info.inParamVec.push_back(std::vector<std::string>());
        info.inParamVec[i].push_back(argv[paraTypeIdx]);
        info.inParamVec[i].push_back(argv[paraValueIdx]);
        paraTypeIdx += PARA_MULTIPLE;
    }
    return true;
}

static bool ParseParameterOut(int argc, char **argv, HdfDbg &info)
{
    int32_t paraTypeIdx = PARA_CNT_IDX + 1 + info.inParam * PARA_MULTIPLE;
    for (int32_t i = 0; i < info.outParam; i++) {
        if (strcmp(argv[paraTypeIdx], "string") != 0 && strcmp(argv[paraTypeIdx], "int") != 0) {
            cout << "parameterType not support:" << argv[paraTypeIdx] << endl;
            return false;
        }
        info.outParamVec.push_back(argv[paraTypeIdx]);
        paraTypeIdx++;
    }
    return true;
}

static bool ParseHdfDbg(int argc, char **argv, HdfDbg &info)
{
    if (argc < DBG_HDI_PARA_MIN_LEN) {
        return false;
    }
    info.loadDevice = strcmp(argv[DBG_HDI_SERVICE_LOAD_IDX], "1") == 0 ? true : false;
    info.serviceName = argv[SERVER_NAME_IDX];
    info.descriptor = OHOS::Str8ToStr16(argv[INTERFACE_DESC_IDX]);
    info.cmd = StrToInt(argv[CMD_ID_IDX]);
    if (!ParseParameterCount(argc, argv, info) || !ParseParameterIn(argc, argv, info) ||
        !ParseParameterOut(argc, argv, info)) {
        return false;
    }
    return true;
}

static int32_t InjectDebugHdi(int argc, char **argv)
{
    HdfDbg info;
    if (!ParseHdfDbg(argc, argv, info)) {
        PrintHelp();
        return HDF_FAILURE;
    }
    auto servmgr = IServiceManager::Get();
    auto devmgr = IDeviceManager::Get();
    if (info.loadDevice) {
        devmgr->LoadDevice(info.serviceName);
        OsalMSleep(WAIT_TIME);
    }
    MessageParcel data;
    data.WriteInterfaceToken(info.descriptor);
    for (int32_t i = 0; i < info.inParam; i++) {
        if (info.inParamVec[i][0] == "string") {
            data.WriteCString(info.inParamVec[i][1].c_str());
        } else {
            data.WriteInt32(StrToInt(info.inParamVec[i][1]));
        }
    }
    MessageParcel reply;
    MessageOption option;
    int32_t ret = HDF_FAILURE;
    auto service = servmgr->GetService(info.serviceName.c_str());
    if (service == nullptr) {
        cout << "getService " << info.serviceName << " failed" << endl;
        goto END;
    }
    ret = service->SendRequest(info.cmd, data, reply, option);
    cout << "call service " << info.serviceName << " hdi cmd:" << info.cmd << " return:" << ret << endl;
    for (int32_t i = 0; i < info.outParam; i++) {
        if (info.outParamVec[i] == "string") {
            cout << "output parameter" << i << ": parameterType is string, parameterValue is " <<
            reply.ReadCString() << endl;
        } else {
            int32_t replyInt;
            reply.ReadInt32(replyInt);
            cout << "output parameter" << i << ": parameterType is int, parameterValue is " << replyInt << endl;
        }
    }
END:
    if (info.loadDevice) {
        devmgr->UnloadDevice(info.serviceName);
    }
    return ret;
}

static void GetAllServiceUserSpace()
{
    auto servmgr = IServiceManager::Get();
    if (servmgr == nullptr) {
        cout << "GetAllServiceUserSpace get ServiceManager failed" << endl;
        return;
    }

    std::vector<HdiServiceInfo> serviceInfos;
    (void)servmgr->ListAllService(serviceInfos);

    PrintAllServiceInfoUser(serviceInfos);
}

static void GetAllServiceKernelSpace()
{
    struct HdfSBuf *data = HdfSbufObtain(DATA_SIZE);
    if (data == nullptr) {
        cout << "GetAllServiceKernelSpace HdfSbufObtain failed" << endl;
        return;
    }

    int32_t ret = HdfListAllService(data);
    OsalMSleep(WAIT_TIME);
    if (ret == HDF_SUCCESS) {
        PrintAllServiceInfoKernel(data, true);
    } else {
        PrintAllServiceInfoKernel(data, false);
    }

    HdfSbufRecycle(data);
}

static void GetAllDeviceUserSpace()
{
    auto devmgr = IDeviceManager::Get();
    if (devmgr == nullptr) {
        cout << "GetAllDeviceUserSpace get DeviceManager failed" << endl;
        return;
    }

    std::vector<HdiDevHostInfo> deviceInfos;
    (void)devmgr->ListAllDevice(deviceInfos);
    PrintALLDeviceInfoUser(deviceInfos);
}

static void GetAllDeviceKernelSpace()
{
    struct HdfSBuf *data = HdfSbufObtain(DATA_SIZE);
    if (data == nullptr) {
        cout << "GetAllDeviceKernelSpace HdfSbufObtain failed" << endl;
        return;
    }

    int32_t ret = HdfListAllDevice(data);
    OsalMSleep(WAIT_TIME);
    if (ret == HDF_SUCCESS) {
        PrintAllDeviceInfoKernel(data, true);
    } else {
        PrintAllDeviceInfoKernel(data, false);
    }

    HdfSbufRecycle(data);
}

static void GetAllInformation()
{
    GetAllServiceUserSpace();
    cout << endl;
    GetAllServiceKernelSpace();
    cout << endl;
    GetAllDeviceUserSpace();
    cout << endl;
    GetAllDeviceKernelSpace();
}

int main(int argc, char **argv)
{
    if (argc == 1 || (argc == HELP_INFO_PARA_CNT && strcmp(argv[FUNC_IDX], "-h") == 0)) {
        PrintHelp();
    } else if (argc == QUERY_INFO_PARA_CNT || argc == QUERY_INFO_PARA_CNT - 1) {
        if (strcmp(argv[FUNC_IDX], "-q") != 0) {
            PrintHelp();
            return HDF_FAILURE;
        }

        if (argc == QUERY_INFO_PARA_CNT - 1) {
            g_getInfoFuncs[GET_INFO_FUNC_NUMS - 1]();
            return HDF_SUCCESS;
        }
        std::string argvStr = argv[QUERY_INFO_PARA_CNT - 1];
        uint32_t queryIdx = static_cast<uint32_t>(StrToInt(argvStr));
        if (queryIdx < GET_INFO_FUNC_NUMS - 1) {
            g_getInfoFuncs[queryIdx]();
        } else {
            PrintHelp();
            return HDF_FAILURE;
        }
    } else if (argc > QUERY_INFO_PARA_CNT) {
        if (strcmp(argv[FUNC_IDX], "-d") == 0) {
            return InjectDebugHdi(argc, argv);
        } else {
            PrintHelp();
            return HDF_FAILURE;
        }
    }

    return HDF_SUCCESS;
}
