#include "ImsConnManService.h"
#include "ImsConnManStateInfo.h"
#include "ImsConnManController.h"
#include "ImsConnManUtils.h"


BEGIN_VOWIFI_NAMESPACE

const string ImsConnManService::TAG = "ImsConnManService";
static const string MTC_S2B_ERROR_CODE_BASE = "5376";

enum {
    MTC_S2B_ERR_AUTH_FAIL           = 0xD200+4,
    MTC_S2B_ERR_EAP_FAIL            = 0xD200+5,
    MTC_S2B_ERR_IP_FAIL             = 0xD200+6,
    MTC_S2B_ERR_AUTH_RESEND_FAIL    = 0xD200+7,
    MTC_S2B_ERR_RESEND_TIME_OUT     = 0xD200+9,
};

ImsConnManService::ImsConnManService() : mMobileNetworkType("unknown") {
    mRequestId = -1;
    mPendingMsgId = MSG_DEFAULT_ID;
    mIsMsgQueuePending = false;
    mMessageArg = 0;
    mIsProcessingLoopMsg = false;
    mIsNeedSync = true;

    mSwitchVowifiFailedCount = 0;
    mSwitchVowifiIpFailedCount = 0;

    mIsDpdHandoverToVolte = false;
    mIsEpdgStopHandoverToVolte = false;
    mIsNoRtpHandoverToVolte = false;
    mIsQuicklySwitchVowifi = false;

    mIsVoltePdnDeactive = true;
    mIsWaitVolteRegAfterImsCallEnd = false;
}

ImsConnManService::~ImsConnManService() {
}

void ImsConnManService::onAttach(int policyId) {
    LOGD(_TAG(TAG), "onAttach: policyId = %d", policyId);

    if (OPERATOR_NAME_ID_INVALID == policyId) {
        LOGE(_TAG(TAG), "onAttach: policyId is invalid !!!!!");
        return;
    }

    switch (policyId) {
        case OPERATOR_NAME_ID_RELIANCE:
            mpPolicy.reset(new ImsConnManReliancePolicy());
            break;

        case OPERATOR_NAME_ID_DEFAULT:
            mpPolicy.reset(new ImsConnManDefaultPolicy());
            break;

        default:
            LOGE(_TAG(TAG), "onAttach: policyId = %d is error id !!!!!", policyId);
            break;
    }

    mpCommonMsgResultTask = new CommonMessageResultTask(this);
    mpHandler.reset(new CommonMessageHandler);
    mpHandler->update(this);
    ImsConnManController::mpHandler = mpHandler;
    mpRtpHandler.reset(new RtpMessageHandler);
    mpRtpHandler->update(this);
    mpPingAddrHandler.reset(new PingAddressHandler);

    ImsConnManStateInfo::setServiceAttached(true);

    LOGD(_TAG(TAG), "onAttach: Wifi-calling is \"%s\"", \
            ImsConnManStateInfo::isWifiCallingEnabled() ? "enabled" : "disabled");

    LOGD(_TAG(TAG), "onAttach: Wifi-calling mode is %s", \
            ImsConnManUtils::getWfcModeNameById(ImsConnManStateInfo::getWifiCallingMode()));

    mMobileNetworkType = PortingInterface::getMobileNetworkType();
    if (mMobileNetworkType.empty()) mMobileNetworkType = "unknown";
    LOGD(_TAG(TAG), "onAttach: mMobileNetworkType = \"%s\"", mMobileNetworkType.c_str());

    int imsBearer = ImsConnManUtils::getCurrentImsBearer();
    LOGD(_TAG(TAG), "onAttach: imsBearer = %s", ImsConnManUtils::getImsBearerNameById(imsBearer));
    if (IMS_TYPE_VOLTE == imsBearer) {
        ImsConnManStateInfo::setVolteRegistered(true);
        ImsConnManStateInfo::setVowifiRegistered(false);
    } else if (IMS_TYPE_VOWIFI == imsBearer) {
        ImsConnManStateInfo::setVolteRegistered(false);
        ImsConnManStateInfo::setVowifiRegistered(true);
    } else if (IMS_TYPE_UNKNOWN == imsBearer) {
        ImsConnManStateInfo::setVolteRegistered(false);
        ImsConnManStateInfo::setVowifiRegistered(false);
    }

    bool isWifiConnected = PortingInterface::isWifiConnected();
    ImsConnManStateInfo::setWifiConnected(isWifiConnected);
    LOGD(_TAG(TAG), "onAttach: Wifi network has been %s", (isWifiConnected ? "Connected" : "Disconnected"));

    if (isMeetInitiateVowifiConditions()) {
        mpPolicy->createPolicyTimerTask(false, true);
    }
}

void ImsConnManService::onDetach() {
    LOGD(_TAG(TAG), "onDetach");

    ImsConnManStateInfo::setServiceAttached(false);

    mpHandler->update(nullptr);
    mpHandler.reset();
    ImsConnManController::mpHandler = mpHandler;
    mpRtpHandler->update(nullptr);
    mpRtpHandler.reset();
    mpPingAddrHandler.reset();
}

void ImsConnManService::operationSuccessed(int id, int type) {
    LOGD(_TAG(TAG), "operationSuccessed: id = %d, type = %s", \
            id, ImsConnManUtils::getOperationTypeNameById(type));

    if (id == ImsConnManUtils::getRequestId()) {
        ImsConnManUtils::setRequestId(-1);
        mIsMsgQueuePending = false;
        ImsConnManStateInfo::setMessageQueuePendingState(mIsMsgQueuePending);

        if (OPERATION_HANDOVER_TO_VOLTE != type) {
            mPendingMsgId = MSG_DEFAULT_ID;
            ImsConnManStateInfo::setPendingMessageId(mPendingMsgId);
        }

        LOGD(_TAG(TAG), "operationSuccessed: mIsMsgQueuePending = \"false\", mPendingMsgId = \"NULL MESSAGE\"");

        switch (type) {
            case OPERATION_SWITCH_TO_VOWIFI:
                mSwitchVowifiFailedCount = 0;
                mSwitchVowifiIpFailedCount = 0;
                mpPingAddrHandler->sendMessage(mpPingAddrHandler->obtainMessage(MSG_PING_PCSCF_ADDRESS), NS_DISPATCH_NORMAL);
                break;

            case OPERATION_HANDOVER_TO_VOWIFI:
                /*if (!ImsConnManStateInfo::isWifiCallingEnabled() || !ImsConnManStateInfo::isWifiConnected()) {
                    ImsConnManController::handoverToVolte(false);
                }*/
                if (ImsConnManStateInfo::isIdleState() && CALL_MODE_NO_CALL == ImsConnManStateInfo::getCallMode()) {
                    LOGE(_TAG(TAG), "operationSuccessed: change to invoke vowifiUnavailable");
                    ImsConnManController::vowifiUnavailable(ImsConnManStateInfo::isWifiConnected());
                }
                break;

            case OPERATION_HANDOVER_TO_VOLTE:
                LOGD(_TAG(TAG), "operationSuccessed: mIsDpdHandoverToVolte = %s, mIsEpdgStopHandoverToVolte = %s, mIsNoRtpHandoverToVolte = %s", \
                        mIsDpdHandoverToVolte ? "true" : "false", \
                        mIsEpdgStopHandoverToVolte ? "true" : "false", \
                        mIsNoRtpHandoverToVolte ? "true" : "false");

                if (MSG_HANDOVER_TO_VOLTE_BY_TIMER == mPendingMsgId) {
                    mPendingMsgId = MSG_DEFAULT_ID;
                    ImsConnManStateInfo::setPendingMessageId(mPendingMsgId);
                    break;
                }

                mPendingMsgId = MSG_DEFAULT_ID;
                ImsConnManStateInfo::setPendingMessageId(mPendingMsgId);

                if (mIsDpdHandoverToVolte || mIsEpdgStopHandoverToVolte || mIsNoRtpHandoverToVolte) {
                    mIsDpdHandoverToVolte = false;
                    mIsEpdgStopHandoverToVolte = false;
                    mIsNoRtpHandoverToVolte = false;
                    break;
                }

                if (ImsConnManStateInfo::isIdleState() && CALL_MODE_NO_CALL == ImsConnManStateInfo::getCallMode()) {
                    LOGD(_TAG(TAG), "operationSuccessed: change to invoke vowifiUnavailable");
                    ImsConnManController::vowifiUnavailable(ImsConnManStateInfo::isWifiConnected());
                }
                break;

            case OPERATION_SET_VOWIFI_UNAVAILABLE:
                break;

            case OPERATION_CANCEL_CURRENT_REQUEST:
                break;

            case OPERATION_RELEASE_VOWIFI_RESOURCE:
                if (mIsQuicklySwitchVowifi) {
                    mpPolicy->createPolicyTimerTask(false, true);
                    mIsQuicklySwitchVowifi = false;
                } else {
                    mpPolicy->createPolicyTimerTask(false, false);
                }
                break;

            default:
                LOGD(_TAG(TAG), "operationSuccessed: unknown type = %d, please check !!!!!", type);
                return;
        }

        if (mIsProcessingLoopMsg) {
            mpHandler->sendMessage(mpHandler->obtainMessage(MSG_LOOP_PROCESS, mMessageArg));
        }
    } else {
        LOGE(_TAG(TAG), "operationSuccessed: \"id\" and \"RequestId\" are different, RequestId = %d, please check !!!!!", \
                ImsConnManUtils::getRequestId());
    }
}

