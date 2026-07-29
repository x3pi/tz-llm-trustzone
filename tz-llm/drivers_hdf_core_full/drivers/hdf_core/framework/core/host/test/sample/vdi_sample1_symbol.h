/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#ifndef VDI_SAMPLE1_SYMBOL_H
#define VDI_SAMPLE1_SYMBOL_H

#include "hdf_load_vdi.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

struct ModuleA1 {
    int (*ServiceA)(void);
    int (*ServiceB)(struct ModuleA1 *modA);
    int priData;
};

struct VdiWrapperA1 {
    struct HdfVdiBase base;
    struct ModuleA1 *module;
};

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif /* VDI_SAMPLE1_SYMBOL_H */
