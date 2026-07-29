/*
 * Copyright (c) 2021-2023 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include "parser/parser.h"

#include <regex>

#include "ast/ast_array_type.h"
#include "ast/ast_enum_type.h"
#include "ast/ast_map_type.h"
#include "ast/ast_parameter.h"
#include "ast/ast_sequenceable_type.h"
#include "ast/ast_smq_type.h"
#include "ast/ast_struct_type.h"
#include "ast/ast_union_type.h"
#include "util/logger.h"
#include "util/string_builder.h"

#define RE_BIN_DIGIT "0[b][0|1]+"      // binary digit
#define RE_OCT_DIGIT "0[0-7]+"         // octal digit
#define RE_DEC_DIGIT "[0-9]+"          // decimal digit
#define RE_HEX_DIFIT "0[xX][0-9a-fA-F]+"  // hexadecimal digit
#define RE_DIGIT_SUFFIX "(u|l|ll|ul|ull|)$"
#define RE_IDENTIFIER "[a-zA-Z_][a-zA-Z0-9_]*"

#define RE_PACKAGE_NUM             3
#define RE_PACKAGE_INDEX           0
#define RE_PACKAGE_MAJOR_VER_INDEX 1
#define RE_PACKAGE_MINOR_VER_INDEX 2

namespace OHOS {
namespace HDI {
static const std::regex RE_PACKAGE(RE_IDENTIFIER "(?:\\." RE_IDENTIFIER ")*\\.[V|v]"
                                                "(" RE_DEC_DIGIT ")_(" RE_DEC_DIGIT ")");
static const std::regex RE_IMPORT(
    RE_IDENTIFIER "(?:\\." RE_IDENTIFIER ")*\\.[V|v]" RE_DEC_DIGIT "_" RE_DEC_DIGIT "." RE_IDENTIFIER);
static std::regex g_binaryNumRe(RE_BIN_DIGIT RE_DIGIT_SUFFIX, std::regex_constants::icase);
static std::regex g_octNumRe(RE_OCT_DIGIT RE_DIGIT_SUFFIX, std::regex_constants::icase);
static std::regex g_decNumRe(RE_DEC_DIGIT RE_DIGIT_SUFFIX, std::regex_constants::icase);
static std::regex g_hexNumRe(RE_HEX_DIFIT RE_DIGIT_SUFFIX, std::regex_constants::icase);

bool Parser::Parse(const std::vector<FileDetail> &fileDetails)
{
    for (const auto &fileDetail : fileDetails) {
        if (!ParseOne(fileDetail.filePath_)) {
            return false;
        }
    }

    if (!PostProcess()) {
        return false;
    }

    return true;
}

bool Parser::ParseOne(const std::string &sourceFile)
{
    if (!Reset(sourceFile)) {
        return false;
    }

    bool ret = ParseFile();
    ret = CheckIntegrity() && ret;
    ret = AddAst(ast_) && ret;
    if (!ret || !errors_.empty()) {
        ShowError();
        return false;
    }

    return true;
}

bool Parser::Reset(const std::string &sourceFile)
{
    bool ret = lexer_.Reset(sourceFile);
    if (!ret) {
        Logger::E(TAG, "Fail to open file '%s'.", sourceFile.c_str());
        return false;
    }

    errors_.clear();
    ast_ = nullptr;
    return true;
}

bool Parser::ParseFile()
{
    ast_ = new AST();
    ast_->SetIdlFile(lexer_.GetFilePath());
    ast_->SetLicense(ParseLicense());

    if (!ParsePackage()) {
        return false;
    }

    if (!ParseImports()) {
        return false;
    }

    if (!ParseTypeDecls()) {
        return false;
    }

    SetAstFileType();
    return true;
}

std::string Parser::ParseLicense()
{
    Token token = lexer_.PeekToken(false);
    if (token.kind == TokenType::COMMENT_BLOCK) {
        lexer_.GetToken(false);
        return token.value;
    }

    return std::string("");
}

bool Parser::ParsePackage()
{
    Token token = lexer_.PeekToken();
    if (token.kind != TokenType::PACKAGE) {
        LogError(token, StringHelper::Format("expected 'package' before '%s' token", token.value.c_str()));
        return false;
    }
    lexer_.GetToken();

    token = lexer_.PeekToken();
    if (token.kind != TokenType::ID) {
        LogError(token, StringHelper::Format("expected name of package before '%s' token", token.value.c_str()));
        lexer_.SkipToken(TokenType::SEMICOLON);
        return false;
    }
    std::string packageName = token.value;
    lexer_.GetToken();

    token = lexer_.PeekToken();
    if (token.kind != TokenType::SEMICOLON) {
        LogError(token, StringHelper::Format("expected ';' before '%s' token", token.value.c_str()));
        return false;
    }
    lexer_.GetToken();

    if (packageName.empty()) {
        LogError(std::string("package name is not expected."));
        return false;
    } else if (!CheckPackageName(lexer_.GetFilePath(), packageName)) {
        LogError(StringHelper::Format(
            "package name '%s' does not match file apth '%s'.", packageName.c_str(), lexer_.GetFilePath().c_str()));
        return false;
    }

    if (!ParserPackageInfo(packageName)) {
        LogError(StringHelper::Format("parse package '%s' infomation failed.", packageName.c_str()));
        return false;
    }

    return true;
}

bool Parser::ParserPackageInfo(const std::string &packageName)
{
    std::cmatch result;
    if (!std::regex_match(packageName.c_str(), result, RE_PACKAGE)) {
        return false;
    }

    if (result.size() < RE_PACKAGE_NUM) {
        return false;
    }

    ast_->SetPackageName(result.str(RE_PACKAGE_INDEX).c_str());
    size_t majorVersion = std::stoul(result.str(RE_PACKAGE_MAJOR_VER_INDEX));
    size_t minorVersion = std::stoul(result.str(RE_PACKAGE_MINOR_VER_INDEX));
    ast_->SetVersion(majorVersion, minorVersion);
    return true;
}

bool Parser::ParseImports()
{
    Token token = lexer_.PeekToken();
    while (token.kind == TokenType::IMPORT || token.kind == TokenType::SEQ) {
        TokenType kind = token.kind;
        lexer_.GetToken();

        token = lexer_.PeekToken();
        if (token.kind != TokenType::ID) {
            LogError(token, StringHelper::Format("expected identifier before '%s' token", token.value.c_str()));
            lexer_.SkipToken(TokenType::SEMICOLON);
            token = lexer_.PeekToken();
            continue;
        }

        if (kind == TokenType::IMPORT) {
            ParseImportInfo();
        } else {
            ParseSequenceableInfo();
        }
        lexer_.GetToken();

        token = lexer_.PeekToken();
        if (token.kind != TokenType::SEMICOLON) {
            LogError(token, StringHelper::Format("expected ';' before '%s'.", token.value.c_str()));
            return false;
        }
        lexer_.GetToken();

        token = lexer_.PeekToken();
    }

    return true;
}

void Parser::ParseImportInfo()
{
    Token token = lexer_.PeekToken();
    std::string importName = token.value;
    if (importName.empty()) {
        LogError(token, StringHelper::Format("import name is empty"));
        return;
    }

    if (!CheckImport(importName)) {
        LogError(token, StringHelper::Format("import name is illegal"));
        return;
    }

    auto iter = allAsts_.find(importName);
    AutoPtr<AST> importAst = (iter != allAsts_.end()) ? iter->second : nullptr;
    if (importAst == nullptr) {
        LogError(token, StringHelper::Format("can not find idl file from import name '%s'", importName.c_str()));
        return;
    }

    if (!CheckImportsVersion(importAst)) {
        LogError(token, StringHelper::Format("extends import version must less than current import version"));
        return;
    }

    if (!ast_->AddImport(importAst)) {
        LogError(token, StringHelper::Format("multiple import of '%s'", importName.c_str()));
        return;
    }
}

void Parser::ParseSequenceableInfo()
{
    Token token = lexer_.PeekToken();
    std::string seqName = token.value;
    if (seqName.empty()) {
        LogError(token, StringHelper::Format("sequenceable name is empty"));
        return;
    }

    AutoPtr<ASTSequenceableType> seqType = new ASTSequenceableType();
    size_t index = seqName.rfind('.');
    if (index != std::string::npos) {
        seqType->SetName(seqName.substr(index + 1));
        seqType->SetNamespace(ast_->ParseNamespace(seqName));
    } else {
        seqType->SetName(seqName);
    }

    AutoPtr<AST> seqAst = new AST();
    seqAst->SetFullName(seqName);
    seqAst->AddSequenceableDef(seqType);
    seqAst->SetAStFileType(ASTFileType::AST_SEQUENCEABLE);
    ast_->AddImport(seqAst);
    AddAst(seqAst);
}

bool Parser::ParseTypeDecls()
{
    Token token = lexer_.PeekToken();
    while (token.kind != TokenType::END_OF_FILE) {
        switch (token.kind) {
            case TokenType::BRACKETS_LEFT:
                ParseAttribute();
                break;
            case TokenType::INTERFACE:
                ParseInterface();
                break;
            case TokenType::ENUM:
                ParseEnumDeclaration();
                break;
            case TokenType::STRUCT:
                ParseStructDeclaration();
                break;
            case TokenType::UNION:
                ParseUnionDeclaration();
                break;
            default:
                LogError(token, StringHelper::Format("'%s' is not expected", token.value.c_str()));
                lexer_.SkipToken(TokenType::SEMICOLON);
                break;
        }
        token = lexer_.PeekToken();
    }
    return true;
}

void Parser::ParseAttribute()
{
    AttrSet attrs = ParseAttributeInfo();
    Token token = lexer_.PeekToken();
    switch (token.kind) {
        case TokenType::INTERFACE:
            ParseInterface(attrs);
            break;
        case TokenType::ENUM:
            ParseEnumDeclaration(attrs);
            break;
        case TokenType::STRUCT:
            ParseStructDeclaration(attrs);
            break;
        case TokenType::UNION:
            ParseUnionDeclaration(attrs);
            break;
        default:
            LogError(token, StringHelper::Format("'%s' is not expected", token.value.c_str()));
            lexer_.SkipToken(token.kind);
            break;
    }
}

AttrSet Parser::ParseAttributeInfo()
{
    AttrSet attrs;
    Token token = lexer_.PeekToken();
    if (token.kind != TokenType::BRACKETS_LEFT) {
        LogError(token, StringHelper::Format("expected '[' before '%s' token", token.value.c_str()));
        lexer_.SkipToken(token.kind);
        return attrs;
    }
    lexer_.GetToken();

    token = lexer_.PeekToken();
    while (token.kind != TokenType::BRACKETS_RIGHT && token.kind != TokenType::END_OF_FILE) {
        if (!AprseAttrUnit(attrs)) {
            return attrs;
        }
        token = lexer_.PeekToken();
        if (token.kind == TokenType::COMMA) {
            lexer_.GetToken();
            token = lexer_.PeekToken();
            continue;
        }

        if (token.kind == TokenType::BRACKETS_RIGHT) {
            lexer_.GetToken();
            break;
        } else {
            LogError(token, StringHelper::Format("expected ',' or ']' before '%s' token", token.value.c_str()));
            lexer_.SkipToken(TokenType::BRACKETS_RIGHT);
            break;
        }
    }

    return attrs;
}

bool Parser::AprseAttrUnit(AttrSet &attrs)
{
    Token token = lexer_.PeekToken();
    switch (token.kind) {
        case TokenType::FULL:
        case TokenType::LITE:
        case TokenType::MINI:
        case TokenType::CALLBACK:
        case TokenType::ONEWAY: {
            if (attrs.find(token) != attrs.end()) {
                LogError(token, StringHelper::Format("Duplicate declared attributes '%s'", token.value.c_str()));
            } else {
                attrs.insert(token);
            }
            lexer_.GetToken();
            break;
        }
        default:
            LogError(token, StringHelper::Format("'%s' is a illegal attribute", token.value.c_str()));
            lexer_.SkipToken(TokenType::BRACKETS_RIGHT);
            return false;
    }
    return true;
}

void Parser::ParseInterface(const AttrSet &attrs)
{
    AutoPtr<ASTInterfaceType> interfaceType = new ASTInterfaceType;
    AutoPtr<ASTAttr> astAttr = ParseInfAttrInfo(attrs);
    interfaceType->SetAttribute(astAttr);

    lexer_.GetToken();
    Token token = lexer_.PeekToken();
    if (token.kind != TokenType::ID) {
        LogError(token, StringHelper::Format("expected interface name before '%s' token", token.value.c_str()));
    } else {
        interfaceType->SetName(token.value);
        interfaceType->SetNamespace(ast_->ParseNamespace(ast_->GetFullName()));
        interfaceType->SetLicense(ast_->GetLicense());
        if (token.value != ast_->GetName()) {
            LogError(
                token, StringHelper::Format("interface name '%s' is not equal idl file name", token.value.c_str()));
        }
        lexer_.GetToken();
    }

    CheckInterfaceAttr(interfaceType, token);
    ParseInterfaceExtends(interfaceType);
    ParseInterfaceBody(interfaceType);
    SetInterfaceVersion(interfaceType);
    ast_->AddInterfaceDef(interfaceType);
}

AutoPtr<ASTAttr> Parser::ParseInfAttrInfo(const AttrSet &attrs)
{
    AutoPtr<ASTAttr> infAttr = new ASTAttr();
    bool isFull = false;
    bool isLite = false;
    bool isMini = false;

    for (const auto &attr : attrs) {
        switch (attr.kind) {
            case TokenType::FULL:
                isFull = true;
                break;
            case TokenType::LITE:
                isLite = true;
                break;
            case TokenType::MINI:
                isMini = true;
                break;
            case TokenType::CALLBACK:
                infAttr->SetValue(ASTAttr::CALLBACK);
                break;
            case TokenType::ONEWAY:
                infAttr->SetValue(ASTAttr::ONEWAY);
                break;
            default:
                LogError(attr, StringHelper::Format("illegal attribute of interface"));
                break;
        }
    }

    if (!isFull && !isLite && !isMini) {
        infAttr->SetValue(ASTAttr::FULL | ASTAttr::LITE | ASTAttr::MINI);
    } else {
        if (isFull) {
            infAttr->SetValue(ASTAttr::FULL);
        }
        if (isLite) {
            infAttr->SetValue(ASTAttr::LITE);
        }
        if (isMini) {
            infAttr->SetValue(ASTAttr::MINI);
        }
    }

    return infAttr;
}

void Parser::CheckInterfaceAttr(const AutoPtr<ASTInterfaceType> &interface, Token token)
{
    bool ret = true;
    std::string systemName;
    switch (Options::GetInstance().GetSystemLevel()) {
        case SystemLevel::FULL:
            systemName = "full";
            ret = interface->IsFull();
            break;
        case SystemLevel::LITE:
            systemName = "lite";
            ret = interface->IsLite();
            break;
        case SystemLevel::MINI:
            systemName = "mini";
            ret = interface->IsMini();
            break;
        default:
            break;
    }

    if (!ret) {
        LogError(token, StringHelper::Format("the system option is '%s', but the '%s' interface has no '%s' attribute",
            systemName.c_str(), interface->GetName().c_str(), systemName.c_str()));
    }
}

void Parser::ParseInterfaceBody(const AutoPtr<ASTInterfaceType> &interface)
{
    Token token = lexer_.PeekToken();
    if (token.kind != TokenType::BRACES_LEFT) {
        LogError(token, StringHelper::Format("expected '{' before '%s' token", token.value.c_str()));
    } else {
        lexer_.GetToken();
    }

    token = lexer_.PeekToken();
    while (token.kind != TokenType::BRACES_RIGHT && token.kind != TokenType::END_OF_FILE) {
        AutoPtr<ASTMethod> method = ParseMethod(interface);
        interface->AddMethod(method);
        token = lexer_.PeekToken();
    }

    token = lexer_.PeekToken();
    if (token.kind != TokenType::BRACES_RIGHT) {
        LogError(token, StringHelper::Format("expected '{' before '%s' token", token.value.c_str()));
    } else {
        lexer_.GetToken();
    }

    token = lexer_.PeekToken();
    if (token.kind == TokenType::SEMICOLON) {
        lexer_.GetToken();
    }

    interface->AddVersionMethod(CreateGetVersionMethod());
}

AutoPtr<ASTMethod> Parser::ParseMethod(const AutoPtr<ASTInterfaceType> &interface)
{
    AutoPtr<ASTMethod> method = new ASTMethod();
    method->SetAttribute(ParseMethodAttr());

    Token token = lexer_.PeekToken();
    if (token.kind != TokenType::ID) {
        LogError(token, StringHelper::Format("expected method name before '%s' token", token.value.c_str()));
    } else {
        method->SetName(token.value);
        lexer_.GetToken();
    }

    CheckMethodAttr(interface, method);
    ParseMethodParamList(method);

    token = lexer_.PeekToken();
    if (token.kind != TokenType::SEMICOLON) {
        LogError(token, StringHelper::Format("expected ';' before '%s' token", token.value.c_str()));
    } else {
        lexer_.GetToken();
    }

    return method;
}

AutoPtr<ASTAttr> Parser::ParseMethodAttr()
{
    if (lexer_.PeekToken().kind != TokenType::BRACKETS_LEFT) {
        return new ASTAttr();
    }

    AttrSet attrs = ParseAttributeInfo();
    AutoPtr<ASTAttr> methodAttr = new ASTAttr();
    bool isFull = false;
    bool isLite = false;
    bool isMini = false;

    for (const auto &attr : attrs) {
        switch (attr.kind) {
            case TokenType::FULL:
                isFull = true;
                break;
            case TokenType::LITE:
                isLite = true;
                break;
            case TokenType::MINI:
                isMini = true;
                break;
            case TokenType::ONEWAY:
                methodAttr->SetValue(ASTAttr::ONEWAY);
                break;
            default:
                LogError(attr, StringHelper::Format("illegal attribute of interface"));
                break;
        }
    }

    if (isFull) {
        methodAttr->SetValue(ASTAttr::FULL);
    }
    if (isLite) {
        methodAttr->SetValue(ASTAttr::LITE);
    }
    if (isMini) {
        methodAttr->SetValue(ASTAttr::MINI);
    }

    return methodAttr;
}

AutoPtr<ASTMethod> Parser::CreateGetVersionMethod()
{
    AutoPtr<ASTMethod> method = new ASTMethod();
    method->SetName("GetVersion");

    AutoPtr<ASTType> type = ast_->FindType("unsigned int");
    if (type == nullptr) {
        type = new ASTUintType();
    }
    AutoPtr<ASTParameter> majorParam = new ASTParameter("majorVer", ParamAttr::PARAM_OUT, type);
    AutoPtr<ASTParameter> minorParam = new ASTParameter("minorVer", ParamAttr::PARAM_OUT, type);

    method->AddParameter(majorParam);
    method->AddParameter(minorParam);
    return method;
}

void Parser::CheckMethodAttr(const AutoPtr<ASTInterfaceType> &interface, const AutoPtr<ASTMethod> &method)
{
    // if the attribute of method is empty, the default value is attribute of interface
    if (!method->IsMini() && !method->IsLite() && !method->IsFull()) {
        method->SetAttribute(interface->GetAttribute());
    }

    if (!interface->IsMini() && method->IsMini()) {
        LogError(StringHelper::Format(
            "the '%s' mehtod can not have 'mini' attribute, because the '%s' interface has no 'mini' attribute",
            method->GetName().c_str(), interface->GetName().c_str()));
    }

    if (!interface->IsLite() && method->IsLite()) {
        LogError(StringHelper::Format(
            "the '%s' mehtod can not have 'lite' attribute, because the '%s' interface has no 'lite' attribute",
            method->GetName().c_str(), interface->GetName().c_str()));
    }

    if (!interface->IsFull() && method->IsFull()) {
        LogError(StringHelper::Format(
            "the '%s' mehtod can not have 'full' attribute, because the '%s' interface has no 'full' attribute",
            method->GetName().c_str(), interface->GetName().c_str()));
    }

    // the method has 'oneway' attribute if interface or method has 'oneway' attribute
    if (interface->IsOneWay() || method->IsOneWay()) {
        method->GetAttribute()->SetValue(ASTAttr::ONEWAY);
    }
}

void Parser::ParseMethodParamList(const AutoPtr<ASTMethod> &method)
{
    Token token = lexer_.PeekToken();
    if (token.kind != TokenType::PARENTHESES_LEFT) {
        LogError(token, StringHelper::Format("expected '(' before '%s' token", token.value.c_str()));
    } else {
        lexer_.GetToken();
    }

    token = lexer_.PeekToken();
    if (token.kind == TokenType::PARENTHESES_RIGHT) {
        lexer_.GetToken();
        return;
    }

    while (token.kind != TokenType::PARENTHESES_RIGHT && token.kind != TokenType::END_OF_FILE) {
        AutoPtr<ASTParameter> param = ParseParam();
        if (method->IsOneWay() && param->GetAttribute() == ParamAttr::PARAM_OUT) {
            LogError(token, StringHelper::Format("the '%s' parameter of '%s' method can not be 'out'",
                param->GetName().c_str(), method->GetName().c_str()));
        }
        method->AddParameter(param);

        token = lexer_.PeekToken();
        if (token.kind == TokenType::COMMA) {
            lexer_.GetToken();
            token = lexer_.PeekToken();
            if (token.kind == TokenType::PARENTHESES_RIGHT) {
                LogError(token, StringHelper::Format(""));
            }
            continue;
        }

        if (token.kind == TokenType::PARENTHESES_RIGHT) {
            lexer_.GetToken();
            break;
        } else {
            LogError(token, StringHelper::Format("expected ',' or ')' before '%s' token", token.value.c_str()));
            lexer_.SkipToken(TokenType::PARENTHESES_RIGHT);
            break;
        }
    }
}

AutoPtr<ASTParameter> Parser::ParseParam()
{
    AutoPtr<ASTParamAttr> paramAttr = ParseParamAttr();
    AutoPtr<ASTType> paramType = ParseType();
    std::string paramName = "";

    Token token = lexer_.PeekToken();
    if (token.kind != TokenType::ID) {
        LogError(token, StringHelper::Format("expected param name before '%s' token", token.value.c_str()));
    } else {
        paramName = token.value;
        lexer_.GetToken();
    }

    if (paramType != nullptr && paramType->IsInterfaceType()) {
        AutoPtr<ASTInterfaceType> ifaceType = dynamic_cast<ASTInterfaceType *>(paramType.Get());
        if (ifaceType->IsCallback() && paramAttr->value_ != ParamAttr::PARAM_IN) {
            LogError(token, StringHelper::Format("'%s' parameter of callback interface type must be 'in' attribute",
                paramName.c_str()));
        } else if (!ifaceType->IsCallback() && paramAttr->value_ != ParamAttr::PARAM_OUT) {
            LogError(token, StringHelper::Format("'%s' parameter of interface type must be 'out' attribute",
                paramName.c_str()));
        }
        if (!ifaceType->IsCallback()) {
            ifaceType->SetSerializable(true);
        }
    }

    return new ASTParameter(paramName, paramAttr, paramType);
}

AutoPtr<ASTParamAttr> Parser::ParseParamAttr()
{
    AutoPtr<ASTParamAttr> attr = new ASTParamAttr(ParamAttr::PARAM_IN);
    Token token = lexer_.PeekToken();
    if (token.kind != TokenType::BRACKETS_LEFT) {
        LogError(token, StringHelper::Format("expected '[' before '%s' token", token.value.c_str()));
    } else {
        lexer_.GetToken();
    }

    token = lexer_.PeekToken();
    if (token.kind == TokenType::IN) {
        attr->value_ = ParamAttr::PARAM_IN;
        lexer_.GetToken();
    } else if (token.kind == TokenType::OUT) {
        attr->value_ = ParamAttr::PARAM_OUT;
        lexer_.GetToken();
    } else {
        LogError(
            token, StringHelper::Format("expected 'in' or 'out' attribute before '%s' token", token.value.c_str()));
    }

    token = lexer_.PeekToken();
    if (token.kind != TokenType::BRACKETS_RIGHT) {
        LogError(token, StringHelper::Format("expected ']' before '%s' token", token.value.c_str()));
    } else {
        lexer_.GetToken();
    }

    return attr;
}

AutoPtr<ASTType> Parser::ParseType()
{
    AutoPtr<ASTType> type = nullptr;
    Token token = lexer_.PeekToken();
    switch (token.kind) {
        case TokenType::BOOLEAN:
        case TokenType::BYTE:
        case TokenType::SHORT:
        case TokenType::INT:
        case TokenType::LONG:
        case TokenType::STRING:
        case TokenType::FLOAT:
        case TokenType::DOUBLE:
        case TokenType::FD:
        case TokenType::ASHMEM:
        case TokenType::NATIVE_BUFFER:
        case TokenType::POINTER:
        case TokenType::UNSIGNED:
            type = ParseBasicType();
            break;
        case TokenType::LIST:
            type = ParseListType();
            break;
        case TokenType::MAP:
            type = ParseMapType();
            break;
        case TokenType::SMQ:
            type = ParseSmqType();
            break;
        case TokenType::ENUM:
        case TokenType::STRUCT:
        case TokenType::UNION:
        case TokenType::ID:
        case TokenType::SEQ:
            type = ParseUserDefType();
            break;
        default:
            LogError(token, StringHelper::Format("'%s' of type is illegal", token.value.c_str()));
            return nullptr;
    }
    if (type == nullptr) {
        LogError(token, StringHelper::Format("this type was not declared in this scope"));
    }
    if (!CheckType(token, type)) {
        return nullptr;
    }

    while (lexer_.PeekToken().kind == TokenType::BRACKETS_LEFT) {
        type = ParseArrayType(type);
    }
    return type;
}

AutoPtr<ASTType> Parser::ParseBasicType()
{
    AutoPtr<ASTType> type = nullptr;
    Token token = lexer_.PeekToken();
    if (token.kind == TokenType::UNSIGNED) {
        type = ParseUnsignedType();
    } else {
        type = ast_->FindType(token.value);
        lexer_.GetToken();
    }

    ast_->AddType(type);
    return type;
}

AutoPtr<ASTType> Parser::ParseUnsignedType()
{
    AutoPtr<ASTType> type = nullptr;
    std::string namePrefix = lexer_.GetToken().value;
    Token token = lexer_.PeekToken();
    switch (token.kind) {
        case TokenType::CHAR:
        case TokenType::SHORT:
        case TokenType::INT:
        case TokenType::LONG:
            type = ast_->FindType(namePrefix + " " + token.value);
            lexer_.GetToken();
            break;
        default:
            LogError(
                token, StringHelper::Format("'unsigned %s' was not declared in the idl file", token.value.c_str()));
            break;
    }

    return type;
}

AutoPtr<ASTType> Parser::ParseArrayType(const AutoPtr<ASTType> &elementType)
{
    lexer_.GetToken(); // '['

    Token token = lexer_.PeekToken();
    if (token.kind != TokenType::BRACKETS_RIGHT) {
        LogError(token, StringHelper::Format("expected ']' before '%s' token", token.value.c_str()));
        return nullptr;
    }
    lexer_.GetToken(); // ']'

    if (elementType == nullptr) {
        return nullptr;
    }

    AutoPtr<ASTArrayType> arrayType = new ASTArrayType();
    arrayType->SetElementType(elementType);
    AutoPtr<ASTType> retType = arrayType.Get();
    ast_->AddType(retType);
    return retType;
}

AutoPtr<ASTType> Parser::ParseListType()
{
    lexer_.GetToken(); // List

    Token token = lexer_.PeekToken();
    if (token.kind != TokenType::ANGLE_BRACKETS_LEFT) {
        LogError(token, StringHelper::Format("expected '<' before '%s' token", token.value.c_str()));
    } else {
        lexer_.GetToken(); // '<'
    }

    AutoPtr<ASTType> type = ParseType(); // element type
    if (type == nullptr) {
        lexer_.SkipToken(TokenType::ANGLE_BRACKETS_RIGHT);
        return nullptr;
    }

    token = lexer_.PeekToken();
    if (token.kind != TokenType::ANGLE_BRACKETS_RIGHT) {
        LogError(token, StringHelper::Format("expected '>' before '%s' token", token.value.c_str()));
    } else {
        lexer_.GetToken(); // '>'
    }

    AutoPtr<ASTListType> listType = new ASTListType();
    listType->SetElementType(type);
    AutoPtr<ASTType> retType = listType.Get();
    ast_->AddType(retType);
    return retType;
}

AutoPtr<ASTType> Parser::ParseMapType()
{
    lexer_.GetToken(); // 'Map'

    Token token = lexer_.PeekToken();
    if (token.kind != TokenType::ANGLE_BRACKETS_LEFT) {
        LogError(token, StringHelper::Format("expected '<' before '%s' token", token.value.c_str()));
    } else {
        lexer_.GetToken(); // '<'
    }

    AutoPtr<ASTType> keyType = ParseType(); // key type
    if (keyType == nullptr) {
        LogError(token, StringHelper::Format("key type '%s' is illegal", token.value.c_str()));
        lexer_.SkipToken(TokenType::ANGLE_BRACKETS_RIGHT);
        return nullptr;
    }

    token = lexer_.PeekToken();
    if (token.kind != TokenType::COMMA) {
        LogError(token, StringHelper::Format("expected ',' before '%s' token", token.value.c_str()));
    } else {
        lexer_.GetToken(); // ','
    }

    AutoPtr<ASTType> valueType = ParseType();
    if (valueType == nullptr) {
        LogError(token, StringHelper::Format("key type '%s' is illegal", token.value.c_str()));
        lexer_.SkipToken(TokenType::ANGLE_BRACKETS_RIGHT);
        return nullptr;
    }

    token = lexer_.PeekToken();
    if (token.kind != TokenType::ANGLE_BRACKETS_RIGHT) {
        LogError(token, StringHelper::Format("expected '>' before '%s' token", token.value.c_str()));
    } else {
        lexer_.GetToken();
    }

    AutoPtr<ASTMapType> mapType = new ASTMapType();
    mapType->SetKeyType(keyType);
    mapType->SetValueType(valueType);
    AutoPtr<ASTType> retType = mapType.Get();
    ast_->AddType(retType);
    return retType;
}

AutoPtr<ASTType> Parser::ParseSmqType()
{
    lexer_.GetToken(); // 'SharedMemQueue'

    Token token = lexer_.PeekToken();
    if (token.kind != TokenType::ANGLE_BRACKETS_LEFT) {
        LogError(token, StringHelper::Format("expected '<' before '%s' token", token.value.c_str()));
    } else {
        lexer_.GetToken(); // '<'
    }

    AutoPtr<ASTType> innerType = ParseType();
    if (innerType == nullptr) {
        lexer_.SkipToken(TokenType::ANGLE_BRACKETS_RIGHT);
        return nullptr;
    }

    token = lexer_.PeekToken();
    if (token.kind != TokenType::ANGLE_BRACKETS_RIGHT) {
        LogError(token, StringHelper::Format("expected '>' before '%s' token", token.value.c_str()));
    } else {
        lexer_.GetToken(); // '>'
    }

    AutoPtr<ASTSmqType> smqType = new ASTSmqType();
    smqType->SetInnerType(innerType);
    AutoPtr<ASTType> retType = smqType.Get();
    ast_->AddType(retType);
    return retType;
}

AutoPtr<ASTType> Parser::ParseUserDefType()
{
    Token token = lexer_.GetToken();
    if (token.kind == TokenType::ID) {
        return ast_->FindType(token.value);
    }

    token = lexer_.PeekToken();
    if (token.kind != TokenType::ID) {
        LogError(token, StringHelper::Format("expected identifier before '%s' token", token.value.c_str()));
        return nullptr;
    } else {
        lexer_.GetToken();
    }

    std::string typeName = token.value;
    AutoPtr<ASTType> type = ast_->FindType(typeName);
    ast_->AddType(type);
    return type;
}

void Parser::ParseEnumDeclaration(const AttrSet &attrs)
{
    AutoPtr<ASTEnumType> enumType = new ASTEnumType;
    enumType->SetAttribute(ParseUserDefTypeAttr(attrs));

    lexer_.GetToken();
    Token token = lexer_.PeekToken();
    if (token.kind != TokenType::ID) {
        LogError(token, StringHelper::Format("expected enum type name before '%s' token", token.value.c_str()));
    } else {
        lexer_.GetToken();
        enumType->SetName(token.value);
    }

    token = lexer_.PeekToken();
    if (token.kind == TokenType::COLON || token.kind == TokenType::BRACES_LEFT) {
        enumType->SetBaseType(ParseEnumBaseType());
    } else {
        LogError(token, StringHelper::Format("expected ':' or '{' before '%s' token", token.value.c_str()));
    }

    ParserEnumMember(enumType);
    token = lexer_.PeekToken();
    if (token.kind != TokenType::BRACES_RIGHT) {
        LogError(token, StringHelper::Format("expected '}' before '%s' token", token.value.c_str()));
        return;
    } else {
        lexer_.GetToken();
    }

    token = lexer_.PeekToken();
    if (token.kind != TokenType::SEMICOLON) {
        LogError(token, StringHelper::Format("expected ';' before '%s' token", token.value.c_str()));
    } else {
        lexer_.GetToken();
    }

    enumType->SetNamespace(ast_->ParseNamespace(ast_->GetFullName()));
    ast_->AddTypeDefinition(enumType.Get());
}

AutoPtr<ASTType> Parser::ParseEnumBaseType()
{
    AutoPtr<ASTType> baseType = nullptr;
    Token token = lexer_.PeekToken();
    if (token.kind != TokenType::COLON) {
        lexer_.GetToken();
        baseType = ast_->FindType("int");
        return baseType;
    }

    lexer_.GetToken();
    token = lexer_.PeekToken();
    baseType = ParseType();
    if (baseType != nullptr) {
        switch (baseType->GetTypeKind()) {
            case TypeKind::TYPE_BYTE:
            case TypeKind::TYPE_SHORT:
            case TypeKind::TYPE_INT:
            case TypeKind::TYPE_LONG:
            case TypeKind::TYPE_UCHAR:
            case TypeKind::TYPE_USHORT:
            case TypeKind::TYPE_UINT:
            case TypeKind::TYPE_ULONG:
            case TypeKind::TYPE_ENUM:
                break;
            default: {
                LogError(token, StringHelper::Format("illegal base type of enum", baseType->ToString().c_str()));
                lexer_.SkipUntilToken(TokenType::BRACES_LEFT);
            }
        }
    }

    token = lexer_.PeekToken();
    if (token.kind != TokenType::BRACES_LEFT) {
        LogError(token, StringHelper::Format("expected '{' before '%s' token", token.value.c_str()));
    }
    lexer_.GetToken();
    return baseType;
}

void Parser::ParserEnumMember(const AutoPtr<ASTEnumType> &enumType)
{
    while (lexer_.PeekToken().kind == TokenType::ID) {
        Token token = lexer_.GetToken();
        AutoPtr<ASTEnumValue> enumValue = new ASTEnumValue(token.value);

        token = lexer_.PeekToken();
        if (token.kind == TokenType::ASSIGN) {
            lexer_.GetToken();
            enumValue->SetExprValue(ParseExpr());
        }

        enumValue->SetType(enumType->GetBaseType());
        if (!enumType->AddMember(enumValue)) {
            LogError(StringHelper::Format("AddMemberException:member '%s' already exists !",
            enumValue->GetName().c_str()));
        }
        token = lexer_.PeekToken();
        if (token.kind == TokenType::COMMA) {
            lexer_.GetToken();
            continue;
        }

        if (token.kind != TokenType::BRACES_RIGHT) {
            LogError(token, StringHelper::Format("expected ',' or '}' before '%s' token", token.value.c_str()));
        }
    }
}

void Parser::ParseStructDeclaration(const AttrSet &attrs)
{
    AutoPtr<ASTStructType> structType = new ASTStructType;
    structType->SetAttribute(ParseUserDefTypeAttr(attrs));

    lexer_.GetToken();
    Token token = lexer_.PeekToken();
    if (token.kind != TokenType::ID) {
        LogError(token, StringHelper::Format("expected struct name before '%s' token", token.value.c_str()));
    } else {
        structType->SetName(token.value);
        lexer_.GetToken();
    }

    token = lexer_.PeekToken();
    if (token.kind != TokenType::BRACES_LEFT) {
        LogError(token, StringHelper::Format("expected '{' before '%s' token", token.value.c_str()));
    } else {
        lexer_.GetToken();
    }

    ParseStructMember(structType);

    token = lexer_.PeekToken();
    if (token.kind != TokenType::BRACES_RIGHT) {
        LogError(token, StringHelper::Format("expected '}' before '%s' token", token.value.c_str()));
    } else {
        lexer_.GetToken();
    }

    token = lexer_.PeekToken();
    if (token.kind != TokenType::SEMICOLON) {
        LogError(token, StringHelper::Format("expected ';' before '%s' token", token.value.c_str()));
    } else {
        lexer_.GetToken();
    }

    structType->SetNamespace(ast_->ParseNamespace(ast_->GetFullName()));
    ast_->AddTypeDefinition(structType.Get());
}

void Parser::ParseStructMember(const AutoPtr<ASTStructType> &structType)
{
    Token token = lexer_.PeekToken();
    while (token.kind != TokenType::BRACES_RIGHT && token.kind != TokenType::END_OF_FILE) {
        AutoPtr<ASTType> memberType = ParseType();
        if (memberType == nullptr) {
            lexer_.SkipToken(TokenType::SEMICOLON);
            token = lexer_.PeekToken();
            continue;
        }

        token = lexer_.PeekToken();
        if (token.kind != TokenType::ID) {
            LogError(token, StringHelper::Format("expected member name before '%s' token", token.value.c_str()));
            lexer_.SkipToken(TokenType::SEMICOLON);
            token = lexer_.PeekToken();
            continue;
        }

        lexer_.GetToken();
        std::string memberName = token.value;
        structType->AddMember(memberType, memberName);

        token = lexer_.PeekToken();
        if (token.kind == TokenType::SEMICOLON) {
            lexer_.GetToken();
            token = lexer_.PeekToken();
            continue;
        }

        if (token.kind != TokenType::BRACES_RIGHT) {
            LogError(token, StringHelper::Format("expected ',' or '}' before '%s' token", token.value.c_str()));
        }
    }
}

void Parser::ParseUnionDeclaration(const AttrSet &attrs)
{
    AutoPtr<ASTUnionType> unionType = new ASTUnionType;
    unionType->SetAttribute(ParseUserDefTypeAttr(attrs));

    lexer_.GetToken();
    Token token = lexer_.PeekToken();
    if (token.kind != TokenType::ID) {
        LogError(token, StringHelper::Format("expected struct name before '%s' token", token.value.c_str()));
    } else {
        unionType->SetName(token.value);
        lexer_.GetToken();
    }

    token = lexer_.PeekToken();
    if (token.kind != TokenType::BRACES_LEFT) {
        LogError(token, StringHelper::Format("expected '{' before '%s' token", token.value.c_str()));
    } else {
        lexer_.GetToken();
    }

    ParseUnionMember(unionType);

    token = lexer_.PeekToken();
    if (token.kind != TokenType::BRACES_RIGHT) {
        LogError(token, StringHelper::Format("expected '}' before '%s' token", token.value.c_str()));
    } else {
        lexer_.GetToken();
    }

    token = lexer_.PeekToken();
    if (token.kind != TokenType::SEMICOLON) {
        LogError(token, StringHelper::Format("expected ';' before '%s' token", token.value.c_str()));
    } else {
        lexer_.GetToken();
    }

    unionType->SetNamespace(ast_->ParseNamespace(ast_->GetFullName()));
    ast_->AddTypeDefinition(unionType.Get());
}

void Parser::ParseUnionMember(const AutoPtr<ASTUnionType> &unionType)
{
    Token token = lexer_.PeekToken();
    while (token.kind != TokenType::BRACES_RIGHT && token.kind != TokenType::END_OF_FILE) {
        AutoPtr<ASTType> memberType = ParseType();
        if (memberType == nullptr) {
            lexer_.SkipToken(TokenType::SEMICOLON);
            token = lexer_.PeekToken();
            continue;
        }

        token = lexer_.PeekToken();
        if (token.kind != TokenType::ID) {
            LogError(token, StringHelper::Format("expected member name before '%s' token", token.value.c_str()));
            lexer_.SkipToken(TokenType::SEMICOLON);
            token = lexer_.PeekToken();
            continue;
        }
        lexer_.GetToken();

        std::string memberName = token.value;
        if (!AddUnionMember(unionType, memberType, memberName)) {
            LogError(token,
                StringHelper::Format(
                    "union not support this type or name of member duplicate '%s'", token.value.c_str()));
        }

        token = lexer_.PeekToken();
        if (token.kind == TokenType::SEMICOLON) {
            lexer_.GetToken();
            token = lexer_.PeekToken();
            continue;
        }

        if (token.kind != TokenType::BRACES_RIGHT) {
            LogError(token, StringHelper::Format("expected ',' or '}' before '%s' token", token.value.c_str()));
        }
    }
}

bool Parser::AddUnionMember(
    const AutoPtr<ASTUnionType> &unionType, const AutoPtr<ASTType> &type, const std::string &name) const
{
    for (size_t i = 0; i < unionType->GetMemberNumber(); i++) {
        std::string memberName = unionType->GetMemberName(i);
        if (name == memberName) {
            return false;
        }
    }

    // Non pod type members are not allowed in the union
    if (!type->IsPod()) {
        return false;
    }

    unionType->AddMember(type, name);
    return true;
}

AutoPtr<ASTAttr> Parser::ParseUserDefTypeAttr(const AttrSet &attrs)
{
    AutoPtr<ASTAttr> attr = new ASTAttr();
    bool isFull = false;
    bool isLite = false;
    bool isMini = false;

    for (const auto &token : attrs) {
        switch (token.kind) {
            case TokenType::FULL:
                isFull = true;
                break;
            case TokenType::LITE:
                isLite = true;
                break;
            case TokenType::MINI:
                isMini = true;
                break;
            default:
                LogError(token, StringHelper::Format("invalid attribute '%s' for type decl", token.value.c_str()));
                break;
        }
    }

    if (!isFull && !isLite && !isMini) {
        attr->SetValue(ASTAttr::FULL | ASTAttr::LITE | ASTAttr::MINI);
    } else {
        if (isFull) {
            attr->SetValue(ASTAttr::FULL);
        }
        if (isLite) {
            attr->SetValue(ASTAttr::LITE);
        }
        if (isMini) {
            attr->SetValue(ASTAttr::MINI);
        }
    }

    return attr;
}

AutoPtr<ASTExpr> Parser::ParseExpr()
{
    lexer_.SetParseMode(Lexer::ParseMode::EXPR_MODE);
    AutoPtr<ASTExpr> value = ParseAndExpr();
    lexer_.SetParseMode(Lexer::ParseMode::DECL_MODE);
    return value;
}

AutoPtr<ASTExpr> Parser::ParseAndExpr()
{
    AutoPtr<ASTExpr> left = ParseXorExpr();
    Token token = lexer_.PeekToken();
    while (token.kind == TokenType::AND) {
        lexer_.GetToken();
        AutoPtr<ASTBinaryExpr> expr = new ASTBinaryExpr;
        expr->op_ = BinaryOpKind::AND;
        expr->lExpr_ = left;
        expr->rExpr_ = ParseXorExpr();

        left = expr.Get();
        token = lexer_.PeekToken();
    }
    return left;
}

AutoPtr<ASTExpr> Parser::ParseXorExpr()
{
    AutoPtr<ASTExpr> left = ParseOrExpr();
    Token token = lexer_.PeekToken();
    while (token.kind == TokenType::XOR) {
        lexer_.GetToken();
        AutoPtr<ASTBinaryExpr> expr = new ASTBinaryExpr;
        expr->op_ = BinaryOpKind::XOR;
        expr->lExpr_ = left;
        expr->rExpr_ = ParseOrExpr();

        left = expr.Get();
        token = lexer_.PeekToken();
    }
    return left;
}

AutoPtr<ASTExpr> Parser::ParseOrExpr()
{
    AutoPtr<ASTExpr> left = ParseShiftExpr();
    Token token = lexer_.PeekToken();
    while (token.kind == TokenType::OR) {
        lexer_.GetToken();
        AutoPtr<ASTBinaryExpr> expr = new ASTBinaryExpr;
        expr->op_ = BinaryOpKind::OR;
        expr->lExpr_ = left;
        expr->rExpr_ = ParseShiftExpr();

        left = expr.Get();
        token = lexer_.PeekToken();
    }
    return left;
}

AutoPtr<ASTExpr> Parser::ParseShiftExpr()
{
    AutoPtr<ASTExpr> left = ParseAddExpr();
    Token token = lexer_.PeekToken();
    while (token.kind == TokenType::LEFT_SHIFT || token.kind == TokenType::RIGHT_SHIFT) {
        lexer_.GetToken();
        BinaryOpKind op = (token.kind == TokenType::LEFT_SHIFT) ? BinaryOpKind::LSHIFT : BinaryOpKind::RSHIFT;
        AutoPtr<ASTBinaryExpr> expr = new ASTBinaryExpr;
        expr->op_ = op;
        expr->lExpr_ = left;
        expr->rExpr_ = ParseAddExpr();

        left = expr.Get();
        token = lexer_.PeekToken();
    }
    return left;
}

AutoPtr<ASTExpr> Parser::ParseAddExpr()
{
    AutoPtr<ASTExpr> left = ParseMulExpr();
    Token token = lexer_.PeekToken();
    while (token.kind == TokenType::ADD || token.kind == TokenType::SUB) {
        lexer_.GetToken();
        BinaryOpKind op = (token.kind == TokenType::ADD) ? BinaryOpKind::ADD : BinaryOpKind::SUB;
        AutoPtr<ASTBinaryExpr> expr = new ASTBinaryExpr;
        expr->op_ = op;
        expr->lExpr_ = left;
        expr->rExpr_ = ParseMulExpr();

        left = expr.Get();
        token = lexer_.PeekToken();
    }
    return left;
}

AutoPtr<ASTExpr> Parser::ParseMulExpr()
{
    AutoPtr<ASTExpr> left = ParseUnaryExpr();
    Token token = lexer_.PeekToken();
    while (
        token.kind == TokenType::STAR || token.kind == TokenType::SLASH || token.kind == TokenType::PERCENT_SIGN) {
        lexer_.GetToken();
        BinaryOpKind op = BinaryOpKind::MUL;
        if (token.kind == TokenType::SLASH) {
            op = BinaryOpKind::DIV;
        } else if (token.kind == TokenType::PERCENT_SIGN) {
            op = BinaryOpKind::MOD;
        }
        AutoPtr<ASTBinaryExpr> expr = new ASTBinaryExpr;
        expr->op_ = op;
        expr->lExpr_ = left;
        expr->rExpr_ = ParseUnaryExpr();

        left = expr.Get();
        token = lexer_.PeekToken();
    }
    return left;
}

AutoPtr<ASTExpr> Parser::ParseUnaryExpr()
{
    Token token = lexer_.PeekToken();
    switch (token.kind) {
        case TokenType::ADD:
        case TokenType::SUB:
        case TokenType::TILDE: {
            lexer_.GetToken();
            AutoPtr<ASTUnaryExpr> expr = new ASTUnaryExpr;
            expr->op_ = UnaryOpKind::PLUS;
            if (token.kind == TokenType::SUB) {
                expr->op_ = UnaryOpKind::MINUS;
            } else if (token.kind == TokenType::TILDE) {
                expr->op_ = UnaryOpKind::TILDE;
            }

            expr->expr_ = ParseUnaryExpr();
            return expr.Get();
        }
        default:
            return ParsePrimaryExpr();
    }
}

AutoPtr<ASTExpr> Parser::ParsePrimaryExpr()
{
    Token token = lexer_.PeekToken();
    switch (token.kind) {
        case TokenType::PARENTHESES_LEFT: {
            lexer_.GetToken();
            AutoPtr<ASTExpr> expr = ParseExpr();
            token = lexer_.PeekToken();
            if (token.kind != TokenType::PARENTHESES_RIGHT) {
                LogError(token, StringHelper::Format("expected ')' before %s token", token.value.c_str()));
            } else {
                lexer_.GetToken();
                expr->isParenExpr = true;
            }
            return expr;
        }
        case TokenType::NUM:
            return ParseNumExpr();
        default:
            LogError(token, StringHelper::Format("this expression is not supported"));
            lexer_.SkipUntilToken(TokenType::COMMA);
            return nullptr;
    }
}

AutoPtr<ASTExpr> Parser::ParseNumExpr()
{
    Token token = lexer_.GetToken();
    if (!CheckNumber(token.value)) {
        LogError(token, StringHelper::Format("unknown integer number: '%s'", token.value.c_str()));
        return nullptr;
    }

    AutoPtr<ASTNumExpr> expr = new ASTNumExpr;
    expr->value_ = token.value;
    return expr.Get();
}

bool Parser::CheckNumber(const std::string& integerVal) const
{
    if (std::regex_match(integerVal, g_binaryNumRe)||
    std::regex_match(integerVal, g_octNumRe)||
    std::regex_match(integerVal, g_decNumRe)||
    std::regex_match(integerVal, g_hexNumRe)) {
        return true;
    }
    return false;
}

bool Parser::CheckType(const Token &token, const AutoPtr<ASTType> &type)
{
    if (type == nullptr) {
        return false;
    }

    if (!CheckTypeByMode(token, type)) {
        return false;
    }

    if (Options::GetInstance().GetLanguage() == Language::C) {
        if (type->IsSequenceableType() || type->IsSmqType() || type->IsAshmemType()) {
            LogError(token, StringHelper::Format("The %s type is not supported by c language.",
                type->ToString().c_str()));
            return false;
        }
    } else if (Options::GetInstance().GetLanguage() == Language::JAVA) {
        switch (type->GetTypeKind()) {
            case TypeKind::TYPE_UCHAR:
            case TypeKind::TYPE_USHORT:
            case TypeKind::TYPE_UINT:
            case TypeKind::TYPE_ULONG:
            case TypeKind::TYPE_ENUM:
            case TypeKind::TYPE_STRUCT:
            case TypeKind::TYPE_UNION:
            case TypeKind::TYPE_SMQ:
            case TypeKind::TYPE_UNKNOWN:
                LogError(token,
                    StringHelper::Format("The '%s' type is not supported by java language.", type->ToString().c_str()));
                return false;
            default:
                break;
        }
    }

    return true;
}

bool Parser::CheckTypeByMode(const Token &token, const AutoPtr<ASTType> &type)
{
    if (!Options::GetInstance().DoPassthrough() && type->IsPointerType()) {
        LogError(token, StringHelper::Format("The %s type is only supported by passthrough mode.",
            type->ToString().c_str()));
        return false;
    }

    if (Options::GetInstance().DoGenerateKernelCode()) {
        switch (type->GetTypeKind()) {
            case TypeKind::TYPE_FLOAT:
            case TypeKind::TYPE_DOUBLE:
            case TypeKind::TYPE_FILEDESCRIPTOR:
            case TypeKind::TYPE_INTERFACE:
            case TypeKind::TYPE_SMQ:
            case TypeKind::TYPE_ASHMEM:
            case TypeKind::TYPE_NATIVE_BUFFER:
            case TypeKind::TYPE_POINTER:
                LogError(token,
                    StringHelper::Format(
                        "The '%s' type is not supported by kernel mode.", type->ToString().c_str()));
                return false;
            default:
                break;
        }
    }
    return true;
}

void Parser::SetAstFileType()
{
    if (ast_->GetInterfaceDef() != nullptr) {
        if (ast_->GetInterfaceDef()->IsCallback()) {
            ast_->SetAStFileType(ASTFileType::AST_ICALLBACK);
        } else {
            ast_->SetAStFileType(ASTFileType::AST_IFACE);
        }
    } else {
        ast_->SetAStFileType(ASTFileType::AST_TYPES);
    }
}

bool Parser::CheckIntegrity()
{
    if (ast_ == nullptr) {
        LogError(std::string("ast is nullptr."));
        return false;
    }

    if (ast_->GetName().empty()) {
        LogError(std::string("ast's name is empty."));
        return false;
    }

    if (ast_->GetPackageName().empty()) {
        LogError(std::string("ast's package name is empty."));
        return false;
    }

    switch (ast_->GetASTFileType()) {
        case ASTFileType::AST_IFACE: {
            return CheckInterfaceAst();
        }
        case ASTFileType::AST_ICALLBACK: {
            return CheckCallbackAst();
        }
        case ASTFileType::AST_SEQUENCEABLE: {
            LogError(std::string("it's impossible that ast is sequenceable."));
            return false;
        }
        case ASTFileType::AST_TYPES: {
            if (ast_->GetInterfaceDef() != nullptr) {
                LogError(std::string("custom ast cannot has interface."));
                return false;
            }
            break;
        }
        default:
            break;
    }

    return true;
}

bool Parser::CheckInterfaceAst()
{
    AutoPtr<ASTInterfaceType> interface = ast_->GetInterfaceDef();
    if (interface == nullptr) {
        LogError(std::string("ast's interface is empty."));
        return false;
    }

    if (ast_->GetTypeDefinitionNumber() > 0) {
        LogError(std::string("interface ast cannot has custom types."));
        return false;
    }

    if (interface->GetMethodNumber() == 0) {
        LogError(std::string("interface ast has no method."));
        return false;
    }
    return true;
}

bool Parser::CheckCallbackAst()
{
    AutoPtr<ASTInterfaceType> interface = ast_->GetInterfaceDef();
    if (interface == nullptr) {
        LogError(std::string("ast's interface is empty."));
        return false;
    }

    if (!interface->IsCallback()) {
        LogError(std::string("ast is callback, but ast's interface is not callback."));
        return false;
    }
    return true;
}

/*
 * filePath: ./ohos/interface/foo/v1_0/IFoo.idl
 * package OHOS.Hdi.foo.v1_0;
 */
