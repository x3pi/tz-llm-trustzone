/*
 * Copyright (c) 2021-2022 Huawei Device Co., Ltd.
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

#include "dev_attribute_serialize.h"
#include "hdf_log.h"
#include "osal_mem.h"

#define HDF_LOG_TAG dev_attr_serialze

#define ATTRIBUTE_PRIVATE_DATA_LENGTH_NULL 0
#define ATTRIBUTE_PRIVATE_DATA_LENGTH_NORMAL 1

bool DeviceAttributeSerialize(const struct HdfDeviceInfo *attribute, struct HdfSBuf *sbuf)
{
    if (attribute == NULL || sbuf == NULL) {
        return false;
    }

    if (!HdfSbufWriteUint32(sbuf, attribute->deviceId) ||
        !HdfSbufWriteUint16(sbuf, attribute->policy) ||
        !HdfSbufWriteString(sbuf, attribute->svcName) ||
        !HdfSbufWriteString(sbuf, attribute->moduleName) ||
        !HdfSbufWriteString(sbuf, attribute->deviceName)) {
        return false;
    }

    if (attribute->deviceMatchAttr != NULL) {
        if (!HdfSbufWriteUint32(sbuf, ATTRIBUTE_PRIVATE_DATA_LENGTH_NORMAL) ||
            !HdfSbufWriteString(sbuf, attribute->deviceMatchAttr)) {
            HDF_LOGE("failed to serialize device attribute");
            return false;
        }
    } else {
        if (!HdfSbufWriteUint32(sbuf, ATTRIBUTE_PRIVATE_DATA_LENGTH_NULL)) {
            HDF_LOGE("failed to serialize device attribute");
            return false;
        }
    }

    return true;
}

static bool DeviceAttributeSet(struct HdfDeviceInfo *attribute, struct HdfSBuf *sbuf)
{
    const char *svcName = HdfSbufReadString(sbuf);
    if (svcName == NULL) {
        HDF_LOGE("Read from sbuf failed, svcName is null");
        return false;
    }
    attribute->svcName = strdup(svcName);
    if (attribute->svcName == NULL) {
        HDF_LOGE("Read from sbuf failed, strdup svcName fail");
        return false;
    }

    const char *moduleName = HdfSbufReadString(sbuf);
    if (moduleName == NULL) {
        HDF_LOGE("Read from parcel failed, moduleName is null");
        return false;
    }
    attribute->moduleName = strdup(moduleName);
    if (attribute->moduleName == NULL) {
        HDF_LOGE("Read from sbuf failed, strdup moduleName fail");
        return false;
    }

    const char *deviceName = HdfSbufReadString(sbuf);
    if (deviceName == NULL) {
        HDF_LOGE("Read from sbuf failed, deviceName is null");
        return false;
    }
    attribute->deviceName = strdup(deviceName);
    if (attribute->deviceName == NULL) {
        HDF_LOGE("Read from sbuf failed, strdup deviceName fail");
        return false;
    }

    uint32_t length;
    if (!HdfSbufReadUint32(sbuf, &length)) {
        HDF_LOGE("Device attribute readDeviceMatchAttr length failed");
        return false;
    }
    if (length == ATTRIBUTE_PRIVATE_DATA_LENGTH_NORMAL) {
        const char *deviceMatchAttr = HdfSbufReadString(sbuf);
        if (deviceMatchAttr == NULL) {
            HDF_LOGE("%s: Read from sbuf failed, deviceMatchAttr is null", __func__);
            return false;
        }
        attribute->deviceMatchAttr = strdup(deviceMatchAttr);
    }

    return true;
}

struct HdfDeviceInfo *DeviceAttributeDeserialize(struct HdfSBuf *sbuf)
{
    if (sbuf == NULL) {
        return NULL;
    }

    struct HdfDeviceInfo *attribute = HdfDeviceInfoNewInstance();
    if (attribute == NULL) {
        HDF_LOGE("OsalMemCalloc failed, attribute is null");
        return NULL;
    }

    if (!HdfSbufReadUint32(sbuf, &attribute->deviceId) || !HdfSbufReadUint16(sbuf, &attribute->policy)) {
        HDF_LOGE("invalid deviceId or policy");
        DeviceSerializedAttributeRelease(attribute);
        return NULL;
    }

    if (DeviceAttributeSet(attribute, sbuf)) {
        return attribute;
    }

    DeviceSerializedAttributeRelease(attribute);
    return NULL;
}

void DeviceSerializedAttributeRelease(struct HdfDeviceInfo *attribute)
{
    if (attribute == NULL) {
        return;
    }

    if (attribute->moduleName != NULL) {
        OsalMemFree((void *)attribute->moduleName);
    }
    if (attribute->svcName != NULL) {
        OsalMemFree((void *)attribute->svcName);
    }
    if (attribute->deviceName != NULL) {
        OsalMemFree((void *)attribute->deviceName);
    }
    if (attribute->deviceMatchAttr != NULL) {
        OsalMemFree((void *)attribute->deviceMatchAttr);
    }
    OsalMemFree(attribute);
}
