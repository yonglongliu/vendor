#ifndef __SPRD_VOWIFI_IMS_CONNMAN_SERVICE_H__
#define __SPRD_VOWIFI_IMS_CONNMAN_SERVICE_H__

#include "ImsConnManPolicy.h"
#include "portinglayer/Message.h"
#include "portinglayer/PortingLog.h"
#include <string>

using std::string;
using std::shared_ptr;


BEGIN_VOWIFI_NAMESPACE

class WifiObserver;
class CommonMessageResultTask;
class CommonMessageHandler;
class RtpMessageHandler;
class PingAddressHandler;

class ImsConnManService {
public:
    explicit ImsConnManService();
    virtual ~ImsConnManService();

    void onAttach(int policyId);
    void onDetach();

public:
    void operationSuccessed(int id, int type);
    void operationFailed(int id, string reason, int type);
    void imsCallEndProcess();
    void voltePdnStateChanged(int state);
    void noRtpReceivedProcess(bool isVideoPacket);
    void rtpReceivedProcess(bool isVideoPacket);
    void mediaQualityReceivedProcess(bool isVideoPacket, int loss, int jitter, int rtt);
    void srvccFaildProcess();
    void imsCallTypeChanged();
    void onUnsolicitedVowifiError(int errorCode);

    void handleMessageBegin(const shared_ptr<Message> &msg);
    void handleMessageEnd(const shared_ptr<Message> &msg);

    void onImsRegisterStateChange(int imsBearer);
    void onCallStateChanged(int callState);
    void onServiceStateChanged(string &newMobileNetworkType);

    void onWifiCallingChanged(bool isWfcEnabled);
    void onWifiCallingModeChanged(int wfcMode);

    void onWifiNetworkConnected();
    void onWifiNetworkDisconnected();

private:
    void onProcessDpdDisconnectedError();
    void onProcessSipStackError(int errorCode);
    void onProcessSecurityRekeyError();
    void onProcessEpdgStopError();

    void clearLoopMsgBuffer();
    void clearSwitchVowifiMsgBuffer();
    void processingLoopMessageBegin(int msgArg);
    void processingLoopMessageEnd(int msgArg);
    void handleMessageSwitchToVowifiBegin();
    void handleMessageSwitchToVowifiEnd();
    void handleMessageHandoverToVowifiBegin();
    void handleMessageHandoverToVowifiEnd();
    void handleMessageHandoverToVolteBegin(int msgArg);
    void handleMessageHandoverToVolteEnd(int msgArg);
    void handleMessageHandoverToVolteByTimerBegin();
    void handleMessageHandoverToVolteByTimerEnd();
    void handleMessageReleaseVowifiResourceBegin();
    void handleMessageReleaseVowifiResourceEnd();
    void handleMessageVowifiUnavailableBegin(int msgArg);
    void handleMessageVowifiUnavailableEnd(int msgArg);
    void handleMessageCancelCurrentRequestBegin();
    void handleMessageCancelCurrentRequestEnd();

    bool isMeetInitiateVowifiConditions();
    void disableWfcOrWifiProcess(bool isDisableWifi);

private:
    static const string TAG;

    string mMobileNetworkType;
    shared_ptr<ImsConnManPolicy> mpPolicy;
    nsCOMPtr<nsIRunnable> mpCommonMsgResultTask;
    shared_ptr<CommonMessageHandler> mpHandler;
    shared_ptr<RtpMessageHandler> mpRtpHandler;
    shared_ptr<PingAddressHandler> mpPingAddrHandler;

    int mRequestId;
    int mPendingMsgId;
    bool mIsMsgQueuePending;
    int mMessageArg;
    bool mIsProcessingLoopMsg;
    bool mIsNeedSync;
    static const int SWITCH_VOWIFI_MSG_BUFFER_SIZE = 1;
    vector<int> mVectorLoopMsgBuffer;
    vector<int> mVectorSwitchVowifiMsgBuffer;

    int mSwitchVowifiFailedCount;
    int mSwitchVowifiIpFailedCount;

    bool mIsDpdHandoverToVolte;
    bool mIsEpdgStopHandoverToVolte;
    bool mIsNoRtpHandoverToVolte;
    bool mIsQuicklySwitchVowifi;

    bool mIsVoltePdnDeactive;
    bool mIsWaitVolteRegAfterImsCallEnd;
};

class CommonMessageResultTask : public nsRunnable {
public:
    CommonMessageResultTask(ImsConnManService *pService) {
        mpService.reset(pService);
    }

    void updateMessage(const shared_ptr<Message> &msg) {
        mpMessage = msg;
    }

    NS_IMETHODIMP Run() {
        mpService->handleMessageEnd(mpMessage);
        return NS_OK;
    }

private:
    shared_ptr<ImsConnManService> mpService;
    shared_ptr<Message> mpMessage;
};

class CommonMessageHandler : public MessageHandler {
public:
    void update(ImsConnManService *pService) {
        if (nullptr != pService) {
            mpService.reset(pService);
        } else {
            if (nullptr != mpService) mpService.reset();
        }
    }

    void handleMessage(const shared_ptr<Message> &msg) {
        mpService->handleMessageBegin(msg);
    }

    void dispatchToMainThread(const shared_ptr<Message> &msg,
                              nsCOMPtr<nsIRunnable> &resultTask,
                              unsigned long dispatchFlag = NS_DISPATCH_SYNC) {
        CommonMessageResultTask *p = static_cast<CommonMessageResultTask *>(resultTask.get());
        p->updateMessage(msg);
        NS_DispatchToMainThread(resultTask, NS_DISPATCH_SYNC);
    }

private:
    shared_ptr<ImsConnManService> mpService;
};

class RtpMessageHandler : public MessageHandler {
public:
    void update(ImsConnManService *pService) {
        if (nullptr != pService) {
            mpService.reset(pService);
        } else {
            if (nullptr != mpService) mpService.reset();
        }
    }

    void handleMessage(const shared_ptr<Message> &msg);

private:
    shared_ptr<ImsConnManService> mpService;
};

class PingAddressHandler : public MessageHandler {
public:
    void handleMessage(const shared_ptr<Message> &msg);
};

END_VOWIFI_NAMESPACE

#endif