bool Parser::CheckPackageName(const std::string &filePath, const std::string &packageName) const
{
    std::string pkgToPath = Options::GetInstance().GetPackagePath(packageName);

    size_t index = filePath.rfind(SEPARATOR);
    if (index == std::string::npos) {
        return false;
    }

    std::string parentDir = filePath.substr(0, index);
    return parentDir == pkgToPath;
}

bool Parser::CheckImport(const std::string &importName)
{
    if (!std::regex_match(importName.c_str(), RE_IMPORT)) {
        LogError(StringHelper::Format("invalid impirt name '%s'", importName.c_str()));
        return false;
    }

    std::string idlFilePath = Options::GetInstance().GetImportFilePath(importName);
    if (!File::CheckValid(idlFilePath)) {
        LogError(StringHelper::Format("can not import '%s'", idlFilePath.c_str()));
        return false;
    }
    return true;
}

bool Parser::AddAst(const AutoPtr<AST> &ast)
{
    if (ast == nullptr) {
        LogError(std::string("ast is nullptr."));
        return false;
    }

    allAsts_[ast->GetFullName()] = ast;
    return true;
}

void Parser::LogError(const std::string &message)
{
    errors_.push_back(message);
}

void Parser::LogError(const Token &token, const std::string &message)
{
    errors_.push_back(StringHelper::Format("[%s] error:%s", LocInfo(token).c_str(), message.c_str()));
}

