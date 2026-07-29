/*
 * Copyright (c) 2021-2022 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include "ast/ast_struct_type.h"
#include "util/string_builder.h"

namespace OHOS {
namespace HDI {
void ASTStructType::AddMember(const AutoPtr<ASTType> &typeName, std::string name)
{
    for (auto it : members_) {
        if (std::get<0>(it) == name) {
            return;
        }
    }
    members_.push_back(std::make_tuple(name, typeName));

    if (!typeName->IsPod()) {
        isPod_ = false;
    }
}

bool ASTStructType::IsStructType()
{
    return true;
}

std::string ASTStructType::Dump(const std::string &prefix)
{
    StringBuilder sb;
    sb.Append(prefix).Append(attr_->Dump(prefix)).Append(" ");
    sb.AppendFormat("struct %s {\n", name_.c_str());
    if (members_.size() > 0) {
        for (auto it : members_) {
            sb.Append(prefix + "  ");
            sb.AppendFormat("%s %s;\n", std::get<1>(it)->ToString().c_str(), std::get<0>(it).c_str());
        }
    }
    sb.Append(prefix).Append("};\n");
    return sb.ToString();
}

TypeKind ASTStructType::GetTypeKind()
{
    return TypeKind::TYPE_STRUCT;
}

std::string ASTStructType::EmitCType(TypeMode mode) const
{
    switch (mode) {
        case TypeMode::NO_MODE:
            return StringHelper::Format("struct %s", name_.c_str());
        case TypeMode::PARAM_IN:
            return StringHelper::Format("const struct %s*", name_.c_str());
        case TypeMode::PARAM_OUT:
            return StringHelper::Format("struct %s*", name_.c_str());
        case TypeMode::LOCAL_VAR:
            return StringHelper::Format("struct %s*", name_.c_str());
        default:
            return "unknow type";
    }
}

std::string ASTStructType::EmitCppType(TypeMode mode) const
{
    switch (mode) {
        case TypeMode::NO_MODE:
            return StringHelper::Format("%s", GetNameWithNamespace(namespace_, name_).c_str());
        case TypeMode::PARAM_IN:
            return StringHelper::Format("const %s&", GetNameWithNamespace(namespace_, name_).c_str());
        case TypeMode::PARAM_OUT:
            return StringHelper::Format("%s&", GetNameWithNamespace(namespace_, name_).c_str());
        case TypeMode::LOCAL_VAR:
            return StringHelper::Format("%s", GetNameWithNamespace(namespace_, name_).c_str());
        default:
            return "unknow type";
    }
}

std::string ASTStructType::EmitJavaType(TypeMode mode, bool isInnerType) const
{
    // currently, Java does not support the struct type.
    return "/";
}

std::string ASTStructType::EmitCTypeDecl() const
{
    StringBuilder sb;
    sb.AppendFormat("struct %s {\n", name_.c_str());

    for (auto it : members_) {
        AutoPtr<ASTType> member = std::get<1>(it);
        std::string memberName = std::get<0>(it);
        sb.Append(TAB).AppendFormat("%s %s;\n", member->EmitCType().c_str(), memberName.c_str());
        if (member->GetTypeKind() == TypeKind::TYPE_ARRAY || member->GetTypeKind() == TypeKind::TYPE_LIST) {
            sb.Append(TAB).AppendFormat("uint32_t %sLen;\n", memberName.c_str());
        }
    }

    sb.Append("}");
    if (IsPod()) {
        sb.Append(" __attribute__ ((aligned(8)))");
    }
    sb.Append(";");

    return sb.ToString();
}

std::string ASTStructType::EmitCppTypeDecl() const
{
    StringBuilder sb;
    sb.AppendFormat("struct %s {\n", name_.c_str());

    for (auto it : members_) {
        AutoPtr<ASTType> member = std::get<1>(it);
        std::string memberName = std::get<0>(it);
        sb.Append(TAB).AppendFormat("%s %s;\n", member->EmitCppType().c_str(), memberName.c_str());
    }

    sb.Append("}");
    if (IsPod()) {
        sb.Append(" __attribute__ ((aligned(8)))");
    }
    sb.Append(";");
    return sb.ToString();
}

std::string ASTStructType::EmitJavaTypeDecl() const
{
    StringBuilder sb;

    return sb.ToString();
}

void ASTStructType::EmitCWriteVar(const std::string &parcelName, const std::string &name, const std::string &ecName,
    const std::string &gotoLabel, StringBuilder &sb, const std::string &prefix) const
{
    sb.Append(prefix).AppendFormat(
        "if (!%sBlockMarshalling(%s, %s)) {\n", name_.c_str(), parcelName.c_str(), name.c_str());
    sb.Append(prefix + TAB).AppendFormat("HDF_LOGE(\"%%{public}s: write %s failed!\", __func__);\n", name.c_str());
    sb.Append(prefix + TAB).AppendFormat("%s = HDF_ERR_INVALID_PARAM;\n", ecName.c_str());
    sb.Append(prefix + TAB).AppendFormat("goto %s;\n", gotoLabel.c_str());
    sb.Append(prefix).Append("}\n");
}

void ASTStructType::EmitCProxyReadVar(const std::string &parcelName, const std::string &name, bool isInnerType,
    const std::string &ecName, const std::string &gotoLabel, StringBuilder &sb, const std::string &prefix) const
{
    sb.Append(prefix).AppendFormat(
        "if (!%sBlockUnmarshalling(%s, %s)) {\n", name_.c_str(), parcelName.c_str(), name.c_str());
    sb.Append(prefix + TAB).AppendFormat("HDF_LOGE(\"%%{public}s: read %s failed!\", __func__);\n", name.c_str());
    sb.Append(prefix + TAB).AppendFormat("%s = HDF_ERR_INVALID_PARAM;\n", ecName.c_str());
    sb.Append(prefix + TAB).AppendFormat("goto %s;\n", gotoLabel.c_str());
    sb.Append(prefix).Append("}\n");
}

void ASTStructType::EmitCStubReadVar(const std::string &parcelName, const std::string &name, const std::string &ecName,
    const std::string &gotoLabel, StringBuilder &sb, const std::string &prefix) const
{
    sb.Append(prefix).AppendFormat(
        "if (!%sBlockUnmarshalling(%s, %s)) {\n", name_.c_str(), parcelName.c_str(), name.c_str());
    sb.Append(prefix + TAB).AppendFormat("HDF_LOGE(\"%%{public}s: read %s failed!\", __func__);\n", name.c_str());
    sb.Append(prefix + TAB).AppendFormat("%s = HDF_ERR_INVALID_PARAM;\n", ecName.c_str());
    sb.Append(prefix + TAB).AppendFormat("goto %s;\n", gotoLabel.c_str());
    sb.Append(prefix).AppendFormat("}\n");
}

void ASTStructType::EmitCppWriteVar(const std::string &parcelName, const std::string &name, StringBuilder &sb,
    const std::string &prefix, unsigned int innerLevel) const
{
    sb.Append(prefix).AppendFormat(
        "if (!%sBlockMarshalling(%s, %s)) {\n", EmitCppType().c_str(), parcelName.c_str(), name.c_str());
    sb.Append(prefix + TAB).AppendFormat("HDF_LOGE(\"%%{public}s: write %s failed!\", __func__);\n", name.c_str());
    sb.Append(prefix + TAB).Append("return HDF_ERR_INVALID_PARAM;\n");
    sb.Append(prefix).Append("}\n");
}

void ASTStructType::EmitCppReadVar(const std::string &parcelName, const std::string &name, StringBuilder &sb,
    const std::string &prefix, bool initVariable, unsigned int innerLevel) const
{
    if (initVariable) {
        sb.Append(prefix).AppendFormat("%s %s;\n", EmitCppType().c_str(), name.c_str());
    }
    sb.Append(prefix).AppendFormat(
        "if (!%sBlockUnmarshalling(%s, %s)) {\n", name_.c_str(), parcelName.c_str(), name.c_str());
    sb.Append(prefix + TAB).AppendFormat("HDF_LOGE(\"%%{public}s: read %s failed!\", __func__);\n", name.c_str());
    sb.Append(prefix + TAB).Append("return HDF_ERR_INVALID_PARAM;\n");
    sb.Append(prefix).Append("}\n");
}

void ASTStructType::EmitCMarshalling(const std::string &name, StringBuilder &sb, const std::string &prefix) const
{
    sb.Append(prefix).AppendFormat("if (!%sBlockMarshalling(data, &%s)) {\n", name_.c_str(), name.c_str());
    sb.Append(prefix + TAB).AppendFormat("HDF_LOGE(\"%%{public}s: write %s failed!\", __func__);\n", name.c_str());
    sb.Append(prefix + TAB).Append("return false;\n");
    sb.Append(prefix).Append("}\n");
}

void ASTStructType::EmitCUnMarshalling(const std::string &name, const std::string &gotoLabel, StringBuilder &sb,
    const std::string &prefix, std::vector<std::string> &freeObjStatements) const
{
    sb.Append(prefix).AppendFormat("if (!%sBlockUnmarshalling(data, %s)) {\n", name_.c_str(), name.c_str());
    sb.Append(prefix + TAB).AppendFormat("HDF_LOGE(\"%%{public}s: read %s failed!\", __func__);\n", name.c_str());
    sb.Append(prefix + TAB).AppendFormat("goto %s;\n", gotoLabel.c_str());
    sb.Append(prefix).Append("}\n");
}

void ASTStructType::EmitCppMarshalling(const std::string &parcelName, const std::string &name, StringBuilder &sb,
    const std::string &prefix, unsigned int innerLevel) const
{
    sb.Append(prefix).AppendFormat(
        "if (!%sBlockMarshalling(%s, %s)) {\n", name_.c_str(), parcelName.c_str(), name.c_str());
    sb.Append(prefix + TAB).AppendFormat("HDF_LOGE(\"%%{public}s: write %s failed!\", __func__);\n", name.c_str());
    sb.Append(prefix + TAB).Append("return false;\n");
    sb.Append(prefix).Append("}\n");
}

void ASTStructType::EmitCppUnMarshalling(const std::string &parcelName, const std::string &name, StringBuilder &sb,
    const std::string &prefix, bool emitType, unsigned int innerLevel) const
{
    sb.Append(prefix).AppendFormat("if (!%sBlockUnmarshalling(data, %s)) {\n", EmitCppType().c_str(), name.c_str());
    sb.Append(prefix + TAB).AppendFormat("HDF_LOGE(\"%%{public}s: read %s failed!\", __func__);\n", name.c_str());
    sb.Append(prefix + TAB).Append("return false;\n");
    sb.Append(prefix).Append("}\n");
}

void ASTStructType::EmitMemoryRecycle(
    const std::string &name, bool isClient, bool ownership, StringBuilder &sb, const std::string &prefix) const
{
    std::string varName = name;
    if (ownership) {
        sb.Append(prefix).AppendFormat("if (%s != NULL) {\n", varName.c_str());
        sb.Append(prefix + TAB).AppendFormat("%sFree(%s, true);\n", name_.c_str(), varName.c_str());
        sb.Append(prefix + TAB).AppendFormat("%s = NULL;\n", varName.c_str());
        sb.Append(prefix).Append("}\n");
    } else {
        sb.Append(prefix).AppendFormat("%sFree(&%s, false);\n", name_.c_str(), name.c_str());
    }
}
} // namespace HDI
} // namespace OHOS