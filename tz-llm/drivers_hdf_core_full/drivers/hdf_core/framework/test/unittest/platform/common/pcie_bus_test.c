/*
 * Copyright (c) 2023 Shenzhen Kaihong Digital Industry Development Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include "pcie_bus_test.h"
#include <securec.h>
#include "hdf_base.h"
#include "hdf_io_service_if.h"
#include "hdf_log.h"
#include "osal_mem.h"
#include "pcie_if.h"
#define USER_LEM_MAX           8192
#define DMA_ALIGN_SIZE         256
#define DMA_TEST_LEN           256
#define PCIE_TEST_DISABLE_ADDR 0xB7
#define PCIE_TEST_UPPER_ADDR   0x28
#define PCIE_TEST_CMD_ADDR     0x04

struct PcieBusTestFunc {
    int32_t cmd;
    int32_t (*func)(struct PcieBusTester *tester);
};

static int32_t TestPcieBusGetInfo(struct PcieBusTester *tester)
{
    int32_t ret;
    struct BusConfig config;
    if (tester == NULL) {
        HDF_LOGE("%s: tester is null.", __func__);
        return HDF_ERR_INVALID_OBJECT;
    }
    ret = tester->busDev.ops.getBusInfo(&tester->busDev, &config);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: getBusInfo failed ret = %d.", __func__, ret);
        return ret;
    }
    return HDF_SUCCESS;
}

static int32_t TestPcieBusDataReadWrite(struct PcieBusTester *tester)
{
    uint8_t disable;
    uint32_t upper;
    uint16_t cmd;
    int32_t ret;
    if (tester == NULL) {
        HDF_LOGE("%s: tester is null.", __func__);
        return HDF_ERR_INVALID_OBJECT;
    }
   
    ret = tester->busDev.ops.readData(&tester->busDev, PCIE_TEST_DISABLE_ADDR, sizeof(disable), &disable);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: disable read failed ret = %d.", __func__, ret);
        return ret;
    }
    HDF_LOGD("%s: disable is %d", __func__, disable);

    ret = tester->busDev.ops.writeData(&tester->busDev, PCIE_TEST_DISABLE_ADDR, sizeof(disable), &disable);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: disable write failed ret = %d.", __func__, ret);
        return ret;
    }

    ret = tester->busDev.ops.readData(&tester->busDev, PCIE_TEST_UPPER_ADDR, sizeof(upper), (uint8_t *)&upper);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: upper read failed ret = %d.", __func__, ret);
        return ret;
    }
    HDF_LOGD("%s: upper is %d", __func__, upper);

    ret = tester->busDev.ops.writeData(&tester->busDev, PCIE_TEST_UPPER_ADDR, sizeof(upper), (uint8_t *)&upper);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: upper write failed ret = %d.", __func__, ret);
        return ret;
    }

    ret = tester->busDev.ops.readData(&tester->busDev, PCIE_TEST_CMD_ADDR, sizeof(cmd), (uint8_t *)&cmd);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: cmd read failed ret = %d.", __func__, ret);
        return ret;
    }
    HDF_LOGD("%s: cmd is %d", __func__, disable);

    ret = tester->busDev.ops.writeData(&tester->busDev, PCIE_TEST_CMD_ADDR, sizeof(cmd), (uint8_t *)&cmd);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: cmd write failed ret = %d.", __func__, ret);
        return ret;
    }
    return HDF_SUCCESS;
}

static int32_t TestPcieBusBulkReadWrite(struct PcieBusTester *tester)
{
    uint32_t data;
    int32_t ret;
    if (tester == NULL) {
        HDF_LOGE("%s: tester is null.", __func__);
        return HDF_ERR_INVALID_OBJECT;
    }
    ret = tester->busDev.ops.bulkRead(&tester->busDev, PCIE_TEST_UPPER_ADDR, sizeof(data), (uint8_t *)&data,
                                      sizeof(data));
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: readFunc0 failed ret = %d.", __func__, ret);
        return ret;
    }
    ret = tester->busDev.ops.bulkWrite(&tester->busDev, PCIE_TEST_UPPER_ADDR, sizeof(data), (uint8_t *)&data,
                                       sizeof(data));
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: writeFunc0 failed ret = %d.", __func__, ret);
        return ret;
    }
    return HDF_SUCCESS;
}

static int32_t TestPcieBusFunc0ReadWrite(struct PcieBusTester *tester)
{
    uint32_t data;
    int32_t ret;
    if (tester == NULL) {
        HDF_LOGE("%s: tester is null.", __func__);
        return HDF_ERR_INVALID_OBJECT;
    }
    ret = tester->busDev.ops.readFunc0(&tester->busDev, PCIE_TEST_UPPER_ADDR, sizeof(data), (uint8_t *)&data);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: readFunc0 failed ret = %d.", __func__, ret);
        return ret;
    }
    ret = tester->busDev.ops.writeFunc0(&tester->busDev, PCIE_TEST_UPPER_ADDR, sizeof(data), (uint8_t *)&data);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: writeFunc0 failed ret = %d.", __func__, ret);
        return ret;
    }
    return HDF_SUCCESS;
}

static void PcieBusIrqHandler(void *arg)
{
    HDF_LOGD("%s: data is %{public}p", __func__, arg);
}

static int32_t TestPcieBusIrqClaimRelease(struct PcieBusTester *tester)
{
    int32_t ret;
    if (tester == NULL) {
        HDF_LOGE("%s: tester is null.", __func__);
        return HDF_ERR_INVALID_OBJECT;
    }
    ret = tester->busDev.ops.claimIrq(&tester->busDev, PcieBusIrqHandler, NULL);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: claimIrq failed ret = %d.", __func__, ret);
        return ret;
    }

    ret = tester->busDev.ops.releaseIrq(&tester->busDev);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: releaseIrq failed ret = %d.", __func__, ret);
        return ret;
    }
    return HDF_SUCCESS;
}

static int32_t TestPcieBusDisalbeReset(struct PcieBusTester *tester)
{
    int32_t ret;
    if (tester == NULL) {
        HDF_LOGE("%s: tester is null.", __func__);
        return HDF_ERR_INVALID_OBJECT;
    }
    ret = tester->busDev.ops.disableBus(&tester->busDev);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: disableBus failed ret = %d.", __func__, ret);
        return ret;
    }
    ret = tester->busDev.ops.reset(&tester->busDev);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: disableBus failed ret = %d.", __func__, ret);
        return ret;
    }
    return HDF_SUCCESS;
}

static int32_t TestPcieBusHostClaimRelease(struct PcieBusTester *tester)
{
    if (tester == NULL) {
        HDF_LOGE("%s: tester is null.", __func__);
        return HDF_ERR_INVALID_OBJECT;
    }
    tester->busDev.ops.claimHost(&tester->busDev);
    tester->busDev.ops.releaseHost(&tester->busDev);
    return HDF_SUCCESS;
}

static int32_t TestPcieBusIoReadWrite(struct PcieBusTester *tester)
{
    uint32_t upper;
    int32_t ret;
    if (tester == NULL) {
        HDF_LOGE("%s: tester is null.", __func__);
        return HDF_ERR_INVALID_OBJECT;
    }
    ret = tester->busDev.ops.ioRead(&tester->busDev, PCIE_TEST_UPPER_ADDR, sizeof(upper), (uint8_t *)&upper);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: upper read failed ret = %d.", __func__, ret);
        return ret;
    }
    HDF_LOGD("%s: upper is %d", __func__, upper);

    ret = tester->busDev.ops.ioWrite(&tester->busDev, PCIE_TEST_UPPER_ADDR, sizeof(upper), (uint8_t *)&upper);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: upper write failed ret = %d.", __func__, ret);
        return ret;
    }
    return HDF_SUCCESS;
}

static int32_t PcieBusMapHandler(void *arg)
{
    HDF_LOGD("%s: data is %{public}p", __func__, arg);
    return HDF_SUCCESS;
}

static int32_t TestPcieBusDmaMapUnMap(struct PcieBusTester *tester)
{
    int32_t ret;
    uintptr_t buf = 0;
    if (tester == NULL) {
        HDF_LOGE("%s: tester is null.", __func__);
        return HDF_ERR_INVALID_OBJECT;
    }
    buf = (uintptr_t)OsalMemAllocAlign(DMA_ALIGN_SIZE, DMA_TEST_LEN);
    if (buf == 0) {
        HDF_LOGE("%s: malloc fail", __func__);
        return HDF_ERR_MALLOC_FAIL;
    }
    // dma to device
    ret = tester->busDev.ops.dmaMap(&tester->busDev, PcieBusMapHandler, buf, DMA_TEST_LEN, PCIE_DMA_TO_DEVICE);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: failed ret = %d", __func__, ret);
        OsalMemFree((void *)buf);
        return ret;
    }

    ret = tester->busDev.ops.dmaUnmap(&tester->busDev, buf, DMA_TEST_LEN, PCIE_DMA_TO_DEVICE);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: failed ret = %d", __func__, ret);
        OsalMemFree((void *)buf);
        return ret;
    }

    /* device to dma */
    ret = tester->busDev.ops.dmaMap(&tester->busDev, PcieBusMapHandler, buf, DMA_TEST_LEN, PCIE_DMA_FROM_DEVICE);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: failed ret = %d", __func__, ret);
        OsalMemFree((void *)buf);
        return ret;
    }
    ret = tester->busDev.ops.dmaUnmap(&tester->busDev, buf, DMA_TEST_LEN, PCIE_DMA_FROM_DEVICE);
    OsalMemFree((void *)buf);
    return HDF_SUCCESS;
}

