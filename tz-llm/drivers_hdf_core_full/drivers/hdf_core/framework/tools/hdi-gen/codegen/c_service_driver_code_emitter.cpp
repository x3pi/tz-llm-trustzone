/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include "codegen/c_service_driver_code_emitter.h"
#include "util/file.h"
#include "util/logger.h"
#include "util/options.h"

namespace OHOS {
namespace HDI {
CServiceDriverCodeEmitter::CServiceDriverCodeEmitter() : CCodeEmitter(), hostName_("host")
{
}

bool CServiceDriverCodeEmitter::ResolveDirectory(const std::string &targetDirectory)
{
    if (ast_->GetASTFileType() != ASTFileType::AST_IFACE) {
        return false;
    }

    directory_ = GetFileParentPath(targetDirectory);
    if (!File::CreateParentDir(directory_)) {
        Logger::E("CServiceDriverCodeEmitter", "Create '%s' failed!", directory_.c_str());
        return false;
    }

    return true;
}

void CServiceDriverCodeEmitter::EmitCode()
{
    switch (mode_) {
        case GenMode::LOW: {
            EmitLowDriverSourceFile();
            break;
        }
        case GenMode::IPC: {
            if (!interface_->IsSerializable()) {
                EmitDriverSourceFile();
            }
            break;
        }
        case GenMode::KERNEL: {
            EmitDriverSourceFile();
            break;
        }
        default:
            break;
    }
}

void CServiceDriverCodeEmitter::EmitLowDriverSourceFile()
{
    std::string filePath =
        File::AdapterPath(StringHelper::Format("%s/%s.c", directory_.c_str(), FileName(baseName_ + "Driver").c_str()));
    File file(filePath, File::WRITE);
    StringBuilder sb;

    EmitLicense(sb);
    sb.Append("\n");
    EmitLowDriverInclusions(sb);
    sb.Append("\n");
    EmitLogTagMacro(sb, FileName(baseName_ + "Driver"));
    sb.Append("\n");
    EmitLowDriverBind(sb);
    sb.Append("\n");
    EmitDriverInit(sb);
    sb.Append("\n");
    EmitLowDriverRelease(sb);
    sb.Append("\n");
    EmitDriverEntryDefinition(sb);

    std::string data = sb.ToString();
    file.WriteData(data.c_str(), data.size());
    file.Flush();
    file.Close();
}

void CServiceDriverCodeEmitter::EmitLowDriverInclusions(StringBuilder &sb) const
{
    HeaderFile::HeaderFileSet headerFiles;
    headerFiles.emplace(HeaderFileType::OWN_MODULE_HEADER_FILE, EmitVersionHeaderName(implName_));
    headerFiles.emplace(HeaderFileType::OTHER_MODULES_HEADER_FILE, "hdf_log");
    headerFiles.emplace(HeaderFileType::OTHER_MODULES_HEADER_FILE, "hdf_device_desc");
    
    for (const auto &file : headerFiles) {
        sb.AppendFormat("%s\n", file.ToString().c_str());
    }
}

void CServiceDriverCodeEmitter::EmitLowDriverBind(StringBuilder &sb) const
{
    sb.AppendFormat("static int Hdf%sDriverBind(struct HdfDeviceObject *deviceObject)\n", baseName_.c_str());
    sb.Append("{\n");
    sb.Append(TAB).Append("HDF_LOGI(\"%s: driver bind start\", __func__);\n");
    sb.Append(TAB).AppendFormat("struct %s *serviceImpl = %sGet();\n", implName_.c_str(), implName_.c_str());
    sb.Append(TAB).Append("if (serviceImpl == NULL) {\n");
    sb.Append(TAB).Append(TAB).Append("HDF_LOGE(\"%s: failed to get service impl\", __func__);\n");
    sb.Append(TAB).Append(TAB).Append("return HDF_FAILURE;\n");
    sb.Append(TAB).Append("}\n");
    sb.Append(TAB).Append("deviceObject->service = &serviceImpl->super.service;\n");
    sb.Append(TAB).Append("return HDF_SUCCESS;\n");
    sb.Append("}\n");
}

void CServiceDriverCodeEmitter::EmitLowDriverRelease(StringBuilder &sb) const
{
    sb.AppendFormat("static void Hdf%sDriverRelease(struct HdfDeviceObject *deviceObject)\n", baseName_.c_str());
    sb.Append("{\n");
    sb.Append(TAB).Append("HDF_LOGI(\"%s: driver release start\", __func__);\n");

    sb.Append(TAB).Append("if (deviceObject == NULL || deviceObject->service == NULL) {\n");
    sb.Append(TAB).Append(TAB).Append("HDF_LOGE(\"%s: invalid device object\", __func__);\n");
    sb.Append(TAB).Append(TAB).Append("return;\n");
    sb.Append(TAB).Append("}\n\n");
    sb.Append(TAB).AppendFormat(
        "struct %s *serviceImpl = (struct %s *)deviceObject->service;\n", implName_.c_str(), implName_.c_str());
    sb.Append(TAB).Append("if (serviceImpl != NULL) {\n");
    sb.Append(TAB).Append(TAB).AppendFormat("%sRelease(serviceImpl);\n", implName_.c_str());
    sb.Append(TAB).Append("}\n");
    sb.Append("}\n");
}

void CServiceDriverCodeEmitter::EmitDriverSourceFile()
{
    std::string filePath =
        File::AdapterPath(StringHelper::Format("%s/%s.c", directory_.c_str(), FileName(baseName_ + "Driver").c_str()));
    File file(filePath, File::WRITE);
    StringBuilder sb;

    EmitLicense(sb);
    EmitDriverInclusions(sb);
    sb.Append("\n");
    EmitLogTagMacro(sb, FileName(baseName_ + "Driver"));
    sb.Append("\n");
    EmitDriverServiceDecl(sb);
    sb.Append("\n");
    if (mode_ == GenMode::KERNEL) {
        EmitKernelDriverDispatch(sb);
        sb.Append("\n");
        EmitDriverInit(sb);
        sb.Append("\n");
        EmitKernelDriverBind(sb);
        sb.Append("\n");
        EmitKernelDriverRelease(sb);
    } else {
        EmitDriverDispatch(sb);
        sb.Append("\n");
        EmitDriverInit(sb);
        sb.Append("\n");
        EmitDriverBind(sb);
        sb.Append("\n");
        EmitDriverRelease(sb);
    }
    sb.Append("\n");
    EmitDriverEntryDefinition(sb);

    std::string data = sb.ToString();
    file.WriteData(data.c_str(), data.size());
    file.Flush();
    file.Close();
}

void CServiceDriverCodeEmitter::EmitDriverInclusions(StringBuilder &sb)
{
    HeaderFile::HeaderFileSet headerFiles;

    if (mode_ == GenMode::KERNEL) {
        headerFiles.emplace(HeaderFileType::OWN_MODULE_HEADER_FILE, EmitVersionHeaderName(implName_));
    } else {
        headerFiles.emplace(HeaderFileType::OWN_MODULE_HEADER_FILE, EmitVersionHeaderName(interfaceName_));
    }

    GetDriverSourceOtherLibInclusions(headerFiles);

    for (const auto &file : headerFiles) {
        sb.AppendFormat("%s\n", file.ToString().c_str());
    }
}

void CServiceDriverCodeEmitter::GetDriverSourceOtherLibInclusions(HeaderFile::HeaderFileSet &headerFiles) const
{
    headerFiles.emplace(HeaderFileType::OTHER_MODULES_HEADER_FILE, "hdf_base");
    headerFiles.emplace(HeaderFileType::OTHER_MODULES_HEADER_FILE, "hdf_log");
    headerFiles.emplace(HeaderFileType::OTHER_MODULES_HEADER_FILE, "osal_mem");
    headerFiles.emplace(HeaderFileType::OTHER_MODULES_HEADER_FILE, "hdf_device_desc");
    if (mode_ != GenMode::KERNEL) {
        headerFiles.emplace(HeaderFileType::OTHER_MODULES_HEADER_FILE, "hdf_device_object");
        headerFiles.emplace(HeaderFileType::OTHER_MODULES_HEADER_FILE, "hdf_remote_service");
        headerFiles.emplace(HeaderFileType::OTHER_MODULES_HEADER_FILE, "stub_collector");
    }
}

void CServiceDriverCodeEmitter::EmitDriverServiceDecl(StringBuilder &sb) const
{
    sb.AppendFormat("struct Hdf%sHost {\n", baseName_.c_str());
    sb.Append(TAB).AppendFormat("struct IDeviceIoService ioService;\n");
    if (mode_ == GenMode::KERNEL) {
        sb.Append(TAB).AppendFormat("struct %s *service;\n", implName_.c_str());
    } else {
        sb.Append(TAB).AppendFormat("struct %s *service;\n", interfaceName_.c_str());
        sb.Append(TAB).Append("struct HdfRemoteService **stubObject;\n");
    }
    sb.Append("};\n");
}

void CServiceDriverCodeEmitter::EmitKernelDriverDispatch(StringBuilder &sb)
{
    sb.AppendFormat(
        "static int32_t %sDriverDispatch(struct HdfDeviceIoClient *client, int cmdId,\n", baseName_.c_str());
    sb.Append(TAB).Append("struct HdfSBuf *data, struct HdfSBuf *reply)\n");
    sb.Append("{\n");
    sb.Append(TAB).AppendFormat("struct Hdf%sHost *%s = CONTAINER_OF(\n", baseName_.c_str(), hostName_.c_str());
    sb.Append(TAB).Append(TAB).AppendFormat(
        "client->device->service, struct Hdf%sHost, ioService);\n", baseName_.c_str());
    sb.Append(TAB).AppendFormat("if (%s->service == NULL || %s->service->stub.OnRemoteRequest == NULL) {\n",
        hostName_.c_str(), hostName_.c_str());
    sb.Append(TAB).Append(TAB).Append("HDF_LOGE(\"%{public}s: invalid service obj\", __func__);\n");
    sb.Append(TAB).Append(TAB).Append("return HDF_ERR_INVALID_OBJECT;\n");
    sb.Append(TAB).Append("}\n\n");
    sb.Append(TAB).AppendFormat("return %s->service->stub.OnRemoteRequest(", hostName_.c_str());
    sb.AppendFormat("&%s->service->stub.interface, cmdId, data, reply);\n", hostName_.c_str());
    sb.Append("}\n");
}

void CServiceDriverCodeEmitter::EmitDriverDispatch(StringBuilder &sb)
{
    sb.AppendFormat(
        "static int32_t %sDriverDispatch(struct HdfDeviceIoClient *client, int cmdId,\n", baseName_.c_str());
    sb.Append(TAB).Append("struct HdfSBuf *data, struct HdfSBuf *reply)\n");
    sb.Append("{\n");
    sb.Append(TAB).AppendFormat("struct Hdf%sHost *%s = CONTAINER_OF(", baseName_.c_str(), hostName_.c_str());
    sb.AppendFormat("client->device->service, struct Hdf%sHost, ioService);\n", baseName_.c_str());
    sb.Append(TAB).AppendFormat("if (%s->service == NULL || %s->stubObject == NULL) {\n",
        hostName_.c_str(), hostName_.c_str());
    sb.Append(TAB).Append(TAB).Append("HDF_LOGE(\"%{public}s: invalid service obj\", __func__);\n");
    sb.Append(TAB).Append(TAB).Append("return HDF_ERR_INVALID_OBJECT;\n");
    sb.Append(TAB).Append("}\n\n");
    sb.Append(TAB).AppendFormat("struct HdfRemoteService *stubObj = *%s->stubObject;\n", hostName_.c_str());
    sb.Append(TAB).AppendFormat("if (stubObj == NULL || stubObj->dispatcher == NULL || ");
    sb.Append("stubObj->dispatcher->Dispatch == NULL) {\n");
    sb.Append(TAB).Append(TAB).Append("return HDF_ERR_INVALID_OBJECT;\n");
    sb.Append(TAB).Append("}\n\n");
    sb.Append(TAB).Append("return stubObj->dispatcher->Dispatch(");
    sb.Append("(struct HdfRemoteService *)stubObj->target, cmdId, data, reply);\n");
    sb.Append("}\n");
}

void CServiceDriverCodeEmitter::EmitDriverInit(StringBuilder &sb) const
{
    sb.AppendFormat("static int Hdf%sDriverInit(struct HdfDeviceObject *deviceObject)\n", baseName_.c_str());
    sb.Append("{\n");
    if (mode_ == GenMode::LOW) {
        sb.Append(TAB).Append("HDF_LOGI(\"%s: driver init start\", __func__);\n");
    } else {
        sb.Append(TAB).Append("HDF_LOGI(\"%{public}s: driver init start\", __func__);\n");
    }
    sb.Append(TAB).Append("return HDF_SUCCESS;\n");
    sb.Append("}\n");
}

void CServiceDriverCodeEmitter::EmitKernelDriverBind(StringBuilder &sb)
{
    sb.AppendFormat("static int Hdf%sDriverBind(struct HdfDeviceObject *deviceObject)\n", baseName_.c_str());
    sb.Append("{\n");
    sb.Append(TAB).Append("HDF_LOGI(\"%{public}s: driver bind start\", __func__);\n");
    sb.Append("\n");

    sb.Append(TAB).AppendFormat("struct Hdf%sHost *%s = (struct Hdf%sHost *)OsalMemCalloc(", baseName_.c_str(),
        hostName_.c_str(), baseName_.c_str());
    sb.AppendFormat("sizeof(struct Hdf%sHost));\n", baseName_.c_str());
    sb.Append(TAB).AppendFormat("if (%s == NULL) {\n", hostName_.c_str());
    sb.Append(TAB).Append(TAB).AppendFormat(
        "HDF_LOGE(\"Hdf%sDriverBind create Hdf%sHost object failed!\");\n", baseName_.c_str(), baseName_.c_str());
    sb.Append(TAB).Append(TAB).Append("return HDF_FAILURE;\n");
    sb.Append(TAB).Append("}\n");
    sb.Append("\n");
    sb.Append(TAB).AppendFormat("%s->ioService.Dispatch = %sDriverDispatch;\n", hostName_.c_str(), baseName_.c_str());
    sb.Append(TAB).AppendFormat("%s->ioService.Open = NULL;\n", hostName_.c_str());
    sb.Append(TAB).AppendFormat("%s->ioService.Release = NULL;\n", hostName_.c_str());
    sb.Append(TAB).AppendFormat("%s->service = %sServiceGet();\n", hostName_.c_str(), baseName_.c_str());
    sb.Append(TAB).AppendFormat("if (%s->service == NULL) {\n", hostName_.c_str());
    sb.Append(TAB).Append(TAB).AppendFormat("OsalMemFree(%s);\n", hostName_.c_str());
    sb.Append(TAB).Append(TAB).Append("HDF_LOGE(\"%{public}s: failed to get service object\", __func__);\n");
    sb.Append(TAB).Append(TAB).Append("return HDF_FAILURE;\n");
    sb.Append(TAB).Append("}\n");
    sb.Append("\n");
    sb.Append(TAB).AppendFormat("deviceObject->service = &%s->ioService;\n", hostName_.c_str());
    sb.Append(TAB).Append("return HDF_SUCCESS;\n");
    sb.Append("}\n");
}

void CServiceDriverCodeEmitter::EmitDriverBind(StringBuilder &sb)
{
    sb.AppendFormat("static int Hdf%sDriverBind(struct HdfDeviceObject *deviceObject)\n", baseName_.c_str());
    sb.Append("{\n");
    sb.Append(TAB).Append("HDF_LOGI(\"%{public}s: driver bind start\", __func__);\n");
    sb.Append(TAB).AppendFormat("int32_t ret = HdfDeviceObjectSetInterfaceDesc(deviceObject, %s);\n",
        interface_->EmitDescMacroName().c_str());
    sb.Append(TAB).Append("if (ret != HDF_SUCCESS) {\n");
    sb.Append(TAB).Append(TAB).Append(
        "HDF_LOGE(\"%{public}s: failed to set interface descriptor of device object\", __func__);\n");
    sb.Append(TAB).Append(TAB).Append("return ret;\n");
    sb.Append(TAB).Append("}\n\n");

    sb.Append(TAB).AppendFormat("struct Hdf%sHost *%s = (struct Hdf%sHost *)OsalMemCalloc(", baseName_.c_str(),
        hostName_.c_str(), baseName_.c_str());
    sb.AppendFormat("sizeof(struct Hdf%sHost));\n", baseName_.c_str());
    sb.Append(TAB).AppendFormat("if (%s == NULL) {\n", hostName_.c_str());
    sb.Append(TAB).Append(TAB).AppendFormat("HDF_LOGE(\"%%{public}s: create Hdf%sHost object failed!\", __func__);\n",
        baseName_.c_str());
    sb.Append(TAB).Append(TAB).Append("return HDF_FAILURE;\n");
    sb.Append(TAB).Append("}\n\n");
    sb.Append(TAB).AppendFormat("struct %s *serviceImpl = %sGet(true);\n", interfaceName_.c_str(),
        interfaceName_.c_str());
    sb.Append(TAB).Append("if (serviceImpl == NULL) {\n");
    sb.Append(TAB).Append(TAB).Append("HDF_LOGE(\"%{public}s: create serviceImpl failed!\", __func__);\n");
    sb.Append(TAB).Append(TAB).AppendFormat("OsalMemFree(%s);\n", hostName_.c_str());
    sb.Append(TAB).Append(TAB).Append("return HDF_FAILURE;\n");
    sb.Append(TAB).Append("}\n\n");
    sb.Append(TAB).AppendFormat("struct HdfRemoteService **stubObj = StubCollectorGetOrNewObject(");
    sb.AppendFormat("%s, serviceImpl);\n", interface_->EmitDescMacroName().c_str());
    sb.Append(TAB).Append("if (stubObj == NULL) {\n");
    sb.Append(TAB).Append(TAB).Append("HDF_LOGE(\"%{public}s: failed to get stub object\", __func__);\n");
    sb.Append(TAB).Append(TAB).AppendFormat("OsalMemFree(%s);\n", hostName_.c_str());
    sb.Append(TAB).Append(TAB).AppendFormat("%sRelease(serviceImpl, true);\n", interfaceName_.c_str());
    sb.Append(TAB).Append(TAB).Append("return HDF_FAILURE;\n");
    sb.Append(TAB).Append("}\n\n");

    sb.Append(TAB).AppendFormat("%s->ioService.Dispatch = %sDriverDispatch;\n", hostName_.c_str(), baseName_.c_str());
    sb.Append(TAB).AppendFormat("%s->ioService.Open = NULL;\n", hostName_.c_str());
    sb.Append(TAB).AppendFormat("%s->ioService.Release = NULL;\n", hostName_.c_str());
    sb.Append(TAB).AppendFormat("%s->service = serviceImpl;\n", hostName_.c_str());
    sb.Append(TAB).AppendFormat("%s->stubObject = stubObj;\n", hostName_.c_str());
    sb.Append(TAB).AppendFormat("deviceObject->service = &%s->ioService;\n", hostName_.c_str());
    sb.Append(TAB).Append("return HDF_SUCCESS;\n");
    sb.Append("}\n");
}

void CServiceDriverCodeEmitter::EmitKernelDriverRelease(StringBuilder &sb)
{
    sb.AppendFormat("static void Hdf%sDriverRelease(struct HdfDeviceObject *deviceObject)\n", baseName_.c_str());
    sb.Append("{\n");
    sb.Append(TAB).AppendFormat("HDF_LOGI(\"Hdf%sDriverRelease enter.\");\n", baseName_.c_str());
    sb.Append(TAB).Append("HDF_LOGI(\"%{public}s: driver release start\", __func__);\n");
    sb.Append(TAB).Append(TAB).Append("return;\n");
    sb.Append(TAB).Append("}\n\n");
    sb.Append(TAB).AppendFormat("struct Hdf%sHost *%s = CONTAINER_OF(", baseName_.c_str(), hostName_.c_str());
    sb.AppendFormat("deviceObject->service, struct Hdf%sHost, ioService);\n", baseName_.c_str());
    sb.Append(TAB).AppendFormat("if (%s != NULL) {\n", hostName_.c_str());
    sb.Append(TAB).Append(TAB).AppendFormat("%sServiceRelease(%s->service);\n", baseName_.c_str(), hostName_.c_str());
    sb.Append(TAB).Append(TAB).AppendFormat("OsalMemFree(%s);\n", hostName_.c_str());
    sb.Append(TAB).Append("}\n");
    sb.Append("}\n");
}

void CServiceDriverCodeEmitter::EmitDriverRelease(StringBuilder &sb)
{
    sb.AppendFormat("static void Hdf%sDriverRelease(struct HdfDeviceObject *deviceObject)\n", baseName_.c_str());
    sb.Append("{\n");
    sb.Append(TAB).Append("HDF_LOGI(\"%{public}s: driver release start\", __func__);\n");

    sb.Append(TAB).Append("if (deviceObject->service == NULL) {\n");
    sb.Append(TAB).Append(TAB).Append("return;\n");
    sb.Append(TAB).Append("}\n\n");

    sb.Append(TAB).AppendFormat("struct Hdf%sHost *%s = CONTAINER_OF(", baseName_.c_str(), hostName_.c_str());
    sb.AppendFormat("deviceObject->service, struct Hdf%sHost, ioService);\n", baseName_.c_str());
    sb.Append(TAB).AppendFormat("if (%s != NULL) {\n", hostName_.c_str());
    sb.Append(TAB).Append(TAB).AppendFormat("StubCollectorRemoveObject(%s, %s->service);\n",
        interface_->EmitDescMacroName().c_str(), hostName_.c_str());
    sb.Append(TAB).Append(TAB).AppendFormat("%sRelease(%s->service, true);\n", interfaceName_.c_str(),
        hostName_.c_str());
    sb.Append(TAB).Append(TAB).AppendFormat("OsalMemFree(%s);\n", hostName_.c_str());
    sb.Append(TAB).Append("}\n");
    sb.Append("}\n");
}

void CServiceDriverCodeEmitter::EmitDriverEntryDefinition(StringBuilder &sb) const
{
    sb.AppendFormat("struct HdfDriverEntry g_%sDriverEntry = {\n", StringHelper::StrToLower(baseName_).c_str());
    sb.Append(TAB).Append(".moduleVersion = 1,\n");
    sb.Append(TAB).AppendFormat(".moduleName = \"%s\",\n", Options::GetInstance().GetPackage().c_str());
    sb.Append(TAB).AppendFormat(".Bind = Hdf%sDriverBind,\n", baseName_.c_str());
    sb.Append(TAB).AppendFormat(".Init = Hdf%sDriverInit,\n", baseName_.c_str());
    sb.Append(TAB).AppendFormat(".Release = Hdf%sDriverRelease,\n", baseName_.c_str());
    sb.Append("};\n\n");
    sb.AppendFormat("HDF_INIT(g_%sDriverEntry);\n", StringHelper::StrToLower(baseName_).c_str());
}
} // namespace HDI
} // namespace OHOS