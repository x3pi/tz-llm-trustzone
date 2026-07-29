/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#ifndef VDI_SAMPLE2_DRIVER_H
#define VDI_SAMPLE2_DRIVER_H

#include "hdf_load_vdi.h"

namespace OHOS {
namespace VDI {
namespace Sample {
namespace V1_0 {
class IVdiSample {
public:
    virtual int ServiceA(void) = 0;
    virtual int ServiceB(IVdiSample *) = 0;
};

class VdiSample : public IVdiSample {
public:
    int ServiceA(void) override;
    int ServiceB(IVdiSample *vdi) override;
    explicit VdiSample(int para);
    virtual ~VdiSample() = default;
private:
    int priData;
};

struct VdiWrapperB {
    struct HdfVdiBase base;
    IVdiSample *module;
};
} // namespace V1_0
} // namespace Sample
} // namespace VDI
} // namespace OHOS

#endif /* VDI_SAMPLE1_DRIVER_H */
