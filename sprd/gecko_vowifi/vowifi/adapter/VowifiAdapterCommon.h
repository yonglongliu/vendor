/************************************************************************

        Copyright (c) 2005-2017 by Spreadtrum, Inc.
                       All rights reserved.

     This software is confidential and proprietary to Spreadtrum,
     Inc. No part of this software may be reproduced, stored, transmitted,
     disclosed or used in any form or by any means other than as expressly
     provided by the written license agreement between Spreadtrum and its
     licensee.

************************************************************************/

#ifndef mozilla_dom_vowifiadapter_VowifiAdapterConstant_h
#define mozilla_dom_vowifiadapter_VowifiAdapterConstant_h
#include <android/log.h>
#include <string>
#include <string.h>
#include <regex>

#include "prlog.h"
#include "mozilla/Logging.h"

#include "nsIIccService.h"  // ICC service

#include "nsIMobileConnectionService.h"   //for imei

#include "nsServiceManagerUtils.h"

#include "nsIMobileConnectionService.h"
#include "nsIMobileDeviceIdentities.h"
#include "nsIMobileNetworkInfo.h"
#include "nsIMobileConnectionInfo.h"

#ifndef nsCOMPtr_h___
#include "nsCOMPtr.h"
#endif
#include "mozilla/StaticPtr.h"
#include "nsString.h"

#include "nsIIccInfo.h"

using namespace std;
using std::vector;

namespace mozilla {
namespace dom {
namespace vowifiadapter {
// vowifi adapter log define

#ifndef LOG
#define LOG(args...)   __android_log_print(ANDROID_LOG_INFO, "[VowifiAdapter]" , ## args)
#endif

#ifndef LOGWA
#define LOGWA(args...)   __android_log_print(ANDROID_LOG_WARN, "[VowifiAdapter]" , ## args)
#endif

#ifndef LOGEA
#define LOGEA(args...)   __android_log_print(ANDROID_LOG_ERROR, "[VowifiAdapter]" , ## args)
#endif


/**
 * UICC application types.
 */
const unsigned long APPTYPE_UNKNOWN = 0;
const unsigned long APPTYPE_SIM = 1;
const unsigned long APPTYPE_USIM = 2;
const unsigned long APPTYPE_RUIM = 3;
const unsigned long APPTYPE_CSIM = 4;
const unsigned long APPTYPE_ISIM = 5;

/**
 * Authentication types for UICC challenge.
 * See RFC 4186 for more detail.
 */
const unsigned long AUTHTYPE_EAP_SIM = 128;
const unsigned long AUTHTYPE_EAP_AKA = 129;

typedef uint32_t ZBOOL;
static const uint32_t ZTRUE = 1;
static const uint32_t ZFALSE = 0;

enum IPVersion {
    NONE = 0, IP_V4 = 1, IP_V6 = 2
};

vector<string> stdSplit(const string &s, const string &seperator) {
    LOG("Entering stdSplit");
    vector < string > result;
    typedef string::size_type string_size;
    string_size i = 0;

    while (i != s.size()) {
        //找到字符串中首个不等于分隔符的字母；
        int flag = 0;
        while (i != s.size() && flag == 0) {
            flag = 1;
            for (string_size x = 0; x < seperator.size(); ++x)
                if (s[i] == seperator[x]) {
                    ++i;
                    flag = 0;
                    break;
                }
        }

        //找到又一个分隔符，将两个分隔符之间的字符串取出；
        flag = 0;
        string_size j = i;
        while (j != s.size() && flag == 0) {
            for (string_size x = 0; x < seperator.size(); ++x)
                if (s[j] == seperator[x]) {
                    flag = 1;
                    break;
                }
            if (flag == 0)
                ++j;
        }
        if (i != j) {
            result.push_back(s.substr(i, j - i));
            i = j;
        }
    }
    return result;
}

class SIMAccountInfo {
public:
    SIMAccountInfo() {
    }
    ;
    static string PROP_KEY_IMSI;
    int _subId;
    string _spn;
    string _imei;
    string _imsi;
    string _impi;
    string _impu;
    string _iccType;
    string _mnc;
    string _mcc;
    bool _isRoaming;

