#include "ImsConnManIccListener.h"
#include "ImsConnManMonitor.h"


BEGIN_VOWIFI_NAMESPACE

const string ImsConnManIccListener::TAG = "ImsConnManIccListener";

NS_IMPL_ISUPPORTS(ImsConnManIccListener, nsIIccListener)

ImsConnManIccListener::ImsConnManIccListener() {
    nsCOMPtr<nsIIccService> iccService = do_GetService(ICC_SERVICE_CONTRACTID);
    if (!iccService) {
        LOGE(_TAG(TAG), "Could not acquire nsIIccService !!!!!");
        return;
    }

    iccService->GetIccByServiceId(PortingInterface::getDefaultDataPhoneId(), getter_AddRefs(mIcc));
    if (!mIcc) {
        LOGE(_TAG(TAG), "Could not acquire nsIIcc !!!!!");
        return;
    }
}

ImsConnManIccListener::~ImsConnManIccListener() {
}

void ImsConnManIccListener::registerListener() {
    if (mIcc)
        mIcc->RegisterListener(this);
}

void ImsConnManIccListener::unregisterListener() {
    if (mIcc)
        mIcc->UnregisterListener(this);
}

NS_IMETHODIMP ImsConnManIccListener::NotifyStkCommand(nsIStkProactiveCmd *aStkProactiveCmd) {
    return NS_OK;
}

NS_IMETHODIMP ImsConnManIccListener::NotifyStkSessionEnd() {
    return NS_OK;
}

NS_IMETHODIMP ImsConnManIccListener::NotifyCardStateChanged() {
    int iccCardState = PortingInterface::getIccCardState();
    LOGD(_TAG(TAG), "NotifyCardStateChanged: Icc Card State: %d", iccCardState);

    if (ImsConnManStateInfo::isReadAirplaneMode()) {
        ImsConnManMonitor::getInstance().updateSimCardState(iccCardState);
    }

    return NS_OK;
}

NS_IMETHODIMP ImsConnManIccListener::NotifyIccInfoChanged() {
    int iccCardState = PortingInterface::getIccCardState();
    LOGD(_TAG(TAG), "NotifyIccInfoChanged: Icc Card State: %d", iccCardState);

    if (SIM_STATE_READY == iccCardState) {
        ImsConnManStateInfo::setReceivedIccInfoChangedState(true);
    }

    if (ImsConnManStateInfo::isReadAirplaneMode()) {
        ImsConnManMonitor::getInstance().updateSimCardState(iccCardState);
    }

    return NS_OK;
}

NS_IMETHODIMP ImsConnManIccListener::NotifyIsimInfoChanged() {
    int iccCardState = PortingInterface::getIccCardState();
    LOGD(_TAG(TAG), "NotifyIsimInfoChanged: Icc Card State: %d", iccCardState);

    if (SIM_STATE_READY == iccCardState) {
        ImsConnManStateInfo::setReceivedIccInfoChangedState(true);
    }

    if (ImsConnManStateInfo::isReadAirplaneMode()) {
        ImsConnManMonitor::getInstance().updateSimCardState(iccCardState);
    }

    return NS_OK;
}

END_VOWIFI_NAMESPACE
