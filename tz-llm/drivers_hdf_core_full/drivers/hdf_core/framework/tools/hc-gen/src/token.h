/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#ifndef HC_GEN_TOKEN_H
#define HC_GEN_TOKEN_H

#include <memory>
#include <ostream>
#include <string>

namespace OHOS {
namespace Hardware {

enum TokenType {
    NUMBER = 256,
    TEMPLATE,
    LITERAL,
    STRING,
    REF_PATH,
    FILE_PATH,
    ROOT,
    INCLUDE,
    DELETE,
};

struct Token {
    int32_t type;
    std::string strval;
    uint64_t numval;
    std::shared_ptr<std::string> src;
    int32_t lineNo;

    Token();
    bool operator==(const Token &token) const;
    bool operator!=(const Token &token) const;
    bool operator==(int32_t otherType) const;
    bool operator!=(int32_t otherType) const;
    friend std::ostream &operator<<(std::ostream &stream, const Token &token);
};

std::ostream &operator<<(std::ostream &stream, const Token &token);
std::string TokenType2String(int32_t type);
} // namespace Hardware
} // namespace OHOS

#endif // HC_GEN_TOKEN_H
