/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include "ast/ast_smq_type.h"

namespace OHOS {
namespace HDI {
bool ASTSmqType::IsSmqType()
{
    return true;
}

bool ASTSmqType::HasInnerType(TypeKind innerTypeKind) const
{
    if (innerType_ == nullptr) {
        return false;
    }

    if (innerType_->GetTypeKind() == innerTypeKind) {
        return true;
    }

    return innerType_->HasInnerType(innerTypeKind);
}

std::string ASTSmqType::ToString() const
{
    return StringHelper::Format("SharedMemQueue<%s>", innerType_->ToString().c_str());
}

TypeKind ASTSmqType::GetTypeKind()
{
    return TypeKind::TYPE_SMQ;
}

std::string ASTSmqType::EmitCppType(TypeMode mode) const
{
    switch (mode) {
        case TypeMode::NO_MODE:
            return StringHelper::Format("std::shared_ptr<SharedMemQueue<%s>>", innerType_->EmitCppType().c_str());
        case TypeMode::PARAM_IN:
            return StringHelper::Format(
                "const std::shared_ptr<SharedMemQueue<%s>>&", innerType_->EmitCppType().c_str());
        case TypeMode::PARAM_OUT:
            return StringHelper::Format("std::shared_ptr<SharedMemQueue<%s>>&", innerType_->EmitCppType().c_str());
        case TypeMode::LOCAL_VAR:
            return StringHelper::Format("std::shared_ptr<SharedMemQueue<%s>>", innerType_->EmitCppType().c_str());
        default:
            return "unknow type";
    }
}

void ASTSmqType::EmitCppWriteVar(const std::string &parcelName, const std::string &name, StringBuilder &sb,
    const std::string &prefix, unsigned int innerLevel) const
{
    sb.Append(prefix).AppendFormat(
        "if (%s == nullptr || !%s->IsGood() || %s->GetMeta() == nullptr || ", name.c_str(), name.c_str(), name.c_str());
    sb.AppendFormat("!%s->GetMeta()->Marshalling(%s)) {\n", name.c_str(), parcelName.c_str());
    sb.Append(prefix + TAB).AppendFormat("HDF_LOGE(\"%%{public}s: write %s failed!\", __func__);\n", name.c_str());
    sb.Append(prefix + TAB).Append("return HDF_ERR_INVALID_PARAM;\n");
    sb.Append(prefix).Append("}\n");
}

void ASTSmqType::EmitCppReadVar(const std::string &parcelName, const std::string &name, StringBuilder &sb,
    const std::string &prefix, bool initVariable, unsigned int innerLevel) const
{
    std::string metaVarName = StringHelper::Format("%sMeta_", name.c_str());
    sb.Append(prefix).AppendFormat(
        "std::shared_ptr<SharedMemQueueMeta<%s>> %s = ", innerType_->EmitCppType().c_str(), metaVarName.c_str());
    sb.AppendFormat(
        "SharedMemQueueMeta<%s>::UnMarshalling(%s);\n", innerType_->EmitCppType().c_str(), parcelName.c_str());
    sb.Append(prefix).AppendFormat("if (%s == nullptr) {\n", metaVarName.c_str());
    sb.Append(prefix + TAB).AppendFormat("HDF_LOGE(\"%%{public}s: SharedMemQueueMeta is nullptr\", __func__);\n");
    sb.Append(prefix + TAB).Append("return HDF_ERR_INVALID_PARAM;\n");
    sb.Append(prefix).Append("}\n\n");

    if (initVariable) {
        sb.Append(prefix).AppendFormat("%s %s = ", EmitCppType(TypeMode::LOCAL_VAR).c_str(), name.c_str());
    } else {
        sb.Append(prefix).AppendFormat("%s = ", name.c_str());
    }

    sb.AppendFormat(
        "std::make_shared<SharedMemQueue<%s>>(*%s);\n", innerType_->EmitCppType().c_str(), metaVarName.c_str());
}

void ASTSmqType::EmitCppMarshalling(const std::string &parcelName, const std::string &name, StringBuilder &sb,
    const std::string &prefix, unsigned int innerLevel) const
{
    sb.Append(prefix).AppendFormat(
        "if (%s == nullptr || !%s->IsGood() || %s->GetMeta() == nullptr || ", name.c_str(), name.c_str(), name.c_str());
    sb.AppendFormat("!%s->GetMeta()->Marshalling(%s)) {\n", name.c_str(), parcelName.c_str());
    sb.Append(prefix + TAB).AppendFormat("HDF_LOGE(\"%%{public}s: write %s failed!\", __func__);\n", name.c_str());
    sb.Append(prefix + TAB).Append("return false;\n");
    sb.Append(prefix).Append("}\n");
}

void ASTSmqType::EmitCppUnMarshalling(const std::string &parcelName, const std::string &name, StringBuilder &sb,
    const std::string &prefix, bool emitType, unsigned int innerLevel) const
{
    size_t index = name.find('.');
    std::string memberName = (index == std::string::npos) ? name : StringHelper::SubStr(name, index + 1);
    std::string metaVarName = StringHelper::Format("%sMeta_", memberName.c_str());

    sb.Append(prefix).AppendFormat(
        "std::shared_ptr<SharedMemQueueMeta<%s>> %s = ", innerType_->EmitCppType().c_str(), metaVarName.c_str());
    sb.AppendFormat(
        "SharedMemQueueMeta<%s>::UnMarshalling(%s);\n", innerType_->EmitCppType().c_str(), parcelName.c_str());
    sb.Append(prefix).AppendFormat("if (%s == nullptr) {\n", metaVarName.c_str());
    sb.Append(prefix + TAB).AppendFormat("HDF_LOGE(\"%%{public}s: SharedMemQueueMeta is nullptr\", __func__);\n");
    sb.Append(prefix + TAB).Append("return false;\n");
    sb.Append(prefix).Append("}\n\n");

    if (emitType) {
        sb.Append(prefix).AppendFormat("%s %s = ", EmitCppType(TypeMode::LOCAL_VAR).c_str(), memberName.c_str());
    } else {
        sb.Append(prefix).AppendFormat("%s = ", name.c_str());
    }

    sb.AppendFormat(
        "std::make_shared<SharedMemQueue<%s>>(*%s);\n", innerType_->EmitCppType().c_str(), metaVarName.c_str());
}

bool ASTAshmemType::IsAshmemType()
{
    return true;
}

std::string ASTAshmemType::ToString() const
{
    return "Ashmem";
}

TypeKind ASTAshmemType::GetTypeKind()
{
    return TypeKind::TYPE_ASHMEM;
}

std::string ASTAshmemType::EmitCppType(TypeMode mode) const
{
    switch (mode) {
        case TypeMode::NO_MODE:
            return StringHelper::Format("sptr<Ashmem>");
        case TypeMode::PARAM_IN:
            return StringHelper::Format("const sptr<Ashmem>&");
        case TypeMode::PARAM_OUT:
            return StringHelper::Format("sptr<Ashmem>&");
        case TypeMode::LOCAL_VAR:
            return StringHelper::Format("sptr<Ashmem>");
        default:
            return "unknow type";
    }
}

void ASTAshmemType::EmitCppWriteVar(const std::string &parcelName, const std::string &name, StringBuilder &sb,
    const std::string &prefix, unsigned int innerLevel) const
{
    sb.Append(prefix).AppendFormat(
        "if (%s == nullptr || !%s.WriteAshmem(%s)) {\n", name.c_str(), parcelName.c_str(), name.c_str());
    sb.Append(prefix + TAB).AppendFormat("HDF_LOGE(\"%%{public}s: failed to write %s\", __func__);\n", name.c_str());
    sb.Append(prefix + TAB).Append("return HDF_ERR_INVALID_PARAM;\n");
    sb.Append(prefix).Append("}\n");
}

void ASTAshmemType::EmitCppReadVar(const std::string &parcelName, const std::string &name, StringBuilder &sb,
    const std::string &prefix, bool initVariable, unsigned int innerLevel) const
{
    if (initVariable) {
        sb.Append(prefix).AppendFormat(
            "%s %s = %s.ReadAshmem();\n", EmitCppType().c_str(), name.c_str(), parcelName.c_str());
    } else {
        sb.Append(prefix).AppendFormat("%s = %s.ReadAshmem();\n", name.c_str(), parcelName.c_str());
    }

    sb.Append(prefix).AppendFormat("if (%s == nullptr) {\n", name.c_str());
    sb.Append(prefix + TAB).Append("HDF_LOGE(\"%{public}s: failed to read ashmem object\", __func__);\n");
    sb.Append(prefix + TAB).Append("return HDF_ERR_INVALID_PARAM;\n");
    sb.Append(prefix).Append("}\n");
}

void ASTAshmemType::EmitCppMarshalling(const std::string &parcelName, const std::string &name, StringBuilder &sb,
    const std::string &prefix, unsigned int innerLevel) const
{
    sb.Append(prefix).AppendFormat(
        "if (%s == nullptr || !%s.WriteAshmem(%s)) {\n", name.c_str(), parcelName.c_str(), name.c_str());
    sb.Append(prefix + TAB).AppendFormat("HDF_LOGE(\"%%{public}s: failed to write %s\", __func__);\n", name.c_str());
    sb.Append(prefix + TAB).Append("return false;\n");
    sb.Append(prefix).Append("}\n");
}

void ASTAshmemType::EmitCppUnMarshalling(const std::string &parcelName, const std::string &name, StringBuilder &sb,
    const std::string &prefix, bool emitType, unsigned int innerLevel) const
{
    if (emitType) {
        sb.Append(prefix).AppendFormat(
            "%s %s = %s.ReadAshmem();\n", EmitCppType().c_str(), name.c_str(), parcelName.c_str());
    } else {
        sb.Append(prefix).AppendFormat("%s = %s.ReadAshmem();\n", name.c_str(), parcelName.c_str());
    }

    sb.Append(prefix).AppendFormat("if (%s == nullptr) {\n", name.c_str());
    sb.Append(prefix + TAB).Append("HDF_LOGE(\"%{public}s: failed to read ashmem object\", __func__);\n");
    sb.Append(prefix + TAB).Append("return false;\n");
    sb.Append(prefix).Append("}\n");
}
} // namespace HDI
} // namespace OHOS