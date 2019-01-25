
/************************************************************************

        Copyright (c) 2005-2017 by Spreadtrum, Inc.
                       All rights reserved.

     This software is confidential and proprietary to Spreadtrum,
     Inc. No part of this software may be reproduced, stored, transmitted,
     disclosed or used in any form or by any means other than as expressly
     provided by the written license agreement between Spreadtrum and its
     licensee.

************************************************************************/

#include "VowifiSmsManager.h"
#include "socket/VowifiArguments.h"
#include "VowifiAdapterControl.h"
#include "json/json.h"
#include "nsServiceManagerUtils.h"
#include "nsINetworkInterface.h"
#include "nsINetworkManager.h"
#include "mozilla/Services.h"
#include "nsJSON.h"
#include "nsJSUtils.h"
#include "nsINetworkService.h"
#include "nsIGonkSmsService.h"

using namespace std;

namespace mozilla {
namespace dom {
namespace vowifiadapter {

VowifiSmsManager::VowifiSmsManager(VowifiAdapterControl *adapterControl):
mAdapterControl(adapterControl), mState(-1) 
{

  LOG("Entering VowifiSmsManager initialization");
  mSmsc = "+999999999";

}


void VowifiSmsManager::RegisterListeners(SmsListener* listener) {
    LOG("Register SmsListener");
    if (listener == nullptr) {
        LOG("Can not register the callback as it is null.");
        return;
    }
    mListener = listener;
}

void VowifiSmsManager::UnregisterListeners() {
    LOG("UnRegister the listener");
    mListener = nullptr;
}


int VowifiSmsManager::SendVowifiSms(nsIRilSendWorkerMessageCallback *callback, string& receiver, int retry, int messageRef, int serial, string& text, int msgMaxSeq, int msgSeq)
{
  Json::Value root, args;
  VowifiArguments argHelp;

  mSmsDeliverCallback = callback; 

  root[JSON_GROUP_ID] = EVENT_GROUP_SMS;
  root[JSON_EVENT_ID] = EVENT_SMS_SENDMSG;
  root[JSON_EVENT_NAME] = EVENT_NAME_SMS_SENDMSG;
  argHelp.PushJsonArg(mSmsc);
  argHelp.PushJsonArg(receiver);
  argHelp.PushJsonArg(retry);
  argHelp.PushJsonArg(messageRef);
  argHelp.PushJsonArg(serial);
  argHelp.PushJsonArg(text);
  argHelp.PushJsonArg(msgMaxSeq);
  argHelp.PushJsonArg(msgSeq);

  argHelp.GetJsonArgs(args);
  args.toStyledString();
  root[JSON_ARGS] = args;
  string noticeStr = root.toStyledString();
  if (!mAdapterControl->SendToService(noticeStr)) {
    LOG("VowifiSmsManager: native call SendVowifiSms failed");
    return false;
  }
  return true;
}

bool VowifiSmsManager::SetSmscAddress(string smsc)
{
  LOG("VowifiSmsManager: setting smsc number.");
  mSmsc = smsc;

  return true;
}


bool VowifiSmsManager::GetSmscAddress(string &smsc)
{
  smsc = mSmsc;

  return true;
}


int VowifiSmsManager::AckSms(int result)
{
  Json::Value root, args;
  VowifiArguments argHelp;
  int success = result ? 1 : 0;
  root[JSON_GROUP_ID] = EVENT_GROUP_SMS;
  root[JSON_EVENT_ID] = EVENT_SMS_ACKLASTINCOMING;
  root[JSON_EVENT_NAME] = EVENT_NAME_SMS_ACKSMS;
  argHelp.PushJsonArg(success);
  argHelp.PushJsonArg(result);

  argHelp.GetJsonArgs(args);
  args.toStyledString();
  root[JSON_ARGS] = args;
  string noticeStr = root.toStyledString();
  if (!mAdapterControl->SendToService(noticeStr)) {
    LOG("VowifiSmsManager: native call AckSms failed");
    return false;
  }
  return true;
}

void VowifiSmsManager::HandleEvent(int eventId, string &eventName, Json::Value &args) 
{
  bool bArgs = true;
  bool bAg = false;
  switch (eventId) {
    case CALLBACK_SMS_OUTGOING: {
    }
      break;
    case CALLBACK_SMS_SENDOK: {
      LOG("VowifiSmsManager: Receive callback SMS Send OK");
      string pdu;
      bAg = VowifiArguments::GetOneArg(args, SMS_PARAM_MSG_PDU, pdu);
      HandleSmsSendOK(pdu);
    }
      break;
    case CALLBACK_SMS_DELIVEROK: {
      LOG("VowifiSmsManager: Receive callback SMS Deliver OK");
      string pdu;
      bAg = VowifiArguments::GetOneArg(args, SMS_PARAM_MSG_PDU, pdu);
      HandleSmsDeliverOK(pdu);
    }
      break;
    case CALLBACK_SMS_INCOMING: {
      LOG("VowifiSmsManager: Receive callback SMS incoming");
      string text;
      string smsc;
      string sender;
      string time;
      long long final_time;
      int pid, dcs;
      unsigned int msgRef, msgSeq, msgCount;
 
      bAg = VowifiArguments::GetOneArg(args, SMS_PARAM_MSG_PDU, text);
      bAg &= VowifiArguments::GetOneArg(args, SMS_PARAM_MSG_SMSC, smsc);
      bAg &= VowifiArguments::GetOneArg(args, SMS_PARAM_MSG_SENDER, sender);
      bAg &= VowifiArguments::GetOneArg(args, SMS_PARAM_MSG_TIME, time);
      bAg &= VowifiArguments::GetOneArg(args, SMS_PARAM_MSG_PID, pid);
      bAg &= VowifiArguments::GetOneArg(args, SMS_PARAM_MSG_DCS, dcs);
      bAg &= VowifiArguments::GetOneArg(args, SMS_PARAM_MSG_SEG_REF, msgRef);
      bAg &= VowifiArguments::GetOneArg(args, SMS_PARAM_MSG_SEQ, msgSeq);
      bAg &= VowifiArguments::GetOneArg(args, SMS_PARAM_MSG_COUNT, msgCount);
      if (!bAg) {
        bArgs = false;
      }
 
      std::istringstream is(time);
      is >> final_time; 
              
      HandleSmsIncoming(text, smsc, sender, final_time, pid, dcs, msgRef, msgSeq, msgCount);
      break;
    }
    default: {
      LOG("VowifiSmsManager: EventId=%d is not supported, skip it!", eventId);
      break;
    }
  }
  if (!bArgs) {
     LOG("SmsServiceCallback arguments list of eventId=%d is incorrected, skip it!", eventId);
  }

}

void VowifiSmsManager::HandleSmsDeliverOK(string pdu)
{
  LOG("VowifiSmsManager: Handle SMS Deliver OK");
  if (mSmsDeliverCallback != NULL) {
    mozilla::AutoSafeJSContext cx;
    JSAutoCompartment ac(cx, xpc::PrivilegedJunkScope());
    JS::Rooted<JSObject*> callObj(cx);
    callObj = JS_NewPlainObject(cx);
    bool resultValue = true;
    bool *result = &resultValue;

    JS::Rooted<JSString*> statusString(cx, JS_NewStringCopyZ(cx, "Success"));
    JS::Rooted<JS::Value> statusMsg(cx);
    statusMsg.setString(statusString);
    JS::Rooted<JS::Value> aMessage(cx, JS::ObjectValue(*callObj));
    mSmsDeliverCallback->HandleResponse(aMessage, result);
  }
}


void VowifiSmsManager::HandleSmsSendOK(string pdu)
{
  LOG("VowifiSmsManager: Handle SMS Send OK");

}

void VowifiSmsManager::HandleSmsIncoming(string text, string smsc, string sender, long long time, int pid, int dcs,  unsigned int msgRef, 
                                         unsigned int msgSeq,  unsigned int msgCount)
{
  nsCOMPtr<nsIGonkSmsService> smsService = do_GetService(SMS_SERVICE_CONTRACTID);
  if (!smsService) {
     LOG("Could not acquire nsIGonkSmsService!");
  }

  if (smsService){
  string lang("");
  uint8_t ser = 5;

  smsService->NotifyMessageReceived(0,                                        // unsigned long aServiceId,
                                    NS_ConvertASCIItoUTF16(smsc.c_str()),     // DOMString aSMSC,
                                    time,                                     // DOMTimeStamp aSentTimestamp,
                                    NS_ConvertASCIItoUTF16(sender.c_str()),   // DOMString aSender,
                                    pid,                                      // unsigned short aPid,
                                    dcs,                                      // unsigned short aEncoding,
                                    0,                                        // unsigned long aMessageClass,           
                                    NS_ConvertASCIItoUTF16(lang.c_str()),     // DOMString aLanguage,
                                    msgRef,                                   // unsigned short aSegmentRef,
                                    msgSeq,                                   // unsigned short aSegmentSeq,
                                    msgCount,                                 // unsigned short aSegmentMaxSeq,
                                    SMS_ORIG_DEST_PORT,                       // unsigned long aOriginatorPort,
                                    SMS_ORIG_DEST_PORT,                       // unsigned long aDestinationPort,
                                    false,                                    // boolean aMwiPresent,
                                    false,                                    // boolean aMwiDiscard,
                                    false,                                    // short aMwiMsgCount,
                                    0,                                        // boolean aMwiActive,
                                    0,                                        // unsigned short aCdmaMessageType,
                                    0,                                        // unsigned long aCdmaTeleservice,
                                    0,                                        // unsigned long aCdmaServiceCategory,
                                    NS_ConvertASCIItoUTF16(text.c_str()),     // DOMString aBody,
                                    &ser,                                     // [array, size_is(aDataLength)] in octet aData,
                                    0);                                       // uint32_t aDataLength       
  }
}


}
}
}
