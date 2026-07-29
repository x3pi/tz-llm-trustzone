/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#ifndef OHOS_HDI_C_SERVICE_DRIVER_CODE_EMITTER_H
#define OHOS_HDI_C_SERVICE_DRIVER_CODE_EMITTER_H

#include "codegen/c_code_emitter.h"

namespace OHOS {
namespace HDI {
class CServiceDriverCodeEmitter : public CCodeEmitter {
public:
    CServiceDriverCodeEmitter();

    ~CServiceDriverCodeEmitter() override = default;

private:
    bool ResolveDirectory(const std::string &targetDirectory) override;

    void EmitCode() override;

    void EmitLowDriverSourceFile();

    void EmitLowDriverInclusions(StringBuilder &sb) const;

    void EmitLowDriverBind(StringBuilder &sb) const;

    void EmitLowDriverRelease(StringBuilder &sb) const;

    void EmitDriverSourceFile();

    void EmitDriverInclusions(StringBuilder &sb);

    void GetDriverSourceOtherLibInclusions(HeaderFile::HeaderFileSet &headerFiles) const;

    void EmitDriverServiceDecl(StringBuilder &sb) const;

    void EmitKernelDriverDispatch(StringBuilder &sb);

    void EmitDriverDispatch(StringBuilder &sb);

    void EmitDriverInit(StringBuilder &sb) const;

    void EmitKernelDriverBind(StringBuilder &sb);

    void EmitDriverBind(StringBuilder &sb);

    void EmitKernelDriverRelease(StringBuilder &sb);

    void EmitDriverRelease(StringBuilder &sb);

    void EmitDriverEntryDefinition(StringBuilder &sb) const;
private:
    std::string hostName_;
};
} // namespace HDI
} // namespace OHOS

#endif // OHOS_HDI_C_SERVICE_DRIVER_CODE_EMITTER_H