/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include "ast/ast_string_type.h"

namespace OHOS {
namespace HDI {
bool ASTStringType::IsStringType()
{
    return true;
}

std::string ASTStringType::ToString() const
{
    return "std::string";
}

TypeKind ASTStringType::GetTypeKind()
{
    return TypeKind::TYPE_STRING;
}

std::string ASTStringType::EmitCType(TypeMode mode) const
{
    switch (mode) {
        case TypeMode::NO_MODE:
            return "char*";
        case TypeMode::PARAM_IN:
            return "const char*";
        case TypeMode::PARAM_OUT:
            return "char*";
        case TypeMode::LOCAL_VAR:
            return "char*";
        default:
            return "unknow type";
    }
}

std::string ASTStringType::EmitCppType(TypeMode mode) const
{
    switch (mode) {
        case TypeMode::NO_MODE:
            return "std::string";
        case TypeMode::PARAM_IN:
            return "const std::string&";
        case TypeMode::PARAM_OUT:
            return "std::string&";
        case TypeMode::LOCAL_VAR:
            return "std::string";
        default:
            return "unknow type";
    }
}

std::string ASTStringType::EmitJavaType(TypeMode mode, bool isInnerType) const
{
    return "std::string";
}

void ASTStringType::EmitCWriteVar(const std::string &parcelName, const std::string &name, const std::string &ecName,
    const std::string &gotoLabel, StringBuilder &sb, const std::string &prefix) const
{
    sb.Append(prefix).AppendFormat("if (!HdfSbufWriteString(%s, %s)) {\n", parcelName.c_str(), name.c_str());
    sb.Append(prefix + TAB).AppendFormat("HDF_LOGE(\"%%{public}s: write %s failed!\", __func__);\n", name.c_str());
    sb.Append(prefix + TAB).AppendFormat("%s = HDF_ERR_INVALID_PARAM;\n", ecName.c_str());
    sb.Append(prefix + TAB).AppendFormat("goto %s;\n", gotoLabel.c_str());
    sb.Append(prefix).Append("}\n");
}

void ASTStringType::EmitCProxyWriteOutVar(const std::string &parcelName, const std::string &name,
    const std::string &ecName, const std::string &gotoLabel, StringBuilder &sb, const std::string &prefix) const
{
    std::string lenName = StringHelper::Format("%sLen", name.c_str());
    sb.Append(prefix).AppendFormat("if (%s == NULL || %s == 0) {\n", name.c_str(), lenName.c_str());
    sb.Append(prefix + TAB).AppendFormat("HDF_LOGE(\"%%{public}s: %s is invalid\", __func__);\n", name.c_str());
    sb.Append(prefix + TAB).AppendFormat("%s = HDF_ERR_INVALID_PARAM;\n", ecName.c_str());
    sb.Append(prefix + TAB).AppendFormat("goto %s;\n", gotoLabel.c_str());
    sb.Append(prefix).Append("}\n\n");
    sb.Append(prefix).AppendFormat("if (!HdfSbufWriteUint32(%s, %s)) {\n", parcelName.c_str(), lenName.c_str());
    sb.Append(prefix + TAB).AppendFormat("HDF_LOGE(\"%%{public}s: write %s failed!\", __func__);\n", lenName.c_str());
    sb.Append(prefix + TAB).AppendFormat("%s = HDF_ERR_INVALID_PARAM;\n", ecName.c_str());
    sb.Append(prefix + TAB).AppendFormat("goto %s;\n", gotoLabel.c_str());
    sb.Append(prefix).Append("}\n");
}

void ASTStringType::EmitCProxyReadVar(const std::string &parcelName, const std::string &name, bool isInnerType,
    const std::string &ecName, const std::string &gotoLabel, StringBuilder &sb, const std::string &prefix) const
{
    sb.Append(prefix).AppendFormat("const char *%s = HdfSbufReadString(%s);\n", name.c_str(), parcelName.c_str());
    sb.Append(prefix).AppendFormat("if (%s == NULL) {\n", name.c_str());
    sb.Append(prefix + TAB).AppendFormat("HDF_LOGE(\"%%{public}s: read %s failed!\", __func__);\n", name.c_str());
    sb.Append(prefix + TAB).AppendFormat("%s = HDF_ERR_INVALID_PARAM;\n", ecName.c_str());
    sb.Append(prefix + TAB).AppendFormat("goto %s;\n", gotoLabel.c_str());
    sb.Append(prefix).Append("}\n");
}

void ASTStringType::EmitCStubReadVar(const std::string &parcelName, const std::string &name, const std::string &ecName,
    const std::string &gotoLabel, StringBuilder &sb, const std::string &prefix) const
{
    sb.Append(prefix).AppendFormat("const char *%s = HdfSbufReadString(%s);\n", name.c_str(), parcelName.c_str());
    sb.Append(prefix).AppendFormat("if (%s == NULL) {\n", name.c_str());
    sb.Append(prefix + TAB).AppendFormat("HDF_LOGE(\"%%{public}s: read %s failed!\", __func__);\n", name.c_str());
    sb.Append(prefix + TAB).AppendFormat("%s = HDF_ERR_INVALID_PARAM;\n", ecName.c_str());
    sb.Append(prefix + TAB).AppendFormat("goto %s;\n", gotoLabel.c_str());
    sb.Append(prefix).Append("}\n");
}

void ASTStringType::EmitCStubReadOutVar(const std::string &buffSizeName, const std::string &memFlagName,
    const std::string &parcelName, const std::string &name, const std::string &ecName, const std::string &gotoLabel,
    StringBuilder &sb, const std::string &prefix) const
{
    std::string lenName = StringHelper::Format("%sLen", name.c_str());

    sb.Append(prefix).AppendFormat("if (%s) {\n", memFlagName.c_str());
    sb.Append(prefix + TAB).AppendFormat("if (!HdfSbufReadUint32(%s, &%s)) {\n", parcelName.c_str(), lenName.c_str());
    sb.Append(prefix + TAB + TAB)
        .AppendFormat("HDF_LOGE(\"%%{public}s: read %s size failed!\", __func__);\n", name.c_str());
    sb.Append(prefix + TAB + TAB).AppendFormat("%s = HDF_ERR_INVALID_PARAM;\n", ecName.c_str());
    sb.Append(prefix + TAB + TAB).AppendFormat("goto %s;\n", gotoLabel.c_str());
    sb.Append(prefix + TAB).Append("}\n\n");
    sb.Append(prefix + TAB).AppendFormat("%s(%s, >, %s / sizeof(char), %s, HDF_ERR_INVALID_PARAM, %s);\n",
        CHECK_VALUE_RET_GOTO_MACRO, lenName.c_str(), MAX_BUFF_SIZE_MACRO, ecName.c_str(), gotoLabel.c_str());

    sb.Append(prefix + TAB).AppendFormat("if (%s > 0) {\n", lenName.c_str());
    sb.Append(prefix + TAB + TAB)
        .AppendFormat("%s = (%s)OsalMemCalloc(%s);\n", name.c_str(), EmitCType().c_str(), lenName.c_str());
    sb.Append(prefix + TAB + TAB).AppendFormat("if (%s == NULL) {\n", name.c_str());
    sb.Append(prefix + TAB + TAB + TAB)
        .AppendFormat("HDF_LOGE(\"%%{public}s: malloc %s failed\", __func__);\n", name.c_str());
    sb.Append(prefix + TAB + TAB + TAB).AppendFormat("%s = HDF_ERR_MALLOC_FAIL;\n", ecName.c_str());
    sb.Append(prefix + TAB + TAB + TAB).AppendFormat("goto %s;\n", gotoLabel.c_str());
    sb.Append(prefix + TAB + TAB).Append("}\n");
    sb.Append(prefix + TAB).Append("}\n");
    sb.Append(prefix).Append("} else {\n");
    sb.Append(prefix + TAB)
        .AppendFormat("%s = (%s)OsalMemCalloc(%s);\n", name.c_str(), EmitCType().c_str(), buffSizeName.c_str());
    sb.Append(prefix + TAB).AppendFormat("if (%s == NULL) {\n", name.c_str());
    sb.Append(prefix + TAB + TAB)
        .AppendFormat("HDF_LOGE(\"%%{public}s: malloc %s failed\", __func__);\n", name.c_str());
    sb.Append(prefix + TAB + TAB).AppendFormat("%s = HDF_ERR_MALLOC_FAIL;\n", ecName.c_str());
    sb.Append(prefix + TAB + TAB).AppendFormat("goto %s;\n", gotoLabel.c_str());
    sb.Append(prefix + TAB).Append("}\n");

    sb.Append(prefix + TAB).AppendFormat("%sLen = %s;\n", name.c_str(), buffSizeName.c_str());
    sb.Append(prefix).Append("}\n\n");
}

void ASTStringType::EmitCppWriteVar(const std::string &parcelName, const std::string &name, StringBuilder &sb,
    const std::string &prefix, unsigned int innerLevel) const
{
    sb.Append(prefix).AppendFormat("if (!%s.WriteCString(%s.c_str())) {\n", parcelName.c_str(), name.c_str());
    sb.Append(prefix + TAB).AppendFormat("HDF_LOGE(\"%%{public}s: write %s failed!\", __func__);\n", name.c_str());
    sb.Append(prefix + TAB).Append("return HDF_ERR_INVALID_PARAM;\n");
    sb.Append(prefix).Append("}\n");
}

void ASTStringType::EmitCppReadVar(const std::string &parcelName, const std::string &name, StringBuilder &sb,
    const std::string &prefix, bool initVariable, unsigned int innerLevel) const
{
    sb.Append(prefix).AppendFormat("const char* %sCp = %s.ReadCString();\n", name.c_str(), parcelName.c_str());
    sb.Append(prefix).AppendFormat("if (%sCp == nullptr) {\n", name.c_str());
    sb.Append(prefix + TAB).AppendFormat("HDF_LOGE(\"%%{public}s: read %s failed\", __func__);\n", name.c_str());
    sb.Append(prefix + TAB).Append("return HDF_ERR_INVALID_PARAM;\n");
    sb.Append(prefix).Append("}\n");
    if (initVariable) {
        sb.Append(prefix).AppendFormat("%s %s = %sCp;\n", EmitCppType().c_str(), name.c_str(), name.c_str());
    } else {
        sb.Append(prefix).AppendFormat("%s = %sCp;\n", name.c_str(), name.c_str());
    }
}

void ASTStringType::EmitCMarshalling(const std::string &name, StringBuilder &sb, const std::string &prefix) const
{
    sb.Append(prefix).AppendFormat("if (!HdfSbufWriteString(data, %s)) {\n", name.c_str());
    sb.Append(prefix + TAB).AppendFormat("HDF_LOGE(\"%%{public}s: write %s failed!\", __func__);\n", name.c_str());
    sb.Append(prefix + TAB).Append("return false;\n");
    sb.Append(prefix).Append("}\n");
}

void ASTStringType::EmitCUnMarshalling(const std::string &name, const std::string &gotoLabel, StringBuilder &sb,
    const std::string &prefix, std::vector<std::string> &freeObjStatements) const
{
    sb.Append(prefix).AppendFormat("const char *%s = HdfSbufReadString(data);\n", name.c_str());
    sb.Append(prefix).AppendFormat("if (%s == NULL) {\n", name.c_str());
    sb.Append(prefix + TAB).AppendFormat("HDF_LOGE(\"%%{public}s: read %s failed!\", __func__);\n", name.c_str());
    EmitFreeStatements(freeObjStatements, sb, prefix + TAB);
    sb.Append(prefix + TAB).AppendFormat("goto %s;\n", gotoLabel.c_str());
    sb.Append(prefix).Append("}\n");
}

void ASTStringType::EmitCppMarshalling(const std::string &parcelName, const std::string &name, StringBuilder &sb,
    const std::string &prefix, unsigned int innerLevel) const
{
    sb.Append(prefix).AppendFormat("if (!%s.WriteCString(%s.c_str())) {\n", parcelName.c_str(), name.c_str());
    sb.Append(prefix + TAB).AppendFormat("HDF_LOGE(\"%%{public}s: write %s failed!\", __func__);\n", name.c_str());
    sb.Append(prefix + TAB).Append("return false;\n");
    sb.Append(prefix).Append("}\n");
}

void ASTStringType::EmitCppUnMarshalling(const std::string &parcelName, const std::string &name, StringBuilder &sb,
    const std::string &prefix, bool emitType, unsigned int innerLevel) const
{
    sb.Append(prefix).AppendFormat("const char* %s = %s.ReadCString();\n", name.c_str(), parcelName.c_str());
    sb.Append(prefix).AppendFormat("if (%s == nullptr) {\n", name.c_str());
    sb.Append(prefix + TAB).AppendFormat("HDF_LOGE(\"%%{public}s: read %s failed\", __func__);\n", name.c_str());
    sb.Append(prefix + TAB).Append("return false;\n");
    sb.Append(prefix).Append("}\n");
}

void ASTStringType::EmitMemoryRecycle(
    const std::string &name, bool isClient, bool ownership, StringBuilder &sb, const std::string &prefix) const
{
    sb.Append(prefix).AppendFormat("if (%s != NULL) {\n", name.c_str());
    sb.Append(prefix + TAB).AppendFormat("OsalMemFree(%s);\n", name.c_str());
    sb.Append(prefix + TAB).AppendFormat("%s = NULL;\n", name.c_str());
    sb.Append(prefix).Append("}\n");
}

void ASTStringType::EmitJavaWriteVar(
    const std::string &parcelName, const std::string &name, StringBuilder &sb, const std::string &prefix) const
{
    sb.Append(prefix).AppendFormat("%s.writeString(%s);\n", parcelName.c_str(), name.c_str());
}

void ASTStringType::EmitJavaReadVar(
    const std::string &parcelName, const std::string &name, StringBuilder &sb, const std::string &prefix) const
{
    sb.Append(prefix).AppendFormat("%s = %s.readString();\n", name.c_str(), parcelName.c_str());
}

void ASTStringType::EmitJavaReadInnerVar(const std::string &parcelName, const std::string &name, bool isInner,
    StringBuilder &sb, const std::string &prefix) const
{
    sb.Append(prefix).AppendFormat(
        "%s %s = %s.readString();\n", EmitJavaType(TypeMode::NO_MODE).c_str(), name.c_str(), parcelName.c_str());
}
} // namespace HDI
} // namespace OHOS