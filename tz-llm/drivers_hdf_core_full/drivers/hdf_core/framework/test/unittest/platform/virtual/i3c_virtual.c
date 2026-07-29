/*
 * Copyright (c) 2021-2023 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include "i3c/i3c_core.h"
#include "device_resource_if.h"
#include "osal_mem.h"
#include "osal_time.h"

#define HDF_LOG_TAG                        i3c_virtual

#define VIRTUAL_I3C_DELAY                   50
#define VIRTUAL_I3C_HOT_JOIN_TEST_ADDR      0x3f

#define VIRTUAL_I3C_TEST_STR_LEN            11
#define VIRTUAL_I3C_TEST_STR                "Hello I3C!"

struct VirtualI3cCntlr {
    struct I3cCntlr cntlr;
    OsalSpinlock spin;
    uint16_t busId;
    uint16_t busMode;
    uint16_t IrqNum;
    uint32_t i3cMaxRate;
    uint32_t i3cRate;
    uint32_t i2cFmRate;
    uint32_t i2cFmPlusRate;
};

struct VirtualI3cTransferData {
    struct I3cMsg *msgs;
    int16_t index;
    int16_t count;
};

struct VirtualI3cIbiSimulator {
    uint16_t ibiAddr;
    uint16_t deviceAddr;
};

static struct VirtualI3cIbiSimulator g_ibiSimulator;

static inline void VirtualI3cSetIbiSimulator(uint16_t addr, uint16_t deviceAddr)
{
    g_ibiSimulator.ibiAddr = addr;
    g_ibiSimulator.deviceAddr = deviceAddr;
}

static inline uint16_t VirtualI3cGetIbiAddr(void)
{
    return g_ibiSimulator.ibiAddr;
}

static inline uint16_t VirtualI3cGetIbiDeviceAddr(void)
{
    return g_ibiSimulator.deviceAddr;
}

static int32_t VirtualI3cXferI2cOneMsgPolling(const struct VirtualI3cCntlr *virtual,
                                              const struct VirtualI3cTransferData *td)
{
    struct I3cMsg *msg = &td->msgs[td->index];

    (void)virtual;
    (void)msg;
    HDF_LOGV("VirtualI3cXferI2cOneMsgPolling: flags:0x%x, len=%d", msg->flags, msg->len);

    return HDF_SUCCESS;
}

static inline void VirtualI3cCntlrInit(struct VirtualI3cCntlr *virtual)
{
    HDF_LOGI("VirtualI3cCntlrInit: cntlr:%u init done!", virtual->busId);
}

static int32_t VirtualSendCccCmd(struct I3cCntlr *cntlr, struct I3cCccCmd *ccc)
{
    (void)cntlr;
    HDF_LOGI("VirtualSendCccCmd: enter, destination: %x!", ccc->dest);

    return HDF_SUCCESS;
}

static int32_t VirtualI3cTransfer(struct I3cCntlr *cntlr, struct I3cMsg *msgs, int16_t count)
{
    if (cntlr == NULL || cntlr->priv == NULL) {
        HDF_LOGE("VirtualI3cTransfer: cntlr or virtual is null!");
        return HDF_ERR_INVALID_OBJECT;
    }

    if (msgs == NULL || count <= 0) {
        HDF_LOGE("VirtualI3cTransfer: err params! count:%d!", count);
        return HDF_ERR_INVALID_PARAM;
    }

    return count;
}

static int32_t VirtualI3cCntlrI2cTransfer(struct I3cCntlr *cntlr, struct I3cMsg *msgs, int16_t count)
{
    int32_t ret;
    int irqSave;
    struct VirtualI3cCntlr *virtual = NULL;
    struct VirtualI3cTransferData td;

    if (cntlr == NULL || cntlr->priv == NULL) {
        HDF_LOGE("VirtualI3cCntlrI2cTransfer: cntlr or virtual is null!");
        return HDF_ERR_INVALID_OBJECT;
    }
    virtual = (struct VirtualI3cCntlr *)cntlr;

    if (msgs == NULL || count <= 0) {
        HDF_LOGE("VirtualI3cCntlrI2cTransfer: err params! count:%d!", count);
        return HDF_ERR_INVALID_PARAM;
    }
    td.msgs = msgs;
    td.count = count;
    td.index = 0;

    irqSave = LOS_IntLock();
    while (td.index < td.count) {
        ret = VirtualI3cXferI2cOneMsgPolling(virtual, &td);
        if (ret != 0) {
            break;
        }
        td.index++;
    }
    LOS_IntRestore(irqSave);
    return (td.index > 0) ? td.index : ret;
}

static int32_t VirtualI3cSetConfig(struct I3cCntlr *cntlr, struct I3cConfig *config)
{
    (void)cntlr;
    (void)config;
    HDF_LOGI("VirtualI3cSetConfig: done!");
    return HDF_SUCCESS;
}

static int32_t VirtualI3cGetConfig(struct I3cCntlr *cntlr, struct I3cConfig *config)
{
    (void)cntlr;
    (void)config;
    HDF_LOGI("VirtualI3cGetConfig: done!");
    return HDF_SUCCESS;
}

static int32_t VirtualI3cRequestIbi(struct I3cDevice *dev)
{
    (void)dev;
    HDF_LOGI("VirtualI3cRequestIbi: done!");

    return HDF_SUCCESS;
}

static void VirtualI3cFreeIbi(struct I3cDevice *dev)
{
    (void)dev;
    HDF_LOGI("VirtualI3cFreeIbi: done!");
}

static const struct I3cMethod g_method = {
    .sendCccCmd = VirtualSendCccCmd,
    .i2cTransfer = VirtualI3cCntlrI2cTransfer,
    .Transfer = VirtualI3cTransfer,
    .setConfig = VirtualI3cSetConfig,
    .getConfig = VirtualI3cGetConfig,
    .requestIbi = VirtualI3cRequestIbi,
    .freeIbi = VirtualI3cFreeIbi,
};

static void VirtualI3cHotJoin(struct VirtualI3cCntlr *virtual)
{
    uint16_t deviceAddr;
    struct I3cDevice *device = NULL;
    int32_t ret;

    deviceAddr = VirtualI3cGetIbiDeviceAddr();
    device = (struct I3cDevice *)OsalMemCalloc(sizeof(*device));
    if (device == NULL) {
        HDF_LOGE("VirtualI3cHotJoin: device is null!");
        return;
    }
    device->cntlr = &virtual->cntlr;
    device->addr = deviceAddr;
    device->type = I3C_CNTLR_I3C_DEVICE;
    device->supportIbi = I3C_DEVICE_SUPPORT_IBI;
    ret = I3cDeviceAdd(device);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("VirtualI3cHotJoin: add i3c device fail!");
        return;
    }
    HDF_LOGI("VirtualI3cHotJoin: done!");
}

static int32_t VirtualI3cReservedAddrWorker(struct VirtualI3cCntlr *virtual, uint16_t addr)
{
    (void)virtual;
    switch (addr) {
        case I3C_HOT_JOIN_ADDR:
             VirtualI3cHotJoin(virtual);
            break;
        case I3C_RESERVED_ADDR_7H3E:
        case I3C_RESERVED_ADDR_7H5E:
        case I3C_RESERVED_ADDR_7H6E:
        case I3C_RESERVED_ADDR_7H76:
        case I3C_RESERVED_ADDR_7H7A:
        case I3C_RESERVED_ADDR_7H7C:
        case I3C_RESERVED_ADDR_7H7F:
            /* All cases of broadcast address single bit error detect */
            HDF_LOGW("VirtualI3cReservedAddrWorker: broadcast Address single bit error!");
            break;
        default:
            HDF_LOGD("VirtualI3cReservedAddrWorker: Reserved address which is not support!");
            break;
    }

    return HDF_SUCCESS;
}

