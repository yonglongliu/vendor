/************************************************************************

        Copyright (c) 2005-2017 by Spreadtrum, Inc.
                       All rights reserved.

     This software is confidential and proprietary to Spreadtrum,
     Inc. No part of this software may be reproduced, stored, transmitted,
     disclosed or used in any form or by any means other than as expressly
     provided by the written license agreement between Spreadtrum and its
     licensee.

************************************************************************/
#ifndef _VOWIFI_UTIL_LOG_H__
#define _VOWIFI_UTIL_LOG_H__
#include <android/log.h>
#include <utils/Log.h>
#include <string>

#ifdef LOG_DEBUG
#include <stdio.h>
#endif

using namespace std;

namespace mozilla {
namespace dom {
namespace vowifiadapter {

#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "[VoWiFiAdapter]"


#ifdef LOG_DEBUG

#ifdef LOGD
#undef LOGD
#endif
#define LOGD printf


#ifdef LOGE
#undef LOGE
#endif
#define LOGE printf


#ifdef LOGW
#undef LOGW
#endif
#define LOGW printf


#ifdef LOGI
#undef LOGI
#endif
#define LOGI printf


#else

#ifdef LOGD
#undef LOGD
#endif
#define LOGD ALOGD


#ifdef LOGE
#undef LOGE
#endif
#define LOGE ALOGE


#ifdef LOGW
#undef LOGW
#endif
#define LOGW ALOGW


#ifdef LOGI
#undef LOGI
#endif
#define LOGI ALOGI


#endif

string base64_encode(char* bytes_to_encode, unsigned int in_len);
string base64_decode(string & encoded_string);

} // namespace vowifiadapter
} // namespace dom
} // namespace mozilla
#endif
