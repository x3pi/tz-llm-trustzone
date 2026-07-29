/*
 * Copyright (c) 2022-2023 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include "lexer/lexer.h"
#include "util/logger.h"
#include "util/string_builder.h"
#include "util/string_helper.h"

namespace OHOS {
namespace HDI {
Lexer::StrTokenTypeMap Lexer::keyWords_ = {
    {"boolean",        TokenType::BOOLEAN      },
    {"byte",           TokenType::BYTE         },
    {"short",          TokenType::SHORT        },
    {"int",            TokenType::INT          },
    {"long",           TokenType::LONG         },
    {"String",         TokenType::STRING       },
    {"float",          TokenType::FLOAT        },
    {"double",         TokenType::DOUBLE       },
    {"FileDescriptor", TokenType::FD           },
    {"Ashmem",         TokenType::ASHMEM       },
    {"NativeBuffer",   TokenType::NATIVE_BUFFER},
    {"Pointer",        TokenType::POINTER      },
    {"List",           TokenType::LIST         },
    {"Map",            TokenType::MAP          },
    {"SharedMemQueue", TokenType::SMQ          },
    {"char",           TokenType::CHAR         },
    {"unsigned",       TokenType::UNSIGNED     },
    {"enum",           TokenType::ENUM         },
    {"struct",         TokenType::STRUCT       },
    {"union",          TokenType::UNION        },
    {"package",        TokenType::PACKAGE      },
    {"sequenceable",   TokenType::SEQ          },
    {"import",         TokenType::IMPORT       },
    {"interface",      TokenType::INTERFACE    },
    {"extends",        TokenType::EXTENDS      },
    {"oneway",         TokenType::ONEWAY       },
    {"callback",       TokenType::CALLBACK     },
    {"full",           TokenType::FULL         },
    {"lite",           TokenType::LITE         },
    {"mini",           TokenType::MINI         },
    {"in",             TokenType::IN           },
    {"out",            TokenType::OUT          },
};

Lexer::StrTokenTypeMap Lexer::symbols_ = {
    {".",  TokenType::DOT                 },
    {",",  TokenType::COMMA               },
    {":",  TokenType::COLON               },
    {"=",  TokenType::ASSIGN              },
    {";",  TokenType::SEMICOLON           },
    {"{",  TokenType::BRACES_LEFT         },
    {"}",  TokenType::BRACES_RIGHT        },
    {"[",  TokenType::BRACKETS_LEFT       },
    {"]",  TokenType::BRACKETS_RIGHT      },
    {"(",  TokenType::PARENTHESES_LEFT    },
    {")",  TokenType::PARENTHESES_RIGHT   },
    {"<",  TokenType::ANGLE_BRACKETS_LEFT },
    {">",  TokenType::ANGLE_BRACKETS_RIGHT},
    {"+",  TokenType::ADD                 },
    {"-",  TokenType::SUB                 },
    {"*",  TokenType::STAR                },
    {"/",  TokenType::SLASH               },
    {"%",  TokenType::PERCENT_SIGN        },
    {"<<", TokenType::LEFT_SHIFT          },
    {">>", TokenType::RIGHT_SHIFT         },
    {"&",  TokenType::AND                 },
    {"^",  TokenType::XOR                 },
    {"|",  TokenType::OR                  },
    {"~",  TokenType::TILDE               },
    {"++", TokenType::PPLUS               },
    {"--", TokenType::MMINUS              },
};

Lexer::Lexer() : filePath_(), file_(nullptr), mode_(ParseMode::DECL_MODE), havePeek_(false), curToken_() {}

bool Lexer::Reset(const std::string &filePath)
{
    file_ = std::make_unique<File>(filePath, int(File::READ));
    if (file_ == nullptr || !file_->IsValid()) {
        return false;
    }

    havePeek_ = false;
    InitCurToken(curToken_);
    return true;
}

Token Lexer::PeekToken(bool skipComment)
{
    if (!havePeek_) {
        ReadToken(curToken_, skipComment);
        havePeek_ = true;
    }
    return curToken_;
}

Token Lexer::GetToken(bool skipComment)
{
    if (!havePeek_) {
        ReadToken(curToken_, skipComment);
    }
    havePeek_ = false;
    return curToken_;
}

void Lexer::SkipCurrentLine()
{
    while (!file_->IsEof()) {
        char c = file_->GetChar();
        if (c == '\n') {
            file_->GetChar();
            break;
        }
    }
    havePeek_ = false;
}

bool Lexer::SkipCurrentLine(char untilChar)
{
    bool ret = true;
    while (!file_->IsEof()) {
        int c = file_->GetChar();
        if (c == untilChar) {
            ret = true;
            break;
        }
        if (c == '\n') {
            file_->GetChar();
            ret = false;
            break;
        }
    }
    havePeek_ = false;
    return ret;
}

void Lexer::Skip(char untilChar)
{
    while (!file_->IsEof()) {
        int c = file_->GetChar();
        if (c == untilChar) {
            break;
        }
    }
    havePeek_ = false;
}

void Lexer::SkipToken(TokenType tokenType)
{
    while (curToken_.kind != tokenType) {
        GetToken(false);
    }
}

void Lexer::SkipUntilToken(TokenType tokenType)
{
    Token token = PeekToken();
    while (token.kind != tokenType) {
        GetToken(false);
        token = PeekToken();
    }
}

void Lexer::SkipEof()
{
    while (!file_->IsEof()) {}
    havePeek_ = false;
}

void Lexer::ReadToken(Token &token, bool skipComment)
{
    if (!file_->IsEof()) {
        InitCurToken(token);
    }
    while (!file_->IsEof()) {
        char c = file_->PeekChar();
        if (isspace(c)) {
            file_->GetChar();
            continue;
        }
        token.location.row = file_->GetCharLineNumber();
        token.location.col = file_->GetCharColumnNumber();
        if (isalpha(c) || c == '_') {
            ReadId(token);
            return;
        } else if (isdigit(c)) {
            ReadNum(token);
            return;
        } else if (c == '<') {
            ReadShiftLeftOp(token);
            return;
        } else if (c == '>') {
            ReadShiftRightOp(token);
            return;
        } else if (c == '+') {
            ReadPPlusOp(token);
            return;
        } else if (c == '-') {
            ReadMMinusOp(token);
            return;
        } else if (c == '/') {
            ReadComment(token);
            if ((token.kind == TokenType::COMMENT_BLOCK || token.kind == TokenType::COMMENT_LINE) && skipComment) {
                InitCurToken(token);
                continue;
            }
            return;
        }
        ReadSymbolToken(token);
        return;
    }
    token.kind = TokenType::END_OF_FILE;
    token.value = "";
}

void Lexer::InitCurToken(Token &token)
{
    token.kind = TokenType::UNKNOWN;
    token.location.filePath = file_->GetPath();
    token.location.row = 1;
    token.location.col = 1;
    token.value = "";
}

void Lexer::ReadId(Token &token)
{
    char c = file_->GetChar();
    StringBuilder sb;
    sb.Append(c);
    while (!file_->IsEof()) {
        c = file_->PeekChar();
        if (isalpha(c) || isdigit(c) || c == '_' || c == '.') {
            c = file_->GetChar();
            sb.Append(c);
            continue;
        }
        if (isspace(c)) {
            file_->GetChar();
        }
        break;
    }

    std::string key = sb.ToString();
    auto it = keyWords_.find(key);
    token.kind = (it != keyWords_.end()) ? it->second : TokenType::ID;
    token.value = sb.ToString();
}

void Lexer::ReadNum(Token &token)
{
    char c = file_->PeekChar();
    if (c == '0') {
        file_->GetChar();
        c = file_->PeekChar();
        if (c == 'b' || c == 'B') {
            // binary number
            ReadBinaryNum(token);
        } else if (isdigit(c)) {
            // octal number
            return ReadOctNum(token);
        } else if (c == 'X' || c == 'x') {
            // hexadecimal number
            return ReadHexNum(token);
        } else {
            // decimal number 0
            token.kind = TokenType::NUM;
            token.value = "0";
        }
    } else {
        ReadDecNum(token);
    }
    ReadNumSuffix(token);
}

void Lexer::ReadBinaryNum(Token &token)
{
    StringBuilder sb;
    char c = file_->GetChar(); // read 'b' or 'B'
    sb.AppendFormat("0%c", c);
    bool err = true;

    while (!file_->IsEof()) {
        c = file_->PeekChar();
        if (c == '0' || c == '1') {
            sb.Append(c);
            file_->GetChar();
            err = false;
        } else {
            break;
        }
    }

    token.kind = err ? TokenType::UNKNOWN : TokenType::NUM;
    token.value = sb.ToString();
}

void Lexer::ReadOctNum(Token &token)
{
    StringBuilder sb;
    sb.Append("0");
    bool err = false;

    while (!file_->IsEof()) {
        char c = file_->PeekChar();
        if (!isdigit(c)) {
            break;
        }

        if (!(c >= '0' && c <= '7')) {
            err = true;
        }
        sb.Append(c);
        file_->GetChar();
    }

    token.kind = err ? TokenType::UNKNOWN : TokenType::NUM;
    token.value = sb.ToString();
}

void Lexer::ReadHexNum(Token &token)
{
    StringBuilder sb;
    char c = file_->GetChar(); // read 'x' or 'X'
    sb.AppendFormat("0%c", c);
    bool err = true;

    while (!file_->IsEof()) {
        c = file_->PeekChar();
        if (isdigit(c) || ((c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'))) {
            sb.Append(c);
            file_->GetChar();
            err = false;
        } else {
            break;
        }
    }

    token.kind = err ? TokenType::UNKNOWN : TokenType::NUM;
    token.value = sb.ToString();
}

void Lexer::ReadDecNum(Token &token)
{
    StringBuilder sb;
    char c = file_->GetChar();
    sb.Append(c);

    while (!file_->IsEof()) {
        c = file_->PeekChar();
        if (!isdigit(c)) {
            break;
        }

        sb.Append(c);
        file_->GetChar();
    }

    token.kind = TokenType::NUM;
    token.value = sb.ToString();
}

void Lexer::ReadNumSuffix(Token &token)
{
    while (!file_->IsEof()) {
        char c = file_->PeekChar();
        if (isalpha(c) || isdigit(c) || c == '_' || c == '.') {
            token.value += c;
            file_->GetChar();
        } else {
            break;
        }
    }
}

void Lexer::ReadShiftLeftOp(Token &token)
{
    char c = file_->GetChar();
    char next = file_->PeekChar();
    if (next == '<') {
        file_->GetChar();
        token.kind = TokenType::LEFT_SHIFT;
        token.value = "<<";
        return;
    }

    std::string symbol = StringHelper::Format("%c", c);
    auto iter = symbols_.find(symbol);
    token.kind = (iter != symbols_.end()) ? iter->second : TokenType::UNKNOWN;
    token.value = symbol;
}

void Lexer::ReadShiftRightOp(Token &token)
{
    char c = file_->GetChar();
    char next = file_->PeekChar();
    if (next == '>' && mode_ == ParseMode::EXPR_MODE) {
        file_->GetChar();
        token.kind = TokenType::RIGHT_SHIFT;
        token.value = ">>";
        return;
    }

    std::string symbol = StringHelper::Format("%c", c);
    auto iter = symbols_.find(symbol);
    token.kind = (iter != symbols_.end()) ? iter->second : TokenType::UNKNOWN;
    token.value = symbol;
}

void Lexer::ReadPPlusOp(Token &token)
{
    char c = file_->GetChar();
    char next = file_->PeekChar();
    if (next == '+') {
        file_->GetChar();
        token.kind = TokenType::PPLUS;
        token.value = "++";
        return;
    }

    std::string symbol = StringHelper::Format("%c", c);
    auto iter = symbols_.find(symbol);
    token.kind = (iter != symbols_.end()) ? iter->second : TokenType::UNKNOWN;
    token.value = symbol;
}

void Lexer::ReadMMinusOp(Token &token)
{
    char c = file_->GetChar();
    char next = file_->PeekChar();
    if (next == '-') {
        file_->GetChar();
        token.kind = TokenType::MMINUS;
        token.value = "--";
        return;
    }

    std::string symbol = StringHelper::Format("%c", c);
    auto iter = symbols_.find(symbol);
    token.kind = (iter != symbols_.end()) ? iter->second : TokenType::UNKNOWN;
    token.value = symbol;
}

void Lexer::ReadComment(Token &token)
{
    char c = file_->GetChar();
    char next = file_->PeekChar();
    if (next == '/') {
        ReadLineComment(token);
        return;
    } else if (next == '*') {
        ReadBlockComment(token);
        return;
    }

    std::string symbol = StringHelper::Format("%c", c);
    auto iter = symbols_.find(symbol);
    token.kind = (iter != symbols_.end()) ? iter->second : TokenType::UNKNOWN;
    token.value = symbol;
}

void Lexer::ReadLineComment(Token &token)
{
    StringBuilder sb;
    char c = file_->GetChar();
    sb.AppendFormat("/%c", c);

    while (!file_->IsEof()) {
        c = file_->GetChar();
        if (c == '\n') {
            break;
        }
        sb.Append(c);
    }

    token.kind = TokenType::COMMENT_LINE;
    token.value = sb.ToString();
}

void Lexer::ReadBlockComment(Token &token)
{
    StringBuilder sb;
    char c = file_->GetChar();
    sb.AppendFormat("/%c", c);

    while (!file_->IsEof()) {
        c = file_->GetChar();
        sb.Append(c);

        if (c == '*' && file_->PeekChar() == '/') {
            c = file_->GetChar();
            sb.Append(c);
            break;
        }
    }

    token.kind = TokenType::COMMENT_BLOCK;
    token.value = sb.ToString();
}

void Lexer::ReadSymbolToken(Token &token)
{
    char c = file_->GetChar();
    std::string symbol = StringHelper::Format("%c", c);
    auto iter = symbols_.find(symbol);
    token.kind = (iter != symbols_.end()) ? iter->second : TokenType::UNKNOWN;
    token.value = symbol;
}
} // namespace HDI
} // namespace OHOS