void ImsConnManService::operationFailed(int id, string reason, int type) {
    LOGE(_TAG(TAG), "operationFailed: id = %d, failed reason = %s, type = %s", \
            id, reason.c_str(), ImsConnManUtils::getOperationTypeNameById(type));

    if (id == ImsConnManUtils::getRequestId()) {
        ImsConnManUtils::setRequestId(-1);
        mIsMsgQueuePending = false;
        ImsConnManStateInfo::setMessageQueuePendingState(mIsMsgQueuePending);

        if (OPERATION_HANDOVER_TO_VOLTE != type) {
            mPendingMsgId = MSG_DEFAULT_ID;
            ImsConnManStateInfo::setPendingMessageId(mPendingMsgId);
        }

        LOGE(_TAG(TAG), "operationFailed: mIsMsgQueuePending = \"false\", mPendingMsgId = \"NULL MESSAGE\"");

        switch (type) {
            case OPERATION_SWITCH_TO_VOWIFI:
                reason = "53769";
                if (!reason.empty() && string::npos != reason.find_first_of(MTC_S2B_ERROR_CODE_BASE)) {
                    switch (atoi(reason.c_str())) {
                        case MTC_S2B_ERR_AUTH_FAIL:
                        case MTC_S2B_ERR_EAP_FAIL:
                        case MTC_S2B_ERR_AUTH_RESEND_FAIL:
                        case MTC_S2B_ERR_RESEND_TIME_OUT:
                            if (mpPolicy->isPopupErrorNotification()) {
                                LOGE(_TAG(TAG), "operationFailed: mSwitchVowifiFailedCount = %d", ++mSwitchVowifiFailedCount);
                                if (mSwitchVowifiFailedCount >= mpPolicy->getPopupErrorRetryCount()) {
                                    mSwitchVowifiFailedCount = 0;
                                    ImsConnManController::showSwitchVowifiFailedNotification();
                                    return;
                                }
                            }
                            break;

                        case MTC_S2B_ERR_IP_FAIL:
                            LOGE(_TAG(TAG), "operationFailed: S2B_ERR_IP_FAIL, mSwitchVowifiIpFailedCount = %d", ++mSwitchVowifiIpFailedCount);
                            if (mSwitchVowifiIpFailedCount >= mpPolicy->getPopupErrorRetryCount()) {
                                if (mpPolicy->isPopupErrorNotification()) {
                                    mSwitchVowifiIpFailedCount = 0;
                                    ImsConnManController::showSwitchVowifiFailedNotification();
                                    return;
                                }

                                goto finished;
                            } else {
finished:
                                mpPolicy->createPolicyTimerTask(true, true);
                                return;
                            }
                            break;
                    }
                }

                ImsConnManController::vowifiUnavailable(ImsConnManStateInfo::isWifiConnected());
                mpPolicy->createPolicyTimerTask(true, false);
                break;

            case OPERATION_HANDOVER_TO_VOWIFI:
                LOGE(_TAG(TAG), "operationFailed: callState = %s, callMode = %s, mIsVoltePdnDeactive = %s", \
                        ImsConnManUtils::getCallStateNameById(ImsConnManStateInfo::getCallState()), \
                        ImsConnManUtils::getCallModeNameById(ImsConnManStateInfo::getCallMode()), \
                        mIsVoltePdnDeactive ? "true" : "false");

                if (ImsConnManStateInfo::isIdleState() && CALL_MODE_NO_CALL == ImsConnManStateInfo::getCallMode()) {
                    LOGE(_TAG(TAG), "operationFailed: change to invoke vowifiUnavailable");
                    ImsConnManController::vowifiUnavailable(ImsConnManStateInfo::isWifiConnected());
                } else {
                    if (mIsVoltePdnDeactive) {
                        ImsConnManController::handoverToVolte(false);
                    } else {
                        ImsConnManController::forcedlyHandoverToVolte();
                    }
                }

                mpPolicy->createPolicyTimerTask(false, false);
                break;

            case OPERATION_HANDOVER_TO_VOLTE:
                LOGE(_TAG(TAG), "operationFailed: mIsDpdHandoverToVolte = %s, mIsEpdgStopHandoverToVolte = %s, mIsNoRtpHandoverToVolte = %s", \
                        mIsDpdHandoverToVolte ? "true" : "false", \
                        mIsEpdgStopHandoverToVolte ? "true" : "false", \
                        mIsNoRtpHandoverToVolte ? "true" : "false");

                if (mIsDpdHandoverToVolte || mIsEpdgStopHandoverToVolte || mIsNoRtpHandoverToVolte) {
                    mIsDpdHandoverToVolte = false;
                    mIsNoRtpHandoverToVolte = false;

                    mPendingMsgId = MSG_DEFAULT_ID;
                    ImsConnManStateInfo::setPendingMessageId(mPendingMsgId);

                    if (mIsEpdgStopHandoverToVolte
                            && ImsConnManStateInfo::isWifiCallingEnabled()
                            && ImsConnManStateInfo::isWifiConnected()) {
                        mIsEpdgStopHandoverToVolte = false;
                        ImsConnManController::hungUpImsCall(ImsConnManStateInfo::isWifiConnected());
                        ImsConnManController::vowifiUnavailable(ImsConnManStateInfo::isWifiConnected());                        
                    }

                    break;
                }

                if (!ImsConnManStateInfo::isWifiCallingEnabled() || !ImsConnManStateInfo::isWifiConnected()) {
                    mPendingMsgId = MSG_DEFAULT_ID;
                    ImsConnManStateInfo::setPendingMessageId(mPendingMsgId);

                    ImsConnManController::hungUpImsCall(ImsConnManStateInfo::isWifiConnected());
                    ImsConnManController::vowifiUnavailable(ImsConnManStateInfo::isWifiConnected());
                    break;
                }

                if (MSG_HANDOVER_TO_VOLTE_BY_TIMER == mPendingMsgId) {
                    mPendingMsgId = MSG_DEFAULT_ID;
                    ImsConnManStateInfo::setPendingMessageId(mPendingMsgId);

                    mpPolicy->createPolicyTimerTask(false, false);
                    break;
                }

                mPendingMsgId = MSG_DEFAULT_ID;
                ImsConnManStateInfo::setPendingMessageId(mPendingMsgId);
                break;

            case OPERATION_SET_VOWIFI_UNAVAILABLE:
                break;

            case OPERATION_CANCEL_CURRENT_REQUEST:
                break;

            case OPERATION_CP_REJECT_SWITCH_TO_VOWIFI:
            case OPERATION_CP_REJECT_HANDOVER_TO_VOWIFI:
                LOGE(_TAG(TAG), "operationFailed: isStartActivateVolte = %s", \
                        ImsConnManStateInfo::isStartActivateVolte() ? "true" : "false");

                mpPolicy->createPolicyTimerTask(false, false);
                break;

            case OPERATION_RELEASE_VOWIFI_RESOURCE:
                mIsQuicklySwitchVowifi = false;
                mpPolicy->createPolicyTimerTask(false, false);
                break;

            default:
                LOGE(_TAG(TAG), "operationFailed: unknown type = %d, please check !!!!!", type);
                return;
        }

        if (mIsProcessingLoopMsg) {
            mpHandler->sendMessage(mpHandler->obtainMessage(MSG_LOOP_PROCESS, mMessageArg));
        }
    } else {
        LOGE(_TAG(TAG), "operationFailed: \"id\" and \"RequestId\" are different, RequestId = %d, please check !!!!!", \
                ImsConnManUtils::getRequestId());
    }
}

void ImsConnManService::imsCallEndProcess() {
    LOGD(_TAG(TAG), "[imsCallEndProcess]: callState = %s, callMode = %s, isImsCallActive = %s", \
            ImsConnManUtils::getCallStateNameById(ImsConnManStateInfo::getCallState()), \
            ImsConnManUtils::getCallModeNameById(ImsConnManStateInfo::getCallMode()), \
            ImsConnManStateInfo::isImsCallActive() ? "true" : "false");

    if (!ImsConnManStateInfo::isVowifiRegistered()
            && MSG_HANDOVER_TO_VOWIFI != mPendingMsgId
            && CALL_STATE_IDLE == ImsConnManStateInfo::getCallState()
            && CALL_MODE_NO_CALL == ImsConnManStateInfo::getCallMode()
            && !ImsConnManStateInfo::isImsCallActive()) {
        ImsConnManController::vowifiUnavailable(ImsConnManStateInfo::isWifiConnected());

        int cpRegState = ImsConnManUtils::getCpRegisterState();
        if (VOLTE_REG_STATE_REGISTERED != cpRegState) {
            mIsWaitVolteRegAfterImsCallEnd = true;
            LOGD(_TAG(TAG), "[imsCallEndProcess]: mIsWaitVolteRegAfterImsCallEnd = %s", \
                    mIsWaitVolteRegAfterImsCallEnd ? "true" : "false");
        }
    }
}

