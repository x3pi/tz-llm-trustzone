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

#include "hdf_dump.h"

#include "file_ex.h"
#include "string_ex.h"

#include "hdf_base.h"
#include "hdf_dump_reg.h"
#include "hdf_log.h"
#include "hdf_sbuf.h"

#define HDF_LOG_TAG hdf_dump

static DevHostDumpFunc g_dump = nullptr;

// The maximum parameter is the parameter sent to the host, including public(num=2) and private(mux_num=20) parameters
static constexpr int32_t MAX_PARA_NUM = 22;

void HdfRegisterDumpFunc(DevHostDumpFunc dump)
{
    g_dump = dump;
}

int32_t HdfDump(int32_t fd, const std::vector<std::u16string> &args)
{
    if (g_dump == nullptr) {
        HDF_LOGE("%{public}s g_dump is null",  __func__);
        return HDF_FAILURE;
    }

    uint32_t argv = args.size();
    HDF_LOGI("%{public}s argv:%{public}u", __func__, argv);
 
    if (argv > MAX_PARA_NUM) {
        HDF_LOGE("%{public}s argv %{public}u is invalid",  __func__, argv);
        return HDF_FAILURE;
    }

    struct HdfSBuf *data = HdfSbufTypedObtain(SBUF_IPC);
    if (data == nullptr) {
        return HDF_FAILURE;
    }

    int32_t ret = HDF_SUCCESS;
    std::string result;
    const char *value = nullptr;

    struct HdfSBuf *reply = HdfSbufTypedObtain(SBUF_IPC);
    if (reply == nullptr) {
        goto FINISHED;
    }

    if (!HdfSbufWriteUint32(data, argv)) {
        goto FINISHED;
    }

    // Convert vector to sbuf structure
    for (uint32_t i = 0; i < argv; i++) {
        HDF_LOGI("%{public}s para:%{public}s", __func__, OHOS::Str16ToStr8(args.at(i)).data());
        if (!HdfSbufWriteString(data, OHOS::Str16ToStr8(args.at(i)).data())) {
            goto FINISHED;
        }
    }

    (void)g_dump(data, reply);

    value = HdfSbufReadString(reply);
    while (value != nullptr) {
        HDF_LOGI("%{public}s reply:%{public}s", __func__, value);
        result.append(value);
        value = HdfSbufReadString(reply);
    }

    if (!OHOS::SaveStringToFd(fd, result)) {
        ret = HDF_FAILURE;
    }

FINISHED:
    HdfSbufRecycle(data);
    HdfSbufRecycle(reply);
    return ret;
}