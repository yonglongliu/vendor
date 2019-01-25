#ifndef __SPRD_VOWIFI_PORTING_LOG_H__
#define __SPRD_VOWIFI_PORTING_LOG_H__

#include <android/log.h>


BEGIN_VOWIFI_NAMESPACE

#ifdef LOG
#undef LOG
#endif

#define LOGD(TAG, args...)  __android_log_print(ANDROID_LOG_DEBUG, TAG, ## args)

#define LOGW(TAG, args...)  __android_log_print(ANDROID_LOG_WARN, TAG, ## args)

#define LOGE(TAG, args...)  __android_log_print(ANDROID_LOG_ERROR, TAG, ## args)

END_VOWIFI_NAMESPACE

#endif
