#include "ImsConnManStateInfo.h"
#include "Constants.h"
#include "portinglayer/PortingConstants.h"


BEGIN_VOWIFI_NAMESPACE

bool ImsConnManStateInfo::mIsServiceAttached = false;
bool ImsConnManStateInfo::mIsWifiConnected = false;
bool ImsConnManStateInfo::mIsWifiCallingEnabled = false;
int ImsConnManStateInfo::mWifiCallingMode = WFC_MODE_UNKNOWN;
bool ImsConnManStateInfo::mIsVolteRegistered = false;
bool ImsConnManStateInfo::mIsVowifiRegistered = false;
int ImsConnManStateInfo::mCallState = CALL_STATE_IDLE;
int ImsConnManStateInfo::mCallMode = CALL_MODE_NO_CALL;
bool ImsConnManStateInfo::mIsImsCallActive = false;
bool ImsConnManStateInfo::mIsRecValidVowifiRtp = false;
bool ImsConnManStateInfo::mIsStartActivateVolte = false;
bool ImsConnManStateInfo::mIsMessageQueuePending = false;
int ImsConnManStateInfo::mPendingMessageId = MSG_DEFAULT_ID;
bool ImsConnManStateInfo::mIsReadAirplaneMode = false;
bool ImsConnManStateInfo::mIsReceivedIccInfoChanged = false;

ImsConnManStateInfo::ImsConnManStateInfo() {
}

ImsConnManStateInfo::~ImsConnManStateInfo() {
}

void ImsConnManStateInfo::setServiceAttached(bool isAttached) {
    mIsServiceAttached = isAttached;
}

bool ImsConnManStateInfo::isServiceAttached() {
    return mIsServiceAttached;
}

void ImsConnManStateInfo::setWifiConnected(bool isConnected) {
    mIsWifiConnected = isConnected;
}

bool ImsConnManStateInfo::isWifiConnected() {
    return mIsWifiConnected;
}

void ImsConnManStateInfo::setWifiCallingEnabled(bool isEnabled) {
    mIsWifiCallingEnabled = isEnabled;
}

bool ImsConnManStateInfo::isWifiCallingEnabled() {
    return mIsWifiCallingEnabled;
}

void ImsConnManStateInfo::setWifiCallingMode(int mode) {
    mWifiCallingMode = mode;
}

int ImsConnManStateInfo::getWifiCallingMode() {
    return mWifiCallingMode;
}

void ImsConnManStateInfo::setVolteRegistered(bool isRegistered) {
    mIsVolteRegistered = isRegistered;
}

bool ImsConnManStateInfo::isVolteRegistered() {
    return mIsVolteRegistered;
}

void ImsConnManStateInfo::setVowifiRegistered(bool isRegistered) {
    mIsVowifiRegistered = isRegistered;
}

bool ImsConnManStateInfo::isVowifiRegistered() {
    return mIsVowifiRegistered;
}

bool ImsConnManStateInfo::isAirplaneModeEnabled() {
    return PortingInterface::isAirplaneModeEnabled();
}

bool ImsConnManStateInfo::isLteCellularNetwork() {
    return PortingInterface::isLteMobileNetwork();;
}

void ImsConnManStateInfo::setCallState(int callState) {
    mCallState = callState;
}

int ImsConnManStateInfo::getCallState() {
    return mCallState;
}

bool ImsConnManStateInfo::isIdleState() {
    return (CALL_STATE_IDLE == getCallState());
}

void ImsConnManStateInfo::setCallMode(int callMode) {
    mCallMode = callMode;
}

int ImsConnManStateInfo::getCallMode() {
    return mCallMode;
}

int ImsConnManStateInfo::getImsCallType() {
    return ImsConnManUtils::getCurrentImsCallType();
}

void ImsConnManStateInfo::setImsCallActive(bool isActive) {
    mIsImsCallActive = isActive;
}

bool ImsConnManStateInfo::isImsCallActive() {
    return mIsImsCallActive;
}

void ImsConnManStateInfo::setRecValidVowifiRtp(bool isReceived) {
    mIsRecValidVowifiRtp = isReceived;
}

bool ImsConnManStateInfo::isRecValidVowifiRtp() {
    return mIsRecValidVowifiRtp;
}

void ImsConnManStateInfo::setStartActivateVolte(bool isStart) {
    mIsStartActivateVolte = isStart;
}

bool ImsConnManStateInfo::isStartActivateVolte() {
    return mIsStartActivateVolte;
}

void ImsConnManStateInfo::setMessageQueuePendingState(bool isPending) {
    mIsMessageQueuePending = isPending;
}

bool ImsConnManStateInfo::isMessageQueuePending() {
    return mIsMessageQueuePending;
}

void ImsConnManStateInfo::setPendingMessageId(int msgId) {
    mPendingMessageId = msgId;
}

int ImsConnManStateInfo::getPendingMessageId() {
    return mPendingMessageId;
}

void ImsConnManStateInfo::setReadAirplaneModeState(bool isRead) {
    mIsReadAirplaneMode = isRead;
}

bool ImsConnManStateInfo::isReadAirplaneMode() {
    return mIsReadAirplaneMode;
}

void ImsConnManStateInfo::setReceivedIccInfoChangedState(bool isReceived) {
    mIsReceivedIccInfoChanged = isReceived;
}

bool ImsConnManStateInfo::isReceivedIccInfoChanged() {
    return mIsReceivedIccInfoChanged;
}

END_VOWIFI_NAMESPACE
