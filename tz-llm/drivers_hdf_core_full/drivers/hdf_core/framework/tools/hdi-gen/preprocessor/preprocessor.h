/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#ifndef OHOS_HDI_PREPROCESSOR_H
#define OHOS_HDI_PREPROCESSOR_H

#include <map>
#include <set>
#include <unordered_set>
#include <vector>

#include "lexer/lexer.h"
#include "util/string_helper.h"

namespace OHOS {
namespace HDI {
class FileDetail {
public:
    inline std::string GetFullName() const
    {
        return StringHelper::Format("%s.%s", packageName_.c_str(), fileName_.c_str());
    }

    std::string Dump() const;

public:
    std::string filePath_;
    std::string fileName_;
    std::string packageName_;
    std::unordered_set<std::string> imports_;
};

using FileDetailMap = std::map<std::string, FileDetail>;

class Preprocessor {
public:
    // analyze idl files and return sorted ids files
    static bool Preprocess(std::vector<FileDetail> &fileDetails);

    static bool UnitPreprocess(FileDetailMap &fileDetails);

private:
    static bool CheckAllFilesPath(const std::set<std::string> &sourceFiles);

    static bool AnalyseImportInfo(std::set<std::string> sourceFiles, FileDetailMap &allFileDetails);

    static bool ParseFileDetail(const std::string &sourceFile, FileDetail &info);

    static bool ParsePackage(Lexer &lexer, FileDetail &info);

    static bool ParseImports(Lexer &lexer, FileDetail &info);

    static bool LoadOtherIdlFiles(
        const FileDetail &ownerFileDetail, FileDetailMap &allFileDetails, std::set<std::string> &sourceFiles);

    static bool CheckCircularReference(const FileDetailMap &allFileDetails,
        std::vector<FileDetail> &compileSourceFiles);

    static void PrintCyclefInfo(FileDetailMap &allFileDetails);

    static void FindCycle(const std::string &curNode, FileDetailMap &allFiles, std::vector<std::string> &trace);

    // check if the file path matches the package name
    static bool CheckPackageName(const std::string &filePath, const std::string &packageName);
};
} // namespace HDI
} // namespace OHOS

#endif // OHOS_HDI_PREPROCESSOR_H