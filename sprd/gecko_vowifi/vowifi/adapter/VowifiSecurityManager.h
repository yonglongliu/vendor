/************************************************************************

        Copyright (c) 2005-2017 by Spreadtrum, Inc.
                       All rights reserved.

     This software is confidential and proprietary to Spreadtrum,
     Inc. No part of this software may be reproduced, stored, transmitted,
     disclosed or used in any form or by any means other than as expressly
     provided by the written license agreement between Spreadtrum and its
     licensee.

************************************************************************/

#ifndef mozilla_dom_vowifiadapter_VowifiSecurityManager_h
#define mozilla_dom_vowifiadapter_VowifiSecurityManager_h
#include "VowifiAdapterCommon.h"
#include <string>

namespace Json{
    class Value;
}

namespace mozilla {
namespace dom {
namespace vowifiadapter {
class SecurityListener;
class VowifiAdapterControl;
class VowifiSecurityManager : public nsIIccCallback
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIICCCALLBACK
    VowifiSecurityManager(VowifiAdapterControl* adapterControl);
    VowifiSecurityManager(){};
    void RegisterListeners(SecurityListener* listener);
    void UnregisterListeners();
    int GetNetId();
    string  GetImsAddress();
    void Attach(SIMAccountInfo* simAccountInfo);
    void attachStopped(bool forHandover, int errorCode);
    void EapAkaRequest(string akaNonce);
    void Deattach(bool isHandover, bool isForceDeat=false);
    void ForceStop();
    bool SetIPVersion(int ipVersion);
    SecurityConfig* GetConfig();
    int GetCurSecurityState();
    void OnJsonCallback(int eventId, Json::Value &args);
    void SetVolteUsedLocalAddr(string imsRegAddr);
    string GetVolteUsedLocalAddr();
    SecurityConfig *mSecurityConfig;
    void deleteSecurityObject(VowifiSecurityManager* securityObject)
    {
        delete securityObject;
    };

    int mNetId = -1;
    int mSubId = 0;
private:
    virtual ~VowifiSecurityManager();
    VowifiAdapterControl* mAdapterControl = nullptr;
    int mState;
    SecurityListener* mListener = nullptr;
    string mAkaResponse;
    string mImsRegAddr;

};

class SecurityListener {
  public:      /**
         * Attachment process callback
         *
         * @param state:See S2bConfig related definitions
         */
        SecurityListener(){};
        virtual void OnProgress(int state);

        /**
         * Attachment success callback
         *
         * @param config:Contains the pcscf address, DNS address, IP address, and other information
         *            distribution
         */
        virtual void OnSuccessed();

        /**
         * Attachment failure callback
         *
         * @param reason:Enumeration of causes for failure, see S2bConfig related definitions
         */
        virtual void OnFailed(int reason);

        /**
         * Attached to stop callback, call stop IS2b () will trigger the callback
         */
        virtual void OnStopped(bool forHandover, int errorCode);

        virtual ~SecurityListener(){};
    };

} // namespace vowifiadapter
} // namespace dom
} // namespace mozilla

#endif // mozilla_dom_vowifiadapter_VowifiSecurityManager_h
