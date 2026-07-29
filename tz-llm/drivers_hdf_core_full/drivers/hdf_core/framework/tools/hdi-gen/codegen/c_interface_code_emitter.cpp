/*
 * Copyright (c) 2021-2023 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include "codegen/c_interface_code_emitter.h"
#include "util/file.h"
#include "util/logger.h"

namespace OHOS {
namespace HDI {
bool CInterfaceCodeEmitter::ResolveDirectory(const std::string &targetDirectory)
{
    if (ast_->GetASTFileType() == ASTFileType::AST_IFACE || ast_->GetASTFileType() == ASTFileType::AST_ICALLBACK) {
        directory_ = GetFileParentPath(targetDirectory);
    } else {
        return false;
    }

    if (!File::CreateParentDir(directory_)) {
        Logger::E("CInterfaceCodeEmitter", "Create '%s' failed!", directory_.c_str());
        return false;
    }

    return true;
}

void CInterfaceCodeEmitter::EmitCode()
{
    switch (mode_) {
        case GenMode::LOW: {
            EmitLowModeInterfaceHeaderFile();
            break;
        }
        case GenMode::PASSTHROUGH:
        case GenMode::IPC:
        case GenMode::KERNEL: {
            EmitInterfaceHeaderFile();
            break;
        }
        default:
            break;
    }
}

void CInterfaceCodeEmitter::EmitLowModeInterfaceHeaderFile()
{
    std::string filePath =
        File::AdapterPath(StringHelper::Format("%s/%s.h", directory_.c_str(), FileName(interfaceName_).c_str()));
    File file(filePath, File::WRITE);
    StringBuilder sb;

    EmitLicense(sb);
    EmitHeadMacro(sb, interfaceFullName_);
    sb.Append("\n");
    EmitImportInclusions(sb);
    sb.Append("\n");
    EmitHeadExternC(sb);
    sb.Append("\n");
    EmitInterfaceVersionMacro(sb);
    if (!interface_->IsSerializable()) {
        sb.Append("\n");
        EmitPreDeclaration(sb);
    }
    sb.Append("\n");
    EmitInterfaceDefinition(sb);
    if (!interface_->IsSerializable()) {
        sb.Append("\n");
        EmitLowModeExternalMethod(sb);
    }
    sb.Append("\n");
    EmitTailExternC(sb);
    sb.Append("\n");
    EmitTailMacro(sb, interfaceFullName_);

    std::string data = sb.ToString();
    file.WriteData(data.c_str(), data.size());
    file.Flush();
    file.Close();
}

void CInterfaceCodeEmitter::EmitLowModeExternalMethod(StringBuilder &sb) const
{
    sb.AppendFormat(
        "inline struct %s *%sGet(const char *serviceName)\n", interfaceName_.c_str(), interfaceName_.c_str());
    sb.Append("{\n");
    sb.Append(TAB).AppendFormat(
        "return (struct %s *)DevSvcManagerClntGetService(serviceName);\n", interfaceName_.c_str());
    sb.Append("}\n");
}

void CInterfaceCodeEmitter::EmitInterfaceHeaderFile()
{
    std::string filePath =
        File::AdapterPath(StringHelper::Format("%s/%s.h", directory_.c_str(), FileName(interfaceName_).c_str()));
    File file(filePath, File::WRITE);
    StringBuilder sb;

    EmitLicense(sb);
    EmitHeadMacro(sb, interfaceFullName_);
    sb.Append("\n");
    EmitImportInclusions(sb);
    sb.Append("\n");
    EmitHeadExternC(sb);
    if (!Options::GetInstance().DoPassthrough()) {
        sb.Append("\n");
        EmitPreDeclaration(sb);
    }
    sb.Append("\n");
    EmitInterfaceDesc(sb);
    sb.Append("\n");
    EmitInterfaceVersionMacro(sb);
    if (!Options::GetInstance().DoPassthrough()) {
        sb.Append("\n");
        EmitInterfaceBuffSizeMacro(sb);
        sb.Append("\n");
        EmitInterfaceMethodCommands(sb, "");
    }
    sb.Append("\n");
    EmitInterfaceDefinition(sb);
    EmitExternalMethod(sb);
    EmitTailExternC(sb);
    sb.Append("\n");
    EmitTailMacro(sb, interfaceFullName_);

    std::string data = sb.ToString();
    file.WriteData(data.c_str(), data.size());
    file.Flush();
    file.Close();
}

void CInterfaceCodeEmitter::EmitImportInclusions(StringBuilder &sb)
{
    HeaderFile::HeaderFileSet headerFiles;

    GetStdlibInclusions(headerFiles);
    GetImportInclusions(headerFiles);
    GetHeaderOtherLibInclusions(headerFiles);

    for (const auto &file : headerFiles) {
        sb.AppendFormat("%s\n", file.ToString().c_str());
    }
}

void CInterfaceCodeEmitter::GetHeaderOtherLibInclusions(HeaderFile::HeaderFileSet &headerFiles) const
{
    headerFiles.emplace(HeaderFileType::OTHER_MODULES_HEADER_FILE, "hdf_base");
    if (!Options::GetInstance().DoGenerateKernelCode()) {
        headerFiles.emplace(HeaderFileType::C_STD_HEADER_FILE, "stdint");
        headerFiles.emplace(HeaderFileType::C_STD_HEADER_FILE, "stdbool");
    }
}

void CInterfaceCodeEmitter::EmitPreDeclaration(StringBuilder &sb) const
{
    sb.Append("struct HdfRemoteService;\n");
}

void CInterfaceCodeEmitter::EmitInterfaceDesc(StringBuilder &sb) const
{
    sb.AppendFormat("#define %s \"%s\"\n", interface_->EmitDescMacroName().c_str(), interfaceFullName_.c_str());
}

void CInterfaceCodeEmitter::EmitInterfaceVersionMacro(StringBuilder &sb) const
{
    sb.AppendFormat("#define %s %u\n", majorVerName_.c_str(), ast_->GetMajorVer());
    sb.AppendFormat("#define %s %u\n", minorVerName_.c_str(), ast_->GetMinorVer());
}

void CInterfaceCodeEmitter::EmitInterfaceDefinition(StringBuilder &sb)
{
    sb.AppendFormat("struct %s {\n", interfaceName_.c_str());
    if (mode_ == GenMode::LOW && !interface_->IsSerializable()) {
        sb.Append(TAB).Append("struct HdfRemoteService *service;\n\n");
    }
    EmitInterfaceMethods(sb, TAB);
    sb.Append("};\n");
}

void CInterfaceCodeEmitter::EmitInterfaceMethods(StringBuilder &sb, const std::string &prefix) const
{
    for (const auto &method : interface_->GetMethodsBySystem(Options::GetInstance().GetSystemLevel())) {
        EmitInterfaceMethod(method, sb, prefix);
        sb.Append("\n");
    }

    EmitInterfaceMethod(interface_->GetVersionMethod(), sb, prefix);
    if (mode_ == GenMode::IPC) {
        sb.Append("\n");
        EmitAsObjectMethod(sb, TAB);
    }
}

void CInterfaceCodeEmitter::EmitInterfaceMethod(
    const AutoPtr<ASTMethod> &method, StringBuilder &sb, const std::string &prefix) const
{
    if (method->GetParameterNumber() == 0) {
        sb.Append(prefix).AppendFormat(
            "int32_t (*%s)(struct %s *self);\n", method->GetName().c_str(), interfaceName_.c_str());
    } else {
        StringBuilder paramStr;
        paramStr.Append(prefix).AppendFormat(
            "int32_t (*%s)(struct %s *self, ", method->GetName().c_str(), interfaceName_.c_str());
        for (size_t i = 0; i < method->GetParameterNumber(); i++) {
            AutoPtr<ASTParameter> param = method->GetParameter(i);
            EmitInterfaceMethodParameter(param, paramStr, "");
            if (i + 1 < method->GetParameterNumber()) {
                paramStr.Append(", ");
            }
        }

        paramStr.Append(");");
        sb.Append(SpecificationParam(paramStr, prefix + TAB));
        sb.Append("\n");
    }
}

void CInterfaceCodeEmitter::EmitAsObjectMethod(StringBuilder &sb, const std::string &prefix) const
{
    sb.Append(prefix).AppendFormat("struct HdfRemoteService* (*AsObject)(struct %s *self);\n", interfaceName_.c_str());
}

void CInterfaceCodeEmitter::EmitExternalMethod(StringBuilder &sb) const
{
    if (Options::GetInstance().DoPassthrough() && interface_->IsSerializable()) {
        return;
    }

    sb.Append("\n");
    EmitInterfaceGetMethodDecl(sb);
    sb.Append("\n");
    EmitInterfaceReleaseMethodDecl(sb);
}

void CInterfaceCodeEmitter::EmitInterfaceGetMethodDecl(StringBuilder &sb) const
{
    if (mode_ == GenMode::KERNEL) {
        sb.AppendFormat("struct %s *%sGet(void);\n", interfaceName_.c_str(), interfaceName_.c_str());
        sb.Append("\n");
        sb.AppendFormat(
            "struct %s *%sGetInstance(const char *instanceName);\n", interfaceName_.c_str(), interfaceName_.c_str());
        return;
    }

    if (interface_->IsSerializable()) {
        sb.Append("// no external method used to create client object, it only support ipc mode\n");
        sb.AppendFormat("struct %s *%sGet(struct HdfRemoteService *remote);\n", interfaceName_.c_str(),
            interfaceName_.c_str());
    } else {
        sb.Append("// external method used to create client object, it support ipc and passthrought mode\n");
        sb.AppendFormat("struct %s *%sGet(bool isStub);\n", interfaceName_.c_str(), interfaceName_.c_str());
        sb.AppendFormat("struct %s *%sGetInstance(const char *serviceName, bool isStub);\n", interfaceName_.c_str(),
            interfaceName_.c_str());
    }
}

void CInterfaceCodeEmitter::EmitInterfaceReleaseMethodDecl(StringBuilder &sb) const
{
    if (mode_ == GenMode::KERNEL) {
        sb.AppendFormat("void %sRelease(struct %s *instance);\n", interfaceName_.c_str(), interfaceName_.c_str());
        return;
    }

    if (interface_->IsCallback()) {
        sb.Append("// external method used to release client object, it support ipc and passthrought mode\n");
        sb.AppendFormat("void %sRelease(struct %s *instance);\n", interfaceName_.c_str(),
            interfaceName_.c_str());
    } else if (interface_->IsSerializable()) {
        sb.Append("// external method used to release client object, it support ipc and passthrought mode\n");
        sb.AppendFormat("void %sRelease(struct %s *instance, bool isStub);\n", interfaceName_.c_str(),
            interfaceName_.c_str());
    } else {
        sb.Append("// external method used to create release object, it support ipc and passthrought mode\n");
        sb.AppendFormat("void %sRelease(struct %s *instance, bool isStub);\n", interfaceName_.c_str(),
            interfaceName_.c_str());
        sb.AppendFormat("void %sReleaseInstance(const char *serviceName, struct %s *instance, bool isStub);\n",
            interfaceName_.c_str(), interfaceName_.c_str());
    }
}
} // namespace HDI
} // namespace OHOS