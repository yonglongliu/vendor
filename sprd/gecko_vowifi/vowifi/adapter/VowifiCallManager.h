/************************************************************************

        Copyright (c) 2005-2017 by Spreadtrum, Inc.
                       All rights reserved.

     This software is confidential and proprietary to Spreadtrum,
     Inc. No part of this software may be reproduced, stored, transmitted,
     disclosed or used in any form or by any means other than as expressly
     provided by the written license agreement between Spreadtrum and its
     licensee.

************************************************************************/

#ifndef mozilla_dom_vowifiadapter_VowifiCallManager_h
#define mozilla_dom_vowifiadapter_VowifiCallManager_h
#include "VowifiAdapterCommon.h"
#include "nsIRadioInterfaceLayer.h"
#include "nsIGonkTelephonyService.h"
#include "nsITelephonyService.h"
#include "nsITelephonyCallInfo.h"

#include "mozilla/dom/telephony/TelephonyCommon.h"
#include "nsAutoPtr.h"
#include <string>

namespace Json {
class Value;
}

namespace mozilla {
namespace dom {
namespace vowifiadapter {
class CallListener;
class VowifiAdapterControl;

const short CALL_STATE_UNKNOWN = -1;
const short CALL_STATE_CONNECTED = 0;
const short CALL_STATE_HELD = 1;
const short CALL_STATE_DIALING = 2;
const short CALL_STATE_ALERTING = 3;
const short CALL_STATE_INCOMING = 4;
const short CALL_STATE_WAITING = 5;  //but this.CALL_STATE_WAITING = 5;in ril_const.js

const short TOA_INTERNATIONAL = 0x91;
const short TOA_UNKNOWN = 0x81;
const unsigned short NOTIFICATION_REMOTE_HELD = 0;
const unsigned short NOTIFICATION_REMOTE_RESUMED = 1;

const unsigned short CALL_PRESENTATION_ALLOWED = 0;
const unsigned short CALL_PRESENTATION_RESTRICTED = 1;
const unsigned short CALL_PRESENTATION_UNKNOWN = 2;
const unsigned short CALL_PRESENTATION_PAYPHONE = 3;

const unsigned short TTY_MODE_OFF = 0;
const unsigned short TTY_MODE_FULL = 1;
const unsigned short TTY_MODE_HCO = 2;
const unsigned short TTY_MODE_VCO = 3;

const unsigned short CALL_VOICE_QUALITY_NORMAL = 0;
const unsigned short CALL_VOICE_QUALITY_HD = 1;

const unsigned short COVERAGE_LOST_IMS_OVER_WIFI = 0;

/**
 * Voice telephony IR.92 & IR.94 (voide + video upgrade/downgrade)
 */
const unsigned short CALL_TYPE_VOICE_N_VIDEO = 1;
/**
 * IR.92 (Voice only)
 */
const unsigned short CALL_TYPE_VOICE = 2;
/**
 * Video telephony to support IR.92 and IR.94 (voice + video upgrade/downgrade)
 */
const unsigned short CALL_TYPE_VIDEO_N_VOICE = 3;
/**
 * Video telephony (audio / video two way)
 * TBD phase 2?
 */
const unsigned short CALL_TYPE_VT = 4;
/**
 * Video telephony (audio two way / video TX one way)
 * TBD phase 2?
 */
const unsigned short CALL_TYPE_VT_TX = 5;
/**
 * Video telephony (audio two way / video RX one way)
 * TBD phase 2?
 */
const unsigned short CALL_TYPE_VT_RX = 6;
/**
 * Video telephony (audio two way / video inactive)
 * TBD phase 2?
 */
const unsigned short CALL_TYPE_VT_NODIR = 7;
/**
 * VideoShare (video two way)
 * TBD phase 2?
 */
const unsigned short CALL_TYPE_VS = 8;
/**
 * VideoShare (video TX one way)
 * TBD phase 2?
 */
const unsigned short CALL_TYPE_VS_TX = 9;
/**
 * VideoShare (video RX one way)
 * TBD phase 2?
 */
const unsigned short CALL_TYPE_VS_RX = 10;

/**
 * It goes through CS, ex: GSM, UMTS, CDMA
 */
const unsigned long RADIO_TECH_CS = 0;

/**
 * It goes through PS, ex: VoLTE
 */
const unsigned long RADIO_TECH_PS = 1;

/**
 * It goes though Wi-Fi.
 */
const unsigned long RADIO_TECH_WIFI = 2;

const string CONF_STATUS_DISCONNECTED = "disconnected";

// Bug 941693, to control execute action's order
typedef enum enumActionType
{
    ENUM_ACTION_UNKNOWN = 0,
    ENUM_ACTION_ACCEPT,
    ENUM_ACTION_REJECT,
    ENUM_ACTION_SWITCH,
    ENUM_ACTION_HOLD,
    ENUM_ACTION_UNHOLD,
    ENUM_ACTION_MUTE,
    ENUM_ACTION_UNMUTE,
    ENUM_ACTION_MERGE,
    ENUM_ACTION_TERM
}ActionType_T;

class PendingAction
{
public:
    ActionType_T _mOrigActionT;  // original parent action trigger

