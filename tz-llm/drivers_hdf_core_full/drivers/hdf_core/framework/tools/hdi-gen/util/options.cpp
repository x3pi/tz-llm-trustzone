/*
 * Copyright (c) 2021-2023 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include "util/options.h"

#include <cstdio>
#include <cstring>
#include <dirent.h>
#include <getopt.h>
#include <sys/stat.h>
#include <unistd.h>

#include <map>

#include "util/common.h"
#include "util/file.h"
#include "util/logger.h"
#include "util/string_helper.h"

namespace OHOS {
namespace HDI {
const char *Options::optSupportArgs = "hvs:m:l:p:c:d:r:o:D:";
static struct option g_longOpts[] = {
    {"help",         no_argument,       nullptr, 'h'},
    {"version",      no_argument,       nullptr, 'v'},
    {"system",       required_argument, nullptr, 's'},
    {"mode",         required_argument, nullptr, 'm'},
    {"language",     required_argument, nullptr, 'l'},
    {"package",      required_argument, nullptr, 'p'},
    {"dump-ast",     no_argument,       nullptr, 'a'},
    {"hash",         no_argument,       nullptr, 'H'},
    {nullptr,        0,                 nullptr, 0  }
};

Options &Options::GetInstance()
{
    static Options option;
    return option;
}

bool Options::Parse(int argc, char *argv[])
{
    int ret = true;
    program = argv[0];
    opterr = 1;
    int op = 0;
    int optIndex = 0;
    while ((op = getopt_long(argc, argv, optSupportArgs, g_longOpts, &optIndex)) != OPT_END) {
        switch (op) {
            case 'v':
                doShowVersion = true;
                break;
            case 's':
                ret = SetSystemLevel(optarg);
                break;
            case 'm':
                ret = SetGenerateMode(optarg);
                break;
            case 'l':
                SetLanguage(optarg);
                break;
            case 'p':
                SetPackage(optarg);
                break;
            case 'a':
                doDumpAST = true;
                break;
            case 'H':
                doHashKey = true;
                break;
            case 'c':
                AddSources(optarg);
                break;
            case 'D':
                AddSourcesByDir(optarg);
                break;
            case 'd':
                SetOutDir(optarg);
                break;
            case 'r':
                ret = AddPackagePath(optarg);
                break;
            case 'o':
                outPutFile = optarg;
                break;
            default:
                doShowUsage = true;
                break;
        }
    }
    return ret ? CheckOptions() : ret;
}

bool Options::SetSystemLevel(const std::string &system)
{
    static std::map<std::string, SystemLevel> systemLevelMap = {
        {"mini", SystemLevel::MINI},
        {"lite", SystemLevel::LITE},
        {"full", SystemLevel::FULL},
    };

    auto levelIter = systemLevelMap.find(system);
    if (levelIter == systemLevelMap.end()) {
        Logger::E(TAG, "invalid system level set: '%s', please input mini/lite/full", system.c_str());
        return false;
    }
    systemLevel = levelIter->second;
    return true;
}

bool Options::SetGenerateMode(const std::string &mode)
{
    static std::map<std::string, GenMode> codeGenMap = {
        {"low", GenMode::LOW},
        {"passthrough", GenMode::PASSTHROUGH},
        {"ipc", GenMode::IPC},
        {"kernel", GenMode::KERNEL},
    };

    auto codeGenIter = codeGenMap.find(mode);
    if (codeGenIter == codeGenMap.end()) {
        Logger::E(TAG, "invalid generate mode set: '%s', please input low/passthrough/ipc/kernel.", mode.c_str());
        return false;
    }
    genMode = codeGenIter->second;
    return true;
}

bool Options::SetLanguage(const std::string &language)
{
    static const std::map<std::string, Language> languageMap = {
        {"c", Language::C},
        {"cpp", Language::CPP},
        {"java", Language::JAVA},
    };

    const auto kindIter = languageMap.find(language);
    if (kindIter == languageMap.end()) {
        Logger::E(TAG, "invalid language '%s', please input c, cpp or java", language.c_str());
        return false;
    }

    doGenerateCode = true;
    genLanguage = kindIter->second;
    return true;
}

void Options::SetPackage(const std::string &infPackage)
{
    idlPackage = infPackage;
}

void Options::AddSources(const std::string &sourceFile)
{
    std::string realPath = File::AdapterRealPath(sourceFile);
    if (realPath.empty()) {
        Logger::E(TAG, "invalid idl file path:%s", sourceFile.c_str());
        return;
    }

    if (sourceFiles.insert(realPath).second == false) {
        Logger::E(TAG, "this idl file has been add:%s", sourceFile.c_str());
        return;
    }
    doCompile = true;
}

void Options::AddSourcesByDir(const std::string &dir)
{
    std::set<std::string> files = File::FindFiles(dir);
    if (!files.empty()) {
        doCompile = true;
        sourceFiles.insert(files.begin(), files.end());
    }
}

bool Options::AddPackagePath(const std::string &packagePath)
{
    size_t index = packagePath.find(":");
    if (index == std::string::npos || index == packagePath.size() - 1) {
        Logger::E(TAG, "invalid option parameters '%s'.", packagePath.c_str());
        return false;
    }

    std::string package = packagePath.substr(0, index);
    std::string path = File::AdapterRealPath(packagePath.substr(index + 1));
    if (path.empty()) {
        Logger::E(TAG, "invalid path '%s'.", packagePath.substr(index + 1).c_str());
        return false;
    }

    auto it = packagePathMap.find(package);
    if (it != packagePathMap.end()) {
        Logger::E(TAG, "The '%s:%s' has been set.", package.c_str(), path.c_str());
        return false;
    }

    packagePathMap[package] = path;
    return true;
}

void Options::SetOutDir(const std::string &dir)
{
    doOutDir = true;
    genDir = dir;
}

bool Options::CheckOptions()
{
    if (doShowUsage || doShowVersion) {
        return true;
    }

    if (doCompile) {
        if (!DoGetHashKey() && !doDumpAST && !doGenerateCode && !doOutDir) {
            Logger::E(TAG, "nothing to do.");
            return false;
        }

        if (!doGenerateCode && doOutDir) {
            Logger::E(TAG, "no target language.");
            return false;
        }

        if (doGenerateCode && !doOutDir) {
            Logger::E(TAG, "no out directory.");
            return false;
        }
    } else {
        if (DoGetHashKey() || doDumpAST || doGenerateCode || doOutDir) {
            Logger::E(TAG, "no idl files.");
            return false;
        }
    }
    return true;
}

void Options::ShowVersion() const
{
    printf("HDI-GEN: %d.%d\n"
           "Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.\n\n",
        VERSION_MAJOR, VERSION_MINOR);
}

void Options::ShowUsage() const
{
    printf("Compile a .idl file and generate C/C++ and Java codes.\n"
           "Usage: idl [options] file\n"
           "Options:\n"
           "  -h, --help                      Display command line options\n"
           "  -v, --version                   Display toolchain version information\n"
           "  -s, --system <value>            Set system level 'mini','lite' or 'full', the default value is 'full'\n"
           "  -m, --mode <value>              Set generate code mode 'low', 'passthrough', 'ipc' or 'kernel',"
           " the default value is 'ipc'\n"
           "  -l, --language <value>          Set language of generate code 'c','cpp','java' or 'hash',"
           " the default value is 'cpp'\n"
           "  -p, --package <package name>    Set package of idl files\n"
           "      --dump-ast                  Display the AST of the compiled file\n"
           "      --hash                      Generate hash info of idl files\n"
           "  -r <rootPackage>:<rootPath>     Set root path of root package\n"
           "  -c <*.idl>                      Compile the .idl file\n"
           "  -D <directory>                  Directory of the idl file\n"
           "  -d <directory>                  Place generated codes into <directory>\n"
           "  -o <file>                       Place the output into <file>\n");
}

/*
 * -r option: -r ohos.hdi:./drivers/interface
 * package:ohos.hdi.foo.v1_0
 * rootPackage:ohos.hdi
 */