    // Used by s2b process.
    string _hplmn;
    string _vplmn;

    // Used by register process.
    string _accountName;
    string _userName;
    string _authName;
    string _authPass;
    string _realm;
    static SIMAccountInfo* Generate(int subId) {
        SIMAccountInfo* info = new SIMAccountInfo();
        info->_subId = subId;

        nsCOMPtr<nsIIccService> iccservice = do_GetService(
                ICC_SERVICE_CONTRACTID);
        if (iccservice) {

            nsCOMPtr<nsIIcc> icc;
            iccservice->GetIccByServiceId(0, getter_AddRefs(icc));
            if (icc) {
                //get imsi
                nsString imsi;
                icc->GetImsi(imsi);
                info->_imsi = string(NS_LossyConvertUTF16toASCII(imsi).get());
                LOG("the value of imsi is %s", info->_imsi.c_str());

                nsCOMPtr<nsIIccInfo> iccInfo;
                icc->GetIccInfo(getter_AddRefs(iccInfo));
                if (iccInfo) {

                    //get mnc,mcc
                    nsString mnc, mcc;
                    iccInfo->GetMnc(mnc);
                    iccInfo->GetMcc(mcc);
                    info->_mnc = string(NS_LossyConvertUTF16toASCII(mnc).get());
                    info->_mcc = string(NS_LossyConvertUTF16toASCII(mcc).get());
                    info->_hplmn = info->_mcc + info->_mnc;
                    LOG("the value of mnc is %s, mcc is %s, hplmn is %s",
                            info->_mnc.c_str(), info->_mcc.c_str(),
                            info->_hplmn.c_str());
                    //get icctype
                    nsString iccType;
                    iccInfo->GetIccType(iccType);
                    info->_iccType = string(
                            NS_LossyConvertUTF16toASCII(iccType).get());
                    LOG("the value of iccType is %s", info->_iccType.c_str());

                    //get spn
                    nsString spn;
                    iccInfo->GetSpn(spn);
                    info->_spn = string(NS_LossyConvertUTF16toASCII(spn).get());
                    if(info->_spn.empty()){
                        info->_spn = "empty";
                    }
                    LOG("the value of spn is %s", info->_spn.c_str());

                } else {
                    LOG("get iccType value error, nsIIccInfo getting failed");
                }

                //get impi, impu, username, realm, authName,
                if (info->_iccType == "isim") {
                    nsCOMPtr<nsIIsimIccInfo> isimInfo;
                    icc->GetIsimInfo(getter_AddRefs(isimInfo));

                    nsString impi;
                    if (isimInfo) {
                        isimInfo->GetImpi(impi);
                        info->_impi = string(
                                NS_LossyConvertUTF16toASCII(impi).get());
                        info->_authName = info->_impi;
                        if (!info->_impi.empty()) {
                            LOG("impi : %s", info->_impi.c_str());
                            vector < string > tempList;
                            tempList = stdSplit(info->_authName, "@");
                            if (tempList.size() == 2) {
                                info->_userName = tempList[0];
                                info->_realm = tempList[1];
                            }

                        } else {
                            LOG("The IMPI is invalid, IMPI: %s",
                                    info->_authName.c_str());
                        }
                    } else {
                        LOG(
                                "get impi value error, nsIIsimIccInfo getting failed");
                    }

                } else {
                    info->_userName = info->_imsi;
                    if(info->_mnc.length() == 2){
                    info->_realm = "ims.mnc0" + info->_mnc + ".mcc" + info->_mcc
                            + ".3gppnetwork.org";
                    }
                    else{
                        info->_realm = "ims.mnc" + info->_mnc + ".mcc" + info->_mcc
                                                    + ".3gppnetwork.org";
                    }
                    info->_authName = info->_imsi + "@" + info->_realm;
                }
                info->_impu = "sip:" + info->_authName;
            } else {
                LOG("get imsi value error, nsIIcc getting failed");
            }
        } else {
            LOG("get imsi value error, nsIIccService getting failed");
        }

        //get imei and vplmn
        nsCOMPtr<nsIMobileConnectionService> mobleservice = do_GetService(
                NS_MOBILE_CONNECTION_SERVICE_CONTRACTID);
        if (mobleservice) {
            nsCOMPtr<nsIMobileConnection> mMobileConnection;
            nsresult rv = mobleservice->GetItemByServiceId(0,
                    getter_AddRefs(mMobileConnection));
            if (NS_SUCCEEDED(rv) && mMobileConnection) {
                //get imei
                nsCOMPtr<nsIMobileDeviceIdentities> deviceIdentities;
                mMobileConnection->GetDeviceIdentities(
                        getter_AddRefs(deviceIdentities));
                if (deviceIdentities) {
                    nsString imei;
                    deviceIdentities->GetImei(imei);
                    info->_imei = string(
                            NS_LossyConvertUTF16toASCII(imei).get());
                    LOG("the value of imei is %s", info->_imei.c_str());
                    info->_accountName = info->_imei;
                } else {
                    LOG(
                            "get imei value error, nsIMobileDeviceIdentities getting failed");
                }

                //generate vplmn
                nsCOMPtr<nsIMobileConnectionInfo> connectionInfo;
                mMobileConnection->GetVoice(getter_AddRefs(connectionInfo));
                if (connectionInfo) {
                    connectionInfo->GetRoaming(&info->_isRoaming);
                    LOG("the value of isRoaming is %d", info->_isRoaming);
                    nsCOMPtr<nsIMobileNetworkInfo> networkInfo;
                    connectionInfo->GetNetwork(getter_AddRefs(networkInfo));
                    if (networkInfo) {
                        nsString mnc, mcc;
                        networkInfo->GetMnc(mnc);
                        networkInfo->GetMcc(mcc);
                        info->_vplmn = string(
                                NS_LossyConvertUTF16toASCII(mcc).get())
                                + string(
                                        NS_LossyConvertUTF16toASCII(mnc).get());
                        LOG("the value of vplmn is %s", info->_vplmn.c_str());
                    } else {
                        LOG(
                                "get vplmn value error, nsIMobileNetworkInfo getting failed, vplmn is the same with hplmn");
                                info->_vplmn = info->_hplmn;
                    }
                } else {
                    LOG(
                            "get vplmn value error, nsIMobileConnectionInfo getting failed, , vplmn is the same with hplmn");
                            info->_vplmn = info->_hplmn;
                }
            }
        } else {
            LOG(
                    "get imei value error, nsIMobileConnectionService getting failed");
        }
        info->_authPass = "todo";
        return info;
    }
};

class SecurityConfig {
public:
    string _pcscf4;
    string _pcscf6;
    string _dns4;
    string _dns6;
    string _ip4;
    string _ip6;
    bool _prefIPv4;
    bool _isSOS;
    int _sessionId;
    uint32_t _useIPVersion = NONE;
    SecurityConfig(int sessionId){
        _sessionId = sessionId;
    }