    bool _mPrevIsConf;           // executed action for a conference
    int  _mPrevId;               // executed call/conference id
    ActionType_T _mPrevActionT;  // executed action type

    bool _mPendIsConf;           // pending action for a conference
    int  _mPendIndex;            // pending call/conference index
    ActionType_T _mPendActionT;  // pending action type

    PendingAction(ActionType_T origType, bool prevConf, bool pendConf,
                  ActionType_T prevType, ActionType_T pendType,
                  int prevId = -1, int pendIndex = -1);
};

class VowifiCallManager {

    typedef enum enumActionProcResult
    {
        ACTION_PROC_FAILED = 0,
        ACTION_PROC_CONTINUE,
        ACTION_PROC_FINISH
    }ActionProcResult_T;

public:
    VowifiCallManager(VowifiAdapterControl *adapterControl);
    void RegisterListeners(CallListener* listener);
    void UnregisterListeners();
    bool IsCallFunEnabled();
    void StopAudioStream();
    void RemoveCall();
    void StartAudioStream();
    void SetVideoQuality(int quality);
    int GetCallId(uint32_t callIndex);
    void SetCallId(uint32_t callIndex, uint32_t id, string &number);
    uint32_t GetCallIndex(uint32_t id);
    string GetCallNumber(uint32_t id);
    bool  IsCallAlive(int callId);
    void StrToInt(uint32_t &int_temp, string &string_temp);
    bool Start(string& number, bool isEmergency);
    bool Accept();
    bool Reject();
    void TerminateCalls(int wifistate);
    bool Terminate(int CallIndex);
    bool Switch();
    bool ConfHold();
    bool ConfResume();
    bool TerminateConfCall();
    bool HangUpForeground();
    bool HangUpBackground();
    bool Mute(bool isMute);
    bool Hold(int callIndex);
    bool Resume(int callIndex);
    bool ConferenceCall(bool isAllHold);
    bool Merge(int callSessNum);
    bool CreateConfcall();
    void SetupConfcall(int confId);
    void InviteConfCallMember();
    void UpdateDataRouterState(int dataRouterState);
    void HandleEvent(int eventId, string &eventName, Json::Value &args);
    void HandleCallOutgoing(int sessionId, bool isConf,
            bool isVideo, string &callee);
    void HandleCallIncoming(int sessionId, bool isConference, bool isVideo,
            string &callee);
    void HandleCallProcessing(int sessionId, string &callee, bool isVideo, int callState);
    void HandleCallTermed(int sessionId, int stateCode, string &callee);
    void HandleCallHoldOrResume(int sessionId, int eventCode, string &callee);
    void HandleRTPReceived(int sessionId, bool isVideo, bool isReceived);
    void HandleRTCPChanged(int lose, int jitter, int rtt, bool isVideo);
    void HandleConfConnected();
    void HandleMergeFailed();
    void HandleConfParticipantsChanged(int eventId, string &phoneNum, string &sipUri, string &newStatus);
    void HandleConfTermed();
    void UpdateRegisterState(int registerState);
    int GetCallCount();

private:
    nsTArray<CallInfo> mCallInfo;
    ConfCallInfo mConfCallInfo;
    vector<PendingAction*> mPendActions;

