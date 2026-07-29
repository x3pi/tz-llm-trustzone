/*
 * Copyright (c) 2021-2023 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include "codegen/cpp_code_emitter.h"

#include <regex>
#include <unordered_set>

#include "util/options.h"

#include "util/logger.h"

namespace OHOS {
namespace HDI {
void CppCodeEmitter::GetStdlibInclusions(HeaderFile::HeaderFileSet &headerFiles)
{
    const AST::TypeStringMap &types = ast_->GetTypes();
    for (const auto &pair : types) {
        AutoPtr<ASTType> type = pair.second;
        switch (type->GetTypeKind()) {
            case TypeKind::TYPE_STRING: {
                headerFiles.emplace(HeaderFileType::CPP_STD_HEADER_FILE, "string");
                break;
            }
            case TypeKind::TYPE_ARRAY:
            case TypeKind::TYPE_LIST: {
                headerFiles.emplace(HeaderFileType::CPP_STD_HEADER_FILE, "vector");
                break;
            }
            case TypeKind::TYPE_MAP: {
                headerFiles.emplace(HeaderFileType::CPP_STD_HEADER_FILE, "map");
                break;
            }
            case TypeKind::TYPE_SMQ: {
                headerFiles.emplace(HeaderFileType::OTHER_MODULES_HEADER_FILE, "base/hdi_smq");
                break;
            }
            case TypeKind::TYPE_ASHMEM: {
                headerFiles.emplace(HeaderFileType::OTHER_MODULES_HEADER_FILE, "ashmem");
                break;
            }
            case TypeKind::TYPE_NATIVE_BUFFER: {
                headerFiles.emplace(HeaderFileType::OTHER_MODULES_HEADER_FILE, "base/native_buffer");
                break;
            }
            default:
                break;
        }
    }
}

void CppCodeEmitter::GetImportInclusions(HeaderFile::HeaderFileSet &headerFiles)
{
    for (const auto &importPair : ast_->GetImports()) {
        AutoPtr<AST> importAst = importPair.second;
        std::string fileName = (importAst->GetASTFileType() == ASTFileType::AST_SEQUENCEABLE) ?
            PackageToFilePath(importAst->GetName()) :
            PackageToFilePath(importAst->GetFullName());
        headerFiles.emplace(HeaderFileType::OWN_MODULE_HEADER_FILE, fileName);
    }
}

void CppCodeEmitter::EmitInterfaceMethodParameter(
    const AutoPtr<ASTParameter> &param, StringBuilder &sb, const std::string &prefix) const
{
    sb.Append(prefix).Append(param->EmitCppParameter());
}

void CppCodeEmitter::EmitLicense(StringBuilder &sb)
{
    if (ast_->GetLicense().empty()) {
        return;
    }
    sb.Append(ast_->GetLicense()).Append("\n\n");
}

void CppCodeEmitter::EmitHeadMacro(StringBuilder &sb, const std::string &fullName) const
{
    std::string macroName = MacroName(fullName);
    sb.Append("#ifndef ").Append(macroName).Append("\n");
    sb.Append("#define ").Append(macroName).Append("\n");
}

void CppCodeEmitter::EmitTailMacro(StringBuilder &sb, const std::string &fullName) const
{
    std::string macroName = MacroName(fullName);
    sb.Append("#endif // ").Append(macroName);
}

void CppCodeEmitter::EmitHeadExternC(StringBuilder &sb) const
{
    sb.Append("#ifdef __cplusplus\n");
    sb.Append("extern \"C\" {\n");
    sb.Append("#endif /* __cplusplus */\n");
}

void CppCodeEmitter::EmitTailExternC(StringBuilder &sb) const
{
    sb.Append("#ifdef __cplusplus\n");
    sb.Append("}\n");
    sb.Append("#endif /* __cplusplus */\n");
}

bool CppCodeEmitter::IsVersion(const std::string &name) const
{
    std::regex rVer("[V|v][0-9]+_[0-9]+");
    return std::regex_match(name.c_str(), rVer);
}

std::vector<std::string> CppCodeEmitter::EmitCppNameSpaceVec(const std::string &namespaceStr) const
{
    std::vector<std::string> result;
    std::vector<std::string> namespaceVec = StringHelper::Split(namespaceStr, ".");
    bool findVersion = false;

    std::string rootPackage = Options::GetInstance().GetRootPackage(namespaceStr);
    size_t rootPackageNum = StringHelper::Split(rootPackage, ".").size();

    for (size_t i = 0; i < namespaceVec.size(); i++) {
        std::string name;
        if (i < rootPackageNum) {
            name = StringHelper::StrToUpper(namespaceVec[i]);
        } else if (!findVersion && IsVersion(namespaceVec[i])) {
            name = StringHelper::Replace(namespaceVec[i], 'v', 'V');
            findVersion = true;
        } else {
            if (findVersion) {
                name = namespaceVec[i];
            } else {
                name = PascalName(namespaceVec[i]);
            }
        }

        result.emplace_back(name);
    }
    return result;
}

std::string CppCodeEmitter::EmitPackageToNameSpace(const std::string &packageName) const
{
    if (packageName.empty()) {
        return packageName;
    }

    StringBuilder nameSpaceStr;
    std::vector<std::string> namespaceVec = EmitCppNameSpaceVec(packageName);
    for (auto nameIter = namespaceVec.begin(); nameIter != namespaceVec.end(); nameIter++) {
        nameSpaceStr.Append(*nameIter);
        if (nameIter != namespaceVec.end() - 1) {
            nameSpaceStr.Append("::");
        }
    }

    return nameSpaceStr.ToString();
}