static struct PcieBusTestFunc g_entry[] = {
    {CMD_TEST_PCIE_BUS_GET_INFO, TestPcieBusGetInfo},
    {CMD_TEST_PCIE_BUS_READ_WRITE_DATA, TestPcieBusDataReadWrite},
    {CMD_TEST_PCIE_BUS_READ_WRITE_BULK, TestPcieBusBulkReadWrite},
    {CMD_TEST_PCIE_BUS_READ_WRITE_FUNC0, TestPcieBusFunc0ReadWrite},
    {CMD_TEST_PCIE_BUS_CLAIM_RELEASE_IRQ, TestPcieBusIrqClaimRelease},
    {CMD_TEST_PCIE_BUS_DISABLE_RESET_BUS, TestPcieBusDisalbeReset},
    {CMD_TEST_PCIE_BUS_CLAIM_RELEASE_HOST, TestPcieBusHostClaimRelease},
    {CMD_TEST_PCIE_BUS_READ_WRITE_IO, TestPcieBusIoReadWrite},
    {CMD_TEST_PCIE_BUS_MAP_UNMAP_DMA, TestPcieBusDmaMapUnMap},
};

static int32_t PcieBusTesterGetConfig(struct PcieBusTestConfig *config)
{
    int32_t ret;
    struct HdfSBuf *reply = NULL;
    struct HdfIoService *service = NULL;
    const void *buf = NULL;
    uint32_t len;

    service = HdfIoServiceBind("PCIE_BUS_TEST");
    if ((service == NULL) || (service->dispatcher == NULL) || (service->dispatcher->Dispatch == NULL)) {
        HDF_LOGE("%s: HdfIoServiceBind failed\n", __func__);
        return HDF_ERR_NOT_SUPPORT;
    }

    reply = HdfSbufObtain(sizeof(*config) + sizeof(uint32_t));
    if (reply == NULL) {
        HDF_LOGE("%s: failed to obtain reply", __func__);
        HdfIoServiceRecycle(service);
        return HDF_ERR_MALLOC_FAIL;
    }

    ret = service->dispatcher->Dispatch(&service->object, 0, NULL, reply);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: remote dispatch failed", __func__);
        HdfIoServiceRecycle(service);
        HdfSbufRecycle(reply);
        return ret;
    }

    if (!HdfSbufReadBuffer(reply, &buf, &len)) {
        HDF_LOGE("%s: read buf failed", __func__);
        HdfIoServiceRecycle(service);
        HdfSbufRecycle(reply);
        return HDF_ERR_IO;
    }

    if (len != sizeof(*config)) {
        HDF_LOGE("%s: config size:%zu, read size:%u", __func__, sizeof(*config), len);
        HdfIoServiceRecycle(service);
        HdfSbufRecycle(reply);
        return HDF_ERR_IO;
    }

    if (memcpy_s(config, sizeof(*config), buf, sizeof(*config)) != EOK) {
        HDF_LOGE("%s: memcpy buf failed", __func__);
        HdfIoServiceRecycle(service);
        HdfSbufRecycle(reply);
        return HDF_ERR_IO;
    }

    HdfIoServiceRecycle(service);
    HdfSbufRecycle(reply);
    return HDF_SUCCESS;
}

