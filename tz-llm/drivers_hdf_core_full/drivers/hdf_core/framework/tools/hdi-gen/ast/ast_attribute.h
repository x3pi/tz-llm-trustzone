/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#ifndef OHOS_HDI_AST_ATTRIBUTE_H
#define OHOS_HDI_AST_ATTRIBUTE_H

#include "ast/ast_node.h"

namespace OHOS {
namespace HDI {
class ASTAttr : public ASTNode {
public:
    using Attribute = uint32_t;
    static constexpr Attribute NONE = 0U;
    static constexpr Attribute MINI = 0x1U;
    static constexpr Attribute LITE = 0x1U << 1;
    static constexpr Attribute FULL = 0x1U << 2;
    static constexpr Attribute ONEWAY = 0x1U << 3;
    static constexpr Attribute CALLBACK = 0x1U << 4;

    explicit ASTAttr(Attribute value = ASTAttr::NONE) : value_(value) {}

    std::string ToString() const override;

    std::string Dump(const std::string &prefix) override;

    inline void SetValue(Attribute value)
    {
        value_ |= value;
    }

    inline Attribute GetValue() const
    {
        return value_;
    }

    bool HasValue(Attribute attr) const
    {
        return (value_ & attr) != 0;
    }

    bool Match(SystemLevel level) const;

private:
    Attribute value_;
};

enum class ParamAttr {
    PARAM_IN,
    PARAM_OUT,
};

class ASTParamAttr : public ASTNode {
public:
    explicit ASTParamAttr(ParamAttr value) : ASTNode(), value_(value) {}

    std::string ToString() const override;

    std::string Dump(const std::string &prefix) override;

public:
    ParamAttr value_;
};
} // namespace HDI
} // namespace OHOS

#endif // OHOS_HDI_AST_ATTRIBUTE_H