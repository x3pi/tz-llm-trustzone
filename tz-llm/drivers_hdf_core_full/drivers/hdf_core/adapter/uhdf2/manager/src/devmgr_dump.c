/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "securec.h"

#include "devmgr_service.h"
#include "devhost_service_clnt.h"
#include "devhost_service_proxy.h"
#include "device_token_clnt.h"
#include "devmgr_service.h"
#include "devsvc_manager.h"
#include "hdf_dlist.h"
#include "hdf_dump_reg.h"
#include "hdf_io_service.h"
#include "hdf_log.h"
#include "hdf_sbuf.h"
#include "devmgr_dump.h"

#define HDF_LOG_TAG devmgr_dump
 
static const char *HELP_COMMENT =
    " usage:\n"
    " -help  :display help information\n"
    " -query :query all services and devices information\n"
    " -host hostName parameter1 parameter2 ... :dump for host, maximum number of parameters is 20\n"
    " -service serviceName parameter1 parameter2 ... :dump for device service, maximum number of parameters is 20\n";

static const uint32_t DATA_SIZE = 5000;
static const uint32_t LINE_SIZE = 128;

static int32_t DevMgrDumpHostFindHost(const char *hostName, struct HdfSBuf *data, struct HdfSBuf *reply)
{
    struct DevmgrService *devMgrSvc = (struct DevmgrService *)DevmgrServiceGetInstance();
    if (devMgrSvc == NULL) {
        return HDF_FAILURE;
    }

    struct DevHostServiceClnt *hostClnt = NULL;
    int32_t ret = HDF_FAILURE;
    bool findFlag = false;

    DLIST_FOR_EACH_ENTRY(hostClnt, &devMgrSvc->hosts, struct DevHostServiceClnt, node) {
        HDF_LOGI("%{public}s hostName:%{public}s %{public}s", __func__, hostClnt->hostName, hostName);
        if (strcmp(hostClnt->hostName, hostName) != 0) {
            continue;
        }
        findFlag = true;
        if (hostClnt->hostService == NULL || hostClnt->hostService->Dump == NULL) {
            (void)HdfSbufWriteString(reply, "The host does not start\n");
            break;
        }
        ret = hostClnt->hostService->Dump(hostClnt->hostService, data, reply);
        break;
    }

    if (!findFlag) {
        (void)HdfSbufWriteString(reply, "The host does not exist\n");
    }

    return ret;
}

static int32_t DevMgrDumpHost(uint32_t argv, struct HdfSBuf *data, struct HdfSBuf *reply)
{
    const char *hostName = HdfSbufReadString(data);
    if (hostName == NULL) {
        HDF_LOGE("%{public}s hostName is null", __func__);
        return HDF_FAILURE;
    }

    struct HdfSBuf *hostData = HdfSbufTypedObtain(SBUF_IPC);
    if (hostData == NULL) {
        return HDF_FAILURE;
    }

    // this parameters are processed in the framework part and will not be sent to the business module
    if (!HdfSbufWriteString(hostData, "dumpHost")) {
        HdfSbufRecycle(hostData);
        return HDF_FAILURE;
    }

    // the hostName will not be send to host
    if (!HdfSbufWriteUint32(hostData, argv - 1)) {
        HdfSbufRecycle(hostData);
        return HDF_FAILURE;
    }

    for (uint32_t i = 0; i < argv - 1; i++) {
        const char *value = HdfSbufReadString(data);
        if (value == NULL || !HdfSbufWriteString(hostData, value)) {
            HdfSbufRecycle(hostData);
            return HDF_FAILURE;
        }
    }

    int32_t ret = DevMgrDumpHostFindHost(hostName, hostData, reply);

    HdfSbufRecycle(hostData);
    return ret;
}

static int32_t DevMgrDumpServiceFindHost(const char *servName, struct HdfSBuf *data, struct HdfSBuf *reply)
{
    struct DevmgrService *devMgrSvc = (struct DevmgrService *)DevmgrServiceGetInstance();
    if (devMgrSvc == NULL) {
        return HDF_FAILURE;
    }

    struct HdfSListIterator iterator;
    struct DevHostServiceClnt *hostClnt = NULL;
    int32_t ret = HDF_FAILURE;
    const char *name = NULL;
    struct HdfSListNode *node = NULL;
    DLIST_FOR_EACH_ENTRY(hostClnt, &devMgrSvc->hosts, struct DevHostServiceClnt, node) {
        HdfSListIteratorInit(&iterator, &hostClnt->devices);
        while (HdfSListIteratorHasNext(&iterator)) {
            node = HdfSListIteratorNext(&iterator);
            struct DeviceTokenClnt *tokenClnt = (struct DeviceTokenClnt *)node;
            if (tokenClnt == NULL || tokenClnt->tokenIf == NULL) {
                continue;
            }
            name = (tokenClnt->tokenIf->servName == NULL) ? "" : tokenClnt->tokenIf->servName;
            HDF_LOGI("%{public}s servName:%{public}s %{public}s", __func__, name, servName);
            if (strcmp(name, servName) != 0) {
                continue;
            }
            if (hostClnt->hostService == NULL || hostClnt->hostService->Dump == NULL) {
                return ret;
            }
            ret = hostClnt->hostService->Dump(hostClnt->hostService, data, reply);
            return ret;
        }
    }

    (void)HdfSbufWriteString(reply, "The service does not exist\n");
    return ret;
}

