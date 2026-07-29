/*
 * Copyright (c) 2021-2022 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#ifndef OHOS_HDI_ASTMETHOD_H
#define OHOS_HDI_ASTMETHOD_H

#include <vector>

#include "ast/ast_node.h"
#include "ast/ast_parameter.h"
#include "util/autoptr.h"

namespace OHOS {
namespace HDI {
class ASTMethod : public ASTNode {
public:
    inline void SetName(const std::string &name)
    {
        name_ = name;
    }

    inline std::string GetName()
    {
        return name_;
    }

    inline void SetAttribute(AutoPtr<ASTAttr> attr)
    {
        if (attr_ != nullptr && attr != nullptr) {
            attr_->SetValue(attr->GetValue());
        }
    }

    inline AutoPtr<ASTAttr> GetAttribute() const
    {
        return attr_;
    }

    inline bool IsOneWay() const
    {
        return attr_->HasValue(ASTAttr::ONEWAY);
    }

    inline bool IsFull() const
    {
        return attr_->HasValue(ASTAttr::FULL);
    }

    inline bool IsLite() const
    {
        return attr_->HasValue(ASTAttr::LITE);
    }

    inline bool IsMini() const
    {
        return attr_->HasValue(ASTAttr::MINI);
    }

    void AddParameter(const AutoPtr<ASTParameter> &parameter);

    AutoPtr<ASTParameter> GetParameter(size_t index);

    inline size_t GetParameterNumber()
    {
        return parameters_.size();
    }

    std::string Dump(const std::string &prefix) override;

private:
    std::string name_;
    AutoPtr<ASTAttr> attr_ = new ASTAttr();
    std::vector<AutoPtr<ASTParameter>> parameters_;
};
} // namespace HDI
} // namespace OHOS

#endif // OHOS_HDI_ASTMETHOD_H