std::string Options::GetRootPackage(const std::string &package) const
{
    const auto &packagePaths = GetPackagePathMap();
    for (const auto &packageRoot : packagePaths) {
        if (StringHelper::StartWith(package, packageRoot.first)) {
            return packageRoot.first;
        }
    }

    return "";
}

/*
 * -r option: -r ohos.hdi:./drivers/interface
 * package:ohos.hdi.foo.v1_0
 * rootPath:./drivers/interface
 */
std::string Options::GetRootPath(const std::string &package) const
{
    const auto &packagePaths = GetPackagePathMap();
    for (const auto &packageRoot : packagePaths) {
        if (StringHelper::StartWith(package, packageRoot.first)) {
            return packageRoot.second;
        }
    }

    return "";
}

/*
 * -r option: -r ohos.hdi:./drivers/interface
 * package:ohos.hdi.foo.v1_0
 * subPackage:foo.v1_0
 */
std::string Options::GetSubPackage(const std::string &package) const
{
    std::string rootPackage = GetRootPackage(package);
    if (rootPackage.empty()) {
        return package;
    }

    return package.substr(rootPackage.size() + 1);
}

/*
 * -r option: -r ohos.hdi:./drivers/interface
 * package:ohos.hdi.foo.v1_0
 * packagePath:./drivers/interface/foo/v1_0
 */
std::string Options::GetPackagePath(const std::string &package) const
{
    std::string rootPackage = "";
    std::string rootPath = "";
    const auto &packagePaths = GetPackagePathMap();
    for (const auto &packageRoot : packagePaths) {
        if (StringHelper::StartWith(package, packageRoot.first)) {
            rootPackage = packageRoot.first;
            rootPath = packageRoot.second;
        }
    }

    if (rootPackage.empty()) {
        // The current path is the root path
        std::string curPath = File::AdapterPath(StringHelper::Replace(package, '.', SEPARATOR));
        return File::AdapterRealPath(curPath);
    }

    if (StringHelper::EndWith(rootPath, SEPARATOR)) {
        rootPath.pop_back();
    }

    std::string subPath = StringHelper::Replace(package.substr(rootPackage.size() + 1), '.', SEPARATOR);
    return File::AdapterPath(rootPath + "/" + subPath);
}

/*
 * -r option: -r ohos.hdi:./drivers/interface
 * import: ohos.hdi.foo.v1_0.MyTypes
 * packagePath:./drivers/interface/foo/v1_0/MyTypes.idl
 */
std::string Options::GetImportFilePath(const std::string &import) const
{
    size_t index = import.rfind('.');
    if (index == std::string::npos) {
        return import;
    }

    std::string dir = GetPackagePath(StringHelper::SubStr(import, 0, index));
    std::string className = import.substr(index + 1);
    return StringHelper::Format("%s%c%s.idl", dir.c_str(), SEPARATOR, className.c_str());
}
} // namespace HDI
} // namespace OHOS