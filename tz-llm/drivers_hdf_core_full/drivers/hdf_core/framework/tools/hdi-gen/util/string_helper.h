/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#ifndef OHOS_HDI_STRING_HELPER_H
#define OHOS_HDI_STRING_HELPER_H

#include <cstring>
#include <string>
#include <vector>

namespace OHOS {
namespace HDI {
class StringHelper {
public:
    static std::vector<std::string> Split(std::string sources, const std::string &limit);

    static bool StartWith(const std::string &value, char prefix);

    static bool StartWith(const std::string &value, const std::string &prefix);

    static bool EndWith(const std::string &value, char suffix);

    static bool EndWith(const std::string &value, const std::string &suffix);

    static std::string Replace(const std::string &value, char oldChar, char newChar);

    static std::string Replace(const std::string &value, const std::string &oldstr, const std::string &newstr);

    static std::string Replace(
        const std::string &value, size_t position, const std::string &substr, const std::string &newstr);

    static std::string Replace(const std::string &value, size_t position, size_t len, const std::string &newStr);

    static std::string SubStr(const std::string &value, size_t start, size_t end = std::string::npos);

    static std::string StrToLower(const std::string &value);

    static std::string StrToUpper(const std::string &value);

    static std::string Format(const char *format, ...);

    static constexpr size_t lineMaxSize = 1024; // 1KB
    static constexpr size_t maxSize = 262144;    // 256KB
};
} // namespace HDI
} // namespace OHOS

#endif // OHOS_HDI_STRING_HELPER_H