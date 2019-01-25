#ifndef __SPRD_VOWIFI_IMS_REGISTER_SERVICE_LISTENER_H__
#define __SPRD_VOWIFI_IMS_REGISTER_SERVICE_LISTENER_H__

#include "ImsConnManService.h"
#include "nsIImsRegService.h"
#include <string>

using std::string;
using std::shared_ptr;


BEGIN_VOWIFI_NAMESPACE

class ImsRegisterServiceListener : public nsIImsRegListener {
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIIMSREGLISTENER

    explicit ImsRegisterServiceListener();

    void update(shared_ptr<ImsConnManService> &service);

    void registerListener();
    void unregisterListener();

private:
    virtual ~ImsRegisterServiceListener();

private:
    static const string TAG;

    shared_ptr<ImsConnManService> mpService;
    nsCOMPtr<nsIImsRegHandler> mImsRegHandler;
};

END_VOWIFI_NAMESPACE

#endif