#include "ImsConnManController.h"
#include "ImsConnManStateInfo.h"


BEGIN_VOWIFI_NAMESPACE

const string ImsConnManController::TAG = "ImsConnManController";
shared_ptr<MessageHandler> ImsConnManController::mpHandler;

void ImsConnManController::switchOrHandoverVowifi(bool isInitiateVowifiAfterLte) {
    if (!ImsConnManStateInfo::isWifiCallingEnabled()
            || !ImsConnManStateInfo::isWifiConnected()
            || ImsConnManStateInfo::isStartActivateVolte()) {
        LOGE(_TAG(TAG), "switchOrHandoverVowifi: Conditions are not satisfied, don't initiate Vowifi !!!!!");
        return;
    }

    if (isInitiateVowifiAfterLte && !ImsConnManStateInfo::isLteCellularNetwork()) {
        LOGE(_TAG(TAG), "switchOrHandoverVowifi: Device isn't in LTE environment, don't initiate Vowifi !!!!!");
        return;
    }

    if (CALL_MODE_CS_CALL == ImsConnManStateInfo::getCallMode()) {
        LOGE(_TAG(TAG), "switchOrHandoverVowifi: Don't handover Vowifi during \"CS Call\" !!!!!");
        return;
    }

    if (ImsConnManStateInfo::isIdleState() && CALL_MODE_NO_CALL == ImsConnManStateInfo::getCallMode()) {
        if (MSG_SWITCH_TO_VOWIFI == ImsConnManStateInfo::getPendingMessageId()) {
            LOGE(_TAG(TAG), "switchOrHandoverVowifi: Don't switch Vowifi again !!!!!");
            return;
        }

        LOGD(_TAG(TAG), "switchOrHandoverVowifi: callState = \"IDLE\", callMode = \"No Call\", [Switch Vowifi]");

        if (nullptr != mpHandler) mpHandler->sendEmptyMessage(MSG_SWITCH_TO_VOWIFI);
    } else if (!ImsConnManStateInfo::isIdleState() && CALL_MODE_VOLTE_CALL == ImsConnManStateInfo::getCallMode()) {
        if (MSG_HANDOVER_TO_VOWIFI == ImsConnManStateInfo::getPendingMessageId()) {
            LOGE(_TAG(TAG), "switchOrHandoverVowifi: Don't handover Vowifi again !!!!!");
            return;
        }

        LOGD(_TAG(TAG), "switchOrHandoverVowifi: callState = \"not IDLE\", callMode = \"Volte Call\", [handover Vowifi]");

        if (nullptr != mpHandler) mpHandler->sendEmptyMessage(MSG_HANDOVER_TO_VOWIFI);
    } else {
        LOGE(_TAG(TAG), "switchOrHandoverVowifi: callState = %s, callMode = %s, don't initiate Vowifi !!!!!", \
                ImsConnManUtils::getCallStateNameById(ImsConnManStateInfo::getCallState()), \
                ImsConnManUtils::getCallModeNameById(ImsConnManStateInfo::getCallMode()));
    }
}

void ImsConnManController::forcedlyHandoverToVolte() {
    LOGD(_TAG(TAG), "forcedlyHandoverToVolte: forcedly [handover Volte]");
    if (nullptr != mpHandler) mpHandler->sendMessage(mpHandler->obtainMessage(MSG_HANDOVER_TO_VOLTE, 1));
}

void ImsConnManController::handoverToVolte(bool isByTimer) {
    if ((ImsConnManStateInfo::isIdleState() && CALL_MODE_NO_CALL == ImsConnManStateInfo::getCallMode())
            || (!ImsConnManStateInfo::isIdleState() && CALL_MODE_CS_CALL == ImsConnManStateInfo::getCallMode())) {
        LOGE(_TAG(TAG), "handoverToVolte: Don't handover during \"No Call\" or \"CS Call\"");
        return;
    }

    if (MSG_HANDOVER_TO_VOLTE == ImsConnManStateInfo::getPendingMessageId()
            || MSG_HANDOVER_TO_VOLTE_BY_TIMER == ImsConnManStateInfo::getPendingMessageId()) {
        LOGE(_TAG(TAG), "handoverToVolte: Don't handover Volte again !!!!!");
        return;
    }

    if (!ImsConnManStateInfo::isIdleState() && CALL_MODE_VOWIFI_CALL == ImsConnManStateInfo::getCallMode()) {
        LOGD(_TAG(TAG), "handoverToVolte: callState = \"not IDLE\", callMode = \"Vowifi Call\", [handover Volte] %s", \
                (isByTimer ? "by Timer" : ""));

        if (isByTimer) {
            if (nullptr != mpHandler) mpHandler->sendEmptyMessage(MSG_HANDOVER_TO_VOLTE_BY_TIMER);
        } else {
            if (nullptr != mpHandler) mpHandler->sendMessage(mpHandler->obtainMessage(MSG_HANDOVER_TO_VOLTE, 0));
        }
    } else {
        LOGE(_TAG(TAG), "handoverToVolte: callState = %s, callMode = %s, don't handover Volte !!!!!", \
                ImsConnManUtils::getCallStateNameById(ImsConnManStateInfo::getCallState()), \
                ImsConnManUtils::getCallModeNameById(ImsConnManStateInfo::getCallMode()));

        if (ImsConnManStateInfo::isIdleState() && CALL_MODE_NO_CALL == ImsConnManStateInfo::getCallMode()) {
            LOGE(_TAG(TAG), "handoverToVolte: change to invoke vowifiUnavailable");
        }
    }
}

void ImsConnManController::releaseVoWifiResource() {
    if (MSG_RELEASE_VOWIFI_RES == ImsConnManStateInfo::getPendingMessageId()) {
        LOGE(_TAG(TAG), "releaseVoWifiResource: Don't release Vowifi resource again !!!!!");
        return;
    }

    LOGD(_TAG(TAG), "releaseVoWifiResource: Release vowifi resource only, but don't notify CP");

    if (nullptr != mpHandler) mpHandler->sendEmptyMessage(MSG_RELEASE_VOWIFI_RES);
}

void ImsConnManController::vowifiUnavailable(bool isWifiConnected) {
    if (MSG_VOWIFI_UNAVAILABLE == ImsConnManStateInfo::getPendingMessageId()) {
        LOGE(_TAG(TAG), "vowifiUnavailable: Don't set Vowifi unavailable again !!!!!");
        return;
    }

    LOGD(_TAG(TAG), "vowifiUnavailable: Release vowifi resource at first, then notify CP self-management");

    if (nullptr != mpHandler) mpHandler->sendMessage(mpHandler->obtainMessage(MSG_VOWIFI_UNAVAILABLE, isWifiConnected));
}

void ImsConnManController::cancelCurrentRequest() {
    if (MSG_CANCEL_CURRENT_REQUEST == ImsConnManStateInfo::getPendingMessageId()) {
        LOGE(_TAG(TAG), "cancelCurrentRequest: Don't cancel current request again !!!!!");
        return;
    }

    LOGD(_TAG(TAG), "cancelCurrentRequest: Cancel current request, and waiting for cancel response");

    if (nullptr != mpHandler) mpHandler->sendEmptyMessage(MSG_CANCEL_CURRENT_REQUEST);
}

void ImsConnManController::showSwitchVowifiFailedNotification() {
    ImsConnManStateInfo::setWifiCallingEnabled(false);
    ImsConnManUtils::showSwitchVowifiFailedNotification();
}

void ImsConnManController::hungUpImsCall(bool isWifiConnected) {
    ImsConnManUtils::hungUpImsCall(isWifiConnected);
}

END_VOWIFI_NAMESPACE
