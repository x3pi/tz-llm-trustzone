/*
 * Copyright (c) 2021-2023 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include "ast/ast_parameter.h"
#include "ast/ast_array_type.h"
#include "util/string_builder.h"

namespace OHOS {
namespace HDI {
std::string ASTParameter::Dump(const std::string &prefix)
{
    StringBuilder sb;

    sb.Append(prefix);
    sb.Append('[');

    if (attr_->value_ == ParamAttr::PARAM_IN) {
        sb.Append("in");
    } else {
        sb.Append("out");
    }

    sb.Append("] ");
    sb.Append(type_->ToString()).Append(' ');
    sb.Append(name_);

    return sb.ToString();
}

std::string ASTParameter::EmitCParameter()
{
    StringBuilder sb;
    switch (type_->GetTypeKind()) {
        case TypeKind::TYPE_BOOLEAN:
        case TypeKind::TYPE_BYTE:
        case TypeKind::TYPE_SHORT:
        case TypeKind::TYPE_INT:
        case TypeKind::TYPE_LONG:
        case TypeKind::TYPE_UCHAR:
        case TypeKind::TYPE_USHORT:
        case TypeKind::TYPE_UINT:
        case TypeKind::TYPE_ULONG:
        case TypeKind::TYPE_FLOAT:
        case TypeKind::TYPE_DOUBLE:
        case TypeKind::TYPE_ENUM:
        case TypeKind::TYPE_FILEDESCRIPTOR:
        case TypeKind::TYPE_INTERFACE:
        case TypeKind::TYPE_STRING:
        case TypeKind::TYPE_STRUCT:
        case TypeKind::TYPE_UNION:
        case TypeKind::TYPE_NATIVE_BUFFER:
        case TypeKind::TYPE_POINTER: {
            StringBuilder paramStr;
            if (attr_->value_ == ParamAttr::PARAM_IN) {
                paramStr.AppendFormat("%s %s", type_->EmitCType(TypeMode::PARAM_IN).c_str(), name_.c_str());
            } else {
                paramStr.AppendFormat("%s %s", type_->EmitCType(TypeMode::PARAM_OUT).c_str(), name_.c_str());
            }
            if (type_->GetTypeKind() == TypeKind::TYPE_STRING && attr_->value_ == ParamAttr::PARAM_OUT) {
                paramStr.AppendFormat(", uint32_t %sLen", name_.c_str());
            }
            return paramStr.ToString();
        }
        case TypeKind::TYPE_ARRAY:
        case TypeKind::TYPE_LIST: {
            StringBuilder paramStr;
            if (attr_->value_ == ParamAttr::PARAM_IN) {
                paramStr.AppendFormat("%s %s", type_->EmitCType(TypeMode::PARAM_IN).c_str(), name_.c_str());
            } else {
                paramStr.AppendFormat("%s %s", type_->EmitCType(TypeMode::PARAM_OUT).c_str(), name_.c_str());
            }
            paramStr.AppendFormat(
                ", uint32_t%s %sLen", (attr_->value_ == ParamAttr::PARAM_IN) ? "" : "*", name_.c_str());
            return paramStr.ToString();
        }
        default:
            return StringHelper::Format("unknow type %s", name_.c_str());
    }
    return sb.ToString();
}

std::string ASTParameter::EmitCppParameter()
{
    if (attr_->value_ == ParamAttr::PARAM_IN) {
        return StringHelper::Format("%s %s", type_->EmitCppType(TypeMode::PARAM_IN).c_str(), name_.c_str());
    } else {
        return StringHelper::Format("%s %s", type_->EmitCppType(TypeMode::PARAM_OUT).c_str(), name_.c_str());
    }
}

std::string ASTParameter::EmitJavaParameter()
{
    StringBuilder sb;
    switch (type_->GetTypeKind()) {
        case TypeKind::TYPE_BOOLEAN:
        case TypeKind::TYPE_BYTE:
        case TypeKind::TYPE_SHORT:
        case TypeKind::TYPE_INT:
        case TypeKind::TYPE_LONG:
        case TypeKind::TYPE_UCHAR:
        case TypeKind::TYPE_USHORT:
        case TypeKind::TYPE_UINT:
        case TypeKind::TYPE_ULONG:
        case TypeKind::TYPE_FLOAT:
        case TypeKind::TYPE_DOUBLE:
        case TypeKind::TYPE_ENUM:
        case TypeKind::TYPE_FILEDESCRIPTOR:
        case TypeKind::TYPE_STRING:
        case TypeKind::TYPE_SEQUENCEABLE:
        case TypeKind::TYPE_INTERFACE:
        case TypeKind::TYPE_STRUCT:
        case TypeKind::TYPE_UNION:
        case TypeKind::TYPE_POINTER:
        case TypeKind::TYPE_ARRAY:
        case TypeKind::TYPE_LIST:
        case TypeKind::TYPE_MAP: {
            return StringHelper::Format("%s %s", type_->EmitJavaType(TypeMode::NO_MODE, false).c_str(), name_.c_str());
        }
        default:
            return StringHelper::Format("unknow type %s", name_.c_str());
    }

    return sb.ToString();
}

std::string ASTParameter::EmitCLocalVar()
{
    StringBuilder sb;
    sb.AppendFormat("%s %s", type_->EmitCType(TypeMode::LOCAL_VAR).c_str(), name_.c_str());
    switch (type_->GetTypeKind()) {
        case TypeKind::TYPE_BOOLEAN:
            sb.Append(" = false");
            break;
        case TypeKind::TYPE_BYTE:
        case TypeKind::TYPE_SHORT:
        case TypeKind::TYPE_INT:
        case TypeKind::TYPE_LONG:
        case TypeKind::TYPE_UCHAR:
        case TypeKind::TYPE_USHORT:
        case TypeKind::TYPE_UINT:
        case TypeKind::TYPE_ULONG:
        case TypeKind::TYPE_FLOAT:
        case TypeKind::TYPE_DOUBLE:
            sb.Append(" = 0");
            break;
        case TypeKind::TYPE_STRING:
        case TypeKind::TYPE_ARRAY:
        case TypeKind::TYPE_LIST:
        case TypeKind::TYPE_STRUCT:
        case TypeKind::TYPE_UNION:
        case TypeKind::TYPE_INTERFACE:
        case TypeKind::TYPE_NATIVE_BUFFER:
            sb.Append(" = NULL");
            break;
        case TypeKind::TYPE_FILEDESCRIPTOR:
            sb.Append(" = -1");
            break;
        default:
            break;
    }
    sb.Append(";");
    return sb.ToString();
}

std::string ASTParameter::EmitCppLocalVar()
{
    StringBuilder sb;
    sb.AppendFormat("%s %s", type_->EmitCppType(TypeMode::LOCAL_VAR).c_str(), name_.c_str());
    switch (type_->GetTypeKind()) {
        case TypeKind::TYPE_BOOLEAN:
            sb.Append(" = false");
            break;
        case TypeKind::TYPE_BYTE:
        case TypeKind::TYPE_SHORT:
        case TypeKind::TYPE_INT:
        case TypeKind::TYPE_LONG:
        case TypeKind::TYPE_UCHAR:
        case TypeKind::TYPE_USHORT:
        case TypeKind::TYPE_UINT:
        case TypeKind::TYPE_ULONG:
        case TypeKind::TYPE_FLOAT:
        case TypeKind::TYPE_DOUBLE:
            sb.Append(" = 0");
            break;
        case TypeKind::TYPE_FILEDESCRIPTOR:
            sb.Append(" = -1");
            break;
        case TypeKind::TYPE_SEQUENCEABLE:
            sb.Append(" = nullptr");
            break;
        default:
            break;
    }
    sb.Append(";");
    return sb.ToString();
}

std::string ASTParameter::EmitJavaLocalVar() const
{
    return "";
}

void ASTParameter::EmitCWriteVar(const std::string &parcelName, const std::string &ecName, const std::string &gotoLabel,
    StringBuilder &sb, const std::string &prefix) const
{
    if (type_ == nullptr) {
        return;
    }

    type_->EmitCWriteVar(parcelName, name_, ecName, gotoLabel, sb, prefix);
}

bool ASTParameter::EmitCProxyWriteOutVar(const std::string &parcelName, const std::string &ecName,
    const std::string &gotoLabel, StringBuilder &sb, const std::string &prefix) const
{
    if (type_ == nullptr) {
        return false;
    }

    if (type_->IsStringType() || type_->IsArrayType() || type_->IsListType()) {
        type_->EmitCProxyWriteOutVar(parcelName, name_, ecName, gotoLabel, sb, prefix);
        return true;
    }

    return false;
}

void ASTParameter::EmitCStubReadOutVar(const std::string &buffSizeName, const std::string &memFlagName,
    const std::string &parcelName, const std::string &ecName, const std::string &gotoLabel, StringBuilder &sb,
    const std::string &prefix) const
{
    if (type_ == nullptr) {
        return;
    }

    if (type_->IsStringType() || type_->IsArrayType() || type_->IsListType()) {
        type_->EmitCStubReadOutVar(buffSizeName, memFlagName, parcelName, name_, ecName, gotoLabel, sb, prefix);
    }
}

void ASTParameter::EmitJavaWriteVar(const std::string &parcelName, StringBuilder &sb, const std::string &prefix) const
{
    if (attr_->value_ == ParamAttr::PARAM_IN) {
        type_->EmitJavaWriteVar(parcelName, name_, sb, prefix);
    } else {
        if (type_->GetTypeKind() == TypeKind::TYPE_ARRAY) {
            sb.Append(prefix).AppendFormat("if (%s == null) {\n", name_.c_str());
            sb.Append(prefix + TAB).AppendFormat("%s.writeInt(-1);\n", parcelName.c_str());
            sb.Append(prefix).Append("} else {\n");
            sb.Append(prefix + TAB).AppendFormat("%s.writeInt(%s.length);\n", parcelName.c_str(), name_.c_str());
            sb.Append(prefix).Append("}\n");
        }
    }
}

void ASTParameter::EmitJavaReadVar(const std::string &parcelName, StringBuilder &sb, const std::string &prefix) const
{
    if (attr_->value_ == ParamAttr::PARAM_OUT) {
        type_->EmitJavaReadVar(parcelName, name_, sb, prefix);
    }
}
} // namespace HDI
} // namespace OHOS