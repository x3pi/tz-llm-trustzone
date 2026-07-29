/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include "codegen/cpp_interface_code_emitter.h"
#include "util/file.h"
#include "util/logger.h"

namespace OHOS {
namespace HDI {
bool CppInterfaceCodeEmitter::ResolveDirectory(const std::string &targetDirectory)
{
    if (ast_->GetASTFileType() == ASTFileType::AST_IFACE || ast_->GetASTFileType() == ASTFileType::AST_ICALLBACK) {
        directory_ = GetFileParentPath(targetDirectory);
    } else {
        return false;
    }

    if (!File::CreateParentDir(directory_)) {
        Logger::E("CppInterfaceCodeEmitter", "Create '%s' failed!", directory_.c_str());
        return false;
    }

    return true;
}

void CppInterfaceCodeEmitter::EmitCode()
{
    switch (mode_) {
        case GenMode::PASSTHROUGH:
        case GenMode::IPC: {
            EmitInterfaceHeaderFile();
            break;
        }
        default:
            break;
    }
}

void CppInterfaceCodeEmitter::EmitInterfaceHeaderFile()
{
    std::string filePath =
        File::AdapterPath(StringHelper::Format("%s/%s.h", directory_.c_str(), FileName(interfaceName_).c_str()));
    File file(filePath, File::WRITE);
    StringBuilder sb;

    EmitLicense(sb);
    EmitHeadMacro(sb, interfaceFullName_);
    sb.Append("\n");
    EmitInterfaceInclusions(sb);
    if (!Options::GetInstance().DoPassthrough()) {
        sb.Append("\n");
        EmitInterfaceBuffSizeMacro(sb);
    }
    sb.Append("\n");
    EmitBeginNamespace(sb);
    EmitUsingNamespace(sb);
    if (!Options::GetInstance().DoPassthrough()) {
        sb.Append("\n");
        if (interface_->GetExtendsInterface() == nullptr) {
            EmitInterfaceMethodCommands(sb, "");
        } else {
            EmitInterfaceMethodCommandsWithExtends(sb, "");
        }
    }
    sb.Append("\n");
    EmitInterfaceDefinition(sb);
    EmitEndNamespace(sb);
    sb.Append("\n");
    EmitTailMacro(sb, interfaceFullName_);

    std::string data = sb.ToString();
    file.WriteData(data.c_str(), data.size());
    file.Flush();
    file.Close();
}

void CppInterfaceCodeEmitter::EmitInterfaceInclusions(StringBuilder &sb)
{
    HeaderFile::HeaderFileSet headerFiles;

    GetStdlibInclusions(headerFiles);
    GetImportInclusions(headerFiles);
    GetHeaderOtherLibInclusions(headerFiles);

    for (const auto &file : headerFiles) {
        sb.AppendFormat("%s\n", file.ToString().c_str());
    }
}

void CppInterfaceCodeEmitter::GetHeaderOtherLibInclusions(HeaderFile::HeaderFileSet &headerFiles) const
{
    headerFiles.emplace(HeaderFileType::OTHER_MODULES_HEADER_FILE, "hdf_base");
    headerFiles.emplace(HeaderFileType::C_STD_HEADER_FILE, "stdint");
    headerFiles.emplace(HeaderFileType::OTHER_MODULES_HEADER_FILE, "hdi_base");
    headerFiles.emplace(HeaderFileType::OTHER_MODULES_HEADER_FILE, "hdf_base");
}

void CppInterfaceCodeEmitter::EmitInterfaceVersionMacro(StringBuilder &sb) const
{
    sb.AppendFormat("#define %s %u\n", majorVerName_.c_str(), ast_->GetMajorVer());
    sb.AppendFormat("#define %s %u\n", minorVerName_.c_str(), ast_->GetMinorVer());
}

void CppInterfaceCodeEmitter::EmitInterfaceDefinition(StringBuilder &sb)
{
    AutoPtr<ASTInterfaceType> interface = interface_->GetExtendsInterface();
    if (interface != nullptr) {
        sb.AppendFormat("class %s : public %s {\n", interfaceName_.c_str(),
            EmitDefinitionByInterface(interface, interfaceName_).c_str());
    } else {
        sb.AppendFormat("class %s : public HdiBase {\n", interfaceName_.c_str());
    }
    sb.Append("public:\n");
    EmitInterfaceDescriptor(sb, TAB);
    sb.Append("\n");
    EmitInterfaceDestruction(sb, TAB);
    sb.Append("\n");
    if (!interface_->IsSerializable()) {
        EmitGetMethodDecl(sb, TAB);
        sb.Append("\n");
    }
    if (interface_->GetExtendsInterface() != nullptr) {
        EmitCastFromDecl(sb, TAB);
        sb.Append("\n");
    }
    EmitInterfaceMethodsDecl(sb, TAB);
    sb.Append("\n");
    EmitGetDescMethod(sb, TAB);
    sb.Append("};\n");
}

void CppInterfaceCodeEmitter::EmitGetDescMethod(StringBuilder &sb, const std::string &prefix) const
{
    AutoPtr<ASTInterfaceType> interface = interface_->GetExtendsInterface();
    if (interface == nullptr) {
        sb.Append(prefix).Append("virtual const std::u16string GetDesc()");
    } else {
        sb.Append(prefix).Append("const std::u16string GetDesc() override");
    }
    sb.Append("\n");
    sb.Append(prefix).Append("{").Append("\n");
    sb.Append(prefix + TAB).Append("return metaDescriptor_;\n");
    sb.Append(prefix).Append("}\n");
}

void CppInterfaceCodeEmitter::EmitInterfaceDescriptor(StringBuilder &sb, const std::string &prefix) const
{
    (void)prefix;
    sb.Append(TAB).AppendFormat("DECLARE_HDI_DESCRIPTOR(u\"%s\");\n", interfaceFullName_.c_str());
}

void CppInterfaceCodeEmitter::EmitCastFromDecl(StringBuilder &sb, const std::string &prefix) const
{
    std::string currentInterface = EmitDefinitionByInterface(interface_, interfaceName_);

    AutoPtr<ASTInterfaceType> interface = interface_->GetExtendsInterface();
    while (interface != nullptr) {
        std::string parentInterface = EmitDefinitionByInterface(interface, interfaceName_);
        sb.Append(prefix).AppendFormat("static sptr<%s> CastFrom(const sptr<%s> &parent);\n",
            currentInterface.c_str(), parentInterface.c_str());
        interface = interface->GetExtendsInterface();
    }
}

void CppInterfaceCodeEmitter::EmitGetMethodDecl(StringBuilder &sb, const std::string &prefix) const
{
    sb.Append(prefix).AppendFormat("static %s Get(bool isStub = false);\n", interface_->EmitCppType().c_str());
    sb.Append(prefix).AppendFormat(
        "static %s Get(const std::string &serviceName, bool isStub = false);\n", interface_->EmitCppType().c_str());
}

void CppInterfaceCodeEmitter::EmitInterfaceDestruction(StringBuilder &sb, const std::string &prefix) const
{
    sb.Append(prefix).AppendFormat("virtual ~%s() = default;\n", interface_->GetName().c_str());
}

void CppInterfaceCodeEmitter::EmitInterfaceMethodsDecl(StringBuilder &sb, const std::string &prefix)
{
    for (const auto &method : interface_->GetMethodsBySystem(Options::GetInstance().GetSystemLevel())) {
        EmitInterfaceMethodDecl(method, sb, prefix);
        sb.Append("\n");
    }

    EmitInterfaceGetVersionMethod(sb, prefix);
    if (interface_->GetExtendsInterface() == nullptr) {
        sb.Append("\n");
        EmitInterfaceIsProxyMethod(sb, prefix);
    }
}

void CppInterfaceCodeEmitter::EmitInterfaceMethodDecl(
    const AutoPtr<ASTMethod> &method, StringBuilder &sb, const std::string &prefix) const
{
    if (method->GetParameterNumber() == 0) {
        sb.Append(prefix).AppendFormat("virtual int32_t %s() = 0;\n", method->GetName().c_str());
    } else {
        StringBuilder paramStr;
        paramStr.Append(prefix).AppendFormat("virtual int32_t %s(", method->GetName().c_str());
        for (size_t i = 0; i < method->GetParameterNumber(); i++) {
            AutoPtr<ASTParameter> param = method->GetParameter(i);
            EmitInterfaceMethodParameter(param, paramStr, "");
            if (i + 1 < method->GetParameterNumber()) {
                paramStr.Append(", ");
            }
        }

        paramStr.Append(") = 0;");
        sb.Append(SpecificationParam(paramStr, prefix + TAB));
        sb.Append("\n");
    }
}

void CppInterfaceCodeEmitter::EmitInterfaceGetVersionMethod(StringBuilder &sb, const std::string &prefix) const
{
    AutoPtr<ASTMethod> method = interface_->GetVersionMethod();
    if (interface_->GetExtendsInterface() == nullptr) {
        sb.Append(prefix).AppendFormat("virtual int32_t %s(", method->GetName().c_str());
    } else {
        sb.Append(prefix).AppendFormat("int32_t %s(", method->GetName().c_str());
    }
    for (size_t i = 0; i < method->GetParameterNumber(); i++) {
        AutoPtr<ASTParameter> param = method->GetParameter(i);
        EmitInterfaceMethodParameter(param, sb, "");
        if (i + 1 < method->GetParameterNumber()) {
            sb.Append(", ");
        }
    }
    sb.Append(")");
    if (interface_->GetExtendsInterface() != nullptr) {
        sb.Append(" override");
    }
    sb.Append("\n");
    sb.Append(prefix).Append("{\n");

    AutoPtr<ASTParameter> majorParam = method->GetParameter(0);
    sb.Append(prefix + TAB).AppendFormat("%s = %d;\n", majorParam->GetName().c_str(), ast_->GetMajorVer());
    AutoPtr<ASTParameter> minorParam = method->GetParameter(1);
    sb.Append(prefix + TAB).AppendFormat("%s = %d;\n", minorParam->GetName().c_str(), ast_->GetMinorVer());

    sb.Append(prefix + TAB).Append("return HDF_SUCCESS;\n");
    sb.Append(prefix).Append("}\n");
}

void CppInterfaceCodeEmitter::EmitInterfaceIsProxyMethod(StringBuilder &sb, const std::string &prefix) const
{
    sb.Append(prefix).AppendFormat("virtual bool %s(", "IsProxy");
    sb.Append(")\n");
    sb.Append(prefix).Append("{\n");
    sb.Append(prefix + TAB).Append("return false;\n");
    sb.Append(prefix).Append("}\n");
}

void CppInterfaceCodeEmitter::EmitInterfaceMethodParameter(
    const AutoPtr<ASTParameter> &param, StringBuilder &sb, const std::string &prefix) const
{
    sb.Append(prefix).Append(param->EmitCppParameter());
}

void CppInterfaceCodeEmitter::EmitInterfaceMethodCommandsWithExtends(StringBuilder &sb, const std::string &prefix)
{
    size_t extendMethods = 0;
    AutoPtr<ASTInterfaceType> interface = interface_->GetExtendsInterface();
    while (interface != nullptr) {
        extendMethods += interface->GetMethodNumber();
        interface = interface->GetExtendsInterface();
    }

    sb.Append(prefix).AppendFormat("enum {\n");
    for (size_t i = 0; i < interface_->GetMethodNumber(); i++) {
        AutoPtr<ASTMethod> method = interface_->GetMethod(i);
        sb.Append(prefix + TAB)
            .Append(EmitMethodCmdID(method))
            .AppendFormat(" = %d", extendMethods + i + 1)
            .Append(",\n");
    }
    sb.Append(prefix).Append("};\n");
}
} // namespace HDI
} // namespace OHOS