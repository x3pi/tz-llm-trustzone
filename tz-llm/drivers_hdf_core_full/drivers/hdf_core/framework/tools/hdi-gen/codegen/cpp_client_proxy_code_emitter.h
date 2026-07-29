/*
 * Copyright (c) 2021-2023 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#ifndef OHOS_HDI_CPP_CLIENT_PROXY_CODE_EMITTER_H
#define OHOS_HDI_CPP_CLIENT_PROXY_CODE_EMITTER_H

#include "codegen/cpp_code_emitter.h"

namespace OHOS {
namespace HDI {
class CppClientProxyCodeEmitter : public CppCodeEmitter {
public:
    CppClientProxyCodeEmitter() : CppCodeEmitter() {}

    ~CppClientProxyCodeEmitter() override = default;

private:
    bool ResolveDirectory(const std::string &targetDirectory) override;

    void EmitCode() override;

    void EmitProxyHeaderFile();

    void EmitProxyHeaderInclusions(StringBuilder &sb);

    void GetHeaderOtherLibInclusions(HeaderFile::HeaderFileSet &headerFiles) const;

    void GetSourceOtherFileInclusions(HeaderFile::HeaderFileSet &headerFiles) const;

    void EmitProxyDecl(StringBuilder &sb, const std::string &prefix);

    void EmitProxyConstructor(StringBuilder &sb, const std::string &prefix) const;

    void EmitProxyMethodDecls(StringBuilder &sb, const std::string &prefix) const;

    void EmitProxyMethodDecl(
        const AutoPtr<ASTMethod> &method, StringBuilder &sb, const std::string &prefix) const;

    void EmitProxyConstants(StringBuilder &sb, const std::string &prefix) const;

    void EmitProxyMethodParameter(
        const AutoPtr<ASTParameter> &param, StringBuilder &sb, const std::string &prefix) const;

    void EmitPassthroughProxySourceFile();

    void EmitPassthroughProxySourceInclusions(StringBuilder &sb);

    void EmitPassthroughGetInstanceMethodImpl(StringBuilder &sb, const std::string &prefix) const;

    void EmitProxySourceFile();

    void EmitProxySourceInclusions(StringBuilder &sb);

    void GetSourceOtherLibInclusions(HeaderFile::HeaderFileSet &headerFiles) const;

    void EmitGetMethodImpl(StringBuilder &sb, const std::string &prefix) const;

    void EmitGetInstanceMethodImpl(StringBuilder &sb, const std::string &prefix);

    void EmitProxyPassthroughtLoadImpl(StringBuilder &sb, const std::string &prefix) const;

    void EmitProxyMethodImpls(StringBuilder &sb, const std::string &prefix);

    void EmitProxyMethodImpl(const AutoPtr<ASTInterfaceType> interface, const AutoPtr<ASTMethod> &method,
        StringBuilder &sb, const std::string &prefix);

    void EmitProxyMethodBody(const AutoPtr<ASTInterfaceType> interface, const AutoPtr<ASTMethod> &method,
        StringBuilder &sb, const std::string &prefix);

    void EmitWriteInterfaceToken(const std::string &parcelName, StringBuilder &sb, const std::string &prefix) const;

    void EmitWriteFlagOfNeedSetMem(const AutoPtr<ASTMethod> &method, const std::string &dataBufName, StringBuilder &sb,
        const std::string &prefix) const;

    void GetUtilMethods(UtilMethodMap &methods) override;

    void EmitProxyStaticMethodDecl(
        const AutoPtr<ASTMethod> &method, StringBuilder &sb, const std::string &prefix) const;

    void EmitProxyStaticMethodImpl(const AutoPtr<ASTMethod> &method, StringBuilder &sb, const std::string &prefix);

    void EmitProxyStaticMethodBody(const AutoPtr<ASTMethod> &method, StringBuilder &sb, const std::string &prefix);

    void EmitProxyIsProxyMethodImpl(StringBuilder &sb, const std::string &prefix) const;
    void EmitProxyCastFromMethodImpls(StringBuilder &sb, const std::string &prefix) const;
    void EmitProxyCastFromMethodImpl(const AutoPtr<ASTInterfaceType> interface, StringBuilder &sb,
        const std::string &prefix) const;
    void EmitProxyCastFromMethodImplTemplate(StringBuilder &sb, const std::string &prefix) const;
};
} // namespace HDI
} // namespace OHOS

#endif // OHOS_HDI_CPP_CLIENT_PROXY_CODE_EMITTER_H