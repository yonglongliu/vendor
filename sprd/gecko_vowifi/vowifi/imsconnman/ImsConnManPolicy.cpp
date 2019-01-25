#include "ImsConnManPolicy.h"
#include "portinglayer/PortingLog.h"
#include <stdlib.h>


BEGIN_VOWIFI_NAMESPACE

static const bool DEBUG = false;
static const unsigned long TIMER_INTERVAL = 1000;

enum {
    NOT_HANDOVER,
    VOLTE_TO_VOWIFI,
    VOWIFI_TO_VOLTE,
    AUTO_ATTACH_VOWIFI,
};

ImsConnManPolicy::ImsConnManPolicy() {
    mWifiRssiAverage = WIFI_SIGNAL_STRENGTH_INVALID;
    mLteRsrpAverage = LTE_SIGNAL_STRENGTH_INVALID;
    mAudioQosLossAverage = 0;
    mAudioQosJitterAverage = 0;
    mAudioQosRttAverage = 0;
    mVideoQosLossAverage = 0;
    mVideoQosJitterAverage = 0;
    mVideoQosRttAverage = 0;

    mIdleThresholdTimer = nullptr;
    mAudioCallThresholdTimer = nullptr;
    mVideoCallThresholdTimer = nullptr;
    mIsExistAnyPolicyTimer = false;
    mCurrentLoopCount = 0;
    mTotalLoopCount = 0;
}

ImsConnManPolicy::~ImsConnManPolicy() {
}

int ImsConnManPolicy::getAverageExcludeMaxMin(vector<int> vectorValue) {
    int invalidValue = 0xFFFFFFFF;

    if (vectorValue.empty()) {
        LOGE(_TAG(TAG), "getAverageExcludeMaxMin: vector is empty, please check !!!!!");
        return invalidValue;
    }

    int total = 0;
    int average = 0;
    int index = 0, maxIndex = 0, minIndex = 0;
    int vectorSize = static_cast<int>(vectorValue.size());
    int maxValue = vectorValue.front();
    int minValue = vectorValue.front();

    for (index = 0; index < vectorSize; index++) {
        if (DEBUG) {
            LOGD(_TAG(TAG), "getAverageExcludeMaxMin: vectorValue[%d] = %d", index, vectorValue.at(index));
        }

        if (maxValue < vectorValue.at(index)) {
            maxIndex = index;
            maxValue = vectorValue.at(index);
        }

        if (minValue > vectorValue.at(index)) {
            minIndex = index;
            minValue = vectorValue.at(index);
        }
    }

    if (DEBUG) {
        LOGD(_TAG(TAG), "getAverageExcludeMaxMin: maxIndex = %d, maxValue = %d, minIndex = %d, minValue = %d", \
                maxIndex, maxValue, minIndex, minValue);
    }

    for (index = 0; index < vectorSize; index++) {
        if (vectorSize > static_cast<int>(mExactlyCalculateLastCount)) {
            if (index != maxIndex && index != minIndex) {
                total += vectorValue.at(index);
            }
        } else {
            total += vectorValue.at(index);
        }
    }

    if (vectorSize > static_cast<int>(mExactlyCalculateLastCount)) {
        if (maxIndex == minIndex) {
            average = total / (vectorSize - 1);
        } else {
            average = total / (vectorSize - 2);
        }
    } else {
        average = total / vectorSize;
    }

    if (DEBUG) {
        LOGD(_TAG(TAG), "getAverageExcludeMaxMin: average = %d", average);
    }

    return average;
}

void ImsConnManPolicy::calculateSignalAverage() {
    mLteRsrpAverage = LTE_SIGNAL_STRENGTH_INVALID;
    mWifiRssiAverage = WIFI_SIGNAL_STRENGTH_INVALID;

    if (!mVectorLteRsrp.empty()) {
        mLteRsrpAverage = getAverageExcludeMaxMin(mVectorLteRsrp);
    } else {
        LOGE(_TAG(TAG), "calculateSignalAverage: mVectorLteRsrp is empty, please check !!!!!");
    }

    if (!mVectorWifiRssi.empty()) {
        mWifiRssiAverage = getAverageExcludeMaxMin(mVectorWifiRssi);
    } else {
        LOGE(_TAG(TAG), "calculateSignalAverage: mVectorWifiRssi is empty, please check !!!!!");
    }
}

void ImsConnManPolicy::calculateAudioQosAverage() {
    mAudioQosLossAverage = 0;
    mAudioQosJitterAverage = 0;
    mAudioQosRttAverage = 0;

    if (!mVectorAudioQosLoss.empty()) {
        mAudioQosLossAverage = getAverageExcludeMaxMin(mVectorAudioQosLoss);
    } else {
        LOGE(_TAG(TAG), "calculateAudioQosAverage: mVectorAudioQosLoss is empty, please check !!!!!");
    }

    if (!mVectorAudioQosJitter.empty()) {
        mAudioQosJitterAverage = getAverageExcludeMaxMin(mVectorAudioQosJitter);
    } else {
        LOGE(_TAG(TAG), "calculateAudioQosAverage: mVectorAudioQosJitter is empty, please check !!!!!");
    }

    if (!mVectorAudioQosRtt.empty()) {
        mAudioQosRttAverage = getAverageExcludeMaxMin(mVectorAudioQosRtt);
    } else {
        LOGE(_TAG(TAG), "calculateAudioQosAverage: mVectorAudioQosRtt is empty, please check !!!!!");
    }
}

void ImsConnManPolicy::calculateVideoQosAverage() {
    mVideoQosLossAverage = 0;
    mVideoQosJitterAverage = 0;
    mVideoQosRttAverage = 0;

    if (!mVectorVideoQosLoss.empty()) {
        mVideoQosLossAverage = getAverageExcludeMaxMin(mVectorVideoQosLoss);
    } else {
        LOGE(_TAG(TAG), "calculateVideoQosAverage: mVectorVideoQosLoss is empty, please check !!!!!");
    }

    if (!mVectorVideoQosJitter.empty()) {
        mVideoQosJitterAverage = getAverageExcludeMaxMin(mVectorVideoQosJitter);
    } else {
        LOGE(_TAG(TAG), "calculateVideoQosAverage: mVectorVideoQosJitter is empty, please check !!!!!");
    }

    if (!mVectorVideoQosRtt.empty()) {
        mVideoQosRttAverage = getAverageExcludeMaxMin(mVectorVideoQosRtt);
    } else {
        LOGE(_TAG(TAG), "calculateVideoQosAverage: mVectorVideoQosRtt is empty, please check !!!!!");
    }
}