static int32_t DevMgrDumpService(uint32_t argv, struct HdfSBuf *data, struct HdfSBuf *reply)
{
    const char *servName = HdfSbufReadString(data);
    if (servName == NULL) {
        HDF_LOGE("%{public}s serviceName is null", __func__);
        return HDF_FAILURE;
    }

    struct HdfSBuf *servData = HdfSbufTypedObtain(SBUF_IPC);
    if (servData == NULL) {
        return HDF_FAILURE;
    }

    // this parameters are processed in the framework part and will not be sent to the business module
    if (!HdfSbufWriteString(servData, "dumpService")) {
        HdfSbufRecycle(servData);
        return HDF_FAILURE;
    }

    // this parameters are processed in the framework part and will not be sent to the business module
    if (!HdfSbufWriteString(servData, servName)) {
        HdfSbufRecycle(servData);
        return HDF_FAILURE;
    }

    if (!HdfSbufWriteUint32(servData, argv - 1)) {
        HdfSbufRecycle(servData);
        return HDF_FAILURE;
    }

    // the servName will not be send to business module
    for (uint32_t i = 0; i < argv - 1; i++) {
        const char *value = HdfSbufReadString(data);
        if (value == NULL || !HdfSbufWriteString(servData, value)) {
            HdfSbufRecycle(servData);
            return HDF_FAILURE;
        }
    }

    int32_t ret = DevMgrDumpServiceFindHost(servName, servData, reply);
    HdfSbufRecycle(servData);
    return ret;
}

static int32_t DevMgrFillDeviceHostInfo(struct HdfSBuf *data, struct HdfSBuf *reply)
{
    const char *name = HdfSbufReadString(data);
    if (name == NULL) {
        return HDF_FAILURE;
    }

    char line[LINE_SIZE];
    // The line is a combination of multiple fields, and the fields are filled with blank characters
    (void)memset_s(line, sizeof(line), ' ', sizeof(line));
    if (memcpy_s(line, sizeof(line), name, strlen(name)) != EOK) {
        HDF_LOGE("%{public}s memcpy_s hostName fail", __func__);
        return HDF_FAILURE;
    }
    HDF_LOGI("%{public}s devName:%{public}s", __func__, name);

    uint32_t hostId;
    (void)HdfSbufReadUint32(data, &hostId);
    const uint32_t hostIdAlign = 32;
    if (sprintf_s(&line[hostIdAlign], sizeof(line) - hostIdAlign, ":0x%x\n", hostId) == -1) {
        HDF_LOGE("%{public}s sprintf_s hostId fail", __func__);
        return HDF_FAILURE;
    }

    (void)HdfSbufWriteString(reply, line);
    return HDF_SUCCESS;
}

