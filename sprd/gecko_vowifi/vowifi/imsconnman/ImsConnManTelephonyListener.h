#ifndef __SPRD_VOWIFI_IMS_CONNMAN_TELEPHONY_LISTENER_H__
#define __SPRD_VOWIFI_IMS_CONNMAN_TELEPHONY_LISTENER_H__

#include "nsITelephonyService.h"
#include <string>
#include "ImsConnManService.h"
#include "portinglayer/PortingMacro.h"


using std::string;
using std::shared_ptr;


BEGIN_VOWIFI_NAMESPACE

class ImsConnManTelephonyListener : public nsITelephonyListener {
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSITELEPHONYLISTENER

    explicit ImsConnManTelephonyListener();

    void update(shared_ptr<ImsConnManService> &service);

    void registerListener();
    void unregisterListener();

private:
    virtual ~ImsConnManTelephonyListener();

private:
    static const string TAG;

    shared_ptr<ImsConnManService> mpService;
    nsCOMPtr<nsITelephonyService> mTeleService;
};

END_VOWIFI_NAMESPACE

#endif
