/*
 * Copyright (c) 2021-2023 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include "ast/ast_interface_type.h"
#include "util/string_builder.h"

namespace OHOS {
namespace HDI {
void ASTInterfaceType::SetNamespace(const AutoPtr<ASTNamespace> &nspace)
{
    ASTType::SetNamespace(nspace);
    if (namespace_ != nullptr) {
        namespace_->AddInterface(this);
    }
}

void ASTInterfaceType::AddMethod(const AutoPtr<ASTMethod> &method)
{
    if (method == nullptr) {
        return;
    }
    methods_.push_back(method);
}

AutoPtr<ASTMethod> ASTInterfaceType::GetMethod(size_t index)
{
    if (index >= methods_.size()) {
        return nullptr;
    }

    return methods_[index];
}

bool ASTInterfaceType::AddExtendsInterface(AutoPtr<ASTInterfaceType> interface)
{
    if (extendsInterface_ != nullptr) {
        return false;
    }
    extendsInterface_ = interface;
    return true;
}

void ASTInterfaceType::SetVersion(size_t &majorVer, size_t &minorVer)
{
    majorVersion_ = majorVer;
    minorVersion_ = minorVer;
}

std::vector<AutoPtr<ASTMethod>> ASTInterfaceType::GetMethodsBySystem(SystemLevel system) const
{
    std::vector<AutoPtr<ASTMethod>> methods;
    for (const auto &method : methods_) {
        if (method->GetAttribute()->Match(system)) {
            methods.push_back(method);
        }
    }
    return methods;
}

bool ASTInterfaceType::IsInterfaceType()
{
    return true;
}

std::string ASTInterfaceType::Dump(const std::string &prefix)
{
    StringBuilder sb;

    sb.Append(prefix);
    sb.Append(prefix).Append(attr_->Dump(prefix)).Append(" ");
    sb.AppendFormat("interface %s {\n", name_.c_str());
    for (auto method : methods_) {
        std::string info = method->Dump(prefix + "  ");
        sb.Append(info);
        if (method != methods_[methods_.size() - 1]) {
            sb.Append('\n');
        }
    }
    sb.Append(prefix).Append("}\n");

    return sb.ToString();
}

TypeKind ASTInterfaceType::GetTypeKind()
{
    return TypeKind::TYPE_INTERFACE;
}

std::string ASTInterfaceType::GetFullName() const
{
    return namespace_->ToString() + name_;
}

std::string ASTInterfaceType::EmitDescMacroName() const
{
    return StringHelper::Format("%s_INTERFACE_DESC", StringHelper::StrToUpper(name_).c_str());
}

std::string ASTInterfaceType::EmitCType(TypeMode mode) const
{
    switch (mode) {
        case TypeMode::NO_MODE:
            return StringHelper::Format("struct %s", name_.c_str());
        case TypeMode::PARAM_IN:
            return StringHelper::Format("struct %s*", name_.c_str());
        case TypeMode::PARAM_OUT:
            return StringHelper::Format("struct %s**", name_.c_str());
        case TypeMode::LOCAL_VAR:
            return StringHelper::Format("struct %s*", name_.c_str());
        default:
            return "unknow type";
    }
}

std::string ASTInterfaceType::EmitCppType(TypeMode mode) const
{
    std::string pointerName = "sptr";
    if (Options::GetInstance().GetSystemLevel() == SystemLevel::LITE) {
        pointerName = "std::shared_ptr";
    }
    switch (mode) {
        case TypeMode::NO_MODE:
            return StringHelper::Format("%s<%s>", pointerName.c_str(), GetNameWithNamespace(namespace_, name_).c_str());
        case TypeMode::PARAM_IN:
            return StringHelper::Format(
                "const %s<%s>&", pointerName.c_str(), GetNameWithNamespace(namespace_, name_).c_str());
        case TypeMode::PARAM_OUT:
            return StringHelper::Format(
                "%s<%s>&", pointerName.c_str(), GetNameWithNamespace(namespace_, name_).c_str());
        case TypeMode::LOCAL_VAR:
            return StringHelper::Format("%s<%s>", pointerName.c_str(), GetNameWithNamespace(namespace_, name_).c_str());
        default:
            return "unknow type";
    }
}

std::string ASTInterfaceType::EmitJavaType(TypeMode mode, bool isInnerType) const
{
    return name_;
}

void ASTInterfaceType::EmitCWriteVar(const std::string &parcelName, const std::string &name, const std::string &ecName,
    const std::string &gotoLabel, StringBuilder &sb, const std::string &prefix) const
{
    sb.Append(prefix).AppendFormat("if (!WriteInterface(%s, %s, %s)) {\n", parcelName.c_str(),
        EmitDescMacroName().c_str(), name.c_str());
    sb.Append(prefix + TAB).AppendFormat("HDF_LOGE(\"%%{public}s: write %s failed!\", __func__);\n", name.c_str());
    sb.Append(prefix + TAB).AppendFormat("%s = HDF_ERR_INVALID_PARAM;\n", ecName.c_str());
    sb.Append(prefix + TAB).AppendFormat("goto %s;\n", gotoLabel.c_str());
    sb.Append(prefix).Append("}\n");
}

void ASTInterfaceType::EmitCProxyReadVar(const std::string &parcelName, const std::string &name, bool isInnerType,
    const std::string &ecName, const std::string &gotoLabel, StringBuilder &sb, const std::string &prefix) const
{
    sb.Append(prefix).AppendFormat("*%s = Read%s(%s);\n", name.c_str(), name_.c_str(), parcelName.c_str());
    sb.Append(prefix).AppendFormat("if (*%s == NULL) {\n", name.c_str());
    sb.Append(prefix + TAB).AppendFormat("HDF_LOGE(\"%%{public}s: read %s failed!\", __func__);\n", name.c_str());
    sb.Append(prefix + TAB).AppendFormat("%s = HDF_ERR_INVALID_PARAM;\n", ecName.c_str());
    sb.Append(prefix + TAB).AppendFormat("goto %s;\n", gotoLabel.c_str());
    sb.Append(prefix).Append("}\n");
}

void ASTInterfaceType::EmitCStubReadVar(const std::string &parcelName, const std::string &name,
    const std::string &ecName, const std::string &gotoLabel, StringBuilder &sb, const std::string &prefix) const
{
    sb.Append(prefix).AppendFormat("%s = Read%s(%s);\n", name.c_str(), name_.c_str(), parcelName.c_str());
    sb.Append(prefix).AppendFormat("if (%s == NULL) {\n", name.c_str());
    sb.Append(prefix + TAB).AppendFormat("HDF_LOGE(\"%%{public}s: read %s failed!\", __func__);\n", name.c_str());
    sb.Append(prefix + TAB).AppendFormat("%s = HDF_ERR_INVALID_PARAM;\n", ecName.c_str());
    sb.Append(prefix + TAB).AppendFormat("goto %s;\n", gotoLabel.c_str());
    sb.Append(prefix).Append("}\n");
}

void ASTInterfaceType::EmitCppWriteVar(const std::string &parcelName, const std::string &name, StringBuilder &sb,
    const std::string &prefix, unsigned int innerLevel) const
{
    sb.Append(prefix).AppendFormat("if (%s == nullptr) {\n", name.c_str());
    sb.Append(prefix + TAB)
        .AppendFormat("HDF_LOGE(\"%%{public}s: parameter %s is nullptr!\", __func__);\n", name.c_str());
    sb.Append(prefix + TAB).Append("return HDF_ERR_INVALID_PARAM;\n");
    sb.Append(prefix).Append("}\n");
    sb.Append("\n");
    sb.Append(prefix).AppendFormat("if (!%s.WriteRemoteObject(", parcelName.c_str());
    sb.AppendFormat("OHOS::HDI::ObjectCollector::GetInstance().GetOrNewObject(%s, %s::GetDescriptor()))) {\n",
        name.c_str(), GetNameWithNamespace(namespace_, name_).c_str());
    sb.Append(prefix + TAB).AppendFormat("HDF_LOGE(\"%%{public}s: write %s failed!\", __func__);\n", name.c_str());
    sb.Append(prefix + TAB).Append("return HDF_ERR_INVALID_PARAM;\n");
    sb.Append(prefix).Append("}\n");
}

void ASTInterfaceType::EmitCppReadVar(const std::string &parcelName, const std::string &name, StringBuilder &sb,
    const std::string &prefix, bool initVariable, unsigned int innerLevel) const
{
    if (initVariable) {
        sb.Append(prefix).AppendFormat("sptr<%s> %s;\n", GetNameWithNamespace(namespace_, name_).c_str(), name.c_str());
    }
    sb.Append(prefix).AppendFormat("sptr<IRemoteObject> %sRemote = %s.ReadRemoteObject();\n", name.c_str(),
        parcelName.c_str());
    sb.Append(prefix).AppendFormat("if (%sRemote == nullptr) {\n", name.c_str());
    sb.Append(prefix + TAB).Append("HDF_LOGE(\"%{public}s: read an invalid remote object\", __func__);\n");
    sb.Append(prefix + TAB).Append("return HDF_ERR_INVALID_PARAM;\n");
    sb.Append(prefix).Append("}\n\n");
    sb.Append(prefix).AppendFormat("%s = new %s(%sRemote);\n", name.c_str(),
        ((StringHelper::StartWith(name_, "I") ? name_.substr(1) : name_) + "Proxy").c_str(), name.c_str());
    sb.Append(prefix).AppendFormat("if (%s == nullptr) {\n", name.c_str());
    sb.Append(prefix + TAB).Append("HDF_LOGE(\"%{public}s: failed to create interface object\", __func__);\n");
    sb.Append(prefix + TAB).Append("return HDF_ERR_INVALID_PARAM;\n");
    sb.Append(prefix).Append("}\n");
}

void ASTInterfaceType::EmitJavaWriteVar(
    const std::string &parcelName, const std::string &name, StringBuilder &sb, const std::string &prefix) const
{
    sb.Append(prefix).AppendFormat("%s.writeRemoteObject(%s.asObject());\n", parcelName.c_str(), name.c_str());
}

void ASTInterfaceType::EmitJavaReadVar(
    const std::string &parcelName, const std::string &name, StringBuilder &sb, const std::string &prefix) const
{
    std::string stubName = StringHelper::StartWith(name_, "I") ? (name_.substr(1) + "Stub") : (name_ + "Stub");
    sb.Append(prefix).AppendFormat(
        "%s = %s.asInterface(%s.readRemoteObject());\n", name.c_str(), stubName.c_str(), parcelName.c_str());
}

void ASTInterfaceType::EmitJavaReadInnerVar(const std::string &parcelName, const std::string &name, bool isInner,
    StringBuilder &sb, const std::string &prefix) const
{
    std::string stubName = StringHelper::StartWith(name_, "I") ? (name_.substr(1) + "Stub") : (name_ + "Stub");
    sb.Append(prefix).AppendFormat("%s %s = %s.asInterface(%s.readRemoteObject());\n",
        EmitJavaType(TypeMode::NO_MODE).c_str(), name.c_str(), stubName.c_str(), parcelName.c_str());
}

void ASTInterfaceType::RegisterWriteMethod(Language language, SerMode mode, UtilMethodMap &methods) const
{
    using namespace std::placeholders;
    if (language == Language::C) {
        methods.emplace("WriteInterface", std::bind(&ASTInterfaceType::EmitCWriteMethods, this, _1, _2, _3, _4));
    }
}

void ASTInterfaceType::RegisterReadMethod(Language language, SerMode mode, UtilMethodMap &methods) const
{
    using namespace std::placeholders;

    switch (language) {
        case Language::C: {
            std::string methodName = StringHelper::Format("Read%s", name_.c_str());
            methods.emplace(methodName, std::bind(&ASTInterfaceType::EmitCReadMethods, this, _1, _2, _3, _4));
            break;
        }
        default:
            break;
    }
}

void ASTInterfaceType::EmitCWriteMethods(StringBuilder &sb, const std::string &prefix,
    const std::string &methodPrefix, bool isDecl) const
{
    std::string methodName = StringHelper::Format("%sWriteInterface", methodPrefix.c_str());
    if (isDecl) {
        sb.Append(prefix).AppendFormat("static bool %s(struct HdfSBuf *parcel, const char *desc, void *interface);\n",
            methodName.c_str());
        return;
    }
    sb.Append(prefix).AppendFormat("static bool %s(struct HdfSBuf *parcel, const char *desc, void *interface)\n",
        methodName.c_str());
    sb.Append(prefix).Append("{\n");
    sb.Append(prefix + TAB).Append("if (interface == NULL) {\n");
    sb.Append(prefix + TAB + TAB).Append("HDF_LOGE(\"%{public}s: invalid interface object\", __func__);\n");
    sb.Append(prefix + TAB + TAB).Append("return false;\n");
    sb.Append(prefix + TAB).Append("}\n\n");
    sb.Append(prefix + TAB).Append("struct HdfRemoteService **stub = StubCollectorGetOrNewObject(desc, interface);\n");
    sb.Append(prefix + TAB).Append("if (stub == NULL) {\n");
    sb.Append(prefix + TAB + TAB).Append(
        "HDF_LOGE(\"%{public}s: failed to get stub of '%{public}s'\", __func__, desc);\n");
    sb.Append(prefix + TAB + TAB).Append("return false;\n");
    sb.Append(prefix + TAB).Append("}\n\n");
    sb.Append(prefix + TAB).Append("if (HdfSbufWriteRemoteService(parcel, *stub) != HDF_SUCCESS) {\n");
    sb.Append(prefix + TAB + TAB).Append("HDF_LOGE(\"%{public}s: failed to write remote service\", __func__);\n");
    sb.Append(prefix + TAB + TAB).Append("return false;\n");
    sb.Append(prefix + TAB).Append("}\n\n");
    sb.Append(prefix + TAB).Append("return true;\n");
    sb.Append(prefix).Append("}\n");
}

void ASTInterfaceType::EmitCReadMethods(StringBuilder &sb, const std::string &prefix,
    const std::string &methodPrefix, bool isDecl) const
{
    std::string methodName = StringHelper::Format("%sRead%s", methodPrefix.c_str(), name_.c_str());
    if (isDecl) {
        sb.Append(prefix).AppendFormat("static struct %s *%s(struct HdfSBuf *parcel);\n", name_.c_str(),
            methodName.c_str());
        return;
    }
    sb.Append(prefix).AppendFormat("static struct %s *%s(struct HdfSBuf *parcel)\n", name_.c_str(),
        methodName.c_str());
    sb.Append(prefix).Append("{\n");
    sb.Append(prefix + TAB).Append("struct HdfRemoteService *remote = HdfSbufReadRemoteService(parcel);\n");
    sb.Append(prefix + TAB).Append("if (remote == NULL) {\n");
    sb.Append(prefix + TAB + TAB).Append("HDF_LOGE(\"%{public}s: ");
    sb.AppendFormat(" failed to read remote service of '%s'\", __func__);\n", name_.c_str());
    sb.Append(prefix + TAB + TAB).Append("return NULL;\n");
    sb.Append(prefix + TAB).Append("}\n\n");
    sb.Append(prefix + TAB).AppendFormat("return %sGet(remote);\n", name_.c_str());
    sb.Append(prefix).Append("}\n");
}
} // namespace HDI
} // namespace OHOS