void CppCodeEmitter::EmitBeginNamespace(StringBuilder &sb)
{
    std::vector<std::string> cppNamespaceVec = EmitCppNameSpaceVec(interface_->GetNamespace()->ToString());
    for (const auto &nspace : cppNamespaceVec) {
        sb.AppendFormat("namespace %s {\n", nspace.c_str());
    }
}

void CppCodeEmitter::EmitEndNamespace(StringBuilder &sb)
{
    std::vector<std::string> cppNamespaceVec = EmitCppNameSpaceVec(interface_->GetNamespace()->ToString());

    for (std::vector<std::string>::const_reverse_iterator nspaceIter = cppNamespaceVec.rbegin();
        nspaceIter != cppNamespaceVec.rend(); ++nspaceIter) {
        sb.AppendFormat("} // %s\n", nspaceIter->c_str());
    }
}

void CppCodeEmitter::EmitUsingNamespace(StringBuilder &sb)
{
    sb.Append("using namespace OHOS;\n");
    sb.Append("using namespace OHOS::HDI;\n");
    EmitImportUsingNamespace(sb);
}

std::string CppCodeEmitter::EmitNamespace(const std::string &packageName) const
{
    if (packageName.empty()) {
        return packageName;
    }

    size_t index = packageName.rfind('.');
    return index != std::string::npos ? StringHelper::SubStr(packageName, 0, index) : packageName;
}

void CppCodeEmitter::EmitImportUsingNamespace(StringBuilder &sb)
{
    using StringSet = std::unordered_set<std::string>;
    StringSet namespaceSet;
    std::string selfNameSpace = EmitPackageToNameSpace(EmitNamespace(ast_->GetFullName()));

    for (const auto &importPair : ast_->GetImports()) {
        AutoPtr<AST> import = importPair.second;
        std::string nameSpace = EmitPackageToNameSpace(EmitNamespace(import->GetFullName()));
        if (nameSpace == selfNameSpace) {
            continue;
        }
        namespaceSet.emplace(nameSpace);
    }

    const AST::TypeStringMap &types = ast_->GetTypes();
    for (const auto &pair : types) {
        AutoPtr<ASTType> type = pair.second;
        if (type->GetTypeKind() == TypeKind::TYPE_SMQ ||
            type->GetTypeKind() == TypeKind::TYPE_NATIVE_BUFFER) {
            namespaceSet.emplace("OHOS::HDI::Base");
            break;
        }
    }

    for (const auto &nspace : namespaceSet) {
        sb.Append("using namespace ").AppendFormat("%s;\n", nspace.c_str());
    }
}

void CppCodeEmitter::EmitWriteMethodParameter(const AutoPtr<ASTParameter> &param,
    const std::string &parcelName, StringBuilder &sb, const std::string &prefix) const
{
    AutoPtr<ASTType> type = param->GetType();
    type->EmitCppWriteVar(parcelName, param->GetName(), sb, prefix);
}

void CppCodeEmitter::EmitReadMethodParameter(const AutoPtr<ASTParameter> &param, const std::string &parcelName,
    bool initVariable, StringBuilder &sb, const std::string &prefix) const
{
    AutoPtr<ASTType> type = param->GetType();
    type->EmitCppReadVar(parcelName, param->GetName(), sb, prefix, initVariable);
}

std::string CppCodeEmitter::MacroName(const std::string &name) const
{
    if (name.empty()) {
        return name;
    }

    std::string macro = StringHelper::StrToUpper(StringHelper::Replace(name, '.', '_')) + "_H";
    return macro;
}

std::string CppCodeEmitter::SpecificationParam(StringBuilder &paramSb, const std::string &prefix) const
{
    size_t maxLineLen = 120;
    size_t replaceLen = 2;
    std::string paramStr = paramSb.ToString();
    size_t preIndex = 0;
    size_t curIndex = 0;

    std::string insertStr = StringHelper::Format("\n%s", prefix.c_str());
    for (; curIndex < paramStr.size(); curIndex++) {
        if (curIndex == maxLineLen && preIndex > 0) {
            StringHelper::Replace(paramStr, preIndex, replaceLen, ",");
            paramStr.insert(preIndex + 1, insertStr);
        } else {
            if (paramStr[curIndex] == ',') {
                preIndex = curIndex;
            }
        }
    }
    return paramStr;
}

std::string CppCodeEmitter::EmitHeaderNameByInterface(AutoPtr<ASTInterfaceType> interface, const std::string &name)
{
    return StringHelper::Format(
        "v%u_%u/%s", interface->GetMajorVersion(), interface->GetMinorVersion(), FileName(name).c_str());
}

std::string CppCodeEmitter::EmitDefinitionByInterface(
    AutoPtr<ASTInterfaceType> interface, const std::string &name) const
{
    StringBuilder sb;
    std::vector<std::string> cppNamespaceVec = EmitCppNameSpaceVec(interface->GetNamespace()->ToString());
    for (const auto &nspace : cppNamespaceVec) {
        sb.AppendFormat("%s", nspace.c_str());
        sb.Append("::");
    }
    sb.Append(name.c_str());
    return sb.ToString();
}
} // namespace HDI
} // namespace OHOS