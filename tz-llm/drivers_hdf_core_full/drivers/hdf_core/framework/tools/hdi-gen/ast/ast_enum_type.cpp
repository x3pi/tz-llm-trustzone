/*
 * Copyright (c) 2021-2022 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include "ast/ast_enum_type.h"
#include "util/string_builder.h"

namespace OHOS {
namespace HDI {
void ASTEnumType::SetBaseType(const AutoPtr<ASTType> &baseType)
{
    if (baseType == nullptr) {
        return;
    }
    if (baseType->GetTypeKind() == TypeKind::TYPE_ENUM) {
        AutoPtr<ASTEnumType> parentEnumType = dynamic_cast<ASTEnumType *>(baseType.Get());
        std::vector<AutoPtr<ASTEnumValue>> parentMembers = parentEnumType->GetMembers();
        for (auto member : parentMembers) {
            members_.push_back(member);
        }
        parentType_= baseType;
        baseType_ = parentEnumType->GetBaseType();
    } else {
        baseType_ = baseType;
    }
}

bool ASTEnumType::AddMember(const AutoPtr<ASTEnumValue> &member)
{
    for (auto members : members_) {
        if (member->GetName() == members->GetName()) {
            return false;
        }
    }
    members_.push_back(member);
    return true;
}

AutoPtr<ASTType> ASTEnumType::GetBaseType()
{
    return baseType_;
}

bool ASTEnumType::IsEnumType()
{
    return true;
}

std::string ASTEnumType::Dump(const std::string &prefix)
{
    StringBuilder sb;
    sb.Append(prefix).Append(attr_->Dump(prefix)).Append(" ");
    if (baseType_ != nullptr) {
        if (parentType_ != nullptr) {
            sb.AppendFormat("enum %s : %s ", name_.c_str(), parentType_->ToString().c_str());
            sb.AppendFormat(" : %s {\n",  baseType_->ToString().c_str());
        } else {
            sb.AppendFormat("enum %s : %s {\n", name_.c_str(), baseType_->ToString().c_str());
        }
    } else {
        sb.AppendFormat("enum %s {\n", name_.c_str());
    }

    if (members_.size() > 0) {
        for (auto it : members_) {
            AutoPtr<ASTExpr> value = it->GetExprValue();
            if (value == nullptr) {
                sb.Append("  ").AppendFormat("%s,\n", it->GetName().c_str());
            } else {
                sb.Append("  ").AppendFormat("%s = %s,\n", it->GetName().c_str(), value->Dump("").c_str());
            }
        }
    }

    sb.Append(prefix).Append("};\n");

    return sb.ToString();
}

TypeKind ASTEnumType::GetTypeKind()
{
    return TypeKind::TYPE_ENUM;
}

std::string ASTEnumType::EmitCType(TypeMode mode) const
{
    switch (mode) {
        case TypeMode::NO_MODE:
            return StringHelper::Format("enum %s", name_.c_str());
        case TypeMode::PARAM_IN:
            return StringHelper::Format("enum %s", name_.c_str());
        case TypeMode::PARAM_OUT:
            return StringHelper::Format("enum %s*", name_.c_str());
        case TypeMode::LOCAL_VAR:
            return StringHelper::Format("enum %s", name_.c_str());
        default:
            return "unknow type";
    }
}

std::string ASTEnumType::EmitCppType(TypeMode mode) const
{
    switch (mode) {
        case TypeMode::NO_MODE:
            return StringHelper::Format("%s", GetNameWithNamespace(namespace_, name_).c_str());
        case TypeMode::PARAM_IN:
            return StringHelper::Format("%s", GetNameWithNamespace(namespace_, name_).c_str());
        case TypeMode::PARAM_OUT:
            return StringHelper::Format("%s&", GetNameWithNamespace(namespace_, name_).c_str());
        case TypeMode::LOCAL_VAR:
            return StringHelper::Format("%s", GetNameWithNamespace(namespace_, name_).c_str());
        default:
            return "unknow type";
    }
}

std::string ASTEnumType::EmitJavaType(TypeMode mode, bool isInnerType) const
{
    // currently, Java does not support the enum type.
    return "/";
}

std::string ASTEnumType::EmitCTypeDecl() const
{
    StringBuilder sb;
    sb.AppendFormat("enum %s {\n", name_.c_str());

    for (auto it : members_) {
        if (it->GetExprValue() == nullptr) {
            sb.Append(TAB).AppendFormat("%s,\n", it->GetName().c_str());
        } else {
            sb.Append(TAB).AppendFormat("%s = %s,\n", it->GetName().c_str(), it->GetExprValue()->EmitCode().c_str());
        }
    }

    sb.Append("};");
    return sb.ToString();
}

std::string ASTEnumType::EmitCppTypeDecl() const
{
    StringBuilder sb;
    if (baseType_ != nullptr) {
        sb.AppendFormat("enum %s : %s {\n", name_.c_str(), baseType_->EmitCppType().c_str());
    } else {
        sb.AppendFormat("enum %s {\n", name_.c_str());
    }

    for (auto it : members_) {
        if (it->GetExprValue() == nullptr) {
            sb.Append(TAB).AppendFormat("%s,\n", it->GetName().c_str());
        } else {
            sb.Append(TAB).AppendFormat("%s = %s,\n", it->GetName().c_str(), it->GetExprValue()->EmitCode().c_str());
        }
    }

    sb.Append("};");
    return sb.ToString();
}

std::string ASTEnumType::EmitJavaTypeDecl() const
{
    StringBuilder sb;

    return sb.ToString();
}

void ASTEnumType::EmitCWriteVar(const std::string &parcelName, const std::string &name, const std::string &ecName,
    const std::string &gotoLabel, StringBuilder &sb, const std::string &prefix) const
{
    sb.Append(prefix).AppendFormat("if (!HdfSbufWriteUint64(%s, (uint64_t)%s)) {\n", parcelName.c_str(), name.c_str());
    sb.Append(prefix + TAB).AppendFormat("HDF_LOGE(\"%%{public}s: write %s failed!\", __func__);\n", name.c_str());
    sb.Append(prefix + TAB).AppendFormat("%s = HDF_ERR_INVALID_PARAM;\n", ecName.c_str());
    sb.Append(prefix + TAB).AppendFormat("goto %s;\n", gotoLabel.c_str());
    sb.Append(prefix).Append("}\n");
}

void ASTEnumType::EmitCProxyReadVar(const std::string &parcelName, const std::string &name, bool isInnerType,
    const std::string &ecName, const std::string &gotoLabel, StringBuilder &sb, const std::string &prefix) const
{
    std::string tmpVarName = "enumTmp";
    sb.Append(prefix).Append("{\n");
    sb.Append(prefix + TAB).AppendFormat("uint64_t %s = 0;\n", tmpVarName.c_str());
    sb.Append(prefix + TAB)
        .AppendFormat("if (!HdfSbufReadUint64(%s, &%s)) {\n", parcelName.c_str(), tmpVarName.c_str());
    sb.Append(prefix + TAB + TAB).AppendFormat("HDF_LOGE(\"%%{public}s: read %s failed!\", __func__);\n", name.c_str());
    sb.Append(prefix + TAB + TAB).AppendFormat("%s = HDF_ERR_INVALID_PARAM;\n", ecName.c_str());
    sb.Append(prefix + TAB + TAB).AppendFormat("goto %s;\n", gotoLabel.c_str());
    sb.Append(prefix + TAB).Append("}\n\n");
    sb.Append(prefix + TAB).AppendFormat("if (%s == NULL) {\n", name.c_str());
    sb.Append(prefix + TAB + TAB).AppendFormat("HDF_LOGE(\"%%{public}s: invlid parameter %s\", __func__);\n",
        name.c_str());
    sb.Append(prefix + TAB + TAB).AppendFormat("%s = HDF_ERR_INVALID_PARAM;\n", ecName.c_str());
    sb.Append(prefix + TAB + TAB).AppendFormat("goto %s;\n", gotoLabel.c_str());
    sb.Append(prefix + TAB).Append("}\n\n");
    sb.Append(prefix + TAB).AppendFormat("*%s = (%s)%s;\n", name.c_str(), EmitCType().c_str(), tmpVarName.c_str());
    sb.Append(prefix).Append("}\n");
}

void ASTEnumType::EmitCStubReadVar(const std::string &parcelName, const std::string &name, const std::string &ecName,
    const std::string &gotoLabel, StringBuilder &sb, const std::string &prefix) const
{
    std::string tmpVarName = "enumTmp";
    sb.Append(prefix).Append("{\n");
    sb.Append(prefix + TAB).AppendFormat("uint64_t %s = 0;\n", tmpVarName.c_str());
    sb.Append(prefix + TAB)
        .AppendFormat("if (!HdfSbufReadUint64(%s, &%s)) {\n", parcelName.c_str(), tmpVarName.c_str());
    sb.Append(prefix + TAB + TAB).AppendFormat("HDF_LOGE(\"%%{public}s: read %s failed!\", __func__);\n", name.c_str());
    sb.Append(prefix + TAB + TAB).AppendFormat("%s = HDF_ERR_INVALID_PARAM;\n", ecName.c_str());
    sb.Append(prefix + TAB + TAB).AppendFormat("goto %s;\n", gotoLabel.c_str());
    sb.Append(prefix + TAB).Append("}\n");
    sb.Append(prefix + TAB).AppendFormat("%s = (%s)%s;\n", name.c_str(), EmitCType().c_str(), tmpVarName.c_str());
    sb.Append(prefix).Append("}\n");
}

void ASTEnumType::EmitCppWriteVar(const std::string &parcelName, const std::string &name, StringBuilder &sb,
    const std::string &prefix, unsigned int innerLevel) const
{
    sb.Append(prefix).AppendFormat("if (!%s.WriteUint64(static_cast<uint64_t>(%s))) {\n", parcelName.c_str(),
        name.c_str());
    sb.Append(prefix + TAB).AppendFormat("HDF_LOGE(\"%%{public}s: write %s failed!\", __func__);\n", name.c_str());
    sb.Append(prefix + TAB).Append("return HDF_ERR_INVALID_PARAM;\n");
    sb.Append(prefix).Append("}\n");
}

void ASTEnumType::EmitCppReadVar(const std::string &parcelName, const std::string &name, StringBuilder &sb,
    const std::string &prefix, bool initVariable, unsigned int innerLevel) const
{
    std::string tmpVarName = "enumTmp";
    if (initVariable) {
        sb.Append(prefix).AppendFormat("%s %s = static_cast<%s>(0);\n", EmitCppType().c_str(), name.c_str(),
            EmitCType().c_str());
    }
    sb.Append(prefix).Append("{\n");
    sb.Append(prefix + TAB).AppendFormat("uint64_t %s = 0;\n", tmpVarName.c_str());
    sb.Append(prefix + TAB).AppendFormat("if (!%s.ReadUint64(%s)) {\n", parcelName.c_str(), tmpVarName.c_str());
    sb.Append(prefix + TAB + TAB).AppendFormat("HDF_LOGE(\"%%{public}s: write %s failed!\", __func__);\n",
        name.c_str());
    sb.Append(prefix + TAB + TAB).Append("return HDF_ERR_INVALID_PARAM;\n");
    sb.Append(prefix + TAB).Append("}\n");
    sb.Append(prefix + TAB).AppendFormat("%s = static_cast<%s>(%s);\n", name.c_str(), EmitCType().c_str(),
        tmpVarName.c_str());
    sb.Append(prefix).Append("}\n");
}

void ASTEnumType::EmitCMarshalling(const std::string &name, StringBuilder &sb, const std::string &prefix) const
{
    sb.Append(prefix).AppendFormat("if (!HdfSbufWriteUint64(data, (uint64_t)%s)) {\n", name.c_str());
    sb.Append(prefix + TAB).AppendFormat("HDF_LOGE(\"%%{public}s: write %s failed!\", __func__);\n", name.c_str());
    sb.Append(prefix + TAB).Append("return false;\n");
    sb.Append(prefix).Append("}\n");
}

void ASTEnumType::EmitCUnMarshalling(const std::string &name, const std::string &gotoLabel, StringBuilder &sb,
    const std::string &prefix, std::vector<std::string> &freeObjStatements) const
{
    std::string tmpVarName = "enumTmp";
    sb.Append(prefix).Append("{\n");
    sb.Append(prefix + TAB).AppendFormat("uint64_t %s = 0;\n", tmpVarName.c_str());
    sb.Append(prefix + TAB).AppendFormat("if (!HdfSbufReadUint64(data, &%s)) {\n", tmpVarName.c_str());
    sb.Append(prefix + TAB + TAB).AppendFormat("HDF_LOGE(\"%%{public}s: read %s failed!\", __func__);\n", name.c_str());
    sb.Append(prefix + TAB + TAB).AppendFormat("goto %s;\n", gotoLabel.c_str());
    sb.Append(prefix + TAB).Append("}\n");
    sb.Append(prefix + TAB).AppendFormat("%s = (%s)%s;\n", name.c_str(), EmitCType().c_str(), tmpVarName.c_str());
    sb.Append(prefix).Append("}\n");
}

void ASTEnumType::EmitCppMarshalling(const std::string &parcelName, const std::string &name, StringBuilder &sb,
    const std::string &prefix, unsigned int innerLevel) const
{
    sb.Append(prefix).AppendFormat(
        "if (!%s.WriteUint64(static_cast<uint64_t>(%s))) {\n", parcelName.c_str(), name.c_str());
    sb.Append(prefix + TAB).AppendFormat("HDF_LOGE(\"%%{public}s: write %s failed!\", __func__);\n", name.c_str());
    sb.Append(prefix + TAB).Append("return false;\n");
    sb.Append(prefix).Append("}\n");
}

void ASTEnumType::EmitCppUnMarshalling(const std::string &parcelName, const std::string &name, StringBuilder &sb,
    const std::string &prefix, bool emitType, unsigned int innerLevel) const
{
    std::string tmpVarName = "enumTmp";
    if (emitType) {
        sb.Append(prefix).AppendFormat("%s %s = static_cast<%s>(0);\n", EmitCppType().c_str(), name.c_str(),
            EmitCType().c_str());
    }
    sb.Append(prefix).Append("{\n");
    sb.Append(prefix + TAB).AppendFormat("uint64_t %s = 0;\n", tmpVarName.c_str());
    sb.Append(prefix + TAB).AppendFormat("if (!%s.ReadUint64(%s)) {\n", parcelName.c_str(), tmpVarName.c_str());
    sb.Append(prefix + TAB + TAB).AppendFormat("HDF_LOGE(\"%%{public}s: write %s failed!\", __func__);\n",
        name.c_str());
    sb.Append(prefix + TAB + TAB).Append("return false;\n");
    sb.Append(prefix + TAB).Append("}\n");
    sb.Append(prefix + TAB).AppendFormat("%s = static_cast<%s>(%s);\n", name.c_str(), EmitCType().c_str(),
        tmpVarName.c_str());
    sb.Append(prefix).Append("}\n");
}
} // namespace HDI
} // namespace OHOS
