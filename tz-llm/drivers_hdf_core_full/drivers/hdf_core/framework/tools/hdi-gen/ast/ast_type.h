/*
 * Copyright (c) 2021-2023 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#ifndef OHOS_HDI_ASTTYPE_H
#define OHOS_HDI_ASTTYPE_H

#include <functional>
#include <regex>
#include <unordered_map>

#include "ast/ast_namespace.h"
#include "ast/ast_node.h"
#include "util/autoptr.h"
#include "util/options.h"
#include "util/string_builder.h"

namespace OHOS {
namespace HDI {
enum class TypeKind {
    TYPE_UNKNOWN = 0,
    TYPE_BOOLEAN,
    TYPE_BYTE,
    TYPE_SHORT,
    TYPE_INT,
    TYPE_LONG,
    TYPE_FLOAT,
    TYPE_DOUBLE,
    TYPE_UCHAR,
    TYPE_USHORT,
    TYPE_UINT,
    TYPE_ULONG,
    TYPE_STRING,
    TYPE_FILEDESCRIPTOR,
    TYPE_SEQUENCEABLE,
    TYPE_INTERFACE,
    TYPE_LIST,
    TYPE_MAP,
    TYPE_ARRAY,
    TYPE_ENUM,
    TYPE_STRUCT,
    TYPE_UNION,
    TYPE_SMQ,
    TYPE_ASHMEM,
    TYPE_NATIVE_BUFFER,
    TYPE_POINTER,
};

enum class TypeMode {
    NO_MODE,   // only type
    PARAM_IN,  // type of the in attribute parameter
    PARAM_OUT, // type of the out attribute parameter
    LOCAL_VAR, // type of the local variable
};

enum class SerMode {
    PROXY_SER,  // the flag of proxy serialized
    STUB_SER,   // the flag of stub serialized
    CUSTOM_SER, // the flag of custom types serialized
};

using UtilMethod = std::function<void(StringBuilder &, const std::string &, const std::string &, bool)>;
using UtilMethodMap = std::unordered_map<std::string, UtilMethod>;

class ASTType : public ASTNode {
public:
    explicit ASTType(TypeKind kind = TypeKind::TYPE_UNKNOWN, bool isPod = true)
        :typeKind_(kind), isPod_(isPod), name_(), namespace_()
    {
    }

    virtual void SetName(const std::string &name);

    virtual std::string GetName();

    virtual void SetNamespace(const AutoPtr<ASTNamespace> &nspace);

    virtual AutoPtr<ASTNamespace> GetNamespace();

    virtual bool IsBooleanType();

    virtual bool IsByteType();

    virtual bool IsShortType();

    virtual bool IsIntegerType();

    virtual bool IsLongType();

    virtual bool IsUcharType();

    virtual bool IsUshortType();

    virtual bool IsUintType();

    virtual bool IsUlongType();

    virtual bool IsFloatType();

    virtual bool IsDoubleType();

    virtual bool IsStringType();

    virtual bool IsListType();

    virtual bool IsMapType();

    virtual bool IsEnumType();

    virtual bool IsStructType();

    virtual bool IsUnionType();

    virtual bool IsInterfaceType();

    virtual bool IsSequenceableType();

    virtual bool IsArrayType();

    virtual bool IsFdType();

    virtual bool IsSmqType();

    virtual bool IsAshmemType();

    virtual bool IsNativeBufferType();

    virtual bool IsPointerType();

    bool IsPod() const;

    virtual bool HasInnerType(TypeKind innerType) const;

    virtual std::string ToShortString();

    std::string ToString() const override;

    virtual TypeKind GetTypeKind();

    virtual std::string EmitCType(TypeMode mode = TypeMode::NO_MODE) const;

    virtual std::string EmitCppType(TypeMode mode = TypeMode::NO_MODE) const;

    virtual std::string EmitJavaType(TypeMode mode, bool isInnerType = false) const;

    virtual void EmitCWriteVar(const std::string &parcelName, const std::string &name, const std::string &ecName,
        const std::string &gotoLabel, StringBuilder &sb, const std::string &prefix) const;

    virtual void EmitCProxyWriteOutVar(const std::string &parcelName, const std::string &name,
        const std::string &ecName, const std::string &gotoLabel, StringBuilder &sb, const std::string &prefix) const;

    virtual void EmitCProxyReadVar(const std::string &parcelName, const std::string &name, bool isInnerType,
        const std::string &ecName, const std::string &gotoLabel, StringBuilder &sb, const std::string &prefix) const;

    virtual void EmitCStubReadVar(const std::string &parcelName, const std::string &name, const std::string &ecName,
        const std::string &gotoLabel, StringBuilder &sb, const std::string &prefix) const;

    virtual void EmitCStubReadOutVar(const std::string &buffSizeName, const std::string &memFlagName,
        const std::string &parcelName, const std::string &name, const std::string &ecName, const std::string &gotoLabel,
        StringBuilder &sb, const std::string &prefix) const;

    virtual void EmitCppWriteVar(const std::string &parcelName, const std::string &name, StringBuilder &sb,
        const std::string &prefix, unsigned int innerLevel = 0) const;

    virtual void EmitCppReadVar(const std::string &parcelName, const std::string &name, StringBuilder &sb,
        const std::string &prefix, bool initVariable, unsigned int innerLevel = 0) const;

    virtual void EmitCMarshalling(const std::string &name, StringBuilder &sb, const std::string &prefix) const;

    virtual void EmitCUnMarshalling(const std::string &name, const std::string &gotoLabel, StringBuilder &sb,
        const std::string &prefix, std::vector<std::string> &freeObjStatements) const;

    void EmitFreeStatements(
        const std::vector<std::string> &freeObjStatements, StringBuilder &sb, const std::string &prefix) const;

    virtual void EmitCppMarshalling(const std::string &parcelName, const std::string &name, StringBuilder &sb,
        const std::string &prefix, unsigned int innerLevel = 0) const;

    virtual void EmitCppUnMarshalling(const std::string &parcelName, const std::string &name, StringBuilder &sb,
        const std::string &prefix, bool emitType, unsigned int innerLevel = 0) const;

    virtual void EmitMemoryRecycle(
        const std::string &name, bool isClient, bool ownership, StringBuilder &sb, const std::string &prefix) const;

    virtual void EmitJavaWriteVar(
        const std::string &parcelName, const std::string &name, StringBuilder &sb, const std::string &prefix) const;

    virtual void EmitJavaReadVar(
        const std::string &parcelName, const std::string &name, StringBuilder &sb, const std::string &prefix) const;

    virtual void EmitJavaReadInnerVar(const std::string &parcelName, const std::string &name, bool isInner,
        StringBuilder &sb, const std::string &prefix) const;

    virtual void RegisterWriteMethod(Language language, SerMode mode, UtilMethodMap &methods) const;

    virtual void RegisterReadMethod(Language language, SerMode mode, UtilMethodMap &methods) const;

    virtual std::string GetNameWithNamespace(AutoPtr<ASTNamespace> space, std::string name) const;

    virtual std::string PascalName(const std::string &name) const;

protected:
    TypeKind typeKind_;
    bool isPod_;
    std::string name_;
    AutoPtr<ASTNamespace> namespace_;
};
} // namespace HDI
} // namespace OHOS

#endif // OHOS_HDI_ASTTYPE_H