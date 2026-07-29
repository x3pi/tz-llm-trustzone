/*
 * Copyright (c) 2020-2022 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include "sta.h"
#include "securec.h"
#include "message/message_router.h"
#include "message/sidecar.h"
#include "wifi_base.h"
#include "hdf_wlan_services.h"
#include "hdf_wlan_utils.h"

#define HDF_LOG_TAG HDF_WIFI_CORE
#define ATTR_MIN_LEN 2

#ifdef __cplusplus
extern "C" {
#endif

inline static int32_t Connect(struct NetDevice *netDev, struct WlanConnectParams *param)
{
    struct HdfChipDriver *chipDriver = GetChipDriver(netDev);
    if (chipDriver == NULL) {
        HDF_LOGE("%s:bad net device found!", __func__);
        return HDF_FAILURE;
    }
    RETURN_IF_CHIPOPS_NOT_IMPLEMENT(chipDriver->staOps, Connect);
    return chipDriver->staOps->Connect(netDev, param);
}

inline static int32_t Disconnect(struct NetDevice *netDev, uint16_t reasonCode)
{
    struct HdfChipDriver *chipDriver = GetChipDriver(netDev);
    if (chipDriver == NULL) {
        HDF_LOGE("%s:bad net device found!", __func__);
        return HDF_FAILURE;
    }
    RETURN_IF_CHIPOPS_NOT_IMPLEMENT(chipDriver->staOps, Disconnect);
    return chipDriver->staOps->Disconnect(netDev, reasonCode);
}

inline static int32_t ScanAll(struct NetDevice *netDev, struct WlanScanRequest *params)
{
    struct HdfChipDriver *chipDriver = GetChipDriver(netDev);
    if (chipDriver == NULL) {
        HDF_LOGE("%s:bad net device found!", __func__);
        return HDF_FAILURE;
    }
    RETURN_IF_CHIPOPS_NOT_IMPLEMENT(chipDriver->staOps, StartScan);
    HDF_LOGI("%s: ScanAll finished!", __func__);
    return chipDriver->staOps->StartScan(netDev, params);
}

inline static int32_t AbortScan(struct NetDevice *netDev)
{
    struct HdfChipDriver *chipDriver = GetChipDriver(netDev);
    if (chipDriver == NULL) {
        HDF_LOGE("%s:bad net device found!", __func__);
        return HDF_FAILURE;
    }
    RETURN_IF_CHIPOPS_NOT_IMPLEMENT(chipDriver->staOps, AbortScan);
    return chipDriver->staOps->AbortScan(netDev);
}

static int WifiFillScanParam(struct WlanScanRequest *params, struct HdfSBuf *reqData)
{
    uint32_t dataSize = 0;
    if (!HdfSbufReadBuffer(reqData, (const void **)&params->bssid, &dataSize)) {
        HDF_LOGE("%s: %s!ParamName=%s", __func__, ERROR_DESC_READ_REQ_FAILED, "apSettings");
        return HDF_FAILURE;
    }
    if (!HdfSbufReadBuffer(reqData, (const void **)&params->ssids, &dataSize)) {
        HDF_LOGE("%s: %s!ParamName=%s", __func__, ERROR_DESC_READ_REQ_FAILED, "ssids");
        return HDF_FAILURE;
    }
    params->ssidCount = dataSize / sizeof(params->ssids[0]);
    if (!HdfSbufReadBuffer(reqData, (const void **)&params->extraIEs, &dataSize)) {
        HDF_LOGE("%s: %s!ParamName=%s", __func__, ERROR_DESC_READ_REQ_FAILED, "extraIes");
        return HDF_FAILURE;
    }
    params->extraIEsLen = dataSize;
    if (!HdfSbufReadBuffer(reqData, (const void **)&params->freqs, &dataSize)) {
        HDF_LOGE("%s: %s!ParamName=%s", __func__, ERROR_DESC_READ_REQ_FAILED, "freqs");
        return HDF_FAILURE;
    }
    params->freqsCount =
        (dataSize / sizeof(params->freqs[0])) < MAX_FREQ_FTILTER_COUNT ? (dataSize / sizeof(params->freqs[0])) : 0;

    return HDF_SUCCESS;
}

static uint8_t WifiValidIeAttr(const uint8_t *ie, uint32_t len)
{
    uint8_t elemLen;
    if (ie == NULL) { // ie null is normal
        return true;
    }
    while (len != 0) {
        if (len < ATTR_MIN_LEN) {
            return false;
        }
        len -= ATTR_MIN_LEN;
        elemLen = ie[1];
        if (elemLen > len) {
            return false;
        }
        len -= elemLen;
        ie += ATTR_MIN_LEN + elemLen;
    }
    return true;
}

static int WifiFillAssocParams(WifiAssociateParams *assoc, struct HdfSBuf *reqData)
{
    uint32_t dataSize = 0;
    if (!HdfSbufReadBuffer(reqData, (const void **)&assoc->bssid, &dataSize) || dataSize != ETH_ADDR_LEN) {
        HDF_LOGE("%s: %s!ParamName=%s", __func__, ERROR_DESC_READ_REQ_FAILED, "bssid");
        return HDF_FAILURE;
    }
    if (!HdfSbufReadBuffer(reqData, (const void **)&assoc->ssid, &(assoc->ssidLen))) {
        HDF_LOGE("%s: %s!ParamName=%s", __func__, ERROR_DESC_READ_REQ_FAILED, "ssid");
        return HDF_FAILURE;
    }
    if (!HdfSbufReadBuffer(reqData, (const void **)&assoc->ie, &(assoc->ieLen))) {
        HDF_LOGE("%s: %s!ParamName=%s", __func__, ERROR_DESC_READ_REQ_FAILED, "ie");
        return HDF_FAILURE;
    }
    if (!HdfSbufReadBuffer(reqData, (const void **)&assoc->key, &dataSize)) {
        HDF_LOGE("%s: %s!ParamName=%s", __func__, ERROR_DESC_READ_REQ_FAILED, "key");
        return HDF_FAILURE;
    }
    if (!HdfSbufReadUint8(reqData, &assoc->authType)) {
        HDF_LOGE("%s: %s!ParamName=%s", __func__, ERROR_DESC_READ_REQ_FAILED, "authType");
        return HDF_FAILURE;
    }
    if (!HdfSbufReadUint8(reqData, &assoc->privacy)) {
        HDF_LOGE("%s: %s!ParamName=%s", __func__, ERROR_DESC_READ_REQ_FAILED, "privacy");
        return HDF_FAILURE;
    }
    if (!HdfSbufReadUint8(reqData, &assoc->keyIdx)) {
        HDF_LOGE("%s: %s!ParamName=%s", __func__, ERROR_DESC_READ_REQ_FAILED, "keyIdx");
        return HDF_FAILURE;
    }
    if (!HdfSbufReadUint8(reqData, &assoc->mfp)) {
        HDF_LOGE("%s: %s!ParamName=%s", __func__, ERROR_DESC_READ_REQ_FAILED, "mfp");
        return HDF_FAILURE;
    }
    if (!HdfSbufReadUint32(reqData, &assoc->freq)) {
        HDF_LOGE("%s: %s!ParamName=%s", __func__, ERROR_DESC_READ_REQ_FAILED, "freq");
        return HDF_FAILURE;
    }
    if (!HdfSbufReadBuffer(reqData, (const void **)&assoc->crypto, &dataSize)) {
        HDF_LOGE("%s: %s!ParamName=%s", __func__, ERROR_DESC_READ_REQ_FAILED, "crypto");
        return HDF_FAILURE;
    }
    if ((assoc->ssid == NULL) || (assoc->ssidLen == 0)) {
        HDF_LOGE("%s:void ssid", __func__);
        return HDF_FAILURE;
    }
    if (WifiValidIeAttr(assoc->ie, assoc->ieLen) == false) {
        HDF_LOGE("%s:illegal ie", __func__);
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

static int WifiSetAssocParams(const WifiAssociateParams *assoc, const struct NetDevice *netdev,
    struct WlanConnectParams *params)
{
    (void)netdev;
    params->ssid = assoc->ssid;
    params->ssidLen = assoc->ssidLen;
    params->ie = assoc->ie;
    params->ieLen = assoc->ieLen;
    if ((assoc->authType > WIFI_AUTHTYPE_AUTOMATIC) || (assoc->authType == WIFI_AUTHTYPE_SAE)) {
        HDF_LOGE("%s:illegal authType %u", __func__, assoc->authType);
        return HDF_FAILURE;
    }
    params->authType = assoc->authType;
    params->bssid = assoc->bssid;
    params->privacy = assoc->privacy;
    if ((assoc->mfp != WIFI_MFP_REQUIRED) && (assoc->mfp != WIFI_MFP_NO) && (assoc->mfp != WIFI_MFP_OPTIONAL)) {
        HDF_LOGE("%s:unexpected mfp.mfp=%u", __func__, assoc->mfp);
        return HDF_FAILURE;
    }
    params->mfp = (WifiMfp)assoc->mfp;
    if (assoc->key != NULL) {
        params->key = assoc->key;
        params->keyLen = assoc->keyLen;
        params->keyIdx = assoc->keyIdx;
    }
    if (memcpy_s(&params->crypto, sizeof(WifiCryptoSetting), assoc->crypto, sizeof(WifiCryptoSetting)) != EOK) {
        HDF_LOGE("%s:copy crypto failed!", __func__);
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

static int32_t WifiCmdAssoc(const RequestContext *context, struct HdfSBuf *reqData, struct HdfSBuf *rspData)
{
    struct WlanConnectParams params = { 0 };
    WifiAssociateParams assoc = { 0 };
    struct NetDevice *netdev = NULL;
    struct WlanHwCapability *capability = NULL;
    const char *ifName = NULL;
    int32_t ret;
    (void)context;
    (void)rspData;
    if (reqData == NULL) {
        HDF_LOGE("%s: reqData is NULL", __func__);
        return HDF_ERR_INVALID_PARAM;
    }
    ifName = HdfSbufReadString(reqData);
    if (ifName == NULL) {
        HDF_LOGE("%s: %s!ParamName=%s", __func__, ERROR_DESC_READ_REQ_FAILED, "ifName");
        return HDF_FAILURE;
    }
    if (WifiFillAssocParams(&assoc, reqData) != HDF_SUCCESS) {
        return HDF_FAILURE;
    }
    if ((netdev = NetDeviceGetInstByName(ifName)) == NULL) {
        HDF_LOGE("%s:netdev not found!ifName=%s", __func__, ifName);
        return HDF_FAILURE;
    }
    if (WifiSetAssocParams(&assoc, netdev, &params) != HDF_SUCCESS) {
        HDF_LOGE("%s: WifiSetAssocParams failed", __func__);
        return HDF_FAILURE;
    }
    capability = GetHwCapability(netdev);
    if (capability == NULL) {
        HDF_LOGE("%s:GetHwCapability failed!", __func__);
        return HDF_FAILURE;
    }
    do {
        params.centerFreq = assoc.freq;
        HDF_LOGI("%s: %s connecting to AP ...", __func__, ifName);
        ret = Connect(netdev, &params);
        if (ret != HDF_SUCCESS) {
            HDF_LOGE("%s:fail to do connect,%d", __func__, ret);
            break;
        }
    } while (false);
    if (capability->Release != NULL) {
        capability->Release(capability);
        capability = NULL;
    }
    HDF_LOGI("%s: Wifi cmd Assoc finished!", __func__);
    return ret;
}

static int32_t WifiCmdDisconnect(const RequestContext *context, struct HdfSBuf *reqData, struct HdfSBuf *rspData)
{
    struct NetDevice *netdev = NULL;
    uint16_t reasonCode;
    const char *ifName = NULL;
    int ret;
    (void)context;
    (void)rspData;
    if (reqData == NULL) {
        HDF_LOGE("%s: reqData is NULL", __func__);
        return HDF_ERR_INVALID_PARAM;
    }
    ifName = HdfSbufReadString(reqData);
    if (ifName == NULL) {
        HDF_LOGE("%s: %s!ParamName=%s", __func__, ERROR_DESC_READ_REQ_FAILED, "ifName");
        return HDF_FAILURE;
    }
    if (!HdfSbufReadUint16(reqData, &reasonCode)) {
        HDF_LOGE("%s: %s!ParamName=%s", __func__, ERROR_DESC_READ_REQ_FAILED, "reasonCode");
        return HDF_FAILURE;
    }
    netdev = NetDeviceGetInstByName(ifName);
    if (netdev == NULL) {
        HDF_LOGE("%s:netdev not found!ifName=%s", __func__, ifName);
        return HDF_FAILURE;
    }
    HDF_LOGW("%s:%s disconnecting from AP...", __func__, ifName);
    ret = Disconnect(netdev, reasonCode);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s:fail to do disconnect,%d", __func__, ret);
    }
    HDF_LOGI("%s: WifiCmdDisconnect finished!", __func__);
    return ret;
}

static int32_t WifiCmdScan(const RequestContext *context, struct HdfSBuf *reqData, struct HdfSBuf *rspData)
{
    struct NetDevice *netdev = NULL;
    const char *ifName = NULL;
    struct WlanScanRequest params = { 0 };
    int32_t ret;
    (void)context;
    (void)rspData;
    if (reqData == NULL) {
        HDF_LOGE("%s: reqData is NULL", __func__);
        return HDF_ERR_INVALID_PARAM;
    }
    ifName = HdfSbufReadString(reqData);
    if (ifName == NULL) {
        HDF_LOGE("%s: %s!ParamName=%s", __func__, ERROR_DESC_READ_REQ_FAILED, "ifName");
        return HDF_FAILURE;
    }
    netdev = NetDeviceGetInstByName(ifName);
    if (netdev == NULL) {
        HDF_LOGE("%s:netdev not found!ifName=%s", __func__, ifName);
        return HDF_FAILURE;
    }
    ret = WifiFillScanParam(&params, reqData);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: WifiFillScanParam failed", __func__);
        return ret;
    }
    ret = ScanAll(netdev, &params);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: ScanAll failed! ret=%d", __func__, ret);
    }
    HDF_LOGI("%s: WifiCmdScan finished!", __func__);
    return ret;
}

static int32_t WifiCmdAbortScan(const RequestContext *context, struct HdfSBuf *reqData, struct HdfSBuf *rspData)
{
    struct NetDevice *netdev = NULL;
    int32_t ret;
    const char *ifName = HdfSbufReadString(reqData);
    (void)context;
    (void)rspData;
    if (ifName == NULL) {
        HDF_LOGE("%s: %s!ParamName=%s", __func__, ERROR_DESC_READ_REQ_FAILED, "ifName");
        return HDF_FAILURE;
    }
    netdev = NetDeviceGetInstByName(ifName);
    if (netdev == NULL) {
        HDF_LOGE("%s:netdev not found!ifName=%s", __func__, ifName);
        return HDF_FAILURE;
    }
    ret = AbortScan(netdev);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s:AbortScan failed!ret=%d", __func__, ret);
    }
    // keep return SUCCESS if AbortScan return FAILED
    return HDF_SUCCESS;
}

static int32_t WifiCmdSetScanningMacAddress(const RequestContext *context, struct HdfSBuf *reqData,
    struct HdfSBuf *rspData)
{
    int32_t ret;
    uint8_t isFuncValid;
    struct NetDevice *netdev = NULL;
    const char *ifName = NULL;
    unsigned char *mac = NULL;
    uint32_t replayDataSize;
    struct HdfChipDriver *chipDriver = NULL;
    (void)context;
    if (reqData == NULL || rspData == NULL) {
        return HDF_ERR_INVALID_PARAM;
    }
    ifName = HdfSbufReadString(reqData);
    if (ifName == NULL) {
        HDF_LOGE("%s: %s!ParamName=%s", __func__, ERROR_DESC_READ_REQ_FAILED, "ifName");
        return HDF_FAILURE;
    }
    netdev = NetDeviceGetInstByName(ifName);
    if (netdev == NULL) {
        HDF_LOGE("%s:netdev not found!ifName=%s", __func__, ifName);
        return HDF_FAILURE;
    }
    if (!HdfSbufReadBuffer(reqData, (const void **)&mac, &replayDataSize) || mac == NULL ||
        replayDataSize != IEEE80211_MAC_ADDR_LEN) {
        HDF_LOGE("%s: %s!ParamName=%s", __func__, ERROR_DESC_READ_REQ_FAILED, "mac");
        return HDF_FAILURE;
    }
    chipDriver = GetChipDriver(netdev);
    if (chipDriver == NULL) {
        HDF_LOGE("%s:bad net device found!", __func__);
        return HDF_FAILURE;
    }
    RETURN_IF_CHIPOPS_NOT_IMPLEMENT(chipDriver->staOps, SetScanningMacAddress);
    ret = chipDriver->staOps->SetScanningMacAddress(netdev, mac, IEEE80211_MAC_ADDR_LEN);
    if (ret != HDF_SUCCESS && ret != HDF_ERR_NOT_SUPPORT) {
        HDF_LOGE("%s: fail to do set scanning mac addr!ret=%d", __func__, ret);
        return ret;
    }
    isFuncValid = (ret == HDF_ERR_NOT_SUPPORT) ? false : true;
    if (!HdfSbufWriteUint8(rspData, isFuncValid)) {
        HDF_LOGE("%s: %s!", __func__, ERROR_DESC_WRITE_RSP_FAILED);
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

static int32_t HdfCmdfillPnoSettings(struct HdfSBuf *reqData,  WifiPnoSettings *pnoSettings)
{
    uint32_t dataSize = 0;

    if (!HdfSbufReadInt32(reqData, &(pnoSettings->min2gRssi))) {
        HDF_LOGE("%s: %s!ParamName=%s", __func__, ERROR_DESC_READ_REQ_FAILED, "min2gRssi");
        return HDF_FAILURE;
    }
    if (!HdfSbufReadInt32(reqData, &(pnoSettings->min5gRssi))) {
        HDF_LOGE("%s: %s!ParamName=%s", __func__, ERROR_DESC_READ_REQ_FAILED, "min5gRssi");
        return HDF_FAILURE;
    }
    if (!HdfSbufReadInt32(reqData, &(pnoSettings->scanIntervalMs))) {
        HDF_LOGE("%s: %s!ParamName=%s", __func__, ERROR_DESC_READ_REQ_FAILED, "scanIntervalMs");
        return HDF_FAILURE;
    }
    if (!HdfSbufReadInt32(reqData, &(pnoSettings->scanIterations))) {
        HDF_LOGE("%s: %s!ParamName=%s", __func__, ERROR_DESC_READ_REQ_FAILED, "scanIterations");
        return HDF_FAILURE;
    }
    if (!HdfSbufReadUint32(reqData, &(pnoSettings->pnoNetworksLen))) {
        HDF_LOGE("%s: %s!ParamName=%s", __func__, ERROR_DESC_READ_REQ_FAILED, "pnoNetworksLen");
        return HDF_FAILURE;
    }
	
    pnoSettings->pnoNetworks = (WifiPnoNetwork *)OsalMemAlloc(sizeof(WifiPnoNetwork) * pnoSettings->pnoNetworksLen);
    if (pnoSettings->pnoNetworks == NULL) {
        HDF_LOGE("%s: alloc memory failed.", __func__);
        return HDF_FAILURE;
    }
    for (uint32_t i = 0; i < pnoSettings->pnoNetworksLen; i++) {
        if (!HdfSbufReadUint8(reqData, &(pnoSettings->pnoNetworks[i].isHidden))) {
            HDF_LOGE("%s: %s!ParamName=%s", __func__, ERROR_DESC_READ_REQ_FAILED, "isHidden");
            return HDF_FAILURE;
        }
        if (!HdfSbufReadBuffer(reqData, (const void **)&pnoSettings->pnoNetworks[i].freqs, &dataSize)) {
            HDF_LOGE("%s: %s!ParamName=%s", __func__, ERROR_DESC_READ_REQ_FAILED, "freqs");
            return HDF_FAILURE;
        }
        pnoSettings->pnoNetworks[i].freqsLen = dataSize / sizeof(pnoSettings->pnoNetworks[i].freqs[0]);
        if (!HdfSbufReadBuffer(reqData, (const void **)&pnoSettings->pnoNetworks[i].ssid.ssid,
            &(pnoSettings->pnoNetworks[i].ssid.ssidLen))) {
            HDF_LOGE("%s: %s!ParamName=%s", __func__, ERROR_DESC_READ_REQ_FAILED, "ssid");
            return HDF_FAILURE;
        }
    }

    return HDF_SUCCESS;
}

static int32_t HdfWlanStartPnoScan(const char *ifName, const WifiPnoSettings *pnoSettings)
{
    struct NetDevice *netdev = NULL;
    struct HdfChipDriver *chipDriver = NULL;
    
    netdev = NetDeviceGetInstByName(ifName);
    if (netdev == NULL) {
        HDF_LOGE("%s:netdev not found!ifName=%s.", __func__, ifName);
        return HDF_FAILURE;
    }
    chipDriver = GetChipDriver(netdev);
    if (chipDriver == NULL) {
        HDF_LOGE("%s:bad net device found!", __func__);
        return HDF_FAILURE;
    }
    if (chipDriver->staOps == NULL) {
        HDF_LOGE("%s: chipDriver->staOps is null", __func__);
        return HDF_ERR_INVALID_OBJECT;
    }
    if (chipDriver->staOps->StartPnoScan == NULL) {
        HDF_LOGE("%s: chipDriver->staOps->StartPnoScan is null", __func__);
        return HDF_ERR_NOT_SUPPORT;
    }
    return chipDriver->staOps->StartPnoScan(netdev, pnoSettings);
}

static void HdfPnosettingsFree(WifiPnoSettings *pnoSettings)
{
    if (pnoSettings == NULL) {
        HDF_LOGE("%{public}s input parameter is NULL!", __func__);
        return;
    }
    if (pnoSettings->pnoNetworks != NULL) {
        OsalMemFree(pnoSettings->pnoNetworks);
        pnoSettings->pnoNetworks = NULL;
    }
    OsalMemFree(pnoSettings);
}

static int32_t WifiCmdStartPnoScan(const RequestContext *context, struct HdfSBuf *reqData, struct HdfSBuf *rspData)
{
    int32_t ret = HDF_FAILURE;
    const char *ifName = NULL;
    (void)rspData;
    (void)context;

    if (reqData == NULL || rspData == NULL) {
        return HDF_ERR_INVALID_PARAM;
    }
    ifName = HdfSbufReadString(reqData);
    if (ifName == NULL) {
        HDF_LOGE("%s: read ifName failed!", __func__);
        return ret;
    }
    WifiPnoSettings *pnoSettings = (WifiPnoSettings *)OsalMemCalloc(sizeof(WifiPnoSettings));
    if (pnoSettings == NULL) {
        HDF_LOGE("%s: calloc memory failed!", __func__);
        return ret;
    }
    ret = HdfCmdfillPnoSettings(reqData, pnoSettings);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: HdfCmdfillPnoSettings failed!", __func__);
        HdfPnosettingsFree(pnoSettings);
        return ret;
    }
    ret = HdfWlanStartPnoScan(ifName, pnoSettings);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: fail to start pno scan, %d", __func__, ret);
    }
    HdfPnosettingsFree(pnoSettings);
    return ret;
}

static int32_t HdfWlanStopPnoScan(const char *ifName)
{
    struct NetDevice *netdev = NULL;
    struct HdfChipDriver *chipDriver = NULL;
    
    netdev = NetDeviceGetInstByName(ifName);
    if (netdev == NULL) {
        HDF_LOGE("%s:netdev not found!ifName=%s.", __func__, ifName);
        return HDF_FAILURE;
    }
    chipDriver = GetChipDriver(netdev);
    if (chipDriver == NULL) {
        HDF_LOGE("%s:bad net device found!", __func__);
        return HDF_FAILURE;
    }
    if (chipDriver->staOps == NULL) {
        HDF_LOGE("%s: chipDriver->staOps is null", __func__);
        return HDF_ERR_INVALID_OBJECT;
    }

    if (chipDriver->staOps->StopPnoScan == NULL) {
        HDF_LOGE("%s: chipDriver->staOps->StopPnoScan is null", __func__);
        return HDF_ERR_NOT_SUPPORT;
    }
    return chipDriver->staOps->StopPnoScan(netdev);
}

static int32_t WifiCmdStopPnoScan(const RequestContext *context, struct HdfSBuf *reqData, struct HdfSBuf *rspData)
{
    int32_t ret = HDF_FAILURE;
    const char *ifName = NULL;
    (void)rspData;
    (void)context;

    if (reqData == NULL || rspData == NULL) {
        return HDF_ERR_INVALID_PARAM;
    }
    ifName = HdfSbufReadString(reqData);
    if (ifName == NULL) {
        HDF_LOGE("%s: read ifName failed!", __func__);
        return ret;
    }
    ret = HdfWlanStopPnoScan(ifName);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: fail to stop pno scan, %d", __func__, ret);
    }
    return ret;
}

static int32_t HdfWlanGetSignalPollInfo(const char *ifName, struct SignalResult *signalInfo)
{
    struct NetDevice *netdev = NULL;
    struct HdfChipDriver *chipDriver = NULL;
    
    netdev = NetDeviceGetInstByName(ifName);
    if (netdev == NULL) {
        HDF_LOGE("%s:netdev not found! ifName=%s.", __func__, ifName);
        return HDF_FAILURE;
    }
    chipDriver = GetChipDriver(netdev);
    if (chipDriver == NULL) {
        HDF_LOGE("%s:bad net device found!", __func__);
        return HDF_FAILURE;
    }
    if (chipDriver->staOps == NULL) {
        HDF_LOGE("%s: chipDriver->staOps is null", __func__);
        return HDF_ERR_INVALID_OBJECT;
    }
    if (chipDriver->staOps->GetSignalPollInfo == NULL) {
        HDF_LOGE("%s: chipDriver->staOps->GetSignalPollInfo is null", __func__);
        return HDF_ERR_NOT_SUPPORT;
    }
    return chipDriver->staOps->GetSignalPollInfo(netdev, signalInfo);
}

static int32_t WifiCmdGetSignalPollInfo(const RequestContext *context, struct HdfSBuf *reqData, struct HdfSBuf *rspData)
{
    (void)context;
    int32_t ret = HDF_FAILURE;
    const char *ifName = NULL;
    struct SignalResult signalInfo = {0};

    if (reqData == NULL || rspData == NULL) {
        return HDF_ERR_INVALID_PARAM;
    }
    ifName = HdfSbufReadString(reqData);
    if (ifName == NULL) {
        HDF_LOGE("%s: read ifName failed!", __func__);
        return ret;
    }
    ret = HdfWlanGetSignalPollInfo(ifName, &signalInfo);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: fail to get signal information, %d", __func__, ret);
        return ret;
    }
    if (!HdfSbufWriteBuffer(rspData, &signalInfo, sizeof(struct SignalResult))) {
        HDF_LOGE("%s: %s!", __func__, ERROR_DESC_WRITE_RSP_FAILED);
        ret = HDF_FAILURE;
    }
    return ret;
}

static struct MessageDef g_wifiStaFeatureCmds[] = {
    DUEMessage(CMD_STA_CONNECT, WifiCmdAssoc, 0),
    DUEMessage(CMD_STA_DISCONNECT, WifiCmdDisconnect, 0),
    DUEMessage(CMD_STA_SCAN, WifiCmdScan, 0),
    DUEMessage(CMD_STA_ABORT_SCAN, WifiCmdAbortScan, 0),
    DUEMessage(CMD_STA_SET_SCAN_MAC_ADDR, WifiCmdSetScanningMacAddress, 0),
    DUEMessage(CMD_STA_START_PNO_SCAN, WifiCmdStartPnoScan, 0),
    DUEMessage(CMD_STA_STOP_PNO_SCAN, WifiCmdStopPnoScan, 0),
    DUEMessage(CMD_STA_GET_SIGNAL_INFO, WifiCmdGetSignalPollInfo, 0)
};
ServiceDefine(STAService, STA_SERVICE_ID, g_wifiStaFeatureCmds);

Service *g_staService = NULL;

static int32_t StaInit(struct WifiFeature *feature)
{
    (void)feature;
    if (g_staService == NULL) {
        ServiceCfg cfg = {
            .dispatcherId = DEFAULT_DISPATCHER_ID
        };
        g_staService = CreateService(STAService, &cfg);
        if (g_staService == NULL) {
            HDF_LOGE("%s: The g_staService is null, CreateService failed!", __func__);
            return HDF_FAILURE;
        }
    }
    return HDF_SUCCESS;
}

static int32_t StaDeinit(struct WifiFeature *feature)
{
    (void)feature;
    if (g_staService != NULL && g_staService->Destroy != NULL) {
        g_staService->Destroy(g_staService);
        g_staService = NULL;
    }
    return HDF_SUCCESS;
}

struct WifiFeature g_staFeature = {
    .name = "sta",
    .init = StaInit,
    .deInit = StaDeinit
};

struct WifiFeature *GetWifiStaFeature(void)
{
    return &g_staFeature;
}

#ifdef __cplusplus
}
#endif
