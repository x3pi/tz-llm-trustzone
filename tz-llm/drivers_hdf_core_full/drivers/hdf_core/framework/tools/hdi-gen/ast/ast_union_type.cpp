/*
 * Copyright (c) 2021-2022 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include "ast/ast_union_type.h"
#include "util/options.h"
#include "util/string_builder.h"

namespace OHOS {
namespace HDI {
void ASTUnionType::AddMember(const AutoPtr<ASTType> &typeName, std::string name)
{
    for (auto it : members_) {
        if (std::get<0>(it) == name) {
            return;
        }
    }
    members_.push_back(std::make_tuple(name, typeName));
}

bool ASTUnionType::IsUnionType()
{
    return true;
}

std::string ASTUnionType::Dump(const std::string &prefix)
{
    StringBuilder sb;
    sb.Append(prefix).Append(attr_->Dump(prefix)).Append(" ");
    sb.AppendFormat("union %s {\n", name_.c_str());
    if (members_.size() > 0) {
        for (auto it : members_) {
            sb.Append(prefix + "  ");
            sb.AppendFormat("%s %s;\n", std::get<1>(it)->ToString().c_str(), std::get<0>(it).c_str());
        }
    }
    sb.Append(prefix).Append("};\n");
    return sb.ToString();
}

TypeKind ASTUnionType::GetTypeKind()
{
    return TypeKind::TYPE_UNION;
}

std::string ASTUnionType::EmitCType(TypeMode mode) const
{
    switch (mode) {
        case TypeMode::NO_MODE:
            return StringHelper::Format("union %s", name_.c_str());
        case TypeMode::PARAM_IN:
            return StringHelper::Format("const union %s*", name_.c_str());
        case TypeMode::PARAM_OUT:
            return StringHelper::Format("union %s*", name_.c_str());
        case TypeMode::LOCAL_VAR:
            return StringHelper::Format("union %s*", name_.c_str());
        default:
            return "unknow type";
    }
}

std::string ASTUnionType::EmitCppType(TypeMode mode) const
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

std::string ASTUnionType::EmitJavaType(TypeMode mode, bool isInnerType) const
{
    // unsupported type
    return "/";
}

std::string ASTUnionType::EmitCTypeDecl() const
{
    StringBuilder sb;
    sb.AppendFormat("union %s {\n", name_.c_str());

    for (auto it : members_) {
        AutoPtr<ASTType> member = std::get<1>(it);
        std::string memberName = std::get<0>(it);
        sb.Append(TAB).AppendFormat("%s %s;\n", member->EmitCType().c_str(), memberName.c_str());
        if (member->GetTypeKind() == TypeKind::TYPE_ARRAY || member->GetTypeKind() == TypeKind::TYPE_LIST) {
            sb.Append(TAB).AppendFormat("uint32_t %sLen;\n", memberName.c_str());
        }
    }

    sb.Append("}  __attribute__ ((aligned(8)));");
    return sb.ToString();
}

std::string ASTUnionType::EmitCppTypeDecl() const
{
    StringBuilder sb;
    sb.AppendFormat("union %s {\n", name_.c_str());

    for (auto it : members_) {
        AutoPtr<ASTType> member = std::get<1>(it);
        std::string memberName = std::get<0>(it);
        sb.Append(TAB).AppendFormat("%s %s;\n", member->EmitCppType().c_str(), memberName.c_str());
    }

    sb.Append("}  __attribute__ ((aligned(8)));");
    return sb.ToString();
}

std::string ASTUnionType::EmitJavaTypeDecl() const
{
    StringBuilder sb;

    return sb.ToString();
}

void ASTUnionType::EmitCWriteVar(const std::string &parcelName, const std::string &name, const std::string &ecName,
    const std::string &gotoLabel, StringBuilder &sb, const std::string &prefix) const
{
    if (Options::GetInstance().DoGenerateKernelCode()) {
        sb.Append(prefix).AppendFormat("if (!HdfSbufWriteBuffer(%s, (const void *)%s, sizeof(%s))) {\n",
            parcelName.c_str(), name.c_str(), EmitCType().c_str());
    } else {
        sb.Append(prefix).AppendFormat("if (!HdfSbufWriteUnpadBuffer(%s, (const uint8_t *)%s, sizeof(%s))) {\n",
            parcelName.c_str(), name.c_str(), EmitCType().c_str());
    }
    sb.Append(prefix + TAB).AppendFormat("HDF_LOGE(\"%%{public}s: write %s failed!\", __func__);\n", name.c_str());
    sb.Append(prefix + TAB).AppendFormat("%s = HDF_ERR_INVALID_PARAM;\n", ecName.c_str());
    sb.Append(prefix + TAB).AppendFormat("goto %s;\n", gotoLabel.c_str());
    sb.Append(prefix).Append("}\n");
}

void ASTUnionType::EmitCProxyReadVar(const std::string &parcelName, const std::string &name, bool isInnerType,
    const std::string &ecName, const std::string &gotoLabel, StringBuilder &sb, const std::string &prefix) const
{
    if (Options::GetInstance().DoGenerateKernelCode()) {
        sb.Append(prefix).AppendFormat("%s *%s = NULL;\n", EmitCType().c_str(), name.c_str());
        sb.Append(prefix).Append("uint32_t len = 0;\n");
        sb.Append(prefix).AppendFormat(
            "if (!HdfSbufReadBuffer(%s, (const void **)&%s, &len)) {\n", parcelName.c_str(), name.c_str());
        sb.Append(prefix + TAB).AppendFormat("HDF_LOGE(\"%%{public}s: read %s failed!\", __func__);\n", name.c_str());
        sb.Append(prefix + TAB).AppendFormat("%s = HDF_ERR_INVALID_PARAM;\n", ecName.c_str());
        sb.Append(prefix + TAB).AppendFormat("goto %s;\n", gotoLabel.c_str());
        sb.Append(prefix).Append("}\n\n");
        sb.Append(prefix).AppendFormat("if (%s == NULL || sizeof(%s) != len) {\n", name.c_str(), EmitCType().c_str());
    } else {
        sb.Append(prefix).AppendFormat("const %s *%s = (%s *)HdfSbufReadUnpadBuffer(%s, sizeof(%s));\n",
            EmitCType().c_str(), name.c_str(), EmitCType().c_str(), parcelName.c_str(), EmitCType().c_str());
        sb.Append(prefix).AppendFormat("if (%s == NULL) {\n", name.c_str());
    }

    sb.Append(prefix + TAB).AppendFormat("HDF_LOGE(\"%%{public}s: read %s failed!\", __func__);\n", name.c_str());
    sb.Append(prefix + TAB).AppendFormat("%s = HDF_ERR_INVALID_PARAM;\n", ecName.c_str());
    sb.Append(prefix + TAB).AppendFormat("goto %s;\n", gotoLabel.c_str());
    sb.Append(prefix).Append("}\n");
}

void ASTUnionType::EmitCStubReadVar(const std::string &parcelName, const std::string &name, const std::string &ecName,
    const std::string &gotoLabel, StringBuilder &sb, const std::string &prefix) const
{
    if (Options::GetInstance().DoGenerateKernelCode()) {
        sb.Append(prefix).AppendFormat("%s *%s = NULL;\n", EmitCType().c_str(), name.c_str());
        sb.Append(prefix).Append("uint32_t len = 0;\n");
        sb.Append(prefix).AppendFormat(
            "if (!HdfSbufReadBuffer(%s, (const void **)&%s, &len)) {\n", parcelName.c_str(), name.c_str());
        sb.Append(prefix + TAB).AppendFormat("HDF_LOGE(\"%%{public}s: read %s failed!\", __func__);\n", name.c_str());
        sb.Append(prefix + TAB).AppendFormat("%s = HDF_ERR_INVALID_PARAM;\n", ecName.c_str());
        sb.Append(prefix + TAB).AppendFormat("goto %s;\n", gotoLabel.c_str());
        sb.Append(prefix).Append("}\n\n");
        sb.Append(prefix).AppendFormat("if (%s == NULL || sizeof(%s) != len) {\n", name.c_str(), EmitCType().c_str());
    } else {
        sb.Append(prefix).AppendFormat("const %s *%s = (%s *)HdfSbufReadUnpadBuffer(%s, sizeof(%s));\n",
            EmitCType().c_str(), name.c_str(), EmitCType().c_str(), parcelName.c_str(), EmitCType().c_str());
        sb.Append(prefix).AppendFormat("if (%s == NULL) {\n", name.c_str());
    }

    sb.Append(prefix + TAB).AppendFormat("HDF_LOGE(\"%%{public}s: read %s failed!\", __func__);\n", name.c_str());
    sb.Append(prefix + TAB).AppendFormat("%s = HDF_ERR_INVALID_PARAM;\n", ecName.c_str());
    sb.Append(prefix + TAB).AppendFormat("goto %s;\n", gotoLabel.c_str());
    sb.Append(prefix).Append("}\n");
}

void ASTUnionType::EmitCppWriteVar(const std::string &parcelName, const std::string &name, StringBuilder &sb,
    const std::string &prefix, unsigned int innerLevel) const
{
    sb.Append(prefix).AppendFormat("if (!%s.WriteUnpadBuffer((const uint8_t *)&%s, sizeof(%s))) {\n",
        parcelName.c_str(), name.c_str(), EmitCppType().c_str());
    sb.Append(prefix + TAB).AppendFormat("HDF_LOGE(\"%%{public}s: write %s failed!\", __func__);\n", name.c_str());
    sb.Append(prefix + TAB).Append("return HDF_ERR_INVALID_PARAM;\n");
    sb.Append(prefix).Append("}\n");
}

void ASTUnionType::EmitCppReadVar(const std::string &parcelName, const std::string &name, StringBuilder &sb,
    const std::string &prefix, bool initVariable, unsigned int innerLevel) const
{
    if (initVariable) {
        sb.Append(prefix).AppendFormat("%s %s;\n", EmitCppType().c_str(), name.c_str());
    }
    sb.Append(prefix).AppendFormat("const %s *%sCp = reinterpret_cast<const %s *>(%s.ReadUnpadBuffer(sizeof(%s)));\n",
        EmitCppType().c_str(), name.c_str(), EmitCppType().c_str(), parcelName.c_str(), EmitCppType().c_str());
    sb.Append(prefix).AppendFormat("if (%sCp == nullptr) {\n", name.c_str());
    sb.Append(prefix + TAB).AppendFormat("HDF_LOGE(\"%%{public}s: read %s failed!\", __func__);\n", name.c_str());
    sb.Append(prefix + TAB).Append("return HDF_ERR_INVALID_PARAM;\n");
    sb.Append(prefix).Append("}\n\n");
    sb.Append(prefix).AppendFormat("if (memcpy_s(&%s, sizeof(%s), %sCp, sizeof(%s)) != EOK) {\n", name.c_str(),
        EmitCppType().c_str(), name.c_str(), EmitCppType().c_str());
    sb.Append(prefix + TAB).AppendFormat("HDF_LOGE(\"%%{public}s: failed to memcpy %s\", __func__);\n", name.c_str());
    sb.Append(prefix + TAB).Append("return HDF_ERR_INVALID_PARAM;\n");
    sb.Append(prefix).Append("}\n");
}

void ASTUnionType::EmitCMarshalling(const std::string &name, StringBuilder &sb, const std::string &prefix) const
{
    if (Options::GetInstance().DoGenerateKernelCode()) {
        sb.Append(prefix).AppendFormat(
            "if (!HdfSbufWriteBuffer(data, (const void *)&%s, sizeof(%s))) {\n", name.c_str(), EmitCType().c_str());
    } else {
        sb.Append(prefix).AppendFormat("if (!HdfSbufWriteUnpadBuffer(data, (const uint8_t *)&%s, sizeof(%s))) {\n",
            name.c_str(), EmitCType().c_str());
    }
    sb.Append(prefix + TAB).AppendFormat("HDF_LOGE(\"%%{public}s: write %s failed!\", __func__);\n", name.c_str());
    sb.Append(prefix + TAB).Append("return false;\n");
    sb.Append(prefix).Append("}\n");
}

void ASTUnionType::EmitCUnMarshalling(const std::string &name, const std::string &gotoLabel, StringBuilder &sb,
    const std::string &prefix, std::vector<std::string> &freeObjStatements) const
{
    if (Options::GetInstance().DoGenerateKernelCode()) {
        sb.Append(prefix).AppendFormat("%s *%s = NULL;\n", EmitCType().c_str(), name.c_str());
        sb.Append(prefix).Append("uint32_t len = 0;\n");
        sb.Append(prefix).AppendFormat("if (!HdfSbufReadBuffer(data, (const void **)&%s, &len)) {\n", name.c_str());
        sb.Append(prefix + TAB).AppendFormat("HDF_LOGE(\"%%{public}s: read %s failed!\", __func__);\n", name.c_str());
        sb.Append(prefix + TAB).AppendFormat("goto %s;\n", gotoLabel.c_str());
        sb.Append(prefix).Append("}\n\n");
        sb.Append(prefix).AppendFormat("if (%s == NULL || sizeof(%s) != len) {\n", name.c_str(), EmitCType().c_str());
    } else {
        sb.Append(prefix).AppendFormat("const %s *%s = (const %s *)HdfSbufReadUnpadBuffer(data, sizeof(%s));\n",
            EmitCType().c_str(), name.c_str(), EmitCType().c_str(), EmitCType().c_str());
        sb.Append(prefix).AppendFormat("if (%s == NULL) {\n", name.c_str());
    }

    sb.Append(prefix + TAB).AppendFormat("HDF_LOGE(\"%%{public}s: read %s failed!\", __func__);\n", name.c_str());
    sb.Append(prefix + TAB).AppendFormat("goto %s;\n", gotoLabel.c_str());
    sb.Append(prefix).Append("}\n");
}

void ASTUnionType::EmitCppMarshalling(const std::string &parcelName, const std::string &name, StringBuilder &sb,
    const std::string &prefix, unsigned int innerLevel) const
{
    sb.Append(prefix).AppendFormat("if (!%s.WriteUnpadBuffer((const void*)&%s, sizeof(%s))) {\n", parcelName.c_str(),
        name.c_str(), EmitCppType().c_str());
    sb.Append(prefix + TAB).AppendFormat("HDF_LOGE(\"%%{public}s: write %s failed!\", __func__);\n", name.c_str());
    sb.Append(prefix + TAB).Append("return false;\n");
    sb.Append(prefix).Append("}\n");
}

void ASTUnionType::EmitCppUnMarshalling(const std::string &parcelName, const std::string &name, StringBuilder &sb,
    const std::string &prefix, bool emitType, unsigned int innerLevel) const
{
    sb.Append(prefix).AppendFormat("const %s *%s = reinterpret_cast<const %s*>(%s.ReadUnpadBuffer(sizeof(%s)));\n",
        EmitCppType().c_str(), name.c_str(), EmitCppType().c_str(), parcelName.c_str(), EmitCppType().c_str());
    sb.Append(prefix).AppendFormat("if (%s == nullptr) {\n", name.c_str());
    sb.Append(prefix + TAB).AppendFormat("HDF_LOGE(\"%%{public}s: read %s failed!\", __func__);\n", name.c_str());
    sb.Append(prefix + TAB).Append("return false;\n");
    sb.Append(prefix).Append("}\n");
}

void ASTUnionType::EmitMemoryRecycle(
    const std::string &name, bool isClient, bool ownership, StringBuilder &sb, const std::string &prefix) const
{
    if (ownership) {
        std::string varName = isClient ? StringHelper::Format("*%s", name.c_str()) : name;
        sb.Append(prefix).AppendFormat("if (%s != NULL) {\n", varName.c_str());
        sb.Append(prefix + TAB).AppendFormat("OsalMemFree(%s);\n", varName.c_str());
        sb.Append(prefix + TAB).AppendFormat("%s = NULL;\n", varName.c_str());
        sb.Append(prefix).Append("}\n");
    }
}
} // namespace HDI
} // namespace OHOS