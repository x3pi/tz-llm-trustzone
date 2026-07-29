/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include "ast/ast_expr.h"
#include "util/string_builder.h"

namespace OHOS {
namespace HDI {
std::string ASTUnaryExpr::Dump(const std::string &prefix)
{
    StringBuilder sb;
    sb.Append(prefix);
    if (isParenExpr) {
        sb.Append("(");
    }

    sb.AppendFormat("%s%s", UnaryOpToString(op_).c_str(), expr_->Dump("").c_str());

    if (isParenExpr) {
        sb.Append(")");
    }

    return sb.ToString();
}

std::string ASTUnaryExpr::UnaryOpToString(UnaryOpKind op) const
{
    switch (op) {
        case UnaryOpKind::PLUS:
            return "+";
        case UnaryOpKind::MINUS:
            return "-";
        case UnaryOpKind::TILDE:
            return "~";
        default:
            return "unknown";
    }
}

std::string ASTBinaryExpr::Dump(const std::string &prefix)
{
    StringBuilder sb;
    sb.Append(prefix);
    if (isParenExpr) {
        sb.Append("(");
    }

    sb.AppendFormat("%s %s %s", lExpr_->Dump("").c_str(), BinaryOpToString(op_).c_str(), rExpr_->Dump("").c_str());

    if (isParenExpr) {
        sb.Append(")");
    }

    return sb.ToString();
}

std::string ASTBinaryExpr::BinaryOpToString(BinaryOpKind op) const
{
    switch (op) {
        case BinaryOpKind::MUL:
            return "*";
        case BinaryOpKind::DIV:
            return "/";
        case BinaryOpKind::MOD:
            return "%";
        case BinaryOpKind::ADD:
            return "+";
        case BinaryOpKind::SUB:
            return "-";
        case BinaryOpKind::LSHIFT:
            return "<<";
        case BinaryOpKind::RSHIFT:
            return ">>";
        case BinaryOpKind::AND:
            return "&";
        case BinaryOpKind::XOR:
            return "^";
        case BinaryOpKind::OR:
            return "|";
        default:
            return "unknown";
    }
}

std::string ASTNumExpr::Dump(const std::string &prefix)
{
    StringBuilder sb;
    sb.Append(prefix);
    if (isParenExpr) {
        sb.Append("(");
    }

    sb.AppendFormat("%s", value_.c_str());

    if (isParenExpr) {
        sb.Append("(");
    }

    return sb.ToString();
}
} // namespace HDI
} // namespace OHOS