    int mIsVideoCall;
    VowifiAdapterControl* mAdapterControl = nullptr;
    int mRegisterState;
    bool mIsAlive;
    string mMoNumber;
    bool mAudioStart;
    //	nsCOMPtr<nsITelephonyService> mTelephonyService;
    CallListener* mListener;
    uint32_t mUseAudioStreamCount;
    uint32_t mIncomingCallAction;
    uint32_t mOutgoingCallId = 0;
    uint32_t mIncomingCallId = 0;

    bool PendingActionPush(ActionType_T origType,
                           bool isPrevConf,
                           int iPrevId,
                           ActionType_T prevType,
                           bool isPendConf,
                           int iPendIndx,
                           ActionType_T pendType
                           );
    bool PendingActionPush(ActionType_T origType,
                           bool isPrevConf,
                           int iPrevId,
                           ActionType_T prevType,
                           bool isPendConf,
                           ActionType_T pendType
                           ); 
    ActionProcResult_T PendingActionProcess(int id, int evntCode);
    bool PendingActionClear();
};

class CallListener {

public:
    CallListener() {
    }
    ;
    virtual void OnCallIncoming(int callIndex, uint16_t callType);
    virtual void OnCallIsEmergency();
    virtual void OnCallEnd();
    virtual void OnHoldResumeOK();
    virtual void OnHoldResumeFailed();
    virtual void OnCallRTPReceived(bool isVideoCall, bool isReceived);
    virtual void OnCallRTCPChanged(bool isVideoCall, int lose, int jitter,
            int rtt);
    virtual void OnAliveCallUpdate(bool isVideoCall);
    virtual void OnEnterECBM();
    virtual void OnExitECBM();
    virtual void OnCallStateChanged(uint32_t alength);
    virtual void OnNotifyError(string error);
    virtual ~CallListener() {
    }
    ;

};


// call eventId and constants

enum enumCallEventId {
    EVENT_CALL_STARTAUDIO=0,
    EVENT_CALL_STOPAUDIO,
    EVENT_CALL_SESSCALL,
    EVENT_CALL_SESSGEOLOC,
    EVENT_CALL_SETMICMUTE,
    EVENT_CALL_SESSTERM,
    EVENT_CALL_SESSHOLD,
    EVENT_CALL_SESSRESUME,
    EVENT_CALL_SESSDTMF,
    EVENT_CALL_SESSANSWER,
    EVENT_CALL_SESSUPDATE,
    EVENT_CALL_SESSUPDATERESP,
    EVENT_CALL_SESSRELEASE,
    EVENT_CALL_SESSUPDATESRVCC,
    EVENT_CALL_CONFCALL,
    EVENT_CALL_CONFINIT,
    EVENT_CALL_CONFSETUP,
    EVENT_CALL_CONFHOLD,
    EVENT_CALL_CONFRESUME,
    EVENT_CALL_CONFADDMEMBERS,
    EVENT_CALL_CONFACCEPTINVITE,
    EVENT_CALL_CONFTERM,
    EVENT_CALL_CONFKICKMEMBERS,
    EVENT_CALL_CONFSETMUTE,
    EVENT_CALL_CONFLOCALIMGTRAN,
    EVENT_CALL_CONFRELEASE,
    EVENT_CALL_CONFUPDATESRVCC,
    EVENT_CALL_SESSSETCAMERACAP,
    EVENT_CALL_SESSGETCAMERACAP,
    EVENT_CALL_CAMERAATTACH,
    EVENT_CALL_CAMERADETACH,
    EVENT_CALL_VIDEOSTART,
    EVENT_CALL_VIDEOSTOP,
    EVENT_CALL_CAPTURESTART,
    EVENT_CALL_CAPTURESTOP,
    EVENT_CALL_CAPTURESTOPALL,
    EVENT_CALL_REMOTERENDERROTATE,
    EVENT_CALL_LOCALRENDERROTATE,
    EVENT_CALL_REMOTERENDERADD,
    EVENT_CALL_LOCALRENDERADD,
    EVENT_CALL_REMOTERENDERRMV,
    EVENT_CALL_LOCALRENDERRMV,
    EVENT_CALL_SETDEFVIDEOLEV,
    EVENT_CALL_GETDEFVIDEOLEV,
    EVENT_CALL_SESSMODIFYREQ,
    EVENT_CALL_SESSMODIFYRESP,
    EVENT_CALL_GETMEDIALOSTRATIO,
    EVENT_CALL_GETMEDIAJITTER,
    EVENT_CALL_GETMEDIARTT,
    EVENT_CALL_SENDUSSD,
    EVENT_CALL_QUERYCALLBARRING,
    EVENT_CALL_QUERYCALLFORWARD,
    EVENT_CALL_QUERYCALLWAIT,
    EVENT_CALL_UPDCALLBARRING,
    EVENT_CALL_UPDCALLFORWARD,
    EVENT_CALL_UPDCALLWAIT,
    EVENT_CALL_UPDCALLCLIR,
    EVENT_CALL_UPDCALLSTATEROUTER,
    EVENT_CALL_FAILED
};

enum enumMtcCallCbId
{
    CALLBACK_INCOMING = 100,
    CALLBACK_OUTGOING,
    CALLBACK_ALERTED,
    CALLBACK_PRACKED,
    CALLBACK_TALKING,
    CALLBACK_TERMED,
    CALLBACK_MDFY_ACPT,
    CALLBACK_MDFYED,
    CALLBACK_MDFY_REQ,
    CALLBACK_HOLD_OK,
    CALLBACK_HOLD_FAILED,
    CALLBACK_UNHOLD_OK,
    CALLBACK_UNHOLD_FAILED,
    CALLBACK_HELD,
    CALLBACK_UNHELD,
    CALLBACK_ADD_AUDIO_OK,
    CALLBACK_ADD_AUDIO_FAILED,
    CALLBACK_RMV_AUDIO_OK,
    CALLBACK_RMV_AUDIO_FAILED,
    CALLBACK_ADD_AUDIO_REQ,
    CALLBACK_ADD_VIDEO_OK,
    CALLBACK_ADD_VIDEO_FAILED,
    CALLBACK_RMV_VIDEO_OK,
    CALLBACK_RMV_VIDEO_FAILED,
    CALLBACK_ADD_VIDEO_REQ,
    CALLBACK_REFERED,
    CALLBACK_TRSF_ACPT,
    CALLBACK_TRSF_TERM,
    CALLBACK_TRSF_FAILED,
    CALLBACK_REDIRECT,
    CALLBACK_INFO,
    CALLBACK_REPLACED,
    CALLBACK_REPLACEOK,
    CALLBACK_REPLACEFAILED,
    CALLBACK_RTP_CONNECTIVITY,
    CALLBACK_CAM_DISCONNED,
    CALLBACK_VIDEO_SIZE,
    CALLBACK_NET_STA_CHANGED,
    CALLBACK_VIDEO_INCOMING_STA,
    CALLBACK_VIDEO_OUTGOING_STA,
    CALLBACK_VIDEO_PROTECT_STA,
    CALLBACK_RTCP_RR,
    CALLBACK_CAPTURE_FRAMERATE,
    CALLBACK_CAPTURE_SIZE,
    CALLBACK_PARTP_UPDATED,
    CALLBACK_SET_ERROR,
    CALLBACK_ADD_AUDIO_CANCEL,
    CALLBACK_ADD_VIDEO_CANCEL,
    CALLBACK_RING_ONCE,
    CALLBACK_START_RING,
    CALLBACK_STOP_RING,

