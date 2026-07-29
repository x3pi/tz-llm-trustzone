/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#ifndef OHOS_HDI_ASTSEQUENCEABLETYPE_H
#define OHOS_HDI_ASTSEQUENCEABLETYPE_H

#include "ast/ast_type.h"

namespace OHOS {
namespace HDI {
class ASTSequenceableType : public ASTType {
public:
    ASTSequenceableType() : ASTType(TypeKind::TYPE_SEQUENCEABLE, false) {}

    void SetNamespace(const AutoPtr<ASTNamespace> &nspace) override;

    bool IsSequenceableType() override;

    std::string Dump(const std::string &prefix) override;

    TypeKind GetTypeKind() override;

    std::string GetFullName() const;

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
};
} // namespace HDI
} // namespace OHOS

#endif // OHOS_HDI_ASTSEQUENCEABLETYPE_H