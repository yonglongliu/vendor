// 
// Spreadtrum Auto Tester
//
// anli   2012-11-09
//
#ifndef _AUTOTEST_20121109_H__
#define _AUTOTEST_20121109_H__

#ifdef LOG_TAG
#undef LOG_TAG
#endif

#define  LOG_TAG "BBAT"
#include <cutils/log.h>

#include <assert.h>
#include <errno.h>
#include <string.h>
#include "eng_modules.h"
#include "eng_diag.h"

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#define LOGI ALOGI
#define LOGD ALOGD
#define LOGW ALOGW
#define LOGE ALOGE
#define FUN_ENTER             LOGD("[ %s ++ ]\n", __FUNCTION__)
#define FUN_EXIT              LOGD("[ %s -- ]\n", __FUNCTION__)

//结构体eng_callback定义在vendor/sprd/proprietories-source/engmode/eng_modules.h文件中
//如下结构体MSG_HEAD_T;定义在vendor/sprd/proprietories-source/engmode/eng_diag.h文件中

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // _DEBUG_20121109_H__