static void DevMgrFillDeviceInfo(struct HdfSBuf *data, struct HdfSBuf *reply, uint32_t *hostCnt, uint32_t *nodeCnt)
{
    char line[LINE_SIZE];
    const uint32_t devNameAlign = 8;
    const uint32_t devIdAlign = 40;
    const uint32_t servNameAlign = 56;
    uint32_t devCnt;
    uint32_t devId;
    const uint32_t strEndLen = 2;

    while (true) {
        if (DevMgrFillDeviceHostInfo(data, reply) != HDF_SUCCESS) {
            return;
        }

        (void)HdfSbufReadUint32(data, &devCnt);
        (*hostCnt)++;

        for (uint32_t i = 0; i < devCnt; i++) {
            // The line is a combination of multiple fields, and the fields are filled with blank characters
            (void)memset_s(line, sizeof(line), ' ', sizeof(line));

            const char *name = HdfSbufReadString(data);
            const char *devName = (name == NULL) ? "" : name;
            if (memcpy_s(&line[devNameAlign], sizeof(line) - devNameAlign, devName, strlen(devName)) != EOK) {
                HDF_LOGE("%{public}s memcpy_s devName fail", __func__);
                return;
            }
            HDF_LOGI("%{public}s devName:%{public}s", __func__, devName);

            (void)HdfSbufReadUint32(data, &devId);
            int32_t devIdLen = sprintf_s(&line[devIdAlign], sizeof(line) - devIdAlign - 1, ":0x%x", devId);
            if (devIdLen == -1) {
                HDF_LOGE("%{public}s sprintf_s devId fail", __func__);
                return;
            }
            line[devIdAlign + devIdLen] = ' '; // Clear the string terminator added by sprintf_s
            line[servNameAlign - 1] = ':';

            name = HdfSbufReadString(data);
            const char *servName = (name == NULL) ? "" : name;
            const uint32_t leftSize = sizeof(line) - servNameAlign - strEndLen;
            if (memcpy_s(&line[servNameAlign], leftSize, servName, strlen(servName)) != EOK) {
                HDF_LOGE("%{public}s memcpy_s servName fail %{public}s", __func__, servName);
                return;
            }
            line[servNameAlign + strlen(servName)] = '\n';
            line[servNameAlign + strlen(servName) + 1] = '\0';

            (void)HdfSbufWriteString(reply, line);
            (*nodeCnt)++;
        }
    }
    return;
}

static void DevMgrFillServiceInfo(struct HdfSBuf *data, struct HdfSBuf *reply, uint32_t *servCnt)
{
    char line[LINE_SIZE];
    const uint32_t devClassAlign = 32;
    const uint32_t devIdAlign = 48;
    const char *servName = NULL;
    uint32_t devClass;
    uint32_t devId;

    while (true) {
        servName = HdfSbufReadString(data);
        if (servName == NULL) {
            return;
        }

        // The line is a combination of multiple fields, and the fields are filled with blank characters
        (void)memset_s(line, sizeof(line), ' ', sizeof(line));
        if (memcpy_s(line, sizeof(line), servName, strlen(servName)) != EOK) {
            HDF_LOGE("%{public}s memcpy_s servName fail", __func__);
            return;
        }
        HDF_LOGI("%{public}s servName:%{public}s", __func__, servName);

        (void)HdfSbufReadUint32(data, &devClass);
        int32_t devClassLen = sprintf_s(&line[devClassAlign], sizeof(line) - devClassAlign - 1, ":0x%x", devClass);
        if (devClassLen == -1) {
            HDF_LOGE("%{public}s sprintf_s devClass fail", __func__);
            return;
        }
        line[devClassAlign + devClassLen] = ' '; // Clear the string terminator added by sprintf_s

        (void)HdfSbufReadUint32(data, &devId);
        if (sprintf_s(&line[devIdAlign], sizeof(line) - devIdAlign, ":0x%x\n", devId) == -1) {
            HDF_LOGE("%{public}s sprintf_s devId fail", __func__);
            return;
        }

        (void)HdfSbufWriteString(reply, line);
        (*servCnt)++;
    }

    return;
}

static void DevMgrQueryUserDevice(struct HdfSBuf *reply)
{
    const char *title = "hdf device information in user space, format:\n" \
                        "hostName                        :hostId\n" \
                        "        deviceName                      :deviceId      :serviceName\n";

    struct IDevmgrService *instance = DevmgrServiceGetInstance();
    if (instance == NULL) {
        return;
    }

    struct HdfSBuf *data = HdfSbufObtain(DATA_SIZE);
    if (data == NULL) {
        return;
    }

    int32_t ret = instance->ListAllDevice(instance, data);
    if (ret != HDF_SUCCESS) {
        HdfSbufRecycle(data);
        return;
    }

    HdfSbufWriteString(reply, title);

    uint32_t hostCnt = 0;
    uint32_t devNodeCnt = 0;
    DevMgrFillDeviceInfo(data, reply, &hostCnt, &devNodeCnt);

    const uint32_t descLen = 128;
    char desc[descLen];
    (void)memset_s(desc, sizeof(desc), 0, sizeof(desc));
    if (sprintf_s(desc, sizeof(desc), "total %u hosts, %u devNodes in user space\n\n", hostCnt, devNodeCnt) != -1) {
        HdfSbufWriteString(reply, desc);
    }

    HdfSbufRecycle(data);
    return;
}