static int32_t I3cIbiHandle(uint32_t irq, void *data)
{
    struct VirtualI3cCntlr *virtual = NULL;
    struct I3cDevice *device = NULL;
    uint16_t ibiAddr;
    char *testStr = VIRTUAL_I3C_TEST_STR;

    (void)irq;
    if (data == NULL) {
        HDF_LOGW("I3cIbiHandle: data is null!");
        return HDF_ERR_INVALID_PARAM;
    }
    virtual = (struct VirtualI3cCntlr *)data;
    ibiAddr = VirtualI3cGetIbiAddr();
    if (I3cCheckReservedAddr(ibiAddr) == I3C_ADDR_RESERVED) {
        HDF_LOGD("I3cIbiHandle: Calling VirtualI3cResAddrWorker...");
        return VirtualI3cReservedAddrWorker(virtual, ibiAddr);
    } else {
        HDF_LOGD("I3cIbiHandle: Calling I3cCntlrIbiCallback...");
        device = I3cGetDeviceByAddr(&virtual->cntlr, ibiAddr);
        if (device == NULL) {
            HDF_LOGE("I3cIbiHandle: device is null!");
            return HDF_ERR_MALLOC_FAIL;
        }
        if (device->ibi->payload > VIRTUAL_I3C_TEST_STR_LEN) {
            /* Put the string "Hello I3C!" into IBI buffer */
            *device->ibi->data = *testStr;
        }
        return I3cCntlrIbiCallback(device);
    }

    return HDF_SUCCESS;
}

