#ifndef __SPRD_VOWIFI_IMS_CONNMAN_DEFAULT_POLICY_H__
#define __SPRD_VOWIFI_IMS_CONNMAN_DEFAULT_POLICY_H__

#include "ImsConnManPolicy.h"


BEGIN_VOWIFI_NAMESPACE

class ImsConnManDefaultPolicy : public ImsConnManPolicy {
public:
    explicit ImsConnManDefaultPolicy();
    virtual ~ImsConnManDefaultPolicy();
};

END_VOWIFI_NAMESPACE

#endif
