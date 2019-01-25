#include "PortingInterface.h"
#include "PortingLog.h"
#include "nsCOMPtr.h"
#include "nsString.h"
#include "nsServiceManagerUtils.h"
#include "nsIIccService.h"
#include "nsIIccInfo.h"
#include "nsIMobileConnectionService.h"
#include "nsIMobileConnectionInfo.h"
#include "nsINetworkManager.h"
#include "nsINetworkInterface.h"
#include "nsIMobileSignalStrength.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsIWifi.h"


BEGIN_VOWIFI_NAMESPACE

const string PortingInterface::TAG = "PortingInterface";
bool PortingInterface::mIsAirplaneModeEnabled = false;

int PortingInterface::getDefaultDataPhoneId() {
    return PRIMARY_PHONE_ID;
}

bool PortingInterface::hasIccCard(int phoneId) {
    nsCOMPtr<nsIIccService> iccService = do_GetService(ICC_SERVICE_CONTRACTID);
    if (!iccService) {
        LOGE(_PTAG(TAG), "hasIccCard: Could not acquire nsIIccService !!!!!");
        return false;
    }

    nsCOMPtr<nsIIcc> icc;
    iccService->GetIccByServiceId(PortingInterface::getDefaultDataPhoneId(), getter_AddRefs(icc));
    if (!icc) {
        LOGE(_PTAG(TAG), "hasIccCard: Could not acquire nsIIcc !!!!!");
        return false;
    }

    uint32_t cardState = SIM_STATE_ABSENT;
    if (NS_OK == icc->GetCardState(&cardState)) {
        return (cardState != SIM_STATE_ABSENT ? true : false);
    } else {
        LOGE(_PTAG(TAG), "hasIccCard: Could not acquire cardState by GetCardState() !!!!!");
    }

    return false;
}

int PortingInterface::getIccCardState(int phoneId) {
    uint32_t cardState = SIM_STATE_ABSENT;

    nsCOMPtr<nsIIccService> iccService = do_GetService(ICC_SERVICE_CONTRACTID);
    if (!iccService) {
        LOGE(_PTAG(TAG), "getIccCardState: Could not acquire nsIIccService !!!!!");
        return cardState;
    }

    nsCOMPtr<nsIIcc> icc;
    iccService->GetIccByServiceId(PortingInterface::getDefaultDataPhoneId(), getter_AddRefs(icc));
    if (!icc) {
        LOGE(_PTAG(TAG), "getIccCardState: Could not acquire nsIIcc !!!!!");
        return cardState;
    }

    if (NS_OK == icc->GetCardState(&cardState)) {
        return cardState;
    } else {
        LOGE(_PTAG(TAG), "getIccCardState: Could not acquire cardState by GetCardState() !!!!!");
    }

    return cardState;
}

int PortingInterface::getIccCardType(int phoneId) {
    nsCOMPtr<nsIIccService> iccService = do_GetService(ICC_SERVICE_CONTRACTID);
    if (!iccService) {
        LOGE(_PTAG(TAG), "getIccCardType: Could not acquire nsIIccService !!!!!");
        return SIM_CARD_TYPE_UNKNOWN;
    }

    nsCOMPtr<nsIIcc> icc;
    iccService->GetIccByServiceId(PortingInterface::getDefaultDataPhoneId(), getter_AddRefs(icc));
    if (!icc) {
        LOGE(_PTAG(TAG), "getIccCardType: Could not acquire nsIIcc !!!!!");
        return SIM_CARD_TYPE_UNKNOWN;
    }

    nsCOMPtr<nsIIccInfo> iccInfo;
    icc->GetIccInfo(getter_AddRefs(iccInfo));
    if (!iccInfo) {
        LOGE(_PTAG(TAG), "getIccCardType: Could not acquire nsIIccInfo !!!!!");
        return SIM_CARD_TYPE_UNKNOWN;
    }

    nsString iccType;
    if (NS_OK == iccInfo->GetIccType(iccType)) {
        if (!iccType.IsVoid() && !iccType.IsEmpty()) {
            LOGD(_PTAG(TAG), "getIccCardType: IccType is \"%s\" card", NS_ConvertUTF16toUTF8(iccType).get());
            if (iccType.EqualsLiteral("sim")) {
                return SIM_CARD_TYPE_SIM;
            } else {
                return SIM_CARD_TYPE_NOT_SIM;
            }
        }
    } else {
        LOGE(_PTAG(TAG), "getIccCardType: Could not acquire iccType by GetIccType() !!!!!");
    }

    return SIM_CARD_TYPE_UNKNOWN;
}

