/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#ifndef OHOS_HDI_ASTARRAYTYPE_H
#define OHOS_HDI_ASTARRAYTYPE_H

#include "ast/ast_type.h"
#include "util/autoptr.h"

namespace OHOS {
namespace HDI {
class ASTArrayType : public ASTType {
public:
    ASTArrayType() : ASTType(TypeKind::TYPE_ARRAY, false), elementType_() {}

    inline void SetElementType(const AutoPtr<ASTType> &elementType)
    {
        elementType_ = elementType;
    }

    inline AutoPtr<ASTType> GetElementType()
    {
        return elementType_;
    }

    bool IsArrayType() override;

    bool HasInnerType(TypeKind innerTypeKind) const override;

    std::string ToString() const override;

    TypeKind GetTypeKind() override;

    std::string EmitCType(TypeMode mode = TypeMode::NO_MODE) const override;

    std::string EmitCppType(TypeMode mode = TypeMode::NO_MODE) const override;

    std::string EmitJavaType(TypeMode mode, bool isInnerType = false) const override;

    void EmitCWriteVar(const std::string &parcelName, const std::string &name, const std::string &ecName,
        const std::string &gotoLabel, StringBuilder &sb, const std::string &prefix) const override;

    void EmitCProxyWriteOutVar(const std::string &parcelName, const std::string &name, const std::string &ecName,
        const std::string &gotoLabel, StringBuilder &sb, const std::string &prefix) const override;

    void EmitCProxyReadVar(const std::string &parcelName, const std::string &name, bool isInnerType,
        const std::string &ecName, const std::string &gotoLabel, StringBuilder &sb,
        const std::string &prefix) const override;

    void EmitCStubReadVar(const std::string &parcelName, const std::string &name, const std::string &ecName,
        const std::string &gotoLabel, StringBuilder &sb, const std::string &prefix) const override;

    void EmitCStubReadOutVar(const std::string &buffSizeName, const std::string &memFlagName,
        const std::string &parcelName, const std::string &name, const std::string &ecName, const std::string &gotoLabel,
        StringBuilder &sb, const std::string &prefix) const override;

    void EmitCppWriteVar(const std::string &parcelName, const std::string &name, StringBuilder &sb,
        const std::string &prefix, unsigned int innerLevel = 0) const override;

    void EmitCppReadVar(const std::string &parcelName, const std::string &name, StringBuilder &sb,
        const std::string &prefix, bool initVariable, unsigned int innerLevel = 0) const override;

    void EmitCMarshalling(const std::string &name, StringBuilder &sb, const std::string &prefix) const override;

    void EmitCUnMarshalling(const std::string &name, const std::string &gotoLabel, StringBuilder &sb,
        const std::string &prefix, std::vector<std::string> &freeObjStatements) const override;

    void EmitCppMarshalling(const std::string &parcelName, const std::string &name, StringBuilder &sb,
        const std::string &prefix, unsigned int innerLevel = 0) const override;

    void EmitCppUnMarshalling(const std::string &parcelName, const std::string &name, StringBuilder &sb,
        const std::string &prefix, bool emitType, unsigned int innerLevel = 0) const override;

    void EmitMemoryRecycle(const std::string &name, bool isClient, bool ownership, StringBuilder &sb,
        const std::string &prefix) const override;

    void EmitJavaWriteVar(const std::string &parcelName,
        const std::string &name, StringBuilder &sb, const std::string &prefix) const override;

    void EmitJavaReadVar(const std::string &parcelName,
        const std::string &name, StringBuilder &sb, const std::string &prefix) const override;

    void EmitJavaReadInnerVar(const std::string &parcelName, const std::string &name, bool isInner,
        StringBuilder &sb, const std::string &prefix) const override;

    void RegisterWriteMethod(Language language, SerMode mode, UtilMethodMap &methods) const override;

    void RegisterReadMethod(Language language, SerMode mode, UtilMethodMap &methods) const override;