static struct PcieBusTester *PcieBusTesterGet(void)
{
    static struct PcieBusTester tester = {0};
    struct HdfConfigWlanBus busConfig = {0};
    int32_t ret = PcieBusTesterGetConfig(&tester.config);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: read config failed:%d", __func__, ret);
        return NULL;
    }
    HdfWlanConfigBusAbs(tester.config.busNum);
    busConfig.busIdx = tester.config.busNum;
    // busConfig.blockSize
    HdfWlanBusAbsInit(&tester.busDev, &busConfig);
    return &tester;
}

static void PcieBusTesterPut(struct PcieBusTester *tester)
{
    if (tester == NULL) {
        HDF_LOGE("%s:tester or service is null", __func__);
        return;
    }
    tester->busDev.ops.deInit(&tester->busDev);
}

int32_t PcieBusTestExecute(int32_t cmd)
{
    uint32_t i;
    int32_t ret = HDF_ERR_NOT_SUPPORT;
    struct PcieBusTester *tester = NULL;

    if (cmd > CMD_TEST_PCIE_MAX) {
        HDF_LOGE("%s: invalid cmd:%d", __func__, cmd);
        return ret;
    }

    tester = PcieBusTesterGet();
    if (tester == NULL) {
        HDF_LOGE("%s: get tester failed", __func__);
        return HDF_ERR_INVALID_OBJECT;
    }

    for (i = 0; i < sizeof(g_entry) / sizeof(g_entry[0]); i++) {
        if (g_entry[i].cmd == cmd && g_entry[i].func != NULL) {
            ret = g_entry[i].func(tester);
            break;
        }
    }
    PcieBusTesterPut(tester);
    HDF_LOGI("[%s][======cmd:%d====ret:%d======]", __func__, cmd, ret);
    return ret;
}