string PortingInterface::getIsimImpi(int phoneId) {
    string impi;

    nsCOMPtr<nsIIccService> iccService = do_GetService(ICC_SERVICE_CONTRACTID);
    if (!iccService) {
        LOGE(_PTAG(TAG), "getIsimImpi: Could not acquire nsIIccService !!!!!");
        return impi;
    }

    nsCOMPtr<nsIIcc> icc;
    iccService->GetIccByServiceId(PortingInterface::getDefaultDataPhoneId(), getter_AddRefs(icc));
    if (!icc) {
        LOGE(_PTAG(TAG), "getIsimImpi: Could not acquire nsIIcc !!!!!");
        return impi;
    }

    nsCOMPtr<nsIIsimIccInfo> isimInfo;
    icc->GetIsimInfo(getter_AddRefs(isimInfo));
    if (!isimInfo) {
        LOGE(_PTAG(TAG), "getIsimImpi: Could not acquire nsIIsimIccInfo !!!!!");
        return impi;
    }

    nsString nsImpi;
    if (NS_OK == isimInfo->GetImpi(nsImpi)) {
        impi = NS_ConvertUTF16toUTF8(nsImpi).get();
    } else {
        LOGE(_PTAG(TAG), "getIsimImpi: Could not acquire impi by GetImpi() !!!!!");
    }

    return impi;
}

string PortingInterface::getSimOperatorNumeric(int phoneId) {
    string hplmn;

    nsCOMPtr<nsIIccService> iccService = do_GetService(ICC_SERVICE_CONTRACTID);
    if (!iccService) {
        LOGE(_PTAG(TAG), "getSimOperatorNumeric: Could not acquire nsIIccService !!!!!");
        return hplmn;
    }

    nsCOMPtr<nsIIcc> icc;
    iccService->GetIccByServiceId(PortingInterface::getDefaultDataPhoneId(), getter_AddRefs(icc));
    if (!icc) {
        LOGE(_PTAG(TAG), "getSimOperatorNumeric: Could not acquire nsIIcc !!!!!");
        return hplmn;
    }

    nsCOMPtr<nsIIccInfo> iccInfo;
    icc->GetIccInfo(getter_AddRefs(iccInfo));
    if (!iccInfo) {
        LOGE(_PTAG(TAG), "getSimOperatorNumeric: Could not acquire nsIIccInfo !!!!!");
        return hplmn;
    }

    nsString nsMcc, nsMnc;
    if (NS_OK == iccInfo->GetMcc(nsMcc) && NS_OK == iccInfo->GetMnc(nsMnc)) {
        hplmn = string(NS_ConvertUTF16toUTF8(nsMcc).get()) + string(NS_ConvertUTF16toUTF8(nsMnc).get());
    } else {
        LOGE(_PTAG(TAG), "getSimOperatorNumeric: Could not acquire mcc or mnc by nsIIccInfo interface !!!!!");
    }

    return hplmn;
}

int PortingInterface::getLteRsrp() {
    if (isLteMobileNetwork()) {
        nsCOMPtr<nsIMobileConnectionService> mobileService = do_GetService(NS_MOBILE_CONNECTION_SERVICE_CONTRACTID);
        if (!mobileService) {
            LOGE(_PTAG(TAG), "getLteRsrp: Could not acquire nsIMobileConnectionService !!!!!");
            return LTE_SIGNAL_STRENGTH_INVALID;
        }

        nsCOMPtr<nsIMobileConnection> mobileConnection;
        mobileService->GetItemByServiceId(PortingInterface::getDefaultDataPhoneId(), getter_AddRefs(mobileConnection));
        if (!mobileConnection) {
            LOGE(_PTAG(TAG), "getLteRsrp: Could not acquire nsIMobileConnection !!!!!");
            return LTE_SIGNAL_STRENGTH_INVALID;
        }

        nsCOMPtr<nsIMobileSignalStrength> singalStrength;
        mobileConnection->GetSignalStrength(getter_AddRefs(singalStrength));
        if (!singalStrength) {
            LOGE(_PTAG(TAG), "getLteRsrp: Could not acquire nsIMobileSignalStrength !!!!!");
            return LTE_SIGNAL_STRENGTH_INVALID;
        }

        int lteRSRP = LTE_SIGNAL_STRENGTH_INVALID;
        singalStrength->GetLteRsrp(&lteRSRP);
        if (LTE_SIGNAL_STRENGTH_INVALID == lteRSRP) lteRSRP = LTE_SIGNAL_STRENGTH_DEFAULT;
        if (PORTING_DEBUG) LOGD(_PTAG(TAG), "getLteRsrp: _____lteRSRP = %d", lteRSRP);

        return lteRSRP;
    } else {
        if (PORTING_DEBUG) LOGD(_PTAG(TAG), "getLteRsrp: _____isn't in LTE network, lteRSRP = %d", LTE_SIGNAL_STRENGTH_INVALID);
        return LTE_SIGNAL_STRENGTH_INVALID;
    }
}

