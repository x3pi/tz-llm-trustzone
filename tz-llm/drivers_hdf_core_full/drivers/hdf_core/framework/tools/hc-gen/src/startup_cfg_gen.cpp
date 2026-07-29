/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include "startup_cfg_gen.h"
#include <algorithm>
#include <string>
#include "ast.h"
#include "file.h"
#include "logger.h"

using namespace OHOS::Hardware;

static constexpr const char *BOOT_CONFIG_TOP =
    "{\n"
    "    \"jobs\" : [{\n"
    "            \"name\" : \"post-fs-data\",\n"
    "            \"cmds\" : [\n"
    "                \"start hdf_devhost\"\n"
    "            ]\n"
    "        }\n"
    "    ],\n"
    "    \"services\" : [\n";
static constexpr const char *BOOT_CONFIG_BOTTOM =
    "    ]\n"
    "}\n";
static constexpr const char *SERVICE_TOP =
    "        {\n"
    "            \"name\" : ";
static constexpr const char *PATH_INFO     = "            \"path\" : [\"/vendor/bin/hdf_devhost\", ";
static constexpr const char *UID_INFO      = "            \"uid\" : ";
static constexpr const char *GID_INFO      = "            \"gid\" : [";
static constexpr const char *CAPS_INFO     = "            \"caps\" : [";
static constexpr const char *DYNAMIC_INFO  = "            \"ondemand\" : true,\n";
static constexpr const char *SECON_INFO    = "            \"secon\" : \"u:r:";
static constexpr const char *CRITICAL_INFO = "            \"critical\" : [";
static constexpr uint32_t INVALID_PRIORITY = 0xffffffff;
static constexpr const char *SAND_BOX_INFO = "            \"sandbox\" : ";
static constexpr uint32_t INVALID_SAND_BOX = 0xffffffff;
StartupCfgGen::StartupCfgGen(const std::shared_ptr<Ast> &ast) : Generator(ast)
{
}

void StartupCfgGen::HeaderTopOutput()
{
    ofs_ << BOOT_CONFIG_TOP;
}

void StartupCfgGen::HeaderBottomOutput()
{
    ofs_ << BOOT_CONFIG_BOTTOM;
    ofs_.close();
}

bool StartupCfgGen::Output()
{
    if (!Initialize()) {
        return false;
    }
    if (!TemplateNodeSeparate()) {
        return false;
    }
    HeaderTopOutput();

    if (!GetHostInfo()) {
        return false;
    }
    HostInfosOutput();

    HeaderBottomOutput();

    return true;
}

bool StartupCfgGen::Initialize()
{
    std::string outFileName = Option::Instance().GetOutputName();
    if (outFileName.empty()) {
        outFileName = Option::Instance().GetSourceNameBase();
    }
    outFileName = Util::File::StripSuffix(outFileName).append(".cfg");
    outFileName_ = Util::File::FileNameBase(outFileName);

    ofs_.open(outFileName, std::ofstream::out | std::ofstream::binary);
    if (!ofs_.is_open()) {
        Logger().Error() << "failed to open output file: " << outFileName;
        return false;
    }

    Logger().Debug() << "output: " << outFileName << outFileName_ << '\n';

    return true;
}

void StartupCfgGen::HostInfoOutput(const std::string &name, bool end)
{
    ofs_ << SERVICE_TOP << "\"" << name << "\",\n";

    if (hostInfoMap_[name].dynamicLoad) {
        ofs_ << DYNAMIC_INFO;
    }

    bool flag = false;
    if ((hostInfoMap_[name].processPriority != INVALID_PRIORITY) &&
        (hostInfoMap_[name].threadPriority != INVALID_PRIORITY)) {
        flag = true;
    }
    if (flag) {
        ofs_ << PATH_INFO << "\"" << hostInfoMap_[name].hostId << "\", \"" << name << "\", \"" <<
            hostInfoMap_[name].processPriority << "\", \"" << hostInfoMap_[name].threadPriority << "\"],\n";
    } else {
        ofs_ << PATH_INFO << "\"" << hostInfoMap_[name].hostId << "\", \"" << name << "\"],\n";
    }

    ofs_ << UID_INFO << "\"" << hostInfoMap_[name].hostUID << "\",\n";
    ofs_ << GID_INFO << hostInfoMap_[name].hostGID << "],\n";

    if (!hostInfoMap_[name].hostCaps.empty()) {
        ofs_ << CAPS_INFO << hostInfoMap_[name].hostCaps << "],\n";
    }

    if (!hostInfoMap_[name].hostCritical.empty()) {
        ofs_ << CRITICAL_INFO << hostInfoMap_[name].hostCritical << "],\n";
    }

    if (hostInfoMap_[name].sandBox != INVALID_SAND_BOX) {
        ofs_ << SAND_BOX_INFO << hostInfoMap_[name].sandBox << ",\n";
    }

    ofs_ << SECON_INFO << name << ":s0\"\n";

    ofs_ << TAB TAB << "}";

    if (!end) {
        ofs_<< ",";
    }
    ofs_ << '\n';
}

