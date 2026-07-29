/*
 * Copyright (c) 2021-2023 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#ifndef OHOS_HDI_PARSER_H
#define OHOS_HDI_PARSER_H

#include <memory>
#include <set>
#include <vector>

#include "ast/ast.h"
#include "ast/ast_attribute.h"
#include "ast/ast_interface_type.h"
#include "ast/ast_method.h"
#include "ast/ast_type.h"
#include "lexer/lexer.h"
#include "preprocessor/preprocessor.h"
#include "util/autoptr.h"
#include "util/light_refcount_base.h"
#include "util/options.h"

namespace OHOS {
namespace HDI {
using AttrSet = std::set<Token, TokenTypeCompare>;
struct AstCompare {
    bool operator()(const AutoPtr<AST> &lhs, const AutoPtr<AST> &rhs) const
    {
        return lhs->GetMinorVer() < rhs->GetMinorVer();
    }
};
using AstMergeMap = std::map<std::string, std::set<AutoPtr<AST>, AstCompare>>;
class Parser {
public:
    Parser() = default;

    ~Parser() = default;

    bool Parse(const std::vector<FileDetail> &fileDetails);

    using StrAstMap = std::unordered_map<std::string, AutoPtr<AST>>;
    inline const StrAstMap &GetAllAst() const
    {
        return allAsts_;
    }

private:
    class Attribute : public LightRefCountBase {
    public:
        bool isOneWay = false;
        bool isCallback = false;
        bool isFull = false;
        bool isLite = false;
    };

    bool ParseOne(const std::string &sourceFile);

    bool Reset(const std::string &sourceFile);

    bool ParseFile();

    std::string ParseLicense();

    bool ParsePackage();

    bool ParserPackageInfo(const std::string &packageName);

    bool ParseImports();

    void ParseImportInfo();

    void ParseSequenceableInfo();

    bool ParseTypeDecls();

    // parse attributes of type
    void ParseAttribute();

    AttrSet ParseAttributeInfo();

    bool AprseAttrUnit(AttrSet &attrs);

    // parse interface type
    void ParseInterface(const AttrSet &attrs = {});

    AutoPtr<ASTAttr> ParseInfAttrInfo(const AttrSet &attrs);

    void CheckInterfaceAttr(const AutoPtr<ASTInterfaceType> &interface, Token token);

    void ParseInterfaceBody(const AutoPtr<ASTInterfaceType> &interface);

    AutoPtr<ASTMethod> ParseMethod(const AutoPtr<ASTInterfaceType> &interface);

    AutoPtr<ASTAttr> ParseMethodAttr();

    AutoPtr<ASTMethod> CreateGetVersionMethod();

    void CheckMethodAttr(const AutoPtr<ASTInterfaceType> &interface, const AutoPtr<ASTMethod> &method);

    void ParseMethodParamList(const AutoPtr<ASTMethod> &method);

    AutoPtr<ASTParameter> ParseParam();

    AutoPtr<ASTParamAttr> ParseParamAttr();

    // parse type
    AutoPtr<ASTType> ParseType();

    AutoPtr<ASTType> ParseBasicType();

    AutoPtr<ASTType> ParseUnsignedType();

    AutoPtr<ASTType> ParseArrayType(const AutoPtr<ASTType> &elementType);

    AutoPtr<ASTType> ParseListType();

    AutoPtr<ASTType> ParseMapType();

    AutoPtr<ASTType> ParseSmqType();

    AutoPtr<ASTType> ParseUserDefType();

    // parse declaration of enum
    void ParseEnumDeclaration(const AttrSet &attrs = {});

    AutoPtr<ASTType> ParseEnumBaseType();

    void ParserEnumMember(const AutoPtr<ASTEnumType> &enumType);

    // parse declaration of struct
    void ParseStructDeclaration(const AttrSet &attrs = {});

    void ParseStructMember(const AutoPtr<ASTStructType> &structType);

    // parse declaration of union
    void ParseUnionDeclaration(const AttrSet &attrs = {});

    void ParseUnionMember(const AutoPtr<ASTUnionType> &unionType);

    bool AddUnionMember(
        const AutoPtr<ASTUnionType> &unionType, const AutoPtr<ASTType> &type, const std::string &name) const;

    AutoPtr<ASTAttr> ParseUserDefTypeAttr(const AttrSet &attrs);

    // parse expression
    AutoPtr<ASTExpr> ParseExpr();

    AutoPtr<ASTExpr> ParseAndExpr();

    AutoPtr<ASTExpr> ParseXorExpr();

    AutoPtr<ASTExpr> ParseOrExpr();

    AutoPtr<ASTExpr> ParseShiftExpr();

    AutoPtr<ASTExpr> ParseAddExpr();

    AutoPtr<ASTExpr> ParseMulExpr();

    AutoPtr<ASTExpr> ParseUnaryExpr();

    AutoPtr<ASTExpr> ParsePrimaryExpr();

    AutoPtr<ASTExpr> ParseNumExpr();

    bool CheckNumber(const std::string& integerVal) const;

    bool CheckType(const Token &token, const AutoPtr<ASTType> &type);

    bool CheckTypeByMode(const Token &token, const AutoPtr<ASTType> &type);

    void SetAstFileType();

    bool CheckIntegrity();

    bool CheckInterfaceAst();

    bool CheckCallbackAst();

    bool CheckPackageName(const std::string &filePath, const std::string &packageName) const;

    bool CheckImport(const std::string &importName);

    void ParseInterfaceExtends(AutoPtr<ASTInterfaceType> &interface);

    void ParseExtendsInfo(AutoPtr<ASTInterfaceType> &interfaceType);

    bool CheckExtendsName(AutoPtr<ASTInterfaceType> &interfaceType, const std::string &extendsName);

    bool CheckExtendsVersion(
        AutoPtr<ASTInterfaceType> &interfaceType, const std::string &extendsName, AutoPtr<AST> extendsAst);

    bool CheckImportsVersion(AutoPtr<AST> extendsAst);

    void SetInterfaceVersion(AutoPtr<ASTInterfaceType> &interfaceType);

    inline static bool IsPrimitiveType(Token token)
    {
        return token.kind >= TokenType::BOOLEAN && token.kind <= TokenType::ASHMEM;
    }

    bool AddAst(const AutoPtr<AST> &ast);

    void LogError(const std::string &message);

    void LogError(const Token &token, const std::string &message);

    void ShowError();

    bool PostProcess();

    bool CheckExistExtends();

    bool GetGenVersion(std::vector<size_t> &version, std::string &genPackageName);

    void GetGenNamespace(AutoPtr<ASTNamespace> &ns);

    void SortAstByName(AstMergeMap &mergeMap, StrAstMap &allAsts);

    void MergeAsts(AstMergeMap &mergeMap);

    void MergeAst(AutoPtr<AST> &targetAst, AutoPtr<AST> sourceAst);

    void MergeImport(AutoPtr<AST> &targetAst, AutoPtr<AST> sourceAst);

    void MergeInterfaceDef(AutoPtr<AST> &targetAst, AutoPtr<AST> sourceAst);

    void MergeTypes(AutoPtr<AST> &targetAst, AutoPtr<AST> sourceAst);

    void MergeSequenceableDef(AutoPtr<AST> &targetAst, AutoPtr<AST> sourceAst);

    void MergeTypeDefinitions(AutoPtr<AST> &targetAst, AutoPtr<AST> sourceAst);

    void ModifyImport(StrAstMap &allAsts, std::string &genPackageName);

    void ModifyPackageNameAndVersion(StrAstMap &allAsts, std::string &genPackageName, std::vector<size_t> genVersion);

    void ModifyInterfaceNamespace(AutoPtr<ASTNamespace> &ns);

    Lexer lexer_;
    std::vector<std::string> errors_;
    AutoPtr<AST> ast_;
    StrAstMap allAsts_;
};
} // namespace HDI
} // namespace OHOS

#endif // OHOS_HDI_PARSER_H