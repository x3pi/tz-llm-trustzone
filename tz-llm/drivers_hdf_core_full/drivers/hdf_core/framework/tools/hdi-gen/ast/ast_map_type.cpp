/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include "ast/ast_map_type.h"

namespace OHOS {
namespace HDI {
bool ASTMapType::IsMapType()
{
    return true;
}

bool ASTMapType::HasInnerType(TypeKind innerTypeKind) const
{
    if (keyType_ != nullptr) {
        if (keyType_->GetTypeKind() == innerTypeKind) {
            return true;
        }

        if (keyType_->HasInnerType(innerTypeKind)) {
            return true;
        }
    }

    if (valueType_ != nullptr) {
        if (valueType_->GetTypeKind() == innerTypeKind) {
            return true;
        }

        if (valueType_->HasInnerType(innerTypeKind)) {
            return true;
        }
    }

    return false;
}

std::string ASTMapType::ToString() const
{
    return StringHelper::Format("Map<%s, %s>", keyType_->ToString().c_str(), valueType_->ToString().c_str());
}

TypeKind ASTMapType::GetTypeKind()
{
    return TypeKind::TYPE_MAP;
}

std::string ASTMapType::EmitCType(TypeMode mode) const
{
    // c language has no map type
    return "/";
}

std::string ASTMapType::EmitCppType(TypeMode mode) const
{
    switch (mode) {
        case TypeMode::NO_MODE:
            return StringHelper::Format(
                "std::map<%s, %s>", keyType_->EmitCppType().c_str(), valueType_->EmitCppType().c_str());
        case TypeMode::PARAM_IN:
            return StringHelper::Format(
                "const std::map<%s, %s>&", keyType_->EmitCppType().c_str(), valueType_->EmitCppType().c_str());
        case TypeMode::PARAM_OUT:
            return StringHelper::Format(
                "std::map<%s, %s>&", keyType_->EmitCppType().c_str(), valueType_->EmitCppType().c_str());
        case TypeMode::LOCAL_VAR:
            return StringHelper::Format(
                "std::map<%s, %s>", keyType_->EmitCppType().c_str(), valueType_->EmitCppType().c_str());
        default:
            return "unknow type";
    }
}

std::string ASTMapType::EmitJavaType(TypeMode mode, bool isInnerType) const
{
    return StringHelper::Format(
        "HashMap<%s, %s>", keyType_->EmitJavaType(mode, true).c_str(), valueType_->EmitJavaType(mode, true).c_str());
}

void ASTMapType::EmitCppWriteVar(const std::string &parcelName, const std::string &name, StringBuilder &sb,
    const std::string &prefix, unsigned int innerLevel) const
{
    sb.Append(prefix).AppendFormat("if (!%s.WriteUint32(%s.size())) {\n", parcelName.c_str(), name.c_str());
    sb.Append(prefix + TAB).AppendFormat("HDF_LOGE(\"%%{public}s: write %s failed!\", __func__);\n", name.c_str());
    sb.Append(prefix + TAB).Append("return HDF_ERR_INVALID_PARAM;\n");
    sb.Append(prefix).Append("}\n");
    std::string elementName = StringHelper::Format("it%d", innerLevel++);
    sb.Append(prefix).AppendFormat("for (auto %s : %s) {\n", elementName.c_str(), name.c_str());

    std::string keyName = StringHelper::Format("(%s.first)", elementName.c_str());
    std::string valueName = StringHelper::Format("(%s.second)", elementName.c_str());
    keyType_->EmitCppWriteVar(parcelName, keyName, sb, prefix + TAB, innerLevel);
    valueType_->EmitCppWriteVar(parcelName, valueName, sb, prefix + TAB, innerLevel);
    sb.Append(prefix).Append("}\n");
}

void ASTMapType::EmitCppReadVar(const std::string &parcelName, const std::string &name, StringBuilder &sb,
    const std::string &prefix, bool initVariable, unsigned int innerLevel) const
{
    if (initVariable) {
        sb.Append(prefix).AppendFormat("%s %s;\n", EmitCppType().c_str(), name.c_str());
    }
    sb.Append(prefix).AppendFormat("uint32_t %sSize = 0;\n", name.c_str());
    sb.Append(prefix).AppendFormat("if (!%s.ReadUint32(%sSize)) {\n", parcelName.c_str(), name.c_str());
    sb.Append(prefix + TAB).Append("HDF_LOGE(\"%{public}s: failed to read size\", __func__);\n");
    sb.Append(prefix + TAB).Append("return HDF_ERR_INVALID_PARAM;\n");
    sb.Append(prefix).Append("}\n\n");
    sb.Append(prefix).AppendFormat("for (uint32_t i = 0; i < %sSize; ++i) {\n", name.c_str());
    std::string keyName = StringHelper::Format("key%d", innerLevel);
    std::string valueName = StringHelper::Format("value%d", innerLevel);
    innerLevel++;
    keyType_->EmitCppReadVar(parcelName, keyName, sb, prefix + TAB, true, innerLevel);
    valueType_->EmitCppReadVar(parcelName, valueName, sb, prefix + TAB, true, innerLevel);
    sb.Append(prefix + TAB).AppendFormat("%s[%s] = %s;\n", name.c_str(), keyName.c_str(), valueName.c_str());
    sb.Append(prefix).Append("}\n");
}

void ASTMapType::EmitCppMarshalling(const std::string &parcelName, const std::string &name, StringBuilder &sb,
    const std::string &prefix, unsigned int innerLevel) const
{
    sb.Append(prefix).AppendFormat("if (!%s.WriteUint32(%s.size())) {\n", parcelName.c_str(), name.c_str());
    sb.Append(prefix + TAB).AppendFormat("HDF_LOGE(\"%%{public}s: write %s.size failed!\", __func__);\n", name.c_str());
    sb.Append(prefix + TAB).Append("return false;\n");
    sb.Append(prefix).Append("}\n");
    std::string elementName = StringHelper::Format("it%d", innerLevel++);
    sb.Append(prefix).AppendFormat("for (const auto& %s : %s) {\n", elementName.c_str(), name.c_str());

    std::string keyName = StringHelper::Format("(%s.first)", elementName.c_str());
    std::string valName = StringHelper::Format("(%s.second)", elementName.c_str());
    keyType_->EmitCppMarshalling(parcelName, keyName, sb, prefix + TAB, innerLevel);
    valueType_->EmitCppMarshalling(parcelName, valName, sb, prefix + TAB, innerLevel);
    sb.Append(prefix).Append("}\n");
}

void ASTMapType::EmitCppUnMarshalling(const std::string &parcelName, const std::string &name, StringBuilder &sb,
    const std::string &prefix, bool emitType, unsigned int innerLevel) const
{
    size_t index = name.find('.', 0);
    std::string memberName = (index == std::string::npos) ? name : StringHelper::SubStr(name, index + 1);
    if (emitType) {
        sb.Append(prefix).AppendFormat("%s %s;\n", EmitCppType().c_str(), memberName.c_str());
    }
    sb.Append(prefix).AppendFormat("uint32_t %sSize = 0;\n", memberName.c_str());
    sb.Append(prefix).AppendFormat("if (!%s.ReadUint32(%sSize)) {\n", parcelName.c_str(), memberName.c_str());
    sb.Append(prefix + TAB).Append("HDF_LOGE(\"%{public}s: failed to read size\", __func__);\n");
    sb.Append(prefix + TAB).Append("return false;\n");
    sb.Append(prefix).Append("}\n\n");
    sb.Append(prefix).AppendFormat("for (uint32_t i = 0; i < %sSize; ++i) {\n", memberName.c_str());
    std::string keyName = StringHelper::Format("key%d", innerLevel);
    std::string valueName = StringHelper::Format("value%d", innerLevel);
    innerLevel++;
    keyType_->EmitCppUnMarshalling(parcelName, keyName, sb, prefix + TAB, true, innerLevel);
    valueType_->EmitCppUnMarshalling(parcelName, valueName, sb, prefix + TAB, true, innerLevel);
    sb.Append(prefix + TAB).AppendFormat("%s[%s] = %s;\n", name.c_str(), keyName.c_str(), valueName.c_str());
    sb.Append(prefix).Append("}\n");
}

void ASTMapType::EmitJavaWriteVar(
    const std::string &parcelName, const std::string &name, StringBuilder &sb, const std::string &prefix) const
{
    sb.Append(prefix).AppendFormat("%s.writeInt(%s.size());\n", parcelName.c_str(), name.c_str());
    sb.Append(prefix).AppendFormat("for (Map.Entry<%s, %s> entry : %s.entrySet()) {\n",
        keyType_->EmitJavaType(TypeMode::NO_MODE, true).c_str(),
        valueType_->EmitJavaType(TypeMode::NO_MODE, true).c_str(), name.c_str());
    keyType_->EmitJavaWriteVar(parcelName, "entry.getKey()", sb, prefix + TAB);
    valueType_->EmitJavaWriteVar(parcelName, "entry.getValue()", sb, prefix + TAB);
    sb.Append(prefix).Append("}\n");
}

void ASTMapType::EmitJavaReadVar(
    const std::string &parcelName, const std::string &name, StringBuilder &sb, const std::string &prefix) const
{
    sb.Append(prefix).AppendFormat("int %sSize = %s.readInt();\n", name.c_str(), parcelName.c_str());
    sb.Append(prefix).AppendFormat("for (int i = 0; i < %sSize; ++i) {\n", name.c_str());

    keyType_->EmitJavaReadInnerVar(parcelName, "key", false, sb, prefix + TAB);
    valueType_->EmitJavaReadInnerVar(parcelName, "value", false, sb, prefix + TAB);

    sb.Append(prefix + TAB).AppendFormat("%s.put(key, value);\n", name.c_str());
    sb.Append(prefix).Append("}\n");
}

void ASTMapType::EmitJavaReadInnerVar(const std::string &parcelName, const std::string &name, bool isInner,
    StringBuilder &sb, const std::string &prefix) const
{
    sb.Append(prefix).AppendFormat("%s %s = new Hash%s();\n", EmitJavaType(TypeMode::NO_MODE).c_str(), name.c_str(),
        EmitJavaType(TypeMode::NO_MODE).c_str());
    sb.Append(prefix).AppendFormat("int %sSize = %s.readInt();\n", name.c_str(), parcelName.c_str());
    sb.Append(prefix).AppendFormat("for (int i = 0; i < %sSize; ++i) {\n", name.c_str());

    keyType_->EmitJavaReadInnerVar(parcelName, "key", true, sb, prefix + TAB);
    valueType_->EmitJavaReadInnerVar(parcelName, "value", true, sb, prefix + TAB);
    sb.Append(prefix + TAB).AppendFormat("%s.put(key, value);\n", name.c_str());
    sb.Append(prefix).Append("}\n");
}

void ASTMapType::RegisterWriteMethod(Language language, SerMode mode, UtilMethodMap &methods) const
{
    keyType_->RegisterWriteMethod(language, mode, methods);
    valueType_->RegisterWriteMethod(language, mode, methods);
}

void ASTMapType::RegisterReadMethod(Language language, SerMode mode, UtilMethodMap &methods) const
{
    keyType_->RegisterReadMethod(language, mode, methods);
    valueType_->RegisterReadMethod(language, mode, methods);
}
} // namespace HDI
} // namespace OHOS