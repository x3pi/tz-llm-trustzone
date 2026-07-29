/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include <gtest/gtest.h>
#include "hdf_io_service.h"
#include "hdf_load_vdi.h"
#include "securec.h"
#include "vdi_sample1_driver.h"
#include "vdi_sample1_symbol.h"
#include "vdi_sample2_driver.h"

namespace OHOS {
using namespace testing::ext;
using OHOS::VDI::Sample::V1_0::VdiWrapperB;
using OHOS::VDI::Sample::V1_0::VdiSample;

class HdfVdiTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp();
    void TearDown();
};

void HdfVdiTest::SetUpTestCase()
{
}

void HdfVdiTest::TearDownTestCase()
{
}

void HdfVdiTest::SetUp()
{
}

void HdfVdiTest::TearDown()
{
}

HWTEST_F(HdfVdiTest, HdfVdiTestSampleABase, TestSize.Level3)
{
    struct HdfVdiObject *vdi = nullptr;
    vdi = HdfLoadVdi("libvdi_sample1_driver.z.so");
    ASSERT_TRUE(vdi != nullptr);
    ASSERT_TRUE(vdi->vdiBase != nullptr);

    uint32_t version = HdfGetVdiVersion(vdi);
    ASSERT_TRUE(version == 1);

    struct VdiWrapperA *vdiWrapper = reinterpret_cast<struct VdiWrapperA *>(vdi->vdiBase);
    ASSERT_TRUE(vdiWrapper->module != nullptr);
    struct ModuleA *modA = reinterpret_cast<struct ModuleA *>(vdiWrapper->module);
    int ret = modA->ServiceA();
    ASSERT_TRUE(ret == HDF_SUCCESS);
    ret = modA->ServiceB(modA);
    ASSERT_TRUE(ret == HDF_SUCCESS);

    HdfCloseVdi(vdi);
}

HWTEST_F(HdfVdiTest, HdfVdiTestSampleAErrorSo, TestSize.Level3)
{
    struct HdfVdiObject *vdi = nullptr;
    vdi = HdfLoadVdi("libvdi_sample1_driver_error.z.so");
    ASSERT_TRUE(vdi == nullptr);
    HdfCloseVdi(vdi);
}

HWTEST_F(HdfVdiTest, HdfVdiTestSampleBBase, TestSize.Level3)
{
    struct HdfVdiObject *vdi = nullptr;
    vdi = HdfLoadVdi("libvdi_sample2_driver.z.so");
    ASSERT_TRUE(vdi != nullptr);
    ASSERT_TRUE(vdi->vdiBase != nullptr);

    uint32_t version = HdfGetVdiVersion(vdi);
    ASSERT_TRUE(version == 1);

    struct VdiWrapperB *vdiWrapper = reinterpret_cast<struct VdiWrapperB *>(vdi->vdiBase);
    ASSERT_TRUE(vdiWrapper->module != nullptr);
    VdiSample *vdiSample = reinterpret_cast<VdiSample *>(vdiWrapper->module);
    int ret = vdiSample->ServiceA();
    ASSERT_TRUE(ret == HDF_SUCCESS);
    ret = vdiSample->ServiceB(vdiSample);
    ASSERT_TRUE(ret == HDF_SUCCESS);

    HdfCloseVdi(vdi);
}

HWTEST_F(HdfVdiTest, HdfVdiTestSampleBErrorSo, TestSize.Level3)
{
    struct HdfVdiObject *vdi = nullptr;
    vdi = HdfLoadVdi("libvdi_sample2_driver_error.z.so");
    ASSERT_TRUE(vdi == nullptr);
    HdfCloseVdi(vdi);
}

HWTEST_F(HdfVdiTest, HdfVdiTestLoadInvalidLibName, TestSize.Level3)
{
    struct HdfVdiObject *vdi = nullptr;
    vdi = HdfLoadVdi(nullptr);
    ASSERT_TRUE(vdi == nullptr);
    HdfCloseVdi(vdi);

    char libName[PATH_MAX + 1];
    (void)memset_s(libName, PATH_MAX, 'a', PATH_MAX);
    libName[PATH_MAX] = 0;
    vdi = HdfLoadVdi(libName);
    ASSERT_TRUE(vdi == nullptr);

    HdfCloseVdi(vdi);
}

HWTEST_F(HdfVdiTest, HdfVdiTestLoadInvalidSymbol, TestSize.Level3)
{
    struct HdfVdiObject *vdi = nullptr;
    vdi = HdfLoadVdi("libvdi_sample1_symbol.z.so");
    ASSERT_TRUE(vdi == nullptr);
}

HWTEST_F(HdfVdiTest, HdfVdiTestNulVdiGetVersion, TestSize.Level3)
{
    struct HdfVdiObject *vdi = nullptr;
    uint32_t version = HdfGetVdiVersion(vdi);
    HdfCloseVdi(vdi);
    ASSERT_TRUE(version == HDF_INVALID_VERSION);
}

HWTEST_F(HdfVdiTest, HdfVdiTestAbnormal, TestSize.Level3)
{
    struct HdfVdiObject obj;
    struct HdfVdiBase base;
    struct HdfVdiObject *vdi = &obj;

    obj.vdiBase = &base;
    obj.dlHandler = 0;
    HdfCloseVdi(vdi);

    obj.vdiBase = nullptr;
    obj.dlHandler = 1;
    HdfCloseVdi(vdi);

    uint32_t version = HdfGetVdiVersion(vdi);
    ASSERT_TRUE(version == HDF_INVALID_VERSION);
}
} // namespace OHOS