bool ImsConnManPolicy::isExactlyCalculateIdleThreshold(int wifiCallingMode) {
    bool ret = false;

    if (mIsExactlyCalculateLogic) {
        if (mVectorWifiRssi.size() < mExactlyCalculateLastCount || mVectorLteRsrp.size() < mExactlyCalculateLastCount) {
            if (mVectorWifiRssi.empty() || mVectorLteRsrp.empty()) {
                LOGE(_TAG(TAG), "isExactlyCalculateIdleThreshold: mVectorWifiRssi.size = %d, mVectorLteRsrp.size = %d, please check !!!!!", \
                        mVectorWifiRssi.size(), mVectorLteRsrp.size());

                return false;
            }

            return true;
        }

        int wifiRssi = WIFI_SIGNAL_STRENGTH_INVALID;
        int lteRsrp = LTE_SIGNAL_STRENGTH_INVALID;

        for (int i = mExactlyCalculateLastCount; i > 0; i--) {
            wifiRssi = mVectorWifiRssi.at(mVectorWifiRssi.size() - i);
            lteRsrp = mVectorLteRsrp.at(mVectorLteRsrp.size() - i);

            if (ImsConnManStateInfo::isVolteRegistered()) {
                if (WFC_MODE_WIFI_PREFERRED == wifiCallingMode) {
                    if (VOLTE_TO_VOWIFI == compareIdleThresholdForWifiPreferred(wifiRssi, lteRsrp)) {
                        ret = true;
                    } else {
                        LOGE(_TAG(TAG), "isExactlyCalculateIdleThreshold: Volte don't switch to Vowifi");
                        ret = false;
                    }
                } else if (WFC_MODE_CELLULAR_PREFERRED == wifiCallingMode) {
                    if (VOLTE_TO_VOWIFI == compareIdleThresholdForCellularPreferred(wifiRssi, lteRsrp)) {
                        ret = true;
                    } else {
                        LOGE(_TAG(TAG), "isExactlyCalculateIdleThreshold: Volte don't switch to Vowifi");
                        ret = false;
                    }
                }
            } else if (ImsConnManStateInfo::isVowifiRegistered()) {
                if (WFC_MODE_WIFI_PREFERRED == wifiCallingMode) {
                    if (VOWIFI_TO_VOLTE == compareIdleThresholdForWifiPreferred(wifiRssi, lteRsrp)) {
                        ret = true;
                    } else {
                        LOGE(_TAG(TAG), "isExactlyCalculateIdleThreshold: Vowifi don't switch to Volte");
                        ret = false;
                    }
                } else if (WFC_MODE_CELLULAR_PREFERRED == wifiCallingMode) {
                    if (VOWIFI_TO_VOLTE == compareIdleThresholdForCellularPreferred(wifiRssi, lteRsrp)) {
                        ret = true;
                    } else {
                        LOGE(_TAG(TAG), "isExactlyCalculateIdleThreshold: Vowifi don't switch to Volte");
                        ret = false;
                    }
                }
            }
        }
    } else {
        ret = true;
    }

    return ret;
}

int ImsConnManPolicy::compareIdleThresholdForWifiPreferred(int rssi, int rsrp) {
    int ret = NOT_HANDOVER;

    if (rssi >= mWifiIdleRssiHigh) {
        ret = VOLTE_TO_VOWIFI;
    } else if (rssi < mWifiIdleRssiLow && rsrp >= mLteRsrpHigh) {
        ret = VOWIFI_TO_VOLTE;
    }

    return ret;
}

void ImsConnManPolicy::loopIdleThresholdForWifiPreferred(int rssi, int rsrp) {
    if (ImsConnManStateInfo::isVolteRegistered()) {
        if (VOLTE_TO_VOWIFI == compareIdleThresholdForWifiPreferred(rssi, rsrp)) {
            if (!isExactlyCalculateIdleThreshold(ImsConnManStateInfo::getWifiCallingMode())) {
                return;
            }

            LOGD(_TAG(TAG), "loopIdleThresholdForWifiPreferred: rssi = %d, rsrp = %d", rssi, rsrp);
            LOGD(_TAG(TAG), "loopIdleThresholdForWifiPreferred: Volte switch to Vowifi !!!!!");
            ImsConnManController::switchOrHandoverVowifi(mIsInitiateVowifiAfterLte);
            releaseAllTimer();
        }
    } else if (ImsConnManStateInfo::isVowifiRegistered()) {
        if (VOWIFI_TO_VOLTE == compareIdleThresholdForWifiPreferred(rssi, rsrp)) {
            if (!isExactlyCalculateIdleThreshold(ImsConnManStateInfo::getWifiCallingMode())) {
                return;
            }

            LOGD(_TAG(TAG), "loopIdleThresholdForWifiPreferred: rssi = %d, rsrp = %d", rssi, rsrp);
            LOGD(_TAG(TAG), "loopIdleThresholdForWifiPreferred: Vowifi switch to Volte !!!!!");
            ImsConnManController::vowifiUnavailable(ImsConnManStateInfo::isWifiConnected());
            releaseAllTimer();
        }
    } else {
        LOGE(_TAG(TAG), "loopIdleThresholdForWifiPreferred: please check !!!!!");
        createPolicyTimerTask(false, false);
    }
}

int ImsConnManPolicy::compareIdleThresholdForCellularPreferred(int rssi, int rsrp) {
    int ret = NOT_HANDOVER;

    if (rssi >= mWifiIdleRssiHigh && rsrp < mLteRsrpLow) {
        ret = VOLTE_TO_VOWIFI;
    } else if (rsrp >= mLteRsrpHigh) {
        ret = VOWIFI_TO_VOLTE;
    }

    return ret;
}

void ImsConnManPolicy::loopIdleThresholdForCellularPreferred(int rssi, int rsrp) {
    if (ImsConnManStateInfo::isVolteRegistered()) {
        if (VOLTE_TO_VOWIFI == compareIdleThresholdForCellularPreferred(rssi, rsrp)) {
            if (!isExactlyCalculateIdleThreshold(ImsConnManStateInfo::getWifiCallingMode())) {
                return;
            }

            LOGD(_TAG(TAG), "loopIdleThresholdForCellularPreferred: rssi = %d, rsrp = %d", rssi, rsrp);
            LOGD(_TAG(TAG), "loopIdleThresholdForCellularPreferred: Volte switch to Vowifi !!!!!");
            ImsConnManController::switchOrHandoverVowifi(mIsInitiateVowifiAfterLte);
            releaseAllTimer();
        }
    } else if (ImsConnManStateInfo::isVowifiRegistered()) {
        if (VOWIFI_TO_VOLTE == compareIdleThresholdForCellularPreferred(rssi, rsrp)) {
            if (!isExactlyCalculateIdleThreshold(ImsConnManStateInfo::getWifiCallingMode())) {
                return;
            }

            LOGD(_TAG(TAG), "loopIdleThresholdForCellularPreferred: rssi = %d, rsrp = %d", rssi, rsrp);
            LOGD(_TAG(TAG), "loopIdleThresholdForCellularPreferred: Vowifi switch to Volte !!!!!");
            ImsConnManController::vowifiUnavailable(ImsConnManStateInfo::isWifiConnected());
            releaseAllTimer();
        }
    } else {
        LOGE(_TAG(TAG), "loopIdleThresholdForCellularPreferred: please check !!!!!");
        createPolicyTimerTask(false, false);
    }
}

void ImsConnManPolicy::loopIdleThreshold(int rssi, int rsrp) {
    switch (ImsConnManStateInfo::getWifiCallingMode()) {
        case WFC_MODE_CELLULAR_PREFERRED:
            loopIdleThresholdForCellularPreferred(rssi, rsrp);
            break;

        case WFC_MODE_WIFI_PREFERRED:
            loopIdleThresholdForWifiPreferred(rssi, rsrp);
            break;

        default:
            LOGE(_TAG(TAG), "loopIdleThreshold: Wifi-Calling mode is error, please check !!!!!");
            break;
    }
}

void ImsConnManPolicy::createIdleThresholdTimerTask() {
    if (ImsConnManStateInfo::isWifiCallingEnabled()
            && ImsConnManStateInfo::isWifiConnected()
            && ImsConnManStateInfo::isLteCellularNetwork()
            && mIsExistAnyPolicyTimer) {
        mCurrentLoopCount++;
        mVectorWifiRssi.push_back(NetworkUtils::getCurrentWifiRssi());
        mVectorLteRsrp.push_back(NetworkUtils::getCurrentLteRsrp());

        if (mTotalLoopCount > 0 && mCurrentLoopCount >= mTotalLoopCount) {
            calculateSignalAverage();
            loopIdleThreshold(mWifiRssiAverage, mLteRsrpAverage);

            if (mIsExistAnyPolicyTimer && mTotalLoopCount > 0 && mCurrentLoopCount >= mTotalLoopCount) {
                mVectorWifiRssi.erase(mVectorWifiRssi.begin());
                mVectorLteRsrp.erase(mVectorLteRsrp.begin());
            }
        }
    } else {
        LOGE(_TAG(TAG), "[createIdleThresholdTimerTask]: Conditions are not satisfied, release [Idle threshold Timer]");
        createPolicyTimerTask(false, false);
    }
}