/* Soft calling is used here to simulate in-band interrupt */
static inline void SoftInterruptTrigger(struct VirtualI3cCntlr *virtual, uint16_t ibiDeviceAddr)
{
    VirtualI3cSetIbiSimulator(I3C_HOT_JOIN_ADDR, ibiDeviceAddr);
    HDF_LOGE("SoftInterruptTrigger: IrqNum: %d!", virtual->IrqNum);

    /* Simulate soft interrupt through direct call */
    if (I3cIbiHandle(virtual->IrqNum, (void *)virtual) != HDF_SUCCESS) {
        HDF_LOGE("SoftInterruptTrigger: Call I3cIbiHandle fail!");
    }
}

static int32_t VirtualI3cHotJoinSimulator(void)
{
    uint16_t busId;
    struct I3cCntlr *cntlr = NULL;
    struct VirtualI3cCntlr *virtual = NULL;

    for (busId = 0; busId < I3C_CNTLR_MAX; busId++) {
        cntlr = I3cCntlrGet(busId);
        if (cntlr == NULL) {
            continue;
        }

        virtual = (struct VirtualI3cCntlr *)cntlr;
        OsalMDelay(VIRTUAL_I3C_DELAY);
        SoftInterruptTrigger(virtual, VIRTUAL_I3C_HOT_JOIN_TEST_ADDR);
    }

    return HDF_SUCCESS;
}

static int32_t VirtualI3cReadDrs(struct VirtualI3cCntlr *virtual, const struct DeviceResourceNode *node)
{
    struct DeviceResourceIface *drsOps = NULL;

    drsOps = DeviceResourceGetIfaceInstance(HDF_CONFIG_SOURCE);
    if (drsOps == NULL || drsOps->GetUint32 == NULL || drsOps->GetUint16 == NULL) {
        HDF_LOGE("VirtualI3cReadDrs: invalid drs ops fail!");
        return HDF_FAILURE;
    }
    if (drsOps->GetUint16(node, "busId", &virtual->busId, 0) != HDF_SUCCESS) {
        HDF_LOGE("VirtualI3cReadDrs: read busId fail!");
        return HDF_ERR_IO;
    }
    if (drsOps->GetUint16(node, "busMode", &virtual->busMode, 0) != HDF_SUCCESS) {
        HDF_LOGE("VirtualI3cReadDrs: read busMode fail!");
        return HDF_ERR_IO;
    }
    if (drsOps->GetUint16(node, "IrqNum", &virtual->IrqNum, 0) != HDF_SUCCESS) {
        HDF_LOGE("VirtualI3cReadDrs: read IrqNum fail!");
        return HDF_ERR_IO;
    }
    if (drsOps->GetUint32(node, "i3cMaxRate", &virtual->i3cMaxRate, 0) != HDF_SUCCESS) {
        HDF_LOGE("VirtualI3cReadDrs: read i3cMaxRate fail!");
        return HDF_ERR_IO;
    }
    if (drsOps->GetUint32(node, "i3cRate", &virtual->i3cRate, 0) != HDF_SUCCESS) {
        HDF_LOGE("VirtualI3cReadDrs: read i3cRate fail!");
        return HDF_ERR_IO;
    }
    if (drsOps->GetUint32(node, "i2cFmRate", &virtual->i2cFmRate, 0) != HDF_SUCCESS) {
        HDF_LOGE("VirtualI3cReadDrs: read i2cFmRate fail!");
        return HDF_ERR_IO;
    }
    if (drsOps->GetUint32(node, "i2cFmPlusRate", &virtual->i2cFmPlusRate, 0) != HDF_SUCCESS) {
        HDF_LOGE("VirtualI3cReadDrs: read i2cFmPlusRate fail!");
        return HDF_ERR_IO;
    }

    return HDF_SUCCESS;
}

