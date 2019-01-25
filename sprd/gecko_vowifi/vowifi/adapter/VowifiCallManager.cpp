/************************************************************************

        Copyright (c) 2005-2017 by Spreadtrum, Inc.
                       All rights reserved.

     This software is confidential and proprietary to Spreadtrum,
     Inc. No part of this software may be reproduced, stored, transmitted,
     disclosed or used in any form or by any means other than as expressly
     provided by the written license agreement between Spreadtrum and its
     licensee.

************************************************************************/

#include "VowifiCallManager.h"
#include "VowifiAdapterControl.h"
#include "mozilla/dom/telephony/TelephonyCallInfo.h"
#include "nsITelephonyService.h"
#include "socket/VowifiArguments.h"
#include <sstream>
//#include "nsServiceManagerUtils.h"
#include "json/json.h"

using namespace std;
using mozilla::dom::telephony::TelephonyCallInfo;

namespace mozilla {
namespace dom {
namespace vowifiadapter {

PendingAction::PendingAction(ActionType_T origType,
                             bool prevConf,
                             bool pendConf,
                             ActionType_T prevType,
                             ActionType_T pendType,
                             int prevId,
                             int pendIndex)
:_mOrigActionT(origType)
,_mPrevIsConf(prevConf)
,_mPrevId(prevId)
,_mPrevActionT(prevType)
,_mPendIsConf(pendConf)
,_mPendIndex(pendIndex)
,_mPendActionT(pendType)
{
}

VowifiCallManager::VowifiCallManager(VowifiAdapterControl* adapterControl)
:mAdapterControl(adapterControl)
,mRegisterState(RegisterState::STATE_IDLE)
,mIsAlive(false)
,mAudioStart(false)
,mUseAudioStreamCount(0)
,mIncomingCallAction(IncomingCallAction::NORMAL)
{
    // mTelephonyService = do_GetService(TELEPHONY_SERVICE_CONTRACTID);
    LOG("VowifiCallManager constructor initialize");
    mConfCallInfo = {0, true};
}

void VowifiCallManager::RegisterListeners(CallListener* listener) {
    LOG("VowifiCallManager Register listener");
    if (listener == nullptr) {
        LOGEA("Can not register the callback as it is null.");
        return;
    }
    mListener = listener;
}

void VowifiCallManager::UnregisterListeners() {
    LOG("VowifiCallManager UnRegister listener");
    mListener = nullptr;
}

bool VowifiCallManager::IsCallFunEnabled() {
    return mRegisterState == RegisterState::STATE_CONNECTED;
}

void VowifiCallManager::StopAudioStream() {
    LOG("Try to stop the audio stream, the current use audio stream count: %d",
        mUseAudioStreamCount);

    // If there is any call used the audio stream, we need reduce the count, else do nothing.
    mUseAudioStreamCount =
            mUseAudioStreamCount > 0 ?
                    mUseAudioStreamCount - 1 : mUseAudioStreamCount;

    if (mUseAudioStreamCount != 0) {
        LOG("There is call need audio stream, needn't stop audio stream, stream count: %d",
             mUseAudioStreamCount);
        return;
    }

    LOG("There isn't any call use the audio stream, need stop audio stream now.");
    {
        Json::Value root, args;
        VowifiArguments argHelp;
        root[JSON_GROUP_ID] = EVENT_GROUP_CALL;
        root[JSON_EVENT_ID] = EVENT_CALL_STOPAUDIO;
        root[JSON_EVENT_NAME] = EVENT_NAME_STOPAUDIO;

        string noticeStr = root.toStyledString();
        if (!mAdapterControl->SendToService(noticeStr)) {
            LOGWA("Native StopAudioStream failed");

        }
    }

}

void VowifiCallManager::RemoveCall() {
    if (mListener != nullptr)
        mListener->OnCallEnd();
    // After remove the session, if the session list is empty, need stop the audio stream.
    if (mAudioStart) {
        StopAudioStream();
    }
}

void VowifiCallManager::StartAudioStream() {
    LOG("Try to start the audio stream, current use audio stream count: %d",
            mUseAudioStreamCount);
    mAudioStart = true;
    // Only start the audio stream on the first call accept or start.
    mUseAudioStreamCount = mUseAudioStreamCount + 1;
    if (mUseAudioStreamCount == 1) {
        Json::Value root, args;
        VowifiArguments argHelp;
        root[JSON_GROUP_ID] = EVENT_GROUP_CALL;
        root[JSON_EVENT_ID] = EVENT_CALL_STARTAUDIO;
        root[JSON_EVENT_NAME] = EVENT_NAME_STARTAUDIO;
        string noticeStr = root.toStyledString();
        if (!mAdapterControl->SendToService(noticeStr)) {
            LOGWA("Native StartAudioStream failed");
        }
    }

}

void VowifiCallManager::SetVideoQuality(int quality) {

}

int VowifiCallManager::GetCallId(uint32_t callIndex) {
    int servCallId = 0;
    for (uint32_t i = 0; i < mCallInfo.Length(); i++) {
        if (mCallInfo[i].index == callIndex) {
            servCallId = mCallInfo[i].id;
            return servCallId;
        }
    }
    LOGWA("GetCallId failed because of no matching index");
    return 0;
}

void VowifiCallManager::SetCallId(uint32_t callIndex, uint32_t id, string &number) {
    CallInfo callInfo = { 0, 0 };

    for (uint32_t i = 0; i < mCallInfo.Length(); i++) {
        if (mCallInfo[i].id == id && mCallInfo[i].index == callIndex) {
            LOGWA("SetCallId error, id: %u already exist", id);
            return;
        }
    }

    if (0 == callIndex) {
//        for (uint32_t i = 0; i < mCallId.Length(); i++) {
//            if (mCallId[i].index != 0 && mCallId[i].id == 0) {
//                mCallId[i].id = id;
//                return;
//            }
//        }
        callInfo.index = mCallInfo.Length()+1;
        callInfo.id = id;
        callInfo.number = number;
        callInfo.isActive = true;
        mCallInfo.AppendElement(callInfo);

    } else {
        callInfo.index = callIndex;
        callInfo.id = id;
        callInfo.number = number;
        callInfo.isActive = true;
        mCallInfo.AppendElement(callInfo);
    }
}

uint32_t VowifiCallManager::GetCallIndex(uint32_t id) {
    uint32_t callIndex = 0;
    for (uint32_t i = 0; i < mCallInfo.Length(); i++) {
        if (mCallInfo[i].id == id) {
            callIndex = mCallInfo[i].index;
            return callIndex;
        }
    }
    LOGWA("Get call index failed because of no matching id");
    return 0;
}

string VowifiCallManager::GetCallNumber(uint32_t id){

    for (uint32_t i = 0; i < mCallInfo.Length(); i++) {
        if (mCallInfo[i].id == id) {
            return mCallInfo[i].number;
        }
    }
    LOGWA("Get call number failed because of no matching id");
    string errString = "get callNumber failed";
    return errString;
}

bool  VowifiCallManager::IsCallAlive(int callId){
    for (uint32_t i = 0; i < mCallInfo.Length(); i++) {
        if (mCallInfo[i].id == (uint32_t) callId) {
            if (mCallInfo[i].isActive) {
                return true;
            } else {
                return false;
            }
        }
    }
    return false;
}

void VowifiCallManager::StrToInt(uint32_t &int_temp, string &string_temp) {
    stringstream stream(string_temp);
    stream >> int_temp;
}

//int sessCall(String peerNumber, String cookie, boolean needAudio, boolean needVideo,boolean ussd, boolean isEmergency)
bool VowifiCallManager::Start(string& number, bool isEmergency) {
    LOG("Initiates an ims call with number: %s, isEmerg: %d",
         number.c_str(), isEmergency);

    string cookie = "";
    bool isNeedAudio = true;
    bool isNeedVideo = false;
    bool isUssd = false;
    {
        Json::Value root, args;
        VowifiArguments argHelp;
        root[JSON_GROUP_ID] = EVENT_GROUP_CALL;
        root[JSON_EVENT_ID] = EVENT_CALL_SESSCALL;
        root[JSON_EVENT_NAME] = EVENT_NAME_SESSCALL;
        argHelp.PushJsonArg(number);
        argHelp.PushJsonArg(cookie);
        argHelp.PushJsonArg(isNeedAudio);
        argHelp.PushJsonArg(isNeedVideo);
        argHelp.PushJsonArg(isUssd);
        argHelp.PushJsonArg(isEmergency);

        argHelp.GetJsonArgs(args);
        args.toStyledString();
        root[JSON_ARGS] = args;
        string noticeStr = root.toStyledString();
        if (!mAdapterControl->SendToService(noticeStr)) {
            LOGWA("native start call sessCall error");
            return false;
        }
    }
    mIsAlive = true;
    mMoNumber = number;
    StartAudioStream();
    return true;
}

bool VowifiCallManager::Accept() {

    bool isConference = false;
    int serCallId = mIncomingCallId;
    bool isVideo = false;
    bool isAudio = true;
    string cookie = "";

    if (isConference) {
        LOG("Accept an incoming call with call type is %d", isConference);
        // mICall.confAcceptInvite(mCallId); not implement
    } else {
        Json::Value root, args;
        VowifiArguments argHelp;
        root[JSON_GROUP_ID] = EVENT_GROUP_CALL;
        root[JSON_EVENT_ID] = EVENT_CALL_SESSANSWER;
        root[JSON_EVENT_NAME] = EVENT_NAME_SESSANSWER;
        argHelp.PushJsonArg(serCallId);
        argHelp.PushJsonArg(cookie);
        argHelp.PushJsonArg(isAudio);
        argHelp.PushJsonArg(isVideo);
        argHelp.GetJsonArgs(args);
        args.toStyledString();
        root[JSON_ARGS] = args;
        string noticeStr = root.toStyledString();
        if (!mAdapterControl->SendToService(noticeStr)) {
            LOGWA("native call accept error");
            return false;
        }
    }
    mIsAlive = true;
    for (uint32_t i = 0; i < mCallInfo.Length(); i++)
    {
        if (mCallInfo[i].id == (uint32_t)serCallId)
        {
            mCallInfo[i].isActive = true;
            break;
        }
    }
    StartAudioStream();
    return true;
}

bool VowifiCallManager::Reject() {
    LOG("Reject an incoming call");

    int serCallId = mIncomingCallId;
    int termReason = 1; //user decline
    string callee;
    {
        Json::Value root, args;
        VowifiArguments argHelp;
        root[JSON_GROUP_ID] = EVENT_GROUP_CALL;
        root[JSON_EVENT_ID] = EVENT_CALL_SESSTERM;
        root[JSON_EVENT_NAME] = EVENT_NAME_SESSTERM;
        argHelp.PushJsonArg(serCallId);
        argHelp.PushJsonArg(termReason);
        argHelp.GetJsonArgs(args);
        args.toStyledString();
        root[JSON_ARGS] = args;
        string noticeStr = root.toStyledString();
        if (!mAdapterControl->SendToService(noticeStr)) {
            LOGWA("Native reject a call failed");
            return false;
        } else {
            //Bug#930388 UE rejects MT Call
            //RemoveCall();
            callee = GetCallNumber(serCallId);
            HandleCallTermed(serCallId, 0, callee);
            return true;
        }
    }
}

void VowifiCallManager::TerminateCalls(int wifistate){
    int statecode = 501; //user terminate
    LOG("TerminateCalls, the CallInfo array length: %d", mCallInfo.Length());
    for (uint32_t i = 0; i < mCallInfo.Length(); i++){
        Terminate(mCallInfo[i].index);
        HandleCallTermed(mCallInfo[i].id, statecode, mCallInfo[i].number);
    }
    // clear pending action list
    PendingActionClear();
}

bool VowifiCallManager::Terminate(int callIndex) {
    LOG("Terminate the call, callIndex is %d", callIndex);
    int serCallId = 0;
    bool isConf = false;
    int termReason = 0; //user terminate, for sip stack
    int statecode = 501; //user terminate, for up layer
    bool terminateResult = true;
    string termNum;

    for (uint32_t i = 0; i < mCallInfo.Length(); i++) {
        if (mCallInfo[i].index == (uint32_t)callIndex) {
            serCallId = mCallInfo[i].id;
            isConf = mCallInfo[i].isConf;
            mCallInfo[i].isConf = false; // should be really terminate in Gecko by HandleCallTermed
            termNum = mCallInfo[i].number;
            break;
        }
    }

    if(!isConf){
        Json::Value root, args;
        VowifiArguments argHelp;
        root[JSON_GROUP_ID] = EVENT_GROUP_CALL;
        root[JSON_EVENT_ID] = EVENT_CALL_SESSTERM;
        root[JSON_EVENT_NAME] = EVENT_NAME_SESSTERM;
        argHelp.PushJsonArg(serCallId);
        argHelp.PushJsonArg(termReason);
        argHelp.GetJsonArgs(args);
        args.toStyledString();
        root[JSON_ARGS] = args;
        string noticeStr = root.toStyledString();
        if (!mAdapterControl->SendToService(noticeStr)) {
            LOGWA("Native terminate a call failed");
            terminateResult = false;
        } else {
            terminateResult = true;
        }
    }else{
        LOG("Kick off a call member from conference");
        Json::Value root, args;
        VowifiArguments argHelp;
        root[JSON_GROUP_ID] = EVENT_GROUP_CALL;
        root[JSON_EVENT_ID] = EVENT_CALL_CONFKICKMEMBERS;
        root[JSON_EVENT_NAME] = EVENT_NAME_CONFKICKMEMBERS;
        argHelp.PushJsonArg(mConfCallInfo.confId);
        argHelp.PushJsonArg(termNum);
        argHelp.GetJsonArgs(args);
        args.toStyledString();
        root[JSON_ARGS] = args;
        string noticeStr = root.toStyledString();
        if (!mAdapterControl->SendToService(noticeStr)) {
            LOGWA("Native kick off a call from conference failed");
            terminateResult = false;
        } else {
            terminateResult = true;
        }
    }

    HandleCallTermed(serCallId, statecode, termNum);

//    for (uint32_t i = 0; i < mCallInfo.Length(); i++){
//            if(mCallInfo[i].index == (uint32_t)callIndex){
//                HandleCallTermed(mCallInfo[i].id, statecode, mCallInfo[i].number);
//                break;
//            }
//            LOG("Terminate call error, can not find callIndex %d", callIndex);
//    }
    return terminateResult;
}

bool VowifiCallManager::Switch() {
    LOG("Switch the existing calls");
    //***********************************
    //  make sure operate as order:
    //    Hold -> Hold done -> UnHold
    //  to avoid voiceAgent wrong
    //***********************************
    bool isSuccess = false;
    int callNotInConf = -1;

    //*******************************
    // 1. for normal call case
    //    -- confId has zero value
    //*******************************
    if (mConfCallInfo.confId == 0) {
        // 1.1 For Single call Hold/Unhold
        if (mCallInfo.Length() == 1) {
            if (mCallInfo[0].isActive)
                isSuccess = Hold(mCallInfo[0].index);
            else
                isSuccess = Resume(mCallInfo[0].index);

        // 1.2 For Two call Swap case
        // if not conference, the callInfo list can keep maxmum two calls
        } else {
            if (mCallInfo[0].isActive && mCallInfo[0].callState == CALL_STATE_CONNECTED) {
                isSuccess = Hold(mCallInfo[0].index);
                //Bug#875198 Hold current call and Accept incoming call
                if (isSuccess && mCallInfo[1].callState == CALL_STATE_INCOMING)
                    //Accept();
                    isSuccess = PendingActionPush(ActionType_T::ENUM_ACTION_SWITCH,
                                                  false,
                                                  mCallInfo[0].id,
                                                  ActionType_T::ENUM_ACTION_HOLD,
                                                  false,
                                                  ActionType_T::ENUM_ACTION_ACCEPT);
                else if (isSuccess && mCallInfo[1].callState == CALL_STATE_HELD)
                    //isSuccess = Resume(mCallInfo[1].index);
                    isSuccess = PendingActionPush(ActionType_T::ENUM_ACTION_SWITCH,
                                                  false,
                                                  mCallInfo[0].id,
                                                  ActionType_T::ENUM_ACTION_HOLD,
                                                  false,
                                                  mCallInfo[1].index,
                                                  ActionType_T::ENUM_ACTION_UNHOLD);
            } else {
                if (mCallInfo[1].isActive) {
                    isSuccess = Hold(mCallInfo[1].index);
                    if (isSuccess)
                        //isSuccess = Resume(mCallInfo[0].index);
                        isSuccess = PendingActionPush(ActionType_T::ENUM_ACTION_SWITCH,
                                                      false,
                                                      mCallInfo[1].id,
                                                      ActionType_T::ENUM_ACTION_HOLD,
                                                      false,
                                                      mCallInfo[0].index,
                                                      ActionType_T::ENUM_ACTION_UNHOLD);
                }
            }
        }
    //*******************************
    // 2. for conference call case
    //    -- confId has non-zero value
    //*******************************
    } else {
        // 2.1 For only one call connection
        if (mCallInfo.Length() == 1) {
            if (mCallInfo[0].isActive)
                isSuccess = ConfHold();
            else
                isSuccess = ConfResume();

        // 2.2 For Two call connections
        //     one conference, one normal call
        } else {
            for (uint32_t i = 0; i < mCallInfo.Length(); i++) {
                if (!mCallInfo[i].isConf)
                {
                    callNotInConf = i;  // find the normal call
                    break;
                }
            }

            if (!mCallInfo[callNotInConf].isActive)
            {
                isSuccess = ConfHold();
                isSuccess = isSuccess && PendingActionPush(
                                              ActionType_T::ENUM_ACTION_SWITCH,
                                              true,
                                              mConfCallInfo.confId,
                                              ActionType_T::ENUM_ACTION_HOLD,
                                              false,
                                              mCallInfo[callNotInConf].index,
                                              ActionType_T::ENUM_ACTION_UNHOLD);
            }else{
                isSuccess = Hold(mCallInfo[callNotInConf].index);
                //isSuccess = ConfResume();
                isSuccess = isSuccess && PendingActionPush(
                                              ActionType_T::ENUM_ACTION_SWITCH,
                                              false,
                                              mCallInfo[callNotInConf].id,
                                              ActionType_T::ENUM_ACTION_HOLD,
                                              true,
                                              ActionType_T::ENUM_ACTION_UNHOLD);
            }

            //for (uint32_t i = 0; i < mCallInfo.Length(); i++) {
            //    if (mCallInfo[i].isActive && !mCallInfo[i].isConf) {
            //        isSuccess = Hold(mCallInfo[i].index);
            //        //isSuccess = ConfResume();
            //        isSuccess = PendingActionPush(ActionType_T::ENUM_ACTION_SWITCH,
            //                                      false,
            //                                      mCallInfo[i].id,
            //                                      ActionType_T::ENUM_ACTION_HOLD,
            //                                      true,
            //                                      ActionType_T::ENUM_ACTION_UNHOLD);
            //        break;
            //    } else if (!mCallInfo[i].isActive && !mCallInfo[i].isConf) {
            //        isSuccess = ConfHold();
            //        //isSuccess = Resume(mCallInfo[i].index);
            //        isSuccess = PendingActionPush(ActionType_T::ENUM_ACTION_SWITCH,
            //                                      true,
            //                                      mConfCallInfo.confId,
            //                                      ActionType_T::ENUM_ACTION_HOLD,
            //                                      false,
            //                                      mCallInfo[i].index,
            //                                      ActionType_T::ENUM_ACTION_UNHOLD);
            //        break;
            //    } else if (mCallInfo[i].isActive && mCallInfo[i].isConf) {
            //        isSuccess = ConfHold();
            //        //Bug#930592 Swap between a Conference call and a call not in Conference
            //        if (callNotInConf > -1) {
            //            LOG("Going to Resume the call not in conference.");
            //            //isSuccess = Resume(mCallInfo[callNotInConf].index);
            //            isSuccess = PendingActionPush(ActionType_T::ENUM_ACTION_SWITCH,
            //                                          true,
            //                                          mConfCallInfo.confId,
            //                                          ActionType_T::ENUM_ACTION_HOLD,
            //                                          false,
            //                                          mCallInfo[callNotInConf].index,
            //                                          ActionType_T::ENUM_ACTION_UNHOLD);
            //        }
            //        break;
            //    } else if (!mCallInfo[i].isActive && mCallInfo[i].isConf) {
            //        if (callNotInConf > -1) {
            //            LOG("Going to Hold the call not in conference.");
            //            isSuccess = Hold(mCallInfo[callNotInConf].index);
            //        }
            //        //isSuccess = ConfResume();
            //        isSuccess = PendingActionPush(ActionType_T::ENUM_ACTION_SWITCH,
            //                                      false,
            //                                      mCallInfo[callNotInConf].id,
            //                                      ActionType_T::ENUM_ACTION_HOLD,
            //                                      true,
            //                                      ActionType_T::ENUM_ACTION_UNHOLD);
            //        break;
            //    }
            //}
        }
    }
    return isSuccess;
}

bool VowifiCallManager::ConfHold(){

     mConfCallInfo.isActive = false;
     Json::Value root, args;
     VowifiArguments argHelp;
     root[JSON_GROUP_ID] = EVENT_GROUP_CALL;
     root[JSON_EVENT_ID] = EVENT_CALL_CONFHOLD;
     root[JSON_EVENT_NAME] = EVENT_NAME_CONFHOLD;
     argHelp.PushJsonArg(mConfCallInfo.confId);
     argHelp.GetJsonArgs(args);
     args.toStyledString();
     root[JSON_ARGS] = args;
     string noticeStr = root.toStyledString();
     if (!mAdapterControl->SendToService(noticeStr)) {
         LOGWA("Native hold conference call failed");
         return false;
     }
     else{
         return true;
     }
}

bool VowifiCallManager::ConfResume(){
    mConfCallInfo.isActive = true;
    Json::Value root, args;
    VowifiArguments argHelp;
    root[JSON_GROUP_ID] = EVENT_GROUP_CALL;
    root[JSON_EVENT_ID] = EVENT_CALL_CONFRESUME;
    root[JSON_EVENT_NAME] = EVENT_NAME_CONFRESUME;
    argHelp.PushJsonArg(mConfCallInfo.confId);
    argHelp.GetJsonArgs(args);
    args.toStyledString();
    root[JSON_ARGS] = args;
    string noticeStr = root.toStyledString();
    if (!mAdapterControl->SendToService(noticeStr)) {
        LOGWA("Native resume conference call failed");
        return false;
    }
    else{
        return true;
    }
}

bool VowifiCallManager::TerminateConfCall(){
    int termReason = 0;
    bool terminateResult = false;
    Json::Value root, args;
    VowifiArguments argHelp;
    root[JSON_GROUP_ID] = EVENT_GROUP_CALL;
    root[JSON_EVENT_ID] = EVENT_CALL_CONFTERM;
    root[JSON_EVENT_NAME] = EVENT_NAME_CONFTERM;
    argHelp.PushJsonArg(mConfCallInfo.confId);
    argHelp.PushJsonArg(termReason);
    argHelp.GetJsonArgs(args);
    args.toStyledString();
    root[JSON_ARGS] = args;
    string noticeStr = root.toStyledString();
    if (!mAdapterControl->SendToService(noticeStr)) {
        LOGWA("Native terminate conference call failed");
        terminateResult = false;
    } else {
        terminateResult = true;
    }

    HandleConfTermed();
    return terminateResult;
}

bool VowifiCallManager::HangUpForeground() {
    bool isSuccess = false;
    uint32_t length =  mCallInfo.Length();
    if (mConfCallInfo.confId != 0) {
        if (mConfCallInfo.isActive) {
             LOG("Terminate the conference call");
             isSuccess = TerminateConfCall();
        } else {
            LOG("Conference call is been held, Terminate the active call");
            for (uint32_t i = 0; i < mCallInfo.Length(); i++) {
                if (mCallInfo[i].isActive && !mCallInfo[i].isConf) {
                    isSuccess = Terminate(mCallInfo[i].index);
                    break;
                }
            }
        }
    } else {
        LOG("No conference, HangUpForeground call");
        for (uint32_t i = 0; i < mCallInfo.Length(); i++) {
            if (mCallInfo[i].isActive) {
                isSuccess = Terminate(mCallInfo[i].index);
                //Bug#931515 After Terminate active call, array length is decreased, so decrease the loop variable to process other call.
                if (mCallInfo.Length() < length)
                    i--;
                //break;
            }else{
                isSuccess = Resume(mCallInfo[i].index);
            }
        }
    }
    return isSuccess;
}

bool VowifiCallManager::HangUpBackground() {
    bool isSuccess = false;
    LOG("VowifiCallManager: HangUpBackground call");
    for (uint32_t i = 0; i < mCallInfo.Length(); i++) {
        if (CALL_STATE_INCOMING == mCallInfo[i].callState) {
            isSuccess = Terminate(mCallInfo[i].index);
            break;
        }
    }
    return isSuccess;
}

bool VowifiCallManager::Mute(bool isMute){
    int serCallId;
    for (uint32_t i = 0; i < mCallInfo.Length(); i++) {
            if (mCallInfo[i].isActive) {
                serCallId = mCallInfo[i].id;
                break;
            }
    }
    Json::Value root, args;
    VowifiArguments argHelp;
    root[JSON_GROUP_ID] = EVENT_GROUP_CALL;
    root[JSON_EVENT_ID] = EVENT_CALL_SETMICMUTE;
    root[JSON_EVENT_NAME] = EVENT_NAME_SETMICMUTE;
    argHelp.PushJsonArg(serCallId);
    argHelp.PushJsonArg(isMute);
    argHelp.GetJsonArgs(args);
    args.toStyledString();
    root[JSON_ARGS] = args;
    string noticeStr = root.toStyledString();
    if (!mAdapterControl->SendToService(noticeStr)) {
        LOGWA("Native mute a call failed");
        return false;
    }
    else{
        return true;
    }


}

bool VowifiCallManager::Hold(int callIndex) {
    bool isConference = false;
    int serCallId = GetCallId(callIndex);
    if (isConference) {
        //res = mICall.confHold(serCallId);
        return true;
    } else {
        Json::Value root, args;
        VowifiArguments argHelp;
        root[JSON_GROUP_ID] = EVENT_GROUP_CALL;
        root[JSON_EVENT_ID] = EVENT_CALL_SESSHOLD;
        root[JSON_EVENT_NAME] = EVENT_NAME_SESSHOLD;
        argHelp.PushJsonArg(serCallId);
        argHelp.GetJsonArgs(args);
        args.toStyledString();
        root[JSON_ARGS] = args;
        string noticeStr = root.toStyledString();
        if (!mAdapterControl->SendToService(noticeStr)) {
            LOGWA("Native hold a call failed");
            return false;
        }
        else{
            return true;
        }
    }
}

bool VowifiCallManager::Resume(int callIndex) {
    bool isConference =  false;
    int serCallId = GetCallId(callIndex);
    if (isConference) {
        //res = mICall.confHold(serCallId);
        return true;
    } else {
        Json::Value root, args;
        VowifiArguments argHelp;
        root[JSON_GROUP_ID] = EVENT_GROUP_CALL;
        root[JSON_EVENT_ID] = EVENT_CALL_SESSRESUME;
        root[JSON_EVENT_NAME] = EVENT_NAME_SESSRESUME;
        argHelp.PushJsonArg(serCallId);
        argHelp.GetJsonArgs(args);
        args.toStyledString();
        root[JSON_ARGS] = args;
        string noticeStr = root.toStyledString();
        if (!mAdapterControl->SendToService(noticeStr)) {
            LOGWA("Native resume a call failed");
            return false;
        }
        else{
            return true;
        }
    }
}

bool VowifiCallManager::ConferenceCall(bool isAllHold) {
    bool isSuccess = false;
    int callSessNum = mCallInfo.Length();
    if(callSessNum < 2 || callSessNum > 5){
        LOGWA("ConferenceCall failed because the callSessNum %d is not valid", callSessNum);
        return false;
    }

    if (!isAllHold) {
        for (uint32_t i = 0; i < mCallInfo.Length(); i++) {
            if (mCallInfo[i].isActive && !mCallInfo[i].isConf) {
                LOG("ConferenceCall going to hold the active call");
                mCallInfo[i].isConfHold = true;
                isSuccess = Hold(mCallInfo[i].index);
                break;
            }
        }
    } else {
        LOG("ConferenceCall going to merge");
        isSuccess = Merge(callSessNum);
    }
    return isSuccess;

}

bool VowifiCallManager::Merge(int callSessNum) {
    if(callSessNum == 2){
        return CreateConfcall();
    }
    else{
         InviteConfCallMember();
         return true;
    }
}

bool VowifiCallManager::CreateConfcall(){
    LOG("CreateConfcall to create a conference call");
    bool isNeedVideo = false;
    {
        Json::Value root, args;
        VowifiArguments argHelp;
        root[JSON_GROUP_ID] = EVENT_GROUP_CALL;
        root[JSON_EVENT_ID] = EVENT_CALL_CONFINIT;
        root[JSON_EVENT_NAME] = EVENT_NAME_CONFINIT;
        argHelp.PushJsonArg(isNeedVideo);
        argHelp.GetJsonArgs(args);
        args.toStyledString();
        root[JSON_ARGS] = args;
        string noticeStr = root.toStyledString();
        if (!mAdapterControl->SendToService(noticeStr)) {
            LOGWA("native create confCall error");
            return false;
        }
    }
    StartAudioStream();
    return true;
}

void VowifiCallManager::SetupConfcall(int confId){
    LOG("SetupConfcall to setup a conference call");
    string cookie = "";
    {
        Json::Value root, args;
        VowifiArguments argHelp;
        root[JSON_GROUP_ID] = EVENT_GROUP_CALL;
        root[JSON_EVENT_ID] = EVENT_CALL_CONFSETUP;
        root[JSON_EVENT_NAME] = EVENT_NAME_CONFSETUP;
        argHelp.PushJsonArg(confId);
        argHelp.PushJsonArg(cookie);
        argHelp.GetJsonArgs(args);
        args.toStyledString();
        root[JSON_ARGS] = args;
        string noticeStr = root.toStyledString();
        if (!mAdapterControl->SendToService(noticeStr)) {
            LOGWA("native setup confCall error");
        }
    }
}

void VowifiCallManager::InviteConfCallMember() {
    for (uint32_t i = 0; i < mCallInfo.Length(); i++) {
        if (!mCallInfo[i].isConf) {
            LOG("Invite a call member to conference");
            Json::Value root, args;
            VowifiArguments argHelp;
            string phoneNum;
            root[JSON_GROUP_ID] = EVENT_GROUP_CALL;
            root[JSON_EVENT_ID] = EVENT_CALL_CONFADDMEMBERS;
            root[JSON_EVENT_NAME] = EVENT_NAME_CONFADDMEMBERS;
            argHelp.PushJsonArg(mConfCallInfo.confId);
            argHelp.PushJsonArg(phoneNum);
            argHelp.PushJsonArg(mCallInfo[i].id);
            argHelp.GetJsonArgs(args);
            args.toStyledString();
            root[JSON_ARGS] = args;
            string noticeStr = root.toStyledString();
            if (!mAdapterControl->SendToService(noticeStr)) {
                LOGWA("native conference Invite call error");
            }
            return;
        }
    }
    //all the call has been invited to the conf, update all the call state;
    if(!mConfCallInfo.isActive && mConfCallInfo.confId != 0)
        ConfResume();
}

void VowifiCallManager::UpdateDataRouterState(int dataRouterState){
    LOG("Update the call state to data router. state: %d", dataRouterState);
    Json::Value root, args;
    VowifiArguments argHelp;
    root[JSON_GROUP_ID] = EVENT_GROUP_CALL;
    root[JSON_EVENT_ID] = EVENT_CALL_UPDCALLSTATEROUTER;
    root[JSON_EVENT_NAME] = EVENT_NAME_UPDCALLSTATEROUTER;
    argHelp.PushJsonArg(dataRouterState);
    argHelp.GetJsonArgs(args);
    args.toStyledString();
    root[JSON_ARGS] = args;
    string noticeStr = root.toStyledString();
    if (!mAdapterControl->SendToService(noticeStr)) {
        LOGWA("native UpdateDataRouterState error");
    }

}

void VowifiCallManager::HandleEvent(int eventId,
                                    string &eventName,
                                    Json::Value &args)
{
    LOG("Vowifi HandleEvent of eventId: %u, name: %s",
         eventId, eventName.c_str());

    bool bArgs = true;
    bool bAg = false;
    int callId = 0;

    switch (eventId) {
    case EVENT_CALL_SESSCALL:
        bAg = VowifiArguments::GetOneArg(args, KEY_ID, callId);
        if (!bAg) {
            bArgs = false;
        }
        LOG("Handle event %s for the call %d", eventName.c_str(), callId);
        SetCallId(0, callId, mMoNumber);
        mOutgoingCallId = callId;
        break;
    case CALLBACK_OUTGOING: {
        bool isVideo = false;
        string callee;
        bAg = VowifiArguments::GetOneArg(args, KEY_ID, callId);
        bAg &= VowifiArguments::GetOneArg(args, KEY_IS_VIDEO, isVideo);
        bAg &= VowifiArguments::GetOneArg(args, KEY_PHONE_NUM, callee);
        if (!bAg) {
            bArgs = false;
        }
        HandleCallOutgoing(callId, false /* not conference */, isVideo, callee);
    }
        break;
    case CALLBACK_INCOMING: {
        bool isVideo = false;
        string callee;
        bAg = VowifiArguments::GetOneArg(args, KEY_ID, callId);
        bAg &= VowifiArguments::GetOneArg(args, KEY_IS_VIDEO, isVideo);
        bAg &= VowifiArguments::GetOneArg(args, KEY_PHONE_NUM, callee);
        if (!bAg) {
            bArgs = false;
        }
        SetCallId(0, callId, callee);
        mIncomingCallId = callId;
        HandleCallIncoming(callId, false /* not conference */, isVideo, callee);
    }
        break;
    case CALLBACK_ALERTED:{
        bool isVideo = false;
        string callee;
        bAg = VowifiArguments::GetOneArg(args, KEY_ID, callId);
        bAg &= VowifiArguments::GetOneArg(args, KEY_IS_VIDEO, isVideo);
        bAg &= VowifiArguments::GetOneArg(args, KEY_PHONE_NUM, callee);
        HandleCallProcessing(callId, callee, isVideo, CALL_STATE_ALERTING);
    }
        break;
    case CALLBACK_TALKING:{
        bool isVideo = false;
        string callee;
        bAg = VowifiArguments::GetOneArg(args, KEY_ID, callId);
        bAg &= VowifiArguments::GetOneArg(args, KEY_IS_VIDEO, isVideo);
        callee = GetCallNumber(callId);
        HandleCallProcessing(callId, callee, isVideo, CALL_STATE_CONNECTED);

    }
        break;
    case CALLBACK_TERMED:{
        int stateCode;
        string callee;
        bAg = VowifiArguments::GetOneArg(args, KEY_ID, callId);
        bAg &= VowifiArguments::GetOneArg(args, KEY_STATE_CODE, stateCode);
        callee = GetCallNumber(callId);
        HandleCallTermed(callId, stateCode, callee);
    }
        break;

    case CALLBACK_HOLD_OK:
    case CALLBACK_HOLD_FAILED:
    case CALLBACK_UNHOLD_OK:
    case CALLBACK_UNHOLD_FAILED:
    case CALLBACK_HELD:
    case CALLBACK_UNHELD:
    case CALLBACK_CONFCB_HOLD_OK:
    case CALLBACK_CONFCB_HOLD_FAILED:
    case CALLBACK_CONFCB_UNHOLD_OK:
    case CALLBACK_CONFCB_UNHOLD_FAILED:{
        string callee;
        bAg = VowifiArguments::GetOneArg(args, KEY_ID, callId);
        callee = GetCallNumber(callId);
        HandleCallHoldOrResume(callId, eventId, callee);
        // handle pending action
        PendingActionProcess(callId, eventId);
    }
        break;

    case CALLBACK_RTP_CONNECTIVITY:{ //EVENT_CODE_CALL_RTP_RECEIVED
        bool isVideo = false;
        bool isReceived = false;
        bAg = VowifiArguments::GetOneArg(args, KEY_ID, callId);
        bAg &= VowifiArguments::GetOneArg(args, KEY_IS_VIDEO, isVideo);
        bAg &= VowifiArguments::GetOneArg(args, KEY_RTP_RECEIVED, isReceived);
        HandleRTPReceived(callId, isVideo, isReceived);
    }

        break;

    case CALLBACK_RTCP_RR:{ //EVENT_CODE_CALL_RTCP_CHANGED
        bAg = VowifiArguments::GetOneArg(args, KEY_ID, callId);
        if(IsCallAlive(callId)){
            bool isVideo = false;
            int lose = 0;
            int jitter = 0;
            int rtt = 0;
            bAg &= VowifiArguments::GetOneArg(args, KEY_RTCP_LOSE, lose);
            bAg &= VowifiArguments::GetOneArg(args, KEY_RTCP_JITTER, jitter);
            bAg &= VowifiArguments::GetOneArg(args, KEY_RTCP_RTT, rtt);
            bAg &= VowifiArguments::GetOneArg(args, KEY_IS_VIDEO, isVideo);
            HandleRTCPChanged(lose, jitter, rtt, isVideo);
        }
    }

        break;
    case EVENT_CALL_FAILED:{
        LOG("Handle the event call failed, skip it");
    }
        break;

    case EVENT_CALL_CONFINIT:
        bAg = VowifiArguments::GetOneArg(args, KEY_ID, callId);
        if (!bAg) {
            bArgs = false;
        }
        LOG("Handle the event <%s> for the confCall %d", eventName.c_str(), callId);
        mConfCallInfo.confId = callId;
        SetupConfcall(callId);
        break;
    case CALLBACK_CONFCB_CONNED:
        HandleConfConnected();
        break;
    case CALLBACK_CONFCB_DISCED:
        HandleMergeFailed();
        break;
    case CALLBACK_CONFCB_IVT_ACPT:
    case CALLBACK_CONFCB_IVT_FAIL:
    case CALLBACK_CONFCB_KICK_ACPT:
    case CALLBACK_CONFCB_KICK_FAIL:{
        string phoneNumber;
        string phoneUri;
        string newStatus = "";
        bAg = VowifiArguments::GetOneArg(args, KEY_PHONE_NUM, phoneNumber);
        bAg &= VowifiArguments::GetOneArg(args, KEY_SIP_URI, phoneUri);
        HandleConfParticipantsChanged(eventId, phoneNumber, phoneUri, newStatus);
    }
        break;
    case CALLBACK_CONFCB_PTPT_UPDT:{
        string phoneNumber;
        string phoneUri;
        string newStatus = "";
        bAg = VowifiArguments::GetOneArg(args, KEY_PHONE_NUM, phoneNumber);
        bAg &= VowifiArguments::GetOneArg(args, KEY_SIP_URI, phoneUri);
        bAg &= VowifiArguments::GetOneArg(args, KEY_CONF_PART_NEW_STATUS, newStatus);
        HandleConfParticipantsChanged(eventId, phoneNumber, phoneUri, newStatus);
        break;
    }
    default:
        LOGWA("Vowifi CallManager handle event Id: %d is not supported, skip it!",
                eventId);
        break;
    }

    if (!bArgs) {
        LOGWA("Vowifi CallManager handle event arguments list is incorrected, skip it!");
    }
}

void VowifiCallManager::HandleConfParticipantsChanged(int eventId, string &phoneNum, string &sipUri, string &newStatus){
    LOG("Handle the conference participant update result.");
    bool isNeedInviteNext = false;

    switch(eventId){
    case CALLBACK_CONFCB_IVT_ACPT:
        LOG("conference call Invite success, phone: %s", phoneNum.c_str());
        for (uint32_t i = 0; i < mCallInfo.Length(); i++) {
            if ((((mCallInfo[i].number).find(phoneNum) != std::string::npos)
                    || (phoneNum.find(mCallInfo[i].number) != std::string::npos))
                    && !mCallInfo[i].isConf) {
                mCallInfo[i].isConf = true;
                //Bug#942749 update the call state
                mCallInfo[i].callState = CALL_STATE_CONNECTED;
                isNeedInviteNext = true;
                break;
            }
        }
        break;
    case CALLBACK_CONFCB_IVT_FAIL:
        LOG("conference call Invite failed, phone: %s",phoneNum.c_str());
        for (uint32_t i = 0; i < mCallInfo.Length(); i++) {
            if ((((mCallInfo[i].number).find(phoneNum) != std::string::npos)
                    || (phoneNum.find(mCallInfo[i].number) != std::string::npos))
                    && !mCallInfo[i].isConf) {
                mCallInfo[i].isConf = false;
                Terminate(mCallInfo[i].index);
                isNeedInviteNext = true;
                break;
            }
        }
        break;
    case CALLBACK_CONFCB_KICK_ACPT:
        LOG("conference kick off a call success");
        break;
    case CALLBACK_CONFCB_KICK_FAIL:
        LOG("conference kick off a call failed");
        break;
    case CALLBACK_CONFCB_PTPT_UPDT:
        LOG("conference call status updated");
        if(newStatus == CONF_STATUS_DISCONNECTED){
            for (uint32_t i = 0; i < mCallInfo.Length(); i++) {
                if (((mCallInfo[i].number).find(phoneNum) != std::string::npos)
                        || (phoneNum.find(mCallInfo[i].number)
                                != std::string::npos)) {
                    mCallInfo[i].isConf = false;
                    Terminate(mCallInfo[i].index);
                    break;
                }
            }
        }
        break;
    default:
        break;
    }

    if(isNeedInviteNext){
        LOG("Invite next call to conference");
        InviteConfCallMember();
    }
}

void VowifiCallManager::HandleConfConnected(){
    LOG("Handle the conference call connected.");
    nsCOMPtr<nsIGonkTelephonyService> telephonyService =
             do_GetService(TELEPHONY_SERVICE_CONTRACTID);

    if (!telephonyService) {
      LOGEA("Could not acquire nsITelephonyService!");
    }
    
    // AutoJSContext cx;
    mozilla::AutoSafeJSContext cx;
    JSAutoCompartment ac(cx, xpc::PrivilegedJunkScope());
    
    JS::Rooted<JSObject*> callsObj(cx);
    callsObj = JS_NewPlainObject(cx);
    
    for(uint32_t i = 0; i < mCallInfo.Length(); i++){
    
        JS::Rooted<JSObject*> callObj(cx);
        callObj = JS_NewPlainObject(cx);
        
        JS::Rooted<JS::Value> state(cx);
        state.setInt32(CALL_STATE_CONNECTED);
        mCallInfo[i].isActive = true;
        LOG("update all the call's state to active");
        
        JS_SetProperty(cx, callObj, "state", state);
        
        JS::Rooted<JS::Value> callIndex(cx);
        callIndex.setInt32(mCallInfo[i].index);
        JS_SetProperty(cx, callObj, "callIndex", callIndex);
        
        JS::Rooted<JS::Value> toa(cx);
        toa.setInt32(TOA_UNKNOWN);
        JS_SetProperty(cx, callObj, "toa", toa);
        
        JS::Rooted<JS::Value> isMpty(cx);
        isMpty.setBoolean(true);
        JS_SetProperty(cx, callObj, "isMpty", isMpty);
        
        JS::Rooted<JS::Value> isMT(cx);
        isMT.setBoolean(mCallInfo[i].isMT);
        JS_SetProperty(cx, callObj, "isMT", isMT);
        
        JS::Rooted<JS::Value> isVoice(cx);
        isVoice.setBoolean(true);
        JS_SetProperty(cx, callObj, "isVoice", isVoice);
        
        JS::Rooted<JS::Value> callNumber(cx);
        JS::Rooted<JSString*> numberString(cx,
                                       JS_NewStringCopyZ(cx, (mCallInfo[i].number).c_str()));
        
        callNumber.setString(numberString);
        JS_SetProperty(cx, callObj, "number", callNumber);
        
        JS::Rooted<JS::Value> numberPresentation(cx);
        numberPresentation.setInt32(CALL_PRESENTATION_ALLOWED);
        JS_SetProperty(cx, callObj, "numberPresentation", numberPresentation);
        
        JS::Rooted<JS::Value> name(cx);
        name.setString(numberString);
        JS_SetProperty(cx, callObj, "name", name);
        
        JS::Rooted<JS::Value> namePresentation(cx);
        namePresentation.setInt32(CALL_PRESENTATION_ALLOWED);
        JS_SetProperty(cx, callObj, "namePresentation", namePresentation);
        
        JS::Rooted<JS::Value> radioTech(cx);
        radioTech.setInt32(RADIO_TECH_WIFI);
        JS_SetProperty(cx, callObj, "radioTech", radioTech);
        
        JS::Rooted<JS::Value> voiceQuality(cx);
        voiceQuality.setInt32(CALL_VOICE_QUALITY_HD);
        JS_SetProperty(cx, callObj, "voiceQuality", voiceQuality);
        
        JS::Rooted<JS::Value> value(cx, JS::ObjectValue(*callObj));
        JS_SetProperty(cx, callsObj, JS_EncodeString(cx, ToString(cx, callIndex)), value);
    
    }
    
    
    JS::Rooted<JS::Value> callMessage(cx, JS::ObjectValue(*callsObj));
    
    if (telephonyService){
       uint32_t serviceID = 0;  //single card
       telephonyService->NotifyCurrentCalls(serviceID, callMessage);
    }
    InviteConfCallMember();
}

void VowifiCallManager::HandleCallOutgoing(int sessionId, bool isConf,
        bool isVideo, string &callee){
    LOG("Handle the outgoing call.");

    for(uint32_t i = 0; i < mCallInfo.Length(); i++){
        if(mCallInfo[i].id == (uint32_t)sessionId){
            mCallInfo[i].isConf = isConf;
            mCallInfo[i].isVedio = isVideo;
            mCallInfo[i].number = callee;
            mCallInfo[i].callState = CALL_STATE_DIALING;
            mCallInfo[i].isMT = true;
            break;
        }
    }

    nsCOMPtr<nsIGonkTelephonyService> telephonyService =
         do_GetService(TELEPHONY_SERVICE_CONTRACTID);

    if (!telephonyService) {
      LOGEA("Could not acquire nsITelephonyService!");
    }
    
    // AutoJSContext cx;
    mozilla::AutoSafeJSContext cx;
    JSAutoCompartment ac(cx, xpc::PrivilegedJunkScope());
    
    JS::Rooted<JSObject*> callsObj(cx);
    callsObj = JS_NewPlainObject(cx);
    
    for(uint32_t i = 0; i < mCallInfo.Length(); i++){
    
        JS::Rooted<JSObject*> callObj(cx);
        callObj = JS_NewPlainObject(cx);
        
        JS::Rooted<JS::Value> state(cx);
        state.setInt32(mCallInfo[i].callState);
        JS_SetProperty(cx, callObj, "state", state);
        
        JS::Rooted<JS::Value> callIndex(cx);
        callIndex.setInt32(mCallInfo[i].index);
        JS_SetProperty(cx, callObj, "callIndex", callIndex);
        
        JS::Rooted<JS::Value> toa(cx);
        toa.setInt32(TOA_UNKNOWN);
        JS_SetProperty(cx, callObj, "toa", toa);
        
        JS::Rooted<JS::Value> isMpty(cx);
        isMpty.setBoolean(false);
        JS_SetProperty(cx, callObj, "isMpty", isMpty);
        
        JS::Rooted<JS::Value> isMT(cx);
        isMT.setBoolean(true);
        JS_SetProperty(cx, callObj, "isMT", isMT);
        
        JS::Rooted<JS::Value> isVoice(cx);
        isVoice.setBoolean(true);
        JS_SetProperty(cx, callObj, "isVoice", isVoice);
        
        JS::Rooted<JS::Value> callNumber(cx);
        JS::Rooted<JSString*> numberString(cx,
                                       JS_NewStringCopyZ(cx, (mCallInfo[i].number).c_str()));
        
        callNumber.setString(numberString);
        JS_SetProperty(cx, callObj, "number", callNumber);
        
        JS::Rooted<JS::Value> numberPresentation(cx);
        numberPresentation.setInt32(CALL_PRESENTATION_ALLOWED);
        JS_SetProperty(cx, callObj, "numberPresentation", numberPresentation);
        
        JS::Rooted<JS::Value> name(cx);
        name.setString(numberString);
        JS_SetProperty(cx, callObj, "name", name);
        
        JS::Rooted<JS::Value> namePresentation(cx);
        namePresentation.setInt32(CALL_PRESENTATION_ALLOWED);
        JS_SetProperty(cx, callObj, "namePresentation", namePresentation);
        
        JS::Rooted<JS::Value> radioTech(cx);
        radioTech.setInt32(RADIO_TECH_WIFI);
        JS_SetProperty(cx, callObj, "radioTech", radioTech);
        
        JS::Rooted<JS::Value> voiceQuality(cx);
        voiceQuality.setInt32(CALL_VOICE_QUALITY_HD);
        JS_SetProperty(cx, callObj, "voiceQuality", voiceQuality);
        
        JS::Rooted<JS::Value> value(cx, JS::ObjectValue(*callObj));
        JS_SetProperty(cx, callsObj, JS_EncodeString(cx, ToString(cx, callIndex)), value);
    
    }
    
    
    JS::Rooted<JS::Value> callMessage(cx, JS::ObjectValue(*callsObj));
    
    if (telephonyService){
        uint32_t serviceID = 0;  //single card
        telephonyService->NotifyCurrentCalls(serviceID, callMessage);
    }
}

void VowifiCallManager::HandleCallIncoming(int sessionId, bool isConf,
        bool isVideo, string &callee) {
    LOG("Handle the incoming call, sessId: %d, isConf: %d",sessionId, isConf);

    for(uint32_t i = 0; i < mCallInfo.Length(); i++){
        if(mCallInfo[i].id == (uint32_t)sessionId){
            mCallInfo[i].isConf = isConf;
            mCallInfo[i].isVedio = isVideo;
            mCallInfo[i].number = callee;
            mCallInfo[i].callState = CALL_STATE_INCOMING;
            mCallInfo[i].isMT = false;
            break;
        }
    }

    nsCOMPtr<nsIGonkTelephonyService> telephonyService =
      do_GetService(TELEPHONY_SERVICE_CONTRACTID);

    if (!telephonyService) {
      LOGEA("Could not acquire nsITelephonyService!");
    }

    // AutoJSContext cx;
    mozilla::AutoSafeJSContext cx;
    JSAutoCompartment ac(cx, xpc::PrivilegedJunkScope());

    JS::Rooted<JSObject*> callsObj(cx);
    callsObj = JS_NewPlainObject(cx);

    for(uint32_t i = 0; i < mCallInfo.Length(); i++){

        JS::Rooted<JSObject*> callObj(cx);
        callObj = JS_NewPlainObject(cx);
        
        JS::Rooted<JS::Value> state(cx);
        state.setInt32(mCallInfo[i].callState);
        JS_SetProperty(cx, callObj, "state", state);
        
        JS::Rooted<JS::Value> callIndex(cx);
        callIndex.setInt32(mCallInfo[i].index);
        JS_SetProperty(cx, callObj, "callIndex", callIndex);
        
        JS::Rooted<JS::Value> toa(cx);
        toa.setInt32(TOA_UNKNOWN);
        JS_SetProperty(cx, callObj, "toa", toa);
        
        JS::Rooted<JS::Value> isMpty(cx);
        isMpty.setBoolean(false);
        JS_SetProperty(cx, callObj, "isMpty", isMpty);
        
        JS::Rooted<JS::Value> isMT(cx);
        isMT.setBoolean(true);
        JS_SetProperty(cx, callObj, "isMT", isMT);
        
        JS::Rooted<JS::Value> isVoice(cx);
        isVoice.setBoolean(true);
        JS_SetProperty(cx, callObj, "isVoice", isVoice);
        
        JS::Rooted<JS::Value> callNumber(cx);
        JS::Rooted<JSString*> numberString(cx,
                                       JS_NewStringCopyZ(cx, (mCallInfo[i].number).c_str()));
        
        callNumber.setString(numberString);
        JS_SetProperty(cx, callObj, "number", callNumber);
        
        JS::Rooted<JS::Value> numberPresentation(cx);
        numberPresentation.setInt32(CALL_PRESENTATION_ALLOWED);
        JS_SetProperty(cx, callObj, "numberPresentation", numberPresentation);
        
        JS::Rooted<JS::Value> name(cx);
        name.setString(numberString);
        JS_SetProperty(cx, callObj, "name", name);
        
        JS::Rooted<JS::Value> namePresentation(cx);
        namePresentation.setInt32(CALL_PRESENTATION_ALLOWED);
        JS_SetProperty(cx, callObj, "namePresentation", namePresentation);
        
        JS::Rooted<JS::Value> radioTech(cx);
        radioTech.setInt32(RADIO_TECH_WIFI);
        JS_SetProperty(cx, callObj, "radioTech", radioTech);
        
        JS::Rooted<JS::Value> voiceQuality(cx);
        voiceQuality.setInt32(CALL_VOICE_QUALITY_HD);
        JS_SetProperty(cx, callObj, "voiceQuality", voiceQuality);
        
        JS::Rooted<JS::Value> value(cx, JS::ObjectValue(*callObj));
        JS_SetProperty(cx, callsObj, JS_EncodeString(cx, ToString(cx, callIndex)), value);

    }

    JS::Rooted<JS::Value> callMessage(cx, JS::ObjectValue(*callsObj));

    // If the current incoming call action is reject or call function is disabled,
    // we need reject the incoming call and give the reason as the local call is busy.
    // Note: The reject action will be set when the secondary card is in the calling,
    //       then we need reject all the incoming call from the VOWIFI.
    if (!IsCallFunEnabled()
        || mIncomingCallAction == IncomingCallAction::REJECT) {
        Reject(); //TBD
    } else {
        // Send the incoming call callback.
        if (mListener != nullptr) mListener->OnCallIncoming(GetCallIndex(sessionId), 0);
        if (telephonyService){
            uint32_t serviceID = 0;  //single card
            telephonyService->NotifyCurrentCalls(serviceID, callMessage);
        }

    }

    // If the user enable the auto answer prop, we need answer this call immediately.  //TBD
    // bool isAutoAnswer = SystemProperties.getBoolean(PROP_KEY_AUTO_ANSWER, false);
    // if (isAutoAnswer) {
    //     Accept(false, info);
    //  }
}

// handle call alerted and call talking
void VowifiCallManager::HandleCallProcessing(int sessionId, string &callee, bool isVideo, int callState){
    LOG("Handle the alerted or outgoing call.");

    for(uint32_t i = 0; i < mCallInfo.Length(); i++){
        if(mCallInfo[i].id == (uint32_t)sessionId){
            mCallInfo[i].isVedio = isVideo;
            mCallInfo[i].number = callee;
            mCallInfo[i].callState = callState;
            break;
        }
    }
    nsCOMPtr<nsIGonkTelephonyService> telephonyService =
      do_GetService(TELEPHONY_SERVICE_CONTRACTID);

    if (!telephonyService) {
      LOGEA("Could not acquire nsITelephonyService!");
    }
    
    // AutoJSContext cx;
    mozilla::AutoSafeJSContext cx;
    JSAutoCompartment ac(cx, xpc::PrivilegedJunkScope());
    
    JS::Rooted<JSObject*> callsObj(cx);
    callsObj = JS_NewPlainObject(cx);
    
    for(uint32_t i = 0; i < mCallInfo.Length(); i++){
    
        JS::Rooted<JSObject*> callObj(cx);
        callObj = JS_NewPlainObject(cx);
        
        JS::Rooted<JS::Value> state(cx);
        state.setInt32(mCallInfo[i].callState);
        JS_SetProperty(cx, callObj, "state", state);
        
        JS::Rooted<JS::Value> callIndex(cx);
        callIndex.setInt32(mCallInfo[i].index);
        JS_SetProperty(cx, callObj, "callIndex", callIndex);
        
        JS::Rooted<JS::Value> toa(cx);
        toa.setInt32(TOA_UNKNOWN);
        JS_SetProperty(cx, callObj, "toa", toa);
        
        JS::Rooted<JS::Value> isMpty(cx);
        isMpty.setBoolean(false);
        JS_SetProperty(cx, callObj, "isMpty", isMpty);
        
        JS::Rooted<JS::Value> isMT(cx);
        isMT.setBoolean(true);
        JS_SetProperty(cx, callObj, "isMT", isMT);
        
        JS::Rooted<JS::Value> isVoice(cx);
        isVoice.setBoolean(true);
        JS_SetProperty(cx, callObj, "isVoice", isVoice);
        
        JS::Rooted<JS::Value> callNumber(cx);
        JS::Rooted<JSString*> numberString(cx,
                                       JS_NewStringCopyZ(cx, (mCallInfo[i].number).c_str()));
        
        callNumber.setString(numberString);
        JS_SetProperty(cx, callObj, "number", callNumber);
        
        JS::Rooted<JS::Value> numberPresentation(cx);
        numberPresentation.setInt32(CALL_PRESENTATION_ALLOWED);
        JS_SetProperty(cx, callObj, "numberPresentation", numberPresentation);
        
        JS::Rooted<JS::Value> name(cx);
        name.setString(numberString);
        JS_SetProperty(cx, callObj, "name", name);
        
        JS::Rooted<JS::Value> namePresentation(cx);
        namePresentation.setInt32(CALL_PRESENTATION_ALLOWED);
        JS_SetProperty(cx, callObj, "namePresentation", namePresentation);
        
        JS::Rooted<JS::Value> radioTech(cx);
        radioTech.setInt32(RADIO_TECH_WIFI);
        JS_SetProperty(cx, callObj, "radioTech", radioTech);
        
        JS::Rooted<JS::Value> voiceQuality(cx);
        voiceQuality.setInt32(CALL_VOICE_QUALITY_HD);
        JS_SetProperty(cx, callObj, "voiceQuality", voiceQuality);
        
        JS::Rooted<JS::Value> value(cx, JS::ObjectValue(*callObj));
        JS_SetProperty(cx, callsObj, JS_EncodeString(cx, ToString(cx, callIndex)), value);
    
    }
    
    
    JS::Rooted<JS::Value> callMessage(cx, JS::ObjectValue(*callsObj));
    
    if (telephonyService){
        uint32_t serviceID = 0;  //single card
        telephonyService->NotifyCurrentCalls(serviceID, callMessage);
    }

}

void VowifiCallManager::HandleCallTermed(int sessionId, int stateCode,
        string &callee) {
    LOG("Handle the call Terminated.");
    for (uint32_t i = 0; i < mCallInfo.Length(); i++) {
        if (mCallInfo[i].id == (uint32_t) sessionId ) {
            if(!mCallInfo[i].isConf){
                mCallInfo.RemoveElementAt(i);
                break;
            }
            else{
                RemoveCall();
                LOG("Received the BYE message after REFER");/*if the call is added to conference, should not be temrinate*/
                return;
            }

        }
    }
    if(mCallInfo.Length() < 2){
        bool isConfAlive = false;
        for(uint32_t i = 0; i < mCallInfo.Length(); i++){
            if(mCallInfo[i].isConf){
                isConfAlive = true;
                break;
            }
        }
        if(!isConfAlive){
            mConfCallInfo.confId = 0;
        }
    }

    nsCOMPtr<nsIGonkTelephonyService> telephonyService = do_GetService(
            TELEPHONY_SERVICE_CONTRACTID);

    if (!telephonyService) {
        LOGEA("Could not acquire nsITelephonyService!");
    }

    // AutoJSContext cx;
    mozilla::AutoSafeJSContext cx;
    JSAutoCompartment ac(cx, xpc::PrivilegedJunkScope());

    if (mCallInfo.Length() == 0) {
        JS::Rooted<JSObject*> callObj(cx);
        callObj = JS_NewPlainObject(cx);

        JS::Rooted < JS::Value > callMessage(cx, JS::ObjectValue(*callObj));
        if (telephonyService) {
            uint32_t serviceID = 0;  //single card
            telephonyService->NotifyCurrentCalls(serviceID, callMessage);
        }

        RemoveCall();
        // clear pending action list
        PendingActionClear();
    } else {
        JS::Rooted<JSObject*> callsObj(cx);
        callsObj = JS_NewPlainObject(cx);

        for (uint32_t i = 0; i < mCallInfo.Length(); i++) {
            JS::Rooted<JSObject*> callObj(cx);
            callObj = JS_NewPlainObject(cx);

            JS::Rooted < JS::Value > state(cx);
            state.setInt32(mCallInfo[i].callState);
            JS_SetProperty(cx, callObj, "state", state);

            JS::Rooted < JS::Value > callIndex(cx);
            callIndex.setInt32(mCallInfo[i].index);
            JS_SetProperty(cx, callObj, "callIndex", callIndex);

            JS::Rooted < JS::Value > toa(cx);
            toa.setInt32(TOA_UNKNOWN);
            JS_SetProperty(cx, callObj, "toa", toa);

            JS::Rooted < JS::Value > isMpty(cx);
            isMpty.setBoolean(mCallInfo[i].isConf);
            JS_SetProperty(cx, callObj, "isMpty", isMpty);

            JS::Rooted < JS::Value > isMT(cx);
            isMT.setBoolean(true);
            JS_SetProperty(cx, callObj, "isMT", isMT);

            JS::Rooted < JS::Value > isVoice(cx);
            isVoice.setBoolean(true);
            JS_SetProperty(cx, callObj, "isVoice", isVoice);

            JS::Rooted < JS::Value > callNumber(cx);
            JS::Rooted<JSString*> numberString(cx,
                    JS_NewStringCopyZ(cx, (mCallInfo[i].number).c_str()));

            callNumber.setString(numberString);
            JS_SetProperty(cx, callObj, "number", callNumber);

            JS::Rooted < JS::Value > numberPresentation(cx);
            numberPresentation.setInt32(CALL_PRESENTATION_ALLOWED);
            JS_SetProperty(cx, callObj, "numberPresentation",
                    numberPresentation);

            JS::Rooted < JS::Value > name(cx);
            name.setString(numberString);
            JS_SetProperty(cx, callObj, "name", name);

            JS::Rooted < JS::Value > namePresentation(cx);
            namePresentation.setInt32(CALL_PRESENTATION_ALLOWED);
            JS_SetProperty(cx, callObj, "namePresentation", namePresentation);

            JS::Rooted < JS::Value > radioTech(cx);
            radioTech.setInt32(RADIO_TECH_WIFI);
            JS_SetProperty(cx, callObj, "radioTech", radioTech);

            JS::Rooted < JS::Value > voiceQuality(cx);
            voiceQuality.setInt32(CALL_VOICE_QUALITY_HD);
            JS_SetProperty(cx, callObj, "voiceQuality", voiceQuality);

            JS::Rooted < JS::Value > value(cx, JS::ObjectValue(*callObj));
            JS_SetProperty(cx, callsObj,
                    JS_EncodeString(cx, ToString(cx, callIndex)), value);

        }

        JS::Rooted < JS::Value > callMessage(cx, JS::ObjectValue(*callsObj));
        if (telephonyService) {
            uint32_t serviceID = 0;  //single card
            telephonyService->NotifyCurrentCalls(serviceID, callMessage);
        }
    }
}

void VowifiCallManager::HandleCallHoldOrResume(int sessionId, int eventCode,
        string &callee) {
    LOG("Handle call Hold or Resume, conf/sess Id: %d, eventCode: %d", sessionId, eventCode);

    nsCOMPtr<nsIGonkTelephonyService> telephonyService = do_GetService(
            TELEPHONY_SERVICE_CONTRACTID);

    if (!telephonyService) {
        LOGEA("Could not acquire nsITelephonyService!");
    }

    string remoteNotify;
    bool isConferenceHold = false;

    if (eventCode == CALLBACK_HOLD_OK) {
        for (uint32_t i = 0; i < mCallInfo.Length(); i++){
            if (mCallInfo[i].id == (uint32_t)sessionId) {
                mCallInfo[i].isActive = false;
                mCallInfo[i].callState = CALL_STATE_HELD;
                isConferenceHold = mCallInfo[i].isConfHold;
                break;
            }
        }

        //for conference, if hold ok, should merge, and the hold result should not notify to gecko
        if(mCallInfo.Length() > 1 && isConferenceHold){
            bool isActive = false;
            for (uint32_t i = 0; i < mCallInfo.Length(); i++){
                isActive |= mCallInfo[i].isActive;  //only if all the call is not active, isActive be false;
            }
            if(!isActive){
                if(mConfCallInfo.confId == 0){
                    ConferenceCall(true);
                }
                else{
                    InviteConfCallMember();
                }

                if (mListener != nullptr) {
                    mListener->OnHoldResumeOK();
                }
                return;
            }
        }

    } else if (eventCode == CALLBACK_UNHOLD_OK) {
        for (uint32_t i = 0; i < mCallInfo.Length(); i++){
            if (mCallInfo[i].id == (uint32_t)sessionId) {
                mCallInfo[i].isActive = true;
                mCallInfo[i].callState = CALL_STATE_CONNECTED;
                break;
            }
        }
    }else if(eventCode == CALLBACK_CONFCB_HOLD_OK){
        LOG("conference hold ok");
        for (uint32_t i = 0; i < mCallInfo.Length(); i++){
            if (mCallInfo[i].isActive && mCallInfo[i].isConf) {
                mCallInfo[i].isActive = false;
                mCallInfo[i].callState = CALL_STATE_HELD;
            }
        }

    }else if(eventCode == CALLBACK_CONFCB_UNHOLD_OK){
        LOG("conference resume ok");
        for (uint32_t i = 0; i < mCallInfo.Length(); i++){
            if (!mCallInfo[i].isActive && mCallInfo[i].isConf) {
                mCallInfo[i].isActive = true;
                mCallInfo[i].callState = CALL_STATE_CONNECTED;
            }
        }
    }
    switch (eventCode) {
    case CALLBACK_HOLD_OK:
    case CALLBACK_UNHOLD_OK:
    case CALLBACK_CONFCB_HOLD_OK:
    case CALLBACK_CONFCB_UNHOLD_OK:{
        LOG("inform the hold/unhold result to up layer");
        if (mListener != nullptr) {
            mListener->OnHoldResumeOK();
        }
        // AutoJSContext cx;
        mozilla::AutoSafeJSContext cx;
        JSAutoCompartment ac(cx, xpc::PrivilegedJunkScope());

        JS::Rooted<JSObject*> callsObj(cx);
        callsObj = JS_NewPlainObject(cx);

        for(uint32_t i = 0; i < mCallInfo.Length(); i++){

            JS::Rooted<JSObject*> callObj(cx);
            callObj = JS_NewPlainObject(cx);
            
            JS::Rooted<JS::Value> state(cx);
            state.setInt32(mCallInfo[i].callState);
            
            LOG("hold/unhold, id:%u, number:%s, state is %d",
                mCallInfo[i].id, mCallInfo[i].number.c_str(), mCallInfo[i].callState);

            JS_SetProperty(cx, callObj, "state", state);
            
            JS::Rooted<JS::Value> callIndex(cx);
            callIndex.setInt32(mCallInfo[i].index);
            JS_SetProperty(cx, callObj, "callIndex", callIndex);
            
            JS::Rooted<JS::Value> toa(cx);
            toa.setInt32(TOA_UNKNOWN);
            JS_SetProperty(cx, callObj, "toa", toa);
            
            JS::Rooted<JS::Value> isMpty(cx);
            isMpty.setBoolean(mCallInfo[i].isConf);
            JS_SetProperty(cx, callObj, "isMpty", isMpty);
            
            JS::Rooted<JS::Value> isMT(cx);
            isMT.setBoolean(true);
            JS_SetProperty(cx, callObj, "isMT", isMT);
            
            JS::Rooted<JS::Value> isVoice(cx);
            isVoice.setBoolean(true);
            JS_SetProperty(cx, callObj, "isVoice", isVoice);
            
            JS::Rooted<JS::Value> callNumber(cx);
            JS::Rooted<JSString*> numberString(cx,
                                           JS_NewStringCopyZ(cx, (mCallInfo[i].number).c_str()));
            
            callNumber.setString(numberString);
            JS_SetProperty(cx, callObj, "number", callNumber);
            
            JS::Rooted<JS::Value> numberPresentation(cx);
            numberPresentation.setInt32(CALL_PRESENTATION_ALLOWED);
            JS_SetProperty(cx, callObj, "numberPresentation", numberPresentation);
            
            JS::Rooted<JS::Value> name(cx);
            name.setString(numberString);
            JS_SetProperty(cx, callObj, "name", name);
            
            JS::Rooted<JS::Value> namePresentation(cx);
            namePresentation.setInt32(CALL_PRESENTATION_ALLOWED);
            JS_SetProperty(cx, callObj, "namePresentation", namePresentation);
            
            JS::Rooted<JS::Value> radioTech(cx);
            radioTech.setInt32(RADIO_TECH_WIFI);
            JS_SetProperty(cx, callObj, "radioTech", radioTech);
            
            JS::Rooted<JS::Value> voiceQuality(cx);
            voiceQuality.setInt32(CALL_VOICE_QUALITY_HD);
            JS_SetProperty(cx, callObj, "voiceQuality", voiceQuality);
            
            JS::Rooted<JS::Value> value(cx, JS::ObjectValue(*callObj));
            JS_SetProperty(cx, callsObj, JS_EncodeString(cx, ToString(cx, callIndex)), value);

        }


        JS::Rooted<JS::Value> callMessage(cx, JS::ObjectValue(*callsObj));

        if (telephonyService) {
            uint32_t serviceID = 0;  //single card
            telephonyService->NotifyCurrentCalls(serviceID, callMessage);
        }
    }
        break;
    case CALLBACK_HOLD_FAILED:
    case CALLBACK_UNHOLD_FAILED:
    case CALLBACK_CONFCB_HOLD_FAILED:
    case CALLBACK_CONFCB_UNHOLD_FAILED:
        LOG("Hold or UnHold failed");
        if (mListener != nullptr) {
            mListener->OnHoldResumeFailed();
        }
        break;

    case CALLBACK_HELD:
        remoteNotify = "RemoteHeld";
        if (telephonyService) {
            uint32_t serviceID = 0;  //single card
            telephonyService->NotifySupplementaryService(serviceID,
                    NS_ConvertASCIItoUTF16(callee.c_str()),
                    NS_ConvertASCIItoUTF16(remoteNotify.c_str()));
        }
        break;
    case CALLBACK_UNHELD:
        remoteNotify = "RemoteResumed";
        if (telephonyService) {
            uint32_t serviceID = 0;  //single card
            telephonyService->NotifySupplementaryService(serviceID,
                    NS_ConvertASCIItoUTF16(callee.c_str()),
                    NS_ConvertASCIItoUTF16(remoteNotify.c_str()));
        }
        break;
    default:
        LOG("eventId %d is not supported, skip it!", eventCode);
        break;
    }
}

void VowifiCallManager::HandleRTPReceived(int sessionId, bool isVideo, bool isReceived){
    LOG("Handle the rtp recieved: %d", isReceived);
    if (IsCallAlive(sessionId)) {
        if (mListener != nullptr) {
            mListener->OnCallRTPReceived(isVideo, isReceived);
        }
    }
}

void VowifiCallManager::HandleRTCPChanged(int lose, int jitter, int rtt, bool isVideo){
    LOG("Handle the rtcp changed.");
    if (mListener != nullptr) {
        mListener->OnCallRTCPChanged(isVideo, lose, jitter, rtt);
    }
}

void VowifiCallManager::HandleConfTermed(){
    LOG("Handle the conference terminated.");
    for (uint32_t i = 0; i < mCallInfo.Length(); i++) {
        if (mCallInfo[i].isConf) {
            mCallInfo.RemoveElementAt(i);
        }
    }

    nsCOMPtr<nsIGonkTelephonyService> telephonyService = do_GetService(
            TELEPHONY_SERVICE_CONTRACTID);
    
    if (!telephonyService) {
        LOGEA("Could not acquire nsITelephonyService!");
    }
    
    // AutoJSContext cx;
    mozilla::AutoSafeJSContext cx;
    JSAutoCompartment ac(cx, xpc::PrivilegedJunkScope());
    
    if (mCallInfo.Length() == 0) {
    
        JS::Rooted<JSObject*> callObj(cx);
        callObj = JS_NewPlainObject(cx);
    
        JS::Rooted < JS::Value > callMessage(cx, JS::ObjectValue(*callObj));
        if (telephonyService) {
            uint32_t serviceID = 0;  //single card
            telephonyService->NotifyCurrentCalls(serviceID, callMessage);
        }
    
    } else {
        JS::Rooted<JSObject*> callsObj(cx);
        callsObj = JS_NewPlainObject(cx);
        
        for (uint32_t i = 0; i < mCallInfo.Length(); i++) {
        
            JS::Rooted<JSObject*> callObj(cx);
            callObj = JS_NewPlainObject(cx);
        
            JS::Rooted < JS::Value > state(cx);
            state.setInt32(CALL_STATE_CONNECTED);
            JS_SetProperty(cx, callObj, "state", state);
        
            JS::Rooted < JS::Value > callIndex(cx);
            callIndex.setInt32(mCallInfo[i].index);
            JS_SetProperty(cx, callObj, "callIndex", callIndex);
        
            JS::Rooted < JS::Value > toa(cx);
            toa.setInt32(TOA_UNKNOWN);
            JS_SetProperty(cx, callObj, "toa", toa);
        
            JS::Rooted < JS::Value > isMpty(cx);
            isMpty.setBoolean(false);
            JS_SetProperty(cx, callObj, "isMpty", isMpty);
        
            JS::Rooted < JS::Value > isMT(cx);
            isMT.setBoolean(true);
            JS_SetProperty(cx, callObj, "isMT", isMT);
        
            JS::Rooted < JS::Value > isVoice(cx);
            isVoice.setBoolean(true);
            JS_SetProperty(cx, callObj, "isVoice", isVoice);
        
            JS::Rooted < JS::Value > callNumber(cx);
            JS::Rooted<JSString*> numberString(cx,
                    JS_NewStringCopyZ(cx, (mCallInfo[i].number).c_str()));
        
            callNumber.setString(numberString);
            JS_SetProperty(cx, callObj, "number", callNumber);
        
            JS::Rooted < JS::Value > numberPresentation(cx);
            numberPresentation.setInt32(CALL_PRESENTATION_ALLOWED);
            JS_SetProperty(cx, callObj, "numberPresentation",
                    numberPresentation);
        
            JS::Rooted < JS::Value > name(cx);
            name.setString(numberString);
            JS_SetProperty(cx, callObj, "name", name);
        
            JS::Rooted < JS::Value > namePresentation(cx);
            namePresentation.setInt32(CALL_PRESENTATION_ALLOWED);
            JS_SetProperty(cx, callObj, "namePresentation", namePresentation);
        
            JS::Rooted < JS::Value > radioTech(cx);
            radioTech.setInt32(RADIO_TECH_WIFI);
            JS_SetProperty(cx, callObj, "radioTech", radioTech);
        
            JS::Rooted < JS::Value > voiceQuality(cx);
            voiceQuality.setInt32(CALL_VOICE_QUALITY_HD);
            JS_SetProperty(cx, callObj, "voiceQuality", voiceQuality);
        
            JS::Rooted < JS::Value > value(cx, JS::ObjectValue(*callObj));
            JS_SetProperty(cx, callsObj,
                    JS_EncodeString(cx, ToString(cx, callIndex)), value);
        
        }
        
        JS::Rooted < JS::Value > callMessage(cx, JS::ObjectValue(*callsObj));
        if (telephonyService) {
            uint32_t serviceID = 0;  //single card
            telephonyService->NotifyCurrentCalls(serviceID, callMessage);
        }
    }

    RemoveCall();
}

void VowifiCallManager::HandleMergeFailed(){
    LOG("Handle conference merge failed");
    for (uint32_t i = 0; i < mCallInfo.Length(); i++) {
        if (!mCallInfo[i].isConf && mCallInfo[i].callState == CALL_STATE_HELD) {
            Resume(mCallInfo[i].index);
            break;
        }
    }
}

void VowifiCallManager::UpdateRegisterState(int registerState){
    mRegisterState = registerState;
}


int VowifiCallManager::GetCallCount() {
    return mCallInfo.Length();
}

bool VowifiCallManager::PendingActionPush(ActionType_T origType,
                                          bool isPrevConf,
                                          int iPrevId,
                                          ActionType_T prevType,
                                          bool isPendConf,
                                          int iPendIndx,
                                          ActionType_T pendType)
{
    PendingAction* p_pendAct = new PendingAction(origType, isPrevConf, isPendConf,
                                      prevType, pendType, iPrevId, iPendIndx);
    if (nullptr == p_pendAct){
        LOGEA("push PendingAction construct pending action object failed");
        return false;
    }

    LOG("push PendingAction previous Id:%d isConf:%d type:%d, pending Index:%d, type:%d",
        iPrevId, isPrevConf, prevType, iPendIndx, pendType);
    mPendActions.push_back(p_pendAct);
    return true;
}

bool VowifiCallManager::PendingActionPush(ActionType_T origType,
                                          bool isPrevConf,
                                          int iPrevId,
                                          ActionType_T prevType,
                                          bool isPendConf,
                                          ActionType_T pendType)
{
    PendingAction* p_pendAct = new PendingAction(origType, isPrevConf, isPendConf,
                                      prevType, pendType, iPrevId);
    if (nullptr == p_pendAct){
        LOGEA("push PendingAction construct pending action object failed");
        return false;
    }

    LOG("push PendingAction previous Id:%d isConf:%d type:%d, pending isConf:%d, type:%d",
        iPrevId, isPrevConf, prevType, isPendConf, pendType);
    mPendActions.push_back(p_pendAct);
    return true;
}

VowifiCallManager::ActionProcResult_T VowifiCallManager::PendingActionProcess(int id, int evntCode)
{
    LOG("process PendingAction when received conf/sess Id: %d eventCode: %d",
         id, evntCode);
    int iSize = (int)mPendActions.size();
    ActionProcResult_T tResult = ACTION_PROC_CONTINUE;
    PendingAction* pPendAct = nullptr;

    if (iSize == 0)
    {
        LOG("the Pending Action size: %d, skip it", iSize);
        return ACTION_PROC_FINISH;
    }

    pPendAct = mPendActions.front();
    if (pPendAct == nullptr)
    {
        LOGEA("the front Pending Action is object null");
        mPendActions.erase(mPendActions.begin());
        return ACTION_PROC_FAILED;
    }

    if (pPendAct->_mPrevId != id)
    {
        LOG("the front Pending Action id doesn't match, front id: %d, evnt id: %d, skip it",
            pPendAct->_mPrevId, id);
        return ACTION_PROC_CONTINUE;
    }

    // TODO, the support type of current
    switch (evntCode)
    {
        case CALLBACK_HOLD_OK:
        case CALLBACK_UNHOLD_OK:
        case CALLBACK_CONFCB_HOLD_OK:
        case CALLBACK_CONFCB_UNHOLD_OK:
            {
                if (pPendAct->_mPrevActionT != ActionType_T::ENUM_ACTION_HOLD
                   && pPendAct->_mPrevActionT != ActionType_T::ENUM_ACTION_UNHOLD)
                {
                    LOGWA("process PA received evntCode: %d doesn't match pending type: %d",
                        evntCode, pPendAct->_mPrevActionT);
                }else{
                    // 1.  process the pending action
                    // pending a conference action
                    if (pPendAct->_mPendIsConf)
                    {
                        if (pPendAct->_mPendActionT == ENUM_ACTION_HOLD)
                        {
                            ConfHold();
                        }else if (pPendAct->_mPendActionT == ENUM_ACTION_UNHOLD)
                        {
                            ConfResume();
                        }else{
                            LOGWA("process PA conference operation type: %d doesn't support",
                                  pPendAct->_mPendActionT);
                            tResult = ACTION_PROC_FAILED;
                        }

                    // pending a normal call action
                    }else{
                        if (pPendAct->_mPendActionT == ENUM_ACTION_HOLD)
                        {
                            Hold(pPendAct->_mPendIndex);
                        }else if (pPendAct->_mPendActionT == ENUM_ACTION_UNHOLD)
                        {
                            Resume(pPendAct->_mPendIndex);
                        }else if (pPendAct->_mPendActionT == ENUM_ACTION_ACCEPT)
                        {
                            Accept();
                        }else{
                            LOGWA("process PA call operation type: %d doesn't support",
                                  pPendAct->_mPendActionT);
                            tResult = ACTION_PROC_FAILED;
                        }
                    }

                    // 2. remove the front pending action from list
                    mPendActions.erase(mPendActions.begin());
                }
            }
            break;
        case CALLBACK_HOLD_FAILED:
        case CALLBACK_UNHOLD_FAILED:
        case CALLBACK_CONFCB_HOLD_FAILED:
        case CALLBACK_CONFCB_UNHOLD_FAILED:
            LOGWA("process PA received evntCode: %d is an operation failed", evntCode);
            break;
        default:
            LOGWA("process PA received evntCode: %d is not supported", evntCode);
            break;
    }

    return tResult;
}

bool VowifiCallManager::PendingActionClear()
{
    vector<PendingAction*>::iterator vIter;
    for (vIter = mPendActions.begin(); vIter != mPendActions.end(); ++vIter)
    {
        if (*vIter == nullptr){
            LOGWA("clean PendingAction find a pending action object is null");
            continue;
        }
        delete *vIter;
    }
    mPendActions.clear();
    return true;
}

} // namespace vowifiadapter
} // namespace dom
} // namespace mozilla
