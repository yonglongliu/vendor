#ifndef __SPRD_VOWIFI_IMS_CONNMAN_INTERFACE_LAYER_H__
#define __SPRD_VOWIFI_IMS_CONNMAN_INTERFACE_LAYER_H__

#include "nsIImsConnManInterfaceLayer.h"
#include "mozilla/StaticPtr.h"


BEGIN_VOWIFI_NAMESPACE

class ImsConnManInterfaceLayer : public nsIImsConnManInterfaceLayer {
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIIMSCONNMANINTERFACELAYER

    explicit ImsConnManInterfaceLayer();

    static already_AddRefed<ImsConnManInterfaceLayer> GetInstance();

private:
    virtual ~ImsConnManInterfaceLayer();

private:
    static const string TAG;

    static StaticRefPtr<ImsConnManInterfaceLayer> sSingleton;
};

END_VOWIFI_NAMESPACE

#endif