static int32_t VirtualI3cParseAndInit(struct HdfDeviceObject *device, const struct DeviceResourceNode *node)
{
    int32_t ret;
    struct VirtualI3cCntlr *virtual = NULL;
    (void)device;

    virtual = (struct VirtualI3cCntlr *)OsalMemCalloc(sizeof(*virtual));
    if (virtual == NULL) {
        HDF_LOGE("VirtualI3cParseAndInit: malloc virtual fail!");
        return HDF_ERR_MALLOC_FAIL;
    }

    ret = VirtualI3cReadDrs(virtual, node);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("VirtualI3cParseAndInit: read drs fail, ret: %d!", ret);
        OsalMemFree(virtual);
        virtual = NULL;
        return ret;
    }

    VirtualI3cCntlrInit(virtual);
    virtual->cntlr.priv = (void *)node;
    virtual->cntlr.busId = (int16_t)virtual->busId;
    virtual->cntlr.ops = &g_method;
    (void)OsalSpinInit(&virtual->spin);
    ret = I3cCntlrAdd(&virtual->cntlr);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("VirtualI3cParseAndInit: add i3c controller fail, ret = %d!", ret);
        (void)OsalSpinDestroy(&virtual->spin);
        OsalMemFree(virtual);
        virtual = NULL;
        return ret;
    }

    return HDF_SUCCESS;
}

static int32_t VirtualI3cInit(struct HdfDeviceObject *device)
{
    int32_t ret;
    const struct DeviceResourceNode *childNode = NULL;

    HDF_LOGD("VirtualI3cInit: enter!");
    if (device == NULL || device->property == NULL) {
        HDF_LOGE("VirtualI3cInit: device or property is null!");
        return HDF_ERR_INVALID_OBJECT;
    }

    DEV_RES_NODE_FOR_EACH_CHILD_NODE(device->property, childNode) {
        ret = VirtualI3cParseAndInit(device, childNode);
        if (ret != HDF_SUCCESS) {
            break;
        }
    }
    return VirtualI3cHotJoinSimulator();
}

static void VirtualI3cRemoveByNode(const struct DeviceResourceNode *node)
{
    int32_t ret;
    int16_t busId;
    struct I3cCntlr *cntlr = NULL;
    struct VirtualI3cCntlr *virtual = NULL;
    struct DeviceResourceIface *drsOps = NULL;

    drsOps = DeviceResourceGetIfaceInstance(HDF_CONFIG_SOURCE);
    if (drsOps == NULL || drsOps->GetUint32 == NULL) {
        HDF_LOGE("VirtualI3cRemoveByNode: invalid drs ops fail!");
        return;
    }

    ret = drsOps->GetUint16(node, "busId", (uint16_t *)&busId, 0);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("VirtualI3cRemoveByNode: read busId fail, ret: %d!", ret);
        return;
    }

    cntlr = I3cCntlrGet(busId);
    if (cntlr != NULL && cntlr->priv == node) {
        I3cCntlrPut(cntlr);
        I3cCntlrRemove(cntlr);
        virtual = (struct VirtualI3cCntlr *)cntlr;
        (void)OsalSpinDestroy(&virtual->spin);
        OsalMemFree(virtual);
    }
    return;
}

static void VirtualI3cRelease(struct HdfDeviceObject *device)
{
    const struct DeviceResourceNode *childNode = NULL;

    HDF_LOGI("VirtualI3cRelease: enter!");

    if (device == NULL || device->property == NULL) {
        HDF_LOGE("VirtualI3cRelease: device or property is null!");
        return;
    }

    DEV_RES_NODE_FOR_EACH_CHILD_NODE(device->property, childNode) {
        VirtualI3cRemoveByNode(childNode);
    }
}

struct HdfDriverEntry g_i3cDriverEntry = {
    .moduleVersion = 1,
    .Init = VirtualI3cInit,
    .Release = VirtualI3cRelease,
    .moduleName = "virtual_i3c_driver",
};
HDF_INIT(g_i3cDriverEntry);