void StartupCfgGen::InitHostInfo(HostInfo &hostData)
{
    hostData.dynamicLoad = true;
    hostData.hostCaps = "";
    hostData.hostUID = "";
    hostData.hostGID = "";
    hostData.hostPriority = 0;
    hostData.hostId = 0;
    hostData.hostCritical = "";
    hostData.processPriority = INVALID_PRIORITY;
    hostData.threadPriority = INVALID_PRIORITY;
    hostData.sandBox = INVALID_SAND_BOX;
}

bool StartupCfgGen::TemplateNodeSeparate()
{
    return ast_->WalkBackward([this](std::shared_ptr<AstObject> &object, int32_t depth) {
        (void)depth;
        if (object->IsNode() && ConfigNode::CastFrom(object)->GetNodeType() == NODE_TEMPLATE) {
            object->Separate();
            return NOERR;
        }
        return NOERR;
    });
}

void StartupCfgGen::HostInfosOutput()
{
    bool end = false;
    uint32_t cnt = 1;
    const uint32_t size = hostInfoMap_.size();

    std::vector<std::pair<std::string, HostInfo>> vect(hostInfoMap_.begin(), hostInfoMap_.end());

    using ElementType = std::pair<std::string, HostInfo>;
    sort(vect.begin(), vect.end(), [] (const ElementType &p1, const ElementType &p2) -> bool {
        return (p1.second.hostPriority == p2.second.hostPriority) ?
            (p1.second.hostId < p2.second.hostId) : (p1.second.hostPriority < p2.second.hostPriority);
    });

    std::vector<std::pair<std::string, HostInfo>>::iterator it = vect.begin();
    for (; it != vect.end(); ++it, ++cnt) {
        if (cnt == size) {
            end = true;
        }
        HostInfoOutput(it->first, end);
    }
}

void StartupCfgGen::GetConfigArray(const std::shared_ptr<AstObject> &term, std::string &config)
{
    if (term == nullptr) {
        return;
    }

    std::shared_ptr<AstObject> arrayObj = term->Child();
    if (arrayObj == nullptr) {
        return;
    }

    uint16_t arraySize = ConfigArray::CastFrom(arrayObj)->ArraySize();
    std::shared_ptr<AstObject> object = arrayObj->Child();
    while (arraySize && object != nullptr) {
        if (!object->StringValue().empty()) {
            config.append("\"").append(object->StringValue()).append("\"");
            if (arraySize != 1) {
                config.append(", ");
            }
        }

        object = object->Next();
        arraySize--;
    }
}

void StartupCfgGen::GetConfigIntArray(const std::shared_ptr<AstObject> &term, std::string &config)
{
    if (term == nullptr) {
        return;
    }

    std::shared_ptr<AstObject> intArrayObj = term->Child();
    if (intArrayObj == nullptr) {
        return;
    }

    uint16_t arraySize = ConfigArray::CastFrom(intArrayObj)->ArraySize();
    std::shared_ptr<AstObject> object = intArrayObj->Child();
    while (arraySize && object != nullptr) {
        std::string value = std::to_string(object->IntegerValue());
        config.append(value);
        if (arraySize != 1) {
            config.append(", ");
        }

        object = object->Next();
        arraySize--;
    }
}

