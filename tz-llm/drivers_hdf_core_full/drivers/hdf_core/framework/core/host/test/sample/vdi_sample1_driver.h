/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#ifndef VDI_SAMPLE1_DRIVER_H
#define VDI_SAMPLE1_DRIVER_H

#include "hdf_load_vdi.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

struct ModuleA {
    int (*ServiceA)(void);
    int (*ServiceB)(struct ModuleA *modA);
    int priData;
};

struct VdiWrapperA {
    struct HdfVdiBase base;
    struct ModuleA *module;
};

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif /* VDI_SAMPLE1_DRIVER_H */