void createIdleThresholdTimerTaskCallback(nsITimer *aTimer, void *aClosure) {
    ImsConnManPolicy *pPolicy = static_cast<ImsConnManPolicy*>(aClosure);
    pPolicy->createIdleThresholdTimerTask();
}

bool ImsConnManPolicy::isExactlyCalculateIdleThresholdWithoutIms(int wifiCallingMode) {
    bool ret = false;

    if (mIsExactlyCalculateLogic) {
        if (WFC_MODE_WIFI_PREFERRED == wifiCallingMode
                || (!ImsConnManStateInfo::isLteCellularNetwork() && mVectorLteRsrp.empty())) {
            if (mVectorWifiRssi.size() < mExactlyCalculateLastCount) {
                if (mVectorWifiRssi.empty()) {
                    LOGE(_TAG(TAG), "isExactlyCalculateIdleThresholdWithoutIms: mVectorWifiRssi.size = %d, please check !!!!!", \
                            mVectorWifiRssi.size());

                    return false;
                }

                return true;
            }

            int wifiRssi = WIFI_SIGNAL_STRENGTH_INVALID;

            for (int i = mExactlyCalculateLastCount; i > 0; i--) {
                wifiRssi = mVectorWifiRssi.at(mVectorWifiRssi.size() - i);

                if (AUTO_ATTACH_VOWIFI == compareIdleThresholdWithoutImsForWifiPreferred(wifiRssi)) {
                    ret = true;
                } else {
                    LOGE(_TAG(TAG), "isExactlyCalculateIdleThresholdWithoutIms: Don't automatically switch to Vowifi");
                    ret = false;
                }
            }
        } else if (WFC_MODE_CELLULAR_PREFERRED == wifiCallingMode) {
            if (mVectorWifiRssi.size() < mExactlyCalculateLastCount || mVectorLteRsrp.size() < mExactlyCalculateLastCount) {
                if (mVectorWifiRssi.empty() || mVectorLteRsrp.empty()) {
                    LOGE(_TAG(TAG), "isExactlyCalculateIdleThresholdWithoutIms: mVectorWifiRssi.size = %d, mVectorLteRsrp.size = %d, please check !!!!!", \
                            mVectorWifiRssi.size(), mVectorLteRsrp.size());

                    return false;
                }

                return true;
            }

            int wifiRssi = WIFI_SIGNAL_STRENGTH_INVALID;
            int lteRsrp = LTE_SIGNAL_STRENGTH_INVALID;

            for (int i = mExactlyCalculateLastCount; i > 0; i--) {
                wifiRssi = mVectorWifiRssi.at(mVectorWifiRssi.size() - i);
                lteRsrp = mVectorLteRsrp.at(mVectorLteRsrp.size() - i);

                if (AUTO_ATTACH_VOWIFI == compareIdleThresholdWithoutImsForCellularPreferred(wifiRssi, lteRsrp)) {
                    ret = true;
                } else {
                    LOGE(_TAG(TAG), "isExactlyCalculateIdleThresholdWithoutIms: Don't automatically switch to Vowifi");
                    ret = false;
                }
            }
        }
    } else {
        ret = true;
    }

    return ret;
}

int ImsConnManPolicy::compareIdleThresholdWithoutImsForWifiPreferred(int rssi) {
    int ret = NOT_HANDOVER;

    if (rssi >= mWifiIdleRssiLow) {
        ret = AUTO_ATTACH_VOWIFI;
    }

    return ret;
}

void ImsConnManPolicy::loopIdleThresholdWithoutImsForWifiPreferred(int rssi) {
    if (!ImsConnManStateInfo::isVolteRegistered() && !ImsConnManStateInfo::isVowifiRegistered()) {
        if (AUTO_ATTACH_VOWIFI == compareIdleThresholdWithoutImsForWifiPreferred(rssi)) {
            if (!isExactlyCalculateIdleThresholdWithoutIms(ImsConnManStateInfo::getWifiCallingMode())) {
                return;
            }

            LOGD(_TAG(TAG), "loopIdleThresholdWithoutImsForWifiPreferred: rssi = %d", rssi);
            LOGD(_TAG(TAG), "loopIdleThresholdWithoutImsForWifiPreferred: Automatically switch to Vowifi !!!!!");
            ImsConnManController::switchOrHandoverVowifi(mIsInitiateVowifiAfterLte);
            releaseAllTimer();
        }
    } else {
        LOGE(_TAG(TAG), "loopIdleThresholdWithoutImsForWifiPreferred: please check !!!!!");
        releaseAllTimer();
    }
}

int ImsConnManPolicy::compareIdleThresholdWithoutImsForCellularPreferred(int rssi, int rsrp) {
    int ret = NOT_HANDOVER;

    if (rssi >= mWifiIdleRssiHigh && rsrp < mLteRsrpLow) {
        ret = AUTO_ATTACH_VOWIFI;
    }

    return ret;
}

void ImsConnManPolicy::loopIdleThresholdWithoutImsForCellularPreferred(int rssi, int rsrp) {
    if (!ImsConnManStateInfo::isVolteRegistered() && !ImsConnManStateInfo::isVowifiRegistered()) {
        if (AUTO_ATTACH_VOWIFI == compareIdleThresholdWithoutImsForCellularPreferred(rssi, rsrp)) {
            if (!isExactlyCalculateIdleThresholdWithoutIms(ImsConnManStateInfo::getWifiCallingMode())) {
                return;
            }

            LOGD(_TAG(TAG), "loopIdleThresholdWithoutImsForCellularPreferred: rssi = %d, rsrp = %d", rssi, rsrp);
            LOGD(_TAG(TAG), "loopIdleThresholdWithoutImsForCellularPreferred: Automatically switch to Vowifi !!!!!");
            ImsConnManController::switchOrHandoverVowifi(mIsInitiateVowifiAfterLte);
            releaseAllTimer();
        }
    } else {
        LOGE(_TAG(TAG), "loopIdleThresholdWithoutImsForCellularPreferred: please check !!!!!");
        releaseAllTimer();
    }
}

void ImsConnManPolicy::loopIdleThresholdWithoutIms(int rssi, int rsrp) {
    switch (ImsConnManStateInfo::getWifiCallingMode()) {
        case WFC_MODE_CELLULAR_PREFERRED:
            loopIdleThresholdWithoutImsForCellularPreferred(rssi, rsrp);
            break;

        case WFC_MODE_WIFI_PREFERRED:
            loopIdleThresholdWithoutImsForWifiPreferred(rssi);
            break;

        default:
            LOGE(_TAG(TAG), "loopIdleThresholdWithoutIms: Wifi-Calling mode is error, please check !!!!!");
            break;
    }
}

