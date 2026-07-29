/*
 * Copyright (c) 2022-2023 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include "lexer/token.h"

#include <unordered_map>

#include "util/common.h"
#include "util/file.h"
#include "util/string_builder.h"
#include "util/string_helper.h"

namespace OHOS {
namespace HDI {
std::string Token::Dump()
{
    StringBuilder sb;
    sb.AppendFormat("{kind:%u, row:%u, col:%u, value:%s}",
        static_cast<size_t>(kind), location.row, location.col, value.c_str());
    return sb.ToString();
}

std::string LocInfo(const Token &token)
{
    size_t index = token.location.filePath.rfind(SEPARATOR);
    std::string fileName =
        (index == std::string::npos) ? token.location.filePath : token.location.filePath.substr(index + 1);
    return StringHelper::Format("%s:%zu:%zu", fileName.c_str(), token.location.row, token.location.col);
}
} // namespace HDI
} // namespace OHOS