    void update(string& pcscf4, string& pcscf6, string& dns4, string& dns6,
            string& ip4, string& ip6, bool prefIPv4, bool isSOS) {
        _pcscf4 = pcscf4;
        _pcscf6 = pcscf6;
        _dns4 = dns4;
        _dns6 = dns6;
        _ip4 = ip4;
        _ip6 = ip6;
        _prefIPv4 = prefIPv4;
        _isSOS = isSOS;
    }

};

struct CallInfo {
    uint32_t index;
    uint32_t id;
    bool isActive;
    string number;
    bool isConf;
    bool isVedio;
    short int callState;
    bool isMT;
    bool isConfHold;
};

struct ConfCallInfo {
    uint32_t confId;
    bool isActive;
};

struct IncomingCallAction {
    static const uint32_t NORMAL = 0;
    static const uint32_t REJECT = 1;
};

class RegisterIPAddress {
public:
    //static string JSON_PCSCF_SEP = ";";

    string mLocalIPv4;
    string mLocalIPv6;
    string mPcscfIPv4[4];
    string mPcscfIPv6[4];

    uint32_t mIPv4Index = 0;
    uint32_t mIPv6Index = 0;
    uint32_t mIPV4Size = 0;
    uint32_t mIPV6Size = 0;
    string mUsedLocalIP;
    string mUsedPcscfIP;

