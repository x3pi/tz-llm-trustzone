/*
 * Copyright (c) 2021-2022 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include "codegen/code_generator.h"
#include "codegen/c_client_proxy_code_emitter.h"
#include "codegen/c_custom_types_code_emitter.h"
#include "codegen/c_interface_code_emitter.h"
#include "codegen/c_service_driver_code_emitter.h"
#include "codegen/c_service_impl_code_emitter.h"
#include "codegen/c_service_stub_code_emitter.h"
#include "codegen/cpp_client_proxy_code_emitter.h"
#include "codegen/cpp_custom_types_code_emitter.h"
#include "codegen/cpp_interface_code_emitter.h"
#include "codegen/cpp_service_driver_code_emitter.h"
#include "codegen/cpp_service_impl_code_emitter.h"
#include "codegen/cpp_service_stub_code_emitter.h"
#include "codegen/java_client_interface_code_emitter.h"
#include "codegen/java_client_proxy_code_emitter.h"
#include "util/options.h"
#include "util/logger.h"

namespace OHOS {
namespace HDI {
GeneratePolicies CodeGenerator::policies_ = {
    {
        SystemLevel::MINI,
        {
            {
                GenMode::LOW,
                {
                    {Language::C, CodeGenerator::GenLowCCode}
                }
            }
        }
    },
    {
        SystemLevel::LITE,
        {
            {
                GenMode::KERNEL,
                {
                    {Language::C, CodeGenerator::GenKernelCode}
                }
            },
            {
                GenMode::PASSTHROUGH,
                {
                    {Language::C, CodeGenerator::GenPassthroughCCode},
                    {Language::CPP, CodeGenerator::GenPassthroughCppCode}
                }
            }
        }
    },
    {
        SystemLevel::FULL,
        {
            {
                GenMode::KERNEL,
                {
                    {Language::C, CodeGenerator::GenKernelCode}
                }
            },
            {
                GenMode::PASSTHROUGH,
                {
                    {Language::C, CodeGenerator::GenPassthroughCCode},
                    {Language::CPP, CodeGenerator::GenPassthroughCppCode}
                }
            },
            {
                GenMode::IPC,
                {
                    {Language::C, CodeGenerator::GenIpcCCode},
                    {Language::CPP, CodeGenerator::GenIpcCppCode},
                    {Language::JAVA, CodeGenerator::GenIpcJavaCode}
                }
            }
        }
    }
};

CodeEmitMap CodeGenerator::cCodeEmitters_ = {
    {"types",     new CCustomTypesCodeEmitter()  },
    {"interface", new CInterfaceCodeEmitter()    },
    {"proxy",     new CClientProxyCodeEmitter()  },
    {"driver",    new CServiceDriverCodeEmitter()},
    {"stub",      new CServiceStubCodeEmitter()  },
    {"service",   new CServiceImplCodeEmitter()  },
};

CodeEmitMap CodeGenerator::cppCodeEmitters_ = {
    {"types",     new CppCustomTypesCodeEmitter()  },
    {"interface", new CppInterfaceCodeEmitter()    },
    {"proxy",     new CppClientProxyCodeEmitter()  },
    {"driver",    new CppServiceDriverCodeEmitter()},
    {"stub",      new CppServiceStubCodeEmitter()  },
    {"service",   new CppServiceImplCodeEmitter()  },
};

CodeEmitMap CodeGenerator::javaCodeEmitters_ = {
    {"interface", new JavaClientInterfaceCodeEmitter()},
    {"proxy",     new JavaClientProxyCodeEmitter()    },
};

bool CodeGenerator::Generate(const StrAstMap &allAst)
{
    auto genCodeFunc = GetCodeGenPoilcy();
    if (!genCodeFunc) {
        return false;
    }

    std::string outDir = Options::GetInstance().GetGenerationDirectory();
    std::set<std::string> sourceFile = Options::GetInstance().GetSourceFiles();
    Language language = Options::GetInstance().GetLanguage();
    if (language == Language::CPP) {
        for (const auto &ast : allAst) {
            if (sourceFile.find(ast.second->GetIdlFilePath()) != sourceFile.end()) {
                genCodeFunc(ast.second, outDir);
            }
        }
    } else if (language == Language::C) {
        for (const auto &ast : allAst) {
            genCodeFunc(ast.second, outDir);
        }
    }

    return true;
}

CodeGenFunc CodeGenerator::GetCodeGenPoilcy()
{
    auto systemPolicies = policies_.find(Options::GetInstance().GetSystemLevel());
    if (systemPolicies == policies_.end()) {
        Logger::E(TAG, "the system level is not supported, please check option");
        return CodeGenFunc{};
    }

    auto genModePolicies = systemPolicies->second;
    auto genModeIter = genModePolicies.find(Options::GetInstance().GetGenMode());
    if (genModeIter == genModePolicies.end()) {
        Logger::E(TAG, "the generate mode is not supported, please check option");
        return CodeGenFunc{};
    }

    auto languagePolicies = genModeIter->second;
    auto languageIter = languagePolicies.find(Options::GetInstance().GetLanguage());
    if (languageIter == languagePolicies.end()) {
        Logger::E(TAG, "the language is not supported, please check option");
        return CodeGenFunc{};
    }
    return languageIter->second;
}

void CodeGenerator::GenIpcCCode(const AutoPtr<AST> &ast, const std::string &outDir)
{
    GenMode mode = GenMode::IPC;
    switch (ast->GetASTFileType()) {
        case ASTFileType::AST_TYPES: {
            cCodeEmitters_["types"]->OutPut(ast, outDir, mode);
            break;
        }
        case ASTFileType::AST_IFACE: {
            cCodeEmitters_["interface"]->OutPut(ast, outDir, mode);
            cCodeEmitters_["proxy"]->OutPut(ast, outDir, mode);
            cCodeEmitters_["driver"]->OutPut(ast, outDir, mode);
            cCodeEmitters_["stub"]->OutPut(ast, outDir, mode);
            cCodeEmitters_["service"]->OutPut(ast, outDir, mode);
            break;
        }
        case ASTFileType::AST_ICALLBACK: {
            cCodeEmitters_["interface"]->OutPut(ast, outDir, mode);
            cCodeEmitters_["proxy"]->OutPut(ast, outDir, mode);
            cCodeEmitters_["stub"]->OutPut(ast, outDir, mode);
            cCodeEmitters_["service"]->OutPut(ast, outDir, mode);
            break;
        }
        default:
            break;
    }
}

void CodeGenerator::GenIpcCppCode(const AutoPtr<AST> &ast, const std::string &outDir)
{
    GenMode mode = GenMode::IPC;
    switch (ast->GetASTFileType()) {
        case ASTFileType::AST_TYPES: {
            cppCodeEmitters_["types"]->OutPut(ast, outDir, mode);
            break;
        }
        case ASTFileType::AST_IFACE: {
            cppCodeEmitters_["interface"]->OutPut(ast, outDir, mode);
            cppCodeEmitters_["proxy"]->OutPut(ast, outDir, mode);
            cppCodeEmitters_["driver"]->OutPut(ast, outDir, mode);
            cppCodeEmitters_["stub"]->OutPut(ast, outDir, mode);
            cppCodeEmitters_["service"]->OutPut(ast, outDir, mode);
            break;
        }
        case ASTFileType::AST_ICALLBACK: {
            cppCodeEmitters_["interface"]->OutPut(ast, outDir, mode);
            cppCodeEmitters_["proxy"]->OutPut(ast, outDir, mode);
            cppCodeEmitters_["stub"]->OutPut(ast, outDir, mode);
            cppCodeEmitters_["service"]->OutPut(ast, outDir, mode);
            break;
        }
        default:
            break;
    }
}

void CodeGenerator::GenIpcJavaCode(const AutoPtr<AST> &ast, const std::string &outDir)
{
    GenMode mode = GenMode::IPC;
    switch (ast->GetASTFileType()) {
        case ASTFileType::AST_TYPES: {
            javaCodeEmitters_["types"]->OutPut(ast, outDir, mode);
            break;
        }
        case ASTFileType::AST_IFACE:
        case ASTFileType::AST_ICALLBACK: {
            javaCodeEmitters_["interface"]->OutPut(ast, outDir, mode);
            javaCodeEmitters_["proxy"]->OutPut(ast, outDir, mode);
            break;
        }
        default:
            break;
    }
}

void CodeGenerator::GenPassthroughCCode(const AutoPtr<AST> &ast, const std::string &outDir)
{
    GenMode mode = GenMode::PASSTHROUGH;
    switch (ast->GetASTFileType()) {
        case ASTFileType::AST_TYPES: {
            cCodeEmitters_["types"]->OutPut(ast, outDir, mode);
            break;
        }
        case ASTFileType::AST_IFACE: {
            cCodeEmitters_["interface"]->OutPut(ast, outDir, mode);
            cCodeEmitters_["proxy"]->OutPut(ast, outDir, mode);
            cCodeEmitters_["service"]->OutPut(ast, outDir, mode);
            break;
        }
        case ASTFileType::AST_ICALLBACK: {
            cCodeEmitters_["interface"]->OutPut(ast, outDir, mode);
            cCodeEmitters_["service"]->OutPut(ast, outDir, mode);
            break;
        }
        default:
            break;
    }
}

void CodeGenerator::GenPassthroughCppCode(const AutoPtr<AST> &ast, const std::string &outDir)
{
    GenMode mode = GenMode::PASSTHROUGH;
    switch (ast->GetASTFileType()) {
        case ASTFileType::AST_TYPES: {
            cppCodeEmitters_["types"]->OutPut(ast, outDir, mode);
            break;
        }
        case ASTFileType::AST_IFACE: {
            cppCodeEmitters_["interface"]->OutPut(ast, outDir, mode);
            cppCodeEmitters_["proxy"]->OutPut(ast, outDir, mode);
            cppCodeEmitters_["service"]->OutPut(ast, outDir, mode);
            break;
        }
        case ASTFileType::AST_ICALLBACK: {
            cppCodeEmitters_["interface"]->OutPut(ast, outDir, mode);
            cppCodeEmitters_["service"]->OutPut(ast, outDir, mode);
            break;
        }
        default:
            break;
    }
}

void CodeGenerator::GenKernelCode(const AutoPtr<AST> &ast, const std::string &outDir)
{
    GenMode mode = GenMode::KERNEL;
    switch (ast->GetASTFileType()) {
        case ASTFileType::AST_TYPES: {
            cCodeEmitters_["types"]->OutPut(ast, outDir, mode);
            break;
        }
        case ASTFileType::AST_IFACE: {
            cCodeEmitters_["interface"]->OutPut(ast, outDir, mode);
            cCodeEmitters_["proxy"]->OutPut(ast, outDir, mode);
            cCodeEmitters_["driver"]->OutPut(ast, outDir, mode);
            cCodeEmitters_["stub"]->OutPut(ast, outDir, mode);
            cCodeEmitters_["service"]->OutPut(ast, outDir, mode);
            break;
        }
        default:
            break;
    }
}

void CodeGenerator::GenLowCCode(const AutoPtr<AST> &ast, const std::string &outDir)
{
    GenMode mode = GenMode::LOW;
    switch (ast->GetASTFileType()) {
        case ASTFileType::AST_TYPES: {
            cCodeEmitters_["types"]->OutPut(ast, outDir, mode);
            break;
        }
        case ASTFileType::AST_IFACE: {
            cCodeEmitters_["interface"]->OutPut(ast, outDir, mode);
            cCodeEmitters_["driver"]->OutPut(ast, outDir, mode);
            cCodeEmitters_["service"]->OutPut(ast, outDir, mode);
            break;
        }
        case ASTFileType::AST_ICALLBACK: {
            cCodeEmitters_["interface"]->OutPut(ast, outDir, mode);
            cCodeEmitters_["service"]->OutPut(ast, outDir, mode);
            break;
        }
        default:
            break;
    }
}
} // namespace HDI
} // namespace OHOS