bool PortingInterface::isLteMobileNetwork(int phoneId) {
    nsCOMPtr<nsIMobileConnectionService> mobileService = do_GetService(NS_MOBILE_CONNECTION_SERVICE_CONTRACTID);
    if (!mobileService) {
        LOGE(_PTAG(TAG), "isLteMobileNetwork: Could not acquire nsIMobileConnectionService !!!!!");
        return false;
    }

    nsCOMPtr<nsIMobileConnection> mobileConnection;
    mobileService->GetItemByServiceId(PortingInterface::getDefaultDataPhoneId(), getter_AddRefs(mobileConnection));
    if (!mobileConnection) {
        LOGE(_PTAG(TAG), "isLteMobileNetwork: Could not acquire nsIMobileConnection !!!!!");
        return false;
    }

    nsCOMPtr<nsIMobileConnectionInfo> dataInfo;
    mobileConnection->GetData(getter_AddRefs(dataInfo));
    if (!dataInfo) {
        LOGE(_PTAG(TAG), "isLteMobileNetwork: Could not acquire nsIMobileConnectionInfo !!!!!");
        return false;
    }

    nsString mobileNetworkType;
    if (NS_OK == dataInfo->GetType(mobileNetworkType)) {
        if (mobileNetworkType.EqualsLiteral("lte")) {
            return true;
        }
    } else {
        LOGE(_PTAG(TAG), "isLteMobileNetwork: Could not acquire network type by GetType() !!!!!");
    }

    return false;
}

string PortingInterface::getMobileNetworkType(int phoneId) {
    string mobileNetworkType;

    nsCOMPtr<nsIMobileConnectionService> mobileService = do_GetService(NS_MOBILE_CONNECTION_SERVICE_CONTRACTID);
    if (!mobileService) {
        LOGE(_PTAG(TAG), "getMobileNetworkType: Could not acquire nsIMobileConnectionService !!!!!");
        return mobileNetworkType;
    }

    nsCOMPtr<nsIMobileConnection> mobileConnection;
    mobileService->GetItemByServiceId(PortingInterface::getDefaultDataPhoneId(), getter_AddRefs(mobileConnection));
    if (!mobileConnection) {
        LOGE(_PTAG(TAG), "getMobileNetworkType: Could not acquire nsIMobileConnection !!!!!");
        return mobileNetworkType;
    }

    nsCOMPtr<nsIMobileConnectionInfo> dataInfo;
    mobileConnection->GetData(getter_AddRefs(dataInfo));
    if (!dataInfo) {
        LOGE(_PTAG(TAG), "getMobileNetworkType: Could not acquire nsIMobileConnectionInfo !!!!!");
        return mobileNetworkType;
    }

    nsString type;
    if (NS_OK == dataInfo->GetType(type)) {
        mobileNetworkType = NS_ConvertUTF16toUTF8(type).get();
    } else {
        LOGE(_PTAG(TAG), "getMobileNetworkType: Could not acquire network type by GetType() !!!!!");
    }

    return mobileNetworkType;
}

