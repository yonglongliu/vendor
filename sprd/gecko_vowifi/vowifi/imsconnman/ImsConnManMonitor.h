#ifndef __SPRD_VOWIFI_IMS_CONNMAN_MONITOR_H__
#define __SPRD_VOWIFI_IMS_CONNMAN_MONITOR_H__

#include "ImsConnManService.h"
#include "ImsConnManIccListener.h"
#include "ImsConnManShutdownListener.h"
#include "ImsConnManTelephonyListener.h"
#include "ImsServiceCallbackListener.h"
#include "ImsRegisterServiceListener.h"
#include "portinglayer/PortingConstants.h"
#include <string>

using std::string;
using std::shared_ptr;


BEGIN_VOWIFI_NAMESPACE

class ImsConnManMonitor {
public:
    static ImsConnManMonitor& getInstance();

    virtual ~ImsConnManMonitor();

    void onInit();
    void updateSimCardState(int simCardState, int slotId = PRIMARY_PHONE_ID);
    void onBindOperatorService();

private:
    explicit ImsConnManMonitor();

    void onPrepareBindOperatorService(int phoneId);
    void onUnbindOperatorService();

private:
    static const string TAG;

    shared_ptr<ImsConnManService> mpService;
    ImsConnManIccListener *mpIccListener;
    ImsConnManShutdownListener *mpShutdownListener;
    ImsConnManTelephonyListener *mpTeleListener;
    ImsServiceCallbackListener *mpImsServiceListener;
    ImsRegisterServiceListener *mpImsRegServiceListener;

    int mPrimaryPhoneId;
    string mPrimaryHplmn;

    int mOldPrimarySimState;
    int mNewPrimarySimState;
};

END_VOWIFI_NAMESPACE

#endif
