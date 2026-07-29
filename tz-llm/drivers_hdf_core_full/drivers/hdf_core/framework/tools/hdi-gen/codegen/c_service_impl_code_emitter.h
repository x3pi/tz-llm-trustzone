/*
 * Copyright (c) 2021-2023 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#ifndef OHOS_HDI_C_SERVICE_IMPL_CODE_EMITTER_H
#define OHOS_HDI_C_SERVICE_IMPL_CODE_EMITTER_H

#include "codegen/c_code_emitter.h"

namespace OHOS {
namespace HDI {
class CServiceImplCodeEmitter : public CCodeEmitter {
public:
    CServiceImplCodeEmitter() : CCodeEmitter() {}

    ~CServiceImplCodeEmitter() override = default;

private:
    bool ResolveDirectory(const std::string &targetDirectory) override;

    void EmitCode() override;

    // generate service header file of low mode
    void EmitLowServiceImplHeaderFile();

    void EmitLowServiceImplInclusions(StringBuilder &sb);

    void EmitLowServiceImplDefinition(StringBuilder &sb);

    // generate service source file of low mode
    void EmitLowServiceImplSourceFile();

    void EmitLowServiceImplGetMethod(StringBuilder &sb);

    // generate service header file of ipc, passthrough, kernel mode
    void EmitServiceImplHeaderFile();

    void EmitServiceImplHeaderInclusions(StringBuilder &sb);

    void EmitServiceImplExternalMethodsDecl(StringBuilder &sb) const;

    void EmitServiceImplSourceFile();

    void EmitServiceImplSourceInclusions(StringBuilder &sb);

    void GetSourceOtherLibInclusions(HeaderFile::HeaderFileSet &headerFiles) const;

    void EmitKernelServiceImplDef(StringBuilder &sb) const;

    void EmitServiceImplDef(StringBuilder &sb) const;

    void EmitServiceImplMethodImpls(StringBuilder &sb, const std::string &prefix) const;

    void EmitServiceImplMethodImpl(
        const AutoPtr<ASTMethod> &method, StringBuilder &sb, const std::string &prefix) const;

    void EmitKernelServiceImplGetMethod(StringBuilder &sb) const;

    void EmitServiceImplGetVersionMethod(StringBuilder &sb, const std::string &prefix) const;

    void EmitServiceImplGetMethod(StringBuilder &sb) const;

    void EmitKernelServiceImplReleaseMethod(StringBuilder &sb) const;

    void EmitServiceImplReleaseMethod(StringBuilder &sb) const;
};
} // namespace HDI
} // namespace OHOS

#endif // OHOS_HDI_C_SERVICE_IMPL_CODE_EMITTER_H