    RegisterIPAddress(string& localIPv4, string& localIPv6, string& pcscfIPv4,
            string& pcscfIPv6) {
        LOG(
                "Get the s2b ip address from localIPv4: %s localIPv6: %s pcscfIPv4: %s pcscfIPv6: %s",
                localIPv4.c_str(), localIPv6.c_str(), pcscfIPv4.c_str(),
                pcscfIPv6.c_str());

        if ((localIPv4.empty() || pcscfIPv4.empty())
                && (localIPv6.empty() || pcscfIPv6.empty())) {
            LOG(
                    "Can not Get the s2b ip address from localIPv4: %s localIPv6: %s pcscfIPv4: %s pcscfIPv6: %s",
                    localIPv4.c_str(), localIPv6.c_str(), pcscfIPv4.c_str(),
                    pcscfIPv6.c_str());
            return;
        }
        mLocalIPv4 = localIPv4;
        LOG("mLocalIPv4  is %s", mLocalIPv4.c_str());
        mLocalIPv6 = localIPv6;
        LOG("mLocalIPv6  is %s", mLocalIPv6.c_str());
        vector < string > pcscfV4List, pcscfV6List;

        pcscfV4List = stdSplit(pcscfIPv4, ";");
        mIPV4Size = pcscfV4List.size();
        for(unsigned int i = 0; (i != pcscfV4List.size())&&( i<4); ++i){
            mPcscfIPv4[i] = pcscfV4List[i];
            LOG("mPcscfIPv4  is %s", mPcscfIPv4[i].c_str());
        }
        pcscfV6List = stdSplit(pcscfIPv6, ";");
        mIPV6Size = pcscfV6List.size();
        for(unsigned int i = 0; (i != pcscfV6List.size())&&( i<4); ++i){
            mPcscfIPv6[i] = pcscfV6List[i];
            LOG("mPcscfIPv6  is %s", mPcscfIPv6[i].c_str());
        }

    }

    string GetLocalIPv4() {
        LOG("mLocalIPv4  is %s", mLocalIPv4.c_str());
        return mLocalIPv4;
    }

    string GetLocalIPv6() {
        LOG("mLocalIPv6  is %s", mLocalIPv6.c_str());
        return mLocalIPv6;
    }

    string GetPcscfIPv4() {
        if (mIPv4Index < mIPV4Size) {
            LOG("mPcscfIPv4  is %s", mPcscfIPv4[mIPv4Index].c_str());
             return mPcscfIPv4[mIPv4Index++];
        }
        return mPcscfIPv4[0];
    }

    string GetPcscfIPv6() {
        if (mIPv6Index < mIPV6Size) {
            LOG("mPcscfIPv6  is %s", mPcscfIPv6[mIPv6Index].c_str());
            return mPcscfIPv6[mIPv6Index++];
        }
        return mPcscfIPv6[0];
    }

    bool IsIPv4Valid() {
        LOG("mIPV4Size is %d, mIPv4Index is %d", mIPV4Size, mIPv4Index);
        return !mLocalIPv4.empty() && mIPV4Size > 0 && mIPv4Index < mIPV4Size;
    }

    bool IsIPv6Valid() {
        LOG("mIPV6Size is %d, mIPv6Index is %d", mIPV6Size, mIPv6Index);
        return !mLocalIPv6.empty() && mIPV6Size > 0 && mIPv6Index < mIPV6Size;
    }

    string GetCurUsedLocalIP() {
        return mUsedLocalIP;
    }

    string GetCurUsedPcscfIP() {
        return mUsedPcscfIP;
    }

