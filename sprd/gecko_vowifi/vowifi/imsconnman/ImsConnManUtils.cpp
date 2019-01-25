#include "ImsConnManUtils.h"
#include "Constants.h"
#include "portinglayer/PortingLog.h"
#include "portinglayer/PortingInterface.h"
#include "portinglayer/PortingConstants.h"
#include <cutils/properties.h>


BEGIN_VOWIFI_NAMESPACE

const string ImsConnManUtils::TAG = "ImsConnManUtils";
int ImsConnManUtils::mRequestId = -1;

static const char* PROPERTY_VOWIFI_LAB_SIM_ENABLE   = "persist.sys.vowifi.lab.sim";
static const char* PROPERTY_VOWIFI_WHITELIST_ENABLE = "persist.sys.vowifi.whitelist";

void ImsConnManUtils::setRequestId(int requestId) {
    mRequestId = requestId;
}

int ImsConnManUtils::getRequestId() {
    return mRequestId;
}

int ImsConnManUtils::switchToVowifi() {
    LOGD(_TAG(TAG), "[Switch to Vowifi]");
    mRequestId = PortingInterface::switchToVowifi();
    return mRequestId;
}

int ImsConnManUtils::handoverToVowifi() {
    LOGD(_TAG(TAG), "[Handover to Vowifi]");
    mRequestId = PortingInterface::handoverToVowifi();
    return mRequestId;
}

int ImsConnManUtils::handoverToVolte() {
    LOGD(_TAG(TAG), "[Handover to Volte]");
    mRequestId = PortingInterface::handoverToVolte();
    return mRequestId;
}

int ImsConnManUtils::releaseVoWifiResource() {
    LOGD(_TAG(TAG), "[Release Vowifi resource]");
    mRequestId = PortingInterface::releaseVoWifiResource();
    return mRequestId;
}

int ImsConnManUtils::setVowifiUnavailable(bool isWifiConnected, bool isOnlySendAT) {
    LOGD(_TAG(TAG), "[Set Vowifi unavailable], isWifiConnected = %s, isOnlySendAT = %s", \
            isWifiConnected ? "true" : "false", \
            isOnlySendAT ? "true" : "false");

    mRequestId = PortingInterface::setVowifiUnavailable(isWifiConnected ? 1 : 0, isOnlySendAT);
    return mRequestId;
}

int ImsConnManUtils::cancelCurrentRequest() {
    LOGD(_TAG(TAG), "[Cancel current request]");
    mRequestId = PortingInterface::cancelCurrentRequest();
    return mRequestId;
}

void ImsConnManUtils::hungUpImsCall(bool isWifiConnected) {
    LOGD(_TAG(TAG), "[Hung up IMS call]");
    PortingInterface::hungUpImsCall(isWifiConnected ? 1 : 0);
    usleep(500 * 1000);
}

void ImsConnManUtils::showSwitchVowifiFailedNotification() {
    LOGD(_TAG(TAG), "[Show switch Vowifi failed notification]");
    PortingInterface::showSwitchVowifiFailedNotification();
}

int ImsConnManUtils::getCurrentImsBearer() {
    return PortingInterface::getCurrentImsBearer();
}

int ImsConnManUtils::getCurrentUsedImsStack() {
    int stackId = PortingInterface::getCurrentUsedImsStack();
    LOGD(_TAG(TAG), "[Current IMS stack]: %s during Ims call", getImsStackNameById(stackId));
    return stackId;
}

int ImsConnManUtils::getCpRegisterState() {
    return PortingInterface::getCpRegisterState();
}

int ImsConnManUtils::getCurrentImsCallType() {
    return PortingInterface::getCurrentImsCallType();
}

string ImsConnManUtils::getCurrentPcscfAddress() {
    return PortingInterface::getCurrentPcscfAddress();
}

string ImsConnManUtils::getCurrentLocalAddress() {
    return PortingInterface::getCurrentLocalAddress();
}

bool ImsConnManUtils::isVowifiLabSimEnable() {
    bool ret = property_get_bool(PROPERTY_VOWIFI_LAB_SIM_ENABLE, false);
    LOGD(_TAG(TAG), "isVowifiLabSimEnable = %s", ret ? "true" : "false");

    return ret;
}

bool ImsConnManUtils::isVowifiWhitelistEnable() {
    bool ret = property_get_bool(PROPERTY_VOWIFI_WHITELIST_ENABLE, false);
    LOGD(_TAG(TAG), "isVowifiWhitelistEnable = %s", ret ? "true" : "false");

    return ret;
}