    CALLBACK_SUPSRVALLLOADOK,
    CALLBACK_SUPSRVALLLOADFAILED,
    CALLBACK_SUPSRVALLUPLOADOK,
    CALLBACK_SUPSRVALLUPLOADFAILED,
    CALLBACK_SUPSRVOIPLOADOK,
    CALLBACK_SUPSRVOIPLOADFAILED,
    CALLBACK_SUPSRVOIPUPLOADOK,
    CALLBACK_SUPSRVOIPUPLOADFAILED,
    CALLBACK_SUPSRVOIRLOADOK,
    CALLBACK_SUPSRVOIRLOADFAILED,
    CALLBACK_SUPSRVOIRUPLOADOK,
    CALLBACK_SUPSRVOIRUPLOADFAILED,
    CALLBACK_SUPSRVTIPLOADOK,
    CALLBACK_SUPSRVTIPLOADFAILED,
    CALLBACK_SUPSRVTIPUPLOADOK,
    CALLBACK_SUPSRVTIPUPLOADFAILED,
    CALLBACK_SUPSRVTIRLOADOK,
    CALLBACK_SUPSRVTIRLOADFAILED,
    CALLBACK_SUPSRVTIRUPLOADOK,
    CALLBACK_SUPSRVTIRUPLOADFAILED,
    CALLBACK_SUPSRVCDIVLOADOK,
    CALLBACK_SUPSRVCDIVLOADFAILED,
    CALLBACK_SUPSRVCDIVUPLOADOK,
    CALLBACK_SUPSRVCDIVUPLOADFAILED,
    CALLBACK_SUPSRVCDIVCAPLOADOK,
    CALLBACK_SUPSRVCDIVCAPLOADFAILED,
    CALLBACK_SUPSRVICBLOADOK,
    CALLBACK_SUPSRVICBLOADFAILED,
    CALLBACK_SUPSRVICBUPLOADOK,
    CALLBACK_SUPSRVICBUPLOADFAILED,
    CALLBACK_SUPSRVOCBLOADOK,
    CALLBACK_SUPSRVOCBLOADFAILED,
    CALLBACK_SUPSRVOCBUPLOADOK,
    CALLBACK_SUPSRVOCBUPLOADFAILED,
    CALLBACK_SUPSRVCBCAPLOADOK,
    CALLBACK_SUPSRVCBCAPLOADFAILED,
    CALLBACK_SUPSRVCWLOADOK,
    CALLBACK_SUPSRVCWLOADFAILED,
    CALLBACK_SUPSRVCWUPLOADOK,
    CALLBACK_SUPSRVCWUPLOADFAILED,

