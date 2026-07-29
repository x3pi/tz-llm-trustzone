/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#ifndef OHOS_HDI_OPTION_H
#define OHOS_HDI_OPTION_H

#include <map>
#include <string>
#include <unordered_map>
#include <set>

#include "util/common.h"

namespace OHOS {
namespace HDI {
class Options {
public:
    using PkgPathMap = std::unordered_map<std::string, std::string>;

    static Options &GetInstance();

    Options(const Options &other) = delete;
    Options operator=(const Options &other) = delete;

    bool Parse(int argc, char *argv[]);

    ~Options() = default;

    inline bool DoShowUsage() const
    {
        return doShowUsage;
    }

    inline bool DoShowVersion() const
    {
        return doShowVersion;
    }

    inline bool DoCompile() const
    {
        return doCompile;
    }

    inline bool DoDumpAST() const
    {
        return doDumpAST;
    }

    inline bool DoGetHashKey() const
    {
        return doHashKey;
    }

    inline bool DoGenerateCode() const
    {
        return doGenerateCode;
    }

    inline bool DoGenerateKernelCode() const
    {
        return genMode == GenMode::KERNEL;
    }

    inline bool DoPassthrough() const
    {
        return genMode == GenMode::PASSTHROUGH;
    }

    inline std::set<std::string> GetSourceFiles() const
    {
        return sourceFiles;
    }

    inline PkgPathMap GetPackagePathMap() const
    {
        return packagePathMap;
    }

    inline std::string GetPackage() const
    {
        return idlPackage;
    }

    inline std::string GetGenerationDirectory() const
    {
        return genDir;
    }

    inline std::string GetOutPutFile() const
    {
        return outPutFile;
    }

    void ShowVersion() const;

    void ShowUsage() const;

    std::string GetRootPackage(const std::string &package) const;

    std::string GetRootPath(const std::string &package) const;

    std::string GetSubPackage(const std::string &package) const;

    std::string GetPackagePath(const std::string &package) const;

    std::string GetImportFilePath(const std::string &import) const;

    inline SystemLevel GetSystemLevel() const
    {
        return systemLevel;
    }

    inline GenMode GetGenMode() const
    {
        return genMode;
    }

    inline Language GetLanguage() const
    {
        return genLanguage;
    }

private:
    Options()
        : program(),
        systemLevel(SystemLevel::FULL),
        genMode(GenMode::IPC),
        genLanguage(Language::CPP),
        idlPackage(),
        sourceFiles(),
        genDir(),
        packagePathMap(),
        outPutFile(),
        doShowUsage(false),
        doShowVersion(false),
        doCompile(false),
        doDumpAST(false),
        doHashKey(false),
        doGenerateCode(false),
        doOutDir(false)
    {
    }

    void SetLongOption(char op);

    bool SetSystemLevel(const std::string &system);

    bool SetGenerateMode(const std::string &mode);

    bool SetLanguage(const std::string &language);

    void SetPackage(const std::string &package);

    bool AddPackagePath(const std::string &packagePath);

    void AddSources(const std::string &sourceFile);

    void AddSourcesByDir(const std::string &dir);

    void SetOutDir(const std::string &dir);

    void SetCodePart(const std::string &part);

    bool CheckOptions();

    static const char *optSupportArgs;
    static constexpr int OPT_END = -1;

    static constexpr int VERSION_MAJOR = 1;
    static constexpr int VERSION_MINOR = 0;

    std::string program;
    SystemLevel systemLevel;
    GenMode genMode;
    Language genLanguage;
    std::string idlPackage;
    std::set<std::string> sourceFiles;
    std::string genDir;
    PkgPathMap packagePathMap;
    std::string outPutFile;
    bool doShowUsage;
    bool doShowVersion;
    bool doCompile;
    bool doDumpAST;
    bool doHashKey;
    bool doGenerateCode;
    bool doOutDir;
};
} // namespace HDI
} // namespace OHOS

#endif // OHOS_HDIL_OPTION_H