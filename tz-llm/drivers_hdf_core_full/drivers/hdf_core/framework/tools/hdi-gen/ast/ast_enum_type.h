/*
 * Copyright (c) 2021-2022 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#ifndef OHOS_HDI_ASTENUMTYPE_H
#define OHOS_HDI_ASTENUMTYPE_H

#include "ast/ast_attribute.h"
#include "ast/ast_expr.h"
#include "ast/ast_type.h"
#include "util/autoptr.h"

#include <vector>

namespace OHOS {
namespace HDI {
class ASTEnumValue : public ASTNode {
public:
    explicit ASTEnumValue(const std::string &name) : mName_(name), value_(nullptr) {}

    inline ~ASTEnumValue() override {}

    inline std::string GetName()
    {
        return mName_;
    }

    inline void SetType(const AutoPtr<ASTType> &type)
    {
        mType_ = type;
    }

    inline AutoPtr<ASTType> GetType()
    {
        return mType_;
    }

    inline void SetExprValue(const AutoPtr<ASTExpr> &value)
    {
        value_ = value;
    }

    inline AutoPtr<ASTExpr> GetExprValue()
    {
        return value_;
    }

private:
    std::string mName_;
    AutoPtr<ASTType> mType_;
    AutoPtr<ASTExpr> value_;
};

class ASTEnumType : public ASTType {
public:
    ASTEnumType() : ASTType(TypeKind::TYPE_ENUM, true), attr_(new ASTAttr()), baseType_(), members_() {}

    inline void SetName(const std::string &name) override
    {
        name_ = name;
    }

    inline std::string GetName() override
    {
        return name_;
    }

    inline void SetAttribute(const AutoPtr<ASTAttr> &attr)
    {
        if (attr != nullptr) {
            attr_ = attr;
        }
    }

    inline bool IsFull()
    {
        return attr_ != nullptr ? attr_->HasValue(ASTAttr::FULL) : false;
    }

    inline bool IsLite()
    {
        return attr_ != nullptr ? attr_->HasValue(ASTAttr::LITE) : false;
    }

    void SetBaseType(const AutoPtr<ASTType> &baseType);
	
    AutoPtr<ASTType> GetBaseType();
	
    bool AddMember(const AutoPtr<ASTEnumValue> &member);
    
    inline std::vector<AutoPtr<ASTEnumValue>> GetMembers()
    {
        return members_;
    }
	
    inline size_t GetMemberNumber()
    {
        return members_.size();
    }

    inline AutoPtr<ASTEnumValue> GetMember(size_t index)
    {
        if (index >= members_.size()) {
            return nullptr;
        }
        return members_[index];
    }

    bool IsEnumType() override;

    std::string Dump(const std::string &prefix) override;

    TypeKind GetTypeKind() override;

    std::string EmitCType(TypeMode mode = TypeMode::NO_MODE) const override;

    std::string EmitCppType(TypeMode mode = TypeMode::NO_MODE) const override;

    std::string EmitJavaType(TypeMode mode, bool isInnerType = false) const override;

    std::string EmitCTypeDecl() const;

    std::string EmitCppTypeDecl() const;

    std::string EmitJavaTypeDecl() const;

    void EmitCWriteVar(const std::string &parcelName, const std::string &name, const std::string &ecName,
        const std::string &gotoLabel, StringBuilder &sb, const std::string &prefix) const override;

    void EmitCProxyReadVar(const std::string &parcelName, const std::string &name, bool isInnerType,
        const std::string &ecName, const std::string &gotoLabel, StringBuilder &sb,
        const std::string &prefix) const override;

    void EmitCStubReadVar(const std::string &parcelName, const std::string &name, const std::string &ecName,
        const std::string &gotoLabel, StringBuilder &sb, const std::string &prefix) const override;

    void EmitCppWriteVar(const std::string &parcelName, const std::string &name, StringBuilder &sb,
        const std::string &prefix, unsigned int innerLevel = 0) const override;

    void EmitCppReadVar(const std::string &parcelName, const std::string &name, StringBuilder &sb,
        const std::string &prefix, bool initVariable, unsigned int innerLevel = 0) const override;

    void EmitCMarshalling(const std::string &name, StringBuilder &sb, const std::string &prefix) const override;

    void EmitCUnMarshalling(const std::string &name, const std::string &gotoLabel, StringBuilder &sb,
        const std::string &prefix, std::vector<std::string> &freeObjStatements) const override;

    void EmitCppMarshalling(const std::string &parcelName, const std::string &name, StringBuilder &sb,
        const std::string &prefix, unsigned int innerLevel = 0) const override;

    void EmitCppUnMarshalling(const std::string &parcelName, const std::string &name, StringBuilder &sb,
        const std::string &prefix, bool emitType, unsigned int innerLevel = 0) const override;

private:
    AutoPtr<ASTAttr> attr_ = new ASTAttr();
    AutoPtr<ASTType> baseType_;
    AutoPtr<ASTType> parentType_;
    std::vector<AutoPtr<ASTEnumValue>> members_;
};
} // namespace HDI
} // namespace OHOS

#endif // OHOS_HDI_ASTENUMTYPE_H