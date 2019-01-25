/************************************************************************

        Copyright (c) 2005-2017 by Spreadtrum, Inc.
                       All rights reserved.

     This software is confidential and proprietary to Spreadtrum,
     Inc. No part of this software may be reproduced, stored, transmitted,
     disclosed or used in any form or by any means other than as expressly
     provided by the written license agreement between Spreadtrum and its
     licensee.

************************************************************************/

#include "mozilla/ClearOnShutdown.h"
#include "VowifiInterfaceLayer.h"
#include "mozilla/ModuleUtils.h"

#include "imsconnman/ImsConnManMonitor.h" //for debug
using mozilla::dom::vowifi::ImsConnManMonitor; //for debug

namespace mozilla {
namespace dom {
namespace vowifiadapter {

#undef LOG
#define LOG(args...)  __android_log_print(ANDROID_LOG_INFO, "[VowifiAdapter]" , ## args)

static  StaticRefPtr<VowifiInterfaceLayer> gVowifiInterfaceLayer;

NS_IMPL_ISUPPORTS(VowifiInterfaceLayer, nsIVowifiInterfaceLayer)

already_AddRefed<VowifiInterfaceLayer> VowifiInterfaceLayer::GetInstance() {
    LOG("entering VowifiInterfaceLayer GetInstance");
    if (!gVowifiInterfaceLayer) {
        gVowifiInterfaceLayer = new VowifiInterfaceLayer();
        ClearOnShutdown(&gVowifiInterfaceLayer);
    }
    RefPtr<VowifiInterfaceLayer> vowifiService = gVowifiInterfaceLayer.get();
    return vowifiService.forget();
}

VowifiInterfaceLayer::VowifiInterfaceLayer() {

    LOG("entering VowifiInterfaceLayer initilization");

    mAdapterControl = new VowifiAdapterControl();
    if(nullptr == mAdapterControl)
    {
        LOG("VowifiSocketControl create failed");
            return;
    }
   // mConnClient->SetMsgHandler(mAdapterControl);

}

VowifiInterfaceLayer::~VowifiInterfaceLayer() {
    LOG("VowifiInterfaceLayer destructor...");

    gVowifiInterfaceLayer = nullptr;
    if (mAdapterControl) {
        delete mAdapterControl;
        mAdapterControl = nullptr;
    }

}

/* void RegisterCallback(nsIVowifiCallback* mCallback); */
NS_IMETHODIMP VowifiInterfaceLayer::RegisterCallback(
        nsIVowifiCallback* Callback) {
    LOG("VowifiInterfaceLayer::RegisterCallback been called");
    mAdapterControl->mCallback = Callback;
    return NS_OK;
}

/* void unegisterCallback(); */
NS_IMETHODIMP VowifiInterfaceLayer::UnregisterCallback() {
    LOG("VowifiInterfaceLayer::UnregisterCallback been called");
    if(mAdapterControl != nullptr)
    {
        mAdapterControl->mCallback = nullptr;
    }
    return NS_OK;
}

/* void resetAll (in PRInt32 delayMillis); */
NS_IMETHODIMP VowifiInterfaceLayer::ResetAll(uint16_t wifiState, uint32_t delayMillis) {
    LOG("VowifiInterfaceLayer::Resetall been called");
    if(mAdapterControl != nullptr)
    {
        mAdapterControl->mResetStep = RESET_STEP_DEREGISTER;
        mAdapterControl->ResetAll();
    }
    return NS_OK;
}

/* void attach (); */
NS_IMETHODIMP VowifiInterfaceLayer::Attach() {
    LOG("VowifiInterfaceLayer::Attach been called");
    if(mAdapterControl != nullptr)
    {
        mAdapterControl->Attach();
    }
    return NS_OK;
}

/* void deattach (); */
NS_IMETHODIMP VowifiInterfaceLayer::Deattach() {
    LOG("Entering Deattach....");
    DeattachHandover(false);

    return NS_OK;
}

/* void deattachHandover (in boolean forHandover); */
NS_IMETHODIMP VowifiInterfaceLayer::DeattachHandover(bool forHandover) {
    LOG("VowifiInterfaceLayer::DeattachHandover been called");
    if(mAdapterControl != nullptr)
    {
        mAdapterControl->Deattach(forHandover);
    }
    return NS_OK;
}

/* void imsRegister (); */
NS_IMETHODIMP VowifiInterfaceLayer::ImsRegister() {
    LOG("VowifiInterfaceLayer::ImsRegister been called");
    if(mAdapterControl != nullptr)
    {
        mAdapterControl->Register();
    }
    return NS_OK;
}

/* void imsReregister (in short networkType, in AString networkInfo); */
NS_IMETHODIMP VowifiInterfaceLayer::ImsReregister(int16_t networkType, const nsAString & info){
    LOG("VowifiInterfaceLayer::ImsReregister been called");
    string networkInfo = string(
                    NS_LossyConvertUTF16toASCII(info).get());
    if(mAdapterControl != nullptr)
    {
        mAdapterControl->Reregister(networkType, networkInfo);
    }
    return NS_OK;
}


/* void imsDeregister (); */
NS_IMETHODIMP VowifiInterfaceLayer::ImsDeregister() {
    LOG("VowifiInterfaceLayer::ImsDeregister been called");
    if(mAdapterControl != nullptr)
    {
        mAdapterControl->Deregister();
    }
    return NS_OK;
}

/* void dial (in AString peerNumber, in boolean isEmergency, in nsIRilSendWorkerMessageCallback callback); */
NS_IMETHODIMP VowifiInterfaceLayer::Dial(const nsAString & peerNumber, bool isEmergency, nsIRilSendWorkerMessageCallback *callback)
{
    LOG("VowifiInterfaceLayer::Dial been called");
    if(mAdapterControl != nullptr)
    {
        mozilla::AutoSafeJSContext cx;
        JSAutoCompartment ac(cx, xpc::PrivilegedJunkScope());
        JS::Rooted<JSObject*> callObj(cx);
        callObj = JS_NewPlainObject(cx);
        bool resultValue = true;
        bool *result = &resultValue;
        string number = string(
                NS_LossyConvertUTF16toASCII(peerNumber).get());
        if(!mAdapterControl->CallStart(number, isEmergency)){

            JS::Rooted<JSString*> erroString(cx,
                                             JS_NewStringCopyZ(cx, "Dial failed"));
            JS::Rooted<JS::Value> errorMsg(cx);
            errorMsg.setString(erroString);
            JS_SetProperty(cx, callObj, "errorMsg", errorMsg);

            JS::Rooted<JS::Value> aMessage(cx, JS::ObjectValue(*callObj));
            callback->HandleResponse(aMessage, result);
        }
        else{
            JS::Rooted < JS::Value > isEmergency(cx);
            isEmergency.setBoolean(false);
            JS_SetProperty(cx, callObj, "isEmergency", isEmergency);

            JS::Rooted < JS::Value > callNumber(cx);
            JS::Rooted<JSString*> numberString(cx,
                    JS_NewStringCopyZ(cx, number.c_str()));

            callNumber.setString(numberString);
            JS_SetProperty(cx, callObj, "number", callNumber);

            JS::Rooted<JS::Value> aMessage(cx, JS::ObjectValue(*callObj));
            callback->HandleResponse(aMessage, result);
        }
    }
    return NS_OK;
}

/* void hungUpCall (in unsigned long callIndex, in nsIRilSendWorkerMessageCallback callback); */
NS_IMETHODIMP VowifiInterfaceLayer::HungUpCall(uint32_t callIndex, nsIRilSendWorkerMessageCallback *callback)
{
    LOG("VowifiInterfaceLayer::HungUpCall been called");
    if(mAdapterControl != nullptr)
    {
        mozilla::AutoSafeJSContext cx;
        JSAutoCompartment ac(cx, xpc::PrivilegedJunkScope());
        JS::Rooted<JSObject*> callObj(cx);
        callObj = JS_NewPlainObject(cx);
        bool resultValue = true;
        bool *result = &resultValue;
        if(!mAdapterControl->CallTerminate(callIndex)){
            JS::Rooted<JSString*> erroString(cx,
                                             JS_NewStringCopyZ(cx, "HungUpCall failed"));
            JS::Rooted<JS::Value> errorMsg(cx);
            errorMsg.setString(erroString);
            JS_SetProperty(cx, callObj, "errorMsg", errorMsg);

            JS::Rooted<JS::Value> aMessage(cx, JS::ObjectValue(*callObj));
            callback->HandleResponse(aMessage, result);
        }
        else{
            JS::Rooted < JS::Value > Index(cx);
            Index.setInt32(callIndex);
            JS_SetProperty(cx, callObj, "callIndex", Index);
            JS::Rooted<JS::Value> aMessage(cx, JS::ObjectValue(*callObj));
            callback->HandleResponse(aMessage, result);
        }
    }
    return NS_OK;
}

/* void switchActiveCall (in nsIRilSendWorkerMessageCallback callback); */
NS_IMETHODIMP VowifiInterfaceLayer::SwitchActiveCall(nsIRilSendWorkerMessageCallback *callback)
{
    LOG("VowifiInterfaceLayer::SwitchActiveCall been called");
    if(mAdapterControl != nullptr)
    {
        mozilla::AutoSafeJSContext cx;
        JSAutoCompartment ac(cx, xpc::PrivilegedJunkScope());
        JS::Rooted<JSObject*> callObj(cx);
        callObj = JS_NewPlainObject(cx);
        bool resultValue = true;
        bool *result = &resultValue;
        if(!mAdapterControl->CallSwitch(callback)){
            JS::Rooted<JSString*> erroString(cx,
                                             JS_NewStringCopyZ(cx, "SwitchActiveCall failed"));
            JS::Rooted<JS::Value> errorMsg(cx);
            errorMsg.setString(erroString);
            JS_SetProperty(cx, callObj, "errorMsg", errorMsg);

            JS::Rooted<JS::Value> aMessage(cx, JS::ObjectValue(*callObj));
            callback->HandleResponse(aMessage, result);
        }
    }
    //In case of error notify from native, hanldeResponse will be called when getting notify from native
    return NS_OK;;
}

/* void hangUpForeground (in nsIRilSendWorkerMessageCallback callback); */
NS_IMETHODIMP VowifiInterfaceLayer::HangUpForeground(nsIRilSendWorkerMessageCallback *callback)
{
    LOG("VowifiInterfaceLayer::HangUpForeground been called");
    if(mAdapterControl != nullptr)
    {
        mozilla::AutoSafeJSContext cx;
        JSAutoCompartment ac(cx, xpc::PrivilegedJunkScope());
        JS::Rooted<JSObject*> callObj(cx);
        callObj = JS_NewPlainObject(cx);
        bool resultValue = true;
        bool *result = &resultValue;
        if(!mAdapterControl->CallHangUpForeground()){

            JS::Rooted<JSString*> erroString(cx,
                                             JS_NewStringCopyZ(cx, "HangUpForeground failed"));
            JS::Rooted<JS::Value> errorMsg(cx);
            errorMsg.setString(erroString);
            JS_SetProperty(cx, callObj, "errorMsg", errorMsg);
            JS::Rooted<JS::Value> aMessage(cx, JS::ObjectValue(*callObj));
            callback->HandleResponse(aMessage, result);
        }
        else{
            JS::Rooted<JS::Value> aMessage(cx, JS::ObjectValue(*callObj));
            callback->HandleResponse(aMessage, result);
        }
    }
    return NS_OK;
}

/* void hangUpBackground (in nsIRilSendWorkerMessageCallback callback); */
NS_IMETHODIMP VowifiInterfaceLayer::HangUpBackground(nsIRilSendWorkerMessageCallback *callback)
{
    LOG("VowifiInterfaceLayer::HangUpBackground been called");
    if(mAdapterControl != nullptr)
    {
        mozilla::AutoSafeJSContext cx;
        JSAutoCompartment ac(cx, xpc::PrivilegedJunkScope());
        JS::Rooted<JSObject*> callObj(cx);
        callObj = JS_NewPlainObject(cx);
        bool resultValue = true;
        bool *result = &resultValue;
        if(!mAdapterControl->CallHangUpBackground()){
            JS::Rooted<JSString*> erroString(cx, JS_NewStringCopyZ(cx, "HangUpBackground failed"));
            JS::Rooted<JS::Value> errorMsg(cx);
            errorMsg.setString(erroString);
            JS_SetProperty(cx, callObj, "errorMsg", errorMsg);
            JS::Rooted<JS::Value> aMessage(cx, JS::ObjectValue(*callObj));
            callback->HandleResponse(aMessage, result);
        }
        else{
            JS::Rooted<JS::Value> aMessage(cx, JS::ObjectValue(*callObj));
            callback->HandleResponse(aMessage, result);
        }
    }
    return NS_OK;
}

/* void reject (in nsIRilSendWorkerMessageCallback callback); */
NS_IMETHODIMP VowifiInterfaceLayer::Reject(nsIRilSendWorkerMessageCallback *callback)
{
    LOG("VowifiInterfaceLayer::Reject been called");
    if(mAdapterControl != nullptr)
    {
        mozilla::AutoSafeJSContext cx;
        JSAutoCompartment ac(cx, xpc::PrivilegedJunkScope());
        JS::Rooted<JSObject*> callObj(cx);
        callObj = JS_NewPlainObject(cx);
        bool resultValue = true;
        bool *result = &resultValue;
        if(!mAdapterControl->CallReject()){
            JS::Rooted<JSString*> erroString(cx,
                                             JS_NewStringCopyZ(cx, "Reject failed"));
            JS::Rooted<JS::Value> errorMsg(cx);
            errorMsg.setString(erroString);
            JS_SetProperty(cx, callObj, "errorMsg", errorMsg);

            JS::Rooted<JS::Value> aMessage(cx, JS::ObjectValue(*callObj));
            callback->HandleResponse(aMessage, result);
        }
        else{
            JS::Rooted<JS::Value> aMessage(cx, JS::ObjectValue(*callObj));
            callback->HandleResponse(aMessage, result);
        }
    }
    return NS_OK;
}

/* void answerCall (in nsIRilSendWorkerMessageCallback callback); */
NS_IMETHODIMP VowifiInterfaceLayer::AnswerCall(nsIRilSendWorkerMessageCallback *callback)
{
    LOG("VowifiInterfaceLayer::AnswerCall been called");
    if(mAdapterControl != nullptr)
    {
        mozilla::AutoSafeJSContext cx;
        JSAutoCompartment ac(cx, xpc::PrivilegedJunkScope());
        JS::Rooted<JSObject*> callObj(cx);
        callObj = JS_NewPlainObject(cx);
        bool resultValue = true;
        bool *result = &resultValue;
        if(!mAdapterControl->CallAccept()){
            JS::Rooted<JSString*> erroString(cx,
                                             JS_NewStringCopyZ(cx, "AnswerCall failed"));
            JS::Rooted<JS::Value> errorMsg(cx);
            errorMsg.setString(erroString);
            JS_SetProperty(cx, callObj, "errorMsg", errorMsg);

            JS::Rooted<JS::Value> aMessage(cx, JS::ObjectValue(*callObj));
            callback->HandleResponse(aMessage, result);
        }
        else{
            JS::Rooted<JS::Value> aMessage(cx, JS::ObjectValue(*callObj));
            callback->HandleResponse(aMessage, result);
        }
    }
    return NS_OK;
}

/* void conferenceCall (in nsIRilSendWorkerMessageCallback callback); */
NS_IMETHODIMP VowifiInterfaceLayer::ConferenceCall(nsIRilSendWorkerMessageCallback *callback)
{
    LOG("VowifiInterfaceLayer::ConferenceCall been called");
    if(mAdapterControl != nullptr)
    {
        mozilla::AutoSafeJSContext cx;
        JSAutoCompartment ac(cx, xpc::PrivilegedJunkScope());
        JS::Rooted<JSObject*> callObj(cx);
        callObj = JS_NewPlainObject(cx);
        bool resultValue = true;
        bool *result = &resultValue;
        if(!mAdapterControl->ConferenceCall()){
            JS::Rooted<JSString*> erroString(cx,
                                             JS_NewStringCopyZ(cx, "ConferenceCall failed"));
            JS::Rooted<JS::Value> errorMsg(cx);
            errorMsg.setString(erroString);
            JS_SetProperty(cx, callObj, "errorMsg", errorMsg);

            JS::Rooted<JS::Value> aMessage(cx, JS::ObjectValue(*callObj));
            callback->HandleResponse(aMessage, result);
        }
        else{
            JS::Rooted<JS::Value> aMessage(cx, JS::ObjectValue(*callObj));
            callback->HandleResponse(aMessage, result);
        }
    }
    return NS_OK;
}

/* void separateCall (in unsigned long callIndex, in nsIRilSendWorkerMessageCallback callback); */
NS_IMETHODIMP VowifiInterfaceLayer::SeparateCall(uint32_t callIndex, nsIRilSendWorkerMessageCallback *callback)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* void setMute (in boolean isMute); */
NS_IMETHODIMP VowifiInterfaceLayer::SetMute(bool isMute)
{
    LOG("VowifiInterfaceLayer::SetMute been called");
    if(mAdapterControl != nullptr)
    {
        mAdapterControl->CallMute(isMute);
    }
    return NS_OK;
}


NS_IMETHODIMP VowifiInterfaceLayer::SendSMS(JS::HandleValue aMessage, nsIRilSendWorkerMessageCallback *callback)
{
    string msg; 

    if (!aMessage.isObject()) {
      MOZ_ASSERT(false, "Not a object.");
      return NS_ERROR_INVALID_ARG;
    }
  
    mozilla::AutoSafeJSContext cx;
    JS::Rooted<JSObject*> obj(cx, &aMessage.toObject());
    JS::Rooted<JS::Value> v(cx);

    //Parse receiver Number
    nsAutoJSString number;
    if (!JS_GetProperty(cx, obj, "number", &v)) {
      LOG("Receiver number is null");
      return NS_ERROR_INVALID_ARG;
    }
    if (!number.init(cx, v)) {
      LOG("Couldn't initialize receiver number from the request.");
      return NS_ERROR_INVALID_ARG;
    }

    string receiver = string(NS_LossyConvertUTF16toASCII(number).get());

    //Parse retryCount
    /*nsAutoJSString rc;
    if (!JS_GetProperty(cx, obj, "retryCount", &v)) {
      LOG("retryCount is null");
    }
    if (!rc.init(cx, v)) {
      LOG("Couldn't initialize retryCount from the request.");
    }
    int retCount = int(NS_LossyConvertUTF16toASCII(rc).get());
    */
    if (!JS_GetProperty(cx, obj, "retryCount", &v)  || !v.isNumber()) {
        LOG("retrycount is sendsms request  is null or v is not number");
    }
    int retCount = v.toNumber();
    LOG("VowifiInterfaceLayer:: retCount = %d", retCount);


    //Parse reference number
    /*nsAutoJSString rn;
    if (!JS_GetProperty(cx, obj, "segmentRef", &v)) {
      LOG("retryCount is null");
    }
    if (!rn.init(cx, v)) {
      LOG("Couldn't initialize retryCount from the request.");
    }
    int refNo = int(NS_LossyConvertUTF16toASCII(rn).get());
    */
    if (!JS_GetProperty(cx, obj, "segmentRef", &v)  || !v.isNumber()) {
        LOG("segmentRef in sendsms request  is null or v is not number");
    }
    int refNo = v.toNumber();
    LOG("VowifiInterfaceLayer:: RefNo = %d", refNo);

   


    //Parse SMS body Ex. { "segments":[{"body":"Test message","encodedBodyLength":22}] }
    //We need to parse {Object[Array{Object}]}
    if (!JS_GetProperty(cx, obj, "segments", &v)) {
      LOG("segments is null");
    }

    //parse array object
    JS::Rooted<JSObject*> arrayObj(cx, &v.toObject());
    bool isArray;
    if (!JS_IsArrayObject(cx, arrayObj, &isArray)) {
      LOG("IsArray failure");
    }
    if (!isArray) {
      LOG("Not an Array");
    }

    uint32_t length;
    MOZ_ALWAYS_TRUE(JS_GetArrayLength(cx, arrayObj, &length));
    NS_ENSURE_TRUE(length, NS_ERROR_INVALID_ARG);

    // int msgCount = length;
    //int msgSeq = 1;
    for (uint32_t i = 0; i < length; ) {
        JS::Rooted<JS::Value> val(cx);
        if (!JS_GetElement(cx, arrayObj, i, &val)) {
           LOG("error !JS_GetElement");
        }

        //parse body object
        JS::Rooted<JSObject*> obj1(cx, &val.toObject());
        JS::Rooted<JS::Value> va(cx);
        nsAutoJSString body;
        if (!JS_GetProperty(cx, obj1, "body", &va)) {
          LOG("body is null");
          return NS_ERROR_INVALID_ARG;
        }
        if (!body.init(cx, va)) {
          LOG("Couldn't initialize body from the request.");
          return NS_ERROR_INVALID_ARG;
        }
        msg = string(NS_LossyConvertUTF16toASCII(body).get());
  
        if (mAdapterControl != nullptr)
        {
          mozilla::AutoSafeJSContext cx;
          JSAutoCompartment ac(cx, xpc::PrivilegedJunkScope());
          JS::Rooted<JSObject*> callObj(cx);
          callObj = JS_NewPlainObject(cx);
          bool resultValue = true;
          bool *result = &resultValue;
          if (!mAdapterControl->SendVowifiSms(callback, receiver, retCount, refNo, 0, msg, length, ++i)){
            JS::Rooted<JSString*> erroString(cx, JS_NewStringCopyZ(cx, "SMS send failed"));
            JS::Rooted<JS::Value> errorMsg(cx);
            errorMsg.setString(erroString);
            JS_SetProperty(cx, callObj, "errorMsg", errorMsg);
            JS::Rooted<JS::Value> aMessage(cx, JS::ObjectValue(*callObj));
            callback->HandleResponse(aMessage, result);
            LOG("VowifiInterfaceLayer::SendSMS failed.");
          }
          else {
            JS::Rooted<JS::Value> aMessage(cx, JS::ObjectValue(*callObj));
            callback->HandleResponse(aMessage, result);
            LOG("VowifiInterfaceLayer::SendSMS Success.");
          }
        }
    }
    return NS_OK;
}


NS_IMETHODIMP VowifiInterfaceLayer::SetSmscAddress(JS::HandleValue aMessage, nsIRilSendWorkerMessageCallback *callback)
{
    string smsc; 

    if (!aMessage.isObject()) {
      MOZ_ASSERT(false, "Not a object.");
      return NS_ERROR_INVALID_ARG;
    }
  
    mozilla::AutoSafeJSContext cx;
    JS::Rooted<JSObject*> obj(cx, &aMessage.toObject());
    JS::Rooted<JS::Value> v(cx);

    //Parse SMSC Number
    nsAutoJSString number;
    if (!JS_GetProperty(cx, obj, "smscAddress", &v)) {
      LOG("SMSC number is null");
      return NS_ERROR_INVALID_ARG;
    }
    if (!number.init(cx, v)) {
      LOG("Couldn't initialize receiver number from the request.");
      return NS_ERROR_INVALID_ARG;
    }

    smsc = string(NS_LossyConvertUTF16toASCII(number).get());

    LOG("VowifiInterfaceLayer::setSmscAddress = [%s]", smsc.c_str());
    if (mAdapterControl != nullptr)
    {
      if (!mAdapterControl->SetSmscAddress(smsc)){
        LOG("VowifiInterfaceLayer::setSmscAddress failed.");
      }
      else {
        LOG("VowifiInterfaceLayer::setSmscAddress Success.");
      }
    }
    return NS_OK;

}


NS_IMETHODIMP VowifiInterfaceLayer::GetSmscAddress(JS::HandleValue aMessage, nsIRilSendWorkerMessageCallback *callback)
{

    LOG("VowifiInterfaceLayer::getSmscAddress been called");
    if(mAdapterControl != nullptr)
    {
        mozilla::AutoSafeJSContext cx;
        JSAutoCompartment ac(cx, xpc::PrivilegedJunkScope());
        JS::Rooted<JSObject*> callObj(cx);
        callObj = JS_NewPlainObject(cx);
        bool resultValue = true;
        bool *result = &resultValue;
        string smsc; 
        if(!mAdapterControl->GetSmscAddress(smsc)){
            JS::Rooted<JSString*> erroString(cx, JS_NewStringCopyZ(cx, "getSmscAddress failed"));
            JS::Rooted<JS::Value> errorMsg(cx);
            errorMsg.setString(erroString);
            JS_SetProperty(cx, callObj, "errorMsg", errorMsg);
            JS::Rooted<JS::Value> aMessage(cx, JS::ObjectValue(*callObj));
            callback->HandleResponse(aMessage, result);
        }
        else{
            JS::Rooted<JSString*> smscString(cx, JS_NewStringCopyZ(cx, smsc.c_str()));
            JS::Rooted<JS::Value> smscMsg(cx);
            smscMsg.setString(smscString);
            JS::Rooted<JS::Value> aMessage(cx, JS::ObjectValue(*callObj));
            callback->HandleResponse(aMessage, result);
        }
    }
    return NS_OK;

}

NS_IMETHODIMP VowifiInterfaceLayer::AckSms(JS::HandleValue aMessage, nsIRilSendWorkerMessageCallback *callback)
{

    LOG("VowifiInterfaceLayer::Ack SMS has been called");

    if (!aMessage.isObject()) {
      MOZ_ASSERT(false, "Not a object.");
      return NS_ERROR_INVALID_ARG;
    }
  
    mozilla::AutoSafeJSContext cx;
    JS::Rooted<JSObject*> obj(cx, &aMessage.toObject());
    JS::Rooted<JS::Value> v(cx);

    //Parse SMSC Number
    /*nsAutoJSString result;
    if (!JS_GetProperty(cx, obj, "result", &v)) {
      LOG("result is null");
    }
    if (!result.init(cx, v)) {
      LOG("Couldn't initialize result from the request.");
    } 
    int result_code = int(NS_LossyConvertUTF16toASCII(result).get());
    */

    if (!JS_GetProperty(cx, obj, "result", &v)  || !v.isNumber()) {
      LOG("result in sendsmsack request  is null or v is not number");
    }
    int result_code = v.toNumber();

    LOG("VowifiInterfaceLayer::result code = [%d]", result_code);
    if (mAdapterControl != nullptr)
    {
      if (!mAdapterControl->AckSms(result_code)){
        LOG("VowifiInterfaceLayer::Ack SMS failed.");
      }
      else {
        LOG("VowifiInterfaceLayer::Ack SMS Success.");
      }
    }

    return NS_OK;
}




/* AString getCurLocalAddress (); */
NS_IMETHODIMP VowifiInterfaceLayer::GetCurLocalAddress(nsAString & _retval) {

    LOG("VowifiInterfaceLayer::GetCurLocalAddress been called");
       if(mAdapterControl != nullptr)
       {
           string localAddr;
           mAdapterControl->GetCurLocalAddress(localAddr);
           _retval = NS_ConvertASCIItoUTF16(localAddr.c_str());
       }
    return NS_OK;
}

/* AString getCurPcscfAddress (); */
NS_IMETHODIMP VowifiInterfaceLayer::GetCurPcscfAddress(nsAString & _retval) {

    LOG("VowifiInterfaceLayer::GetCurPcscfAddress been called");
       if(mAdapterControl != nullptr)
       {
           string pcscfAddr;
           mAdapterControl->GetCurPcscfAddress(pcscfAddr);
           _retval = NS_ConvertASCIItoUTF16(pcscfAddr.c_str());
       }
    return NS_OK;
}

/* PRInt32 getCallCount (); */
NS_IMETHODIMP VowifiInterfaceLayer::GetCallCount(int16_t *_retval) {
    LOG("VowifiInterfaceLayer::GetCallCount been called");
       if(mAdapterControl != nullptr)
       {
           mAdapterControl->GetCallCount(_retval);
       }
    return NS_OK;
}

/* PRInt32 getAliveCallLose (); */
NS_IMETHODIMP VowifiInterfaceLayer::GetAliveCallLose(int16_t *_retval) {
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* PRInt32 getAliveCallJitter (); */
NS_IMETHODIMP VowifiInterfaceLayer::GetAliveCallJitter(int16_t *_retval) {
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* PRInt32 getAliveCallRtt (); */
NS_IMETHODIMP VowifiInterfaceLayer::GetAliveCallRtt(int16_t *_retval) {
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP VowifiInterfaceLayer::UpdateDataRouterState(uint32_t dataRouterState){
    LOG("VowifiInterfaceLayer::UpdateDataRouterState been called");
    if(mAdapterControl != nullptr)
    {
        mAdapterControl->UpdateDataRouterState(dataRouterState);
    }
    return NS_OK;
}

/* void setVolteUsedLocalAddr(AString addr) */
NS_IMETHODIMP VowifiInterfaceLayer::SetVolteUsedLocalAddr(const nsAString &addr)
{
    LOG("VowifiInterfaceLayer::SetVolteUsedLocalAddr has been called");
    string imsregaddr = string(
                    NS_LossyConvertUTF16toASCII(addr).get());
    if(mAdapterControl != nullptr)
    {
        mAdapterControl->SetVolteUsedLocalAddr(imsregaddr);
    }
    return NS_OK;
}


NS_GENERIC_FACTORY_SINGLETON_CONSTRUCTOR(VowifiInterfaceLayer,
        VowifiInterfaceLayer::GetInstance)

NS_DEFINE_NAMED_CID(NS_VOWIFIINTERFACELAYER_CID);

static const mozilla::Module::CIDEntry kVowifiInterfaceLayerCIDs[] = { {
        &kNS_VOWIFIINTERFACELAYER_CID, false, nullptr,
        VowifiInterfaceLayerConstructor }, { nullptr } };

static const mozilla::Module::ContractIDEntry kVowifiInterfaceLayerContracts[] =
        { { VOWIFIINTERFACELAYER_CONTRACTID, &kNS_VOWIFIINTERFACELAYER_CID }, {
                nullptr } };


static const mozilla::Module kVowifiInterfaceLayerModule = {
        mozilla::Module::kVersion, kVowifiInterfaceLayerCIDs,
        kVowifiInterfaceLayerContracts,
        nullptr };

} // namespace vowifiadapter
} // namespace dom
} // namespace mozilla

NSMODULE_DEFN (VowifiInterfaceLayerModule) =
        &mozilla::dom::vowifiadapter::kVowifiInterfaceLayerModule;