    string GetLocalIP(bool isIPv4) {
        mUsedLocalIP = isIPv4 ? GetLocalIPv4() : GetLocalIPv6();
        return mUsedLocalIP;
    }

    string GetPcscfIP(bool isIPv4) {
        mUsedPcscfIP = isIPv4 ? GetPcscfIPv4() : GetPcscfIPv6();
        return mUsedPcscfIP;
    }

    uint32_t GetValidIPVersion(bool prefIPv4) {
        if (prefIPv4) {
            if (IsIPv4Valid())
                return IPVersion::IP_V4;
            if (IsIPv6Valid())
                return IPVersion::IP_V6;
        } else {
            if (IsIPv6Valid())
                return IPVersion::IP_V6;
            if (IsIPv4Valid())
                return IPVersion::IP_V4;
        }
        LOG("invalid IPVersion");
        return IPVersion::NONE;
    }

};

enum WifiState {
    CONNECTED, DISCONNECTED
};

enum DataRouterState {
    CALL_VOLTE, CALL_VOWIFI, CALL_NONE
};

enum AttachState {
    STATE_IDLE = 0, STATE_PROGRESSING, STATE_CONNECTED
};

struct VolteNetworkType {
    static const uint32_t IEEE_802_11 = -1;
    static const uint32_t E_UTRAN_FDD = 4; // 3gpp
    static const uint32_t E_UTRAN_TDD = 5; // 3gpp
};

struct VowifiNetworkType {
    static const uint32_t IEEE_802_11 = 1;
    static const uint32_t E_UTRAN_FDD = 9; // 3gpp
    static const uint32_t E_UTRAN_TDD = 10; // 3gpp
};

struct Result {
    // For only contains two result, success or fail.
    static const uint32_t FAIL = 0;
    static const uint32_t SUCCESS = 1;

