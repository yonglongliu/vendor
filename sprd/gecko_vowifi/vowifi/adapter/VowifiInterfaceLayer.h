/************************************************************************

        Copyright (c) 2005-2017 by Spreadtrum, Inc.
                       All rights reserved.

     This software is confidential and proprietary to Spreadtrum,
     Inc. No part of this software may be reproduced, stored, transmitted,
     disclosed or used in any form or by any means other than as expressly
     provided by the written license agreement between Spreadtrum and its
     licensee.

************************************************************************/

#ifndef mozilla_dom_vowifiadapter_VowifiInterfaceLayer_h
#define mozilla_dom_vowifiadapter_VowifiInterfaceLayer_h


#include "nsIVowifiInterfaceLayer.h"
#include "mozilla/StaticPtr.h"
#include <android/log.h>
#include "VowifiSecurityManager.h"
#include "VowifiRegisterManager.h"
#include "VowifiCallManager.h"
#include "VowifiAdapterCommon.h"
#include "nsServiceManagerUtils.h"
#include "mozilla/Logging.h"
#include "VowifiAdapterControl.h"
#include "nsIRadioInterfaceLayer.h"
#include "nsIGonkTelephonyService.h"

#ifndef nsCOMPtr_h___
#include "nsCOMPtr.h"
#endif

namespace mozilla {
namespace dom {
namespace vowifiadapter {


#define VOWIFIINTERFACELAYER_CLASSNAME "VowifiInterfaceLayer"

class VowifiInterfaceLayer : public nsIVowifiInterfaceLayer

{
public:
    NS_DECL_ISUPPORTS NS_DECL_NSIVOWIFIINTERFACELAYER
    static already_AddRefed<VowifiInterfaceLayer> GetInstance();
    VowifiInterfaceLayer();
    VowifiAdapterControl* mAdapterControl = nullptr;
private:
  virtual ~VowifiInterfaceLayer();


};


} // namespace vowifiadapter
} // namespace dom
} // namespace mozilla
#endif //mozilla_dom_vowifiadapter_VowifiInterfaceLayer_h
