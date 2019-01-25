#ifndef __SPRD_VOWIFI_IMS_CONNMAN_ICC_LISTENER_H__
#define __SPRD_VOWIFI_IMS_CONNMAN_ICC_LISTENER_H__

#include "nsIIccService.h"
#include "nsServiceManagerUtils.h"
#include <string>

using std::string;


BEGIN_VOWIFI_NAMESPACE

class ImsConnManIccListener : public nsIIccListener {
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIICCLISTENER

    explicit ImsConnManIccListener();

    void registerListener();
    void unregisterListener();

private:
    virtual ~ImsConnManIccListener();

private:
    static const string TAG;

    nsCOMPtr<nsIIcc> mIcc;
};

END_VOWIFI_NAMESPACE

#endif
