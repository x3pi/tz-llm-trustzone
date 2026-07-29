/*
 * Copyright (c) 2021-2023 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#ifndef OHOS_HDI_C_CODE_EMITTER_H
#define OHOS_HDI_C_CODE_EMITTER_H

#include "ast/ast.h"
#include "codegen/code_emitter.h"
#include "util/autoptr.h"
#include "util/string_builder.h"

namespace OHOS {
namespace HDI {
class CCodeEmitter : public CodeEmitter {
public:
    ~CCodeEmitter() override = default;

protected:
    void GetStdlibInclusions(HeaderFile::HeaderFileSet &headerFiles);

    void GetImportInclusions(HeaderFile::HeaderFileSet &headerFiles);

    void EmitInterfaceMethodParameter(
        const AutoPtr<ASTParameter> &parameter, StringBuilder &sb, const std::string &prefix) const;

    void EmitMethodNeedLoopVar(const AutoPtr<ASTMethod> &method,
        bool needRW, bool needFree, StringBuilder &sb, const std::string &prefix) const;

    bool EmitNeedLoopVar(const AutoPtr<ASTType> &type, bool needRW, bool needFree) const;

    void EmitErrorHandle(const AutoPtr<ASTMethod> &method, const std::string &gotoLabel, bool isClient,
        StringBuilder &sb, const std::string &prefix) const;

    void EmitLicense(StringBuilder &sb);

    void EmitHeadMacro(StringBuilder &sb, const std::string &fullName) const;

    void EmitTailMacro(StringBuilder &sb, const std::string &fullName) const;

    void EmitHeadExternC(StringBuilder &sb) const;

    void EmitTailExternC(StringBuilder &sb) const;

    std::string MacroName(const std::string &name) const;

    std::string SpecificationParam(StringBuilder &paramSb, const std::string &prefix) const;

    static constexpr const char *errorsLabelName_ = "ERRORS";
    static constexpr const char *finishedLabelName_ = "FINISHED";
};
} // namespace HDI
} // namespace OHOS

#endif // OHOS_HDI_C_CODE_EMITTER_H