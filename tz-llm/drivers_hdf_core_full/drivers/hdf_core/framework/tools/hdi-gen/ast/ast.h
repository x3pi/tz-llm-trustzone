/*
 * Copyright (c) 2021-2023 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#ifndef OHOS_HDI_AST_H
#define OHOS_HDI_AST_H

#include <unordered_map>
#include <vector>

#include "ast/ast_array_type.h"
#include "ast/ast_boolean_type.h"
#include "ast/ast_native_buffer_type.h"
#include "ast/ast_byte_type.h"
#include "ast/ast_double_type.h"
#include "ast/ast_enum_type.h"
#include "ast/ast_fd_type.h"
#include "ast/ast_float_type.h"
#include "ast/ast_integer_type.h"
#include "ast/ast_interface_type.h"
#include "ast/ast_long_type.h"
#include "ast/ast_map_type.h"
#include "ast/ast_namespace.h"
#include "ast/ast_node.h"
#include "ast/ast_pointer_type.h"
#include "ast/ast_sequenceable_type.h"
#include "ast/ast_short_type.h"
#include "ast/ast_smq_type.h"
#include "ast/ast_string_type.h"
#include "ast/ast_struct_type.h"
#include "ast/ast_uchar_type.h"
#include "ast/ast_uint_type.h"
#include "ast/ast_ulong_type.h"
#include "ast/ast_union_type.h"
#include "ast/ast_ushort_type.h"
#include "util/autoptr.h"

namespace OHOS {
namespace HDI {
enum class ASTFileType {
    AST_IFACE,        // this idl file contains class of normal interface
    AST_CALL_IFACE,   // this idl file contains class of interface that as parameter
    AST_ICALLBACK,    // this idl file contains class of callback interface
    AST_TYPES,        // this idl file contains custom types
    AST_SEQUENCEABLE, // this is not an idl file, but a c++/java file
};

class AST : public ASTNode {
public:
    using StrASTMap = std::unordered_map<std::string, AutoPtr<AST>>;
    using TypeStringMap = std::unordered_map<std::string, AutoPtr<ASTType>>;

    ~AST() override = default;

    void SetAStFileType(ASTFileType fileType)
    {
        astFileType_ = fileType;
    }

    ASTFileType GetASTFileType() const
    {
        return astFileType_;
    }

    void SetIdlFile(const std::string &idlFile);

    inline std::string GetName()
    {
        return name_;
    }

    void SetFullName(const std::string &fullName);

    inline std::string GetFullName()
    {
        return packageName_ + "." + name_;
    }

    inline std::string GetPackageName() const
    {
        return packageName_;
    }

    inline void SetLicense(const std::string &license)
    {
        license_ = license;
    }

    inline std::string GetLicense()
    {
        return license_;
    }

    void SetPackageName(const std::string &packageName);

    AutoPtr<ASTNamespace> ParseNamespace(const std::string &nspaceStr);

    void AddNamespace(const AutoPtr<ASTNamespace> &nspace);

    AutoPtr<ASTNamespace> FindNamespace(const std::string &nspaceStr);

    AutoPtr<ASTNamespace> GetNamespace(size_t index);

    inline std::vector<AutoPtr<ASTNamespace>> GetNamespace()
    {
        return namespaces_;
    }

    inline size_t GetNamespaceNumber()
    {
        return namespaces_.size();
    }

    void AddInterfaceDef(const AutoPtr<ASTInterfaceType> &interface);

    inline AutoPtr<ASTInterfaceType> GetInterfaceDef()
    {
        return interfaceDef_;
    }

    void AddSequenceableDef(const AutoPtr<ASTSequenceableType> &sequenceable);

    inline AutoPtr<ASTSequenceableType> GetSequenceableDef()
    {
        return sequenceableDef_;
    }

    void AddType(const AutoPtr<ASTType> &type);

    AutoPtr<ASTType> FindType(const std::string &typeName, bool lookImports = true);

    inline const TypeStringMap &GetTypes() const
    {
        return types_;
    }

    inline size_t GetTypeNumber() const
    {
        return types_.size();
    }

    void AddTypeDefinition(const AutoPtr<ASTType> &type);

    inline size_t GetTypeDefinitionNumber() const
    {
        return typeDefinitions_.size();
    }

    AutoPtr<ASTType> GetTypeDefintion(size_t index);

    std::string Dump(const std::string &prefix) override;

    bool AddImport(const AutoPtr<AST> &importAst);

    void ClearImport()
    {
        return imports_.clear();
    }

    inline const StrASTMap &GetImports() const
    {
        return imports_;
    }

    void SetVersion(size_t &majorVer, size_t &minorVer);

    inline size_t GetMajorVer() const
    {
        return majorVersion_;
    }

    inline size_t GetMinorVer() const
    {
        return minorVersion_;
    }

    std::string GetVersion() const
    {
        return StringHelper::Format("%u.%u", majorVersion_, minorVersion_);
    }

    inline std::string GetIdlFilePath()
    {
        return idlFilePath_;
    }

private:
    ASTFileType astFileType_ = ASTFileType::AST_IFACE;
    std::string name_;
    std::string license_;
    std::string packageName_;
    size_t majorVersion_;
    size_t minorVersion_;
    std::vector<AutoPtr<ASTNamespace>> namespaces_;
    std::vector<AutoPtr<ASTType>> typeDefinitions_;
    AutoPtr<ASTSequenceableType> sequenceableDef_ = nullptr;
    AutoPtr<ASTInterfaceType> interfaceDef_ = nullptr;

    StrASTMap imports_;
    TypeStringMap types_;

    static TypeStringMap basicTypes_;

    std::string idlFilePath_;
};
} // namespace HDI
} // namespace OHOS

#endif // OHOS_HDI_AST_H