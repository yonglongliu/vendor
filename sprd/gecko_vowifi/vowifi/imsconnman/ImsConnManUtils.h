#ifndef __SPRD_VOWIFI_IMS_CONNMAN_UTILS_H__
#define __SPRD_VOWIFI_IMS_CONNMAN_UTILS_H__

#include "portinglayer/PortingMacro.h"
#include <string>

using std::string;


BEGIN_VOWIFI_NAMESPACE

class ImsConnManUtils {
public:
    static void setRequestId(int requestId);
    static int getRequestId();

    static int switchToVowifi();
    static int handoverToVowifi();
    static int handoverToVolte();
    static int releaseVoWifiResource();
    static int setVowifiUnavailable(bool isWifiConnected, bool isOnlySendAT);
    static int cancelCurrentRequest();
    static void hungUpImsCall(bool isWifiConnected);
    static void showSwitchVowifiFailedNotification();

    static int getCurrentImsBearer();       /*IMS_REGISTER_TYPE*/
    static int getCurrentUsedImsStack();    /*IMS_REGISTER_TYPE*/
    static int getCpRegisterState();        /*VOLTE_REGISTER_STATE*/
    static int getCurrentImsCallType();     /*IMS_CALL_TYPE*/

    static string getCurrentPcscfAddress();
    static string getCurrentLocalAddress();

    static bool isVowifiLabSimEnable();
    static bool isVowifiWhitelistEnable();

    static char* getWfcModeNameById(int wfcMode);
    static char* getMessageNameById(int messageId);
    static char* getOperationTypeNameById(int type);
    static char* getCpRegisterStateNameById(int regState);
    static char* getImsBearerNameById(int imsBearer);
    static char* getImsStackNameById(int imsStack);
    static char* getCallStateNameById(int callState);
    static char* getCallModeNameById(int callMode);
    static char* getImsCallTypeNameById(int imsCallType);

private:
    static const string TAG;

    static int mRequestId;

};

END_VOWIFI_NAMESPACE

#endif
