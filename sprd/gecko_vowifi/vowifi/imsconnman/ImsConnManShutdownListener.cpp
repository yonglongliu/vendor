#include "ImsConnManShutdownListener.h"


BEGIN_VOWIFI_NAMESPACE

const string ImsConnManShutdownListener::TAG = "ImsConnManShutdownListener";

NS_IMPL_ISUPPORTS(ImsConnManShutdownListener, nsIObserver)

ImsConnManShutdownListener::ImsConnManShutdownListener() {
    mObserverService = mozilla::services::GetObserverService();
    if (!mObserverService) {
        LOGE(_TAG(TAG), "Could not acquire nsIObserverService !!!!!");
        return;
    }
}

ImsConnManShutdownListener::~ImsConnManShutdownListener() {
}

void ImsConnManShutdownListener::registerListener() {
    if (mObserverService) {
        mObserverService->AddObserver(this, NS_XPCOM_WILL_SHUTDOWN_OBSERVER_ID, false);
    }
}

void ImsConnManShutdownListener::unregisterListener() {
    if (mObserverService)
        mObserverService->RemoveObserver(this, NS_XPCOM_WILL_SHUTDOWN_OBSERVER_ID);
}

NS_IMETHODIMP
ImsConnManShutdownListener::Observe(nsISupports *aSubject, const char * aTopic, const char16_t * aData) {
    LOGD(_TAG(TAG), "Observe: Shutdown device..... %s", aTopic);

    if (0 == strcmp(aTopic, NS_XPCOM_WILL_SHUTDOWN_OBSERVER_ID)) {
        LOGD(_TAG(TAG), "Observe: Shutdown device.....");
    }

    return NS_OK;
}

END_VOWIFI_NAMESPACE
