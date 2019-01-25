#include "ImsConnManDefaultPolicy.h"


BEGIN_VOWIFI_NAMESPACE

ImsConnManDefaultPolicy::ImsConnManDefaultPolicy() {
    TAG = "ImsConnManDefaultPolicy";

    //mIsPopupErrorNotification = true;
    mIsPopupErrorNotification = false;
    mPopupErrorRetryCount = 3;

    mIsInitiateVowifiAfterLte = false;

    mLteRsrpLow = -115;
    mLteRsrpHigh = -106;

    mWifiIdleRssiLow = -82;
    mWifiIdleRssiHigh = -75;
    mWifiCallRssiLow = -75;
    mWifiCallRssiHigh = -70;

    mQosAudioLossMedium = 30;
    mQosAudioJitterMedium = 200;
    mQosAudioRttMedium = 2000;
    mQosVideoLossMedium = 20;
    mQosVideoJitterMedium = 200;
    mQosVideoRttMedium = 2000;

    mQosAudioLossLow = 40;
    mQosAudioJitterLow = 300;
    mQosAudioRttLow = 3000;
    mQosVideoLossLow = 30;
    mQosVideoJitterLow = 300;
    mQosVideoRttLow = 3000;

    mVowifiThresholdLoopCount = 7;
    mVowifiQosLoopCount = 7;
    mVolteThresholdLoopCount = 15;
    mVolteSpecialLoopCount = mVolteThresholdLoopCount;

    mIsQuicklyCalculateLogic = true;
    mQuicklyCalculateCount = 1;

    mIsExactlyCalculateLogic = true;
    mExactlyCalculateLastCount = 2;

    mIsPunitiveLogic = true;
    mPunitiveIntervalTime = 5;
    mPunitiveMaxTime = 30;
}

ImsConnManDefaultPolicy::~ImsConnManDefaultPolicy() {
}

END_VOWIFI_NAMESPACE