void ImsConnManService::voltePdnStateChanged(int state) {
    switch (state) {
        case VOLTE_PDN_ACTIVE_FAILED:
            LOGD(_TAG(TAG), "CP module de-actived Volte PDN, callState = %s, callMode = %s, mPendingMsgId = %s", \
                    ImsConnManUtils::getCallStateNameById(ImsConnManStateInfo::getCallState()), \
                    ImsConnManUtils::getCallModeNameById(ImsConnManStateInfo::getCallMode()), \
                    ImsConnManUtils::getMessageNameById(mPendingMsgId));

            mIsVoltePdnDeactive = true;
            ImsConnManStateInfo::setStartActivateVolte(false);

            if (!ImsConnManStateInfo::isVowifiRegistered()
                    && !mIsMsgQueuePending
                    && isMeetInitiateVowifiConditions()
                    && !ImsConnManStateInfo::isIdleState()
                    && CALL_MODE_VOLTE_CALL == ImsConnManStateInfo::getCallMode()) {
                LOGD(_TAG(TAG), "CP module de-actived Volte PDN during Volte call, need immediately handover vowifi");
                mpPolicy->createPolicyTimerTask(false, true);
            }
            break;

        case VOLTE_PDN_READY:
            LOGD(_TAG(TAG), "CP module successfully actived Volte PDN, callState = %s, callMode = %s, mPendingMsgId = %s", \
                    ImsConnManUtils::getCallStateNameById(ImsConnManStateInfo::getCallState()), \
                    ImsConnManUtils::getCallModeNameById(ImsConnManStateInfo::getCallMode()), \
                    ImsConnManUtils::getMessageNameById(mPendingMsgId));

            mIsVoltePdnDeactive = false;

            if (!ImsConnManStateInfo::isIdleState()
                    && CALL_MODE_NO_CALL != ImsConnManStateInfo::getCallMode()
                    && CALL_MODE_CS_CALL != ImsConnManStateInfo::getCallMode()) {
                ImsConnManStateInfo::setStartActivateVolte(false);
            }
            break;

        case VOLTE_PDN_START:
            LOGD(_TAG(TAG), "CP module start to active Volte PDN, callState = %s, callMode = %s, mPendingMsgId = %s", \
                    ImsConnManUtils::getCallStateNameById(ImsConnManStateInfo::getCallState()), \
                    ImsConnManUtils::getCallModeNameById(ImsConnManStateInfo::getCallMode()), \
                    ImsConnManUtils::getMessageNameById(mPendingMsgId));

            ImsConnManStateInfo::setStartActivateVolte(true);
            break;

        default:
            break;
    }
}

void ImsConnManService::noRtpReceivedProcess(bool isVideoPacket) {
}

void ImsConnManService::rtpReceivedProcess(bool isVideoPacket) {
}

void ImsConnManService::mediaQualityReceivedProcess(bool isVideoPacket, int loss, int jitter, int rtt) {
}

void ImsConnManService::srvccFaildProcess() {
    LOGD(_TAG(TAG), "srvccFaildProcess: imsBearer = %s, callState = %s, callMode = %s", \
            ImsConnManUtils::getImsBearerNameById(ImsConnManUtils::getCurrentImsBearer()), \
            ImsConnManUtils::getCallStateNameById(ImsConnManStateInfo::getCallState()), \
            ImsConnManUtils::getCallModeNameById(ImsConnManStateInfo::getCallMode()));

    ImsConnManController::hungUpImsCall(ImsConnManStateInfo::isWifiConnected());

    if (IMS_TYPE_VOWIFI == ImsConnManUtils::getCurrentUsedImsStack()) {
        ImsConnManController::vowifiUnavailable(ImsConnManStateInfo::isWifiConnected());
    }
}

void ImsConnManService::imsCallTypeChanged() {
    if (!ImsConnManStateInfo::isIdleState() && CALL_MODE_VOWIFI_CALL == ImsConnManStateInfo::getCallMode()) {
        LOGD(_TAG(TAG), "imsCallTypeChanged: callMode = %s, callType = %s", \
                ImsConnManUtils::getCallModeNameById(ImsConnManStateInfo::getCallMode()), \
                ImsConnManUtils::getImsCallTypeNameById(ImsConnManStateInfo::getImsCallType()));

        mpPolicy->createPolicyTimerTask(false, false);
    }
}

void ImsConnManService::onProcessDpdDisconnectedError() {
    LOGE(_TAG(TAG), "onProcessDpdDisconnectedError: isVowifiRegistered = %s, imsBearer = %s, callState = %s, callMode = %s", \
            ImsConnManStateInfo::isVowifiRegistered() ? "true" : "false", \
            ImsConnManUtils::getImsBearerNameById(ImsConnManUtils::getCurrentImsBearer()), \
            ImsConnManUtils::getCallStateNameById(ImsConnManStateInfo::getCallState()), \
            ImsConnManUtils::getCallModeNameById(ImsConnManStateInfo::getCallMode()));

    if (ImsConnManStateInfo::isWifiCallingEnabled()
            && ImsConnManStateInfo::isWifiConnected()
            && ImsConnManStateInfo::isVowifiRegistered()) {
        if (ImsConnManStateInfo::isIdleState() && CALL_MODE_NO_CALL == ImsConnManStateInfo::getCallMode()) {
            ImsConnManController::vowifiUnavailable(ImsConnManStateInfo::isWifiConnected());
        } else if (!ImsConnManStateInfo::isIdleState() && CALL_MODE_VOWIFI_CALL == ImsConnManStateInfo::getCallMode()) {
            if (ImsConnManStateInfo::isLteCellularNetwork()) {
                mIsDpdHandoverToVolte = true;
                ImsConnManController::handoverToVolte(false);
            } else {
                ImsConnManController::hungUpImsCall(ImsConnManStateInfo::isWifiConnected());
                ImsConnManController::vowifiUnavailable(ImsConnManStateInfo::isWifiConnected());
            }
        }
    }
}

void ImsConnManService::onProcessSipStackError(int errorCode) {
    string errorCodeName;

    if (ERROR_SIP_TIMEOUT == errorCode) {
        errorCodeName = "SIP TIMEOUT";
    } else if (ERROR_SIP_LOGOUT == errorCode) {
        errorCodeName = "SIP LOGOUT";
    }

    LOGE(_TAG(TAG), "onProcessSipStackError: %s, isVowifiRegistered = %s, imsBearer = %s, callState = %s, callMode = %s", \
            errorCodeName.c_str(), \
            ImsConnManStateInfo::isVowifiRegistered() ? "true" : "false", \
            ImsConnManUtils::getImsBearerNameById(ImsConnManUtils::getCurrentImsBearer()), \
            ImsConnManUtils::getCallStateNameById(ImsConnManStateInfo::getCallState()), \
            ImsConnManUtils::getCallModeNameById(ImsConnManStateInfo::getCallMode()));

    if (ImsConnManStateInfo::isWifiCallingEnabled()
            && ImsConnManStateInfo::isWifiConnected()) {
        if (ImsConnManStateInfo::isIdleState() && CALL_MODE_NO_CALL == ImsConnManStateInfo::getCallMode()) {
            ImsConnManController::vowifiUnavailable(ImsConnManStateInfo::isWifiConnected());
        } else if (!ImsConnManStateInfo::isIdleState() && IMS_TYPE_VOWIFI == ImsConnManUtils::getCurrentUsedImsStack()) {
            ImsConnManController::hungUpImsCall(ImsConnManStateInfo::isWifiConnected());
            ImsConnManController::vowifiUnavailable(ImsConnManStateInfo::isWifiConnected());
        }
    }
}

void ImsConnManService::onProcessSecurityRekeyError() {
    LOGE(_TAG(TAG), "onProcessSecurityRekeyError: isVowifiRegistered = %s, imsBearer = %s, callState = %s, callMode = %s", \
            ImsConnManStateInfo::isVowifiRegistered() ? "true" : "false", \
            ImsConnManUtils::getImsBearerNameById(ImsConnManUtils::getCurrentImsBearer()), \
            ImsConnManUtils::getCallStateNameById(ImsConnManStateInfo::getCallState()), \
            ImsConnManUtils::getCallModeNameById(ImsConnManStateInfo::getCallMode()));

    if (ImsConnManStateInfo::isWifiCallingEnabled()
            && ImsConnManStateInfo::isWifiConnected()
            && ImsConnManStateInfo::isVowifiRegistered()) {
        if (ImsConnManStateInfo::isIdleState() && CALL_MODE_NO_CALL == ImsConnManStateInfo::getCallMode()) {
            mIsQuicklySwitchVowifi = true;
            ImsConnManController::releaseVoWifiResource();
        } else if (!ImsConnManStateInfo::isIdleState() && CALL_MODE_VOWIFI_CALL == ImsConnManStateInfo::getCallMode()) {
            mIsQuicklySwitchVowifi = true;

            ImsConnManController::hungUpImsCall(ImsConnManStateInfo::isWifiConnected());
            ImsConnManController::releaseVoWifiResource();
        }
    }
}

