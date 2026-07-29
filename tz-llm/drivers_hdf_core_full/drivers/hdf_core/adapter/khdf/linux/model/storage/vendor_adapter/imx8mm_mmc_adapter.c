/*
 * hi35xx_mmc_adapter.c
 *
 * hi35xx linux mmc driver implement.
 *
 * Copyright (c) 2022 Huawei Device Co., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 */

#include <linux/mmc/host.h>
#include "hdf_base.h"
#include "hdf_log.h"

#define HDF_LOG_TAG imx8mm_mmc_adapter_c

struct mmc_host *sdhci_esdhc_get_mmc_host(int slot);
void sdhci_esdhc_sdio_rescan(int slot);

struct mmc_host *GetMmcHost(int32_t slot)
{
    HDF_LOGD("imx8mm GetMmcHost entry");
    return sdhci_esdhc_get_mmc_host(slot);
}

void SdioRescan(int slot)
{
    HDF_LOGD("imx8mm SdioRescan entry");
    sdhci_esdhc_sdio_rescan(slot);
}
