#include "ImsConnManTelephonyListener.h"
#include "Constants.h"
#include "ImsConnManUtils.h"
#include "ImsConnManStateInfo.h"
#include "nsITelephonyCallInfo.h"
#include "portinglayer/PortingConstants.h"
#include "nsServiceManagerUtils.h"



BEGIN_VOWIFI_NAMESPACE

const string ImsConnManTelephonyListener::TAG = "ImsConnManTelephonyListener";

NS_IMPL_ISUPPORTS(ImsConnManTelephonyListener, nsITelephonyListener)

ImsConnManTelephonyListener::ImsConnManTelephonyListener() {
    mTeleService = do_GetService(TELEPHONY_SERVICE_CONTRACTID);
    if (!mTeleService) {
        LOGE(_TAG(TAG), "Could not acquire nsITelephonyService !!!!!");
        return;
    }
}

ImsConnManTelephonyListener::~ImsConnManTelephonyListener() {
}

void ImsConnManTelephonyListener::update(shared_ptr<ImsConnManService> &service) {
    if (nullptr != service) {
        LOGD(_TAG(TAG), "update: ImsConnManService isn't nullptr");
        mpService = service;
    } else {
        LOGD(_TAG(TAG), "update: ImsConnManService is nullptr");
        if (nullptr != mpService) mpService.reset();
    }
}

void ImsConnManTelephonyListener::registerListener() {
    if (mTeleService)
        mTeleService->RegisterListener(this);
}

void ImsConnManTelephonyListener::unregisterListener() {
    if (mTeleService)
        mTeleService->UnregisterListener(this);
}

NS_IMETHODIMP
ImsConnManTelephonyListener::EnumerateCallStateComplete() {
    return NS_OK;
}

NS_IMETHODIMP
ImsConnManTelephonyListener::EnumerateCallState(nsITelephonyCallInfo *info) {
    LOGD(_TAG(TAG), "EnumerateCallState: check CallStateChanged()");
    return NS_OK;
}

NS_IMETHODIMP
ImsConnManTelephonyListener::CallStateChanged(uint32_t aLength, nsITelephonyCallInfo** aAllInfo) {
    LOGD(_TAG(TAG), "CallStateChanged: aLength = %d", aLength);

    for (uint32_t i = 0; i < aLength; ++i) {
        uint32_t callIndex;
        uint16_t callState;
        nsString callNumber;
        nsString disconnectedReason;
        bool isOutgoingCall;
        bool isConferenceCall;

        aAllInfo[i]->GetCallIndex(&callIndex);
        aAllInfo[i]->GetCallState(&callState);
        aAllInfo[i]->GetNumber(callNumber);
        aAllInfo[i]->GetDisconnectedReason(disconnectedReason);
        aAllInfo[i]->GetIsOutgoing(&isOutgoingCall);
        aAllInfo[i]->GetIsConference(&isConferenceCall);
        LOGD(_TAG(TAG), "CallStateChanged: callIndex = %d, callState = %s", \
                callIndex, ImsConnManUtils::getCallStateNameById(callState));
        LOGD(_TAG(TAG), "CallStateChanged: callNumber = %s, disconnectedReason = %s", \
                NS_ConvertUTF16toUTF8(callNumber).get(), \
                NS_ConvertUTF16toUTF8(disconnectedReason).get());
        LOGD(_TAG(TAG), "CallStateChanged: isOutgoingCall = %s, isConferenceCall = %s", \
                isOutgoingCall ? "true" : "false", \
                isConferenceCall ? "true" : "false");

        int callMode = CALL_MODE_NO_CALL;
        int imsBearer = ImsConnManUtils::getCurrentImsBearer();
        LOGD(_TAG(TAG), "CallStateChanged: imsBearer = %s", ImsConnManUtils::getImsBearerNameById(imsBearer));
        //Commenting below code for now ,because ImsCM failed to start audio call timer when call state changed to active in function onCallStateChanged(),
	//will delete the below code if not at all required after testing.

       /* if (IMS_TYPE_VOLTE == imsBearer) {
            if (CALL_STATE_IDLE != callState) {
                callMode = CALL_MODE_VOLTE_CALL;
            }
        } else if (IMS_TYPE_VOWIFI == imsBearer) {
            if (CALL_STATE_IDLE != callState) {
                callMode = CALL_MODE_VOWIFI_CALL;
            }
        } else if (IMS_TYPE_UNKNOWN == imsBearer) {
            if (CALL_STATE_IDLE != callState) {
                callMode = CALL_MODE_CS_CALL;
            }
        }

        ImsConnManStateInfo::setCallState(callState);
        ImsConnManStateInfo::setCallMode(callMode);
         */

        LOGD(_TAG(TAG), "CallStateChanged: callState = %s, callMode = %s", \
                ImsConnManUtils::getCallStateNameById(callState), \
                ImsConnManUtils::getCallModeNameById(callMode));

        if (nullptr != mpService) mpService->onCallStateChanged(callState);
    }

    return NS_OK;
}

NS_IMETHODIMP
ImsConnManTelephonyListener::SupplementaryServiceNotification(uint32_t aServiceId,
                                                              int32_t aCallIndex,
                                                              uint16_t aNotification) {
    return NS_OK;
}

NS_IMETHODIMP
ImsConnManTelephonyListener::NotifyCdmaCallWaiting(uint32_t aServiceId,
                                                   const nsAString& aNumber,
                                                   uint16_t aNumberPresentation,
                                                   const nsAString& aName,
                                                   uint16_t aNamePresentation) {
    return NS_OK;
}

NS_IMETHODIMP
ImsConnManTelephonyListener::NotifyConferenceError(const nsAString & name,
                                                   const nsAString & message) {
    return NS_OK;
}

NS_IMETHODIMP
ImsConnManTelephonyListener::NotifyRingbackTone(bool playRingbackTone) {
    return NS_OK;
}

NS_IMETHODIMP
ImsConnManTelephonyListener::NotifyTtyModeReceived(uint16_t mode) {
    return NS_OK;
}

NS_IMETHODIMP
ImsConnManTelephonyListener::NotifyTelephonyCoverageLosing(uint16_t type) {
    return NS_OK;
}

END_VOWIFI_NAMESPACE