    void RegisterWritePodArrayMethod(Language language, SerMode mode, UtilMethodMap &methods) const;

    void RegisterWriteStringArrayMethod(Language language, SerMode mode, UtilMethodMap &methods) const;

    void RegisterReadPodArrayMethod(Language language, SerMode mode, UtilMethodMap &methods) const;

    void RegisterReadStringArrayMethod(Language language, SerMode mode, UtilMethodMap &methods) const;

    // c methods about reading and writing array with pod element

    void EmitCWriteMethods(
        StringBuilder &sb, const std::string &prefix, const std::string &methodPrefix, bool isDecl) const;

    void EmitCReadMethods(
        StringBuilder &sb, const std::string &prefix, const std::string &methodPrefix, bool isDecl) const;

    void EmitCStubReadMethods(
        StringBuilder &sb, const std::string &prefix, const std::string &methodPrefix, bool isDecl) const;

    void EmitCStubReadMethodBody(StringBuilder &sb, const std::string &prefix) const;

    // c methods about reading and writing string array

    void EmitCWriteStrArrayMethods(
        StringBuilder &sb, const std::string &prefix, const std::string &methodPrefix, bool isDecl) const;

    void EmitCReadStrArrayMethods(
        StringBuilder &sb, const std::string &prefix, const std::string &methodPrefix, bool isDecl) const;

    void EmitCReadStrArrayMethodBody(StringBuilder &sb, const std::string &prefix) const;

    void EmitCCheckParamOfReadStringArray(StringBuilder &sb, const std::string &prefix) const;

    void EmitCStubReadStrArrayMethods(
        StringBuilder &sb, const std::string &prefix, const std::string &methodPrefix, bool isDecl) const;

    void EmitCStubReadStrArrayMethodBody(StringBuilder &sb, const std::string &prefix) const;

    void EmitCStubReadStrArrayFree(StringBuilder &sb, const std::string &prefix) const;

    // cpp methods about reading and writing array with pod element

    void EmitCppWriteMethods(
        StringBuilder &sb, const std::string &prefix, const std::string &methodPrefix, bool isDecl) const;

    void EmitCppReadMethods(
        StringBuilder &sb, const std::string &prefix, const std::string &methodPrefix, bool isDecl) const;

protected:
    void EmitJavaWriteArrayVar(
        const std::string &parcelName, const std::string &name, StringBuilder &sb, const std::string &prefix) const;

    void EmitCMallocVar(const std::string &name, const std::string &lenName, bool isClient, const std::string &ecName,
        const std::string &gotoLabel, StringBuilder &sb, const std::string &prefix) const;

    void EmitCStringElementUnMarshalling(const std::string &name, const std::string &gotoLabel, StringBuilder &sb,
        const std::string &newPrefix, std::vector<std::string> &freeObjStatements) const;

    AutoPtr<ASTType> elementType_;
};

class ASTListType : public ASTArrayType {
public:
    ASTListType() : ASTArrayType()
    {
        typeKind_ = TypeKind::TYPE_LIST;
        isPod_ = false;
    }

    bool IsArrayType() override;

    bool IsListType() override;

    std::string ToString() const override;

    TypeKind GetTypeKind() override;

    std::string EmitJavaType(TypeMode mode, bool isInnerType = false) const override;

    void EmitJavaWriteVar(const std::string &parcelName, const std::string &name, StringBuilder &sb,
        const std::string &prefix) const override;

    void EmitJavaReadVar(const std::string &parcelName, const std::string &name, StringBuilder &sb,
        const std::string &prefix) const override;

    void EmitJavaReadInnerVar(const std::string &parcelName, const std::string &name, bool isInner, StringBuilder &sb,
        const std::string &prefix) const override;
};
} // namespace HDI
} // namespace OHOS

#endif // OHOS_HDI_ASTARRAYTYPE_H