void ImsConnManService::onProcessEpdgStopError() {
    LOGE(_TAG(TAG), "onProcessEpdgStopError: isVowifiRegistered = %s, imsBearer = %s, callState = %s, callMode = %s", \
            ImsConnManStateInfo::isVowifiRegistered() ? "true" : "false", \
            ImsConnManUtils::getImsBearerNameById(ImsConnManUtils::getCurrentImsBearer()), \
            ImsConnManUtils::getCallStateNameById(ImsConnManStateInfo::getCallState()), \
            ImsConnManUtils::getCallModeNameById(ImsConnManStateInfo::getCallMode()));

    if (ImsConnManStateInfo::isWifiCallingEnabled()
            && ImsConnManStateInfo::isWifiConnected()
            && ImsConnManStateInfo::isVowifiRegistered()) {
        if (ImsConnManStateInfo::isIdleState() && CALL_MODE_NO_CALL == ImsConnManStateInfo::getCallMode()) {
            ImsConnManController::vowifiUnavailable(ImsConnManStateInfo::isWifiConnected());
        } else if (!ImsConnManStateInfo::isIdleState() && CALL_MODE_VOWIFI_CALL == ImsConnManStateInfo::getCallMode()) {
            if (ImsConnManStateInfo::isLteCellularNetwork()) {
                mIsEpdgStopHandoverToVolte = true;
                ImsConnManController::handoverToVolte(false);
            } else {
                ImsConnManController::hungUpImsCall(ImsConnManStateInfo::isWifiConnected());
                ImsConnManController::vowifiUnavailable(ImsConnManStateInfo::isWifiConnected());
            }
        }
    }
}

void ImsConnManService::onUnsolicitedVowifiError(int errorCode) {
    switch (errorCode) {
        case ERROR_DPD_DISCONNECTED:
            onProcessDpdDisconnectedError();
            break;

        case ERROR_SIP_TIMEOUT:
        case ERROR_SIP_LOGOUT:
            onProcessSipStackError(errorCode);
            break;

        case ERROR_SECURITY_REKEY:
            onProcessSecurityRekeyError();
            break;

        case ERROR_EPDG_STOP:
            onProcessEpdgStopError();
            break;

        default:
            LOGE(_TAG(TAG), "onUnsolicitedVowifiError: invalid error code = %d", errorCode);
            break;
    }
}

void ImsConnManService::clearLoopMsgBuffer() {
    if (!mVectorLoopMsgBuffer.empty())  {
        LOGD(_TAG(TAG), "clearLoopMsgBuffer: remove all of \"Loop Message Queue\", size = %d, mVectorLoopMsgBuffer[0] = %s", \
                mVectorLoopMsgBuffer.size(), \
                ImsConnManUtils::getMessageNameById(mVectorLoopMsgBuffer.at(0)));

        mVectorLoopMsgBuffer.clear();
    }
}

void ImsConnManService::clearSwitchVowifiMsgBuffer() {
    if (!mVectorSwitchVowifiMsgBuffer.empty()) {
        LOGD(_TAG(TAG), "clearSwitchVowifiMsgBuffer: remove all of \"MSG_SWITCH_TO_VOWIFI Message Queue\", size = %d", \
                mVectorSwitchVowifiMsgBuffer.size());

        mVectorSwitchVowifiMsgBuffer.clear();
    }
}

void ImsConnManService::processingLoopMessageBegin(int msgArg) {
    if (!mIsMsgQueuePending) {
        if (!mVectorLoopMsgBuffer.empty()) {
            int i = 0;
            vector<int>::iterator iter;
            for (iter = mVectorLoopMsgBuffer.begin(); iter != mVectorLoopMsgBuffer.end(); iter++, i++) {
                LOGD(_TAG(TAG), "processingLoopMessageBegin: \"Loop Message Queue\" hava mVectorLoopMsgBuffer[%d] = %s", \
                        i, ImsConnManUtils::getMessageNameById(*iter));
            }
        } else {
            mMessageArg = 0;
            mIsProcessingLoopMsg = false;
            LOGD(_TAG(TAG), "processingLoopMessageBegin: Need to reset mIsProcessingLoopMsg when \"Loop Message Queue\" is empty, " \
                    "mIsProcessingLoopMsg = %s", mIsProcessingLoopMsg ? "true" : "false");
        }
    }
}

void ImsConnManService::processingLoopMessageEnd(int msgArg) {
    if (mIsMsgQueuePending) {
        mMessageArg = msgArg;
        //mpHandler->sendMessage(mpHandler->obtainMessage(MSG_LOOP_PROCESS, msgArg));
        return;
    }

    mMessageArg = 0;
    mIsProcessingLoopMsg = false;

    int queueSize = mVectorLoopMsgBuffer.size();
    LOGD(_TAG(TAG), "processingLoopMessageEnd: \"Loop Message Queue\" size = %d", queueSize);
    if (queueSize > 0) {
        int loopMsgId = mVectorLoopMsgBuffer.back();
        LOGD(_TAG(TAG), "processingLoopMessageEnd: send and remove this Message from \"Loop Message Queue\", loopMsgId = %s", \
                ImsConnManUtils::getMessageNameById(loopMsgId));

        mVectorLoopMsgBuffer.clear();
        mpHandler->sendMessage(mpHandler->obtainMessage(loopMsgId, msgArg));
    }
}

void ImsConnManService::handleMessageSwitchToVowifiBegin() {
    LOGD(_TAG(TAG), "handleMessageSwitchToVowifiBegin: mIsMsgQueuePending = %s", mIsMsgQueuePending ? "true" : "false");

    if (mIsWaitVolteRegAfterImsCallEnd) {
        LOGE(_TAG(TAG), "handleMessageSwitchToVowifiBegin: Waiting for Volte registered after Volte call end, don't [switch vowifi]");
        return;
    }

    if (mIsMsgQueuePending || !mVectorLoopMsgBuffer.empty()) {
        LOGD(_TAG(TAG), "handleMessageSwitchToVowifiBegin: \"MSG_SWITCH_TO_VOWIFI\", mPendingMsgId = %s, mIsProcessingLoopMsg = %s", \
                ImsConnManUtils::getMessageNameById(mPendingMsgId), \
                mIsProcessingLoopMsg ? "true" : "false");

        clearLoopMsgBuffer();

        mVectorLoopMsgBuffer.push_back(MSG_SWITCH_TO_VOWIFI);
        LOGD(_TAG(TAG), "handleMessageSwitchToVowifiBegin: add element in \"Loop Message Queue\" +++++++ \"MSG_SWITCH_TO_VOWIFI\"");

        if (MSG_SWITCH_TO_VOWIFI == mPendingMsgId) {
            mVectorSwitchVowifiMsgBuffer.push_back(MSG_SWITCH_TO_VOWIFI);
            LOGD(_TAG(TAG), "handleMessageSwitchToVowifiBegin: add element in \"MSG_SWITCH_TO_VOWIFI Message Queue\" <<<<<<< \"MSG_SWITCH_TO_VOWIFI\", " \
                    "size = %d", mVectorSwitchVowifiMsgBuffer.size());
        }
    }
}

void ImsConnManService::handleMessageSwitchToVowifiEnd() {
    if (mIsMsgQueuePending || !mVectorLoopMsgBuffer.empty()) {
        if (!mIsProcessingLoopMsg) {
            mIsProcessingLoopMsg = true;
            mpHandler->sendMessage(mpHandler->obtainMessage(MSG_LOOP_PROCESS, 0));
        }

        return;
    }

    int imsBearer = ImsConnManUtils::getCurrentImsBearer();
    LOGD(_TAG(TAG), "handleMessageSwitchToVowifiEnd: isVowifiRegistered = %s, imsBearer = %s", \
            ImsConnManStateInfo::isVowifiRegistered() ? "true" : "false", \
            ImsConnManUtils::getImsBearerNameById(imsBearer));

    if (ImsConnManStateInfo::isVowifiRegistered() || IMS_TYPE_VOWIFI == imsBearer) {
        LOGE(_TAG(TAG), "handleMessageSwitchToVowifiEnd: Vowifi is registered, don't [switch vowifi] again");
        return;
    }

    mPendingMsgId = MSG_SWITCH_TO_VOWIFI;
    ImsConnManStateInfo::setPendingMessageId(mPendingMsgId);
    mRequestId = ImsConnManUtils::switchToVowifi();
    if (mRequestId >= 0) {
        mIsMsgQueuePending = true;
        ImsConnManStateInfo::setMessageQueuePendingState(mIsMsgQueuePending);
        LOGD(_TAG(TAG), "handleMessageSwitchToVowifiEnd: mRequestId = %d", mRequestId);

        clearSwitchVowifiMsgBuffer();
    } else {
        mPendingMsgId = MSG_DEFAULT_ID;
        ImsConnManStateInfo::setPendingMessageId(mPendingMsgId);
        LOGE(_TAG(TAG), "handleMessageSwitchToVowifiEnd: [switch vowifi] failed, mRequestId = %d", mRequestId);
    }
}

