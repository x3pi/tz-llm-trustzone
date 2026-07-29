/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include <hash/hash.h>
#include <fstream>
#include <iostream>
#include <sstream>

#include "util/options.h"
#include "util/common.h"
#include "util/logger.h"

namespace OHOS {
namespace HDI {
bool Hash::GenHashKey()
{
    FileDetailMap fileDetails;
    if (!Preprocessor::UnitPreprocess(fileDetails)) {
        return false;
    }

    std::string filePath = Options::GetInstance().GetOutPutFile();
    return filePath.empty() ? FormatStdout(fileDetails) : FormatFile(fileDetails, filePath);
}

bool Hash::FormatStdout(const FileDetailMap &fileDetails)
{
    std::vector<std::string> hashInfos = Hash::GetHashInfo(fileDetails);
    if (hashInfos.empty()) {
        return false;
    }

    for (const auto &info : hashInfos) {
        std::cout << info << "\n";
    }
    return true;
}

bool Hash::FormatFile(const FileDetailMap &fileDetails, const std::string &filePath)
{
    std::vector<std::string> hashInfos = Hash::GetHashInfo(fileDetails);
    if (hashInfos.empty()) {
        return false;
    }

    std::ofstream hashFile(filePath, std::ios::out | std::ios ::binary);
    if (!hashFile.is_open()) {
        Logger::E(TAG, "failed to open %s", filePath.c_str());
        return false;
    }

    for (const auto &info : hashInfos) {
        hashFile << info << "\n";
    }

    hashFile.close();
    return true;
}

std::vector<std::string> Hash::GetHashInfo(const FileDetailMap &fileDetails)
{
    std::vector<std::string> hashInfos;
    for (const auto &detail : fileDetails) {
        size_t haskKey = 0;
        std::stringstream hashInfo;
        if (!Hash::GenFileHashKey(detail.second.filePath_, haskKey)) {
            return std::vector<std::string>();
        }
        hashInfo << detail.first << ":" << haskKey;
        hashInfos.push_back(hashInfo.str());
    }

    return hashInfos;
}

bool Hash::GenFileHashKey(const std::string &path, size_t &hashKey)
{
    std::ifstream fs(path);
    if (!fs.is_open()) {
        Logger::E(TAG, "invalid file path '%s'", path.c_str());
        return false;
    }

    std::stringstream buffer;
    buffer << fs.rdbuf();

    hashKey = std::hash<std::string>()(buffer.str());
    return true;
}
} // namespace HDI
} // namespace OHOS