void ImsConnManPolicy::createIdleThresholdTimerTaskWithoutIms() {
    if (ImsConnManStateInfo::isWifiCallingEnabled()
            && ImsConnManStateInfo::isWifiConnected()
            && ImsConnManStateInfo::isLteCellularNetwork()
            && mIsExistAnyPolicyTimer) {
        mCurrentLoopCount++;
        mVectorWifiRssi.push_back(NetworkUtils::getCurrentWifiRssi());
        mVectorLteRsrp.push_back(NetworkUtils::getCurrentLteRsrp());

        if (mTotalLoopCount > 0 && mCurrentLoopCount >= mTotalLoopCount) {
            calculateSignalAverage();
            loopIdleThresholdWithoutIms(mWifiRssiAverage, mLteRsrpAverage);

            if (mIsExistAnyPolicyTimer && mTotalLoopCount > 0 && mCurrentLoopCount >= mTotalLoopCount) {
                mVectorWifiRssi.erase(mVectorWifiRssi.begin());
                mVectorLteRsrp.erase(mVectorLteRsrp.begin());
            }
        }
    } else {
        LOGE(_TAG(TAG), "[createIdleThresholdTimerTaskWithoutIms]: Conditions are not satisfied, release [Idle threshold Timer without IMS]");
        createPolicyTimerTask(false, false);
    }
}

void createIdleThresholdTimerTaskWithoutImsCallback(nsITimer *aTimer, void *aClosure) {
    ImsConnManPolicy *pPolicy = static_cast<ImsConnManPolicy*>(aClosure);
    pPolicy->createIdleThresholdTimerTaskWithoutIms();
}

void ImsConnManPolicy::createIdleThresholdTimerTaskWithoutLte() {
    if (ImsConnManStateInfo::isWifiCallingEnabled()
            && ImsConnManStateInfo::isWifiConnected()
            && !ImsConnManStateInfo::isLteCellularNetwork()
            && mIsExistAnyPolicyTimer) {
        mCurrentLoopCount++;
        mVectorWifiRssi.push_back(NetworkUtils::getCurrentWifiRssi());

        if (mTotalLoopCount > 0 && mCurrentLoopCount >= mTotalLoopCount) {
            calculateSignalAverage();
            loopIdleThresholdWithoutImsForWifiPreferred(mWifiRssiAverage);

            if (mIsExistAnyPolicyTimer && mTotalLoopCount > 0 && mCurrentLoopCount >= mTotalLoopCount) {
                mVectorWifiRssi.erase(mVectorWifiRssi.begin());
            }
        }
    } else {
        LOGE(_TAG(TAG), "[createIdleThresholdTimerTaskWithoutLte]: Conditions are not satisfied, release [Idle threshold Timer without LTE]");
        createPolicyTimerTask(false, false);
    }
}

void createIdleThresholdTimerTaskWithoutLteCallback(nsITimer *aTimer, void *aClosure) {
    ImsConnManPolicy *pPolicy = static_cast<ImsConnManPolicy*>(aClosure);
    pPolicy->createIdleThresholdTimerTaskWithoutLte();
}

void ImsConnManPolicy::createIdleThresholdTimerTaskWifiOnly() {
    if (ImsConnManStateInfo::isWifiCallingEnabled()
            && ImsConnManStateInfo::isWifiConnected()
            && mIsExistAnyPolicyTimer) {
        mCurrentLoopCount++;

        if (mTotalLoopCount > 0 && mCurrentLoopCount >= mTotalLoopCount) {
            ImsConnManController::switchOrHandoverVowifi(mIsInitiateVowifiAfterLte);
            releaseAllTimer();
        }
    } else {
        LOGE(_TAG(TAG), "[createIdleThresholdTimerTaskWifiOnly]: Conditions are not satisfied, release [Idle threshold Timer Wifi only]");
        createPolicyTimerTask(false, false);
    }
}

void createIdleThresholdTimerTaskWifiOnlyCallback(nsITimer *aTimer, void *aClosure) {
    ImsConnManPolicy *pPolicy = static_cast<ImsConnManPolicy*>(aClosure);
    pPolicy->createIdleThresholdTimerTaskWifiOnly();
}

bool ImsConnManPolicy::isExactlyCalculateImsCallThreshold(int wifiCallingMode, int imsCallType) {
    bool ret = false;

    if (mIsExactlyCalculateLogic) {
        if (mVectorWifiRssi.size() < mExactlyCalculateLastCount || mVectorLteRsrp.size() < mExactlyCalculateLastCount) {
            if (mVectorWifiRssi.empty() || mVectorLteRsrp.empty()) {
                LOGE(_TAG(TAG), "isExactlyCalculateImsCallThreshold: mVectorWifiRssi.size = %d, mVectorLteRsrp.size = %d, please check !!!!!", \
                        mVectorWifiRssi.size(), mVectorLteRsrp.size());

                return false;
            }

            return true;
        }

        int wifiRssi = WIFI_SIGNAL_STRENGTH_INVALID;
        int lteRsrp = LTE_SIGNAL_STRENGTH_INVALID;

        for (int i = mExactlyCalculateLastCount; i > 0; i--) {
            wifiRssi = mVectorWifiRssi.at(mVectorWifiRssi.size() - i);
            lteRsrp = mVectorLteRsrp.at(mVectorLteRsrp.size() - i);

            if (ImsConnManStateInfo::isVolteRegistered()) {
                if (WFC_MODE_WIFI_PREFERRED == wifiCallingMode) {
                    if (IMS_CALL_TYPE_AUDIO == imsCallType) {
                        if (VOLTE_TO_VOWIFI == compareAudioCallThresholdForWifiPreferred(wifiRssi, lteRsrp)) {
                            ret = true;
                        } else {
                            LOGE(_TAG(TAG), "isExactlyCalculateImsCallThreshold: Volte don't handover to Vowifi");
                            ret = false;
                        }
                    } else if (IMS_CALL_TYPE_VIDEO == imsCallType) {
                        if (VOLTE_TO_VOWIFI == compareVideoCallThresholdForWifiPreferred(wifiRssi, lteRsrp)) {
                            ret = false;
                        } else {
                            LOGE(_TAG(TAG), "isExactlyCalculateImsCallThreshold: Volte don't handover to Vowifi");
                            ret = false;
                        }
                    }
                } else if (WFC_MODE_CELLULAR_PREFERRED == wifiCallingMode) {
                    if (IMS_CALL_TYPE_AUDIO == imsCallType) {
                        if (VOLTE_TO_VOWIFI == compareAudioCallThresholdForCellularPreferred(wifiRssi, lteRsrp)) {
                            ret = true;
                        } else {
                            LOGE(_TAG(TAG), "isExactlyCalculateImsCallThreshold: Volte don't handover to Vowifi");
                            ret = false;
                        }
                    } else if (IMS_CALL_TYPE_VIDEO == imsCallType) {
                        if (VOLTE_TO_VOWIFI == compareVideoCallThresholdForCellularPreferred(wifiRssi, lteRsrp)) {
                            ret = true;
                        } else {
                            LOGE(_TAG(TAG), "isExactlyCalculateImsCallThreshold: Volte don't handover to Vowifi");
                            ret = false;
                        }
                    }
                }
            } else if (ImsConnManStateInfo::isVowifiRegistered()) {
                if (WFC_MODE_WIFI_PREFERRED == wifiCallingMode) {
                    if (IMS_CALL_TYPE_AUDIO == imsCallType) {
                        if (VOWIFI_TO_VOLTE == compareAudioCallThresholdForWifiPreferred(wifiRssi, lteRsrp)) {
                            ret = true;
                        } else {
                            LOGE(_TAG(TAG), "isExactlyCalculateImsCallThreshold: Vowifi don't handover to Volte");
                            ret = false;
                        }
                    } else if (IMS_CALL_TYPE_VIDEO == imsCallType) {
                        if (VOWIFI_TO_VOLTE == compareVideoCallThresholdForWifiPreferred(wifiRssi, lteRsrp)) {
                            ret = true;
                        } else {
                            LOGE(_TAG(TAG), "isExactlyCalculateImsCallThreshold: Vowifi don't handover to Volte");
                            ret = false;
                        }
                    }
                } else if (WFC_MODE_CELLULAR_PREFERRED == wifiCallingMode) {
                    if (IMS_CALL_TYPE_AUDIO == imsCallType) {
                        if (VOWIFI_TO_VOLTE == compareAudioCallThresholdForCellularPreferred(wifiRssi, lteRsrp)) {
                            ret = true;
                        } else {
                            LOGE(_TAG(TAG), "isExactlyCalculateImsCallThreshold: Vowifi don't handover to Volte");
                            ret = false;
                        }
                    } else if (IMS_CALL_TYPE_VIDEO == imsCallType) {
                        if (VOWIFI_TO_VOLTE == compareVideoCallThresholdForCellularPreferred(wifiRssi, lteRsrp)) {
                            ret = true;
                        } else {
                            LOGE(_TAG(TAG), "isExactlyCalculateImsCallThreshold: Vowifi don't handover to Volte");
                            ret = false;
                        }
                    }
                }
            }
        }
    } else {
        ret = true;
    }

    return ret;
}

