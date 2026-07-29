/*
 * Copyright (c) 2021-2023 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#ifndef OHOS_HDI_JAVA_CLIENT_PROXY_CODE_EMITTER_H
#define OHOS_HDI_JAVA_CLIENT_PROXY_CODE_EMITTER_H

#include "java_code_emitter.h"
#include "util/file.h"

namespace OHOS {
namespace HDI {
class JavaClientProxyCodeEmitter : public JavaCodeEmitter {
public:
    JavaClientProxyCodeEmitter() : JavaCodeEmitter() {}

    ~JavaClientProxyCodeEmitter() override = default;

private:
    bool ResolveDirectory(const std::string &targetDirectory) override;

    void EmitCode() override;

    void EmitProxyFile();

    void EmitProxyImports(StringBuilder &sb) const;

    void EmitProxyCorelibImports(StringBuilder &sb) const;

    void EmitProxySelfDefinedTypeImports(StringBuilder &sb) const;

    void EmitProxyDBinderImports(StringBuilder &sb) const;

    void EmitProxyImpl(StringBuilder &sb);

    void EmitProxyConstants(StringBuilder &sb, const std::string &prefix);

    void EmitProxyConstructor(StringBuilder &sb, const std::string &prefix) const;

    void EmitProxyMethodImpls(StringBuilder &sb, const std::string &prefix) const;

    void EmitProxyMethodImpl(const AutoPtr<ASTMethod> &method, StringBuilder &sb, const std::string &prefix) const;

    void EmitInterfaceMethodParameter(
        const AutoPtr<ASTParameter> &param, StringBuilder &sb, const std::string &prefix) const;

    void EmitProxyMethodBody(const AutoPtr<ASTMethod> &method, StringBuilder &sb, const std::string &prefix) const;

    void EmitReadMethodParameter(const AutoPtr<ASTParameter> &param, const std::string &parcelName, StringBuilder &sb,
        const std::string &prefix) const;

    void EmitReadVariable(const std::string &parcelName, const std::string &name, const AutoPtr<ASTType> &type,
        ParamAttr attribute, StringBuilder &sb, const std::string &prefix);

    void EmitReadArrayVariable(const std::string &parcelName, const std::string &name,
        const AutoPtr<ASTArrayType> &arrayType, ParamAttr attribute, StringBuilder &sb,
        const std::string &prefix) const;

    void EmitReadOutArrayVariable(const std::string &parcelName, const std::string &name,
        const AutoPtr<ASTArrayType> &arrayType, StringBuilder &sb, const std::string &prefix);

    void EmitReadOutVariable(const std::string &parcelName, const std::string &name, const AutoPtr<ASTType> &type,
        StringBuilder &sb, const std::string &prefix);

    void EmitLocalVariable(const AutoPtr<ASTParameter> &param, StringBuilder &sb, const std::string &prefix) const;
};
} // namespace HDI
} // namespace OHOS

#endif // OHOS_HDI_JAVA_CLIENT_PROXY_CODE_EMITTER_H