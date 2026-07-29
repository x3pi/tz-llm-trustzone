/*
 * Copyright (c) 2021-2023 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include "codegen/cpp_service_driver_code_emitter.h"
#include "util/file.h"
#include "util/logger.h"
#include "util/options.h"

namespace OHOS {
namespace HDI {
bool CppServiceDriverCodeEmitter::ResolveDirectory(const std::string &targetDirectory)
{
    if (ast_->GetASTFileType() != ASTFileType::AST_IFACE) {
        return false;
    }

    directory_ = GetFileParentPath(targetDirectory);
    if (!File::CreateParentDir(directory_)) {
        Logger::E("CppServiceDriverCodeEmitter", "Create '%s' failed!", directory_.c_str());
        return false;
    }

    return true;
}

void CppServiceDriverCodeEmitter::EmitCode()
{
    if (mode_ == GenMode::IPC) {
        if (!interface_->IsSerializable()) {
            EmitDriverSourceFile();
        }
    }
}

void CppServiceDriverCodeEmitter::EmitDriverSourceFile()
{
    std::string filePath = File::AdapterPath(
        StringHelper::Format("%s/%s.cpp", directory_.c_str(), FileName(baseName_ + "Driver").c_str()));
    File file(filePath, File::WRITE);
    StringBuilder sb;

    EmitLicense(sb);
    EmitDriverInclusions(sb);
    sb.Append("\n");
    EmitLogTagMacro(sb, FileName(baseName_ + "Driver"));
    sb.Append("\n");
    EmitDriverUsings(sb);
    sb.Append("\n");
    EmitDriverServiceDecl(sb);
    sb.Append("\n");
    EmitDriverDispatch(sb);
    sb.Append("\n");
    EmitDriverInit(sb);
    sb.Append("\n");
    EmitDriverBind(sb);
    sb.Append("\n");
    EmitDriverRelease(sb);
    sb.Append("\n");
    EmitDriverEntryDefinition(sb);

    std::string data = sb.ToString();
    file.WriteData(data.c_str(), data.size());
    file.Flush();
    file.Close();
}

void CppServiceDriverCodeEmitter::EmitDriverInclusions(StringBuilder &sb)
{
    HeaderFile::HeaderFileSet headerFiles;

    headerFiles.emplace(HeaderFileType::OTHER_MODULES_HEADER_FILE, "hdf_base");
    headerFiles.emplace(HeaderFileType::OTHER_MODULES_HEADER_FILE, "hdf_log");
    headerFiles.emplace(HeaderFileType::OTHER_MODULES_HEADER_FILE, "hdf_device_desc");
    headerFiles.emplace(HeaderFileType::OTHER_MODULES_HEADER_FILE, "hdf_sbuf_ipc");
    headerFiles.emplace(HeaderFileType::OWN_MODULE_HEADER_FILE, EmitVersionHeaderName(stubName_));

    for (const auto &file : headerFiles) {
        sb.AppendFormat("%s\n", file.ToString().c_str());
    }
}

void CppServiceDriverCodeEmitter::EmitDriverUsings(StringBuilder &sb)
{
    std::string nspace = EmitPackageToNameSpace(interface_->GetNamespace()->ToString());
    sb.AppendFormat("using namespace %s;\n", nspace.c_str());
}

void CppServiceDriverCodeEmitter::EmitDriverServiceDecl(StringBuilder &sb) const
{
    sb.AppendFormat("struct Hdf%sHost {\n", baseName_.c_str());
    sb.Append(TAB).Append("struct IDeviceIoService ioService;\n");
    sb.Append(TAB).Append("OHOS::sptr<OHOS::IRemoteObject> stub;\n");
    sb.Append("};\n");
}

void CppServiceDriverCodeEmitter::EmitDriverDispatch(StringBuilder &sb) const
{
    std::string objName = StringHelper::Format("hdf%sHost", baseName_.c_str());
    sb.AppendFormat("static int32_t %sDriverDispatch(", baseName_.c_str());
    sb.Append("struct HdfDeviceIoClient *client, int cmdId, struct HdfSBuf *data,\n");
    sb.Append(TAB).Append("struct HdfSBuf *reply)\n");
    sb.Append("{\n");
    sb.Append(TAB).AppendFormat("auto *%s = CONTAINER_OF(", objName.c_str());
    sb.AppendFormat("client->device->service, struct Hdf%sHost, ioService);\n\n", baseName_.c_str());

    sb.Append(TAB).Append("OHOS::MessageParcel *dataParcel = nullptr;\n");
    sb.Append(TAB).Append("OHOS::MessageParcel *replyParcel = nullptr;\n");
    sb.Append(TAB).Append("OHOS::MessageOption option;\n\n");

    sb.Append(TAB).Append("if (SbufToParcel(data, &dataParcel) != HDF_SUCCESS) {\n");
    sb.Append(TAB).Append(TAB).Append("HDF_LOGE(\"%{public}s: invalid data sbuf object to dispatch\", __func__);\n");
    sb.Append(TAB).Append(TAB).Append("return HDF_ERR_INVALID_PARAM;\n");
    sb.Append(TAB).Append("}\n");
    sb.Append(TAB).Append("if (SbufToParcel(reply, &replyParcel) != HDF_SUCCESS) {\n");
    sb.Append(TAB).Append(TAB).Append("HDF_LOGE(\"%{public}s: invalid reply sbuf object to dispatch\", __func__);\n");
    sb.Append(TAB).Append(TAB).Append("return HDF_ERR_INVALID_PARAM;\n");
    sb.Append(TAB).Append("}\n\n");

    sb.Append(TAB).AppendFormat(
        "return %s->stub->SendRequest(cmdId, *dataParcel, *replyParcel, option);\n", objName.c_str());
    sb.Append("}\n");
}

void CppServiceDriverCodeEmitter::EmitDriverInit(StringBuilder &sb) const
{
    sb.AppendFormat("static int Hdf%sDriverInit(struct HdfDeviceObject *deviceObject)\n", baseName_.c_str());
    sb.Append("{\n");
    sb.Append(TAB).Append("HDF_LOGI(\"%{public}s: driver init start\", __func__);\n");
    sb.Append(TAB).Append("return HDF_SUCCESS;\n");
    sb.Append("}\n");
}

void CppServiceDriverCodeEmitter::EmitDriverBind(StringBuilder &sb) const
{
    std::string objName = StringHelper::Format("hdf%sHost", baseName_.c_str());
    sb.AppendFormat("static int Hdf%sDriverBind(struct HdfDeviceObject *deviceObject)\n", baseName_.c_str());
    sb.Append("{\n");
    sb.Append(TAB).Append("HDF_LOGI(\"%{public}s: driver bind start\", __func__);\n");

    sb.Append(TAB).AppendFormat("auto *%s = new (std::nothrow) Hdf%sHost;\n", objName.c_str(), baseName_.c_str());
    sb.Append(TAB).AppendFormat("if (%s == nullptr) {\n", objName.c_str());
    sb.Append(TAB).Append(TAB).AppendFormat(
        "HDF_LOGE(\"%%{public}s: failed to create create Hdf%sHost object\", __func__);\n", baseName_.c_str());
    sb.Append(TAB).Append(TAB).Append("return HDF_FAILURE;\n");
    sb.Append(TAB).Append("}\n\n");

    sb.Append(TAB).AppendFormat("%s->ioService.Dispatch = %sDriverDispatch;\n", objName.c_str(), baseName_.c_str());
    sb.Append(TAB).AppendFormat("%s->ioService.Open = NULL;\n", objName.c_str());
    sb.Append(TAB).AppendFormat("%s->ioService.Release = NULL;\n\n", objName.c_str());

    sb.Append(TAB).AppendFormat(
        "auto serviceImpl = %s::Get(true);\n", EmitDefinitionByInterface(interface_, interfaceName_).c_str());
    sb.Append(TAB).Append("if (serviceImpl == nullptr) {\n");
    sb.Append(TAB).Append(TAB).Append("HDF_LOGE(\"%{public}s: failed to get of implement service\", __func__);\n");
    sb.Append(TAB).Append(TAB).AppendFormat("delete %s;\n", objName.c_str());
    sb.Append(TAB).Append(TAB).Append("return HDF_FAILURE;\n");
    sb.Append(TAB).Append("}\n\n");

    sb.Append(TAB).AppendFormat("%s->stub = OHOS::HDI::ObjectCollector::GetInstance().", objName.c_str());
    sb.Append("GetOrNewObject(serviceImpl,\n");
    sb.Append(TAB).Append(TAB).AppendFormat(
        "%s::GetDescriptor());\n", EmitDefinitionByInterface(interface_, interfaceName_).c_str());
    sb.Append(TAB).AppendFormat("if (%s->stub == nullptr) {\n", objName.c_str());
    sb.Append(TAB).Append(TAB).Append("HDF_LOGE(\"%{public}s: failed to get stub object\", __func__);\n");
    sb.Append(TAB).Append(TAB).AppendFormat("delete %s;\n", objName.c_str());
    sb.Append(TAB).Append(TAB).AppendFormat("return HDF_FAILURE;\n");
    sb.Append(TAB).Append("}\n\n");

    sb.Append(TAB).AppendFormat("deviceObject->service = &%s->ioService;\n", objName.c_str());
    sb.Append(TAB).Append("return HDF_SUCCESS;\n");
    sb.Append("}\n");
}

void CppServiceDriverCodeEmitter::EmitDriverRelease(StringBuilder &sb) const
{
    std::string objName = StringHelper::Format("hdf%sHost", baseName_.c_str());
    sb.AppendFormat("static void Hdf%sDriverRelease(struct HdfDeviceObject *deviceObject)\n", baseName_.c_str());
    sb.Append("{\n");
    sb.Append(TAB).Append("HDF_LOGI(\"%{public}s: driver release start\", __func__);\n");
    sb.Append(TAB).AppendFormat("if (deviceObject->service == nullptr) {\n");
    sb.Append(TAB).Append(TAB).AppendFormat("return;\n");
    sb.Append(TAB).Append("}\n\n");
    sb.Append(TAB).AppendFormat("auto *%s = CONTAINER_OF(", objName.c_str());
    sb.AppendFormat("deviceObject->service, struct Hdf%sHost, ioService);\n", baseName_.c_str());
    sb.Append(TAB).AppendFormat("if (%s != nullptr) {\n", objName.c_str());
    sb.Append(TAB).Append(TAB).AppendFormat("delete %s;\n", objName.c_str());
    sb.Append(TAB).Append("}\n");
    sb.Append("}\n");
}

void CppServiceDriverCodeEmitter::EmitDriverEntryDefinition(StringBuilder &sb) const
{
    sb.AppendFormat("struct HdfDriverEntry g_%sDriverEntry = {\n", StringHelper::StrToLower(baseName_).c_str());
    sb.Append(TAB).Append(".moduleVersion = 1,\n");
    sb.Append(TAB).AppendFormat(".moduleName = \"%s\",\n", Options::GetInstance().GetPackage().c_str());
    sb.Append(TAB).AppendFormat(".Bind = Hdf%sDriverBind,\n", baseName_.c_str());
    sb.Append(TAB).AppendFormat(".Init = Hdf%sDriverInit,\n", baseName_.c_str());
    sb.Append(TAB).AppendFormat(".Release = Hdf%sDriverRelease,\n", baseName_.c_str());
    sb.Append("};\n\n");
    EmitHeadExternC(sb);
    sb.AppendFormat("HDF_INIT(g_%sDriverEntry);\n", StringHelper::StrToLower(baseName_).c_str());
    EmitTailExternC(sb);
}
} // namespace HDI
} // namespace OHOS
