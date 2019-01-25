/************************************************************************

        Copyright (c) 2005-2017 by Spreadtrum, Inc.
                       All rights reserved.

     This software is confidential and proprietary to Spreadtrum,
     Inc. No part of this software may be reproduced, stored, transmitted,
     disclosed or used in any form or by any means other than as expressly
     provided by the written license agreement between Spreadtrum and its
     licensee.

************************************************************************/

#include "VowifiSecurityManager.h"
#include "VowifiAdapterControl.h"
#include "socket/VowifiArguments.h"
#include "json/json.h"
#include "nsServiceManagerUtils.h"

#include "nsINetworkInterface.h"
#include "nsINetworkManager.h"
#include "mozilla/dom/ToJSValue.h"
#include "mozilla/Services.h"
#include "nsJSON.h"
#include "nsJSUtils.h"
#include "nsINetworkService.h"
using namespace std;
namespace mozilla {
namespace dom {
namespace vowifiadapter {

VowifiSecurityManager::VowifiSecurityManager(VowifiAdapterControl *adapterControl):
        mAdapterControl(adapterControl), mState(-1) {
    LOG("Entering VowifiSecurityManager initialization");
}

void VowifiSecurityManager::RegisterListeners(SecurityListener* listener) {
    LOG("Register SecurityListener");
    if (listener == nullptr) {
        LOGEA("Can not register the callback as it is null.");
        return;
    }
    mListener = listener;
}

void VowifiSecurityManager::UnregisterListeners() {
    LOG("uRegister the listener");
    mListener = nullptr;
}

// get netid
int VowifiSecurityManager::GetNetId() {
    string netIdString;
    LOG("Entering getNetId");
    nsCOMPtr<nsINetworkManager> networkManager = do_GetService(
            "@mozilla.org/network/manager;1");

    nsCOMPtr<nsINetworkService> networkService = do_GetService(
             "@mozilla.org/network/service;1");
    if (NS_WARN_IF(!networkManager)) {
        LOGEA("get neworkManager error.");
    } else {
        nsCOMPtr<nsINetworkInfo> activeNetworkInfo;
        networkManager->GetActiveNetworkInfo(getter_AddRefs(activeNetworkInfo));
        if (NS_WARN_IF(!activeNetworkInfo)) {
            LOGEA("get activeNetworkInfo error.");
        } else {
            nsString ifname;
            activeNetworkInfo->GetName(ifname);

            if (networkService) {
                mozilla::AutoSafeJSContext cx;
                JS::RootedValue rvalue(cx);
                JS::MutableHandleValue netID(&rvalue);
                networkService->GetNetId(ifname,netID);
               JSString* valstr;
               valstr = ToString(cx, netID);
               JSFlatString* flatNetId = JS_ASSERT_STRING_IS_FLAT( valstr);
               nsAutoString charsNetId;
               AssignJSFlatString(charsNetId, flatNetId);
               netIdString = string(
                       NS_LossyConvertUTF16toASCII(charsNetId).get());
               mNetId = atoi(netIdString.c_str());
               LOG("the value of netid is %d", mNetId);
               return mNetId;
            } else {
                LOGEA("get netid error, get nsINetworkService error");
            }
        }
    }
    LOG("get netid error, default value is -1");
    return mNetId;
}

void VowifiSecurityManager::Attach(SIMAccountInfo* simAccountInfo) {
    LOG("Start the s2b attach.");
    int type = 1;  //normal = 1
    int subId = mSubId;
    int netId = GetNetId();
    string hplmn = simAccountInfo->_hplmn;
    string vplmn = simAccountInfo->_vplmn;
    //int hplmn = 51009; //temp change for smartfren
    //int vplmn = 51009;
   // bool roamingType = simAccountInfo->_isRoaming;
    string imsi = simAccountInfo->_imsi;
    string imei = simAccountInfo->_imei;
    string imsIpAddress = GetVolteUsedLocalAddr();
    LOG("Attach::imsIpAddress %s",imsIpAddress.c_str()); 
    string lastEpdgAddr;
    Json::Value root, args;
    root[JSON_GROUP_ID] = EVENT_GROUP_S2B;
    root[JSON_EVENT_ID] = EVENT_S2B_START;
    root[JSON_EVENT_NAME] = EVENT_NAME_S2BSTART;
    VowifiArguments argHelp;
    argHelp.PushJsonArg(type);
    argHelp.PushJsonArg(subId);
    argHelp.PushJsonArg(netId);
    argHelp.PushJsonArg(hplmn);
    argHelp.PushJsonArg(vplmn);
    //argHelp.PushJsonArg(roamingType);
    argHelp.PushJsonArg(imsi);
    argHelp.PushJsonArg(imei);
    argHelp.PushJsonArg(imsIpAddress);
    argHelp.PushJsonArg(lastEpdgAddr);
    argHelp.GetJsonArgs(args);
    root[JSON_ARGS] = args;
    string noticeStr = root.toStyledString();

    if (!mAdapterControl->SendToService(noticeStr)) {
        LOGEA("s2b attach error");
    }


}

void VowifiSecurityManager::attachStopped(bool forHandover, int errorCode) {
    if (mListener != NULL)
        mListener->OnStopped(forHandover, errorCode);
    mState = AttachState::STATE_IDLE;
    mSecurityConfig = NULL;
}

void VowifiSecurityManager::EapAkaRequest(string akaNonce) {
    LOG("Entering EapAkaRequest");
    string akaResponse;

    //GetIccAuthentication
    nsCOMPtr<nsIIccService> service =
       do_GetService(ICC_SERVICE_CONTRACTID);
     NS_ENSURE_TRUE_VOID(service);

     nsCOMPtr<nsIIcc> icc;
     service->GetIccByServiceId(0, getter_AddRefs(icc));
     NS_ENSURE_TRUE_VOID(icc);
     nsCOMPtr<nsIIccCallback> callback = new VowifiSecurityManager();
     icc->GetIccAuthentication(APPTYPE_USIM, AUTHTYPE_EAP_AKA, NS_ConvertASCIItoUTF16(akaNonce.c_str()), callback);
}

void VowifiSecurityManager::Deattach(bool isHandover, bool isForceDeat) {
    LOG("Start the s2b Deattach.");
    int dummyId = 0;
    Json::Value root;
    Json::Value args;
    root[JSON_GROUP_ID] = EVENT_GROUP_S2B;
    root[JSON_EVENT_ID] = EVENT_S2B_STOP;
    root[JSON_EVENT_NAME] = EVENT_NAME_S2BSTOP;
    VowifiArguments argHelp;

    if (isForceDeat)
        // just send a fake session-Id
        argHelp.PushJsonArg(dummyId);
    else if(mSecurityConfig != nullptr){
        argHelp.PushJsonArg(mSecurityConfig->_sessionId);
    }else{
        attachStopped(isHandover, 0);
        return;
    }

    argHelp.PushJsonArg(isHandover);
    argHelp.PushJsonArg(isForceDeat);

    argHelp.GetJsonArgs(args);
    root[JSON_ARGS] = args;
    string noticeStr = root.toStyledString();

    if (!mAdapterControl->SendToService(noticeStr))
    {
        LOGEA("s2b deattach error");
    }
}

void VowifiSecurityManager::ForceStop() {
    LOG("Force stop the s2b. Do force de-attach for no handover.");
    Deattach(false, true);
}

bool VowifiSecurityManager::SetIPVersion(int ipVersion) {
    LOG("Set the IP version: %d", ipVersion);

    Json::Value root;
    Json::Value args;
    Json::Value mVal;
    root[JSON_GROUP_ID] = EVENT_GROUP_S2B;
    if (ipVersion == NONE) {
        root[JSON_EVENT_ID] = EVENT_S2B_DELETETUNEL;
        root[JSON_EVENT_NAME] = EVENT_NAME_S2BDELE_TUNEL;
        string noticeStr = root.toStyledString();
        if (!mAdapterControl->SendToService(noticeStr)) {
            LOGEA("s2b %s error", EVENT_NAME_S2BDELE_TUNEL.c_str());
            return false;
        }
        return true;
    } else {
        root[JSON_EVENT_ID] = EVENT_S2B_SWITCHLOGIN;
        root[JSON_EVENT_NAME] = EVENT_NAME_S2BSWITCH_LOGIN;
        VowifiArguments argHelp;
        argHelp.PushJsonArg(mSecurityConfig->_sessionId);
        argHelp.PushJsonArg(ipVersion);
        argHelp.GetJsonArgs(args);
        root[JSON_ARGS] = args;
        string noticeStr = root.toStyledString();
        if (!mAdapterControl->SendToService(noticeStr)) {
            LOGEA("s2b %s error", EVENT_NAME_S2BSWITCH_LOGIN.c_str());
            return false;
        }
        return true;
    }

}
SecurityConfig* VowifiSecurityManager::GetConfig() {
    return mSecurityConfig;
}

int VowifiSecurityManager::GetCurSecurityState() {
    return mState;
}


void VowifiSecurityManager::SetVolteUsedLocalAddr(string imsRegAddr) {
    mImsRegAddr = imsRegAddr;
}

string VowifiSecurityManager::GetVolteUsedLocalAddr() {
    return mImsRegAddr;
}

void VowifiSecurityManager::OnJsonCallback(int eventId, Json::Value &args) {
    LOG("Entering OnJsonCallback");
    bool bArgs = true;
    bool bAg = false;
    switch (eventId) {
    case EVENT_S2B_CBAUTH_REQ: {
        LOG("attach authentication request");
        string akaNonce;
        bAg = VowifiArguments::GetOneArg(args, SECURITY_PARAM_NONCE,
                akaNonce);
        if (!bAg) {
            bArgs = false;
        } else {
            EapAkaRequest(akaNonce);
        }
        break;

    }
    case EVENT_S2B_CBSUCCESSED: {
        LOG("attach success");
        string localIP4, localIP6, pcscfIP4, pcscfIP6, dnsIP4, dnsIP6;
        int sessionId;
        ZBOOL prefIPv4 = ZFALSE;
        ZBOOL isSOS = ZFALSE;

        bAg = VowifiArguments::GetOneArg(args, SECURITY_PARAM_ID,
                        sessionId);
        bAg &= VowifiArguments::GetOneArg(args, SECURITY_PARAM_LOCAL_IP4,
                localIP4);
        bAg &= VowifiArguments::GetOneArg(args, SECURITY_PARAM_LOCAL_IP6,
                localIP6);
        bAg &= VowifiArguments::GetOneArg(args, SECURITY_PARAM_PCSCF_IP4,
                pcscfIP4);
        bAg &= VowifiArguments::GetOneArg(args, SECURITY_PARAM_PCSCF_IP6,
                pcscfIP6);
        bAg &= VowifiArguments::GetOneArg(args, SECURITY_PARAM_DNS_IP4,
                dnsIP4);
        bAg &= VowifiArguments::GetOneArg(args, SECURITY_PARAM_DNS_IP6,
                dnsIP6);
        bAg &= VowifiArguments::GetOneArg(args, SECURITY_PARAM_PREF_IP4,
                prefIPv4);
        bAg &= VowifiArguments::GetOneArg(args, SECURITY_PARAM_SOS,
                isSOS);

        if (!bAg) {
            bArgs = false;
        } else {
            mSecurityConfig = new SecurityConfig(sessionId);
            mSecurityConfig->update(pcscfIP4, pcscfIP6, dnsIP4,
                    dnsIP6, localIP4, localIP6, prefIPv4, isSOS);

            // Switch the IP version as preferred first, then notify the result and
            // update the state.
            int useIPVersion = mSecurityConfig->_prefIPv4 ? IP_V4 : IP_V6;
            if (SetIPVersion(useIPVersion)) {
                mSecurityConfig->_useIPVersion = useIPVersion;
                mListener->OnSuccessed();
                mState = STATE_CONNECTED;
            } else {
                mListener->OnFailed(0);
                mState = STATE_IDLE;
            }
        }
    }
        break;
    case EVENT_S2B_CBPROGRESSING: {
        int sessionId = 0;
        int stateCode = 0;
        bAg  = VowifiArguments::GetOneArg(args, SECURITY_PARAM_ID,
                sessionId);
        bAg &= VowifiArguments::GetOneArg(args, SECURITY_PARAM_STATE_CODE,
                stateCode);

        if (!bAg) {
            bArgs = false;
        } else {
            if (mListener != nullptr) {
                mListener->OnProgress(stateCode);
            }
            mState = STATE_PROGRESSING;
            mSecurityConfig = new SecurityConfig(sessionId);
        }
    }
        break;
    case EVENT_S2B_CBFAILED: {
        LOG("attach failed");
        int errorCode = 0;
        bAg = VowifiArguments::GetOneArg(args, SECURITY_PARAM_STATE_CODE,
                errorCode);
        LOG("S2b attach failed, errorCode: %d", errorCode);
        if (!bAg) {
            bArgs = false;
        } else {
            if (mListener != nullptr) {
                mListener->OnFailed(errorCode);
            }
            mState = STATE_IDLE;
        }
    }

        break;
    case EVENT_S2B_CBSTOPPED:{
        int errorCode = 0;
        bAg = VowifiArguments::GetOneArg(args, SECURITY_PARAM_STATE_CODE,
                errorCode);
        if (!bAg) {
            bArgs = false;
        } else {
            bool forHandover = (errorCode == NativeErrorCode::IKE_HANDOVER_STOP);
            attachStopped(forHandover, errorCode);
        }
        break;
    }
    default:
        LOGWA("SecurityService eventId=%d is not supported, skip it!", eventId);
        break;
    }

    if (!bArgs) {
        LOGWA("SecurityService arguments list of eventId=%d is incorrected, skip it!",
             eventId);
    }
}

//nsIIccCallback
NS_IMPL_ISUPPORTS(VowifiSecurityManager, nsIIccCallback)

/* void notifySuccess (); */
NS_IMETHODIMP VowifiSecurityManager::NotifySuccess()
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* void notifySuccessWithBoolean (in boolean aResult); */
NS_IMETHODIMP VowifiSecurityManager::NotifySuccessWithBoolean(bool aResult)
{
    LOG("Entering NotifySuccessWithBoolean");
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* void notifyGetCardLockRetryCount (in long aCount); */
NS_IMETHODIMP VowifiSecurityManager::NotifyGetCardLockRetryCount(int32_t aCount)
{
    LOG("Entering NotifyGetCardLockRetryCount");
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* void notifyRetrievedIccContacts ([array, size_is (aCount)] in nsIIccContact aContacts, in uint32_t aCount); */
NS_IMETHODIMP VowifiSecurityManager::NotifyRetrievedIccContacts(nsIIccContact **aContacts, uint32_t aCount)
{
    LOG("Entering NotifyRetrievedIccContacts");
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* void notifyUpdatedIccContact (in nsIIccContact aContact); */
NS_IMETHODIMP VowifiSecurityManager::NotifyUpdatedIccContact(nsIIccContact *aContact)
{
    LOG("Entering NotifyUpdatedIccContact");
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* void notifyError (in DOMString aErrorMsg); */
NS_IMETHODIMP VowifiSecurityManager::NotifyError(const nsAString & aErrorMsg)
{
    LOG("Entering NotifyError");
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* void notifyCardLockError (in DOMString aErrorMsg, in long aRetryCount); */
NS_IMETHODIMP VowifiSecurityManager::NotifyCardLockError(const nsAString & aErrorMsg, int32_t aRetryCount)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP
VowifiSecurityManager::NotifyAuthResponse(const nsAString & aData)
{
  LOG("Entering NotifyAuthResponse");

  RefPtr<VowifiInterfaceLayer> interfaceLayer =
          VowifiInterfaceLayer::GetInstance();
  if (!interfaceLayer) {
    return NS_ERROR_FAILURE;
  }
  mAkaResponse = string(NS_LossyConvertUTF16toASCII(aData).get());
  LOG("the value of mAkaResponse is %s", mAkaResponse.c_str());
  Json::Value root, args;
  root[JSON_GROUP_ID] = EVENT_GROUP_S2B;
  root[JSON_EVENT_ID] = EVENT_S2B_AUTH_RESP;
  root[JSON_EVENT_NAME] = EVENT_NAME_ONAUTH_REQ;
  VowifiArguments argHelp;
  argHelp.PushJsonArg(mSubId);
  argHelp.PushJsonArg(mAkaResponse);
  argHelp.GetJsonArgs(args);
  root[JSON_ARGS] = args;
  string noticeStr = root.toStyledString();
  LOG("NotifyAuthResponse the data going to be sent is %s", noticeStr.c_str());
  if (!interfaceLayer->mAdapterControl->SendToService(noticeStr)) {
      LOGEA("send EapAkaResponse error");
  }
  return NS_OK;
}

VowifiSecurityManager::~VowifiSecurityManager() {
}

} // namespace vowifiadapter
} // namespace dom
} // namespace mozilla
