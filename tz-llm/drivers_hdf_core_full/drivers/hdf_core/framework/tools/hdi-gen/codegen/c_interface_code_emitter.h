/*
 * Copyright (c) 2021-2023 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#ifndef OHOS_HDI_C_CLIENT_INTERFACE_CODE_EMITTER_H
#define OHOS_HDI_C_CLIENT_INTERFACE_CODE_EMITTER_H

#include "codegen/c_code_emitter.h"

namespace OHOS {
namespace HDI {
class CInterfaceCodeEmitter : public CCodeEmitter {
public:
    CInterfaceCodeEmitter() : CCodeEmitter() {}

    ~CInterfaceCodeEmitter() override = default;

private:
    bool ResolveDirectory(const std::string &targetDirectory) override;

    void EmitCode() override;

    void EmitLowModeInterfaceHeaderFile();

    void EmitLowModeExternalMethod(StringBuilder &sb) const;

    void EmitInterfaceHeaderFile();

    void EmitImportInclusions(StringBuilder &sb);

    void GetHeaderOtherLibInclusions(HeaderFile::HeaderFileSet &headerFiles) const;

    void EmitPreDeclaration(StringBuilder &sb) const;

    void EmitInterfaceDesc(StringBuilder &sb) const;

    void EmitInterfaceVersionMacro(StringBuilder &sb) const;

    void EmitInterfaceDefinition(StringBuilder &sb);

    void EmitInterfaceMethods(StringBuilder &sb, const std::string &prefix) const;

    void EmitInterfaceMethod(const AutoPtr<ASTMethod> &method, StringBuilder &sb, const std::string &prefix) const;

    void EmitAsObjectMethod(StringBuilder &sb, const std::string &prefix) const;

    void EmitExternalMethod(StringBuilder &sb) const;

    void EmitInterfaceGetMethodDecl(StringBuilder &sb) const;

    void EmitInterfaceReleaseMethodDecl(StringBuilder &sb) const;
};
} // namespace HDI
} // namespace OHOS

#endif // OHOS_HDI_C_CLIENT_INTERFACE_CODE_EMITTER_H