int PortingInterface::getWifiRssi() {
    nsCOMPtr<nsIInterfaceRequestor> interfaceReq = do_GetService("@mozilla.org/telephony/system-worker-manager;1");
    if (!interfaceReq) {
        LOGE(_PTAG(TAG), "getWifiRssi: Could not acquire nsIInterfaceRequestor !!!!!");
        return WIFI_SIGNAL_STRENGTH_INVALID;
    }

    nsCOMPtr<nsIWifi> wifi = do_GetInterface(interfaceReq);
    if (!wifi) {
        LOGE(_PTAG(TAG), "getWifiRssi: Could not acquire nsIWifi !!!!!");
        return WIFI_SIGNAL_STRENGTH_INVALID;
    }

    nsCOMPtr<nsIWifiInfo> wifiInfo;
    wifi->GetWifiNetworkInfo(getter_AddRefs(wifiInfo));
    if (!wifiInfo) {
        LOGE(_PTAG(TAG), "getWifiRssi: Could not acquire nsIWifiInfo !!!!!");
        return WIFI_SIGNAL_STRENGTH_INVALID;;
    }

    int wifiRssi = WIFI_SIGNAL_STRENGTH_INVALID;
    wifiInfo->GetRssi(&wifiRssi);
    if (PORTING_DEBUG) LOGD(_PTAG(TAG), "getWifiRssi: _____wifiRssi = %d", wifiRssi);

    return wifiRssi;
}

bool PortingInterface::isWifiConnected() {
    nsCOMPtr<nsINetworkManager> networkManager = do_GetService("@mozilla.org/network/manager;1");
    if (!networkManager) {
        LOGE(_PTAG(TAG), "isWifiConnected: Could not acquire nsINetworkManager !!!!!");
        return false;
    }

    nsCOMPtr<nsINetworkInfo> activeNetworkInfo;
    networkManager->GetActiveNetworkInfo(getter_AddRefs(activeNetworkInfo));
    if (!activeNetworkInfo) {
        LOGE(_PTAG(TAG), "isWifiConnected: Could not acquire nsINetworkInfo !!!!!");
        return false;
    }

    int networkType, networkState;
    if (NS_OK == activeNetworkInfo->GetType(&networkType) && NS_OK == activeNetworkInfo->GetState(&networkState)) {
        if (nsINetworkInfo::NETWORK_TYPE_WIFI == networkType &&  WIFI_STATE_CONNECTED == networkState) {
            return true;
        }
    } else {
        LOGE(_PTAG(TAG), "isWifiConnected: Could not acquire network type or state by GetType() or GetState !!!!!");
    }

    return false;
}

void PortingInterface::setAirplaneModeEnabled(bool isEnabled) {
    mIsAirplaneModeEnabled = isEnabled;
}

bool PortingInterface::isAirplaneModeEnabled() {
    return mIsAirplaneModeEnabled;
}

void PortingInterface::setImsServiceCallbackListener(nsIImsServiceCallback *listener) {
    nsCOMPtr<nsIImsService> imsService = do_GetService(IMSSERVICE_CONTRACTID);
    if (!imsService) {
        LOGE(_PTAG(TAG), "setImsServiceCallbackListener: Could not acquire nsIImsService !!!!!");
        return;
    }

    if (NS_OK != imsService->SetImsServiceListener(listener)) {
        LOGE(_PTAG(TAG), "setImsServiceCallbackListener: Could not set Ims service listener !!!!!");
    }
}

int PortingInterface::switchToVowifi() {
    nsCOMPtr<nsIImsService> imsService = do_GetService(IMSSERVICE_CONTRACTID);
    if (!imsService) {
        LOGE(_PTAG(TAG), "switchToVowifi: Could not acquire nsIImsService !!!!!");
        return -1;
    }

    int16_t ret;
    if (NS_OK == imsService->SwitchImsFeature(IMS_TYPE_VOWIFI, &ret)) {
        return static_cast<int>(ret);
    } else {
        LOGE(_PTAG(TAG), "switchToVowifi: Could not switch to Vowifi !!!!!");
    }

    return -1;
}

int PortingInterface::handoverToVowifi() {
    nsCOMPtr<nsIImsService> imsService = do_GetService(IMSSERVICE_CONTRACTID);
    if (!imsService) {
        LOGE(_PTAG(TAG), "handoverToVowifi: Could not acquire nsIImsService !!!!!");
        return -1;
    }

    int16_t ret;
    if (NS_OK == imsService->StartHandover(IMS_TYPE_VOWIFI, &ret)) {
        return static_cast<int>(ret);
    } else {
        LOGE(_PTAG(TAG), "handoverToVowifi: Could not handover to Vowifi !!!!!");
    }

    return -1;
}

