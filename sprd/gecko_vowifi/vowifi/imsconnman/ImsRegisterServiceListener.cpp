#include "ImsRegisterServiceListener.h"
#include "nsString.h"
#include "nsIImsServiceCallback.h"
#include "nsServiceManagerUtils.h"


BEGIN_VOWIFI_NAMESPACE

const string ImsRegisterServiceListener::TAG = "ImsRegisterServiceListener";

NS_IMPL_ISUPPORTS(ImsRegisterServiceListener, nsIImsRegListener)

ImsRegisterServiceListener::ImsRegisterServiceListener() {
    nsCOMPtr<nsIImsRegService> imsService = do_GetService(IMS_REG_SERVICE_CONTRACTID);
    if (!imsService) {
        LOGE(_TAG(TAG), "Could not acquire nsIImsRegService !!!!!");
        return;
    }

    imsService->GetHandlerByServiceId(PortingInterface::getDefaultDataPhoneId(), getter_AddRefs(mImsRegHandler));
    if (!mImsRegHandler) {
        LOGE(_TAG(TAG), "Could not acquire nsIImsRegHandler !!!!!");
        return;
    }
}

ImsRegisterServiceListener::~ImsRegisterServiceListener() {
}

void ImsRegisterServiceListener::update(shared_ptr<ImsConnManService> &service) {
    if (nullptr != service) {
        LOGD(_TAG(TAG), "update: ImsConnManService isn't nullptr");
        mpService = service;
    } else {
        LOGD(_TAG(TAG), "update: ImsConnManService is nullptr");
        if (nullptr != mpService) mpService.reset();
    }
}

void ImsRegisterServiceListener::registerListener() {
    if (mImsRegHandler)
        mImsRegHandler->RegisterListener(this);
}

void ImsRegisterServiceListener::unregisterListener() {
    if (mImsRegHandler)
        mImsRegHandler->UnregisterListener(this);
}

NS_IMETHODIMP ImsRegisterServiceListener::NotifyEnabledStateChanged(bool aEnabled) {
    return NS_OK;
}

NS_IMETHODIMP ImsRegisterServiceListener::NotifyPreferredProfileChanged(uint16_t aProfile) {
    return NS_OK;
}

NS_IMETHODIMP ImsRegisterServiceListener::NotifyCapabilityChanged(int16_t aCapability,
                                                                  const nsAString & aUnregisteredReason) {
    return NS_OK;
}

END_VOWIFI_NAMESPACE
