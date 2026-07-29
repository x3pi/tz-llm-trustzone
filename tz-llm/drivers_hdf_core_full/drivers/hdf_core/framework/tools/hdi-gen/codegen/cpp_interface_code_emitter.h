/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#ifndef OHOS_HDI_CPP_INTERFACE_CODE_EMITTER_H
#define OHOS_HDI_CPP_INTERFACE_CODE_EMITTER_H

#include "codegen/cpp_code_emitter.h"

namespace OHOS {
namespace HDI {
class CppInterfaceCodeEmitter : public CppCodeEmitter {
public:
    CppInterfaceCodeEmitter() : CppCodeEmitter() {}

    ~CppInterfaceCodeEmitter() override = default;

private:
    bool ResolveDirectory(const std::string &targetDirectory) override;

    void EmitCode() override;

    void EmitInterfaceHeaderFile();

    void EmitInterfaceInclusions(StringBuilder &sb);

    void GetHeaderOtherLibInclusions(HeaderFile::HeaderFileSet &headerFiles) const;

    void EmitInterfaceVersionMacro(StringBuilder &sb) const;

    void EmitInterfaceDefinition(StringBuilder &sb);

    void EmitInterfaceDescriptor(StringBuilder &sb, const std::string &prefix) const;

    void EmitGetMethodDecl(StringBuilder &sb, const std::string &prefix) const;

    void EmitInterfaceDestruction(StringBuilder &sb, const std::string &prefix) const;

    void EmitInterfaceMethodsDecl(StringBuilder &sb, const std::string &prefix);

    void EmitInterfaceMethodDecl(const AutoPtr<ASTMethod> &method, StringBuilder &sb, const std::string &prefix) const;

    void EmitInterfaceGetVersionMethod(StringBuilder &sb, const std::string &prefix) const;

    void EmitInterfaceIsProxyMethod(StringBuilder &sb, const std::string &prefix) const;

    void EmitInterfaceMethodParameter(
        const AutoPtr<ASTParameter> &param, StringBuilder &sb, const std::string &prefix) const;

    void EmitInterfaceMethodCommandsWithExtends(StringBuilder &sb, const std::string &prefix);

    void EmitGetDescMethod(StringBuilder &sb, const std::string &prefix) const;
    void EmitCastFromDecl(StringBuilder &sb, const std::string &prefix) const;
};
} // namespace HDI
} // namespace OHOS

#endif // OHOS_HDI_CPP_INTERFACE_CODE_EMITTER_H