int PortingInterface::handoverToVolte() {
    nsCOMPtr<nsIImsService> imsService = do_GetService(IMSSERVICE_CONTRACTID);
    if (!imsService) {
        LOGE(_PTAG(TAG), "handoverToVolte: Could not acquire nsIImsService !!!!!");
        return -1;
    }

    int16_t ret;
    if (NS_OK == imsService->StartHandover(IMS_TYPE_VOLTE, &ret)) {
        return static_cast<int>(ret);
    } else {
        LOGE(_PTAG(TAG), "handoverToVolte: Could not handover to Volte !!!!!");
    }

    return -1;
}

int PortingInterface::releaseVoWifiResource() {
    nsCOMPtr<nsIImsService> imsService = do_GetService(IMSSERVICE_CONTRACTID);
    if (!imsService) {
        LOGE(_PTAG(TAG), "releaseVoWifiResource: Could not acquire nsIImsService !!!!!");
        return -1;
    }

    int16_t ret;
    if (NS_OK == imsService->ReleaseVoWifiResource(&ret)) {
        return static_cast<int>(ret);
    } else {
        LOGE(_PTAG(TAG), "releaseVoWifiResource: Could not release Vowifi resource !!!!!");
    }

    return -1;
}

int PortingInterface::setVowifiUnavailable(int wifiConnectState, bool isOnlySendAT) {
    nsCOMPtr<nsIImsService> imsService = do_GetService(IMSSERVICE_CONTRACTID);
    if (!imsService) {
        LOGE(_PTAG(TAG), "setVowifiUnavailable: Could not acquire nsIImsService !!!!!");
        return -1;
    }

    int16_t ret;
    if (NS_OK == imsService->SetVoWifiUnavailable(wifiConnectState, isOnlySendAT, &ret)) {
        return static_cast<int>(ret);
    } else {
        LOGE(_PTAG(TAG), "setVowifiUnavailable: Could not set Vowifi unavailable !!!!!");
    }

    return -1;
}

int PortingInterface::cancelCurrentRequest() {
    nsCOMPtr<nsIImsService> imsService = do_GetService(IMSSERVICE_CONTRACTID);
    if (!imsService) {
        LOGE(_PTAG(TAG), "cancelCurrentRequest: Could not acquire nsIImsService !!!!!");
        return -1;
    }

    int16_t ret;
    if (NS_OK == imsService->CancelCurrentRequest(&ret)) {
        return static_cast<int>(ret);
    } else {
        LOGE(_PTAG(TAG), "cancelCurrentRequest: Could not cancel current request !!!!!");
    }

    return -1;
}

void PortingInterface::hungUpImsCall(int wifiConnectState) {
    nsCOMPtr<nsIImsService> imsService = do_GetService(IMSSERVICE_CONTRACTID);
    if (!imsService) {
        LOGE(_PTAG(TAG), "hungUpImsCall: Could not acquire nsIImsService !!!!!");
        return;
    }

    if (NS_OK != imsService->TerminateCalls(wifiConnectState)) {
        LOGE(_PTAG(TAG), "hungUpImsCall: Could not hung up Ims call !!!!!");
    }
}

void PortingInterface::showSwitchVowifiFailedNotification() {
    nsCOMPtr<nsIImsService> imsService = do_GetService(IMSSERVICE_CONTRACTID);
    if (!imsService) {
        LOGE(_PTAG(TAG), "showSwitchVowifiFailedNotification: Could not acquire nsIImsService !!!!!");
        return;
    }

    if (NS_OK != imsService->ShowVowifiNotification()) {
        LOGE(_PTAG(TAG), "showSwitchVowifiFailedNotification: Could not show switch Vowifi failed notification !!!!!");
    }
}

int PortingInterface::getCurrentImsBearer() {
    nsCOMPtr<nsIImsService> imsService = do_GetService(IMSSERVICE_CONTRACTID);
    if (!imsService) {
        LOGE(_PTAG(TAG), "getCurrentImsBearer: Could not acquire nsIImsService !!!!!");
        return IMS_TYPE_UNKNOWN;
    }

    int16_t ret;
    if (NS_OK == imsService->GetCurrentImsFeature(&ret)) {
        return static_cast<int>(ret);
    } else {
        LOGE(_PTAG(TAG), "getCurrentImsBearer: Could not get current Ims Feature !!!!!");
    }

    return IMS_TYPE_UNKNOWN;
}

