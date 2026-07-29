/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#ifndef OHOS_HDI_ASTPOINTERTYPE_H
#define OHOS_HDI_ASTPOINTERTYPE_H

#include "ast/ast_type.h"

namespace OHOS {
namespace HDI {
class ASTPointerType : public ASTType {
public:
    ASTPointerType() : ASTType(TypeKind::TYPE_POINTER, true) {}

    bool IsPointerType() override;

    std::string ToString() const override;

    TypeKind GetTypeKind() override;

    std::string EmitCType(TypeMode mode = TypeMode::NO_MODE) const override;

    std::string EmitCppType(TypeMode mode = TypeMode::NO_MODE) const override;

    // the 'Pointer' type only support use in 'passthrough' mode
};
} // namespace HDI
} // namespace OHOS

#endif // OHOS_HDI_ASTPOINTERTYPE_H