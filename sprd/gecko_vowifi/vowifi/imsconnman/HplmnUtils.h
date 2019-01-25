#ifndef __SPRD_VOWIFI_IMS_CONNMAN_HPLMN_UTILS_H__
#define __SPRD_VOWIFI_IMS_CONNMAN_HPLMN_UTILS_H__

#include "portinglayer/PortingMacro.h"
#include <string>

using std::string;


BEGIN_VOWIFI_NAMESPACE

class HplmnUtils {
public:
    explicit HplmnUtils();
    virtual ~HplmnUtils();

    static string getHplmn(int phoneId);

    /**
     * Return current operator id for new different Policy class.
     */
    static int currentPolicyOperatorId();

    static void resetPolicyOperatorId();

private:
    /**
     * Used for adapting operator id by hplmn for new different Policy class.
     */
    static int adaptPolicyOperatorId(string& hplmn);

private:
    static const int VALID_IMSI_MCC = 200;
    static const string TAG;

    static int mPolicyOperatorId;
};

END_VOWIFI_NAMESPACE

#endif
