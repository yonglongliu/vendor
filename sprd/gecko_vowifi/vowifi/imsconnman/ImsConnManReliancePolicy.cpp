#include "ImsConnManReliancePolicy.h"


BEGIN_VOWIFI_NAMESPACE

ImsConnManReliancePolicy::ImsConnManReliancePolicy() {
    TAG = "ImsConnManReliancePolicy";

    mIsPopupErrorNotification = false;
    mPopupErrorRetryCount = 3;

    mIsInitiateVowifiAfterLte = false;

    mLteRsrpLow = -115;
    mLteRsrpHigh = -106;

    mWifiIdleRssiLow = -82;
    mWifiIdleRssiHigh = -75;
    mWifiCallRssiLow = -75;
    mWifiCallRssiHigh = -70;

    mQosAudioLossMedium = 35;
    mQosAudioJitterMedium = 250;
    mQosAudioRttMedium = 2500;
    mQosVideoLossMedium = 30;
    mQosVideoJitterMedium = 250;
    mQosVideoRttMedium = 2500;

    mQosAudioLossLow = 45;
    mQosAudioJitterLow = 350;
    mQosAudioRttLow = 3500;
    mQosVideoLossLow = 40;
    mQosVideoJitterLow = 350;
    mQosVideoRttLow = 3500;

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

ImsConnManReliancePolicy::~ImsConnManReliancePolicy() {
}

END_VOWIFI_NAMESPACE