void Parser::ShowError()
{
    for (const auto &errMsg : errors_) {
        Logger::E(TAG, "%s", errMsg.c_str());
    }
}

void Parser::ParseInterfaceExtends(AutoPtr<ASTInterfaceType> &interface)
{
    Token token = lexer_.PeekToken();
    if (token.kind != TokenType::EXTENDS) {
        return;
    }
    lexer_.GetToken();
    token = lexer_.PeekToken();
    if (token.kind != TokenType::ID) {
        LogError(
            token, StringHelper::Format("expected  extends interface name before '%s' token", token.value.c_str()));
        lexer_.SkipToken(TokenType::BRACES_LEFT);
        return;
    }
    ParseExtendsInfo(interface);
    lexer_.GetToken();
}

void Parser::ParseExtendsInfo(AutoPtr<ASTInterfaceType> &interfaceType)
{
    Token token = lexer_.PeekToken();
    std::string extendsInterfaceName = token.value;
    if (extendsInterfaceName.empty()) {
        LogError(token, StringHelper::Format("extends interface name is empty"));
        return;
    }
    if (!CheckImport(extendsInterfaceName)) {
        LogError(token, StringHelper::Format("extends interface name is illegal"));
        return;
    }
    auto iter = allAsts_.find(extendsInterfaceName);
    AutoPtr<AST> extendsAst = (iter != allAsts_.end()) ? iter->second : nullptr;
    if (extendsAst == nullptr) {
        LogError(token,
            StringHelper::Format("can not find idl file by extends interface name '%s', please check import info",
                extendsInterfaceName.c_str()));
        return;
    }
    if (!CheckExtendsName(interfaceType, extendsInterfaceName)) {
        LogError(token,
            StringHelper::Format(
                "extends interface name must same as current interface name '%s'", interfaceType->GetName().c_str()));
        return;
    }
    if (!CheckExtendsVersion(interfaceType, extendsInterfaceName, extendsAst)) {
        LogError(token, StringHelper::Format("extends interface version must less than current interface version"));
        return;
    }
    if (!interfaceType->AddExtendsInterface(extendsAst->GetInterfaceDef())) {
        LogError(token, StringHelper::Format("multiple extends of '%s'", interfaceType->GetName().c_str()));
        return;
    }
}