static void DevMgrQueryUserService(struct HdfSBuf *reply)
{
    const char *title = "hdf service information in user space, format:\n" \
                        "serviceName                     :devClass       :devId\n";

    struct IDevSvcManager *instance = DevSvcManagerGetInstance();
    if (instance == NULL) {
        return;
    }

    struct HdfSBuf *data = HdfSbufObtain(DATA_SIZE);
    if (data == NULL) {
        return;
    }

    instance->ListAllService(instance, data);

    HdfSbufWriteString(reply, title);
    uint32_t servCnt = 0;
    DevMgrFillServiceInfo(data, reply, &servCnt);

    const uint32_t descLen = 128;
    char desc[descLen];
    (void)memset_s(desc, sizeof(desc), 0, sizeof(desc));
    if (sprintf_s(desc, sizeof(desc), "total %u services in user space\n\n", servCnt) != -1) {
        HdfSbufWriteString(reply, desc);
    }

    HdfSbufRecycle(data);
    return;
}

static void DevMgrQueryKernelDevice(struct HdfSBuf *reply)
{
    const char *title = "hdf device information in kernel space, format:\n" \
                        "hostName                        :hostId\n" \
                        "        deviceName                      :deviceId      :serviceName\n";

    struct HdfSBuf *data = HdfSbufObtain(DATA_SIZE);
    if (data == NULL) {
        return;
    }

    int32_t ret =  HdfListAllDevice(data);
    if (ret != HDF_SUCCESS) {
        HdfSbufRecycle(data);
        return;
    }

    uint32_t hostCnt = 0;
    uint32_t devNodeCnt = 0;
    HdfSbufWriteString(reply, title);
    DevMgrFillDeviceInfo(data, reply, &hostCnt, &devNodeCnt);

    const uint32_t descLen = 128;
    char desc[descLen];
    (void)memset_s(desc, sizeof(desc), 0, sizeof(desc));
    if (sprintf_s(desc, sizeof(desc), "total %u hosts, %u devNodes in kernel space\n\n", hostCnt, devNodeCnt) != -1) {
        HdfSbufWriteString(reply, desc);
    }

    HdfSbufRecycle(data);
    return;
}

static void DevMgrQueryKernelService(struct HdfSBuf *reply)
{
    const char *title = "hdf service information in kernel space, format:\n" \
                        "serviceName                     :devClass       :devId\n";

    struct HdfSBuf *data = HdfSbufObtain(DATA_SIZE);
    if (data == NULL) {
        return;
    }

    int32_t ret = HdfListAllService(data);
    if (ret != HDF_SUCCESS) {
        HdfSbufRecycle(data);
        return;
    }

    HdfSbufWriteString(reply, title);
    uint32_t servCnt = 0;
    DevMgrFillServiceInfo(data, reply, &servCnt);

    const uint32_t descLen = 128;
    char desc[descLen];
    (void)memset_s(desc, sizeof(desc), 0, sizeof(desc));
    if (sprintf_s(desc, sizeof(desc), "total %u services in kernel space\n\n", servCnt) != -1) {
        HdfSbufWriteString(reply, desc);
    }

    HdfSbufRecycle(data);
    return;
}

static void DevMgrQueryInfo(struct HdfSBuf *reply)
{
    DevMgrQueryUserDevice(reply);
    DevMgrQueryUserService(reply);
    DevMgrQueryKernelDevice(reply);
    DevMgrQueryKernelService(reply);
    return;
}

static int32_t DevMgrDump(struct HdfSBuf *data, struct HdfSBuf *reply)
{
    if (data == NULL || reply == NULL) {
        return HDF_FAILURE;
    }

    uint32_t argv = 0;
    HdfSbufReadUint32(data, &argv);

    if (argv == 0) {
        (void)HdfSbufWriteString(reply, HELP_COMMENT);
        return HDF_SUCCESS;
    }

    const char *value = HdfSbufReadString(data);
    if (value == NULL) {
        HDF_LOGE("%{public}s arg is invalid", __func__);
        return HDF_FAILURE;
    }

    HDF_LOGI("%{public}s argv:%{public}d", value, argv);
    if (argv == 1) {
        if (strcmp(value, "-help") == 0) {
            (void)HdfSbufWriteString(reply, HELP_COMMENT);
            return HDF_SUCCESS;
        } else if (strcmp(value, "-query") == 0) {
            DevMgrQueryInfo(reply);
            return HDF_SUCCESS;
        } else {
            (void)HdfSbufWriteString(reply, HELP_COMMENT);
            return HDF_SUCCESS;
        }
    } else {
        if (strcmp(value, "-host") == 0) {
            return DevMgrDumpHost(argv - 1, data, reply);
        } else if (strcmp(value, "-service") == 0) {
            return DevMgrDumpService(argv - 1, data, reply);
        } else {
            (void)HdfSbufWriteString(reply, HELP_COMMENT);
            return HDF_SUCCESS;
        }
    }

    return HDF_SUCCESS;
}

void DevMgrRegisterDumpFunc(void)
{
    HdfRegisterDumpFunc(DevMgrDump);
}
