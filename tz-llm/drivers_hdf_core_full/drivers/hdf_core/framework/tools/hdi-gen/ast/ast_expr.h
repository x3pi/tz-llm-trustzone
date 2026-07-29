/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#ifndef OHOS_HDI_AST_EXPRE_H
#define OHOS_HDI_AST_EXPRE_H

#include "ast/ast_node.h"
#include "util/autoptr.h"

namespace OHOS {
namespace HDI {
class ASTExpr : public ASTNode {
public:
    inline std::string EmitCode()
    {
        return Dump("");
    }

    bool isParenExpr = false;
};

enum class UnaryOpKind {
    PLUS,  // +
    MINUS, // -
    TILDE, // ~
};

class ASTUnaryExpr : public ASTExpr {
public:
    std::string Dump(const std::string &prefix) override;
    std::string UnaryOpToString(UnaryOpKind op) const;

public:
    UnaryOpKind op_;
    AutoPtr<ASTExpr> expr_;
};

enum class BinaryOpKind {
    MUL,    // *
    DIV,    // /
    MOD,    // %
    ADD,    // +
    SUB,    // -
    LSHIFT, // <<
    RSHIFT, // >>
    AND,    // &
    XOR,    // ^
    OR,     // |
};

class ASTBinaryExpr : public ASTExpr {
public:
    std::string Dump(const std::string &prefix) override;
    std::string BinaryOpToString(BinaryOpKind op) const;

public:
    BinaryOpKind op_;
    AutoPtr<ASTExpr> lExpr_;
    AutoPtr<ASTExpr> rExpr_;
};

class ASTNumExpr : public ASTExpr {
public:
    std::string Dump(const std::string &prefix) override;
    std::string value_;
};
} // namespace HDI
} // namespace OHOS

#endif // OHOS_HDI_AST_EXPRE_H