bool Parser::CheckExtendsName(AutoPtr<ASTInterfaceType> &interfaceType, const std::string &extendsInterfaceName)
{
    size_t index = extendsInterfaceName.rfind(".");
    std::string interfaceName = interfaceType->GetName();
    if (extendsInterfaceName.substr(index + 1).compare(interfaceName) != 0) {
        return false;
    }
    return true;
}

bool Parser::CheckExtendsVersion(
    AutoPtr<ASTInterfaceType> &interfaceType, const std::string &extendsName, AutoPtr<AST> extendsAst)
{
    if (extendsAst->GetMajorVer() != ast_->GetMajorVer() || extendsAst->GetMinorVer() >= ast_->GetMinorVer()) {
        return false;
    }
    return true;
}

bool Parser::CheckImportsVersion(AutoPtr<AST> extendsAst)
{
    if (extendsAst->GetMajorVer() != ast_->GetMajorVer() || extendsAst->GetMinorVer() > ast_->GetMinorVer()) {
        return false;
    }
    return true;
}

void Parser::SetInterfaceVersion(AutoPtr<ASTInterfaceType> &interfaceType)
{
    size_t majorVer = ast_->GetMajorVer();
    size_t minorVer = ast_->GetMinorVer();
    interfaceType->SetVersion(majorVer, minorVer);
}