int ImsConnManPolicy::compareAudioCallThresholdForWifiPreferred(int rssi, int rsrp) {
    int ret = NOT_HANDOVER;

    if (rssi >= mWifiCallRssiHigh) {
        ret = VOLTE_TO_VOWIFI;
    } else if (rssi < mWifiCallRssiLow && rsrp >= mLteRsrpHigh) {
        ret = VOWIFI_TO_VOLTE;
    }

    return ret;
}

void ImsConnManPolicy::loopAudioCallThresholdForWifiPreferred(int rssi, int rsrp) {
    if (ImsConnManStateInfo::isVolteRegistered()) {
        if (VOLTE_TO_VOWIFI == compareAudioCallThresholdForWifiPreferred(rssi, rsrp)) {
            if (!isExactlyCalculateImsCallThreshold(ImsConnManStateInfo::getWifiCallingMode(), IMS_CALL_TYPE_AUDIO)) {
                return;
            }

            LOGD(_TAG(TAG), "loopAudioCallThresholdForWifiPreferred: rssi = %d, rsrp = %d", rssi, rsrp);
            LOGD(_TAG(TAG), "loopAudioCallThresholdForWifiPreferred: Volte handover to Vowifi !!!!!");
            ImsConnManController::switchOrHandoverVowifi(mIsInitiateVowifiAfterLte);
            releaseAllTimer();
        }
    } else if (ImsConnManStateInfo::isVowifiRegistered()) {
        if (VOWIFI_TO_VOLTE == compareAudioCallThresholdForWifiPreferred(rssi, rsrp)) {
            if (!isExactlyCalculateImsCallThreshold(ImsConnManStateInfo::getWifiCallingMode(), IMS_CALL_TYPE_AUDIO)) {
                return;
            }

            LOGD(_TAG(TAG), "loopAudioCallThresholdForWifiPreferred: rssi = %d, rsrp = %d", rssi, rsrp);
            LOGD(_TAG(TAG), "loopAudioCallThresholdForWifiPreferred: Vowifi handover to Volte !!!!!");
            ImsConnManController::handoverToVolte(true);
            releaseAllTimer();
        }
    } else {
        LOGE(_TAG(TAG), "loopAudioCallThresholdForWifiPreferred: please check !!!!!");
        releaseAllTimer();
    }
}

int ImsConnManPolicy::compareAudioCallThresholdForCellularPreferred(int rssi, int rsrp) {
    int ret = NOT_HANDOVER;

    if (rssi >= mWifiCallRssiHigh && rsrp < mLteRsrpLow) {
        ret = VOLTE_TO_VOWIFI;
    } else if (rsrp >= mLteRsrpHigh) {
        ret = VOWIFI_TO_VOLTE;
    }

    return ret;
}

void ImsConnManPolicy::loopAudioCallThresholdForCellularPreferred(int rssi, int rsrp) {
    if (ImsConnManStateInfo::isVolteRegistered()) {
        if (VOLTE_TO_VOWIFI == compareAudioCallThresholdForCellularPreferred(rssi, rsrp)) {
            if (!isExactlyCalculateImsCallThreshold(ImsConnManStateInfo::getWifiCallingMode(), IMS_CALL_TYPE_AUDIO)) {
                return;
            }

            LOGD(_TAG(TAG), "loopAudioCallThresholdForCellularPreferred: rssi = %d, rsrp = %d", rssi, rsrp);
            LOGD(_TAG(TAG), "loopAudioCallThresholdForCellularPreferred: Volte handover to Vowifi !!!!!");
            ImsConnManController::switchOrHandoverVowifi(mIsInitiateVowifiAfterLte);
            releaseAllTimer();
        }
    } else if (ImsConnManStateInfo::isVowifiRegistered()) {
        if (VOWIFI_TO_VOLTE == compareAudioCallThresholdForCellularPreferred(rssi, rsrp)) {
            if (!isExactlyCalculateImsCallThreshold(ImsConnManStateInfo::getWifiCallingMode(), IMS_CALL_TYPE_AUDIO)) {
                return;
            }

            LOGD(_TAG(TAG), "loopAudioCallThresholdForCellularPreferred: rssi = %d, rsrp = %d", rssi, rsrp);
            LOGD(_TAG(TAG), "loopAudioCallThresholdForCellularPreferred: Vowifi handover to Volte !!!!!");
            ImsConnManController::handoverToVolte(true);
            releaseAllTimer();
        }
    } else {
        LOGE(_TAG(TAG), "loopAudioCallThresholdForCellularPreferred: please check !!!!!");
        releaseAllTimer();
    }
}

void ImsConnManPolicy::loopAudioCallThreshold(int rssi, int rsrp) {
    switch (ImsConnManStateInfo::getWifiCallingMode()) {
        case WFC_MODE_CELLULAR_PREFERRED:
            loopAudioCallThresholdForCellularPreferred(rssi, rsrp);
            break;

        case WFC_MODE_WIFI_PREFERRED:
            loopAudioCallThresholdForWifiPreferred(rssi, rsrp);
            break;

        default:
            LOGE(_TAG(TAG), "loopAudioCallThreshold: Wifi-Calling mode is error, please check !!!!!");
            break;
    }
}

void ImsConnManPolicy::createAudioCallThresholdTimerTask() {
    if (ImsConnManStateInfo::isWifiCallingEnabled()
            && ImsConnManStateInfo::isWifiConnected()
            && ImsConnManStateInfo::isLteCellularNetwork()
            && mIsExistAnyPolicyTimer) {
        mCurrentLoopCount++;
        mVectorWifiRssi.push_back(NetworkUtils::getCurrentWifiRssi());
        mVectorLteRsrp.push_back(NetworkUtils::getCurrentLteRsrp());

        if (mTotalLoopCount > 0 && mCurrentLoopCount >= mTotalLoopCount) {
            calculateSignalAverage();
            loopAudioCallThreshold(mWifiRssiAverage, mLteRsrpAverage);

            if (mIsExistAnyPolicyTimer && mTotalLoopCount > 0 && mCurrentLoopCount >= mTotalLoopCount) {
                mVectorWifiRssi.erase(mVectorWifiRssi.begin());
                mVectorLteRsrp.erase(mVectorLteRsrp.begin());
            }
        }
    } else {
        LOGE(_TAG(TAG), "[createAudioCallThresholdTimerTask]: Conditions are not satisfied, release [Audio call threshold Timer]");
        createPolicyTimerTask(false, false);
    }
}

void createAudioCallThresholdTimerTaskCallback(nsITimer *aTimer, void *aClosure) {
    ImsConnManPolicy *pPolicy = static_cast<ImsConnManPolicy*>(aClosure);
    pPolicy->createAudioCallThresholdTimerTask();
}

