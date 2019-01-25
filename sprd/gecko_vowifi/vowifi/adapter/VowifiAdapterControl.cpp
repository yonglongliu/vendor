/************************************************************************

 Copyright (c) 2005-2017 by Spreadtrum, Inc.
 All rights reserved.

 This software is confidential and proprietary to Spreadtrum,
 Inc. No part of this software may be reproduced, stored, transmitted,
 disclosed or used in any form or by any means other than as expressly
 provided by the written license agreement between Spreadtrum and its
 licensee.

 ************************************************************************/

#include "VowifiAdapterControl.h"
#include "json/json.h"
#include "nsXPCOMCIDInternal.h"
#ifndef __gen_nsIThreadManager_h__
#include "nsIThreadManager.h"
#endif

#ifndef __gen_nsIEventTarget_h__
#include "nsIEventTarget.h"
#endif
#ifndef nsThreadUtils_h__
#include "nsThreadUtils.h"
#endif

#ifndef __gen_nsISupports_h__
#include "nsISupports.h"
#endif
#include <string>
#include <unistd.h>

using namespace std;
namespace mozilla {
namespace dom {
namespace vowifiadapter {

//static int gThreadLocked = 0;
//static void GetLock(){
//    gThreadLocked = 1;
//}
//static void ReleaseLock(){
//    gThreadLocked = 0;
//}

//NS_IMPL_ISUPPORTS(SocketProcessingTask, nsIRunnable)

VowifiAdapterControl::VowifiAdapterControl() {
    LOG("Entering VowifiAdapterControl initilization");

    mConnClient = new VowifiConnClient();
    if (nullptr == mConnClient) {
        LOGEA("VowifiConnClient create failed");
        return;
    }
    mSecurityMgr = new VowifiSecurityManager(this);
    if (nullptr == mSecurityMgr) {
        LOGEA("VowifiSecurityManager create failed");
        return;
    }

    mRegisterMgr = new VowifiRegisterManager(this);
    if (nullptr == mRegisterMgr) {
        LOGEA("VowifiRegisterManager create failed");
        return;
    }

    mCallMgr = new VowifiCallManager(this);
    if (nullptr == mCallMgr) {
        LOGEA("VowifiCallManager create failed");
        return;
    }
   
    mSmsMgr = new VowifiSmsManager(this);
    if (nullptr == mCallMgr) {
        LOGEA("VowifiCallManager create failed");
        return;
    }

    mResultRunnable = new SocketProcessingResultTask(this);

    mConnClient->SetMsgHandler(this);

    LOG("going to SocketProcess thread creation");
    nsresult rv;
    nsCOMPtr<nsIThreadManager> tm = do_GetService(NS_THREADMANAGER_CONTRACTID);
    rv = tm->NewThread(0, 0, getter_AddRefs(mWorkerThread));
    if (NS_SUCCEEDED(rv)) {
        LOG("SocketProcess thread creation succeed");
        nsCOMPtr<nsIRunnable> msgTask = new SocketProcessingAsyncTask(
                                                        mConnClient, this);
        mWorkerThread->Dispatch(msgTask, nsIEventTarget::DISPATCH_NORMAL);
        LOG("SocketProcessingTask dispatched");

        nsIThread *thread = NS_GetCurrentThread();
        // Wait for socket connect complete.
        while (!mConnClient->IsConnected()) {
            LOG("socket still not connetcted, handle next event");
            NS_ProcessNextEvent(thread);
        }
    } else {
        LOGEA("SocketProcess thread creation failed");
    }

    Init();
}

void VowifiAdapterControl::Init() {
    mSecurityMgr->RegisterListeners(this);
    mRegisterMgr->RegisterListeners(this);
    mCallMgr->RegisterListeners(this);
    mSmsMgr->RegisterListeners(this);

    // Bug 971239, once socket connected, ResetAll in force
    // to make sure VowifiService on initial state
    this->mResetStep = RESET_STEP_DEREGISTER;
    this->ResetAll();
}

bool VowifiAdapterControl::SendToService(string &noticeStr) {
    if (mConnClient == nullptr) {
        LOGEA("mConnClient is null, need to be checked");
        return false;
    }
    if (mConnClient->IsConnected())
        return mConnClient->Send(noticeStr.c_str(), noticeStr.length());
    else {
        LOGWA("socket still not connetcted, please wait");
        sleep(1);  //debug, TBD
        if (mConnClient->IsConnected())
            return mConnClient->Send(noticeStr.c_str(), noticeStr.length());
        else {
            LOGEA("SendToService error");
            return false;
        }
    }
}

void VowifiAdapterControl::HandlerMsgInMainThread() {
    LOG("--> AdapterControl HandlerMsg: %s", mBuffer_);
    Json::Value root;
    Json::Reader reader;

    string currCmd;
    while (GetNextCmd(currCmd)) {
        if (!reader.parse(currCmd.c_str(), root)) {
            LOGEA("HandlerMsg: the Json input parse failed!");
            gHandleMsgCompleted = true;
            return;
        }
        string groupId = root[JSON_GROUP_ID].asString();

        int eventId = root[JSON_EVENT_ID].asInt();
        string eventName = root[JSON_EVENT_NAME].asString();

        Json::Value argums;
        if (root.isMember(JSON_ARGS)) {

            argums = root[JSON_ARGS];
        }

        if (groupId.compare(EVENT_GROUP_CALL) == 0) {
            if (mCallMgr != nullptr) {
                mCallMgr->HandleEvent(eventId, eventName, argums);
            }
        } else if (groupId.compare(EVENT_GROUP_REG) == 0) {
            if (mRegisterMgr != nullptr) {
                mRegisterMgr->OnRegisterStateChanged(eventId, argums);
            }
        } else if (groupId.compare(EVENT_GROUP_SMS) == 0) {
            if (mSmsMgr != nullptr) {
                mSmsMgr->HandleEvent(eventId, eventName, argums);
            }
        } else if (groupId.compare(EVENT_GROUP_S2B) == 0) {
            if (mSecurityMgr != nullptr) {
                mSecurityMgr->OnJsonCallback(eventId, argums);
            }
        } else {
            LOGWA("HandlerMsg: groupId=%s, eventId=%d is unknown!",
                    groupId.c_str(), eventId);
        }
    };
    ClearBuffer();
    gHandleMsgCompleted = true;
}

void VowifiAdapterControl::HandlerMsg() {
    gHandleMsgCompleted = false;

    NS_DispatchToMainThread(mResultRunnable);
    // Wait for the cleanup thread to complete.
    while(!gHandleMsgCompleted) {
        usleep(50);
    }
    return;

}

void VowifiAdapterControl::Attach() {
    mSimAccountInfo = SIMAccountInfo::Generate(0);
    if (mSecurityMgr != nullptr) {
        mSecurityMgr->Attach(mSimAccountInfo);

    }
}

void VowifiAdapterControl::Deattach(bool isHandover) {
    if (mSecurityMgr != nullptr) {
        mSecurityMgr->Deattach(isHandover);
    }
}

void VowifiAdapterControl::SecurityForceStop() {
    if (mSecurityMgr != nullptr) {
        mSecurityMgr->ForceStop();
    }
}

void VowifiAdapterControl::ResetRegisterState(int newState)
{
    if (mRegisterMgr != nullptr)
        mRegisterMgr->UpdateRegisterState(newState);
}

void VowifiAdapterControl::ResetAll() {
    LOG("Reset the security and sip stack");
    // Update the register state to call manager as it will be reset later.
    if (mCallMgr != nullptr)
        mCallMgr->UpdateRegisterState(RegisterState::STATE_IDLE);  //TBD
    // Before reset the sip stack, we'd like to terminate all the calls.
    TerminateCalls(WifiState::CONNECTED);
    if (mResetStep == RESET_STEP_DEREGISTER) {
        if (mRegisterMgr->GetCurRegisterState()
                == RegisterState::STATE_CONNECTED) {
            Deregister();

        } else {
            // As do not register, we'd like to force reset.
            LOG("Do not register now, transfer to force reset.");
            // Force stop the register and s2b.
            mResetStep = RESET_STEP_FORCE_RESET;
            RegisterForceStop();
            SecurityForceStop();
        }
    } else if (mResetStep == RESET_STEP_DEATTACH) {
        // De-attach from the EPDG.
        Deattach(false);
    } else {
        mResetStep = RESET_STEP_INVALID;
        LOG("Shouldn't be here. reset step is: %d ", mResetStep);
    }
}

void VowifiAdapterControl::Register() {
    //GetLock();
    mCmdRegisterState = CMD_STATE_PROGRESS;
    if (mSecurityMgr != nullptr) {
        // Check the security state, if it is attached. We could start the register process.
        if (mSecurityMgr->GetCurSecurityState() != AttachState::STATE_CONNECTED
                && mSecurityMgr->GetConfig() == nullptr) {
            LOG("Please wait for attach success. Current attach state: %d",
                    mSecurityMgr->GetCurSecurityState());
            if (mCallback != nullptr)   //TBD
                mCallback->OnReregisterFinished(false, 0);
            //ReleaseLock();
            return;
        }
    }
    if (mRegisterMgr != nullptr) {
        mRegisterMgr->PrepareForLogin(mSimAccountInfo);
    }
    LOG("release the thread lock");
    //ReleaseLock();
}

void VowifiAdapterControl::Reregister(int networkType, string& networkInfo){
    if (mRegisterMgr != nullptr) {
        mRegisterMgr->Reregister(networkType, networkInfo);
    }
}

void VowifiAdapterControl::Deregister() {
    TerminateCalls(WifiState::CONNECTED);
    if (mRegisterMgr != nullptr) {
        mRegisterMgr->Deregister();
    }
}

bool VowifiAdapterControl::RegisterForceStop() {
    return mRegisterMgr->ForceStop();
}

void VowifiAdapterControl::RegisterFailed() {
    mCmdRegisterState = CMD_STATE_INVALID;
    mRegisterIP = nullptr;
    if(mCallback != nullptr){
        mCallback->OnRegisterStateChanged(mRegisterMgr->GetCurRegisterState(), 0);
    }
    mRegisterMgr->ForceStop();
}

void VowifiAdapterControl::RegisterLogin() {

    LOG("Entering RegisterLogin.");

    if (mRegisterIP == nullptr) {
        // Can not get the register IP.
        LOGEA("Failed to login as the register IP is null.");
        RegisterFailed();
        return;
    }

    bool startRegister = false;
    uint32_t regVersion = mRegisterIP->GetValidIPVersion(
            mSecurityMgr->GetConfig()->_prefIPv4);
    if (regVersion != IPVersion::NONE) {
        bool useIPv4 = (regVersion == IPVersion::IP_V4);
        if (regVersion == mSecurityMgr->GetConfig()->_useIPVersion
                || mSecurityMgr->SetIPVersion(regVersion)) {
            mSecurityMgr->GetConfig()->_useIPVersion = regVersion;
            startRegister = true;
            string localIP = mRegisterIP->GetLocalIP(useIPv4);
            string pcscfIP = mRegisterIP->GetPcscfIP(useIPv4);
            mRegisterMgr->Login(useIPv4, localIP, pcscfIP);
        }
    }

    if (!startRegister) {
        // The IPv6 & IPv4 invalid, it means login failed. Update the result.
        // Can not get the register IP.
        RegisterFailed();
    }
}

void VowifiAdapterControl::RegisterLogout(int stateCode) {
    mCmdRegisterState = CMD_STATE_INVALID;
    mRegisterIP = nullptr;
    if (mCallback != nullptr) {
        mCallback->OnRegisterStateChanged(RegisterState::STATE_IDLE, 0);
        mCallback->OnUnsolicitedUpdate(
                stateCode == NativeErrorCode::REG_TIMEOUT ?
                        UnsolicitedCode::SIP_TIMEOUT :
                        UnsolicitedCode::SIP_LOGOUT);
    }

    mRegisterMgr->ForceStop();
}

void VowifiAdapterControl::TerminateCalls(int wifiState) {

    if (mCallMgr != nullptr) {
        mCallMgr->TerminateCalls(wifiState);
    }

}

bool VowifiAdapterControl::StartEmergencyCall(string& number) {
    //TBD
    return true;
}

bool VowifiAdapterControl::StartNormalCall(string& number) {
    if (mCallMgr != nullptr) {
        return mCallMgr->Start(number, false);
    }
    return false;
}

bool VowifiAdapterControl::CallStart(string& number, bool isEmergency) {
    if (mCallMgr != nullptr) {
        if (!mCallMgr->IsCallFunEnabled() || number.empty()) {
            LOG("vowifi is not registered or the number is null, call start failed");
            return false;
        }
    }
    if (isEmergency) {
        return StartEmergencyCall(number);
    } else {
        return StartNormalCall(number);
    }

}

bool VowifiAdapterControl::CallTerminate(int callIndex) {
    if (mCallMgr != nullptr) {
        return mCallMgr->Terminate(callIndex);
    } else {
        return false;
    }
}

bool VowifiAdapterControl::CallReject() {
    if (mCallMgr != nullptr) {
        return mCallMgr->Reject();
    } else {
        return false;
    }
}

bool VowifiAdapterControl::CallAccept() {
    if (mCallMgr != nullptr) {
        return mCallMgr->Accept();
    } else {
        return false;
    }
}

bool VowifiAdapterControl::CallSwitch(
        nsIRilSendWorkerMessageCallback *callback) {
    mSwitchCallback = callback;
    if (mCallMgr != nullptr) {
        return mCallMgr->Switch();
    } else {
        return false;
    }
}

bool VowifiAdapterControl::CallHangUpForeground() {
    if (mCallMgr != nullptr) {
        return mCallMgr->HangUpForeground();
    } else {
        return false;
    }
}

bool VowifiAdapterControl::CallHangUpBackground() {
    if (mCallMgr != nullptr) {
        return mCallMgr->HangUpBackground();
    } else {
        return false;
    }
}

bool VowifiAdapterControl::CallMute(bool isMute) {
    if (mCallMgr != nullptr) {
        return mCallMgr->Mute(isMute);
    } else {
        return false;
    }
}

bool VowifiAdapterControl::ConferenceCall() {
    if (mCallMgr != nullptr) {
        return mCallMgr->ConferenceCall(false);
    } else {
        return false;
    }
}


bool VowifiAdapterControl::SendVowifiSms(nsIRilSendWorkerMessageCallback *callback, string& reciever, int retry, int messageRef, int serial, string& text, int msgMaxSeq, int msgSeq)
{
    if (mSmsMgr != nullptr) {
        return mSmsMgr->SendVowifiSms(callback, reciever, retry, messageRef, serial, text, msgMaxSeq, msgSeq);
    } else {
        return false;
    }
}


bool VowifiAdapterControl::SetSmscAddress(string smsc)
{
    if (mSmsMgr != nullptr) {
        return mSmsMgr->SetSmscAddress(smsc);
    } else {
        return false;
    }
}


bool VowifiAdapterControl::GetSmscAddress(string &smsc)
{
    if (mSmsMgr != nullptr) {
        return mSmsMgr->GetSmscAddress(smsc);
    } else {
        return false;
    }
}


bool VowifiAdapterControl::AckSms(int result)
{
    if (mSmsMgr != nullptr) {
        return mSmsMgr->AckSms(result);
    } else {
        return false;
    }
}



void VowifiAdapterControl::UpdateDataRouterState(int dataRouterState){
    if (mCallMgr != nullptr) {
         mCallMgr->UpdateDataRouterState(dataRouterState);
    }
    if (mCallback != nullptr) {
        mCallback->OnUpdateDRStateFinished();
    }

}

void VowifiAdapterControl::SetVolteUsedLocalAddr(string imsregaddr){
    LOG("VowifiAdapterControl :: SetVolteUsedLocalAddr has been called" );
    if (mSecurityMgr != nullptr) {
        mSecurityMgr->SetVolteUsedLocalAddr(imsregaddr);
    }
}

void VowifiAdapterControl::GetCurLocalAddress(string& localAddr){
    if (mRegisterIP != nullptr
            && mRegisterMgr != nullptr
            && mRegisterMgr->GetCurRegisterState() == RegisterState::STATE_CONNECTED) {
        localAddr = mRegisterIP->GetCurUsedLocalIP();
        LOG("GetCurLocalAddress localAddr is %s", localAddr.c_str());
        return;
    }
    localAddr = "";
}

void VowifiAdapterControl::GetCurPcscfAddress(string& pcscfAddr){
    if (mRegisterIP != nullptr
            && mRegisterMgr != nullptr
            && mRegisterMgr->GetCurRegisterState() == RegisterState::STATE_CONNECTED) {
        pcscfAddr = mRegisterIP->GetCurUsedPcscfIP();
        LOG("GetCurPcscfAddress pcscfAddr is %s", pcscfAddr.c_str());
        return;
    }
    pcscfAddr = "";
}

void VowifiAdapterControl::GetCallCount(short int *_retval){
    *_retval = (short int)(mCallMgr == nullptr ? 0 :mCallMgr->GetCallCount());
}


//SecurityListener
void VowifiAdapterControl::OnProgress(int state) {
    if (mCallback != nullptr) {
        mCallback->OnAttachStateChanged(state);
    }
}

void VowifiAdapterControl::OnSuccessed() {
    // Register(); //for debug
    if (mCallback != nullptr) {
        mCallback->OnAttachFinished(true, 0);
    }
}

void VowifiAdapterControl::OnFailed(int reason) {
    if (mCallback != nullptr) {
        mCallback->OnAttachFinished(false, reason);
        if ((unsigned int) mResetStep >= RESET_STEP_DEATTACH
                && reason == NativeErrorCode::IKE_INTERRUPT_STOP) {
            LOG("Attached failed cased by interrupt. It means reset finished.");
            mResetStep = RESET_STEP_INVALID;
            mCallback->OnResetFinished(Result::SUCCESS, 0);
        }
    }
}

void VowifiAdapterControl::OnStopped(bool forHandover, int errorCode) {
    // If the stop action is for reset, we will notify the reset finished result.
    if (!forHandover && mResetStep != RESET_STEP_INVALID) {
        if (mResetStep == RESET_STEP_DEREGISTER) {
            // It means the de-register do not finished. We'd like to force stop register.
            RegisterForceStop();
        }
        LOG(
                "S2b stopped, it means the reset action finished. Notify the result.");
        mResetStep = RESET_STEP_INVALID;
        if (mCallback != nullptr) {
            mCallback->OnResetFinished(Result::SUCCESS, 0);
        }
    } else if (mCallback != nullptr) {
        LOG("OnAttachStopped called");
        mCallback->OnAttachStopped(errorCode);
        if (errorCode == NativeErrorCode::DPD_DISCONNECT) {
            mCallback->OnUnsolicitedUpdate(
                    UnsolicitedCode::SECURITY_DPD_DISCONNECTED);
        } else if (errorCode == NativeErrorCode::IPSEC_REKEY_FAIL
                || errorCode == NativeErrorCode::IKE_REKEY_FAIL) {
            mCallback->OnUnsolicitedUpdate(
                    UnsolicitedCode::SECURITY_REKEY_FAILED);
        } else {
            mCallback->OnUnsolicitedUpdate(UnsolicitedCode::SECURITY_STOP);
        }
    }
}

//registerListener
void VowifiAdapterControl::OnReregisterFinished(bool success, int errorCode) {
    if (success) {
        if (mCallback != nullptr) {
            mCallback->OnReregisterFinished(success, errorCode);
            //Bug#940481 callback to ImsService to update registration state
            mCallback->OnRegisterStateChanged(mRegisterMgr->GetCurRegisterState(), errorCode);
        }
    } else {
        int callCount = mCallMgr->GetCallCount();
        if (callCount > 0) {
            // It means there is call, and we'd like to notify as register logout after
            // all the call ends.
            mNeedResetAfterCall = true;
        } else {
            // There isn't any calls. As get the re-register failed, we'd like to notify
            // as register logout now.
            errorCode =
                    (errorCode == NativeErrorCode::REG_EXPIRED_TIMEOUT) ?
                            NativeErrorCode::REG_TIMEOUT : errorCode;
            RegisterLogout(errorCode);
        }
    }
}

void VowifiAdapterControl::OnLoginFinished(bool success, int stateCode,
        int retryAfter) {
    if (success || retryAfter > 0) {
        // If login success or get retry-after, notify the register state changed.
        //string number = "1175";
        //CallStart(number, false); //for calling debug
        mCmdRegisterState = CMD_STATE_FINISHED;
        if (mCallback != nullptr) {
            mCallback->OnRegisterStateChanged(
                    mRegisterMgr->GetCurRegisterState(), stateCode);
        }
        // After login success, we need set the video quality.
        //mCallMgr.setVideoQuality(Utilities.getDefaultVideoQuality(mPreferences));  //TBD
    } else if (!success && stateCode == NativeErrorCode::REG_SERVER_FORBIDDEN) {
        // If failed caused by server forbidden, set register failed.
        LOG("Login failed as server forbidden. state code is: %d", stateCode);
        RegisterFailed();
    } else {
        // As the PCSCF address may be not only one. For example, there are two IPv6
        // addresses and two IPv4 addresses. So we will try to login again.
        LOG("Last login action is failed, try to use exist address to login again");
        RegisterLogin();
    }
}
void VowifiAdapterControl::OnLogout(int stateCode) {
    LOG("OnLogout called");
    if (mResetStep != RESET_STEP_INVALID) {
        LOG("Get the register state changed for reset action.");
        if (mResetStep == RESET_STEP_DEREGISTER) {
            // Register state changed caused by reset. Update the step and send the message.
            mResetStep = RESET_STEP_DEATTACH;
            Deattach(false);
        } else {
            // Shouldn't be here, do nothing, ignore this event.
            LOG("Do nothing for reset action ignore this register state change.");
        }
    } else {
        mRegisterIP = nullptr;
        if (mCallback != nullptr) {
            LOG("OnRegisterStateChanged called");
            mCallback->OnRegisterStateChanged(
                    mRegisterMgr->GetCurRegisterState(), 0);
            mCallback->OnUnsolicitedUpdate(
                    stateCode == NativeErrorCode::REG_TIMEOUT ?
                            UnsolicitedCode::SIP_TIMEOUT :
                            UnsolicitedCode::SIP_LOGOUT);
        }
    }
    // As already logout, force stop to reset sip stack.
    mRegisterMgr->ForceStop();
}

void VowifiAdapterControl::OnPrepareFinished(bool success, int errorCode) {
    LOG("Entering OnPrepareFinished");
    if (success) {
        // As prepare success, start the login process now. And try from the IPv6;
        SecurityConfig* config = mSecurityMgr->GetConfig();
        mRegisterIP = new RegisterIPAddress(config->_ip4, config->_ip6,
                config->_pcscf4, config->_pcscf6);
        RegisterLogin();
    } else {
        // Prepare failed, give the register result as failed.
        RegisterFailed();
    }

}

void VowifiAdapterControl::OnRegisterStateChanged(int newState) {
    // If the register state changed, update the register state to call manager.
    if (mCallMgr != nullptr) {
        mCallMgr->UpdateRegisterState(newState);
    }
}

//CallListener

void VowifiAdapterControl::OnCallIncoming(int callIndex, uint16_t callType) {
    if (mCallback != nullptr) {
        mCallback->OnCallIncoming((uint32_t)callIndex, callType);
    }
}

void VowifiAdapterControl::OnCallIsEmergency() { //TBD by phase 2
}

void VowifiAdapterControl::OnCallEnd() {
    if (mCallMgr != nullptr && mCallMgr->GetCallCount() < 1
            && mCallback != nullptr) {
        mCallback->OnAllCallsEnd();

        // Check if need reset after all the calls end.
        if (mNeedResetAfterCall) {
            mNeedResetAfterCall = false;

            // Notify as register logout.
            RegisterLogout(0);
        }
    }
}

void VowifiAdapterControl::OnHoldResumeOK() {
    if (mSwitchCallback != nullptr) {
        mozilla::AutoSafeJSContext cx;
        JSAutoCompartment ac(cx, xpc::PrivilegedJunkScope());
        JS::Rooted<JSObject*> callObj(cx);
        callObj = JS_NewPlainObject(cx);
        bool resultValue = true;
        bool *result = &resultValue;
        JS::Rooted < JS::Value > aMessage(cx, JS::ObjectValue(*callObj));
        mSwitchCallback->HandleResponse(aMessage, result);

    }
    mSwitchCallback = nullptr;  //only callswtich has been called, mSwitchCallback be initialized
}

void VowifiAdapterControl::OnHoldResumeFailed() {
    if (mSwitchCallback != nullptr) {
        mozilla::AutoSafeJSContext cx;
        JSAutoCompartment ac(cx, xpc::PrivilegedJunkScope());
        JS::Rooted<JSObject*> callObj(cx);
        callObj = JS_NewPlainObject(cx);
        bool resultValue = true;
        bool *result = &resultValue;

        JS::Rooted<JSString*> erroString(cx,
                JS_NewStringCopyZ(cx, "SwitchActiveCall failed"));
        JS::Rooted < JS::Value > errorMsg(cx);
        errorMsg.setString(erroString);
        JS_SetProperty(cx, callObj, "errorMsg", errorMsg);

        JS::Rooted < JS::Value > aMessage(cx, JS::ObjectValue(*callObj));
        mSwitchCallback->HandleResponse(aMessage, result);
    }
    mSwitchCallback = nullptr; //only callswtich has been called, mSwitchCallback be initialized
}

void VowifiAdapterControl::OnCallRTPReceived(bool isVideoCall,
        bool isReceived) {
    if (mCallback != nullptr) {
        if (isReceived) {
            mCallback->OnRtpReceived(isVideoCall);
        } else {
            mCallback->OnNoRtpReceived(isVideoCall);
        }
    }
}

void VowifiAdapterControl::OnCallRTCPChanged(bool isVideoCall, int lose,
        int jitter, int rtt) {
    if (mCallback != nullptr) {
        mCallback->OnMediaQualityChanged(isVideoCall, lose, jitter, rtt);
    }
}


void VowifiAdapterControl::OnSmsIncoming(int callIndex, uint16_t callType) {

}

void VowifiAdapterControl::OnAliveCallUpdate(bool isVideoCall) {
//    if (mCallback != nullptr) {
//        mCallback->OnAliveCallUpdate(isVideoCall);
//    }
}

void VowifiAdapterControl::OnEnterECBM() { //TBD by phase 2
}

void VowifiAdapterControl::OnExitECBM() { //TBD by phase 2
}
void VowifiAdapterControl::OnCallStateChanged(uint32_t alength) {

}
void VowifiAdapterControl::OnNotifyError(string error) {

}
VowifiAdapterControl::~VowifiAdapterControl() {
    LOG("VowifiAdapterControl destructor...");

    if (mConnClient) {
        mConnClient->CloseConn();
        delete mConnClient;
        mConnClient = nullptr;
    }
    if (mSecurityMgr) {
        mSecurityMgr->deleteSecurityObject(mSecurityMgr);
        mSecurityMgr = nullptr;
    }

    if (mRegisterMgr) {
        mRegisterMgr->deleteRegisterObject(mRegisterMgr);
        mRegisterMgr = nullptr;
    }
    if (mCallMgr) {
        delete mCallMgr;
        mCallMgr = nullptr;
    }
    if (mSmsMgr) {
        delete mSmsMgr;
        mSmsMgr = nullptr;
    }

    mWorkerThread->Shutdown();

}

} // namespace vowifiadapter
} // namespace dom
} // namespace mozilla
