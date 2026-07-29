/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include "ast/ast_sequenceable_type.h"
#include "util/string_builder.h"

namespace OHOS {
namespace HDI {
void ASTSequenceableType::SetNamespace(const AutoPtr<ASTNamespace> &nspace)
{
    ASTType::SetNamespace(nspace);
    if (namespace_ != nullptr) {
        namespace_->AddSequenceable(this);
    }
}

bool ASTSequenceableType::IsSequenceableType()
{
    return true;
}

TypeKind ASTSequenceableType::GetTypeKind()
{
    return TypeKind::TYPE_SEQUENCEABLE;
}

std::string ASTSequenceableType::Dump(const std::string &prefix)
{
    StringBuilder sb;

    sb.Append(prefix).Append("sequenceable ");
    if (namespace_ != nullptr) {
        sb.Append(namespace_->ToString());
    }
    sb.Append(name_);
    sb.Append(";\n");

    return sb.ToString();
}

std::string ASTSequenceableType::GetFullName() const
{
    return namespace_->ToString() + name_;
}

std::string ASTSequenceableType::EmitCType(TypeMode mode) const
{
    // c language has no Sequenceable type
    return "/";
}

std::string ASTSequenceableType::EmitCppType(TypeMode mode) const
{
    switch (mode) {
        case TypeMode::NO_MODE:
            return StringHelper::Format("sptr<%s>", GetNameWithNamespace(namespace_, name_).c_str());
        case TypeMode::PARAM_IN:
            return StringHelper::Format("const sptr<%s>&", GetNameWithNamespace(namespace_, name_).c_str());
        case TypeMode::PARAM_OUT:
            return StringHelper::Format("sptr<%s>&", GetNameWithNamespace(namespace_, name_).c_str());
        case TypeMode::LOCAL_VAR:
            return StringHelper::Format("sptr<%s>", GetNameWithNamespace(namespace_, name_).c_str());
        default:
            return "unknow type";
    }
}

std::string ASTSequenceableType::EmitJavaType(TypeMode mode, bool isInnerType) const
{
    return name_;
}

void ASTSequenceableType::EmitCppWriteVar(const std::string &parcelName, const std::string &name, StringBuilder &sb,
    const std::string &prefix, unsigned int innerLevel) const
{
    sb.Append(prefix).AppendFormat("if (!%s.WriteStrongParcelable(%s)) {\n", parcelName.c_str(), name.c_str());
    sb.Append(prefix + TAB).AppendFormat("HDF_LOGE(\"%%{public}s: write %s failed!\", __func__);\n", name.c_str());
    sb.Append(prefix + TAB).Append("return HDF_ERR_INVALID_PARAM;\n");
    sb.Append(prefix).Append("}\n");
}

void ASTSequenceableType::EmitCppReadVar(const std::string &parcelName, const std::string &name, StringBuilder &sb,
    const std::string &prefix, bool initVariable, unsigned int innerLevel) const
{
    if (initVariable) {
        sb.Append(prefix).AppendFormat("sptr<%s> %s = %s.ReadStrongParcelable<%s>();\n", name_.c_str(), name.c_str(),
            parcelName.c_str(), name_.c_str());
    } else {
        sb.Append(prefix).AppendFormat(
            "%s = %s.ReadStrongParcelable<%s>();\n", name.c_str(), parcelName.c_str(), name_.c_str());
    }
}

void ASTSequenceableType::EmitCppMarshalling(const std::string &parcelName, const std::string &name, StringBuilder &sb,
    const std::string &prefix, unsigned int innerLevel) const
{
    sb.Append(prefix).AppendFormat("if (!%s.WriteStrongParcelable(%s)) {\n", parcelName.c_str(), name.c_str());
    sb.Append(prefix + TAB).AppendFormat("HDF_LOGE(\"%%{public}s: write %s failed!\", __func__);\n", name.c_str());
    sb.Append(prefix + TAB).Append("return false;\n");
    sb.Append(prefix).Append("}\n");
}

void ASTSequenceableType::EmitCppUnMarshalling(const std::string &parcelName, const std::string &name,
    StringBuilder &sb, const std::string &prefix, bool emitType, unsigned int innerLevel) const
{
    if (emitType) {
        sb.Append(prefix).AppendFormat("%s %s = %s.ReadStrongParcelable<%s>();\n", EmitCppType().c_str(), name.c_str(),
            parcelName.c_str(), name_.c_str());
    } else {
        sb.Append(prefix).AppendFormat(
            "%s = %s.ReadStrongParcelable<%s>();\n", name.c_str(), parcelName.c_str(), name_.c_str());
    }
}

void ASTSequenceableType::EmitJavaWriteVar(
    const std::string &parcelName, const std::string &name, StringBuilder &sb, const std::string &prefix) const
{
    if (EmitJavaType(TypeMode::NO_MODE) == "IRemoteObject") {
        sb.Append(prefix).AppendFormat("%s.writeRemoteObject(%s);\n", parcelName.c_str(), name.c_str());
        return;
    }
    sb.Append(prefix).AppendFormat("%s.writeSequenceable(%s);\n", parcelName.c_str(), name.c_str());
}

void ASTSequenceableType::EmitJavaReadVar(
    const std::string &parcelName, const std::string &name, StringBuilder &sb, const std::string &prefix) const
{
    if (EmitJavaType(TypeMode::NO_MODE) == "IRemoteObject") {
        sb.Append(prefix).AppendFormat("%s = %s.readRemoteObject();\n", name.c_str(), parcelName.c_str());
        return;
    }
    sb.Append(prefix).AppendFormat("%s.readSequenceable(%s);\n", parcelName.c_str(), name.c_str());
}

void ASTSequenceableType::EmitJavaReadInnerVar(const std::string &parcelName, const std::string &name, bool isInner,
    StringBuilder &sb, const std::string &prefix) const
{
    if (!isInner && EmitJavaType(TypeMode::NO_MODE) == "IRemoteObject") {
        sb.Append(prefix).AppendFormat("IRemoteObject %s = %s.readRemoteObject();\n", name.c_str(), parcelName.c_str());
        return;
    }
    if (!isInner) {
        sb.Append(prefix).AppendFormat("%s %s = new %s();\n", EmitJavaType(TypeMode::NO_MODE).c_str(), name.c_str(),
            EmitJavaType(TypeMode::NO_MODE).c_str());
    }
    sb.Append(prefix).AppendFormat("%s.readSequenceable(%s);\n", parcelName.c_str(), name.c_str());
}
} // namespace HDI
} // namespace OHOS