int ImsConnManPolicy::compareVideoCallThresholdForWifiPreferred(int rssi, int rsrp) {
    int ret = NOT_HANDOVER;

    if (rssi >= mWifiCallRssiHigh) {
        ret = VOLTE_TO_VOWIFI;
    } else if (rssi < mWifiCallRssiLow && rsrp >= mLteRsrpHigh) {
        ret = VOWIFI_TO_VOLTE;
    }

    return ret;
}

void ImsConnManPolicy::loopVideoCallThresholdForWifiPreferred(int rssi, int rsrp) {
    if (ImsConnManStateInfo::isVolteRegistered()) {
        if (VOLTE_TO_VOWIFI == compareVideoCallThresholdForWifiPreferred(rssi, rsrp)) {
            if (!isExactlyCalculateImsCallThreshold(ImsConnManStateInfo::getWifiCallingMode(), IMS_CALL_TYPE_VIDEO)) {
                return;
            }

            LOGD(_TAG(TAG), "loopVideoCallThresholdForWifiPreferred: rssi = %d, rsrp = %d", rssi, rsrp);
            LOGD(_TAG(TAG), "loopVideoCallThresholdForWifiPreferred: Volte handover to Vowifi !!!!!");
            ImsConnManController::switchOrHandoverVowifi(mIsInitiateVowifiAfterLte);
            releaseAllTimer();
        }
    } else if (ImsConnManStateInfo::isVowifiRegistered()) {
        if (VOWIFI_TO_VOLTE == compareVideoCallThresholdForWifiPreferred(rssi, rsrp)) {
            if (!isExactlyCalculateImsCallThreshold(ImsConnManStateInfo::getWifiCallingMode(), IMS_CALL_TYPE_VIDEO)) {
                return;
            }

            LOGD(_TAG(TAG), "loopVideoCallThresholdForWifiPreferred: rssi = %d, rsrp = %d", rssi, rsrp);
            LOGD(_TAG(TAG), "loopVideoCallThresholdForWifiPreferred: Vowifi handover to Volte !!!!!");
            ImsConnManController::handoverToVolte(true);
            releaseAllTimer();
        }
    } else {
        LOGE(_TAG(TAG), "loopVideoCallThresholdForWifiPreferred: please check !!!!!");
        releaseAllTimer();
    }
}

int ImsConnManPolicy::compareVideoCallThresholdForCellularPreferred(int rssi, int rsrp) {
    int ret = NOT_HANDOVER;

    if (rssi >= mWifiCallRssiHigh && rsrp < mLteRsrpLow) {
        ret = VOLTE_TO_VOWIFI;
    } else if (rsrp >= mLteRsrpHigh) {
        ret = VOWIFI_TO_VOLTE;
    }

    return ret;
}

void ImsConnManPolicy::loopVideoCallThresholdForCellularPreferred(int rssi, int rsrp) {
    if (ImsConnManStateInfo::isVolteRegistered()) {
        if (VOLTE_TO_VOWIFI == compareVideoCallThresholdForCellularPreferred(rssi, rsrp)) {
            if (!isExactlyCalculateImsCallThreshold(ImsConnManStateInfo::getWifiCallingMode(), IMS_CALL_TYPE_VIDEO)) {
                return;
            }

            LOGD(_TAG(TAG), "loopVideoCallThresholdForCellularPreferred: rssi = %d, rsrp = %d", rssi, rsrp);
            LOGD(_TAG(TAG), "loopVideoCallThresholdForCellularPreferred: Volte handover to Vowifi !!!!!");
            ImsConnManController::switchOrHandoverVowifi(mIsInitiateVowifiAfterLte);
            releaseAllTimer();
        }
    } else if (ImsConnManStateInfo::isVowifiRegistered()) {
        if (VOWIFI_TO_VOLTE == compareVideoCallThresholdForCellularPreferred(rssi, rsrp)) {
            if (!isExactlyCalculateImsCallThreshold(ImsConnManStateInfo::getWifiCallingMode(), IMS_CALL_TYPE_VIDEO)) {
                return;
            }

            LOGD(_TAG(TAG), "loopVideoCallThresholdForCellularPreferred: rssi = %d, rsrp = %d", rssi, rsrp);
            LOGD(_TAG(TAG), "loopVideoCallThresholdForCellularPreferred: Vowifi handover to Volte !!!!!");
            ImsConnManController::handoverToVolte(true);
            releaseAllTimer();
        }
    } else {
        LOGE(_TAG(TAG), "loopVideoCallThresholdForCellularPreferred: please check !!!!!");
        releaseAllTimer();
    }
}

void ImsConnManPolicy::loopVideoCallThreshold(int rssi, int rsrp) {
    switch (ImsConnManStateInfo::getWifiCallingMode()) {
        case WFC_MODE_CELLULAR_PREFERRED:
            loopVideoCallThresholdForCellularPreferred(rssi, rsrp);
            break;

        case WFC_MODE_WIFI_PREFERRED:
            loopVideoCallThresholdForWifiPreferred(rssi, rsrp);
            break;

        default:
            LOGE(_TAG(TAG), "loopVideoCallThreshold: Wifi-Calling mode is error, please check !!!!!");
            break;
    }
}

void ImsConnManPolicy::createVideoCallThresholdTimerTask() {
    if (ImsConnManStateInfo::isWifiCallingEnabled()
            && ImsConnManStateInfo::isWifiConnected()
            && ImsConnManStateInfo::isLteCellularNetwork()
            && mIsExistAnyPolicyTimer) {
        mCurrentLoopCount++;
        mVectorWifiRssi.push_back(NetworkUtils::getCurrentWifiRssi());
        mVectorLteRsrp.push_back(NetworkUtils::getCurrentLteRsrp());

        if (mTotalLoopCount > 0 && mCurrentLoopCount >= mTotalLoopCount) {
            calculateSignalAverage();
            loopVideoCallThreshold(mWifiRssiAverage, mLteRsrpAverage);

            if (mIsExistAnyPolicyTimer && mTotalLoopCount > 0 && mCurrentLoopCount >= mTotalLoopCount) {
                mVectorWifiRssi.erase(mVectorWifiRssi.begin());
                mVectorLteRsrp.erase(mVectorLteRsrp.begin());
            }
        }
    } else {
        LOGE(_TAG(TAG), "[createVideoCallThresholdTimerTask]: Conditions are not satisfied, release [Video call threshold Timer]");
        createPolicyTimerTask(false, false);
    }
}

void createVideoCallThresholdTimerTaskCallback(nsITimer *aTimer, void *aClosure) {
    ImsConnManPolicy *pPolicy = static_cast<ImsConnManPolicy*>(aClosure);
    pPolicy->createVideoCallThresholdTimerTask();
}

void ImsConnManPolicy::releaseAllTimer() {
    //need add lock
    LOGD(_TAG(TAG), "releaseAllTimer !!!!!");

    if (nullptr != mIdleThresholdTimer) {
        mIdleThresholdTimer->Cancel();
        mIdleThresholdTimer = nullptr;
    }

    if (nullptr != mAudioCallThresholdTimer) {
        mAudioCallThresholdTimer->Cancel();
        mAudioCallThresholdTimer = nullptr;
    }

    if (nullptr != mVideoCallThresholdTimer) {
        mVideoCallThresholdTimer->Cancel();
        mVideoCallThresholdTimer = nullptr;
    }

    mIsExistAnyPolicyTimer = false;
    mCurrentLoopCount = 0;
    mTotalLoopCount = 0;

    mWifiRssiAverage = WIFI_SIGNAL_STRENGTH_INVALID;
    mLteRsrpAverage = LTE_SIGNAL_STRENGTH_INVALID;

    if (!mVectorWifiRssi.empty()) {
        mVectorWifiRssi.clear();
    }

    if (!mVectorLteRsrp.empty()) {
        mVectorLteRsrp.clear();
    }
}

