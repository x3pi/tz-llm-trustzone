/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#ifndef OHOS_HDI_NATIVE_BUFFER_H
#define OHOS_HDI_NATIVE_BUFFER_H

#include "ast/ast_type.h"

namespace OHOS {
namespace HDI {
class ASTNativeBufferType : public ASTType {
public:
    ASTNativeBufferType() : ASTType(TypeKind::TYPE_NATIVE_BUFFER, false) {}

    bool IsNativeBufferType() override;

    std::string ToString() const override;

    TypeKind GetTypeKind() override;

    std::string EmitCType(TypeMode mode = TypeMode::NO_MODE) const override;

    std::string EmitCppType(TypeMode mode = TypeMode::NO_MODE) const override;

    void EmitCWriteVar(const std::string &parcelName, const std::string &name, const std::string &ecName,
        const std::string &gotoLabel, StringBuilder &sb, const std::string &prefix) const override;

    void EmitCProxyReadVar(const std::string &parcelName, const std::string &name, bool isInnerType,
        const std::string &ecName, const std::string &gotoLabel, StringBuilder &sb,
        const std::string &prefix) const override;

    void EmitCStubReadVar(const std::string &parcelName, const std::string &name, const std::string &ecName,
        const std::string &gotoLabel, StringBuilder &sb, const std::string &prefix) const override;

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

    void EmitMemoryRecycle(const std::string &name,
        bool isClient, bool ownership, StringBuilder &sb, const std::string &prefix) const override;
};
} // namespace HDI
} // namespace OHOS

#endif // OHOS_HDI_NATIVE_BUFFER_H