char* ImsConnManUtils::getWfcModeNameById(int wfcMode) {
    const char* name = "\"Unknown\"";

    switch (wfcMode) {
        case WFC_MODE_WIFI_ONLY:
            name = "\"Wi-Fi only\"";
            break;

        case WFC_MODE_WIFI_PREFERRED:
            name = "\"Wi-Fi preferred\"";
            break;

        case WFC_MODE_CELLULAR_ONLY:
            name = "\"Cellular only\"";
            break;

        case WFC_MODE_CELLULAR_PREFERRED:
            name = "\"Cellular preferred\"";
            break;

        default:
            LOGE(_TAG(TAG), "getWfcModeNameById: invalid wfcMode id: %d, please check !!!!!", wfcMode);
            break;
    }

    return const_cast<char*>(name);
}

char* ImsConnManUtils::getMessageNameById(int messageId) {
    const char* name = "\"Unknown\"";

    switch (messageId) {
        case MSG_DEFAULT_ID:
            name = "\"NULL MESSAGE\"";
            break;

        case MSG_SWITCH_TO_VOWIFI:
            name = "\"MSG_SWITCH_TO_VOWIFI\"";
            break;

        case MSG_HANDOVER_TO_VOWIFI:
            name = "\"MSG_HANDOVER_TO_VOWIFI\"";
            break;

        case MSG_HANDOVER_TO_VOLTE:
            name = "\"MSG_HANDOVER_TO_VOLTE\"";
            break;

        case MSG_HANDOVER_TO_VOLTE_BY_TIMER:
            name = "\"MSG_HANDOVER_TO_VOLTE_BY_TIMER\"";
            break;

        case MSG_RELEASE_VOWIFI_RES:
            name = "\"MSG_RELEASE_VOWIFI_RES\"";
            break;

        case MSG_VOWIFI_UNAVAILABLE:
            name = "\"MSG_VOWIFI_UNAVAILABLE\"";
            break;

        case MSG_CANCEL_CURRENT_REQUEST:
            name = "\"MSG_CANCEL_CURRENT_REQUEST\"";
            break;

        default:
            LOGE(_TAG(TAG), "getMessageNameById: invalid message id: %d, please check !!!!!", messageId);
            break;
    }

    return const_cast<char*>(name);
}

char* ImsConnManUtils::getOperationTypeNameById(int type) {
    const char* name = "\"Unknown\"";

    switch (type) {
        case OPERATION_SWITCH_TO_VOWIFI:
            name = "\"OPERATION_SWITCH_TO_VOWIFI\"";
            break;

        case OPERATION_HANDOVER_TO_VOWIFI:
            name = "\"OPERATION_HANDOVER_TO_VOWIFI\"";
            break;

        case OPERATION_HANDOVER_TO_VOLTE:
            name = "\"OPERATION_HANDOVER_TO_VOLTE\"";
            break;

        case OPERATION_SET_VOWIFI_UNAVAILABLE:
            name = "\"OPERATION_SET_VOWIFI_UNAVAILABLE\"";
            break;

        case OPERATION_CANCEL_CURRENT_REQUEST:
            name = "\"OPERATION_CANCEL_CURRENT_REQUEST\"";
            break;

        case OPERATION_CP_REJECT_SWITCH_TO_VOWIFI:
            name = "\"OPERATION_CP_REJECT_SWITCH_TO_VOWIFI\"";
            break;

        case OPERATION_CP_REJECT_HANDOVER_TO_VOWIFI:
            name = "\"OPERATION_CP_REJECT_HANDOVER_TO_VOWIFI\"";
            break;

        case OPERATION_RELEASE_VOWIFI_RESOURCE:
            name = "\"OPERATION_RELEASE_VOWIFI_RESOURCE\"";
            break;

        default:
            LOGE(_TAG(TAG), "getOperationTypeNameById: invlid type id: %d, please check !!!!!", type);
            break;
    }

    return const_cast<char*>(name);
}

