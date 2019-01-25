/************************************************************************

        Copyright (c) 2005-2017 by Spreadtrum, Inc.
                       All rights reserved.

     This software is confidential and proprietary to Spreadtrum,
     Inc. No part of this software may be reproduced, stored, transmitted,
     disclosed or used in any form or by any means other than as expressly
     provided by the written license agreement between Spreadtrum and its
     licensee.

************************************************************************/

#include "VowifiRegisterManager.h"
#include "VowifiAdapterControl.h"
#include "socket/VowifiArguments.h"
#include "json/json.h"

using namespace std;
namespace mozilla {
namespace dom {
namespace vowifiadapter {

VowifiRegisterManager::VowifiRegisterManager(VowifiAdapterControl *ptr) :
        mAdapterControl(ptr), mLoginPrepared(false), mRegisterState(
                RegisterState::STATE_IDLE) {
    LOG("Entering VowifiRegisterManager initialization");
}


void VowifiRegisterManager::RegisterListeners(RegisterListener* listener) {
    LOG("Register RegisterListener");
    if (listener == nullptr) {
        LOG("Can not register the callback as it is null.");
        return;
    }
    mListener = listener;
}
void VowifiRegisterManager::UnregisterListeners() {
    LOG("UnRegister the listener");
    mListener = nullptr;
}
void VowifiRegisterManager::PrepareForLogin(SIMAccountInfo* simAccountInfo) {
    LOG("Start the s2b PrepareForLogin.");

    {
        Json::Value root;
        root[JSON_GROUP_ID] = EVENT_GROUP_REG;
        root[JSON_EVENT_ID] = EVENT_REG_RESET;
        root[JSON_EVENT_NAME] = EVENT_NAME_RESET;
        string noticeStr = root.toStyledString();
        if (!mAdapterControl->SendToService(noticeStr)) {
            LOG("register PrepareForLogin error");
        }
    }
    mLoginPrepared = false;
    UpdateRegisterState(RegisterState::STATE_IDLE);

    if (simAccountInfo == nullptr) {
        LOG("Can not get the account info ");
        if (mListener != nullptr)
            mListener->OnPrepareFinished(false, 0);
        return;
    }
    // Prepare for login, need open account, start client and update settings.
    if (CliOpen(simAccountInfo) && CliStart() && CliUpdateSettings(simAccountInfo)) {
        mLoginPrepared = true;
    }

    if (mListener != nullptr)
        mListener->OnPrepareFinished(mLoginPrepared, 0);
}

void VowifiRegisterManager::Login(bool isIPv4, string localIP, string pcscfIP) {

    LOG("Try to login to the ims, current register state: %d", mRegisterState);
    LOG("Login with the local ip is: %s, pcscf ip is: %s", localIP.c_str(),
            pcscfIP.c_str());

    if (mRegisterState == RegisterState::STATE_CONNECTED) {
        // Already registered notify the register state.
        if (mListener != nullptr)
            mListener->OnLoginFinished(true, 0, 0);
        return;
    } else if (mRegisterState == RegisterState::STATE_PROGRESSING) {
        // Already in the register process, do nothing.
        return;
    } else if (!mLoginPrepared) {
        // Make sure already prepare for login, otherwise the login process can not start.
        LOG("Please prepare for login first.");
        if (mListener != nullptr)
            mListener->OnLoginFinished(false, 0, 0);
        return;
    }
    // The current register status is false.

    UpdateRegisterState(RegisterState::STATE_PROGRESSING);

    {
        bool isSOS = false;
        string dnsIP = "";
        bool isRelogin =  false;
        Json::Value root, args;
        VowifiArguments argHelp;
        root[JSON_GROUP_ID] = EVENT_GROUP_REG;
        root[JSON_EVENT_ID] = EVENT_REG_LOGIN;
        root[JSON_EVENT_NAME] = EVENT_NAME_LOGIN;
        argHelp.PushJsonArg(isSOS);
        argHelp.PushJsonArg(isIPv4);
        argHelp.PushJsonArg(localIP);
        argHelp.PushJsonArg(pcscfIP);
        argHelp.PushJsonArg(dnsIP);
        argHelp.PushJsonArg(isRelogin);

        argHelp.GetJsonArgs(args);
        args.toStyledString();
        root[JSON_ARGS] = args;
        string noticeStr = root.toStyledString();
        if (!mAdapterControl->SendToService(noticeStr)) {
            LOG("register CliUpdateSettings error");
            UpdateRegisterState(RegisterState::STATE_IDLE);
            if (mListener != nullptr)
                mListener->OnLoginFinished(false, 0, 0);
        }
    }

}

void VowifiRegisterManager::Reregister(int networkType, string& networkInfo){
    LOG("Re-register, with the networkType: %d", networkType);
    if (mRegisterState != RegisterState::STATE_CONNECTED || networkInfo.empty()) {
        // The current register state is false, can not re-register.
        LOG("The current register state: %d, can not re-register with empty info which used to handover.",mRegisterState);
        return;
    }
    int type = GetVowifiNetworkType(networkType);
    {
        Json::Value root, args;
        VowifiArguments argHelp;

        root[JSON_GROUP_ID] = EVENT_GROUP_REG;
        root[JSON_EVENT_ID] = EVENT_REG_REFRESH;
        root[JSON_EVENT_NAME] = EVENT_NAME_REFRESH;
        argHelp.PushJsonArg(type);
        argHelp.PushJsonArg(networkInfo);

        argHelp.GetJsonArgs(args);
        args.toStyledString();
        root[JSON_ARGS] = args;
        string noticeStr = root.toStyledString();
        if (!mAdapterControl->SendToService(noticeStr)) {
            LOG("native Reregister failed");
        }
    }
}

void VowifiRegisterManager::Deregister() {
    LOG("Try to logout from the ims, current register state is: %d",
            mRegisterState);

    if (mRegisterState == RegisterState::STATE_IDLE) {
        // The current status is idle or unknown, give the callback immediately.
        if (mListener != nullptr)
            mListener->OnLogout(0);
        return;
    } else if (mRegisterState == RegisterState::STATE_PROGRESSING) {
        // Already in the register progress, we'd like to cancel current process.
        ForceStop();
    } else if (mRegisterState == RegisterState::STATE_CONNECTED) {
        // The current register status is true;
        {
            Json::Value root;
            root[JSON_GROUP_ID] = EVENT_GROUP_REG;
            root[JSON_EVENT_ID] = EVENT_REG_LOGOUT;
            root[JSON_EVENT_NAME] = EVENT_NAME_LOGOUT;
            string noticeStr = root.toStyledString();
            if (!mAdapterControl->SendToService(noticeStr)) {
                LOG("register PrepareForLogin error");
            }
        }
        UpdateRegisterState(RegisterState::STATE_PROGRESSING);
    } else {
        // Shouldn't be here.
        LOG(
                "Try to logout from the ims, shouldn't be here. register state is: %d",
                mRegisterState);
    }
}

bool VowifiRegisterManager::ForceStop() {

    LOG("Stop current register process. registerState is: %d", mRegisterState);
    {
        Json::Value root;
        root[JSON_GROUP_ID] = EVENT_GROUP_REG;
        root[JSON_EVENT_ID] = EVENT_REG_RESET;
        root[JSON_EVENT_NAME] = EVENT_NAME_RESET;
        string noticeStr = root.toStyledString();
        if (!mAdapterControl->SendToService(noticeStr)) {
            LOG("register ForceStop error");
        }
    }
    // For force stop, we'd like do not handle the failed action, and set the register state
    // to idle immediately.
    UpdateRegisterState(RegisterState::STATE_IDLE);
    mLoginPrepared = false;
    return true;
}

int VowifiRegisterManager::GetCurRegisterState() {
    return mRegisterState;
}

bool VowifiRegisterManager::CliOpen(SIMAccountInfo* info) {
    LOG("Try to open the account.");

    if (info == nullptr) {
        LOG(
                "Failed open account as register interface or the account info is null.");
        return false;
    }
    {
        Json::Value root, args;
        VowifiArguments argHelp;
        root[JSON_GROUP_ID] = EVENT_GROUP_REG;
        root[JSON_EVENT_ID] = EVENT_REG_OPEN;
        root[JSON_EVENT_NAME] = EVENT_NAME_OPEN;
        argHelp.PushJsonArg(info->_accountName);
        argHelp.GetJsonArgs(args);
        args.toStyledString();
        root[JSON_ARGS] = args;
        string noticeStr = root.toStyledString();
        if (!mAdapterControl->SendToService(noticeStr)) {
            LOG("register cliOpen error");
            return false;
        }
    }
    return true;

}

bool VowifiRegisterManager::CliStart() {
    LOG("Try to start the client.");
    {
        Json::Value root;
        root[JSON_GROUP_ID] = EVENT_GROUP_REG;
        root[JSON_EVENT_ID] = EVENT_REG_START;
        root[JSON_EVENT_NAME] = EVENT_NAME_START;
        string noticeStr = root.toStyledString();
        if (!mAdapterControl->SendToService(noticeStr)) {
            LOG("register cliStart error");
            return false;
        }
    }
    return true;
}

bool VowifiRegisterManager::CliUpdateSettings(SIMAccountInfo* info) {
    LOG("Try to update the account settings.");
    {
        Json::Value root, args;
        VowifiArguments argHelp;
        string bssid = "28:6c:07:67:24:d2";  //hard code for "vowifi" AP
        bool isSrvccSupt = false;
        root[JSON_GROUP_ID] = EVENT_GROUP_REG;
        root[JSON_EVENT_ID] = EVENT_REG_UPDATESETTINGS;
        root[JSON_EVENT_NAME] = EVENT_NAME_UPDATESETTINGS;
        argHelp.PushJsonArg(info->_subId);
        argHelp.PushJsonArg(info->_spn);
        argHelp.PushJsonArg(info->_imei);
        argHelp.PushJsonArg(info->_userName);
        argHelp.PushJsonArg(info->_authName);
        argHelp.PushJsonArg(info->_authPass);
        argHelp.PushJsonArg(info->_realm);
        argHelp.PushJsonArg(info->_impu);
        argHelp.PushJsonArg(bssid);
        argHelp.PushJsonArg(isSrvccSupt);
        argHelp.GetJsonArgs(args);
        args.toStyledString();
        root[JSON_ARGS] = args;
        string noticeStr = root.toStyledString();
        if (!mAdapterControl->SendToService(noticeStr)) {
            LOG("register CliUpdateSettings error");
            return false;
        }
    }
    return true;
}

void VowifiRegisterManager::UpdateRegisterState(int newState) {
    mRegisterState = newState;
    if (mListener != nullptr)
        mListener->OnRegisterStateChanged(newState);
}

int VowifiRegisterManager::GetVowifiNetworkType(int volteType) {
    switch (volteType) {
    case VolteNetworkType::IEEE_802_11:
        return VowifiNetworkType::IEEE_802_11;
    case VolteNetworkType::E_UTRAN_FDD:
        return VowifiNetworkType::E_UTRAN_FDD;
    case VolteNetworkType::E_UTRAN_TDD:
        return VowifiNetworkType::E_UTRAN_TDD;
    default:
        LOG("Do not support this volte network type now, type is: %d",
                volteType);
        return VowifiNetworkType::IEEE_802_11;
    }
}

void VowifiRegisterManager::EapAkaRequest(string akaNonce) {
    LOG("Entering EapAkaRequest");
    string akaResponse;

    //GetIccAuthentication
    nsCOMPtr<nsIIccService> service =
       do_GetService(ICC_SERVICE_CONTRACTID);
     NS_ENSURE_TRUE_VOID(service);

     nsCOMPtr<nsIIcc> icc;
     service->GetIccByServiceId(0, getter_AddRefs(icc));
     NS_ENSURE_TRUE_VOID(icc);
     nsCOMPtr<nsIIccCallback> callback = new VowifiRegisterManager();
     icc->GetIccAuthentication(APPTYPE_USIM, AUTHTYPE_EAP_AKA, NS_ConvertASCIItoUTF16(akaNonce.c_str()), callback);
}

void VowifiRegisterManager::OnRegisterStateChanged(int eventId,
        Json::Value &args) {
    LOG("Get the register state changed callback");
    bool bArgs = true;
    bool bAg = false;
    switch (eventId) {
    case EVENT_REG_AUTH_CHALLENGE_REQ:{
        string akaNonce;
        bAg = VowifiArguments::GetOneArg(args, REGISTER_PARAM_NONCE, akaNonce);
        if (!bAg) {
            bArgs = false;
        } else {
            EapAkaRequest(akaNonce);
        }
    }
        break;

    case EVENT_REG_LOGIN_OK:
        LOG("Register success");
        // Update the register state to connected, and notify the state changed.
        UpdateRegisterState(RegisterState::STATE_CONNECTED);
        if (mListener != nullptr)
            mListener->OnLoginFinished(true, 0, 0);
        break;
    case EVENT_REG_LOGIN_FAILED: {
        LOG("Register failed");
        int stateCode = 0;
        int retryAfter = 0;
        // Update the register state to unknown, and notify the state changed.
        UpdateRegisterState(RegisterState::STATE_IDLE);
        if (mListener != nullptr) {
            bAg = VowifiArguments::GetOneArg(args, REGISTER_PARAM_STATE_CODE, stateCode);
            bAg &= VowifiArguments::GetOneArg(args, REGISTER_PARAM_RETRY_AFTER, retryAfter);

        }
        if (!bAg) {
            bArgs = false;
        } else {
            mListener->OnLoginFinished(false, stateCode, retryAfter);
        }
    }
        break;
    case EVENT_REG_LOGOUTED: {
        int stateCode = 0;
        // Update the register state to idle, and reset the sip stack.
        UpdateRegisterState(RegisterState::STATE_IDLE);
        mLoginPrepared = false;
        if (mListener != nullptr) {
            bAg = VowifiArguments::GetOneArg(args, REGISTER_PARAM_STATE_CODE, stateCode);
        }
        if (!bAg) {
            bArgs = false;
        } else {
            mListener->OnLogout(stateCode);
        }
    }
        break;
    case EVENT_REG_REREGISTER_OK:
        UpdateRegisterState(RegisterState::STATE_CONNECTED);
        if (mListener != nullptr)
            mListener->OnReregisterFinished(true, 0);
        break;
    case EVENT_REG_REREGISTER_FAILED:
        if (mListener != nullptr)
            mListener->OnReregisterFinished(false, 0);
        break;
    case EVENT_REG_REGISTER_STATE_UPDATE:
        // Do not handle this event now.
        break;
    default:
        LOG("CallService eventId=%d is not supported, skip it!", eventId);
        break;
    }
    if (!bArgs) {
        LOG("CallService arguments list of eventId=%d is incorrected, skip it!",
                eventId);
    }
}


//nsIIccCallback   for iccauthentication
NS_IMPL_ISUPPORTS(VowifiRegisterManager, nsIIccCallback)

/* void notifySuccess (); */
NS_IMETHODIMP VowifiRegisterManager::NotifySuccess()
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* void notifySuccessWithBoolean (in boolean aResult); */
NS_IMETHODIMP VowifiRegisterManager::NotifySuccessWithBoolean(bool aResult)
{
    LOG("Entering NotifySuccessWithBoolean");
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* void notifyGetCardLockRetryCount (in long aCount); */
NS_IMETHODIMP VowifiRegisterManager::NotifyGetCardLockRetryCount(int32_t aCount)
{
    LOG("Entering NotifyGetCardLockRetryCount");
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* void notifyRetrievedIccContacts ([array, size_is (aCount)] in nsIIccContact aContacts, in uint32_t aCount); */
NS_IMETHODIMP VowifiRegisterManager::NotifyRetrievedIccContacts(nsIIccContact **aContacts, uint32_t aCount)
{
    LOG("Entering NotifyRetrievedIccContacts");
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* void notifyUpdatedIccContact (in nsIIccContact aContact); */
NS_IMETHODIMP VowifiRegisterManager::NotifyUpdatedIccContact(nsIIccContact *aContact)
{
    LOG("Entering NotifyUpdatedIccContact");
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* void notifyError (in DOMString aErrorMsg); */
NS_IMETHODIMP VowifiRegisterManager::NotifyError(const nsAString & aErrorMsg)
{
    LOG("Entering NotifyError");
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* void notifyCardLockError (in DOMString aErrorMsg, in long aRetryCount); */
NS_IMETHODIMP VowifiRegisterManager::NotifyCardLockError(const nsAString & aErrorMsg, int32_t aRetryCount)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP
VowifiRegisterManager::NotifyAuthResponse(const nsAString & aData)
{
  LOG("Entering NotifyAuthResponse");

  RefPtr<VowifiInterfaceLayer> interfaceLayer =
          VowifiInterfaceLayer::GetInstance();
  if (!interfaceLayer) {
    return NS_ERROR_FAILURE;
  }
  string akaNonce = string(NS_LossyConvertUTF16toASCII(aData).get());
  LOG("the value of mAkaResponse is %s", akaNonce.c_str());
  Json::Value root, args;
  int subid = 0;
  root[JSON_GROUP_ID] = EVENT_GROUP_REG;
  root[JSON_EVENT_ID] = EVENT_REG_AUTH_CHALLENGE_RESP;
  root[JSON_EVENT_NAME] = EVENT_NAME_ONAUTH_REQ;
  VowifiArguments argHelp;
  argHelp.PushJsonArg(subid);
  argHelp.PushJsonArg(akaNonce);
  argHelp.GetJsonArgs(args);
  root[JSON_ARGS] = args;
  string noticeStr = root.toStyledString();
  LOG("NotifyAuthResponse the data going to be sent is %s", noticeStr.c_str());
  if (!interfaceLayer->mAdapterControl->SendToService(noticeStr)) {
      LOG("send EapAkaResponse error");
  }
  return NS_OK;
}

VowifiRegisterManager::~VowifiRegisterManager() {
}

} // namespace vowifiadapter
} // namespace dom
} // namespace mozilla