bool Parser::PostProcess()
{
    Language language = Options::GetInstance().GetLanguage();
    if (language != Language::C) {
        return true;
    }
    if (!CheckExistExtends()) {
        return true;
    }
    std::vector<size_t> genVersion = {0, 0};
    std::string genPackageName;
    AutoPtr<ASTNamespace> ns;
    if (!GetGenVersion(genVersion, genPackageName)) {
        return false;
    }
    GetGenNamespace(ns);
    AstMergeMap mergeMap;
    SortAstByName(mergeMap, allAsts_);
    allAsts_.clear();
    MergeAsts(mergeMap);
    ModifyImport(allAsts_, genPackageName);
    ModifyPackageNameAndVersion(allAsts_, genPackageName, genVersion);
    ModifyInterfaceNamespace(ns);

    return true;
}

bool Parser::CheckExistExtends()
{
    return std::any_of(allAsts_.begin(), allAsts_.end(), [](const std::pair<std::string, AutoPtr<AST>> &astPair) {
        return astPair.second->GetInterfaceDef() != nullptr &&
            astPair.second->GetInterfaceDef()->GetExtendsInterface() != nullptr;
    });
}

bool Parser::GetGenVersion(std::vector<size_t> &version, std::string &genPackageName)
{
    std::set<std::string> sourceFile = Options::GetInstance().GetSourceFiles();
    for (const auto &ast : allAsts_) {
        if (sourceFile.find(ast.second->GetIdlFilePath()) != sourceFile.end()) {
            if (genPackageName == "") {
                genPackageName = ast.second->GetPackageName();
                version[0] = ast.second->GetMajorVer();
                version[1] = ast.second->GetMinorVer();
                continue;
            }
            if (genPackageName != ast.second->GetPackageName() || version[0] != ast.second->GetMajorVer() ||
                version[1] != ast.second->GetMinorVer()) {
                LogError(StringHelper::Format("merge ast failed, source files must have same package and version"));
                return false;
            }
        }
    }
    return true;
}

