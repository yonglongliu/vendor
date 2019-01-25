/*
 * Copyright (C) 2012 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef __FB_LOG_H__
#define __FB_LOG_H__
//#include <stdio.h>
//#include <stdlib.h>
//#include <cutils/log.h>
//#define LOG_TAG "SW_ISP"
#define DEBUG_STR     "%d, %s: "
//#define DEBUG_ARGS    __LINE__,__FUNCTION__

#define SWISP_LOGE(format,...) \
	ALOGE(DEBUG_STR format, DEBUG_ARGS, ##__VA_ARGS__)

#define SWISP_LOGW(format,...) \
        ALOGW(DEBUG_STR format, DEBUG_ARGS, ##__VA_ARGS__)

#define SWISP_LOGI(format,...) \
	ALOGI(DEBUG_STR format, DEBUG_ARGS, ##__VA_ARGS__)

#define SWISP_LOGD(format,...) \
	ALOGD(DEBUG_STR format, DEBUG_ARGS, ##__VA_ARGS__)

#endif
