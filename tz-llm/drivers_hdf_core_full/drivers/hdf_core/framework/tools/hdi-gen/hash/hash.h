/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#ifndef OHOS_HDI_HASH_H
#define OHOS_HDI_HASH_H

#include <string>
#include <vector>

#include "preprocessor/preprocessor.h"

namespace OHOS {
namespace HDI {
class Hash {
public:
    static bool GenHashKey();

private:
    static bool FormatStdout(const FileDetailMap &fileDetails);

    static bool FormatFile(const FileDetailMap &fileDetails, const std::string &filePath);

    static std::vector<std::string> GetHashInfo(const FileDetailMap &fileDetails);

    static bool GenFileHashKey(const std::string &path, size_t &hashKey);
};
} // namespace HDI
} // namespace OHOS

#endif // OHOS_HDI_HASH_H