void Parser::GetGenNamespace(AutoPtr<ASTNamespace> &ns)
{
    std::set<std::string> sourceFile = Options::GetInstance().GetSourceFiles();
    for (const auto &ast : allAsts_) {
        if (sourceFile.find(ast.second->GetIdlFilePath()) != sourceFile.end()) {
            if (ast.second->GetInterfaceDef() != nullptr) {
                ns = ast.second->GetInterfaceDef()->GetNamespace();
                return;
            }
        }
    }
}

void Parser::SortAstByName(AstMergeMap &mergeMap, StrAstMap &allAsts)
{
    for (const auto &astPair : allAsts) {
        AutoPtr<AST> ast = astPair.second;
        mergeMap[ast->GetName()].emplace(ast);
    }
}

void Parser::MergeAsts(AstMergeMap &mergeMap)
{
    for (const auto &setPair : mergeMap) {
        auto astSet = setPair.second;
        AutoPtr<AST> targetAst = nullptr;
        for (const auto &ast : astSet) {
            MergeAst(targetAst, ast);
        }
        AddAst(targetAst);
    }
}

void Parser::MergeAst(AutoPtr<AST> &targetAst, AutoPtr<AST> sourceAst)
{
    if (targetAst == nullptr) {
        targetAst = sourceAst;
        return;
    }
    MergeImport(targetAst, sourceAst);
    MergeInterfaceDef(targetAst, sourceAst);
    MergeTypeDefinitions(targetAst, sourceAst);
    MergeTypes(targetAst, sourceAst);
    MergeSequenceableDef(targetAst, sourceAst);
}

