/*
 * Copyright (c) 2021-2023 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#ifndef OHOS_HDI_CPP_CODE_EMITTER_H
#define OHOS_HDI_CPP_CODE_EMITTER_H

#include "ast/ast.h"
#include "codegen/code_emitter.h"
#include "util/autoptr.h"
#include "util/string_builder.h"

namespace OHOS {
namespace HDI {
class CppCodeEmitter : public CodeEmitter {
public:
    ~CppCodeEmitter() override = default;

    bool OutPut(const AutoPtr<AST> &ast, const std::string &targetDirectory);

protected:
    void GetStdlibInclusions(HeaderFile::HeaderFileSet &headerFiles);

    void GetImportInclusions(HeaderFile::HeaderFileSet &headerFiles);

    void EmitInterfaceMethodParameter(
        const AutoPtr<ASTParameter> &param, StringBuilder &sb, const std::string &prefix) const;

    void EmitLicense(StringBuilder &sb);

    void EmitHeadMacro(StringBuilder &sb, const std::string &fullName) const;

    void EmitTailMacro(StringBuilder &sb, const std::string &fullName) const;

    void EmitHeadExternC(StringBuilder &sb) const;

    void EmitTailExternC(StringBuilder &sb) const;

    bool IsVersion(const std::string &name) const;

    std::vector<std::string> EmitCppNameSpaceVec(const std::string &namespaceStr) const;

    std::string EmitPackageToNameSpace(const std::string &packageName) const;

    virtual void EmitBeginNamespace(StringBuilder &sb);

    virtual void EmitEndNamespace(StringBuilder &sb);

    virtual void EmitUsingNamespace(StringBuilder &sb);

    std::string EmitNamespace(const std::string &packageName) const;

    void EmitImportUsingNamespace(StringBuilder &sb);

    void EmitWriteMethodParameter(const AutoPtr<ASTParameter> &param, const std::string &parcelName, StringBuilder &sb,
        const std::string &prefix) const;

    void EmitReadMethodParameter(const AutoPtr<ASTParameter> &param, const std::string &parcelName, bool initVariable,
        StringBuilder &sb, const std::string &prefix) const;

    std::string MacroName(const std::string &name) const;

    std::string SpecificationParam(StringBuilder &paramSb, const std::string &prefix) const;

    std::string EmitHeaderNameByInterface(AutoPtr<ASTInterfaceType> interface, const std::string &name);

    std::string EmitDefinitionByInterface(AutoPtr<ASTInterfaceType> interface, const std::string &name) const;
};
} // namespace HDI
} // namespace OHOS

#endif // OHOS_HDI_CPP_CODE_EMITTER_H