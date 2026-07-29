/*
 * Copyright (c) 2022-2023 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include "preprocessor/preprocessor.h"

#include <algorithm>
#include <queue>

#include "util/common.h"
#include "util/file.h"
#include "util/logger.h"
#include "util/options.h"
#include "util/string_builder.h"

namespace OHOS {
namespace HDI {
std::string FileDetail::Dump() const
{
    StringBuilder sb;
    sb.AppendFormat("filePath:%s\n", filePath_.c_str());
    sb.AppendFormat("fileName:%s\n", fileName_.c_str());
    sb.AppendFormat("packageName:%s\n", packageName_.c_str());
    if (imports_.size() == 0) {
        sb.Append("import:[]\n");
    } else {
        sb.Append("import:[\n");
        for (const auto &importStr : imports_) {
            sb.AppendFormat("%s,\n", importStr.c_str());
        }
        sb.Append("]\n");
    }
    return sb.ToString();
}

bool Preprocessor::Preprocess(std::vector<FileDetail> &fileDetails)
{
    std::set<std::string> sourceFiles = Options::GetInstance().GetSourceFiles();

    // check all path of idl files
    if (!CheckAllFilesPath(sourceFiles)) {
        return false;
    }

    // analyse impirt infomation of all idl file
    FileDetailMap allFileDetails;
    if (!AnalyseImportInfo(sourceFiles, allFileDetails)) {
        return false;
    }

    // calculate the order of idl files to compile by counter-topological sorting
    if (!CheckCircularReference(allFileDetails, fileDetails)) {
        return false;
    }

    return true;
}

bool Preprocessor::UnitPreprocess(FileDetailMap &fileDetails)
{
    std::set<std::string> sourceFiles = Options::GetInstance().GetSourceFiles();
    // check all path of idl files
    if (!CheckAllFilesPath(sourceFiles)) {
        return false;
    }

    for (const auto &sourceFile : sourceFiles) {
        FileDetail info;
        if (!ParseFileDetail(sourceFile, info)) {
            return false;
        }
        fileDetails[info.GetFullName()] = info;
    }

    return true;
}

bool Preprocessor::CheckAllFilesPath(const std::set<std::string> &sourceFiles)
{
    if (sourceFiles.empty()) {
        Logger::E(TAG, "no source files");
        return false;
    }

    bool ret = true;
    for (const auto &filePath : sourceFiles) {
        if (!File::CheckValid(filePath)) {
            Logger::E(TAG, "invailed file path '%s'.", filePath.c_str());
            ret = false;
        }
    }

    return ret;
}

bool Preprocessor::AnalyseImportInfo(std::set<std::string> sourceFiles, FileDetailMap &allFileDetails)
{
    std::set<std::string> processSource(sourceFiles);
    while (!processSource.empty()) {
        auto fileIter = processSource.begin();
        FileDetail info;
        if (!ParseFileDetail(*fileIter, info)) {
            return false;
        }

        processSource.erase(fileIter);
        allFileDetails[info.GetFullName()] = info;
        if (!LoadOtherIdlFiles(info, allFileDetails, processSource)) {
            Logger::E(TAG, "failed to load other by %s", info.GetFullName().c_str());
            return false;
        }
    }

    return true;
}

bool Preprocessor::ParseFileDetail(const std::string &sourceFile, FileDetail &info)
{
    Lexer lexer;
    if (!lexer.Reset(sourceFile)) {
        Logger::E(TAG, "failed to open file '%s'.", sourceFile.c_str());
        return false;
    }

    info.filePath_ = lexer.GetFilePath();
    size_t startIndex = info.filePath_.rfind(SEPARATOR);
    size_t endIndex = info.filePath_.rfind(".idl");
    if (startIndex == std::string::npos || endIndex == std::string::npos || (startIndex >= endIndex)) {
        Logger::E(TAG, "failed to get file name from '%s'.", info.filePath_.c_str());
        return false;
    }
    info.fileName_ = StringHelper::SubStr(info.filePath_, startIndex + 1, endIndex);

    if (!ParsePackage(lexer, info)) {
        return false;
    }

    if (!ParseImports(lexer, info)) {
        return false;
    }
    return true;
}

bool Preprocessor::ParsePackage(Lexer &lexer, FileDetail &info)
{
    Token token = lexer.PeekToken();
    if (token.kind != TokenType::PACKAGE) {
        Logger::E(TAG, "%s: expected 'package' before '%s' token", LocInfo(token).c_str(), token.value.c_str());
        return false;
    }
    lexer.GetToken();

    token = lexer.PeekToken();
    if (token.kind != TokenType::ID) {
        Logger::E(TAG, "%s: expected package name before '%s' token", LocInfo(token).c_str(), token.value.c_str());
        return false;
    }

    if (!CheckPackageName(info.filePath_, token.value)) {
        Logger::E(TAG, "%s:package name '%s' does not match file path '%s'", LocInfo(token).c_str(),
            token.value.c_str(), info.filePath_.c_str());
        return false;
    }
    info.packageName_ = token.value;
    lexer.GetToken();

    token = lexer.PeekToken();
    if (token.kind != TokenType::SEMICOLON) {
        Logger::E(TAG, "%s:expected ';' before '%s' token", LocInfo(token).c_str(), token.value.c_str());
        return false;
    }
    lexer.GetToken();
    return true;
}

bool Preprocessor::ParseImports(Lexer &lexer, FileDetail &info)
{
    Token token = lexer.PeekToken();
    while (token.kind != TokenType::END_OF_FILE) {
        if (token.kind != TokenType::IMPORT) {
            lexer.GetToken();
            token = lexer.PeekToken();
            continue;
        }

        lexer.GetToken();
        token = lexer.PeekToken();
        if (token.kind != TokenType::ID) {
            Logger::E(TAG, "%s: expected import name before '%s' token", LocInfo(token).c_str(), token.value.c_str());
            return false;
        }

        if (!File::CheckValid(Options::GetInstance().GetImportFilePath(token.value))) {
            Logger::E(TAG, "%s: import invalid package '%s'", LocInfo(token).c_str(), token.value.c_str());
            return false;
        }

        info.imports_.emplace(token.value);
        lexer.GetToken();

        token = lexer.PeekToken();
        if (token.kind != TokenType::SEMICOLON) {
            Logger::E(TAG, "%s:expected ';' before '%s' token", LocInfo(token).c_str(), token.value.c_str());
            return false;
        }
        lexer.GetToken();

        token = lexer.PeekToken();
    }
    return true;
}

bool Preprocessor::LoadOtherIdlFiles(
    const FileDetail &ownerFileDetail, FileDetailMap &allFileDetails, std::set<std::string> &sourceFiles)
{
    for (const auto &importName : ownerFileDetail.imports_) {
        if (allFileDetails.find(importName) != allFileDetails.end()) {
            continue;
        }

        std::string otherFilePath = Options::GetInstance().GetImportFilePath(importName);
        if (otherFilePath.empty()) {
            Logger::E(TAG, "importName:%s, is failed", importName.c_str());
            return false;
        }

        sourceFiles.insert(otherFilePath);
    }
    return true;
}

bool Preprocessor::CheckCircularReference(const FileDetailMap &allFileDetails,
    std::vector<FileDetail> &compileSourceFiles)
{
    FileDetailMap allFileDetailsTemp = allFileDetails;
    std::queue<FileDetail> fileQueue;
    for (const auto &filePair : allFileDetailsTemp) {
        const FileDetail &file = filePair.second;
        if (file.imports_.size() == 0) {
            fileQueue.push(file);
        }
    }

    compileSourceFiles.clear();
    while (!fileQueue.empty()) {
        FileDetail curFile = fileQueue.front();
        fileQueue.pop();
        compileSourceFiles.push_back(allFileDetails.at(curFile.GetFullName()));

        for (auto &filePair : allFileDetailsTemp) {
            FileDetail &otherFile = filePair.second;
            if (otherFile.imports_.empty()) {
                continue;
            }

            auto position = otherFile.imports_.find(curFile.GetFullName());
            if (position != otherFile.imports_.end()) {
                otherFile.imports_.erase(position);
            }

            if (otherFile.imports_.size() == 0) {
                fileQueue.push(otherFile);
            }
        }
    }

    if (compileSourceFiles.size() == allFileDetailsTemp.size()) {
        return true;
    }

    PrintCyclefInfo(allFileDetailsTemp);
    return false;
}

void Preprocessor::PrintCyclefInfo(FileDetailMap &allFileDetails)
{
    for (FileDetailMap::iterator it = allFileDetails.begin(); it != allFileDetails.end();) {
        if (it->second.imports_.size() == 0) {
            it = allFileDetails.erase(it);
        } else {
            ++it;
        }
    }

    for (const auto &filePair : allFileDetails) {
        std::vector<std::string> traceNodes;
        FindCycle(filePair.second.GetFullName(), allFileDetails, traceNodes);
    }
}

void Preprocessor::FindCycle(const std::string &curNode, FileDetailMap &allFiles, std::vector<std::string> &trace)
{
    auto iter = std::find_if(trace.begin(), trace.end(), [curNode](const std::string &name) {
        return name == curNode;
    });
    if (iter != trace.end()) {
        if (iter == trace.begin()) {
            // print circular reference infomation
            StringBuilder sb;
            for (const auto &nodeName : trace) {
                sb.AppendFormat("%s -> ", nodeName.c_str());
            }
            sb.AppendFormat("%s", curNode.c_str());
            Logger::E(TAG, "error: there are circular reference:\n%s", sb.ToString().c_str());
        }
        return;
    }

    trace.push_back(curNode);
    for (const auto &importFileName : allFiles[curNode].imports_) {
        FindCycle(importFileName, allFiles, trace);
    }

    trace.pop_back();
}

/*
 * filePath: ./ohos/interface/foo/v1_0/IFoo.idl
 * package ohos.hdi.foo.v1_0;
 */
bool Preprocessor::CheckPackageName(const std::string &filePath, const std::string &packageName)
{
    std::string pkgToPath = Options::GetInstance().GetPackagePath(packageName);

    size_t index = filePath.rfind(SEPARATOR);
    if (index == std::string::npos) {
        return false;
    }

    std::string parentDir = filePath.substr(0, index);
    return parentDir == pkgToPath;
}
} // namespace HDI
} // namespace OHOS