void Parser::MergeImport(AutoPtr<AST> &targetAst, AutoPtr<AST> sourceAst)
{
    for (const auto &importPair : sourceAst->GetImports()) {
        AutoPtr<AST> importAst = importPair.second;
        targetAst->AddImport(importAst);
    }
}

void Parser::MergeInterfaceDef(AutoPtr<AST> &targetAst, AutoPtr<AST> sourceAst)
{
    AutoPtr<ASTInterfaceType> sourceInterface = sourceAst->GetInterfaceDef();
    if (sourceInterface == nullptr) {
        return;
    }
    AutoPtr<ASTInterfaceType> targetInterface = targetAst->GetInterfaceDef();
    if (targetInterface == nullptr) {
        targetInterface = sourceInterface;
        return;
    }

    for (size_t i = 0; i < sourceInterface->GetMethodNumber(); i++) {
        targetInterface->AddMethod(sourceInterface->GetMethod(i));
    }
    targetInterface->SetSerializable(sourceInterface->IsSerializable());
}

void Parser::MergeTypeDefinitions(AutoPtr<AST> &targetAst, AutoPtr<AST> sourceAst)
{
    for (size_t i = 0; i < sourceAst->GetTypeDefinitionNumber(); i++) {
        targetAst->AddTypeDefinition(sourceAst->GetTypeDefintion(i));
    }
}

