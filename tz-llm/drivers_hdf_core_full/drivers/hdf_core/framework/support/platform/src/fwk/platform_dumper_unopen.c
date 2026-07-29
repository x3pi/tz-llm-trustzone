/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */
#include "platform_dumper.h"

struct PlatformDumper *PlatformDumperCreate(const char *name)
{
    (void)name;
    return NULL;
}

void PlatformDumperDestroy(struct PlatformDumper *dumper)
{
    (void)dumper;
}

int32_t PlatformDumperDump(struct PlatformDumper *dumper)
{
    (void)dumper;
    return HDF_SUCCESS;
}

int32_t PlatformDumperAddData(struct PlatformDumper *dumper, const struct PlatformDumperData *data)
{
    (void)dumper;
    (void)data;
    return HDF_SUCCESS;
}

int32_t PlatformDumperAddDatas(struct PlatformDumper *dumper, struct PlatformDumperData datas[], int size)
{
    (void)dumper;
    (void)datas;
    (void)size;
    return HDF_SUCCESS;
}

int32_t PlatformDumperSetMethod(struct PlatformDumper *dumper, struct PlatformDumperMethod *ops)
{
    (void)dumper;
    (void)ops;
    return HDF_SUCCESS;
}

int32_t PlatformDumperDelData(struct PlatformDumper *dumper, const char *name, enum PlatformDumperDataType type)
{
    (void)dumper;
    (void)name;
    (void)type;
    return HDF_SUCCESS;
}

int32_t PlatformDumperClearDatas(struct PlatformDumper *dumper)
{
    (void)dumper;
    return HDF_SUCCESS;
}
