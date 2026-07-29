/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include "ast/ast_native_buffer_type.h"

namespace OHOS {
namespace HDI {
bool ASTNativeBufferType::IsNativeBufferType()
{
    return true;
}

std::string ASTNativeBufferType::ToString() const
{
    return "NativeBuffer";
}

TypeKind ASTNativeBufferType::GetTypeKind()
{
    return TypeKind::TYPE_NATIVE_BUFFER;
}

std::string ASTNativeBufferType::EmitCType(TypeMode mode) const
{
    switch (mode) {
        case TypeMode::NO_MODE:
            return "BufferHandle*";
        case TypeMode::PARAM_IN:
            return "const BufferHandle*";
        case TypeMode::PARAM_OUT:
            return "BufferHandle**";
        case TypeMode::LOCAL_VAR:
            return "BufferHandle*";
        default:
            return "unknow type";
    }
}

std::string ASTNativeBufferType::EmitCppType(TypeMode mode) const
{
    switch (mode) {
        case TypeMode::NO_MODE:
            return "sptr<NativeBuffer>";
        case TypeMode::PARAM_IN:
            return "const sptr<NativeBuffer>&";
        case TypeMode::PARAM_OUT:
            return "sptr<NativeBuffer>&";
        case TypeMode::LOCAL_VAR:
            return "sptr<NativeBuffer>";
        default:
            return "unknow type";
    }
}

void ASTNativeBufferType::EmitCWriteVar(const std::string &parcelName, const std::string &name,
    const std::string &ecName, const std::string &gotoLabel, StringBuilder &sb, const std::string &prefix) const
{
    sb.Append(prefix).AppendFormat("if (!HdfSbufWriteNativeBufferHandle(%s, %s)) {\n", parcelName.c_str(),
        name.c_str());
    sb.Append(prefix + TAB).AppendFormat("HDF_LOGE(\"%%{public}s: failed to write %s\", __func__);\n", name.c_str());
    sb.Append(prefix + TAB).Append("return HDF_ERR_INVALID_PARAM;\n");
    sb.Append(prefix).Append("}\n");
}

void ASTNativeBufferType::EmitCProxyReadVar(const std::string &parcelName, const std::string &name, bool isInnerType,
    const std::string &ecName, const std::string &gotoLabel, StringBuilder &sb, const std::string &prefix) const
{
    sb.Append(prefix);
    if (!isInnerType) {
        sb.Append("*");
    }
    sb.AppendFormat("%s = HdfSbufReadNativeBufferHandle(%s);\n", name.c_str(), parcelName.c_str());
    sb.Append(prefix).AppendFormat("if (%s == NULL) {\n", name.c_str());
    sb.Append(prefix + TAB).AppendFormat("HDF_LOGE(\"%%{public}s: read %s failed!\", __func__);\n", name.c_str());
    sb.Append(prefix + TAB).AppendFormat("%s = HDF_ERR_INVALID_PARAM;\n", ecName.c_str());
    sb.Append(prefix + TAB).AppendFormat("goto %s;\n", gotoLabel.c_str());
    sb.Append(prefix).Append("}\n");
}

void ASTNativeBufferType::EmitCStubReadVar(const std::string &parcelName, const std::string &name,
    const std::string &ecName, const std::string &gotoLabel, StringBuilder &sb, const std::string &prefix) const
{
    sb.Append(prefix).AppendFormat("%s = HdfSbufReadNativeBufferHandle(%s);\n", name.c_str(), parcelName.c_str());
    sb.Append(prefix).AppendFormat("if (%s == NULL) {\n", name.c_str());
    sb.Append(prefix + TAB).AppendFormat("HDF_LOGE(\"%%{public}s: read %s failed!\", __func__);\n", name.c_str());
    sb.Append(prefix + TAB).AppendFormat("%s = HDF_ERR_INVALID_PARAM;\n", ecName.c_str());
    sb.Append(prefix + TAB).AppendFormat("goto %s;\n", gotoLabel.c_str());
    sb.Append(prefix).Append("}\n");
}

void ASTNativeBufferType::EmitCppWriteVar(const std::string &parcelName, const std::string &name, StringBuilder &sb,
    const std::string &prefix, unsigned int innerLevel) const
{
    sb.Append(prefix).AppendFormat("if (!%s.WriteStrongParcelable(%s)) {\n", parcelName.c_str(), name.c_str());
    sb.Append(prefix + TAB).AppendFormat("HDF_LOGE(\"%%{public}s: failed to write %s\", __func__);\n", name.c_str());
    sb.Append(prefix + TAB).Append("return HDF_ERR_INVALID_PARAM;\n");
    sb.Append(prefix).Append("}\n");
}

void ASTNativeBufferType::EmitCppReadVar(const std::string &parcelName, const std::string &name, StringBuilder &sb,
    const std::string &prefix, bool initVariable, unsigned int innerLevel) const
{
    sb.Append(prefix);
    if (initVariable) {
        sb.AppendFormat("%s ", EmitCppType().c_str());
    }
    sb.AppendFormat("%s = %s.ReadStrongParcelable<NativeBuffer>();\n", name.c_str(), parcelName.c_str());
}

void ASTNativeBufferType::EmitCMarshalling(const std::string &name, StringBuilder &sb, const std::string &prefix) const
{
    sb.Append(prefix).AppendFormat("if (!HdfSbufWriteNativeBufferHandle(data, %s)) {\n", name.c_str());
    sb.Append(prefix + TAB).AppendFormat("HDF_LOGE(\"%%{public}s: write %s failed!\", __func__);\n", name.c_str());
    sb.Append(prefix + TAB).Append("return false;\n");
    sb.Append(prefix).Append("}\n");
}

void ASTNativeBufferType::EmitCUnMarshalling(const std::string &name, const std::string &gotoLabel, StringBuilder &sb,
    const std::string &prefix, std::vector<std::string> &freeObjStatements) const
{
    sb.Append(prefix).AppendFormat("%s = HdfSbufReadNativeBufferHandle(data);\n", name.c_str());
    sb.Append(prefix).AppendFormat("if (%s == NULL) {\n", name.c_str());
    sb.Append(prefix + TAB).AppendFormat("HDF_LOGE(\"%%{public}s: read %s failed!\", __func__);\n", name.c_str());
    EmitFreeStatements(freeObjStatements, sb, prefix + TAB);
    sb.Append(prefix + TAB).AppendFormat("goto %s;\n", gotoLabel.c_str());
    sb.Append(prefix).Append("}\n");
}

void ASTNativeBufferType::EmitCppMarshalling(const std::string &parcelName, const std::string &name, StringBuilder &sb,
    const std::string &prefix, unsigned int innerLevel) const
{
    sb.Append(prefix).AppendFormat("if (!%s.WriteStrongParcelable(%s)) {\n", parcelName.c_str(), name.c_str());
    sb.Append(prefix + TAB).AppendFormat("HDF_LOGE(\"%%{public}s: failed to write %s\", __func__);\n", name.c_str());
    sb.Append(prefix + TAB).Append("return false;\n");
    sb.Append(prefix).Append("}\n");
}

void ASTNativeBufferType::EmitCppUnMarshalling(const std::string &parcelName, const std::string &name,
    StringBuilder &sb, const std::string &prefix, bool emitType, unsigned int innerLevel) const
{
    sb.Append(prefix);
    if (emitType) {
        sb.AppendFormat("%s ", EmitCppType().c_str());
    }
    sb.AppendFormat("%s = %s.ReadStrongParcelable<NativeBuffer>();\n", name.c_str(), parcelName.c_str());
}

void ASTNativeBufferType::EmitMemoryRecycle(
    const std::string &name, bool isClient, bool ownership, StringBuilder &sb, const std::string &prefix) const
{
    sb.Append(prefix).AppendFormat("if (%s != NULL) {\n", name.c_str());
    sb.Append(prefix + TAB).AppendFormat("FreeNativeBufferHandle(%s);\n", name.c_str());
    sb.Append(prefix + TAB).AppendFormat("%s = NULL;\n", name.c_str());
    sb.Append(prefix).Append("}\n");
}
} // namespace HDI
} // namespace OHOS