void StartupCfgGen::GetProcessPriority(const std::shared_ptr<AstObject> &term, HostInfo &hostData)
{
    if (term == nullptr) {
        return;
    }

    std::shared_ptr<AstObject> object = term->Lookup("processPriority", PARSEROP_CONFTERM);
    if (object != nullptr) {
        hostData.processPriority = static_cast<int32_t>(object->Child()->IntegerValue());
    }
    object = term->Lookup("threadPriority", PARSEROP_CONFTERM);
    if (object != nullptr) {
        hostData.threadPriority = static_cast<int32_t>(object->Child()->IntegerValue());
    }
}

void StartupCfgGen::GetHostLoadMode(const std::shared_ptr<AstObject> &hostInfo, HostInfo &hostData)
{
    uint32_t preload;
    std::shared_ptr<AstObject> current = nullptr;
    std::shared_ptr<AstObject> devNodeInfo = nullptr;

    std::shared_ptr<AstObject> devInfo = hostInfo->Child();
    while (devInfo != nullptr) {
        if (!devInfo->IsNode()) {
            devInfo = devInfo->Next();
            continue;
        }

        devNodeInfo = devInfo->Child();
        while (devNodeInfo != nullptr) {
            current = devNodeInfo->Lookup("preload", PARSEROP_CONFTERM);
            if (current == nullptr) {
                devNodeInfo = devNodeInfo->Next();
                continue;
            }

            preload = current->Child()->IntegerValue();
            if (preload == 0 || preload == 1) {
                hostData.dynamicLoad = false;
            }

            devNodeInfo = devNodeInfo->Next();
        }
        devInfo = devInfo->Next();
    }
}

void StartupCfgGen::GetHostGID(const std::shared_ptr<AstObject> &term, std::string &config,
    const std::string &name)
{
    // get array format configuration
    GetConfigArray(term, config);

    // if the array format is not available, get the string format configuration
    if (config.empty()) {
        if (term != nullptr && !term->Child()->StringValue().empty()) {
            config.append("\"").append(term->Child()->StringValue()).append("\"");
        }
    }

    // use the default name as gid
    if (config.empty()) {
        config.append("\"").append(name).append("\"");
    }
}

bool StartupCfgGen::GetHostInfo()
{
    std::shared_ptr<AstObject> deviceInfo = ast_->GetAstRoot()->Lookup("device_info", PARSEROP_CONFNODE);
    std::shared_ptr<AstObject> object = nullptr;
    std::string serviceName;
    HostInfo hostData;
    uint32_t hostId = 0;

    if (deviceInfo == nullptr) {
        Logger().Error() << "do not find device_info node";
        return false;
    }

    std::shared_ptr<AstObject> hostInfo = deviceInfo->Child();
    while (hostInfo != nullptr) {
        object = hostInfo->Lookup("hostName", PARSEROP_CONFTERM);
        if (object == nullptr) {
            hostInfo = hostInfo->Next();
            continue;
        }

        InitHostInfo(hostData);
        serviceName = object->Child()->StringValue();

        object = hostInfo->Lookup("priority", PARSEROP_CONFTERM);
        if (object != nullptr) {
            hostData.hostPriority = object->Child()->IntegerValue();
        }

        hostData.hostUID = serviceName;
        object = hostInfo->Lookup("uid", PARSEROP_CONFTERM);
        if (object != nullptr && !object->Child()->StringValue().empty()) {
            hostData.hostUID = object->Child()->StringValue();
        }

        object = hostInfo->Lookup("gid", PARSEROP_CONFTERM);
        GetHostGID(object, hostData.hostGID, serviceName);

        object = hostInfo->Lookup("caps", PARSEROP_CONFTERM);
        GetConfigArray(object, hostData.hostCaps);

        GetHostLoadMode(hostInfo, hostData);

        object = hostInfo->Lookup("critical", PARSEROP_CONFTERM);
        GetConfigIntArray(object, hostData.hostCritical);
        GetProcessPriority(hostInfo, hostData);

        object = hostInfo->Lookup("sandbox", PARSEROP_CONFTERM);
        if (object != nullptr) {
            hostData.sandBox = object->Child()->IntegerValue();
        }

        hostData.hostId = hostId;
        hostInfoMap_.insert(make_pair(serviceName, hostData));
        hostId++;
        hostInfo = hostInfo->Next();
    }
    return true;
}
