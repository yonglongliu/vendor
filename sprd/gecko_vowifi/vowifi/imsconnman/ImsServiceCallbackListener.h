#ifndef __SPRD_VOWIFI_IMS_SERVICE_CALLBACK_LISTENER_H__
#define __SPRD_VOWIFI_IMS_SERVICE_CALLBACK_LISTENER_H__

#include "ImsConnManService.h"
#include "nsIImsServiceCallback.h"
#include <string>

using std::string;
using std::shared_ptr;


BEGIN_VOWIFI_NAMESPACE

class ImsServiceCallbackListener : public nsIImsServiceCallback {
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIIMSSERVICECALLBACK

    explicit ImsServiceCallbackListener();

    void update(shared_ptr<ImsConnManService> &service);

private:
    virtual ~ImsServiceCallbackListener();

private:
    static const string TAG;

    shared_ptr<ImsConnManService> mpService;
};

END_VOWIFI_NAMESPACE

#endif