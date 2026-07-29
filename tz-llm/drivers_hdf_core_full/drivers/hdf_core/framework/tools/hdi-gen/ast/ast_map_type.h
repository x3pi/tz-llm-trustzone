/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#ifndef OHOS_HDI_ASTMAPTYPE_H
#define OHOS_HDI_ASTMAPTYPE_H

#include "ast/ast_type.h"
#include "util/autoptr.h"

namespace OHOS {
namespace HDI {
class ASTMapType : public ASTType {
public:
    ASTMapType() : ASTType(TypeKind::TYPE_MAP, false), keyType_(), valueType_() {}

    inline void SetKeyType(const AutoPtr<ASTType> &keyType)
    {
        keyType_ = keyType;
    }

    inline AutoPtr<ASTType> GetKeyType()
    {
        return keyType_;
    }

    inline void SetValueType(const AutoPtr<ASTType> &valueType)
    {
        valueType_ = valueType;
    }

    inline AutoPtr<ASTType> GetValueType()
    {
        return valueType_;
    }

    bool IsMapType() override;

    bool HasInnerType(TypeKind innerTypeKind) const override;

    std::string ToString() const override;

    TypeKind GetTypeKind() override;

    std::string EmitCType(TypeMode mode = TypeMode::NO_MODE) const override;

    std::string EmitCppType(TypeMode mode = TypeMode::NO_MODE) const override;

    std::string EmitJavaType(TypeMode mode, bool isInnerType = false) const override;

    void EmitCppWriteVar(const std::string &parcelName, const std::string &name, StringBuilder &sb,
        const std::string &prefix, unsigned int innerLevel = 0) const override;

    void EmitCppReadVar(const std::string &parcelName, const std::string &name, StringBuilder &sb,
        const std::string &prefix, bool initVariable, unsigned int innerLevel = 0) const override;

    void EmitCppMarshalling(const std::string &parcelName, const std::string &name, StringBuilder &sb,
        const std::string &prefix, unsigned int innerLevel = 0) const override;

    void EmitCppUnMarshalling(const std::string &parcelName, const std::string &name, StringBuilder &sb,
        const std::string &prefix, bool emitType, unsigned int innerLevel = 0) const override;

    void EmitJavaWriteVar(const std::string &parcelName, const std::string &name, StringBuilder &sb,
        const std::string &prefix) const override;

    void EmitJavaReadVar(const std::string &parcelName, const std::string &name, StringBuilder &sb,
        const std::string &prefix) const override;

    void EmitJavaReadInnerVar(const std::string &parcelName, const std::string &name, bool isInner, StringBuilder &sb,
        const std::string &prefix) const override;

    void RegisterWriteMethod(Language language, SerMode mode, UtilMethodMap &methods) const override;

    void RegisterReadMethod(Language language, SerMode mode, UtilMethodMap &methods) const override;

private:
    AutoPtr<ASTType> keyType_;
    AutoPtr<ASTType> valueType_;
};
} // namespace HDI
} // namespace OHOS

#endif // OHOS_HDI_ASTMAPTYPE_H