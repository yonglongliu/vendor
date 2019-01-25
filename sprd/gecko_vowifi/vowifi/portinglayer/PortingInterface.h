#ifndef __SPRD_VOWIFI_PORTING_INTERFACE_H__
#define __SPRD_VOWIFI_PORTING_INTERFACE_H__

#include "PortingConstants.h"
#include "nsITimer.h"
#include <string>

using std::string;


BEGIN_VOWIFI_NAMESPACE

class PortingInterface {
public:
    static int getDefaultDataPhoneId();
    static bool hasIccCard(int phoneId = PRIMARY_PHONE_ID);
    static int getIccCardState(int phoneId = PRIMARY_PHONE_ID);
    static int getIccCardType(int phoneId = PRIMARY_PHONE_ID);
    static string getIsimImpi(int phoneId = PRIMARY_PHONE_ID);
    static string getSimOperatorNumeric(int phoneId = PRIMARY_PHONE_ID);
    static int getLteRsrp();
    static bool isLteMobileNetwork(int phoneId = PRIMARY_PHONE_ID);
    static string getMobileNetworkType(int phoneId = PRIMARY_PHONE_ID);
    static int getWifiRssi();
    static bool isWifiConnected();
    static void setAirplaneModeEnabled(bool isEnabled);
    static bool isAirplaneModeEnabled();

    static void setImsServiceCallbackListener(nsIImsServiceCallback *listener);
    static int switchToVowifi();
    static int handoverToVowifi();
    static int handoverToVolte();
    static int releaseVoWifiResource();
    static int setVowifiUnavailable(int wifiConnectState, bool isOnlySendAT);
    static int cancelCurrentRequest();
    static void hungUpImsCall(int wifiConnectState);
    static void showSwitchVowifiFailedNotification();

    static int getCurrentImsBearer();
    static int getCurrentUsedImsStack();
    static int getCpRegisterState();
    static int getCurrentImsCallType();

    static string getCurrentPcscfAddress();
    static string getCurrentLocalAddress();

    static already_AddRefed<nsITimer> createTimer(nsTimerCallbackFunc callback,
                                                  void *arg,
                                                  unsigned long intervalTime,
                                                  unsigned long timerType);

private:
    static const string TAG;
    static bool mIsAirplaneModeEnabled;
};

END_VOWIFI_NAMESPACE

#endif
