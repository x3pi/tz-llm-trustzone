/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include "ast/ast_pointer_type.h"

namespace OHOS {
namespace HDI {
bool ASTPointerType::IsPointerType()
{
    return true;
}

std::string ASTPointerType::ToString() const
{
    return "Pointer";
}

TypeKind ASTPointerType::GetTypeKind()
{
    return TypeKind::TYPE_POINTER;
}

std::string ASTPointerType::EmitCType(TypeMode mode) const
{
    switch (mode) {
        case TypeMode::NO_MODE:
            return "void*";
        case TypeMode::PARAM_IN:
            return "const void*";
        case TypeMode::PARAM_OUT:
            return "void**";
        case TypeMode::LOCAL_VAR:
            return "void*";
        default:
            return "unknow type";
    }
}

std::string ASTPointerType::EmitCppType(TypeMode mode) const
{
    switch (mode) {
        case TypeMode::NO_MODE:
            return "void*";
        case TypeMode::PARAM_IN:
            return "const void*";
        case TypeMode::PARAM_OUT:
            return "void*&";
        case TypeMode::LOCAL_VAR:
            return "void*";
        default:
            return "unknow type";
    }
}
} // namespace HDI
} // namespace OHOS