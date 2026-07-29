/*
 * Copyright (c) 2021-2023 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include "codegen/cpp_client_proxy_code_emitter.h"
#include "util/file.h"
#include "util/logger.h"
#include "util/string_helper.h"

namespace OHOS {
namespace HDI {
bool CppClientProxyCodeEmitter::ResolveDirectory(const std::string &targetDirectory)
{
    if (ast_->GetASTFileType() == ASTFileType::AST_IFACE || ast_->GetASTFileType() == ASTFileType::AST_ICALLBACK) {
        directory_ = GetFileParentPath(targetDirectory);
    } else {
        return false;
    }

    if (!File::CreateParentDir(directory_)) {
        Logger::E("CppClientProxyCodeEmitter", "Create '%s' failed!", directory_.c_str());
        return false;
    }

    return true;
}

void CppClientProxyCodeEmitter::EmitCode()
{
    switch (mode_) {
        case GenMode::PASSTHROUGH: {
            if (!interface_->IsSerializable()) {
                EmitPassthroughProxySourceFile();
            }
            break;
        }
        case GenMode::IPC: {
            EmitProxyHeaderFile();
            EmitProxySourceFile();
        }
        default:
            break;
    }
}

void CppClientProxyCodeEmitter::EmitProxyHeaderFile()
{
    std::string filePath =
        File::AdapterPath(StringHelper::Format("%s/%s.h", directory_.c_str(), FileName(baseName_ + "Proxy").c_str()));
    File file(filePath, File::WRITE);
    StringBuilder sb;

    EmitLicense(sb);
    EmitHeadMacro(sb, proxyFullName_);
    sb.Append("\n");
    EmitProxyHeaderInclusions(sb);
    sb.Append("\n");
    EmitBeginNamespace(sb);
    sb.Append("\n");
    EmitProxyDecl(sb, "");
    sb.Append("\n");
    EmitEndNamespace(sb);
    sb.Append("\n");
    EmitTailMacro(sb, proxyFullName_);

    std::string data = sb.ToString();
    file.WriteData(data.c_str(), data.size());
    file.Flush();
    file.Close();
}

void CppClientProxyCodeEmitter::EmitProxyHeaderInclusions(StringBuilder &sb)
{
    HeaderFile::HeaderFileSet headerFiles;

    headerFiles.emplace(HeaderFileType::OWN_HEADER_FILE, EmitVersionHeaderName(interfaceName_));
    if (interface_->GetExtendsInterface() != nullptr) {
        headerFiles.emplace(
            HeaderFileType::OWN_HEADER_FILE, EmitHeaderNameByInterface(interface_->GetExtendsInterface(), proxyName_));
    }
    GetHeaderOtherLibInclusions(headerFiles);

    for (const auto &file : headerFiles) {
        sb.AppendFormat("%s\n", file.ToString().c_str());
    }
}

void CppClientProxyCodeEmitter::GetHeaderOtherLibInclusions(HeaderFile::HeaderFileSet &headerFiles) const
{
    headerFiles.emplace(HeaderFileType::OTHER_MODULES_HEADER_FILE, "iproxy_broker");
}

void CppClientProxyCodeEmitter::EmitProxyDecl(StringBuilder &sb, const std::string &prefix)
{
    (void)prefix;
    sb.AppendFormat("class %s : public IProxyBroker<%s> {\n", proxyName_.c_str(),
        EmitDefinitionByInterface(interface_, interfaceName_).c_str());
    sb.Append("public:\n");
    EmitProxyConstructor(sb, TAB);
    sb.Append("\n");
    EmitProxyMethodDecls(sb, TAB);
    sb.Append("\n");
    sb.Append("private:\n");
    EmitProxyConstants(sb, TAB);
    sb.Append("};\n");
}

void CppClientProxyCodeEmitter::EmitProxyConstructor(StringBuilder &sb, const std::string &prefix) const
{
    sb.Append(prefix).AppendFormat("explicit %s(const sptr<IRemoteObject>& remote)", proxyName_.c_str());
    sb.AppendFormat(
        " : IProxyBroker<%s>(remote) {}\n\n", EmitDefinitionByInterface(interface_, interfaceName_).c_str());
    sb.Append(prefix).AppendFormat("virtual ~%s() = default;\n", proxyName_.c_str());
}

void CppClientProxyCodeEmitter::EmitProxyMethodDecls(StringBuilder &sb, const std::string &prefix) const
{
    EmitProxyIsProxyMethodImpl(sb, prefix);
    AutoPtr<ASTInterfaceType> interface = interface_;
    while (interface != nullptr) {
        for (const auto &method : interface->GetMethodsBySystem(Options::GetInstance().GetSystemLevel())) {
            EmitProxyMethodDecl(method, sb, prefix);
            sb.Append("\n");
        }
        interface = interface->GetExtendsInterface();
    }
    EmitProxyMethodDecl(interface_->GetVersionMethod(), sb, prefix);
    for (const auto &method : interface_->GetMethodsBySystem(Options::GetInstance().GetSystemLevel())) {
        sb.Append("\n");
        EmitProxyStaticMethodDecl(method, sb, prefix);
    }
    if (interface_->GetExtendsInterface() == nullptr) {
        sb.Append("\n");
        EmitProxyStaticMethodDecl(interface_->GetVersionMethod(), sb, prefix);
    }
}

void CppClientProxyCodeEmitter::EmitProxyMethodDecl(
    const AutoPtr<ASTMethod> &method, StringBuilder &sb, const std::string &prefix) const
{
    if (method->GetParameterNumber() == 0) {
        sb.Append(prefix).AppendFormat("int32_t %s() override;\n", method->GetName().c_str());
    } else {
        StringBuilder paramStr;
        paramStr.Append(prefix).AppendFormat("int32_t %s(", method->GetName().c_str());

        for (size_t i = 0; i < method->GetParameterNumber(); i++) {
            AutoPtr<ASTParameter> param = method->GetParameter(i);
            EmitProxyMethodParameter(param, paramStr, "");
            if (i + 1 < method->GetParameterNumber()) {
                paramStr.Append(", ");
            }
        }

        paramStr.Append(") override;");

        sb.Append(SpecificationParam(paramStr, prefix + TAB));
        sb.Append("\n");
    }
}

void CppClientProxyCodeEmitter::EmitProxyStaticMethodDecl(
    const AutoPtr<ASTMethod> &method, StringBuilder &sb, const std::string &prefix) const
{
    if (method->GetParameterNumber() == 0) {
        sb.Append(prefix).AppendFormat(
            "static int32_t %s_(const sptr<IRemoteObject> remote);\n", method->GetName().c_str());
    } else {
        StringBuilder paramStr;
        paramStr.Append(prefix).AppendFormat("static int32_t %s_(", method->GetName().c_str());

        for (size_t i = 0; i < method->GetParameterNumber(); i++) {
            AutoPtr<ASTParameter> param = method->GetParameter(i);
            EmitProxyMethodParameter(param, paramStr, "");
            paramStr.Append(", ");
        }
        paramStr.Append("const sptr<IRemoteObject> remote");

        paramStr.Append(");");

        sb.Append(SpecificationParam(paramStr, prefix + TAB));
        sb.Append("\n");
    }
}

void CppClientProxyCodeEmitter::EmitProxyConstants(StringBuilder &sb, const std::string &prefix) const
{
    sb.Append(prefix).AppendFormat(
        "static inline BrokerDelegator<%s> delegator_;\n", EmitDefinitionByInterface(interface_, proxyName_).c_str());
}

void CppClientProxyCodeEmitter::EmitProxyMethodParameter(
    const AutoPtr<ASTParameter> &param, StringBuilder &sb, const std::string &prefix) const
{
    sb.Append(prefix).Append(param->EmitCppParameter());
}

void CppClientProxyCodeEmitter::EmitPassthroughProxySourceFile()
{
    std::string filePath =
        File::AdapterPath(StringHelper::Format("%s/%s.cpp", directory_.c_str(), FileName(proxyName_).c_str()));
    File file(filePath, File::WRITE);
    StringBuilder sb;

    EmitLicense(sb);
    EmitPassthroughProxySourceInclusions(sb);
    sb.Append("\n");
    EmitLogTagMacro(sb, FileName(proxyName_));
    sb.Append("\n");
    EmitBeginNamespace(sb);
    EmitGetMethodImpl(sb, "");
    sb.Append("\n");
    EmitPassthroughGetInstanceMethodImpl(sb, "");
    EmitEndNamespace(sb);

    std::string data = sb.ToString();
    file.WriteData(data.c_str(), data.size());
    file.Flush();
    file.Close();
}

void CppClientProxyCodeEmitter::EmitPassthroughProxySourceInclusions(StringBuilder &sb)
{
    HeaderFile::HeaderFileSet headerFiles;

    headerFiles.emplace(HeaderFileType::OWN_HEADER_FILE, EmitVersionHeaderName(interfaceName_));
    if (Options::GetInstance().GetSystemLevel() == SystemLevel::LITE) {
        headerFiles.emplace(HeaderFileType::CPP_STD_HEADER_FILE, "codecvt");
        headerFiles.emplace(HeaderFileType::CPP_STD_HEADER_FILE, "locale");
    } else {
        headerFiles.emplace(HeaderFileType::OTHER_MODULES_HEADER_FILE, "string_ex");
    }
    headerFiles.emplace(HeaderFileType::OTHER_MODULES_HEADER_FILE, "hdi_support");
    headerFiles.emplace(HeaderFileType::OTHER_MODULES_HEADER_FILE, "hdf_log");

    for (const auto &file : headerFiles) {
        sb.AppendFormat("%s\n", file.ToString().c_str());
    }
}

void CppClientProxyCodeEmitter::EmitPassthroughGetInstanceMethodImpl(StringBuilder &sb,
    const std::string &prefix) const
{
    sb.Append(prefix).AppendFormat("%s %s::Get(const std::string &serviceName, bool isStub)\n",
        interface_->EmitCppType().c_str(), interface_->GetName().c_str());
    sb.Append(prefix).Append("{\n");
    EmitProxyPassthroughtLoadImpl(sb, prefix + TAB);
    sb.Append(prefix + TAB).Append("return nullptr;\n");
    sb.Append(prefix).Append("}\n");
}

void CppClientProxyCodeEmitter::EmitProxySourceFile()
{
    std::string filePath =
        File::AdapterPath(StringHelper::Format("%s/%s.cpp", directory_.c_str(), FileName(proxyName_).c_str()));
    File file(filePath, File::WRITE);
    StringBuilder sb;

    EmitLicense(sb);
    EmitProxySourceInclusions(sb);
    sb.Append("\n");
    EmitLogTagMacro(sb, FileName(proxyName_));
    sb.Append("\n");
    EmitBeginNamespace(sb);
    sb.Append("\n");
    UtilMethodMap utilMethods;
    GetUtilMethods(utilMethods);
    EmitUtilMethods(sb, "", utilMethods, true);
    sb.Append("\n");
    if (interface_->GetExtendsInterface() != nullptr) {
        EmitProxyCastFromMethodImplTemplate(sb, "");
        sb.Append("\n");
    }
    if (!interface_->IsSerializable()) {
        EmitGetMethodImpl(sb, "");
        sb.Append("\n");
        EmitGetInstanceMethodImpl(sb, "");
        sb.Append("\n");
    }
    EmitProxyCastFromMethodImpls(sb, "");
    EmitUtilMethods(sb, "", utilMethods, false);
    EmitProxyMethodImpls(sb, "");
    sb.Append("\n");
    EmitEndNamespace(sb);

    std::string data = sb.ToString();
    file.WriteData(data.c_str(), data.size());
    file.Flush();
    file.Close();
}

void CppClientProxyCodeEmitter::EmitProxySourceInclusions(StringBuilder &sb)
{
    HeaderFile::HeaderFileSet headerFiles;
    headerFiles.emplace(HeaderFileType::OWN_HEADER_FILE, EmitVersionHeaderName(proxyName_));
    GetSourceOtherLibInclusions(headerFiles);
    GetSourceOtherFileInclusions(headerFiles);

    for (const auto &file : headerFiles) {
        sb.AppendFormat("%s\n", file.ToString().c_str());
    }
}

void CppClientProxyCodeEmitter::GetSourceOtherLibInclusions(HeaderFile::HeaderFileSet &headerFiles) const
{
    if (!interface_->IsSerializable()) {
        headerFiles.emplace(HeaderFileType::OTHER_MODULES_HEADER_FILE, "iservmgr_hdi");
    }
    headerFiles.emplace(HeaderFileType::OTHER_MODULES_HEADER_FILE, "hdf_base");
    headerFiles.emplace(HeaderFileType::OTHER_MODULES_HEADER_FILE, "hdf_log");
    headerFiles.emplace(HeaderFileType::OTHER_MODULES_HEADER_FILE, "message_option");
    headerFiles.emplace(HeaderFileType::OTHER_MODULES_HEADER_FILE, "message_parcel");
    headerFiles.emplace(HeaderFileType::OTHER_MODULES_HEADER_FILE, "hdi_support");
    headerFiles.emplace(HeaderFileType::OTHER_MODULES_HEADER_FILE, "string_ex");

    const AST::TypeStringMap &types = ast_->GetTypes();
    for (const auto &pair : types) {
        AutoPtr<ASTType> type = pair.second;
        if (type->GetTypeKind() == TypeKind::TYPE_UNION) {
            headerFiles.emplace(HeaderFileType::OTHER_MODULES_HEADER_FILE, "securec");
            break;
        }
    }

    for (const auto &method : interface_->GetMethodsBySystem(Options::GetInstance().GetSystemLevel())) {
        for (size_t paramIndex = 0; paramIndex < method->GetParameterNumber(); paramIndex++) {
            AutoPtr<ASTParameter> param = method->GetParameter(paramIndex);
            AutoPtr<ASTType> paramType = param->GetType();
            if (param->GetAttribute() == ParamAttr::PARAM_IN &&
                (paramType->IsInterfaceType() || paramType->HasInnerType(TypeKind::TYPE_INTERFACE))) {
                headerFiles.emplace(HeaderFileType::OTHER_MODULES_HEADER_FILE, "object_collector");
            }

            if (param->GetAttribute() == ParamAttr::PARAM_OUT &&
                (paramType->IsInterfaceType() || paramType->HasInnerType(TypeKind::TYPE_INTERFACE))) {
                headerFiles.emplace(HeaderFileType::OTHER_MODULES_HEADER_FILE, "iproxy_broker");
            }
        }
    }
}

void CppClientProxyCodeEmitter::GetSourceOtherFileInclusions(HeaderFile::HeaderFileSet &headerFiles) const
{
    for (const auto &method : interface_->GetMethodsBySystem(Options::GetInstance().GetSystemLevel())) {
        for (size_t paramIndex = 0; paramIndex < method->GetParameterNumber(); paramIndex++) {
            AutoPtr<ASTParameter> param = method->GetParameter(paramIndex);
            AutoPtr<ASTType> paramType = param->GetType();
            if (param->GetAttribute() == ParamAttr::PARAM_OUT &&
                (param->GetType()->IsInterfaceType() || paramType->HasInnerType(TypeKind::TYPE_INTERFACE))) {
                AutoPtr<ASTInterfaceType> type = dynamic_cast<ASTInterfaceType *>(paramType.Get());
                std::string FileName = InterfaceToFilePath(paramType->ToString());
                headerFiles.emplace(HeaderFileType::OWN_MODULE_HEADER_FILE, FileName);
            }
        }
    }
}

void CppClientProxyCodeEmitter::EmitGetMethodImpl(StringBuilder &sb, const std::string &prefix) const
{
    sb.Append(prefix).AppendFormat("%s %s::Get(bool isStub)\n", interface_->EmitCppType().c_str(),
        EmitDefinitionByInterface(interface_, interfaceName_).c_str());
    sb.Append(prefix).Append("{\n");
    sb.Append(prefix + TAB)
        .AppendFormat("return %s::Get(\"%s\", isStub);\n", interfaceName_.c_str(), FileName(implName_).c_str());
    sb.Append(prefix).Append("}\n");
}

void CppClientProxyCodeEmitter::EmitGetInstanceMethodImpl(StringBuilder &sb, const std::string &prefix)
{
    std::string objName = "proxy";
    std::string serMajorName = "serMajorVer";
    std::string serMinorName = "serMinorVer";
    sb.Append(prefix).AppendFormat("sptr<%s> %s::Get(const std::string& serviceName, bool isStub)\n",
        EmitDefinitionByInterface(interface_, interfaceName_).c_str(),
        EmitDefinitionByInterface(interface_, interfaceName_).c_str());
    sb.Append(prefix).Append("{\n");
    EmitProxyPassthroughtLoadImpl(sb, prefix + TAB);
    sb.Append(prefix + TAB).Append("using namespace OHOS::HDI::ServiceManager::V1_0;\n");
    sb.Append(prefix + TAB).Append("auto servMgr = IServiceManager::Get();\n");
    sb.Append(prefix + TAB).Append("if (servMgr == nullptr) {\n");
    sb.Append(prefix + TAB + TAB).Append("HDF_LOGE(\"%{public}s:get IServiceManager failed!\", __func__);\n");
    sb.Append(prefix + TAB + TAB).Append("return nullptr;\n");
    sb.Append(prefix + TAB).Append("}\n\n");
    sb.Append(prefix + TAB).Append("sptr<IRemoteObject> remote = ");
    sb.Append("servMgr->GetService(serviceName.c_str());\n");
    sb.Append(prefix + TAB).Append("if (remote == nullptr) {\n");
    sb.Append(prefix + TAB + TAB).Append("HDF_LOGE(\"%{public}s:get remote object failed!\", __func__);\n");
    sb.Append(prefix + TAB + TAB).Append("return nullptr;\n");
    sb.Append(prefix + TAB).Append("}\n\n");
    sb.Append(prefix + TAB).AppendFormat("sptr<%s> %s = new %s(remote);\n",
        EmitDefinitionByInterface(interface_, interfaceName_).c_str(), objName.c_str(),
        ((StringHelper::StartWith(interfaceName_, "I") ? interfaceName_.substr(1) : interfaceName_) +
        "Proxy").c_str());
    sb.Append(prefix + TAB).AppendFormat("if (%s == nullptr) {\n", objName.c_str());
    sb.Append(prefix + TAB + TAB).Append("HDF_LOGE(\"%{public}s:iface_cast failed!\", __func__);\n");
    sb.Append(prefix + TAB + TAB).Append("return nullptr;\n");
    sb.Append(prefix + TAB).Append("}\n\n");

    sb.Append(prefix + TAB).AppendFormat("uint32_t %s = 0;\n", serMajorName.c_str());
    sb.Append(prefix + TAB).AppendFormat("uint32_t %s = 0;\n", serMinorName.c_str());
    sb.Append(prefix + TAB).AppendFormat("int32_t %s = %s->GetVersion(%s, %s);\n",
        errorCodeName_.c_str(), objName.c_str(), serMajorName.c_str(), serMinorName.c_str());
    sb.Append(prefix + TAB).AppendFormat("if (%s != HDF_SUCCESS) {\n", errorCodeName_.c_str());
    sb.Append(prefix + TAB + TAB).Append("HDF_LOGE(\"%{public}s:get version failed!\", __func__);\n");
    sb.Append(prefix + TAB + TAB).Append("return nullptr;\n");
    sb.Append(prefix + TAB).Append("}\n\n");

    sb.Append(prefix + TAB).AppendFormat("if (%s != %d) {\n", serMajorName.c_str(), ast_->GetMajorVer());
    sb.Append(prefix + TAB + TAB).Append("HDF_LOGE(\"%{public}s:check version failed! ");
    sb.Append("version of service:%u.%u");
    sb.AppendFormat(", version of client:%d.%d\", __func__, ", ast_->GetMajorVer(), ast_->GetMinorVer());
    sb.AppendFormat("%s, %s);\n", serMajorName.c_str(), serMinorName.c_str());
    sb.Append(prefix + TAB + TAB).Append("return nullptr;\n");
    sb.Append(prefix + TAB).Append("}\n\n");
    sb.Append(prefix + TAB).AppendFormat("return %s;\n", objName.c_str());
    sb.Append(prefix).Append("}\n");
}

void CppClientProxyCodeEmitter::EmitProxyPassthroughtLoadImpl(StringBuilder &sb, const std::string &prefix) const
{
    sb.Append(prefix).AppendFormat("if (isStub) {\n");

    if (Options::GetInstance().GetSystemLevel() == SystemLevel::LITE) {
        sb.Append(prefix + TAB).Append("std::string desc = ");
        sb.Append("std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t>{}.to_bytes(");
        sb.AppendFormat("%s::GetDescriptor());\n", EmitDefinitionByInterface(interface_, interfaceName_).c_str());
    } else {
        sb.Append(prefix + TAB).AppendFormat("std::string desc = Str16ToStr8(%s::GetDescriptor());\n",
            EmitDefinitionByInterface(interface_, interfaceName_).c_str());
    }
    sb.Append(prefix + TAB).Append("void *impl = LoadHdiImpl(desc.c_str(), ");
    sb.AppendFormat("serviceName == \"%s\" ? \"service\" : serviceName.c_str());\n", FileName(implName_).c_str());
    sb.Append(prefix + TAB).Append("if (impl == nullptr) {\n");
    sb.Append(prefix + TAB + TAB).Append("HDF_LOGE(\"failed to load hdi impl %{public}s\", desc.data());\n");
    sb.Append(prefix + TAB + TAB).Append("return nullptr;\n");
    sb.Append(prefix + TAB).Append("}\n");

    if (Options::GetInstance().GetSystemLevel() == SystemLevel::LITE) {
        sb.Append(prefix + TAB).AppendFormat("return std::shared_ptr<%s>(reinterpret_cast<%s *>(impl));\n",
            EmitDefinitionByInterface(interface_, interfaceName_).c_str(),
            EmitDefinitionByInterface(interface_, interfaceName_).c_str());
    } else {
        sb.Append(prefix + TAB).AppendFormat("return reinterpret_cast<%s *>(impl);\n",
            EmitDefinitionByInterface(interface_, interfaceName_).c_str());
    }
    sb.Append(prefix).Append("}\n\n");
}

void CppClientProxyCodeEmitter::EmitProxyMethodImpls(StringBuilder &sb, const std::string &prefix)
{
    AutoPtr<ASTInterfaceType> interface = interface_;
    AutoPtr<ASTInterfaceType> metaInterface = interface_;
    while (interface != nullptr) {
        for (const auto &method : interface->GetMethodsBySystem(Options::GetInstance().GetSystemLevel())) {
            EmitProxyMethodImpl(interface, method, sb, prefix);
            sb.Append("\n");
        }
        interface = interface->GetExtendsInterface();
        if (interface != nullptr) {
            metaInterface = interface;
        }
    }
    AutoPtr<ASTMethod> verMethod = interface_->GetVersionMethod();
    EmitProxyMethodImpl(metaInterface, verMethod, sb, prefix);
    for (const auto &method : interface_->GetMethodsBySystem(Options::GetInstance().GetSystemLevel())) {
        sb.Append("\n");
        EmitProxyStaticMethodImpl(method, sb, prefix);
    }
    if (interface_->GetExtendsInterface() == nullptr) {
        sb.Append("\n");
        EmitProxyStaticMethodImpl(interface_->GetVersionMethod(), sb, prefix);
    }
}

void CppClientProxyCodeEmitter::EmitProxyIsProxyMethodImpl(StringBuilder &sb, const std::string &prefix) const
{
    sb.Append(prefix).Append("inline bool IsProxy() override\n");
    sb.Append(prefix).Append("{\n");
    sb.Append(prefix + TAB).Append("return true;\n");
    sb.Append(prefix).Append("}\n\n");
}

void CppClientProxyCodeEmitter::EmitProxyCastFromMethodImpls(StringBuilder &sb, const std::string &prefix) const
{
    AutoPtr<ASTInterfaceType> interface = interface_->GetExtendsInterface();
    while (interface != nullptr) {
        EmitProxyCastFromMethodImpl(interface, sb, prefix);
        sb.Append(prefix).Append("\n");
        interface = interface->GetExtendsInterface();
    }
}

void CppClientProxyCodeEmitter::EmitProxyCastFromMethodImpl(const AutoPtr<ASTInterfaceType> interface,
    StringBuilder &sb, const std::string &prefix) const
{
    std::string currentInterface = EmitDefinitionByInterface(interface_, interfaceName_);
    std::string parentInterface = EmitDefinitionByInterface(interface, interfaceName_);

    sb.Append(prefix).AppendFormat("sptr<%s> %s::CastFrom(const sptr<%s> &parent)\n",
        currentInterface.c_str(), currentInterface.c_str(), parentInterface.c_str());
    sb.Append(prefix).Append("{\n");
    sb.Append(prefix + TAB).AppendFormat("return CastFromTemplate<%s, %s>(parent);\n",
        currentInterface.c_str(), parentInterface.c_str());
    sb.Append(prefix).Append("}\n");
}

void CppClientProxyCodeEmitter::EmitProxyCastFromMethodImplTemplate(StringBuilder &sb, const std::string &prefix) const
{
    std::string serMajorName = "serMajorVer";
    std::string serMinorName = "serMinorVer";

    sb.Append(prefix).Append("template<typename ChildType, typename ParentType>\n");
    sb.Append(prefix).Append("static sptr<ChildType> CastFromTemplate(const sptr<ParentType> &parent)\n");
    sb.Append(prefix).Append("{\n");
    sb.Append(prefix + TAB).Append("if (parent == nullptr) {\n");
    sb.Append(prefix + TAB + TAB).Append("HDF_LOGE(\"%{public}s:parent is nullptr!\", __func__);\n");
    sb.Append(prefix + TAB + TAB).Append("return nullptr;\n");
    sb.Append(prefix + TAB).Append("}\n\n");

    sb.Append(prefix + TAB).Append("if (!parent->IsProxy()) {\n");
    sb.Append(prefix + TAB + TAB).Append("HDF_LOGE(\"%{public}s:not proxy, not support castfrom!\", __func__);\n");
    sb.Append(prefix + TAB + TAB).Append("return nullptr;\n");
    sb.Append(prefix + TAB).Append("}\n\n");

    sb.Append(prefix + TAB).AppendFormat("sptr<IRemoteObject> remote = OHOS::HDI::hdi_objcast<ParentType>(parent);\n");
    sb.Append(prefix + TAB).Append("if (remote == nullptr) {\n");
    sb.Append(prefix + TAB + TAB).Append("HDF_LOGE(\"%{public}s:hdi_objcast failed!\", __func__);\n");
    sb.Append(prefix + TAB + TAB).Append("return nullptr;\n");
    sb.Append(prefix + TAB).Append("}\n\n");

    sb.Append(prefix + TAB).AppendFormat("sptr<ChildType> proxy = OHOS::HDI::hdi_facecast<ChildType>(remote);\n");
    sb.Append(prefix + TAB).Append("if (proxy == nullptr) {\n");
    sb.Append(prefix + TAB + TAB).Append("HDF_LOGE(\"%{public}s:hdi_facecast failed!\", __func__);\n");
    sb.Append(prefix + TAB + TAB).Append("return nullptr;\n");
    sb.Append(prefix + TAB).Append("}\n\n");

    sb.Append(prefix + TAB).AppendFormat("uint32_t %s = 0;\n", serMajorName.c_str());
    sb.Append(prefix + TAB).AppendFormat("uint32_t %s = 0;\n", serMinorName.c_str());
    sb.Append(prefix + TAB).AppendFormat("int32_t %s = proxy->GetVersion(%s, %s);\n",
        errorCodeName_.c_str(), serMajorName.c_str(), serMinorName.c_str());
    sb.Append(prefix + TAB).AppendFormat("if (%s != HDF_SUCCESS) {\n", errorCodeName_.c_str());
    sb.Append(prefix + TAB + TAB).Append("HDF_LOGE(\"%{public}s:get version failed!\", __func__);\n");
    sb.Append(prefix + TAB + TAB).Append("return nullptr;\n");
    sb.Append(prefix + TAB).Append("}\n\n");

    sb.Append(prefix + TAB).AppendFormat("if (%s != %d) {\n", serMajorName.c_str(), ast_->GetMajorVer());
    sb.Append(prefix + TAB + TAB).Append("HDF_LOGE(\"%{public}s:check version failed! ");
    sb.Append("version of service:%u.%u");
    sb.AppendFormat(", version of client:%d.%d\", __func__, ", ast_->GetMajorVer(), ast_->GetMinorVer());
    sb.AppendFormat("%s, %s);\n", serMajorName.c_str(), serMinorName.c_str());
    sb.Append(prefix + TAB + TAB).Append("return nullptr;\n");
    sb.Append(prefix + TAB).Append("}\n\n");

    sb.Append(prefix + TAB).Append("return proxy;\n");
    sb.Append(prefix).Append("}\n");
}

void CppClientProxyCodeEmitter::EmitProxyMethodImpl(const AutoPtr<ASTInterfaceType> interface,
    const AutoPtr<ASTMethod> &method, StringBuilder &sb, const std::string &prefix)
{
    if (method->GetParameterNumber() == 0) {
        sb.Append(prefix).AppendFormat(
            "int32_t %s::%s()\n", EmitDefinitionByInterface(interface_, proxyName_).c_str(), method->GetName().c_str());
    } else {
        StringBuilder paramStr;
        paramStr.Append(prefix).AppendFormat(
            "int32_t %s::%s(", EmitDefinitionByInterface(interface_, proxyName_).c_str(), method->GetName().c_str());
        for (size_t i = 0; i < method->GetParameterNumber(); i++) {
            AutoPtr<ASTParameter> param = method->GetParameter(i);
            EmitProxyMethodParameter(param, paramStr, "");
            if (i + 1 < method->GetParameterNumber()) {
                paramStr.Append(", ");
            }
        }

        paramStr.Append(")");

        sb.Append(SpecificationParam(paramStr, prefix + TAB));
        sb.Append("\n");
    }
    EmitProxyMethodBody(interface, method, sb, prefix);
}

void CppClientProxyCodeEmitter::EmitProxyStaticMethodImpl(
    const AutoPtr<ASTMethod> &method, StringBuilder &sb, const std::string &prefix)
{
    if (method->GetParameterNumber() == 0) {
        sb.Append(prefix).AppendFormat("int32_t %s::%s_(const sptr<IRemoteObject> remote)\n",
            EmitDefinitionByInterface(interface_, proxyName_).c_str(), method->GetName().c_str());
    } else {
        StringBuilder paramStr;
        paramStr.Append(prefix).AppendFormat(
            "int32_t %s::%s_(", EmitDefinitionByInterface(interface_, proxyName_).c_str(), method->GetName().c_str());
        for (size_t i = 0; i < method->GetParameterNumber(); i++) {
            AutoPtr<ASTParameter> param = method->GetParameter(i);
            EmitProxyMethodParameter(param, paramStr, "");
            paramStr.Append(", ");
        }

        paramStr.Append("const sptr<IRemoteObject> remote)");
        sb.Append(SpecificationParam(paramStr, prefix + TAB));
        sb.Append("\n");
    }
    EmitProxyStaticMethodBody(method, sb, prefix);
}

void CppClientProxyCodeEmitter::EmitProxyMethodBody(const AutoPtr<ASTInterfaceType> interface,
    const AutoPtr<ASTMethod> &method, StringBuilder &sb, const std::string &prefix)
{
    sb.Append(prefix).Append("{\n");
    sb.Append(prefix + TAB).AppendFormat("return %s::%s_(",
        EmitDefinitionByInterface(interface, proxyName_).c_str(), method->GetName().c_str());
    if (method->GetParameterNumber() > 0) {
        for (size_t i = 0; i < method->GetParameterNumber(); i++) {
            AutoPtr<ASTParameter> param = method->GetParameter(i);
            sb.Append(param->GetName().c_str());
            sb.Append(", ");
        }
    }
    sb.Append("Remote());\n");
    sb.Append(prefix).Append("}\n");
}

void CppClientProxyCodeEmitter::EmitProxyStaticMethodBody(
    const AutoPtr<ASTMethod> &method, StringBuilder &sb, const std::string &prefix)
{
    std::string option = method->IsOneWay() ? "MessageOption::TF_ASYNC" : "MessageOption::TF_SYNC";
    sb.Append(prefix).Append("{\n");
    sb.Append(prefix + TAB).AppendFormat("MessageParcel %s;\n", dataParcelName_.c_str());
    sb.Append(prefix + TAB).AppendFormat("MessageParcel %s;\n", replyParcelName_.c_str());
    sb.Append(prefix + TAB).AppendFormat("MessageOption %s(%s);\n", optionName_.c_str(), option.c_str());
    sb.Append("\n");

    // write interface token
    EmitWriteInterfaceToken(dataParcelName_, sb, prefix + TAB);
    sb.Append("\n");

    EmitWriteFlagOfNeedSetMem(method, dataParcelName_, sb, prefix + TAB);

    if (method->GetParameterNumber() > 0) {
        for (size_t i = 0; i < method->GetParameterNumber(); i++) {
            AutoPtr<ASTParameter> param = method->GetParameter(i);
            if (param->GetAttribute() == ParamAttr::PARAM_IN) {
                EmitWriteMethodParameter(param, dataParcelName_, sb, prefix + TAB);
                sb.Append("\n");
            }
        }
    }
    sb.Append(prefix + TAB).AppendFormat("if (remote == nullptr) {\n");
    sb.Append(prefix + TAB + TAB).Append("HDF_LOGE(\"%{public}s: invalid remote object!\", __func__);\n");
    sb.Append(prefix + TAB + TAB).Append("return HDF_ERR_INVALID_OBJECT;\n");
    sb.Append(prefix + TAB).Append("}\n\n");
    sb.Append(prefix + TAB).AppendFormat("int32_t %s = remote->SendRequest(%s, %s, %s, %s);\n", errorCodeName_.c_str(),
        EmitMethodCmdID(method).c_str(), dataParcelName_.c_str(), replyParcelName_.c_str(), optionName_.c_str());
    sb.Append(prefix + TAB).AppendFormat("if (%s != HDF_SUCCESS) {\n", errorCodeName_.c_str());
    sb.Append(prefix + TAB + TAB).AppendFormat(
        "HDF_LOGE(\"%%{public}s failed, error code is %%{public}d\", __func__, %s);\n", errorCodeName_.c_str());
    sb.Append(prefix + TAB + TAB).AppendFormat("return %s;\n", errorCodeName_.c_str());
    sb.Append(prefix + TAB).Append("}\n");

    if (!method->IsOneWay()) {
        sb.Append("\n");
        for (size_t i = 0; i < method->GetParameterNumber(); i++) {
            AutoPtr<ASTParameter> param = method->GetParameter(i);
            if (param->GetAttribute() == ParamAttr::PARAM_OUT) {
                EmitReadMethodParameter(param, replyParcelName_, false, sb, prefix + TAB);
                sb.Append("\n");
            }
        }
    }

    sb.Append(prefix + TAB).AppendFormat("return %s;\n", errorCodeName_.c_str());
    sb.Append(prefix).Append("}\n");
}

void CppClientProxyCodeEmitter::EmitWriteInterfaceToken(
    const std::string &parcelName, StringBuilder &sb, const std::string &prefix) const
{
    sb.Append(prefix).AppendFormat("if (!%s.WriteInterfaceToken(%s::GetDescriptor())) {\n", parcelName.c_str(),
        EmitDefinitionByInterface(interface_, interfaceName_).c_str());
    sb.Append(prefix + TAB)
        .AppendFormat("HDF_LOGE(\"%%{public}s: failed to write interface descriptor!\", __func__);\n");
    sb.Append(prefix + TAB).AppendFormat("return HDF_ERR_INVALID_PARAM;\n");
    sb.Append(prefix).Append("}\n");
}

void CppClientProxyCodeEmitter::EmitWriteFlagOfNeedSetMem(const AutoPtr<ASTMethod> &method,
    const std::string &dataBufName, StringBuilder &sb, const std::string &prefix) const
{
    if (NeedFlag(method)) {
        sb.Append(prefix).AppendFormat("if (!%s.WriteBool(false)) {\n", dataBufName.c_str());
        sb.Append(prefix + TAB).Append("HDF_LOGE(\"%{public}s:failed to write flag of memory setting!\", __func__);\n");
        sb.Append(prefix + TAB).AppendFormat("return HDF_ERR_INVALID_PARAM;\n");
        sb.Append(prefix).Append("}\n\n");
    }
}

void CppClientProxyCodeEmitter::GetUtilMethods(UtilMethodMap &methods)
{
    for (const auto &method : interface_->GetMethodsBySystem(Options::GetInstance().GetSystemLevel())) {
        for (size_t paramIndex = 0; paramIndex < method->GetParameterNumber(); paramIndex++) {
            AutoPtr<ASTParameter> param = method->GetParameter(paramIndex);
            AutoPtr<ASTType> paramType = param->GetType();
            if (param->GetAttribute() == ParamAttr::PARAM_IN) {
                paramType->RegisterWriteMethod(Options::GetInstance().GetLanguage(), SerMode::PROXY_SER, methods);
            } else {
                paramType->RegisterReadMethod(Options::GetInstance().GetLanguage(), SerMode::PROXY_SER, methods);
            }
        }
    }
}
} // namespace HDI
} // namespace OHOS