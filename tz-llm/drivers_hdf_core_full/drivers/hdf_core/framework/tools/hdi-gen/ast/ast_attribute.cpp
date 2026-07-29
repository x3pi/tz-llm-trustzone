/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include "ast/ast_attribute.h"
#include "util/string_builder.h"

namespace OHOS {
namespace HDI {
std::string ASTAttr::ToString() const
{
    std::vector<std::string> attrs;
    if (value_ & ASTAttr::MINI) {
        attrs.push_back("mini");
    }

    if (value_ & ASTAttr::LITE) {
        attrs.push_back("lite");
    }

    if (value_ & ASTAttr::FULL) {
        attrs.push_back("full");
    }

    if (value_ & ASTAttr::ONEWAY) {
        attrs.push_back("oneway");
    }

    if (value_ & ASTAttr::CALLBACK) {
        attrs.push_back("callback");
    }

    StringBuilder sb;
    sb.Append("[");
    for (size_t i = 0; i < attrs.size(); i++) {
        sb.Append(attrs[i]);
        if (i + 1 < attrs.size()) {
            sb.Append(", ");
        }
    }
    sb.Append("]");
    return sb.ToString();
}

std::string ASTAttr::Dump(const std::string &prefix)
{
    return prefix + ToString();
}

bool ASTAttr::Match(SystemLevel level) const
{
    switch (level) {
        case SystemLevel::MINI:
            return HasValue(ASTAttr::MINI);
        case SystemLevel::LITE:
            return HasValue(ASTAttr::LITE);
        case SystemLevel::FULL:
            return HasValue(ASTAttr::FULL);
        default:
            return false;
    }
}

std::string ASTParamAttr::ToString() const
{
    return StringHelper::Format("[%s]", (value_ == ParamAttr::PARAM_IN) ? "in" : "out");
}

std::string ASTParamAttr::Dump(const std::string &prefix)
{
    return prefix + ToString();
}
} // namespace HDI
} // namespace OHOS