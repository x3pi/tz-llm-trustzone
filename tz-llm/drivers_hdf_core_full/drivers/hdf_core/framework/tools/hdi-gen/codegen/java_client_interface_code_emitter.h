/*
 * Copyright (c) 2021-2023 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#ifndef OHOS_HDI_JAVA_CLIENT_INTERFACE_CODE_EMITTER_H
#define OHOS_HDI_JAVA_CLIENT_INTERFACE_CODE_EMITTER_H

#include "java_code_emitter.h"

namespace OHOS {
namespace HDI {
class JavaClientInterfaceCodeEmitter : public JavaCodeEmitter {
public:
    JavaClientInterfaceCodeEmitter() : JavaCodeEmitter() {}

    ~JavaClientInterfaceCodeEmitter() override = default;

private:
    bool ResolveDirectory(const std::string &targetDirectory) override;

    void EmitCode() override;

    void EmitInterfaceFile();

    void EmitInterfaceImports(StringBuilder &sb) const;

    void EmitInterfaceCorelibImports(StringBuilder &sb) const;

    void EmitInterfaceDBinderImports(StringBuilder &sb) const;

    void EmitInterfaceSelfDefinedTypeImports(StringBuilder &sb) const;

    void EmitInterfaceDefinition(StringBuilder &sb);

    void EmitInterfaceMethods(StringBuilder &sb, const std::string &prefix) const;

    void EmitInterfaceMethod(const AutoPtr<ASTMethod> &method, StringBuilder &sb, const std::string &prefix) const;

    void EmitInterfaceMethodParameter(
        const AutoPtr<ASTParameter> &param, StringBuilder &sb, const std::string &prefix) const;
};
} // namespace HDI
} // namespace OHOS

#endif // OHOS_HDI_JAVA_CLIENT_INTERFACE_CODE_EMITTER_H