int PortingInterface::getCurrentUsedImsStack() {
    nsCOMPtr<nsIImsService> imsService = do_GetService(IMSSERVICE_CONTRACTID);
    if (!imsService) {
        LOGE(_PTAG(TAG), "getCurrentUsedImsStack: Could not acquire nsIImsService !!!!!");
        return IMS_TYPE_UNKNOWN;
    }

    int16_t ret;
    if (NS_OK == imsService->GetCallType(&ret)) {
        return static_cast<int>(ret);
    } else {
        LOGE(_PTAG(TAG), "getCurrentUsedImsStack: Could not get current used Ims stack !!!!!");
    }

    return IMS_TYPE_UNKNOWN;
}

int PortingInterface::getCpRegisterState() {
    nsCOMPtr<nsIImsService> imsService = do_GetService(IMSSERVICE_CONTRACTID);
    if (!imsService) {
        LOGE(_PTAG(TAG), "getCpRegisterState: Could not acquire nsIImsService !!!!!");
        return VOLTE_REG_STATE_INACTIVE;
    }

    int16_t ret;
    if (NS_OK == imsService->GetVolteRegisterState(&ret)) {
        return static_cast<int>(ret);
    } else {
        LOGE(_PTAG(TAG), "getCpRegisterState: Could not get CP register state !!!!!");
    }

    return VOLTE_REG_STATE_INACTIVE;
}

int PortingInterface::getCurrentImsCallType() {
    nsCOMPtr<nsIImsService> imsService = do_GetService(IMSSERVICE_CONTRACTID);
    if (!imsService) {
        LOGE(_PTAG(TAG), "getCurrentImsCallType: Could not acquire nsIImsService !!!!!");
        return IMS_CALL_TYPE_UNKNOWN;
    }

    int16_t ret;
    if (NS_OK == imsService->GetCurrentImsVideoState(&ret)) {
        return static_cast<int>(ret);
    } else {
        LOGE(_PTAG(TAG), "getCurrentImsCallType: Could not get current Ims call type !!!!!");
    }

    return IMS_CALL_TYPE_UNKNOWN;
}

string PortingInterface::getCurrentPcscfAddress() {
    string pcscfAddress;

    nsCOMPtr<nsIImsService> imsService = do_GetService(IMSSERVICE_CONTRACTID);
    if (!imsService) {
        LOGE(_PTAG(TAG), "getCurrentPcscfAddress: Could not acquire nsIImsService !!!!!");
        return pcscfAddress;
    }

    nsString nsPcscfAddress;
    if (NS_OK == imsService->GetCurPcscfAddress(nsPcscfAddress)) {
        pcscfAddress = NS_ConvertUTF16toUTF8(nsPcscfAddress).get();
        LOGD(_PTAG(TAG), "getCurrentPcscfAddress: pcscfAddress = %s", pcscfAddress.c_str());
    } else {
        LOGE(_PTAG(TAG), "getCurrentPcscfAddress: Could not get current P-CSCF address !!!!!");
    }

    return pcscfAddress;
}

string PortingInterface::getCurrentLocalAddress() {
    string localAddress;

    nsCOMPtr<nsIImsService> imsService = do_GetService(IMSSERVICE_CONTRACTID);
    if (!imsService) {
        LOGE(_PTAG(TAG), "getCurrentLocalAddress: Could not acquire nsIImsService !!!!!");
        return localAddress;
    }

    nsString nsLocalAddress;
    if (NS_OK == imsService->GetCurLocalAddress(nsLocalAddress)) {
        localAddress = NS_ConvertUTF16toUTF8(nsLocalAddress).get();
        LOGD(_PTAG(TAG), "getCurrentLocalAddress: localAddress = %s", localAddress.c_str());
    } else {
        LOGE(_PTAG(TAG), "getCurrentLocalAddress: Could not get current local address !!!!!");
    }

    return localAddress;
}

already_AddRefed<nsITimer> PortingInterface::createTimer(nsTimerCallbackFunc callback,
                                                         void *arg,
                                                         unsigned long intervalTime,
                                                         unsigned long timerType) {
    nsCOMPtr<nsITimer> timer = do_CreateInstance(NS_TIMER_CONTRACTID);
    if (!timer) {
        LOGE(_PTAG(TAG), "createTimer: Could not acquire nsITimer !!!!!");
        return nullptr;
    }

    if (NS_FAILED(timer->InitWithFuncCallback(callback, arg, intervalTime, timerType))) {
        LOGE(_PTAG(TAG), "createTimer: Create Timer failed !!!!!");
        return nullptr;
    }

    return timer.forget();
}

END_VOWIFI_NAMESPACE
