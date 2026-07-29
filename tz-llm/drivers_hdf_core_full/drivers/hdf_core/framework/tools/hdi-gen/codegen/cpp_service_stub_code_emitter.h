/*
 * Copyright (c) 2021-2023 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#ifndef OHOS_HDI_CPP_SERVICE_STUB_CODE_EMITTER_H
#define OHOS_HDI_CPP_SERVICE_STUB_CODE_EMITTER_H

#include "codegen/cpp_code_emitter.h"

namespace OHOS {
namespace HDI {
class CppServiceStubCodeEmitter : public CppCodeEmitter {
public:
    CppServiceStubCodeEmitter() : CppCodeEmitter() {}

    ~CppServiceStubCodeEmitter() override = default;

private:
    bool ResolveDirectory(const std::string &targetDirectory) override;

    void EmitCode() override;

    // ISample.idl -> sample_service_stub.h
    void EmitStubHeaderFile();

    void EmitStubHeaderInclusions(StringBuilder &sb);

    void GetHeaderOtherLibInclusions(HeaderFile::HeaderFileSet &headerFiles) const;

    void EmitStubUsingNamespace(StringBuilder &sb) const;

    void EmitStubDecl(StringBuilder &sb);

    void EmitStubBody(StringBuilder &sb, const std::string &prefix) const;

    void EmitStubConstructorDecl(StringBuilder &sb, const std::string &prefix) const;

    void EmitStubOnRequestDecl(StringBuilder &sb, const std::string &prefix) const;

    void EmitStubMethodDecls(StringBuilder &sb, const std::string &prefix) const;

    void EmitStubMethodDecl(const AutoPtr<ASTMethod> &method, StringBuilder &sb, const std::string &prefix) const;

    void EmitStubPrivateData(StringBuilder &sb, const std::string &prefix) const;

    // ISample.idl -> sample_service_stub.cpp
    void EmitStubSourceFile();

    void EmitStubSourceInclusions(StringBuilder &sb);

    void GetSourceOtherLibInclusions(HeaderFile::HeaderFileSet &headerFiles) const;

    void GetSourceOtherFileInclusions(HeaderFile::HeaderFileSet &headerFiles) const;

    void EmitInterfaceGetMethodImpl(StringBuilder &sb, const std::string &prefix) const;

    void EmitGetMethodImpl(StringBuilder &sb, const std::string &prefix) const;

    void EmitGetInstanceMethodImpl(StringBuilder &sb, const std::string &prefix) const;

    void EmitStubConstructorImpl(StringBuilder &sb, const std::string &prefix) const;

    void EmitStubOnRequestMethodImpl(StringBuilder &sb, const std::string &prefix);

    void EmitStubMethodImpls(StringBuilder &sb, const std::string &prefix) const;

    void EmitStubMethodImpl(AutoPtr<ASTInterfaceType> interface, const AutoPtr<ASTMethod> &method, StringBuilder &sb,
        const std::string &prefix) const;

    void EmitStubStaticMethodImpl(const AutoPtr<ASTMethod> &method, StringBuilder &sb, const std::string &prefix) const;

    void EmitStubCallMethod(const AutoPtr<ASTMethod> &method, StringBuilder &sb, const std::string &prefix) const;

    void EmitStubReadInterfaceToken(const std::string &parcelName, StringBuilder &sb, const std::string &prefix) const;

    void EmitStubReadMemFlag(const AutoPtr<ASTMethod> &method, const std::string &parcelName, StringBuilder &sb,
        const std::string &prefix) const;

    void EmitLocalVariable(const AutoPtr<ASTParameter> &param, const std::string &parcelName, StringBuilder &sb,
        const std::string &prefix) const;

    void GetUtilMethods(UtilMethodMap &methods) override;

    void EmitStubStaticMethodDecl(const AutoPtr<ASTMethod> &method, StringBuilder &sb, const std::string &prefix) const;
};
} // namespace HDI
} // namespace OHOS

#endif // OHOS_HDI_CPP_SERVICE_STUB_CODE_EMITTER_H