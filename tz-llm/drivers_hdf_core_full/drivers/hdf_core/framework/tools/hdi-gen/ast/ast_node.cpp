/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include "ast/ast_node.h"

namespace OHOS {
namespace HDI {
ASTNode::~ASTNode() {}

std::string ASTNode::ToString() const
{
    return "ASTNode";
}

std::string ASTNode::Dump(const std::string &prefix)
{
    return prefix + "ASTNode";
}
} // namespace HDI
} // namespace OHOS