    CALLBACK_CDIVSUBSOK,
    CALLBACK_CDIVSUBSFAILED,
    CALLBACK_CDIVUNSUBSOK,
    CALLBACK_CDIVUNSUBSFAILED,
    CALLBACK_CDIVRECVNTFY,

    CALLBACK_EMCALLINDICATION
};


enum enumMtcConfCbId
{
    CALLBACK_CONFCB_INCOMING = 200,
    CALLBACK_CONFCB_OUTGOING,
    CALLBACK_CONFCB_ALERTED,
    CALLBACK_CONFCB_CONNED,
    CALLBACK_CONFCB_DISCED,
    CALLBACK_CONFCB_IVT_ACPT,
    CALLBACK_CONFCB_IVT_FAIL,
    CALLBACK_CONFCB_KICK_ACPT,
    CALLBACK_CONFCB_KICK_FAIL,
    CALLBACK_CONFCB_PTPT_UPDT,
    CALLBACK_CONFCB_MDFY_ACPT,
    CALLBACK_CONFCB_MDFYED,
    CALLBACK_CONFCB_HOLD_OK,
    CALLBACK_CONFCB_HOLD_FAILED,
    CALLBACK_CONFCB_UNHOLD_OK,
    CALLBACK_CONFCB_UNHOLD_FAILED,
    CALLBACK_CONFCB_HELD,
    CALLBACK_CONFCB_UNHELD,
    CALLBACK_CONFCB_NET_STA_CHANGED,
    CALLBACK_CONFCB_RTP_CONNECTIVITY,
    CALLBACK_CONFCB_RTCP_RR,
    CALLBACK_CONFCB_ERROR
};


// Call & Conference
static const std::string KEY_ID = "id";
static const std::string KEY_STATE_CODE = "state_code";
static const std::string KEY_ALERT_TYPE = "alert_type";
static const std::string KEY_IS_VIDEO = "is_video";
static const std::string KEY_PHONE_NUM = "phone_num";
static const std::string KEY_SIP_URI = "sip_uri";
static const std::string KEY_VIDEO_HEIGHT = "video_height";
static const std::string KEY_VIDEO_WIDTH = "video_width";
static const std::string KEY_VIDEO_ORIENTATION = "video_orientation";
static const std::string KEY_VIDEO_LEVEL = "video_level";
static const std::string KEY_RTP_RECEIVED = "rtp_received";
static const std::string KEY_RTCP_LOSE = "rtcp_lose";
static const std::string KEY_RTCP_JITTER = "rtcp_jitter";
static const std::string KEY_RTCP_RTT = "rtcp_rtt";
static const std::string KEY_CONF_PART_NEW_STATUS = "conf_part_new_status";
static const std::string KEY_EMERG_IND_URN_URI = "emergency_call_ind_urn_uri";
static const std::string KEY_EMERG_IND_ACTION_TYPE = "emergency_call_ind_action_type";
static const std::string KEY_EMERG_IND_REASON = "emergency_call_ind_reason";
static const std::string KEY_USSD_INFO_RECEIVED = "ussd_info_received";
static const std::string KEY_USSD_MODE = "ussd_mode";
static const std::string KEY_CALL_FAILED_FUNC = "failed_function";

// Call
static const int CALL_EVENT_CODE_BASE = 100;
static const int EVENT_CODE_CALL_INCOMING = CALL_EVENT_CODE_BASE + 1;
static const int EVENT_CODE_CALL_OUTGOING = CALL_EVENT_CODE_BASE + 2;
static const int EVENT_CODE_CALL_ALERTED = CALL_EVENT_CODE_BASE + 3;
static const int EVENT_CODE_CALL_TALKING = CALL_EVENT_CODE_BASE + 4;
static const int EVENT_CODE_CALL_TERMINATE = CALL_EVENT_CODE_BASE + 5;
static const int EVENT_CODE_CALL_HOLD_OK = CALL_EVENT_CODE_BASE + 6;
static const int EVENT_CODE_CALL_HOLD_FAILED = CALL_EVENT_CODE_BASE + 7;
static const int EVENT_CODE_CALL_RESUME_OK = CALL_EVENT_CODE_BASE + 8;
static const int EVENT_CODE_CALL_RESUME_FAILED = CALL_EVENT_CODE_BASE + 9;
static const int EVENT_CODE_CALL_HOLD_RECEIVED = CALL_EVENT_CODE_BASE + 10;
static const int EVENT_CODE_CALL_RESUME_RECEIVED = CALL_EVENT_CODE_BASE + 11;
static const int EVENT_CODE_CALL_ADD_VIDEO_OK = CALL_EVENT_CODE_BASE + 12;
static const int EVENT_CODE_CALL_ADD_VIDEO_FAILED = CALL_EVENT_CODE_BASE + 13;
static const int EVENT_CODE_CALL_REMOVE_VIDEO_OK = CALL_EVENT_CODE_BASE + 14;
static const int EVENT_CODE_CALL_REMOVE_VIDEO_FAILED = CALL_EVENT_CODE_BASE + 15;
static const int EVENT_CODE_CALL_ADD_VIDEO_REQUEST = CALL_EVENT_CODE_BASE + 16;
static const int EVENT_CODE_CALL_ADD_VIDEO_CANCEL = CALL_EVENT_CODE_BASE + 17;
static const int EVENT_CODE_CALL_RTP_RECEIVED = CALL_EVENT_CODE_BASE + 18;
static const int EVENT_CODE_CALL_RTCP_CHANGED = CALL_EVENT_CODE_BASE + 19;
static const int EVENT_CODE_CALL_IS_FOCUS = CALL_EVENT_CODE_BASE + 20;
static const int EVENT_CODE_CALL_IS_EMERGENCY = CALL_EVENT_CODE_BASE + 21;
static const int EVENT_CODE_USSD_INFO_RECEIVED = CALL_EVENT_CODE_BASE + 22;

static const std::string EVENT_NAME_STARTAUDIO = "startAudio";
static const std::string EVENT_NAME_STOPAUDIO = "stopAudio";
static const std::string EVENT_NAME_SESSCALL = "sessCall";
static const std::string EVENT_NAME_SETMICMUTE = "setMicMute";
static const std::string EVENT_NAME_SESSTERM = "sessTerm";
static const std::string EVENT_NAME_SESSHOLD = "sessHold";
static const std::string EVENT_NAME_SESSRESUME = "sessResume";
static const std::string EVENT_NAME_SESSDTMF = "sessDtmf";
static const std::string EVENT_NAME_SESSANSWER = "sessAnswer";
static const std::string EVENT_NAME_SESSUPDATE = "sessUpdate";
static const std::string EVENT_NAME_SESSUPDATERESP = "sessUpdateResp";
static const std::string EVENT_NAME_CONFCALL = "confCall";
static const std::string EVENT_NAME_CONFINIT = "confInit";
static const std::string EVENT_NAME_CONFSETUP = "confSetup";
static const std::string EVENT_NAME_CONFHOLD = "confHold";
static const std::string EVENT_NAME_CONFRESUME = "confResume";
static const std::string EVENT_NAME_CONFADDMEMBERS = "confAddMembers";
static const std::string EVENT_NAME_CONFACCEPTINVITE = "confAcceptInvite";
static const std::string EVENT_NAME_CONFTERM = "confTerm";
static const std::string EVENT_NAME_CONFKICKMEMBERS = "confKickMember";
static const std::string EVENT_NAME_CONFSETMUTE = "confSetMute";
static const std::string EVENT_NAME_CONFLOCALIMGTRAN = "confLocalImgtran";
static const std::string EVENT_NAME_SESSSETCAMERACAP = "sessSetCameraCap";
static const std::string EVENT_NAME_SESSGETCAMERACAP = "sessGetCameraCap";
static const std::string EVENT_NAME_CAMERAATTACH = "cameraAttach";
static const std::string EVENT_NAME_CAMERADETACH = "cameraDettach";
static const std::string EVENT_NAME_VIDEOSTART = "videoStart";
static const std::string EVENT_NAME_VIDEOSTOP = "videoStop";
static const std::string EVENT_NAME_CAPTURESTART = "captureStart";
static const std::string EVENT_NAME_CAPTURESTOP = "captureStop";
static const std::string EVENT_NAME_CAPTURESTOPALL = "captureStopAll";
static const std::string EVENT_NAME_REMOTERENDERROTATE = "remoteRenderRotate";
static const std::string EVENT_NAME_LOCALRENDERROTATE = "localRenderRotate";
static const std::string EVENT_NAME_REMOTERENDERADD = "remoutRenderAdd";
static const std::string EVENT_NAME_LOCALRENDERADD = "loacalRenderAdd";
static const std::string EVENT_NAME_REMOTERENDERRMV = "remoteRenderMv";
static const std::string EVENT_NAME_LOCALRENDERRMV = "loacalRenderMv";
static const std::string EVENT_NAME_SETVIDEOQUAL = "setVideoQual";
static const std::string EVENT_NAME_GETVIDEOQUAL = "getVideoQual";
static const std::string EVENT_NAME_SESSMODIFYREQ = "sessModifyReq";
static const std::string EVENT_NAME_SESSMODIFYRESP = "sessModifyResp";
static const std::string EVENT_NAME_GETMEDIALOSTRATIO = "getMediaLostRatio";
static const std::string EVENT_NAME_GETMEDIAJITTER = "getMeidaJitter";
static const std::string EVENT_NAME_GETMEDIARTT = "getMeidaAtt";
static const std::string EVENT_NAME_SENDUSSD = "sendUSSD";
static const std::string EVENT_NAME_QUERYCALLBARRING = "queryCallBarring";
static const std::string EVENT_NAME_QUERYCALLFORWARD = "queryCallForward";
static const std::string EVENT_NAME_QUERYCALLWAIT = "queryCallWait";
static const std::string EVENT_NAME_UPDCALLBARRING = "updateCallBarring";
static const std::string EVENT_NAME_UPDCALLFORWARD = "updateCallForward";
static const std::string EVENT_NAME_UPDCALLWAIT = "updateCallWating";
static const std::string EVENT_NAME_UPDCALLSTATEROUTER = "updateCallStateRouter";
static const std::string EVENT_NAME_INCOMING = "callIcoming";
static const std::string EVENT_NAME_ID = "notifyCallId";

static const std::string EVENT_CALL_INCOMING = "call_incoming";
static const std::string EVENT_CALL_OUTGOING = "call_outgoing";
static const std::string EVENT_CALL_ALERTED = "call_alerted";
static const std::string EVENT_CALL_TALKING = "call_talking";
static const std::string EVENT_CALL_TERMINATE = "call_terminate";
static const std::string EVENT_CALL_HOLD_OK = "call_hold_ok";
static const std::string EVENT_CALL_HOLD_FAILED = "call_hold_failed";
static const std::string EVENT_CALL_RESUME_OK = "call_resume_ok";
static const std::string EVENT_CALL_RESUME_FAILED = "call_resume_failed";
static const std::string EVENT_CALL_HOLD_RECEIVED = "call_hold_received";
static const std::string EVENT_CALL_RESUME_RECEIVED = "call_resume_received";
static const std::string EVENT_CALL_UPD_VIDEO_OK = "call_upd_video_ok";
static const std::string EVENT_CALL_UPD_VIDEO_FAILED = "call_upd_video_failed";
static const std::string EVENT_CALL_ADD_VIDEO_REQUEST = "call_add_video_request";
static const std::string EVENT_CALL_ADD_VIDEO_CANCEL = "call_add_video_cancel";
static const std::string EVENT_CALL_RTP_RECEIVED = "call_rtp_received";
static const std::string EVENT_CALL_RTCP_CHANGED = "call_rtcp_changed";
static const std::string EVENT_CALL_IS_FOCUS = "call_is_focus";
static const std::string EVENT_CALL_IS_EMERGENCY = "call_is_emergency";
static const std::string EVENT_USSD_INFO_RECEIVED = "ussd_info_received";
} // namespace vowifiadapter
} // namespace dom
} // namespace mozilla

#endif // mozilla_dom_vowifiadapter_VowifiCallManager_h
