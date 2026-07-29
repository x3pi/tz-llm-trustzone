/*
 * Copyright (c) 2021-2023 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include "ast/ast.h"

#include <cstdlib>

#include "util/string_builder.h"

namespace OHOS {
namespace HDI {
AST::TypeStringMap AST::basicTypes_ = {
    {"boolean",        new ASTBooleanType()     },
    {"byte",           new ASTByteType()        },
    {"short",          new ASTShortType()       },
    {"int",            new ASTIntegerType()     },
    {"long",           new ASTLongType()        },
    {"float",          new ASTFloatType()       },
    {"double",         new ASTDoubleType()      },
    {"String",         new ASTStringType()      },
    {"unsigned char",  new ASTUcharType()       },
    {"unsigned short", new ASTUshortType()      },
    {"unsigned int",   new ASTUintType()        },
    {"unsigned long",  new ASTUlongType()       },
    {"FileDescriptor", new ASTFdType()          },
    {"Ashmem",         new ASTAshmemType()      },
    {"NativeBuffer",   new ASTNativeBufferType()},
    {"Pointer",        new ASTPointerType()     },
};

void AST::SetIdlFile(const std::string &idlFile)
{
    idlFilePath_ = idlFile;
#ifdef __MINGW32__
    size_t index = idlFilePath_.rfind('\\');
#else
    size_t index = idlFilePath_.rfind('/');
#endif

    size_t end = idlFilePath_.rfind(".idl");
    if (end == std::string::npos) {
        end = idlFile.size();
    }

    name_ = StringHelper::SubStr(idlFilePath_, (index == std::string::npos) ? 0 : (index + 1), end);
}

void AST::SetFullName(const std::string &fullName)
{
    size_t index = fullName.rfind('.');
    if (index != std::string::npos) {
        packageName_ = StringHelper::SubStr(fullName, 0, index);
        name_ = StringHelper::SubStr(fullName, index + 1);
    } else {
        packageName_ = "";
        name_ = fullName;
    }
}

void AST::SetPackageName(const std::string &packageName)
{
    packageName_ = packageName;
    ParseNamespace(packageName_);
}

AutoPtr<ASTNamespace> AST::ParseNamespace(const std::string &nspaceStr)
{
    AutoPtr<ASTNamespace> currNspace;
    size_t begin = 0;
    size_t index = 0;
    while ((index = nspaceStr.find('.', begin)) != std::string::npos) {
        std::string ns = StringHelper::SubStr(nspaceStr, begin, index);
        AutoPtr<ASTNamespace> nspace;
        if (currNspace == nullptr) {
            nspace = FindNamespace(ns);
        } else {
            nspace = currNspace->FindNamespace(ns);
        }
        if (nspace == nullptr) {
            nspace = new ASTNamespace(ns);
            if (currNspace == nullptr) {
                AddNamespace(nspace);
            } else {
                currNspace->AddNamespace(nspace);
            }
        }
        currNspace = nspace;
        begin = index + 1;
    }
    return currNspace;
}

void AST::AddNamespace(const AutoPtr<ASTNamespace> &nspace)
{
    if (nspace == nullptr) {
        return;
    }
    namespaces_.push_back(nspace);
}

AutoPtr<ASTNamespace> AST::FindNamespace(const std::string &nspaceStr)
{
    for (auto nspace : namespaces_) {
        if (nspace->ToShortString() == nspaceStr) {
            return nspace;
        }
    }
    return nullptr;
}

AutoPtr<ASTNamespace> AST::GetNamespace(size_t index)
{
    if (index >= namespaces_.size()) {
        return nullptr;
    }

    return namespaces_[index];
}

void AST::AddInterfaceDef(const AutoPtr<ASTInterfaceType> &interface)
{
    if (interface == nullptr) {
        return;
    }

    interfaceDef_ = interface;
    AddType(interface.Get());
}

void AST::AddSequenceableDef(const AutoPtr<ASTSequenceableType> &sequenceable)
{
    if (sequenceable == nullptr) {
        return;
    }

    sequenceableDef_ = sequenceable;
    AddType(sequenceable.Get());
}

void AST::AddType(const AutoPtr<ASTType> &type)
{
    if (type == nullptr) {
        return;
    }

    types_[type->ToString()] = type;
}

AutoPtr<ASTType> AST::FindType(const std::string &typeName, bool lookImports)
{
    if (typeName.empty()) {
        return nullptr;
    }

    for (const auto &type : types_) {
        if ((typeName.find('.') == std::string::npos && type.second->GetName() == typeName) ||
            type.first == typeName) {
            return type.second;
        }
    }

    auto basicTypePair = basicTypes_.find(typeName);
    if (basicTypePair != basicTypes_.end()) {
        return basicTypePair->second;
    }

    if (!lookImports) {
        return nullptr;
    }

    AutoPtr<ASTType> type = nullptr;
    for (const auto &importPair : imports_) {
        type = importPair.second->FindType(typeName, false);
        if (type != nullptr) {
            break;
        }
    }
    return type;
}

void AST::AddTypeDefinition(const AutoPtr<ASTType> &type)
{
    if (type == nullptr) {
        return;
    }

    AddType(type);
    typeDefinitions_.push_back(type);
}

AutoPtr<ASTType> AST::GetTypeDefintion(size_t index)
{
    if (index >= typeDefinitions_.size()) {
        return nullptr;
    }
    return typeDefinitions_[index];
}

std::string AST::Dump(const std::string &prefix)
{
    StringBuilder sb;

    sb.Append(prefix);
    sb.Append("AST[");
    sb.Append("name: ").Append(name_).Append(" ");
    sb.Append("file: ").Append(idlFilePath_);
    sb.Append("]\n");

    sb.Append("package ").Append(packageName_).Append(";");
    sb.Append('\n');
    sb.Append('\n');

    if (imports_.size() > 0) {
        for (const auto &import : imports_) {
            sb.AppendFormat("import %s;\n", import.first.c_str());
        }
        sb.Append("\n");
    }

    if (typeDefinitions_.size() > 0) {
        for (auto type : typeDefinitions_) {
            std::string info = type->Dump("");
            sb.Append(info).Append("\n");
        }
    }

    if (interfaceDef_ != nullptr) {
        std::string info = interfaceDef_->Dump("");
        sb.Append(info).Append("\n");
    }

    return sb.ToString();
}

bool AST::AddImport(const AutoPtr<AST> &importAst)
{
    if (imports_.find(importAst->GetFullName()) != imports_.end()) {
        return false;
    }

    imports_[importAst->GetFullName()] = importAst;

    return true;
}

void AST::SetVersion(size_t &majorVer, size_t &minorVer)
{
    majorVersion_ = majorVer;
    minorVersion_ = minorVer;
}
} // namespace HDI
} // namespace OHOS