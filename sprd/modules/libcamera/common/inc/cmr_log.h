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

#ifndef _CMR_LOG_H_
#define _CMR_LOG_H_

/*
 * LEVEL_OVER_LOGE - only show ALOGE, err log is always show
 * LEVEL_OVER_LOGW - show ALOGE and ALOGW
 * LEVEL_OVER_LOGI - show ALOGE, ALOGW and ALOGI
 * LEVEL_OVER_LOGD - show ALOGE, ALOGW, ALOGI and ALOGD
 * LEVEL_OVER_LOGV - show ALOGE, ALOGW, ALOGI and ALOGD, ALOGV
 */
enum {
    LEVEL_OVER_LOGE = 1,
    LEVEL_OVER_LOGW,
    LEVEL_OVER_LOGI,
    LEVEL_OVER_LOGD,
    LEVEL_OVER_LOGV,
};

#define DEBUG_STR "%d, %s: "
#define DEBUG_ARGS __LINE__, __FUNCTION__

extern cmr_int g_isp_log_level;
extern cmr_int g_oem_log_level;
extern cmr_int g_sensor_log_level;

#ifndef WIN32
#define ISP_LOGE(format, ...) ALOGE(DEBUG_STR format, DEBUG_ARGS, ##__VA_ARGS__)
#define ISP_LOGW(format, ...)                                                  \
    ALOGW_IF(g_isp_log_level >= LEVEL_OVER_LOGW, DEBUG_STR format, DEBUG_ARGS, \
             ##__VA_ARGS__)
#define ISP_LOGI(format, ...)                                                  \
    ALOGI_IF(g_isp_log_level >= LEVEL_OVER_LOGI, DEBUG_STR format, DEBUG_ARGS, \
             ##__VA_ARGS__)
#define ISP_LOGD(format, ...)                                                  \
    ALOGD_IF(g_isp_log_level >= LEVEL_OVER_LOGD, DEBUG_STR format, DEBUG_ARGS, \
             ##__VA_ARGS__)
/* ISP_LOGV uses ALOGD_IF */
#define ISP_LOGV(format, ...)                                                  \
    ALOGD_IF(g_isp_log_level >= LEVEL_OVER_LOGV, DEBUG_STR format, DEBUG_ARGS, \
             ##__VA_ARGS__)
#else
#define ISP_LOGE printf
#define ISP_LOGW printf
#define ISP_LOGI printf
#define ISP_LOGD printf
#define ISP_LOGV printf
#endif

#define CMR_LOGE(format, ...) ALOGE(DEBUG_STR format, DEBUG_ARGS, ##__VA_ARGS__)
#define CMR_LOGW(format, ...)                                                  \
    ALOGW_IF(g_oem_log_level >= LEVEL_OVER_LOGW, DEBUG_STR format, DEBUG_ARGS, \
             ##__VA_ARGS__)
#define CMR_LOGI(format, ...)                                                  \
    ALOGI_IF(g_oem_log_level >= LEVEL_OVER_LOGI, DEBUG_STR format, DEBUG_ARGS, \
             ##__VA_ARGS__)
#define CMR_LOGD(format, ...)                                                  \
    ALOGD_IF(g_oem_log_level >= LEVEL_OVER_LOGD, DEBUG_STR format, DEBUG_ARGS, \
             ##__VA_ARGS__)
/* CMR_LOGV uses ALOGD_IF */
#define CMR_LOGV(format, ...)                                                  \
    ALOGD_IF(g_oem_log_level >= LEVEL_OVER_LOGV, DEBUG_STR format, DEBUG_ARGS, \
             ##__VA_ARGS__)

#define SENSOR_LOGE(format, ...)                                               \
    ALOGE(DEBUG_STR format, DEBUG_ARGS, ##__VA_ARGS__)
#define SENSOR_LOGW(format, ...)                                               \
    ALOGW_IF(g_sensor_log_level >= LEVEL_OVER_LOGW, DEBUG_STR format,          \
             DEBUG_ARGS, ##__VA_ARGS__)
#define SENSOR_LOGI(format, ...)                                               \
    ALOGI_IF(g_sensor_log_level >= LEVEL_OVER_LOGI, DEBUG_STR format,          \
             DEBUG_ARGS, ##__VA_ARGS__)
#define SENSOR_LOGD(format, ...)                                               \
    ALOGD_IF(g_sensor_log_level >= LEVEL_OVER_LOGD, DEBUG_STR format,          \
             DEBUG_ARGS, ##__VA_ARGS__)
/* SENSOR_LOGV uses ALOGD_IF */
#define SENSOR_LOGV(format, ...)                                               \
    ALOGD_IF(g_sensor_log_level >= LEVEL_OVER_LOGV, DEBUG_STR format,          \
             DEBUG_ARGS, ##__VA_ARGS__)
#define SENSOR_PRINT_ERR SENSOR_LOGE
#define SENSOR_PRINT_HIGH SENSOR_LOGI
#define SENSOR_PRINT SENSOR_LOGI
#define SENSOR_TRACE SENSOR_LOGI

void isp_init_log_level(void);
void oem_init_log_level(void);
void sensor_init_log_level(void);

#endif
