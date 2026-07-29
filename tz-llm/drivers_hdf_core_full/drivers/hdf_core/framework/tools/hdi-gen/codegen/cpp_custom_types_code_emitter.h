/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#ifndef OHOS_HDI_CPP_CUSTOM_TYPES_CODE_EMITTER_H
#define OHOS_HDI_CPP_CUSTOM_TYPES_CODE_EMITTER_H

#include "codegen/cpp_code_emitter.h"

namespace OHOS {
namespace HDI {
class CppCustomTypesCodeEmitter : public CppCodeEmitter {
public:
    CppCustomTypesCodeEmitter() : CppCodeEmitter() {}

    ~CppCustomTypesCodeEmitter() override = default;

private:
    bool ResolveDirectory(const std::string &targetDirectory) override;

    void EmitCode() override;

    void EmitPassthroughCustomTypesHeaderFile();

    void EmitPassthroughHeaderFileInclusions(StringBuilder& sb);

    void EmitCustomTypesHeaderFile();

    void EmitHeaderFileInclusions(StringBuilder &sb);

    void EmitForwardDeclaration(StringBuilder &sb) const;

    void EmitUsingNamespace(StringBuilder &sb) override;

    void EmitCustomTypeDecls(StringBuilder &sb) const;

    void EmitCustomTypeDecl(StringBuilder &sb, const AutoPtr<ASTType> &type) const;

    void EmitCustomTypeFuncDecl(StringBuilder &sb) const;

    void EmitCustomTypeMarshallingDecl(StringBuilder &sb, const AutoPtr<ASTStructType> &type) const;

    void EmitCustomTypeUnmarshallingDecl(StringBuilder &sb, const AutoPtr<ASTStructType> &type) const;

    void EmitCustomTypesSourceFile();

    void EmitSourceFileInclusions(StringBuilder &sb);

    void GetSourceOtherLibInclusions(HeaderFile::HeaderFileSet &headerFiles) const;

    void EmitCustomTypeDataProcess(StringBuilder &sb) const;

    void EmitCustomTypeMarshallingImpl(StringBuilder &sb, const AutoPtr<ASTStructType> &type) const;

    void EmitCustomTypeUnmarshallingImpl(StringBuilder &sb, const AutoPtr<ASTStructType> &type) const;

    void EmitBeginNamespace(StringBuilder &sb) override;

    void EmitEndNamespace(StringBuilder &sb) override;

    void GetUtilMethods(UtilMethodMap &methods) override;
};
} // namespace HDI
} // namespace OHOS

#endif // OHOS_HDI_CPP_CUSTOM_TYPES_CODE_EMITTER_H