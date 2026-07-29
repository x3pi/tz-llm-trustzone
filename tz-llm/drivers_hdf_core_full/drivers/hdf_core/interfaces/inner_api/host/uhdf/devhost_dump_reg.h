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

#ifndef DEVHOST_DUMP_REG_H
#define DEVHOST_DUMP_REG_H

#include "hdf_sbuf.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef int32_t (*DevHostDumpFunc)(struct HdfSBuf *data, struct HdfSBuf *reply);

int32_t DevHostRegisterDumpHost(DevHostDumpFunc dump);
int32_t DevHostRegisterDumpService(const char *servName, DevHostDumpFunc dump);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* DEVHOST_DUMP_REG_H */
