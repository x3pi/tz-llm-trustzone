/*
 * Copyright (c) 2022 Beijing OSWare Technology Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include <drm/drm_mipi_dsi.h>
#include <video/mipi_display.h>
#include <linux/of.h>
#include "hdf_base.h"
#include "hdf_device_desc.h"
#include "hdf_log.h"
#include "mipi_dsi_core.h"
#include "mipi_dsi_if.h"
#include "osal_time.h"

#define HDF_LOG_TAG mipi_drm_adapter

static struct mipi_dsi_device *GetLinuxPanel(const struct MipiDsiCntlr *cntlr)
{
    if (cntlr == NULL) {
        HDF_LOGE("%s: dev is NULL!", __func__);
        return NULL;
    }
    if (cntlr->devNo >= MAX_CNTLR_CNT) {
        HDF_LOGE("%s: dev is NULL!", __func__);
        return NULL;
    }

    return (struct mipi_dsi_device *)cntlr->priv;
}

static int32_t MipiDsiAdapterSetConfig(struct MipiDsiCntlr *cntlr)
{
    (void)cntlr;
    return HDF_SUCCESS;
}

// "toshiba,panel-tc358775"
static int32_t MipiDsiAdapterAttach(struct MipiDsiCntlr *cntlr, uint8_t *name)
{
    int32_t ret = 0;
    struct device_node *panelNode = NULL;
    struct mipi_dsi_device *linuxPanel = NULL;

    if ((cntlr == NULL) || (name == NULL)) {
        HDF_LOGE("%s: cntlr or name is NULL!", __func__);
        return HDF_ERR_INVALID_PARAM;
    }
    if (cntlr->devNo >= MAX_CNTLR_CNT) {
        HDF_LOGE("%s: cntlr->devNo is erro!", __func__);
        return HDF_ERR_INVALID_PARAM;
    }

    panelNode = of_find_compatible_node(NULL, NULL, name);
    if (panelNode == NULL) {
        HDF_LOGE("%s: [of_find_compatible_node] failed!", __func__);
        return HDF_FAILURE;
    }
    linuxPanel = of_find_mipi_dsi_device_by_node(panelNode);
    if (linuxPanel == NULL) {
        HDF_LOGE("%s: [of_find_mipi_dsi_device_by_node] failed!", __func__);
        return HDF_FAILURE;
    }
    ret = mipi_dsi_attach(linuxPanel);
    if (ret < 0) {
        HDF_LOGE("%s: mipi_dsi_attach failed.", __func__);
        return HDF_FAILURE;
    }
    cntlr->priv = (DevHandle)linuxPanel;

    return HDF_SUCCESS;
}

extern void hdf_drm_mode_to_videomode(void);
static int32_t MipiDsiAdapterSetCmd(struct MipiDsiCntlr *cntlr, struct DsiCmdDesc *cmd)
{
    int32_t ret = 0;
    struct mipi_dsi_device *linuxPanel = NULL;

    if (cntlr == NULL || cmd == NULL) {
        HDF_LOGE("%s: cntlr or cmd is NULL.", __func__);
        return HDF_ERR_INVALID_OBJECT;
    }
    linuxPanel = GetLinuxPanel(cntlr);
    if (linuxPanel == NULL) {
        HDF_LOGE("%s: linuxPanel is NULL!", __func__);
        return HDF_ERR_INVALID_OBJECT;
    }

    if (cmd->payload == NULL) {
        HDF_LOGE("%s: payload is NULL!", __func__);
        return HDF_ERR_INVALID_OBJECT;
    }

    hdf_drm_mode_to_videomode();
    if ((cmd->dataType == MIPI_DSI_GENERIC_SHORT_WRITE_1_PARAM) ||    // 0x13,
        (cmd->dataType == MIPI_DSI_GENERIC_SHORT_WRITE_2_PARAM) ||    // 0x23
        (cmd->dataType == MIPI_DSI_GENERIC_LONG_WRITE)) {             // 0x29
        ret = mipi_dsi_generic_write(linuxPanel, cmd->payload, cmd->dataLen);
        if (ret < 0) {
            HDF_LOGE("%s: [mipi_dsi_generic_write] failed.", __func__);
            return HDF_FAILURE;
        }
    } else if ((cmd->dataType == MIPI_DSI_DCS_SHORT_WRITE) ||         // 0x05
        (cmd->dataType == MIPI_DSI_DCS_SHORT_WRITE_PARAM) ||          // 0x15
        (cmd->dataType == MIPI_DSI_DCS_LONG_WRITE)) {                 // 0x39
        ret = mipi_dsi_dcs_write_buffer(linuxPanel, cmd->payload, cmd->dataLen);
        HDF_LOGE("%s: [mipi_dsi_dcs_write_buffer] dataType = 0x%x, payload = 0x%x, dataLen = 0x%x",
            __func__, cmd->dataType, cmd->payload[0], cmd->dataLen);
        if (ret < 0) {
            HDF_LOGE("%s: [mipi_dsi_dcs_write_buffer] failed. ret =  %d", __func__, ret);
            return HDF_FAILURE;
        }
    } else {
        if (cmd->dataLen == 0) {
            cmd->payload = NULL;
        }
        ret = mipi_dsi_dcs_write(linuxPanel, cmd->dataType, cmd->payload, cmd->dataLen);
        HDF_LOGE("%s: [mipi_dsi_dcs_write] dataType = 0x%x, dataLen = 0x%x",
            __func__, cmd->dataType, cmd->dataLen);
        if (ret < 0) {
            HDF_LOGE("%s: [mipi_dsi_dcs_write] failed.", __func__);
            return HDF_FAILURE;
        }
    }

    if (cmd->delay > 0) {
        OsalMDelay(cmd->delay);
    }

    return HDF_SUCCESS;
}

static struct MipiDsiCntlr g_mipiTx = {
    .devNo = 0
};

static struct MipiDsiCntlrMethod g_method = {
    .setCntlrCfg = MipiDsiAdapterSetConfig,
    .setCmd = MipiDsiAdapterSetCmd,
    .getCmd = NULL,
    .toHs = NULL,
    .toLp = NULL,
    .enterUlps = NULL,
    .exitUlps = NULL,
    .powerControl = NULL,
    .attach = MipiDsiAdapterAttach,
};

static int32_t MipiDsiAdapterBind(struct HdfDeviceObject *device)
{
    int32_t ret = 0;

    if (device == NULL) {
        HDF_LOGE("%s: device is NULL!", __func__);
        return HDF_ERR_INVALID_OBJECT;
    }

    HDF_LOGI("%s: enter.", __func__);
    g_mipiTx.priv = NULL;
    g_mipiTx.ops = &g_method;
    ret = MipiDsiRegisterCntlr(&g_mipiTx, device);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: [MipiDsiRegisterCntlr] failed.", __func__);
        return ret;
    }
    HDF_LOGI("%s: success.", __func__);

    return HDF_SUCCESS;
}

static int32_t MipiDsiAdapterInit(struct HdfDeviceObject *device)
{
    (void)device;

    HDF_LOGI("%s: success.", __func__);
    return HDF_SUCCESS;
}

static void MipiDsiAdapterRelease(struct HdfDeviceObject *device)
{
    struct MipiDsiCntlr *cntlr = NULL;

    if (device == NULL) {
        HDF_LOGE("%s: device is NULL.", __func__);
        return;
    }
    cntlr = MipiDsiCntlrFromDevice(device);
    if (cntlr == NULL) {
        HDF_LOGE("%s: cntlr is NULL.", __func__);
        return;
    }
    MipiDsiUnregisterCntlr(cntlr);
    cntlr->priv = NULL;

    HDF_LOGI("%s: success.", __func__);
}

struct HdfDriverEntry g_mipiDsiLinuxDriverEntry = {
    .moduleVersion = 1,
    .Bind = MipiDsiAdapterBind,
    .Init = MipiDsiAdapterInit,
    .Release = MipiDsiAdapterRelease,
    .moduleName = "linux_mipi_drm_adapter",
};
HDF_INIT(g_mipiDsiLinuxDriverEntry);
