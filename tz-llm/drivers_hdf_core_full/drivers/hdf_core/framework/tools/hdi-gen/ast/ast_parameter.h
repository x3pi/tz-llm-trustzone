/*
 * Copyright (c) 2021-2022 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#ifndef OHOS_HDI_ASTPARAMETER_H
#define OHOS_HDI_ASTPARAMETER_H

#include "ast/ast_attribute.h"
#include "ast/ast_node.h"
#include "ast/ast_type.h"
#include "util/autoptr.h"

namespace OHOS {
namespace HDI {
class ASTParameter : public ASTNode {
public:
    ASTParameter(const std::string &name, ParamAttr attribute, const AutoPtr<ASTType> &type)
        : ASTNode(), name_(name), attr_(new ASTParamAttr(attribute)), type_(type)
    {
    }

    ASTParameter(const std::string &name, const AutoPtr<ASTParamAttr> &attribute, const AutoPtr<ASTType> &type)
        : ASTNode(), name_(name), attr_(attribute), type_(type)
    {
    }

    inline std::string GetName()
    {
        return name_;
    }

    inline AutoPtr<ASTType> GetType()
    {
        return type_;
    }

    inline ParamAttr GetAttribute()
    {
        return attr_->value_;
    }

    std::string Dump(const std::string &prefix) override;

    std::string EmitCParameter();

    std::string EmitCppParameter();

    std::string EmitJavaParameter();

    std::string EmitCLocalVar();

    std::string EmitCppLocalVar();

    std::string EmitJavaLocalVar() const;

    void EmitCWriteVar(const std::string &parcelName, const std::string &ecName, const std::string &gotoLabel,
        StringBuilder &sb, const std::string &prefix) const;

    bool EmitCProxyWriteOutVar(const std::string &parcelName, const std::string &ecName, const std::string &gotoLabel,
        StringBuilder &sb, const std::string &prefix) const;

    void EmitCStubReadOutVar(const std::string &buffSizeName, const std::string &memFlagName,
        const std::string &parcelName, const std::string &ecName, const std::string &gotoLabel, StringBuilder &sb,
        const std::string &prefix) const;

    void EmitJavaWriteVar(const std::string &parcelName, StringBuilder &sb, const std::string &prefix) const;

    void EmitJavaReadVar(const std::string &parcelName, StringBuilder &sb, const std::string &prefix) const;

private:
    std::string name_;
    AutoPtr<ASTParamAttr> attr_;
    AutoPtr<ASTType> type_;
};
} // namespace HDI
} // namespace OHOS

#endif // OHOS_HDI_ASTPARAMETER_H