void Parser::MergeSequenceableDef(AutoPtr<AST> &targetAst, AutoPtr<AST> sourceAst)
{
    if (sourceAst->GetSequenceableDef() != nullptr) {
        targetAst->AddSequenceableDef(sourceAst->GetSequenceableDef());
    }
}

void Parser::MergeTypes(AutoPtr<AST> &targetAst, AutoPtr<AST> sourceAst)
{
    for (const auto &typePair : sourceAst->GetTypes()) {
        targetAst->AddType(typePair.second);
    }
}

void Parser::ModifyImport(StrAstMap &allAsts, std::string &genPackageName)
{
    for (const auto &astPair : allAsts) {
        StrAstMap modifiedImport;
        StrAstMap import = astPair.second->GetImports();
        for (const auto &importPair : import) {
            if (importPair.second->GetName() == astPair.second->GetName()) {
                continue;
            }
            modifiedImport[importPair.second->GetName()] = importPair.second;
        }
        astPair.second->ClearImport();
        for (const auto &importPair : modifiedImport) {
            importPair.second->SetPackageName(genPackageName);
            astPair.second->AddImport(importPair.second);
        }
    }
}

void Parser::ModifyPackageNameAndVersion(
    StrAstMap &allAsts, std::string &genPackageName, std::vector<size_t> genVersion)
{
    for (const auto &astPair : allAsts) {
        astPair.second->SetPackageName(genPackageName);
        astPair.second->SetVersion(genVersion[0], genVersion[1]);
    }
}

void Parser::ModifyInterfaceNamespace(AutoPtr<ASTNamespace> &ns)
{
    for (const auto &astPair : allAsts_) {
        AutoPtr<ASTInterfaceType> interface = astPair.second->GetInterfaceDef();
        if (interface != nullptr) {
            interface->SetNamespace(ns);
        }
    }
}
} // namespace HDI
} // namespace OHOS
