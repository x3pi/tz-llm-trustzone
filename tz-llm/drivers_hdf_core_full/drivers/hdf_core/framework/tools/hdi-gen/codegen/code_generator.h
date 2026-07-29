/*
 * Copyright (c) 2021-2022 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#ifndef OHOS_HDI_CODEGENERATOR_H
#define OHOS_HDI_CODEGENERATOR_H

#include <functional>
#include <map>
#include <string>

#include "codegen/code_emitter.h"

namespace OHOS {
namespace HDI {
using CodeEmitMap = std::unordered_map<std::string, AutoPtr<CodeEmitter>>;
using CodeGenFunc = std::function<void(const AutoPtr<AST>&, const std::string&)>;
using GeneratePolicies = std::map<SystemLevel, std::map<GenMode, std::map<Language, CodeGenFunc>>>;

class CodeGenerator : public LightRefCountBase {
public:
    using StrAstMap = std::unordered_map<std::string, AutoPtr<AST>>;

    bool Generate(const StrAstMap &allAst);

private:
    static CodeGenFunc GetCodeGenPoilcy();

    static void GenIpcCCode(const AutoPtr<AST> &ast, const std::string &outDir);

    static void GenIpcCppCode(const AutoPtr<AST> &ast, const std::string &outDir);

    static void GenIpcJavaCode(const AutoPtr<AST> &ast, const std::string &outDir);

    static void GenPassthroughCCode(const AutoPtr<AST> &ast, const std::string &outDir);

    static void GenPassthroughCppCode(const AutoPtr<AST> &ast, const std::string &outDir);

    static void GenKernelCode(const AutoPtr<AST> &ast, const std::string &outDir);

    static void GenLowCCode(const AutoPtr<AST> &ast, const std::string &outDir);

    static GeneratePolicies policies_;
    static CodeEmitMap cCodeEmitters_;
    static CodeEmitMap cppCodeEmitters_;
    static CodeEmitMap javaCodeEmitters_;
};
} // namespace HDI
} // namespace OHOS

#endif // OHOS_HDI_CODEGENERATOR_H