void ImsConnManService::handleMessageHandoverToVowifiBegin() {
    LOGD(_TAG(TAG), "handleMessageHandoverToVowifiBegin: mIsMsgQueuePending = %s", mIsMsgQueuePending ? "true" : "false");

    if (mIsMsgQueuePending || !mVectorLoopMsgBuffer.empty()) {
        LOGD(_TAG(TAG), "handleMessageHandoverToVowifiBegin: \"MSG_HANDOVER_TO_VOWIFI\", mPendingMsgId = %s, mIsProcessingLoopMsg = %s", \
                ImsConnManUtils::getMessageNameById(mPendingMsgId), \
                mIsProcessingLoopMsg ? "true" : "false");

        clearLoopMsgBuffer();

        mVectorLoopMsgBuffer.push_back(MSG_HANDOVER_TO_VOWIFI);
        LOGD(_TAG(TAG), "handleMessageHandoverToVowifiBegin: add element in \"Loop Message Queue\" +++++++ \"MSG_HANDOVER_TO_VOWIFI\"");
    }
}

void ImsConnManService::handleMessageHandoverToVowifiEnd() {
    if (mIsMsgQueuePending || !mVectorLoopMsgBuffer.empty()) {
        if (!mIsProcessingLoopMsg) {
            mIsProcessingLoopMsg = true;
            mpHandler->sendMessage(mpHandler->obtainMessage(MSG_LOOP_PROCESS, 0));
        }

        return;
    }

    int imsBearer = ImsConnManUtils::getCurrentImsBearer();
    LOGD(_TAG(TAG), "handleMessageHandoverToVowifiEnd: isVowifiRegistered = %s, imsBearer = %s", \
            ImsConnManStateInfo::isVowifiRegistered() ? "true" : "false", \
            ImsConnManUtils::getImsBearerNameById(imsBearer));

    if (ImsConnManStateInfo::isVowifiRegistered() || IMS_TYPE_VOWIFI == imsBearer) {
        LOGD(_TAG(TAG), "handleMessageHandoverToVowifiEnd: Vowifi is registered, don't [handover vowifi] again");
        return;
    }

    mPendingMsgId = MSG_HANDOVER_TO_VOWIFI;
    ImsConnManStateInfo::setPendingMessageId(mPendingMsgId);
    mRequestId = ImsConnManUtils::handoverToVowifi();
    if (mRequestId) {
        mIsMsgQueuePending = true;
        ImsConnManStateInfo::setMessageQueuePendingState(mIsMsgQueuePending);
        LOGD(_TAG(TAG), "handleMessageHandoverToVowifiEnd: mRequestId = %d", mRequestId);
    } else {
        mPendingMsgId = MSG_DEFAULT_ID;
        ImsConnManStateInfo::setPendingMessageId(mPendingMsgId);
        LOGE(_TAG(TAG), "handleMessageHandoverToVowifiEnd: [handover vowifi] failed, mRequestId = %d", mRequestId);
    }
}

void ImsConnManService::handleMessageHandoverToVolteBegin(int msgArg) {
    LOGD(_TAG(TAG), "handleMessageHandoverToVolteBegin: mIsMsgQueuePending = %s, %s handover to Volte", \
            mIsMsgQueuePending ? "true" : "false", \
            msgArg ? "forcedly" : "un-forcedly");

    if (mIsMsgQueuePending || !mVectorLoopMsgBuffer.empty()) {
        LOGD(_TAG(TAG), "handleMessageHandoverToVolteBegin: \"MSG_HANDOVER_TO_VOLTE\", mPendingMsgId = %s, mIsProcessingLoopMsg = %s", \
                ImsConnManUtils::getMessageNameById(mPendingMsgId), \
                mIsProcessingLoopMsg ? "true" : "false");

        clearLoopMsgBuffer();

        mVectorLoopMsgBuffer.push_back(MSG_HANDOVER_TO_VOLTE);
        LOGD(_TAG(TAG), "handleMessageHandoverToVolteBegin: add element in \"Loop Message Queue\" +++++++ \"MSG_HANDOVER_TO_VOLTE\"");
    }
}

void ImsConnManService::handleMessageHandoverToVolteEnd(int msgArg) {
    if (mIsMsgQueuePending || !mVectorLoopMsgBuffer.empty()) {
        if (!mIsProcessingLoopMsg) {
            mIsProcessingLoopMsg = true;
            mpHandler->sendMessage(mpHandler->obtainMessage(MSG_LOOP_PROCESS, msgArg));
        }

        return;
    }

    bool isForcedly = msgArg;
    int imsBearer = ImsConnManUtils::getCurrentImsBearer();
    LOGD(_TAG(TAG), "handleMessageHandoverToVolteEnd: isVolteRegistered = %s, imsBearer = %s", \
            ImsConnManStateInfo::isVowifiRegistered() ? "true" : "false", \
            ImsConnManUtils::getImsBearerNameById(imsBearer));

    if (!isForcedly && (ImsConnManStateInfo::isVolteRegistered() || IMS_TYPE_VOLTE == imsBearer)) {
        LOGE(_TAG(TAG), "handleMessageHandoverToVolteEnd: Volte is registered, don't [handover volte]");
        return;
    }

    mPendingMsgId = MSG_HANDOVER_TO_VOLTE;
    ImsConnManStateInfo::setPendingMessageId(mPendingMsgId);
    mRequestId = ImsConnManUtils::handoverToVolte();
    if (mRequestId >= 0) {
        mIsMsgQueuePending = true;
        ImsConnManStateInfo::setMessageQueuePendingState(mIsMsgQueuePending);
        LOGD(_TAG(TAG), "handleMessageHandoverToVolteEnd: mRequestId = %d", mRequestId);
    } else {
        mPendingMsgId = MSG_DEFAULT_ID;
        ImsConnManStateInfo::setPendingMessageId(mPendingMsgId);
        LOGE(_TAG(TAG), "handleMessageHandoverToVolteEnd: [handover volte] failed, mRequestId = %d", mRequestId);
    }
}

void ImsConnManService::handleMessageHandoverToVolteByTimerBegin() {
    LOGD(_TAG(TAG), "handleMessageHandoverToVolteByTimerBegin: mIsMsgQueuePending = %s", mIsMsgQueuePending ? "true" : "false");

    if (mIsMsgQueuePending || !mVectorLoopMsgBuffer.empty()) {
        LOGD(_TAG(TAG), "handleMessageHandoverToVolteByTimerBegin: \"MSG_HANDOVER_TO_VOLTE_BY_TIMER\", mPendingMsgId = %s, mIsProcessingLoopMsg = %s", \
                ImsConnManUtils::getMessageNameById(mPendingMsgId), \
                mIsProcessingLoopMsg ? "true" : "false");

        clearLoopMsgBuffer();

        mVectorLoopMsgBuffer.push_back(MSG_HANDOVER_TO_VOLTE_BY_TIMER);
        LOGD(_TAG(TAG), "handleMessageHandoverToVolteByTimerBegin: add element in \"Loop Message Queue\" +++++++ \"MSG_HANDOVER_TO_VOLTE_BY_TIMER\"");
    }
}

void ImsConnManService::handleMessageHandoverToVolteByTimerEnd() {
    if (mIsMsgQueuePending || !mVectorLoopMsgBuffer.empty()) {
        if (!mIsProcessingLoopMsg) {
            mIsProcessingLoopMsg = true;
            mpHandler->sendMessage(mpHandler->obtainMessage(MSG_LOOP_PROCESS, 0));
        }

        return;
    }

    int imsBearer = ImsConnManUtils::getCurrentImsBearer();
    LOGD(_TAG(TAG), "handleMessageHandoverToVolteByTimerEnd: isVolteRegistered = %s, imsBearer = %s", \
            ImsConnManStateInfo::isVowifiRegistered() ? "true" : "false", \
            ImsConnManUtils::getImsBearerNameById(imsBearer));

    if (ImsConnManStateInfo::isVolteRegistered() || IMS_TYPE_VOLTE == imsBearer) {
        LOGE(_TAG(TAG), "handleMessageHandoverToVolteByTimerEnd: Volte is registered, don't [handover volte by time] again");
        return;
    }

    mPendingMsgId = MSG_HANDOVER_TO_VOLTE_BY_TIMER;
    ImsConnManStateInfo::setPendingMessageId(mPendingMsgId);
    mRequestId = ImsConnManUtils::handoverToVolte();
    if (mRequestId) {
        mIsMsgQueuePending = true;
        ImsConnManStateInfo::setMessageQueuePendingState(mIsMsgQueuePending);
        LOGD(_TAG(TAG), "handleMessageHandoverToVolteByTimerEnd: mRequestId = %d", mRequestId);
    } else {
        mPendingMsgId = MSG_DEFAULT_ID;
        ImsConnManStateInfo::setPendingMessageId(mPendingMsgId);
        LOGE(_TAG(TAG), "handleMessageHandoverToVolteByTimerEnd: [handover volte by time] failed, mRequestId = %d", mRequestId);
    }
}

