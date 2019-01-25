#ifndef __SPRD_VOWIFI_IMS_CONNMAN_SHUTDOWN_LISTENER_H__
#define __SPRD_VOWIFI_IMS_CONNMAN_SHUTDOWN_LISTENER_H__

#include <string>

using std::string;


BEGIN_VOWIFI_NAMESPACE

class ImsConnManShutdownListener : public nsIObserver {
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIOBSERVER

    explicit ImsConnManShutdownListener();

    void registerListener();
    void unregisterListener();

private:
    virtual ~ImsConnManShutdownListener();

private:
    static const string TAG;

    nsCOMPtr<nsIObserverService> mObserverService;
};

END_VOWIFI_NAMESPACE

#endif
