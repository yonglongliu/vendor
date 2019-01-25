#ifndef __SPRD_VOWIFI_IMS_CONNMAN_POLICY_H__
#define __SPRD_VOWIFI_IMS_CONNMAN_POLICY_H__

#include "portinglayer/PortingMacro.h"
#include "nsITimer.h"
#include "nsCOMPtr.h"
#include <string>
#include <vector>

using std::string;
using std::vector;


BEGIN_VOWIFI_NAMESPACE

class ImsConnManPolicy {
public:
    explicit ImsConnManPolicy();
    virtual ~ImsConnManPolicy();

    void releaseAllTimer();

    /**
     * Used for create all kinds of policy Timer task.
     *
     * @param isOperationFailed
     * true: Telephony feedback operation failed in Idle state.
     * false: Telephony feedback operation successful in Idle state or other states.
     *
     * @param isQuicklyInitiate
     * true: Existence quickly initiate vowifi logic.
     * false: Inexistence quickly initiate vowifi logic.
     *
     */
    void createPolicyTimerTask(bool isIdleOperationFailed, bool isQuicklyInitiate);

    void triggerPunitiveLogic();
    void resetVolteSpecialLoopCount();

    bool isPopupErrorNotification();
    int getPopupErrorRetryCount();

    bool isExistAnyPolicyTimer();

private:
    int getAverageExcludeMaxMin(vector<int> vectorValue);
    void calculateSignalAverage();
    void calculateAudioQosAverage();
    void calculateVideoQosAverage();

    bool isExactlyCalculateIdleThreshold(int wifiCallingMode);
    int compareIdleThresholdForWifiPreferred(int rssi, int rsrp);
    void loopIdleThresholdForWifiPreferred(int rssi, int rsrp);
    int compareIdleThresholdForCellularPreferred(int rssi, int rsrp);
    void loopIdleThresholdForCellularPreferred(int rssi, int rsrp);
    void loopIdleThreshold(int rssi, int rsrp);
    void createIdleThresholdTimerTask();
    friend void createIdleThresholdTimerTaskCallback(nsITimer *aTimer, void *aClosure);

    bool isExactlyCalculateIdleThresholdWithoutIms(int wifiCallingMode);
    int compareIdleThresholdWithoutImsForWifiPreferred(int rssi);
    void loopIdleThresholdWithoutImsForWifiPreferred(int rssi);
    int compareIdleThresholdWithoutImsForCellularPreferred(int rssi, int rsrp);
    void loopIdleThresholdWithoutImsForCellularPreferred(int rssi, int rsrp);
    void loopIdleThresholdWithoutIms(int rssi, int rsrp);
    void createIdleThresholdTimerTaskWithoutIms();
    friend void createIdleThresholdTimerTaskWithoutImsCallback(nsITimer *aTimer, void *aClosure);
    void createIdleThresholdTimerTaskWithoutLte();
    friend void createIdleThresholdTimerTaskWithoutLteCallback(nsITimer *aTimer, void *aClosure);

    void createIdleThresholdTimerTaskWifiOnly();
    friend void createIdleThresholdTimerTaskWifiOnlyCallback(nsITimer *aTimer, void *aClosure);

    bool isExactlyCalculateImsCallThreshold(int wifiCallingMode, int imsCallType);
    int compareAudioCallThresholdForWifiPreferred(int rssi, int rsrp);
    void loopAudioCallThresholdForWifiPreferred(int rssi, int rsrp);
    int compareAudioCallThresholdForCellularPreferred(int rssi, int rsrp);
    void loopAudioCallThresholdForCellularPreferred(int rssi, int rsrp);
    void loopAudioCallThreshold(int rssi, int rsrp);
    void createAudioCallThresholdTimerTask();
    friend void createAudioCallThresholdTimerTaskCallback(nsITimer *aTimer, void *aClosure);

    int compareVideoCallThresholdForWifiPreferred(int rssi, int rsrp);
    void loopVideoCallThresholdForWifiPreferred(int rssi, int rsrp);
    int compareVideoCallThresholdForCellularPreferred(int rssi, int rsrp);
    void loopVideoCallThresholdForCellularPreferred(int rssi, int rsrp);
    void loopVideoCallThreshold(int rssi, int rsrp);
    void createVideoCallThresholdTimerTask();
    friend void createVideoCallThresholdTimerTaskCallback(nsITimer *aTimer, void *aClosure);

protected:
    string TAG;

    bool mIsPopupErrorNotification;
    uint32_t mPopupErrorRetryCount;

    bool mIsInitiateVowifiAfterLte;

    int mLteRsrpLow;
    int mLteRsrpHigh;

    int mWifiIdleRssiLow;
    int mWifiIdleRssiHigh;
    int mWifiCallRssiLow;
    int mWifiCallRssiHigh;

    int mQosAudioLossMedium;
    int mQosAudioJitterMedium;
    int mQosAudioRttMedium;
    int mQosVideoLossMedium;
    int mQosVideoJitterMedium;
    int mQosVideoRttMedium;

    int mQosAudioLossLow;
    int mQosAudioJitterLow;
    int mQosAudioRttLow;
    int mQosVideoLossLow;
    int mQosVideoJitterLow;
    int mQosVideoRttLow;

    uint32_t mVowifiThresholdLoopCount;
    uint32_t mVowifiQosLoopCount;
    uint32_t mVolteThresholdLoopCount;
    uint32_t mVolteSpecialLoopCount;

    bool mIsQuicklyCalculateLogic;
    uint32_t mQuicklyCalculateCount;

    bool mIsExactlyCalculateLogic;
    uint32_t mExactlyCalculateLastCount;

    bool mIsPunitiveLogic;
    uint32_t mPunitiveIntervalTime;
    uint32_t mPunitiveMaxTime;

private:
    int mWifiRssiAverage;
    int mLteRsrpAverage;
    vector<int> mVectorWifiRssi;
    vector<int> mVectorLteRsrp;

    int mAudioQosLossAverage;
    int mAudioQosJitterAverage;
    int mAudioQosRttAverage;
    vector<int> mVectorAudioQosLoss;
    vector<int> mVectorAudioQosJitter;
    vector<int> mVectorAudioQosRtt;

    int mVideoQosLossAverage;
    int mVideoQosJitterAverage;
    int mVideoQosRttAverage;
    vector<int> mVectorVideoQosLoss;
    vector<int> mVectorVideoQosJitter;
    vector<int> mVectorVideoQosRtt;

    nsCOMPtr<nsITimer> mIdleThresholdTimer;
    nsCOMPtr<nsITimer> mAudioCallThresholdTimer;
    nsCOMPtr<nsITimer> mVideoCallThresholdTimer;
    bool mIsExistAnyPolicyTimer;
    int mCurrentLoopCount;
    int mTotalLoopCount;
};

END_VOWIFI_NAMESPACE

#endif