void ImsConnManService::handleMessageReleaseVowifiResourceBegin() {
    LOGD(_TAG(TAG), "handleMessageReleaseVowifiResourceBegin: mIsMsgQueuePending = %s", mIsMsgQueuePending ? "true" : "false");

    if (mIsMsgQueuePending || !mVectorLoopMsgBuffer.empty()) {
        LOGD(_TAG(TAG), "handleMessageReleaseVowifiResourceBegin: \"MSG_RELEASE_VOWIFI_RES\", mPendingMsgId = %s, mIsProcessingLoopMsg = %s", \
                ImsConnManUtils::getMessageNameById(mPendingMsgId), \
                mIsProcessingLoopMsg ? "true" : "false");

        clearLoopMsgBuffer();

        mVectorLoopMsgBuffer.push_back(MSG_RELEASE_VOWIFI_RES);
        LOGD(_TAG(TAG), "handleMessageReleaseVowifiResourceBegin: add element in \"Loop Message Queue\" +++++++ \"MSG_RELEASE_VOWIFI_RES\"");
    }
}

void ImsConnManService::handleMessageReleaseVowifiResourceEnd() {
    if (mIsMsgQueuePending || !mVectorLoopMsgBuffer.empty()) {
        if (!mIsProcessingLoopMsg) {
            mIsProcessingLoopMsg = true;
            mpHandler->sendMessage(mpHandler->obtainMessage(MSG_LOOP_PROCESS, 0));
        }

        return;
    }

    mPendingMsgId = MSG_RELEASE_VOWIFI_RES;
    ImsConnManStateInfo::setPendingMessageId(mPendingMsgId);
    mRequestId = ImsConnManUtils::releaseVoWifiResource();
    if (mRequestId >= 0) {
        mIsMsgQueuePending = true;
        ImsConnManStateInfo::setMessageQueuePendingState(mIsMsgQueuePending);
        LOGD(_TAG(TAG), "handleMessageReleaseVowifiResourceEnd: mRequestId = %d", mRequestId);
    } else {
        mPendingMsgId = MSG_DEFAULT_ID;
        ImsConnManStateInfo::setPendingMessageId(mPendingMsgId);
        LOGE(_TAG(TAG), "handleMessageReleaseVowifiResourceEnd: [release vowifi resource] failed, mRequestId = %d", mRequestId);
    }
}

void ImsConnManService::handleMessageVowifiUnavailableBegin(int msgArg) {
    LOGD(_TAG(TAG), "handleMessageVowifiUnavailableBegin: mIsMsgQueuePending = %s, Wifi network is %s", \
            mIsMsgQueuePending ? "true" : "false", \
            msgArg ? "\"Connected\"" : "\"Disconnected\"");

    if (!ImsConnManStateInfo::isVowifiRegistered() && !msgArg) {
        mIsNeedSync = false;
        LOGD(_TAG(TAG), "handleMessageVowifiUnavailableBegin: Don't set Vowifi unavailable during non-Vowifi");

        return;
    }

    if (mIsMsgQueuePending || !mVectorLoopMsgBuffer.empty()) {
        LOGD(_TAG(TAG), "handleMessageVowifiUnavailableBegin: \"MSG_VOWIFI_UNAVAILABLE\", mPendingMsgId = %s, mIsProcessingLoopMsg = %s", \
                ImsConnManUtils::getMessageNameById(mPendingMsgId), \
                mIsProcessingLoopMsg ? "true" : "false");

        clearLoopMsgBuffer();

        LOGD(_TAG(TAG), "handleMessageVowifiUnavailableBegin: \"MSG_SWITCH_TO_VOWIFI Message Queue\" size = %d", \
                mVectorSwitchVowifiMsgBuffer.size());
        if (mVectorSwitchVowifiMsgBuffer.size() >= SWITCH_VOWIFI_MSG_BUFFER_SIZE
                && MSG_SWITCH_TO_VOWIFI == mPendingMsgId) {
            clearSwitchVowifiMsgBuffer();

            LOGD(_TAG(TAG), "handleMessageVowifiUnavailableBegin: change to invoke cancelCurrentRequest when over SWITCH_VOWIFI_MSG_BUFFER_SIZE");
            ImsConnManController::cancelCurrentRequest();
            return;
        }

        mVectorLoopMsgBuffer.push_back(MSG_VOWIFI_UNAVAILABLE);
        LOGD(_TAG(TAG), "handleMessageVowifiUnavailableBegin: add element in \"Loop Message Queue\" +++++++ \"MSG_VOWIFI_UNAVAILABLE\"");
    }
}

void ImsConnManService::handleMessageVowifiUnavailableEnd(int msgArg) {
    if (!mIsNeedSync) {
        mIsNeedSync = true;
        return;
    }

    if (mIsMsgQueuePending || !mVectorLoopMsgBuffer.empty()) {
        if (!mIsProcessingLoopMsg) {
            mIsProcessingLoopMsg = true;
            mpHandler->sendMessage(mpHandler->obtainMessage(MSG_LOOP_PROCESS, msgArg));
        }

        return;
    }

    mPendingMsgId = MSG_VOWIFI_UNAVAILABLE;
    ImsConnManStateInfo::setPendingMessageId(mPendingMsgId);
    mRequestId = ImsConnManUtils::setVowifiUnavailable(msgArg, false);
    if (mRequestId >= 0) {
        mIsMsgQueuePending = true;
        ImsConnManStateInfo::setMessageQueuePendingState(mIsMsgQueuePending);
        LOGD(_TAG(TAG), "handleMessageVowifiUnavailableEnd: mRequestId = %d", mRequestId);

        clearSwitchVowifiMsgBuffer();
    } else {
        mPendingMsgId = MSG_DEFAULT_ID;
        ImsConnManStateInfo::setPendingMessageId(mPendingMsgId);
        LOGE(_TAG(TAG), "handleMessageVowifiUnavailableEnd: [set vowifi unavailable] failed, mRequestId = %d", mRequestId);
    }
}

void ImsConnManService::handleMessageCancelCurrentRequestBegin() {
    LOGD(_TAG(TAG), "handleMessageCancelCurrentRequestBegin: mIsMsgQueuePending = %s", mIsMsgQueuePending ? "true" : "false");

    if (mIsMsgQueuePending || !mVectorLoopMsgBuffer.empty()) {
        LOGD(_TAG(TAG), "handleMessageCancelCurrentRequestBegin: \"MSG_CANCEL_CURRENT_REQUEST\", mPendingMsgId = %s, mIsProcessingLoopMsg = %s", \
                ImsConnManUtils::getMessageNameById(mPendingMsgId), \
                mIsProcessingLoopMsg ? "true" : "false");

        clearLoopMsgBuffer();

        if (MSG_RELEASE_VOWIFI_RES == mPendingMsgId
                || MSG_VOWIFI_UNAVAILABLE == mPendingMsgId
                || MSG_CANCEL_CURRENT_REQUEST == mPendingMsgId) {
            mIsNeedSync = false;
            clearSwitchVowifiMsgBuffer();
            LOGD(_TAG(TAG), "handleMessageCancelCurrentRequestBegin: don't [cancel current request] " \
                    "during process of release or unavailable or cancel Vowifi");

            return;
        }
    }
}

void ImsConnManService::handleMessageCancelCurrentRequestEnd() {
    if (!mIsNeedSync) {
        mIsNeedSync = true;
        return;
    }

    LOGD(_TAG(TAG), "handleMessageCancelCurrentRequestEnd: previous operation mRequestId = %d", mRequestId);
    mPendingMsgId = MSG_CANCEL_CURRENT_REQUEST;
    ImsConnManStateInfo::setPendingMessageId(mPendingMsgId);
    mRequestId = ImsConnManUtils::cancelCurrentRequest();
    if (mRequestId >= 0) {
        mIsMsgQueuePending = true;
        ImsConnManStateInfo::setMessageQueuePendingState(mIsMsgQueuePending);
        LOGD(_TAG(TAG), "handleMessageCancelCurrentRequestEnd: mRequestId = %d", mRequestId);

        clearSwitchVowifiMsgBuffer();
    } else {
        mPendingMsgId = MSG_DEFAULT_ID;
        ImsConnManStateInfo::setPendingMessageId(mPendingMsgId);
        LOGE(_TAG(TAG), "handleMessageCancelCurrentRequestEnd: [cancel current request] failed, mRequestId = %d", mRequestId);
    }
}

void ImsConnManService::handleMessageBegin(const shared_ptr<Message> &msg) {
    switch (msg->what) {
        case MSG_LOOP_PROCESS:
            processingLoopMessageBegin(msg->arg1);
            break;

        case MSG_SWITCH_TO_VOWIFI:
            handleMessageSwitchToVowifiBegin();
            break;

        case MSG_HANDOVER_TO_VOWIFI:
            handleMessageHandoverToVowifiBegin();
            break;

        case MSG_HANDOVER_TO_VOLTE:
            handleMessageHandoverToVolteBegin(msg->arg1);
            break;

        case MSG_HANDOVER_TO_VOLTE_BY_TIMER:
            handleMessageHandoverToVolteByTimerBegin();
            break;

        case MSG_RELEASE_VOWIFI_RES:
            handleMessageReleaseVowifiResourceBegin();
            break;

        case MSG_VOWIFI_UNAVAILABLE:
            handleMessageVowifiUnavailableBegin(msg->arg1);
            break;

        case MSG_CANCEL_CURRENT_REQUEST:
            handleMessageCancelCurrentRequestBegin();
            break;

        default:
            LOGE(_TAG(TAG), "handleMessageBegin: invalid message id: %d", msg->what);
            return;
    }

    mpHandler->dispatchToMainThread(msg, mpCommonMsgResultTask);
}

