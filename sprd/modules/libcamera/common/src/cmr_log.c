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
#define LOG_TAG "cmr_log"

#include <cutils/properties.h>
#include "cmr_types.h"
#include "cmr_log.h"

cmr_int g_isp_log_level = LEVEL_OVER_LOGW;
cmr_int g_oem_log_level = LEVEL_OVER_LOGD;
cmr_int g_sensor_log_level = LEVEL_OVER_LOGD;

void isp_init_log_level(void) {
    char prop[PROPERTY_VALUE_MAX];
    int val = 0;

    property_get("persist.sys.camera.isp.log", prop, "0");
    val = atoi(prop);
    if (0 < val)
        g_isp_log_level = val;
}

void oem_init_log_level(void) {
    char value[PROPERTY_VALUE_MAX];
    int val = 0;
    int turn_off_flag = 0;

    property_get("persist.sys.camera.hal.log", value, "0");
    val = atoi(value);
    if (0 < val)
        g_oem_log_level = val;

    // user verson camera log dont print >= LOGD
    property_get("ro.debuggable", value, "1");
    if (!strcmp(value, "0")) {
        g_oem_log_level = LEVEL_OVER_LOGI;
    }
}

void sensor_init_log_level(void) {
    char prop[PROPERTY_VALUE_MAX];
    int val = 0;

    property_get("persist.sys.camera.hal.log", prop, "0");
    val = atoi(prop);
    if (0 < val)
        g_sensor_log_level = val;
}
