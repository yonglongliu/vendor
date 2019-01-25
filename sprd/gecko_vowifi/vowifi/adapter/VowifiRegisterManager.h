/************************************************************************

        Copyright (c) 2005-2017 by Spreadtrum, Inc.
                       All rights reserved.

     This software is confidential and proprietary to Spreadtrum,
     Inc. No part of this software may be reproduced, stored, transmitted,
     disclosed or used in any form or by any means other than as expressly
     provided by the written license agreement between Spreadtrum and its
     licensee.

************************************************************************/

#ifndef mozilla_dom_vowifiadapter_VowifiRegisterManager_h
#define mozilla_dom_vowifiadapter_VowifiRegisterManager_h
#include "VowifiAdapterCommon.h"
#include <string>

namespace Json {
class Value;
}

namespace mozilla {
namespace dom {
namespace vowifiadapter {
class RegisterListener;
class SIMAccountInfo;
class VowifiAdapterControl;

class VowifiRegisterManager : public nsIIccCallback
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIICCCALLBACK
    VowifiRegisterManager(VowifiAdapterControl *ptr);
    VowifiRegisterManager(){};
    void RegisterListeners(RegisterListener* listener);
    void UnregisterListeners();
    void PrepareForLogin(SIMAccountInfo* simAccountInfo);
    void Login(bool isIPv4, string localIP, string pcscfIP);
    void Reregister(int networkType, string& networkInfo);
    void Deregister();
    bool ForceStop();
    int GetCurRegisterState();
    bool CliOpen(SIMAccountInfo* info);
    bool CliStart();
    bool CliUpdateSettings(SIMAccountInfo* info);
    void UpdateRegisterState(int newState);
    int GetVowifiNetworkType(int volteType);
    void EapAkaRequest(string akaNonce);
    void OnRegisterStateChanged(int eventId, Json::Value &args);
    void deleteRegisterObject(VowifiRegisterManager* registerObject)
    {
        delete registerObject;
    };
private:
    virtual ~VowifiRegisterManager();
    VowifiAdapterControl* mAdapterControl = nullptr;
    bool mLoginPrepared;
    int mRegisterState;
    RegisterListener* mListener = nullptr;
};

class RegisterListener {

public:
    /**
     * Refresh the registration result callback
     *
     * @param success true if success or false if failed.
     */
    RegisterListener() {
    }
    ;
    virtual void OnReregisterFinished(bool success, int errorCode);

    virtual void OnLoginFinished(bool success, int stateCode, int retryAfter);

    virtual void OnLogout(int stateCode) = 0;

    virtual void OnPrepareFinished(bool success, int errorCode);

    virtual void OnRegisterStateChanged(int newState);

    virtual ~RegisterListener() {
    }
    ;
};

string SIMAccountInfo::PROP_KEY_IMSI = "persist.sys.sprd.temp.imsi";

} // namespace vowifiadapter
} // namespace dom
} // namespace mozilla

#endif // mozilla_dom_vowifiadapter_VowifiRegisterManager_h