char* ImsConnManUtils::getCpRegisterStateNameById(int regState) {
    const char* name = "\"Unknown\"";

    switch (regState) {
        case VOLTE_REG_STATE_INACTIVE:
            name = "\"VOLTE INACTIVE\"";
            break;

        case VOLTE_REG_STATE_REGISTERED:
            name = "\"VOLTE REGISTERED\"";
            break;

        case VOLTE_REG_STATE_REGISTERING:
            name = "\"VOLTE REGISTERING\"";
            break;

        case VOLTE_REG_STATE_FAILED:
            name = "\"VOLTE FAILED\"";
            break;

        case VOLTE_REG_STATE_UNKNOWN:
            name = "\"VOLTE UNKNOWN\"";
            break;

        case VOLTE_REG_STATE_ROAMING:
            name = "\"VOLTE ROAMING\"";
            break;

        case VOLTE_REG_STATE_DEREGISTERING:
            name = "\"VOLTE DEREGISTERING:\"";
            break;

        default:
            LOGE(_TAG(TAG), "getCpRegisterStateNameById: invalid volte register state id: %d, please check !!!!!", regState);
            break;
    }

    return const_cast<char*>(name);
}

char* ImsConnManUtils::getImsBearerNameById(int imsBearer) {
    const char* name = "\"Unknown\"";

    switch (imsBearer) {
        case IMS_TYPE_UNKNOWN:
            if (PortingInterface::isLteMobileNetwork()) {
                name = "\"Lte network\"";
            } else {
                name = "\"Circuit Switch\"";
            }
            break;

        case IMS_TYPE_VOLTE:
            name = "\"VoLte\"";
            break;

        case IMS_TYPE_VOWIFI:
            name = "\"VoWifi\"";
            break;

        default:
            LOGE(_TAG(TAG), "getImsBearerNameById: invalid Ims bearer id: %d, please check !!!!!", imsBearer);
            break;
    }

    return const_cast<char*>(name);
}

char* ImsConnManUtils::getImsStackNameById(int imsStack) {
    const char* name = "\"Didn't use any Ims stack\"";

    switch (imsStack) {
        case IMS_TYPE_UNKNOWN:
            break;

        case IMS_TYPE_VOLTE:
            name = "\"VoLte IMS stack\"";
            break;

        case IMS_TYPE_VOWIFI:
            name = "\"VoWifi IMS stack\"";
            break;

        default:
            LOGE(_TAG(TAG), "getImsStackNameById: invalid Ims stack id: %d, please check !!!!!", imsStack);
            break;
    }

    return const_cast<char*>(name);
}

char* ImsConnManUtils::getCallStateNameById(int callState) {
    const char* name = "\"Unknown\"";

    switch (callState) {
        case CALL_STATE_IDLE:
            name = "\"IDLE\"";
            break;

        case CALL_STATE_DIALING:
            name = "\"DIALING\"";
            break;

        case CALL_STATE_ALERTING:
            name = "\"ALERTING\"";
            break;

        case CALL_STATE_CONNECTED:
            name = "\"ACTIVE\"";
            break;

        case CALL_STATE_HELD:
            name = "\"HOLD\"";
            break;

        case CALL_STATE_INCOMING:
            name = "\"INCOMING\"";
            break;

        default:
            LOGE(_TAG(TAG), "getCallStateNameById: invalid call state id: %d, please check !!!!!", callState);
            break;
    }

    return const_cast<char*>(name);
}

char* ImsConnManUtils::getCallModeNameById(int callMode) {
    const char* name = "\"Unknown\"";

    switch (callMode) {
        case CALL_MODE_NO_CALL:
            name = "\"No Call\"";
            break;

        case CALL_MODE_CS_CALL:
            name = "\"CS Call\"";
            break;

        case CALL_MODE_VOLTE_CALL:
            name = "\"Volte Call\"";
            break;

        case CALL_MODE_VOWIFI_CALL:
            name = "\"Vowifi Call\"";
            break;

        default:
            LOGE(_TAG(TAG), "getCallModeNameById: invalid call mode id: %d, please check !!!!!", callMode);
            break;
    }

    return const_cast<char*>(name);
}

char* ImsConnManUtils::getImsCallTypeNameById(int imsCallType) {
    const char* name = "\"Unknown\"";

    switch (imsCallType) {
        case IMS_CALL_TYPE_UNKNOWN:
            name = "\"UNKNOWN IMS CALL TYPE\"";
            break;

        case IMS_CALL_TYPE_AUDIO:
            name = "\"AUDIO CALL\"";
            break;

        case IMS_CALL_TYPE_VIDEO:
            name = "\"VIDEO CALL\"";
            break;

        default:
            LOGE(_TAG(TAG), "getImsCallTypeNameById: invalid Ims call type id: %d, please check !!!!!", imsCallType);
            break;
    }

    return const_cast<char*>(name);
}

END_VOWIFI_NAMESPACE
