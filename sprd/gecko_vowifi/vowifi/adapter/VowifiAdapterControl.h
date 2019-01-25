/************************************************************************

        Copyright (c) 2005-2017 by Spreadtrum, Inc.
                       All rights reserved.

     This software is confidential and proprietary to Spreadtrum,
     Inc. No part of this software may be reproduced, stored, transmitted,
     disclosed or used in any form or by any means other than as expressly
     provided by the written license agreement between Spreadtrum and its
     licensee.

************************************************************************/


#ifndef mozilla_dom_vowifiadapter_VowifiSocketControl_h
#define mozilla_dom_vowifiadapter_VowifiSocketControl_h

#include "socket/VowifiConnClient.h"
#include "socket/VowifiMsgHandler.h"
#include "VowifiSecurityManager.h"
#include "VowifiRegisterManager.h"
#include "VowifiCallManager.h"
#include "VowifiSmsManager.h"
#ifndef nsThreadUtils_h__
#include "nsThreadUtils.h"
#endif
#include <string>
#include <unistd.h>

#include "nsIVowifiCallback.h"
#include "nsIRadioInterfaceLayer.h"

static bool gHandleMsgCompleted = false;

namespace mozilla {
namespace dom {
namespace vowifiadapter {

class VowifiAdapterControl: public VowifiMsgHandler
                          , public SecurityListener
                          , public RegisterListener
                          , public CallListener
                          , public SmsListener
{
public:
    VowifiAdapterControl();
    void Init();
    bool SendToService(std::string &noticeStr);
    /* handle received socket message */
    void HandlerMsg();
    void HandlerMsgInMainThread();
    ~VowifiAdapterControl();
    void Attach();
    void Deattach(bool isHandover);
    void ResetRegisterState(int newState);
    void ResetAll();
    void SecurityForceStop();
    void Register();
    void Reregister(int networkType, string& networkInfo);
    void Deregister();
    void RegisterFailed();
    void RegisterLogin();
    void RegisterLogout(int stateCode);
    bool RegisterForceStop();
    void TerminateCalls(int wifiState);

    bool StartEmergencyCall(string& number);
    bool StartNormalCall(string& number);
    bool CallStart(string& number, bool isEmergency);
    bool CallTerminate(int callIndex);
    bool CallReject();
    bool CallAccept();
    bool CallSwitch(nsIRilSendWorkerMessageCallback *callback);
    bool CallHangUpForeground();
    bool CallHangUpBackground();
    bool CallMute(bool isMute);
    bool ConferenceCall();
    void UpdateDataRouterState(int dataRouterState);
    void SetVolteUsedLocalAddr(string imsregaddr);
    void GetCurLocalAddress(string& localAddr);
    void GetCurPcscfAddress(string& pcscfAddr);
    void GetCallCount(short int *_retval);
    bool SendVowifiSms(nsIRilSendWorkerMessageCallback *callback, string& receiver, int retry, int messageRef, int serial, string& text, int msgMaxSeq, int msgSeq);
    bool SetSmscAddress(string smsc);
    bool GetSmscAddress(string &smsc);
    bool AckSms(int result);


    //securityListener
    void OnProgress(int state);
    void OnSuccessed();
    void OnFailed(int reason);
    void OnStopped(bool forHandover, int errorCode);

    //registerListener
    void OnReregisterFinished(bool success, int errorCode);
    void OnLoginFinished(bool success, int stateCode, int retryAfter);
    void OnLogout(int stateCode);
    void OnPrepareFinished(bool success, int errorCode);
    void OnRegisterStateChanged(int newState);

    //CallListener

    void OnCallIncoming(int callIndex, uint16_t callType);
    void OnCallIsEmergency();
    void OnCallEnd();
    void OnHoldResumeOK();
    void OnHoldResumeFailed();
    void OnCallRTPReceived(bool isVideoCall, bool isReceived);
    void OnCallRTCPChanged(bool isVideoCall, int lose, int jitter, int rtt);
    void OnAliveCallUpdate(bool isVideoCall);
    void OnEnterECBM();
    void OnExitECBM();
    void OnCallStateChanged(uint32_t alength);
    void OnNotifyError(string error);
    //SmsListener
    void OnSmsIncoming(int callIndex, uint16_t callType);
    string mImei;
    VowifiRegisterManager* mRegisterMgr = nullptr;
    int mResetStep = 0;
    nsCOMPtr<nsIVowifiCallback> mCallback = nullptr;
    nsIRilSendWorkerMessageCallback* mSwitchCallback = nullptr;
private:
    nsCOMPtr<nsIThread> mWorkerThread;
    VowifiConnClient* mConnClient = nullptr;
    VowifiSecurityManager* mSecurityMgr = nullptr;
    VowifiSmsManager* mSmsMgr = nullptr;
    VowifiCallManager* mCallMgr = nullptr;
    nsCOMPtr<nsIRunnable> mResultRunnable;
    SIMAccountInfo* mSimAccountInfo = nullptr;
    RegisterIPAddress* mRegisterIP;
    bool mNeedResetAfterCall = false;
    int mCmdRegisterState = CMD_STATE_INVALID;


};

class SocketProcessingResultTask : public nsRunnable{
public:
    SocketProcessingResultTask(VowifiAdapterControl* adapterControl)
    : mWorkerThread(do_GetCurrentThread())
    , mAdapterControl(adapterControl)
    {
        MOZ_ASSERT(!NS_IsMainThread());
    }

    NS_IMETHOD Run() {
        MOZ_ASSERT(NS_IsMainThread());
        mAdapterControl->HandlerMsgInMainThread();
        return NS_OK;
    }

private:
    nsCOMPtr<nsIThread> mWorkerThread;
    VowifiAdapterControl* mAdapterControl;

};

class SocketProcessingAsyncTask : public nsRunnable {
public:
    SocketProcessingAsyncTask(VowifiConnClient* connClient,
                              VowifiAdapterControl* adapterCtrl)
    :mConnClient(connClient)
    ,mAdapterControl(adapterCtrl)
    {
        LOG("Entering SocketProcessingTask");
    }

    NS_IMETHOD Run() {
        mConnClient->InitConnect();
        do{
            /* the socket poll loop */
            mConnClient->ReadLoop();
            mConnClient->ReConnect();
            // postpone 0.5 sec to reconnect to avoid system impact
            usleep(500 * 1000);
            if (mConnClient->IsConnected())
            {
                // Bug 971239, once socket connected, ResetAll in force
                // to make sure VowifiService on initial state
                mAdapterControl->mResetStep = RESET_STEP_DEREGISTER;
                mAdapterControl->ResetRegisterState(RegisterState::STATE_IDLE);
                mAdapterControl->ResetAll();
            }
        }while(1);

        return NS_OK;
    }

private:
    VowifiConnClient* mConnClient;
    VowifiAdapterControl* mAdapterControl;

protected:
    ~SocketProcessingAsyncTask() {
    }
};

} // namespace vowifiadapter
} // namespace dom
} // namespace mozilla

#endif // mozilla_dom_vowifiadapter_VowifiSocketControl_h
