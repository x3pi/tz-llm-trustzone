/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include "codegen/code_generator.h"
#include "parser/parser.h"
#include "preprocessor/preprocessor.h"
#include "util/file.h"
#include "util/logger.h"
#include "util/options.h"
#include "hash/hash.h"

using namespace OHOS::HDI;

int main(int argc, char **argv)
{
    Options &options = Options::GetInstance();
    if (!options.Parse(argc, argv)) {
        return -1;
    }

    if (options.DoShowUsage()) {
        options.ShowUsage();
        return 0;
    }
    if (options.DoShowVersion()) {
        options.ShowVersion();
        return 0;
    }

    if (options.DoGetHashKey()) {
        return Hash::GenHashKey() ? 0 : -1;
    }

    if (!options.DoCompile()) {
        return 0;
    }

    std::vector<FileDetail> fileDetails;
    if (!Preprocessor::Preprocess(fileDetails)) {
        Logger::E("MAIN", "failed to preprocess");
        return -1;
    }

    Parser parser;
    if (!parser.Parse(fileDetails)) {
        Logger::E("MAIN", "failed to parse file");
        return -1;
    }

    if (options.DoDumpAST()) {
        for (const auto &astPair : parser.GetAllAst()) {
            printf("%s\n", astPair.second->Dump("").c_str());
        }
    }

    if (!options.DoGenerateCode()) {
        return 0;
    }

    if (!CodeGenerator().Generate(parser.GetAllAst())) {
        Logger::E("hdi-gen", "failed to generate code");
        return -1;
    }
    return 0;
}