void ImsConnManService::handleMessageEnd(const shared_ptr<Message> &msg) {
    switch (msg->what) {
        case MSG_LOOP_PROCESS:
            processingLoopMessageEnd(msg->arg1);
            break;

        case MSG_SWITCH_TO_VOWIFI:
            handleMessageSwitchToVowifiEnd();
            break;

        case MSG_HANDOVER_TO_VOWIFI:
            handleMessageHandoverToVowifiEnd();
            break;

        case MSG_HANDOVER_TO_VOLTE:
            handleMessageHandoverToVolteEnd(msg->arg1);
            break;

        case MSG_HANDOVER_TO_VOLTE_BY_TIMER:
            handleMessageHandoverToVolteByTimerEnd();
            break;

        case MSG_RELEASE_VOWIFI_RES:
            handleMessageReleaseVowifiResourceEnd();
            break;

        case MSG_VOWIFI_UNAVAILABLE:
            handleMessageVowifiUnavailableEnd(msg->arg1);
            break;

        case MSG_CANCEL_CURRENT_REQUEST:
            handleMessageCancelCurrentRequestEnd();
            break;

        default:
            LOGE(_TAG(TAG), "handleMessageEnd: invalid message id: %d", msg->what);
            break;
    }}

void ImsConnManService::onImsRegisterStateChange(int imsBearer) {
    if (imsBearer != ImsConnManUtils::getCurrentImsBearer()) {
        LOGE(_TAG(TAG), "[onImsRegisterStateChange]: imsBearer = %s and getCurrentImsBearer = %s value are differently, please check !!!!!", \
                ImsConnManUtils::getImsBearerNameById(imsBearer), \
                ImsConnManUtils::getImsBearerNameById(ImsConnManUtils::getCurrentImsBearer()));
        return;
    }

    int cpRegState = ImsConnManUtils::getCpRegisterState();
    LOGD(_TAG(TAG), "[onImsRegisterStateChange]: imsBearer = %s, cpRegState = %s", \
            ImsConnManUtils::getImsBearerNameById(imsBearer), \
            ImsConnManUtils::getCpRegisterStateNameById(cpRegState));

    if (IMS_TYPE_VOLTE == imsBearer) {
        bool newVolteReg = true;
        ImsConnManStateInfo::setVowifiRegistered(!newVolteReg);

        LOGD(_TAG(TAG), "[onImsRegisterStateChange]: new Volte is \"Registered\", old Volte is %s, mCurPendingMsgId = %s", \
                ImsConnManStateInfo::isVolteRegistered() ? "\"Registered\"" : "\"Un-Registered\"", \
                ImsConnManUtils::getMessageNameById(mPendingMsgId));

        if (!ImsConnManStateInfo::isIdleState()) {
            ImsConnManStateInfo::setCallMode(CALL_MODE_VOLTE_CALL);
        }

        LOGD(_TAG(TAG), "[onImsRegisterStateChange]: new Volte is \"Registered\", callState = %s, callMode = %s, " \
                "isStartActivateVolte = %s, mIsWaitVolteRegAfterImsCallEnd = %s", \
                ImsConnManUtils::getCallStateNameById(ImsConnManStateInfo::getCallState()), \
                ImsConnManUtils::getCallModeNameById(ImsConnManStateInfo::getCallMode()), \
                ImsConnManStateInfo::isStartActivateVolte() ? "true" : "false", \
                mIsWaitVolteRegAfterImsCallEnd ? "true" : "false");

        ImsConnManStateInfo::setStartActivateVolte(false);

        if (VOLTE_REG_STATE_REGISTERED == cpRegState) {
            mIsWaitVolteRegAfterImsCallEnd = false;
        }

        if (ImsConnManStateInfo::isVolteRegistered() == newVolteReg) {
            LOGD(_TAG(TAG), "[onImsRegisterStateChange]: old Volte \"Registered\" state is same as the new state");

            if (!mpPolicy->isExistAnyPolicyTimer()) {
                mpPolicy->createPolicyTimerTask(false, false);
            }
        } else {
            LOGD(_TAG(TAG), "[onImsRegisterStateChange]: old Volte \"Un-Registered\" state isn't same as the new state, invoke createPolicyTimerTask");

            ImsConnManStateInfo::setVolteRegistered(newVolteReg);
            mpPolicy->createPolicyTimerTask(false, false);
        }
    } else if (IMS_TYPE_VOWIFI == imsBearer) {
        bool newVowifiReg = true;
        ImsConnManStateInfo::setVolteRegistered(!newVowifiReg);

        LOGD(_TAG(TAG), "[onImsRegisterStateChange]: new Vowifi is \"Registered\", old Vowifi is %s, mCurPendingMsgId = %s", \
                ImsConnManStateInfo::isVowifiRegistered() ? "\"Registered\"" : "\"Un-Registered\"", \
                ImsConnManUtils::getMessageNameById(mPendingMsgId));

        if (!ImsConnManStateInfo::isIdleState()) {
            ImsConnManStateInfo::setCallMode(CALL_MODE_VOWIFI_CALL);
        }

        LOGD(_TAG(TAG), "[onImsRegisterStateChange]: new Vowifi is \"Registered\", callState = %s, callMode = %s, ", \
                ImsConnManUtils::getCallStateNameById(ImsConnManStateInfo::getCallState()), \
                ImsConnManUtils::getCallModeNameById(ImsConnManStateInfo::getCallMode()));

        if (ImsConnManStateInfo::isVowifiRegistered() == newVowifiReg) {
            LOGD(_TAG(TAG), "[onImsRegisterStateChange]: old Vowifi \"Registered\" state is same as the new state");

            if (!mpPolicy->isExistAnyPolicyTimer()) {
                mpPolicy->createPolicyTimerTask(false, false);
            }
        } else {
            LOGD(_TAG(TAG), "[onImsRegisterStateChange]: old Vowifi \"Un-Registered\" state isn't same as the new state, invoke createPolicyTimerTask");

            ImsConnManStateInfo::setVowifiRegistered(newVowifiReg);
            mpPolicy->createPolicyTimerTask(false, false);
            mpPolicy->triggerPunitiveLogic();
        }
    } else {
        if (IMS_TYPE_UNKNOWN == imsBearer
                && (VOLTE_REG_STATE_FAILED == cpRegState || VOLTE_REG_STATE_INACTIVE == cpRegState)) {
            bool isOldVolteRegistered = ImsConnManStateInfo::isVolteRegistered();
            bool isOldVowifiRegistered = ImsConnManStateInfo::isVowifiRegistered();

            LOGD(_TAG(TAG), "[onImsRegisterStateChange]: Both new Volte and new Vowifi are \"Un-Registered\", " \
                    "old Volte is %s, old Vowifi is %s, isStartActivateVolte = %s, mIsWaitVolteRegAfterImsCallEnd = %s, mCurPendingMsgId = %s", \
                    isOldVolteRegistered ? "\"Registered\"" : "\"Un-Registered\"", \
                    isOldVowifiRegistered ? "\"Registered\"" : "\"Un-Registered\"", \
                    ImsConnManStateInfo::isStartActivateVolte() ? "true" : "false", \
                    mIsWaitVolteRegAfterImsCallEnd ? "true" : "false", \
                    ImsConnManUtils::getMessageNameById(mPendingMsgId));

            ImsConnManStateInfo::setVolteRegistered(false);
            ImsConnManStateInfo::setVowifiRegistered(false);
            ImsConnManStateInfo::setStartActivateVolte(false);
            mIsWaitVolteRegAfterImsCallEnd = false;

            if (ImsConnManStateInfo::isLteCellularNetwork()) {
                if (!mpPolicy->isExistAnyPolicyTimer()) {
                    mpPolicy->createPolicyTimerTask(false, false);
                } else if (isOldVolteRegistered) {
                    mpPolicy->createPolicyTimerTask(false, false);
                }
            } else if (!ImsConnManStateInfo::isLteCellularNetwork() && !mpPolicy->isExistAnyPolicyTimer()) {
                mpPolicy->createPolicyTimerTask(false, false);
            }
        }
    }
}

