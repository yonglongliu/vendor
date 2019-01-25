#ifndef __SPRD_VOWIFI_IMS_CONNMAN_CONTROLLER_H__
#define __SPRD_VOWIFI_IMS_CONNMAN_CONTROLLER_H__

#include <string>

using std::string;


BEGIN_VOWIFI_NAMESPACE

class ImsConnManController {
public:
    static void switchOrHandoverVowifi(bool isInitiateVowifiAfterLte);
    static void forcedlyHandoverToVolte();
    static void handoverToVolte(bool isByTimer);
    static void releaseVoWifiResource();
    static void vowifiUnavailable(bool isWifiConnected);
    static void cancelCurrentRequest();
    static void showSwitchVowifiFailedNotification();
    static void hungUpImsCall(bool isWifiConnected);

private:
    friend class ImsConnManService;

private:
    static const string TAG;
    static shared_ptr<MessageHandler> mpHandler;
};

END_VOWIFI_NAMESPACE

#endif