void ImsConnManPolicy::createPolicyTimerTask(bool isIdleOperationFailed, bool isQuicklyInitiate) {
    releaseAllTimer();

    bool isWfcEnabled = ImsConnManStateInfo::isWifiCallingEnabled();
    int wfcMode = ImsConnManStateInfo::getWifiCallingMode();
    bool isWifiConnected = ImsConnManStateInfo::isWifiConnected();
    bool isLteCellularNetwork = ImsConnManStateInfo::isLteCellularNetwork();
    bool isVolteRegistered = ImsConnManStateInfo::isVolteRegistered();
    bool isVowifiRegistered = ImsConnManStateInfo::isVowifiRegistered();
    int imsCallType = ImsConnManStateInfo::getImsCallType();

    LOGD(_TAG(TAG), "createPolicyTimerTask: isIdleOperationFailed = %s, isQuicklyInitiate = %s", \
            isIdleOperationFailed ? "true" : "false", \
            isQuicklyInitiate ? "true" : "false");

    if (WFC_MODE_CELLULAR_ONLY == wfcMode) {
        LOGD(_TAG(TAG), "createPolicyTimerTask: Don't create timer task during %s", ImsConnManUtils::getWfcModeNameById(wfcMode));

        if (ImsConnManStateInfo::isIdleState() && CALL_MODE_NO_CALL == ImsConnManStateInfo::getCallMode()) {
            if (ImsConnManStateInfo::isMessageQueuePending()
                    && MSG_SWITCH_TO_VOWIFI == ImsConnManStateInfo::getPendingMessageId()) {
                ImsConnManController::cancelCurrentRequest();
            } else if (isVowifiRegistered) {
                ImsConnManController::vowifiUnavailable(isWifiConnected);
            }
        }

        return;
    } else if (WFC_MODE_WIFI_ONLY == wfcMode) {
        if (isWfcEnabled && isWifiConnected && !isVowifiRegistered) {
            if (ImsConnManStateInfo::isIdleState() && CALL_MODE_NO_CALL == ImsConnManStateInfo::getCallMode()) {
                if (nullptr == mIdleThresholdTimer) {
                    mIsExistAnyPolicyTimer = true;
                    LOGD(_TAG(TAG), "createPolicyTimerTask: create [Idle] threshold timer task during %s", \
                            ImsConnManUtils::getWfcModeNameById(wfcMode));

                    if (isQuicklyInitiate && mIsQuicklyCalculateLogic) {
                        mTotalLoopCount = mQuicklyCalculateCount;
                    } else {
                        mTotalLoopCount = mVowifiThresholdLoopCount;
                    }

                    LOGD(_TAG(TAG), "[createIdleThresholdTimerTaskWifiOnly]: mTotalLoopCount = %d, wifiCallingMode = %s", \
                            mTotalLoopCount, ImsConnManUtils::getWfcModeNameById(wfcMode));

                    mIdleThresholdTimer = PortingInterface::createTimer(createIdleThresholdTimerTaskWifiOnlyCallback,
                            static_cast<void*>(this),
                            TIMER_INTERVAL,
                            nsITimer::TYPE_REPEATING_PRECISE_CAN_SKIP);
                }
            }
        } else {
            LOGD(_TAG(TAG), "createPolicyTimerTask: Don't create timer task during %s", ImsConnManUtils::getWfcModeNameById(wfcMode));
        }

        return;
    }

    if (isWfcEnabled && isWifiConnected) {
        if (isLteCellularNetwork) {
            if (isVolteRegistered) {
                if (ImsConnManStateInfo::isIdleState() && CALL_MODE_NO_CALL == ImsConnManStateInfo::getCallMode()) {
                    //Idle of Volte
                    if (nullptr == mIdleThresholdTimer) {
                        mIsExistAnyPolicyTimer = true;
                        LOGD(_TAG(TAG), "createPolicyTimerTask: create [Idle] threshold timer task during Volte idle");

                        if (isQuicklyInitiate && mIsQuicklyCalculateLogic) {
                            mTotalLoopCount = mQuicklyCalculateCount;
                        } else {
                            mTotalLoopCount = mVolteThresholdLoopCount;
                        }

                        LOGD(_TAG(TAG), "[createIdleThresholdTimerTask]: mTotalLoopCount = %d, wifiCallingMode = %s", \
                                mTotalLoopCount, ImsConnManUtils::getWfcModeNameById(wfcMode));

                        mIdleThresholdTimer = PortingInterface::createTimer(createIdleThresholdTimerTaskCallback,
                                static_cast<void*>(this),
                                TIMER_INTERVAL,
                                nsITimer::TYPE_REPEATING_PRECISE_CAN_SKIP);
                    }
                } else {
                    //Volte Call
                    LOGD(_TAG(TAG), "createPolicyTimerTask: \"Volte Call\" and %s", ImsConnManUtils::getImsCallTypeNameById(imsCallType));

                    if (IMS_CALL_TYPE_AUDIO == imsCallType) {
                        if (nullptr == mAudioCallThresholdTimer) {
                            mIsExistAnyPolicyTimer = true;
                            LOGD(_TAG(TAG), "createPolicyTimerTask: create [Audio call] threshold timer task during Volte call");

                            mTotalLoopCount = mVolteSpecialLoopCount;
                            LOGD(_TAG(TAG), "[createAudioCallThresholdTimerTask]: mTotalLoopCount = %d, wifiCallingMode = %s", \
                                    mTotalLoopCount, ImsConnManUtils::getWfcModeNameById(wfcMode));

                            mAudioCallThresholdTimer = PortingInterface::createTimer(createAudioCallThresholdTimerTaskCallback,
                                    static_cast<void*>(this),
                                    TIMER_INTERVAL,
                                    nsITimer::TYPE_REPEATING_PRECISE_CAN_SKIP);
                        }
                    } else if (IMS_CALL_TYPE_VIDEO == imsCallType) {
                        if (nullptr == mVideoCallThresholdTimer) {
                            mIsExistAnyPolicyTimer = true;
                            LOGD(_TAG(TAG), "createPolicyTimerTask: create [Video call] threshold timer task during Volte call");

                            mTotalLoopCount = mVolteSpecialLoopCount;
                            LOGD(_TAG(TAG), "[createVideoCallThresholdTimerTask]: mTotalLoopCount = %d, wifiCallingMode = %s", \
                                    mTotalLoopCount, ImsConnManUtils::getWfcModeNameById(wfcMode));

                            mVideoCallThresholdTimer = PortingInterface::createTimer(createVideoCallThresholdTimerTaskCallback,
                                    static_cast<void*>(this),
                                    TIMER_INTERVAL,
                                    nsITimer::TYPE_REPEATING_PRECISE_CAN_SKIP);
                        }
                    }
                }
            } else if (isVowifiRegistered) {
                if (ImsConnManStateInfo::isIdleState() && CALL_MODE_NO_CALL == ImsConnManStateInfo::getCallMode()) {
                    //Idle of Vowifi
                    if (nullptr == mIdleThresholdTimer) {
                        mIsExistAnyPolicyTimer = true;
                        LOGD(_TAG(TAG), "createPolicyTimerTask: create [Idle] threshold timer task during Vowifi idle");

                        mTotalLoopCount = mVowifiThresholdLoopCount;
                        LOGD(_TAG(TAG), "[createIdleThresholdTimerTask]: mTotalLoopCount = %d, wifiCallingMode = %s", \
                                mTotalLoopCount, ImsConnManUtils::getWfcModeNameById(wfcMode));

                        mIdleThresholdTimer = PortingInterface::createTimer(createIdleThresholdTimerTaskCallback,
                                static_cast<void*>(this),
                                TIMER_INTERVAL,
                                nsITimer::TYPE_REPEATING_PRECISE_CAN_SKIP);
                    }
                } else {
                    //Vowifi Call
                    LOGD(_TAG(TAG), "createPolicyTimerTask: \"Vowifi Call\" and %s", ImsConnManUtils::getImsCallTypeNameById(imsCallType));

                    if (IMS_CALL_TYPE_AUDIO == imsCallType) {
                        if (nullptr == mAudioCallThresholdTimer) {
                            mIsExistAnyPolicyTimer = true;
                            LOGD(_TAG(TAG), "createPolicyTimerTask: create [Audio call] threshold timer task during Vowifi call");

                            mTotalLoopCount = mVowifiThresholdLoopCount;
                            LOGD(_TAG(TAG), "[createAudioCallThresholdTimerTask]: mTotalLoopCount = %d, wifiCallingMode = %s", \
                                    mTotalLoopCount, ImsConnManUtils::getWfcModeNameById(wfcMode));

                            mAudioCallThresholdTimer = PortingInterface::createTimer(createAudioCallThresholdTimerTaskCallback,
                                    static_cast<void*>(this),
                                    TIMER_INTERVAL,
                                    nsITimer::TYPE_REPEATING_PRECISE_CAN_SKIP);
                        }
                    } else if (IMS_CALL_TYPE_VIDEO == imsCallType) {
                        if (nullptr == mVideoCallThresholdTimer) {
                            mIsExistAnyPolicyTimer = true;
                            LOGD(_TAG(TAG), "createPolicyTimerTask: create [Video call] threshold timer task during Vowifi call");

                            mTotalLoopCount = mVowifiThresholdLoopCount;
                            LOGD(_TAG(TAG), "[createVideoCallThresholdTimerTask]: mTotalLoopCount = %d, wifiCallingMode = %s", \
                                    mTotalLoopCount, ImsConnManUtils::getWfcModeNameById(wfcMode));

                            mVideoCallThresholdTimer = PortingInterface::createTimer(createVideoCallThresholdTimerTaskCallback,
                                    static_cast<void*>(this),
                                    TIMER_INTERVAL,
                                    nsITimer::TYPE_REPEATING_PRECISE_CAN_SKIP);
                        }
                    }
                }
            } else {
                if (ImsConnManStateInfo::isIdleState() && CALL_MODE_NO_CALL == ImsConnManStateInfo::getCallMode()) {
                    if (nullptr == mIdleThresholdTimer) {
                        mIsExistAnyPolicyTimer = true;
                        LOGD(_TAG(TAG), "createPolicyTimerTask: create [Idle] threshold timer task during Wifi and Lte network");

                        if (isQuicklyInitiate && mIsQuicklyCalculateLogic) {
                            mTotalLoopCount = mQuicklyCalculateCount;
                        } else {
                            mTotalLoopCount = mVowifiThresholdLoopCount;
                        }

                        LOGD(_TAG(TAG), "[createIdleThresholdTimerTaskWithoutIms]: mTotalLoopCount = %d, wifiCallingMode = %s", \
                                mTotalLoopCount, ImsConnManUtils::getWfcModeNameById(wfcMode));

                        mIdleThresholdTimer = PortingInterface::createTimer(createIdleThresholdTimerTaskWithoutImsCallback,
                                static_cast<void*>(this),
                                TIMER_INTERVAL,
                                nsITimer::TYPE_REPEATING_PRECISE_CAN_SKIP);
                    }
                }
            }
        } else {
            if (!isVolteRegistered && !isVowifiRegistered) {
                if (ImsConnManStateInfo::isIdleState() && CALL_MODE_NO_CALL == ImsConnManStateInfo::getCallMode()) {
                    if (nullptr == mIdleThresholdTimer) {
                        mIsExistAnyPolicyTimer = true;
                        LOGD(_TAG(TAG), "createPolicyTimerTask: create [Idle] threshold timer task during Wifi and 2/3G or UNKNOWN network");

                        if (isQuicklyInitiate && mIsQuicklyCalculateLogic) {
                            mTotalLoopCount = mQuicklyCalculateCount;
                        } else {
                            mTotalLoopCount = mVowifiThresholdLoopCount;
                        }

                        LOGD(_TAG(TAG), "[createIdleThresholdTimerTaskWithoutLte]: mTotalLoopCount = %d, wifiCallingMode = %s", \
                                mTotalLoopCount, ImsConnManUtils::getWfcModeNameById(wfcMode));

                        mIdleThresholdTimer = PortingInterface::createTimer(createIdleThresholdTimerTaskWithoutLteCallback,
                                static_cast<void*>(this),
                                TIMER_INTERVAL,
                                nsITimer::TYPE_REPEATING_PRECISE_CAN_SKIP);
                    }
                }
            } else {
                LOGE(_TAG(TAG), "createPolicyTimerTask: don't create timer task during Wifi or Vowifi and 2/3G or UNKNOWN network");
            }
        }
    } else {
        LOGE(_TAG(TAG), "createPolicyTimerTask: Conditions are not satisfied, don't create timer task, " \
                "isWfcEnabled = %s, isWifiConnected = %s, isLteCellularNetwork = %s", \
                isWfcEnabled ? "true" : "false", \
                isWifiConnected ? "true" : "false", \
                isLteCellularNetwork ? "true" : "false");
    }

    /*vector<int> lteRsrp(7, -110);
    mVectorLteRsrp = lteRsrp;

    vector<int> wifiRssi(7, 0);
    mVectorWifiRssi = wifiRssi;

    calculateSignalAverage();


    int arr_1[] = {0, 10, 30, 50, 10, 0, 50};
    int arr_2[] = {-100, -90, -87, -60, -110, -60, -110};
    int arr_3[] = {-100, 10, 0, -87, -60, 50, 100};
    for (uint32_t i = 0; i < sizeof(arr_1)/sizeof(int); i++) {
        mVectorAudioQosLoss.push_back(arr_1[i]);
        mVectorAudioQosJitter.push_back(arr_2[i]);
        mVectorAudioQosRtt.push_back(arr_3[i]);
    }

    calculateAudioQosAverage();*/
}

