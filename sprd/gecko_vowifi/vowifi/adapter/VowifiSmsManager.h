
/************************************************************************

        Copyright (c) 2005-2017 by Spreadtrum, Inc.
                       All rights reserved.

     This software is confidential and proprietary to Spreadtrum,
     Inc. No part of this software may be reproduced, stored, transmitted,
     disclosed or used in any form or by any means other than as expressly
     provided by the written license agreement between Spreadtrum and its
     licensee.

************************************************************************/

#ifndef mozilla_dom_vowifiadapter_VowifiSmsManager_h
#define mozilla_dom_vowifiadapter_VowifiSmsManager_h

#include "VowifiAdapterCommon.h"

namespace mozilla {
namespace dom {
namespace vowifiadapter {

class VowifiAdapterControl;
class SmsListener;

const short CALLBACK_SMS_INCOMING = 5;
const short CALLBACK_SMS_OUTGOING = 6;
const short CALLBACK_SMS_SENDOK = 7;
const short CALLBACK_SMS_DELIVEROK = 8;

const unsigned long SMS_ORIG_DEST_PORT = 4294967295;

static const std::string SMS_PARAM_MSG_PDU = "sms_msg_pdu";
static const std::string SMS_PARAM_MSG_SMSC = "sms_msg_smsc";
static const std::string SMS_PARAM_MSG_SENDER = "sms_msg_sender";
static const std::string SMS_PARAM_MSG_TIME = "sms_msg_time";
static const std::string SMS_PARAM_MSG_PID = "sms_msg_pid";
static const std::string SMS_PARAM_MSG_DCS = "sms_msg_dcs";
static const std::string SMS_PARAM_MSG_SEG_REF = "sms_msg_seg_ref";
static const std::string SMS_PARAM_MSG_SEQ = "sms_msg_seq";
static const std::string SMS_PARAM_MSG_COUNT = "sms_msg_count";



//class VowifiSmsManager : public nsIIccCallback
class VowifiSmsManager
{
public:
    //NS_DECL_ISUPPORTS
    //NS_DECL_NSIICCCALLBACK
    VowifiSmsManager(VowifiAdapterControl* adapterControl);
    VowifiSmsManager(){};
    ~VowifiSmsManager(){};
    void RegisterListeners(SmsListener* listener);
    void UnregisterListeners();
    int SendVowifiSms(nsIRilSendWorkerMessageCallback *callback, string& pdu, int retry, int messageRef, int serial, string& text, int msgMaxSeq, int msgSeq);
    void HandleEvent(int eventId, string &eventName, Json::Value &args);
    void HandleSmsIncoming(string text, string smsc, string sender, long long time, int pid, int dcs,  unsigned int msgRef,
                           unsigned int msgSeq,  unsigned int msgCount);
    void HandleSmsSendOK(string pdu);
    void HandleSmsDeliverOK(string pdu);
    bool SetSmscAddress(string smsc);
    bool GetSmscAddress(string &smsc);
    int AckSms(int result);

private:
    VowifiAdapterControl *mAdapterControl = nullptr;
    int mState;
    std::string mSmsc;
    nsIRilSendWorkerMessageCallback *mSmsDeliverCallback;
    SmsListener* mListener = nullptr;
};


enum enumSMSEventId {
     
     EVENT_SMS_SENDMSG,
     EVENT_SMS_ACKLASTINCOMING
};

static const std::string EVENT_NAME_SMS_SENDMSG = "sms_sendmsg";
static const std::string EVENT_NAME_SMS_ACKSMS = "sms_acksms";


class SmsListener {

public:
    SmsListener() {};
    virtual ~SmsListener()  {};
    virtual void OnSmsIncoming(int callIndex, uint16_t callType);

};



}
}
}
#endif