void ImsConnManService::onCallStateChanged(int callState) {
    int imsBearer = ImsConnManUtils::getCurrentImsBearer();

    LOGD(_TAG(TAG), "[onCallStateChanged]: imsBearer = %s, callState = %s, callMode = %s, cpRegState = %s", \
            ImsConnManUtils::getImsBearerNameById(imsBearer), \
            ImsConnManUtils::getCallStateNameById(callState), \
            ImsConnManUtils::getCallModeNameById(ImsConnManStateInfo::getCallMode()), \
            ImsConnManUtils::getCpRegisterStateNameById(ImsConnManUtils::getCpRegisterState()));

    bool isCalling = false;
    ImsConnManStateInfo::setCallState(callState);
    if (CALL_STATE_IDLE != callState) {
        isCalling = true;
    }

    if (!isCalling) {
        if (CALL_MODE_CS_CALL == ImsConnManStateInfo::getCallMode()) {
            LOGD(_TAG(TAG), "[onCallStateChanged]: \"End CS Call\"");
            ImsConnManStateInfo::setCallMode(CALL_MODE_NO_CALL);

            if (isMeetInitiateVowifiConditions()) {
                mpPolicy->createPolicyTimerTask(false, false);
            }
        } else if (CALL_MODE_VOLTE_CALL == ImsConnManStateInfo::getCallMode()
                || CALL_MODE_VOWIFI_CALL == ImsConnManStateInfo::getCallMode()) {
            LOGD(_TAG(TAG), "[onCallStateChanged]: \"End ImsCall\" = %s", \
                    ImsConnManUtils::getCallModeNameById(ImsConnManStateInfo::getCallMode()));

            ImsConnManStateInfo::setCallMode(CALL_MODE_NO_CALL);
            ImsConnManStateInfo::setImsCallActive(false);
            mpPolicy->resetVolteSpecialLoopCount();

            mpPolicy->createPolicyTimerTask(false, false);
        }
    } else {
        if (IMS_TYPE_VOLTE == imsBearer && ImsConnManStateInfo::isVolteRegistered()) {
            LOGD(_TAG(TAG), "[onCallStateChanged]: \"Volte Call\" and %s", \
                    ImsConnManUtils::getImsCallTypeNameById(ImsConnManStateInfo::getImsCallType()));

            if (CALL_MODE_VOLTE_CALL != ImsConnManStateInfo::getCallMode()) {
                if (CALL_STATE_CONNECTED == callState) ImsConnManStateInfo::setImsCallActive(true);
                ImsConnManStateInfo::setCallMode(CALL_MODE_VOLTE_CALL);
                mpPolicy->createPolicyTimerTask(false, false);
            }
        } else if (IMS_TYPE_VOWIFI == imsBearer && ImsConnManStateInfo::isVowifiRegistered()) {
            LOGD(_TAG(TAG), "[onCallStateChanged]: \"Vowifi Call\" and %s", \
                    ImsConnManUtils::getImsCallTypeNameById(ImsConnManStateInfo::getImsCallType()));

            if (CALL_MODE_VOWIFI_CALL != ImsConnManStateInfo::getCallMode()) {
                if (CALL_STATE_CONNECTED == callState) ImsConnManStateInfo::setImsCallActive(true);
                ImsConnManStateInfo::setCallMode(CALL_MODE_VOWIFI_CALL);
                mpPolicy->createPolicyTimerTask(false, false);
            }
        } else if (IMS_TYPE_UNKNOWN == imsBearer
                && !ImsConnManStateInfo::isVolteRegistered()
                && !ImsConnManStateInfo::isVowifiRegistered()) {
            LOGD(_TAG(TAG), "[onCallStateChanged]: \"CS Call\"");
            ImsConnManStateInfo::setCallMode(CALL_MODE_CS_CALL);

            if (mIsMsgQueuePending && MSG_SWITCH_TO_VOWIFI == ImsConnManStateInfo::getPendingMessageId()) {
                ImsConnManController::cancelCurrentRequest();
            }
        } else {
            LOGE(_TAG(TAG), "[onCallStateChanged]: invalid call state, please check !!!!!");
            ImsConnManStateInfo::setCallMode(CALL_MODE_NO_CALL);
        }
    }
}

void ImsConnManService::onServiceStateChanged(string &newMobileNetworkType) {
    if (mMobileNetworkType == newMobileNetworkType) {
        LOGD(_TAG(TAG), "[onServiceStateChanged]: old MobileNetworkType is same as the new, " \
                "mMobileNetworkType = %s, newMobileNetworkType = %s", \
                mMobileNetworkType.c_str(), newMobileNetworkType.c_str());
    } else {
        LOGD(_TAG(TAG), "[onServiceStateChanged]: old MobileNetworkType isn't same as the new, " \
                "mMobileNetworkType = %s, newMobileNetworkType = %s", \
                mMobileNetworkType.c_str(), newMobileNetworkType.c_str());

        if (0 == newMobileNetworkType.compare("lte") && ImsConnManStateInfo::isLteCellularNetwork()) {
            mpPolicy->createPolicyTimerTask(false, false);
        } else {
            if (!mpPolicy->isExistAnyPolicyTimer() && !ImsConnManStateInfo::isVowifiRegistered()) {
                mpPolicy->createPolicyTimerTask(false, false);
            }
        }

        mMobileNetworkType = newMobileNetworkType;
    }
}

bool ImsConnManService::isMeetInitiateVowifiConditions() {
    return (ImsConnManStateInfo::isWifiCallingEnabled()
            && ImsConnManStateInfo::isWifiConnected()
            && !ImsConnManStateInfo::isStartActivateVolte()
            && CALL_MODE_CS_CALL != ImsConnManStateInfo::getCallMode());
}

void ImsConnManService::disableWfcOrWifiProcess(bool isDisableWifi) {
    if (ImsConnManStateInfo::isIdleState() && CALL_MODE_NO_CALL == ImsConnManStateInfo::getCallMode()) {
        mSwitchVowifiFailedCount = 0;
        mSwitchVowifiIpFailedCount = 0;

        if (isDisableWifi && mIsMsgQueuePending) {
            ImsConnManController::cancelCurrentRequest();
        } else if (ImsConnManStateInfo::isVowifiRegistered()) {
            ImsConnManController::vowifiUnavailable(ImsConnManStateInfo::isWifiConnected());
        }
    } else if (!ImsConnManStateInfo::isIdleState() && CALL_MODE_CS_CALL != ImsConnManStateInfo::getCallMode()) {
        if (ImsConnManStateInfo::isAirplaneModeEnabled() || !ImsConnManStateInfo::isLteCellularNetwork()) {
            if (CALL_MODE_VOWIFI_CALL == ImsConnManStateInfo::getCallMode()
                    || IMS_TYPE_VOWIFI == ImsConnManUtils::getCurrentUsedImsStack()) {
                ImsConnManController::hungUpImsCall(ImsConnManStateInfo::isWifiConnected());
                ImsConnManController::vowifiUnavailable(ImsConnManStateInfo::isWifiConnected());
            }
        } else if (ImsConnManStateInfo::isLteCellularNetwork()) {
            if (CALL_MODE_VOWIFI_CALL == ImsConnManStateInfo::getCallMode()
                    || (mIsMsgQueuePending && MSG_HANDOVER_TO_VOWIFI == ImsConnManStateInfo::getPendingMessageId())) {
                if (mIsVoltePdnDeactive) {
                    ImsConnManController::handoverToVolte(false);
                }
            }
        }
    }
}

void ImsConnManService::onWifiCallingChanged(bool isWfcEnabled) {
    LOGD(_TAG(TAG), "[onWifiCallingChanged]: \"Wifi-Calling\" database has been changed, isWfcEnabled = %s, mIsVoltePdnDeactive = %s", \
            isWfcEnabled ? "true" : "false", \
            mIsVoltePdnDeactive ? "true" : "false");

    if (isMeetInitiateVowifiConditions()) {
        mpPolicy->createPolicyTimerTask(false, true);
    } else if (!isWfcEnabled && ImsConnManStateInfo::isWifiConnected()) {
        disableWfcOrWifiProcess(false);
    }
}

void ImsConnManService::onWifiCallingModeChanged(int wfcMode) {
    LOGD(_TAG(TAG), "[onWifiCallingModeChanged]: \"Wifi-Calling mode\" database has been changed, wfcMode = %s", \
            ImsConnManUtils::getWfcModeNameById(wfcMode));

    if (!ImsConnManStateInfo::isStartActivateVolte()
            && ImsConnManStateInfo::isIdleState()
            && CALL_MODE_CS_CALL != ImsConnManStateInfo::getCallMode()
            && CALL_MODE_NO_CALL == ImsConnManStateInfo::getCallMode()) {
        mpPolicy->createPolicyTimerTask(false, true);
    }
}

void ImsConnManService::onWifiNetworkConnected() {
    LOGD(_TAG(TAG), "[onWifiNetworkConnected]: Wifi has been \"Connected\"");

    if (isMeetInitiateVowifiConditions()) {
        mpPolicy->createPolicyTimerTask(false, true);
    }
}

void ImsConnManService::onWifiNetworkDisconnected() {
    LOGD(_TAG(TAG), "[onWifiNetworkDisconnected]: Wifi has been \"Disconnected\", mIsVoltePdnDeactive = %s", \
            mIsVoltePdnDeactive ? "true" : "false");

    if (ImsConnManStateInfo::isWifiCallingEnabled()) {
        disableWfcOrWifiProcess(true);
    }
}

void RtpMessageHandler::handleMessage(const shared_ptr<Message> &msg) {
}

void PingAddressHandler::handleMessage(const shared_ptr<Message> &msg) {
    if (MSG_PING_PCSCF_ADDRESS == msg->what) {
        NetworkUtils::isPcscfAddressReachable(
            ImsConnManUtils::getCurrentLocalAddress(),
            ImsConnManUtils::getCurrentPcscfAddress());
    }
}

END_VOWIFI_NAMESPACE