void ImsConnManPolicy::triggerPunitiveLogic() {
    if (mIsPunitiveLogic) {
        if (ImsConnManStateInfo::isWifiCallingEnabled()
                && ImsConnManStateInfo::isWifiConnected()
                && ImsConnManStateInfo::isLteCellularNetwork()
                && !ImsConnManStateInfo::isIdleState()) {
            mVolteSpecialLoopCount += mPunitiveIntervalTime;
            if (mVolteSpecialLoopCount >= mPunitiveMaxTime) {
                mVolteSpecialLoopCount = mPunitiveMaxTime;
            }

            LOGD(_TAG(TAG), "triggerPunitiveLogic: %s punitive logic, mVolteSpecialLoopCount = %d", \
                    mIsPunitiveLogic ? "enabled" : "disabled", mVolteSpecialLoopCount);
        }
    }
}

void ImsConnManPolicy::resetVolteSpecialLoopCount() {
    mVolteSpecialLoopCount = mVolteThresholdLoopCount;
}

bool ImsConnManPolicy::isPopupErrorNotification() {
    return mIsPopupErrorNotification;
}

int ImsConnManPolicy::getPopupErrorRetryCount() {
    return mPopupErrorRetryCount;
}

bool ImsConnManPolicy::isExistAnyPolicyTimer() {
    LOGD(_TAG(TAG), "isExistAnyPolicyTimer = %s", mIsExistAnyPolicyTimer ? "true" : "false");
    return mIsExistAnyPolicyTimer;
}

END_VOWIFI_NAMESPACE
