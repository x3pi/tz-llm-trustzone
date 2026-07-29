/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#ifndef OHOS_HDI_C_CUSTOM_TYPES_CODE_EMITTER_H
#define OHOS_HDI_C_CUSTOM_TYPES_CODE_EMITTER_H

#include <vector>

#include "codegen/c_code_emitter.h"

namespace OHOS {
namespace HDI {
class CCustomTypesCodeEmitter : public CCodeEmitter {
public:
    CCustomTypesCodeEmitter() : CCodeEmitter() {}

    ~CCustomTypesCodeEmitter() override = default;

private:
    bool ResolveDirectory(const std::string &targetDirectory) override;

    void EmitCode() override;

    void EmitPassthroughCustomTypesHeaderFile();

    void EmitPassthroughHeaderInclusions(StringBuilder &sb);

    void EmitCustomTypesHeaderFile();

    void EmitHeaderInclusions(StringBuilder &sb);

    void EmitForwardDeclaration(StringBuilder &sb) const;

    void EmitCustomTypeDecls(StringBuilder &sb) const;

    void EmitCustomTypeDecl(StringBuilder &sb, const AutoPtr<ASTType> &type) const;

    void EmitCustomTypeFuncDecl(StringBuilder &sb) const;

    void EmitCustomTypeMarshallingDecl(StringBuilder &sb, const AutoPtr<ASTStructType> &type) const;

    void EmitCustomTypeUnmarshallingDecl(StringBuilder &sb, const AutoPtr<ASTStructType> &type) const;

    void EmitCustomTypeFreeDecl(StringBuilder &sb, const AutoPtr<ASTStructType> &type) const;

    void EmitCustomTypesSourceFile();

    void EmitSoucreInclusions(StringBuilder &sb);

    void GetSourceOtherLibInclusions(HeaderFile::HeaderFileSet &headerFiles) const;

    void EmitCustomTypeDataProcess(StringBuilder &sb);

    void EmitCustomTypeMarshallingImpl(StringBuilder &sb, const AutoPtr<ASTStructType> &type);

    void EmitCustomTypeUnmarshallingImpl(StringBuilder &sb, const AutoPtr<ASTStructType> &type);

    void EmitMarshallingVarDecl(const AutoPtr<ASTStructType> &type,
        const std::string &name, StringBuilder &sb, const std::string &prefix) const;

    void EmitUnmarshallingVarDecl(const AutoPtr<ASTStructType> &type,
        const std::string &name, StringBuilder &sb, const std::string &prefix) const;

    void EmitParamCheck(const std::string &name, StringBuilder &sb, const std::string &prefix) const;

    void EmitPodTypeUnmarshalling(const AutoPtr<ASTStructType> &type,
        const std::string &name, StringBuilder &sb, const std::string &prefix) const;

    void EmitMemberUnmarshalling(const AutoPtr<ASTType> &type, const std::string &name, const std::string &memberName,
        StringBuilder &sb, const std::string &prefix);

    void EmitStringMemberUnmarshalling(const AutoPtr<ASTType> &type, const std::string &memberName,
        const std::string &varName, StringBuilder &sb, const std::string &prefix);

    void EmitArrayMemberUnmarshalling(const AutoPtr<ASTType> &type, const std::string &memberName,
        const std::string &varName, StringBuilder &sb, const std::string &prefix);

    void EmitCustomTypeFreeImpl(StringBuilder &sb, const AutoPtr<ASTStructType> &type) const;

    bool NeedEmitInitVar(const AutoPtr<ASTType> &type, bool needFree);

    void EmitCustomTypeMemoryRecycle(const AutoPtr<ASTStructType> &type,
        const std::string &name, StringBuilder &sb, const std::string &prefix) const;

    void GetUtilMethods(UtilMethodMap &methods) override;

    std::vector<std::string> freeObjStatements_;
};
} // namespace HDI
} // namespace OHOS

#endif // OHOS_HDI_C_CUSTOM_TYPES_CODE_EMITTER_H