    // For invalid id.
    static const uint32_t INVALID_ID = -1;
};

struct UnsolicitedCode {
    static const uint32_t SECURITY_DPD_DISCONNECTED = 1;
    static const uint32_t SIP_TIMEOUT = 2;
    static const uint32_t SIP_LOGOUT = 3;
    static const uint32_t SECURITY_REKEY_FAILED = 4;
    static const uint32_t SECURITY_STOP = 5;
};

struct NativeErrorCode {
    static const uint32_t IKE_INTERRUPT_STOP = 0xD200 + 198;
    static const uint32_t IKE_HANDOVER_STOP    = 0xD200 + 199;
    static const uint32_t DPD_DISCONNECT = 0xD200 + 15;
    static const uint32_t IPSEC_REKEY_FAIL = 0xD200 + 10;
    static const uint32_t IKE_REKEY_FAIL = 0xD200 + 11;
    static const uint32_t REG_TIMEOUT = 0xE100 + 5;
    static const uint32_t REG_SERVER_FORBIDDEN = 0xE100 + 8;
    static const uint32_t REG_EXPIRED_TIMEOUT  = 0xE100 + 17;
    static const uint32_t REG_EXPIRED_OTHER    = 0xE100 + 18;
};
static const uint32_t MSG_RESET = 1;
static const uint32_t MSG_RESET_FORCE = 2;
static const uint32_t MSG_ATTACH = 3;
static const uint32_t MSG_DEATTACH = 4;
static const uint32_t MSG_REGISTER = 5;
static const uint32_t MSG_DEREGISTER = 6;
static const uint32_t MSG_REREGISTER = 7;
static const uint32_t MSG_UPDATE_DATAROUTER_STATE = 8;
static const uint32_t MSG_TERMINATE_CALLS = 9;


static const uint32_t CMD_STATE_INVALID = 0;
static const uint32_t CMD_STATE_PROGRESS = 1;
static const uint32_t CMD_STATE_FINISHED = 2;


static const uint32_t RESET_TIMEOUT = 5000; // 5s
static const uint32_t RESET_STEP_INVALID = 0;
static const uint32_t RESET_STEP_DEREGISTER = 1;
static const uint32_t RESET_STEP_DEATTACH = 2;
static const uint32_t RESET_STEP_FORCE_RESET = 3;

// Interwork Event syntax between SprdVowifiService and VowifiAdapter
/*-------------------------------------------------*
 *    The JSON format                              *
 * ================================================*
 {
 GroupID,
 EvnentID,
 Args [
 {
 "arg1": "1"                      // uint32_t
 },
 {
 "arg2": "1"                      // bool
 },
 {
 "arg3": "28845@sip.vowifi.cn"    //string
 },
 {
 "arg4": [ "12245@sip.mmtel.cn", "12345@video.cn" ]    //string[]
 }
 ]
 }
 *-------------------------------------------------*/


/*---------------------------------------------------
*     1. General
----------------------------------------------------*/

// General parameters
static const std::string JSON_GROUP_ID = "groupId";
static const std::string JSON_EVENT_ID = "eventId";
static const std::string JSON_ARGS     = "args";
static const std::string JSON_EVENT_NAME = "eventName";
static const std::string JSON_RETURN_RESULT = "return_result";

// Service and Callback Group ID
static const std::string EVENT_GROUP_CALL = "mtcCallGrp";
static const std::string EVENT_GROUP_REG  = "mtcRegGrp";
static const std::string EVENT_GROUP_SMS  = "mtcSmsGrp";
static const std::string EVENT_GROUP_S2B  = "mtcS2bGrp";

// Callback constants
static const std::string KEY_EVENT_CODE = "event_code";
static const std::string KEY_EVENT_NAME = "event_name";
static const std::string KEY_EVENT_SUCCESS = "successful";
static const std::string KEY_EVENT_FAIL = "failed";


//S2B event ID
enum enumS2bEventId {
    EVENT_S2B_START = 0,
    EVENT_S2B_STOP,
    EVENT_S2B_GET_STATE,
    EVENT_S2B_SWITCHLOGIN,
    EVENT_S2B_DELETETUNEL,
    EVENT_S2B_AUTH_RESP,
    // mapping to callback
    EVENT_S2B_CBSUCCESSED,
    EVENT_S2B_CBFAILED,
    EVENT_S2B_CBPROGRESSING,
    EVENT_S2B_CBSTOPPED,
    EVENT_S2B_CBAUTH_REQ
};

enum enumMtcS2bCbId {
    CALLBACK_S2B_S2B_OK = 0,
    CALLBACK_S2B_S2B_FAIL,
    CALLBACK_S2B_AUTH_IND,
    CALLBACK_S2B_SIM_AUTH_IND
};


/*---------------------------------------------------
*     2. S2B Attach
----------------------------------------------------*/

//S2B parameters
static const std::string SECURITY_PARAM_ID = "session_id";
static const std::string SECURITY_PARAM_STATE_CODE = "state_code";

static const std::string SECURITY_PARAM_ERROR_CODE =    "security_param_error_code";
static const std::string SECURITY_PARAM_PROGRESS_STATE ="security_param_progress_state";
static const std::string SECURITY_PARAM_LOCAL_IP4 =     "security_param_local_ip4";
static const std::string SECURITY_PARAM_LOCAL_IP6 =     "security_param_local_ip6";
static const std::string SECURITY_PARAM_PCSCF_IP4 =     "security_param_pcscf_ip4";
static const std::string SECURITY_PARAM_PCSCF_IP6 =     "security_param_pcscf_ip6";
static const std::string SECURITY_PARAM_DNS_IP4 =       "security_param_dns_ip4";
static const std::string SECURITY_PARAM_DNS_IP6 =       "security_param_dns_ip6";
static const std::string SECURITY_PARAM_PREF_IP4 =      "security_param_pref_ip4";
static const std::string SECURITY_PARAM_HANDOVER =      "security_param_handover";
static const std::string SECURITY_PARAM_SOS =           "security_param_is_sos";
static const std::string SECURITY_PARAM_SUBID =         "security_param_sub_id";
static const std::string SECURITY_PARAM_NONCE =         "security_param_akanonce";

//S2B event name of stub
static const std::string EVENT_NAME_S2BSTART = "s2b_start";
static const std::string EVENT_NAME_S2BSTOP = "s2b_stop";
static const std::string EVENT_NAME_S2BSTATE = "s2b_state";
static const std::string EVENT_NAME_S2BSWITCH_LOGIN = "s2b_switch_login";
static const std::string EVENT_NAME_S2BDELE_TUNEL = "s2b_dele_tunel";

//S2B event name of callback
static const std::string EVENT_NAME_ONSUCCESSED = "attach_successed";
static const std::string EVENT_NAME_ONFAILED = "attach_failed";
static const std::string EVENT_NAME_ONPROGRESSING = "attach_progressing";
static const std::string EVENT_NAME_ONSTOPPED = "attach_stopped";
static const std::string EVENT_NAME_ONAUTH_REQ = "attach_authReq";
static const std::string EVENT_NAME_ONSTATE = "attach_state";



//register event ID
enum enumRegEventId {
    EVENT_REG_OPEN=0,
    EVENT_REG_START,
    EVENT_REG_UPDATESETTINGS,
    EVENT_REG_LOGIN,
    EVENT_REG_REFRESH,
    EVENT_REG_LOGOUT,
    EVENT_REG_RESET,
    EVENT_REG_SET_GEOENABLE,
    EVENT_REG_SET_GEOINFO,
    // mapping to callback
    EVENT_REG_LOGIN_OK ,
    EVENT_REG_LOGIN_FAILED,
    EVENT_REG_LOGOUTED,
    EVENT_REG_REREGISTER_OK,
    EVENT_REG_REREGISTER_FAILED,
    EVENT_REG_REGISTER_STATE_UPDATE,
    EVENT_REG_AUTH_CHALLENGE_REQ=98,
    EVENT_REG_AUTH_CHALLENGE_RESP=99
};

/*---------------------------------------------------
*     3. Register
----------------------------------------------------*/

// Register
static const std::string REGISTER_PARAM_STATE_CODE  = "state_code";
static const std::string REGISTER_PARAM_RETRY_AFTER = "retry_after";
static const std::string REGISTER_PARAM_NONCE       = "auth_nonce";
static const std::string REGISTER_PARAM_CHALLENGE   = "auth_challenge";

// register event name
static const std::string EVENT_NAME_OPEN = "cliOpen";
static const std::string EVENT_NAME_START ="cliStart";
static const std::string EVENT_NAME_UPDATESETTINGS ="cliUpdateSettings";
static const std::string EVENT_NAME_LOGIN = "cliLogin";
static const std::string EVENT_NAME_REFRESH = "cliRefresh";
static const std::string EVENT_NAME_LOGOUT = "cliLogout";
static const std::string EVENT_NAME_RESET = "cliReset";
static const std::string EVENT_NAME_SET_GEOENABLE = "setGeoEnable";
static const std::string EVENT_NAME_SET_GEOINFO = "setGeoInfo";

// register callback
static const std::string EVENT_NAME_LOGIN_OK = "login_ok";
static const std::string EVENT_NAME_LOGIN_FAILED = "login_failed";
static const std::string EVENT_NAME_LOGOUTED = "logouted";
static const std::string EVENT_NAME_REREGISTER_OK = "refresh_ok";
static const std::string EVENT_NAME_REREGISTER_FAILED = "refresh_failed";
static const std::string EVENT_NAME_STATE_UPDATE = "state_update";
static const std::string EVENT_NAME_AUTHCHALL_REQ  = "auth_challenge_req";
static const std::string EVENT_NAME_AUTHCHALL_RESP = "auth_challenge_resp";


struct RegisterState {
    static const uint32_t STATE_IDLE = 0;
    static const uint32_t STATE_PROGRESSING = 1;
    static const uint32_t STATE_CONNECTED = 2;
};

/*---------------------------------------------------
*     4. Call and Conference  please go to VowifiCallManager.h
----------------------------------------------------*/


} // namespace vowifiadapter
} // namespace dom
} // namespace mozilla

#endif // mozilla_dom_vowifiadapter_VowifiAdapterConstant_h
