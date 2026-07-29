/*
 * Copyright (c) 2021-2023 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#ifndef OHOS_HDI_C_SERVICE_STUB_CODEE_MITTER_H
#define OHOS_HDI_C_SERVICE_STUB_CODEE_MITTER_H

#include "codegen/c_code_emitter.h"

namespace OHOS {
namespace HDI {
class CServiceStubCodeEmitter : public CCodeEmitter {
public:
    CServiceStubCodeEmitter() : CCodeEmitter() {}

    ~CServiceStubCodeEmitter() override = default;

private:
    bool ResolveDirectory(const std::string &targetDirectory) override;

    void EmitCode() override;

    void EmitServiceStubHeaderFile();

    void EmitStubHeaderInclusions(StringBuilder &sb);

    void EmitCbServiceStubDef(StringBuilder &sb) const;

    void EmitCbServiceStubMethodsDcl(StringBuilder &sb) const;

    void EmitServiceStubSourceFile();

    void EmitStubSourceInclusions(StringBuilder &sb);

    void GetSourceOtherLibInclusions(HeaderFile::HeaderFileSet &headerFiles) const;

    // get or release method for driver interface object
    void EmitExternalMethodImpl(StringBuilder &sb);

    void EmitGetMethodImpl(StringBuilder &sb) const;

    void EmitGetInstanceMehtodImpl(StringBuilder &sb) const;

    void EmitReleaseMethodImpl(StringBuilder &sb) const;

    void EmitReleaseInstanceMethodImpl(StringBuilder &sb) const;

    void EmitServiceStubMethodImpls(StringBuilder &sb, const std::string &prefix);

    void EmitServiceStubMethodImpl(const AutoPtr<ASTMethod> &method, StringBuilder &sb,
        const std::string &prefix) const;

    void EmitReadFlagVariable(bool readFlag, StringBuilder &sb, const std::string &prefix) const;

    void EmitStubLocalVariable(const AutoPtr<ASTParameter> &param, StringBuilder &sb, const std::string &prefix) const;

    void EmitReadStubMethodParameter(const AutoPtr<ASTParameter> &param, const std::string &parcelName,
        const std::string &gotoLabel, StringBuilder &sb, const std::string &prefix) const;

    void EmitReadCStringStubMethodParameter(const AutoPtr<ASTParameter> &param, const std::string &parcelName,
        const std::string &gotoLabel, StringBuilder &sb, const std::string &prefix, AutoPtr<ASTType> &type) const;

    void EmitOutVarMemInitialize(const AutoPtr<ASTParameter> &param, const std::string &parcelName,
        const std::string &gotoLabel, StringBuilder &sb, const std::string &prefix) const;

    void EmitStubCallMethod(const AutoPtr<ASTMethod> &method,
        const std::string &gotoLabel, StringBuilder &sb, const std::string &prefix) const;

    void EmitCallParameter(
        StringBuilder &sb, const AutoPtr<ASTType> &type, ParamAttr attribute, const std::string &name) const;

    void EmitStubGetVerMethodImpl(
        const AutoPtr<ASTMethod> &method, StringBuilder &sb, const std::string &prefix) const;

    void EmitStubAsObjectMethodImpl(StringBuilder &sb, const std::string &prefix) const;

    void EmitKernelStubOnRequestMethodImpl(StringBuilder &sb, const std::string &prefix);

    void EmitKernelStubConstruct(StringBuilder &sb) const;

    void EmitStubOnRequestMethodImpl(StringBuilder &sb, const std::string &prefix);

    void EmitStubRemoteDispatcher(StringBuilder &sb) const;

    void EmitStubNewInstance(StringBuilder &sb) const;

    void EmitStubReleaseMethod(StringBuilder &sb) const;

    void EmitStubConstructor(StringBuilder &sb) const;

    void EmitStubRegAndUnreg(StringBuilder &sb) const;

    void GetUtilMethods(UtilMethodMap &methods) override;
};
} // namespace HDI
} // namespace OHOS

#endif // OHOS_HDI_C_SERVICE_STUB_CODEE_MITTER_H