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

#include "sprd_power.h"
#include "utils.h"
#include "common.h"

static int power_hint_enable = 1;

static bool is_in_interactive = false;
static bool sceenoff_configed = false;
static bool boost_for_benchmark = false;

static void power_set_interactive(struct power_module __unused *module, int on)
{
    struct sprd_power_module *pm = (struct sprd_power_module *)module;

    if (CC_UNLIKELY(power_hint_enable == 0)) return;

    if (CC_UNLIKELY(!pm->init_done)) {
        ALOGE("%s: power hint is not inited", __func__);
        return;
    }

    ENTER("%d", on);

    if (is_in_interactive != !!on) {
        is_in_interactive = !!on;

        pthread_mutex_lock(&pm->lock);
        if (power_mode == POWER_HINT_VENDOR_MODE_NORMAL) {
            if (is_in_interactive && sceenoff_configed) {
                boost(POWER_HINT_VENDOR_SCREEN_OFF, 0, 0, 0);
                sceenoff_configed = false;
            } else if (!is_in_interactive) {
                int is_powered = property_get_int32("sys.sprd.power.ispowered", 0);
                if (is_powered == 0) {
                    boost(POWER_HINT_VENDOR_SCREEN_OFF, 0, 1, 0);
                    sceenoff_configed = true;
                }
            }
        } else if (is_in_interactive) {
            boost(POWER_HINT_VENDOR_SCREEN_OFF, 0, 0, 0);
            usleep(60000);
            boost(POWER_HINT_VENDOR_SCREEN_ON, 0, 1, 0);
        } else {
            boost(POWER_HINT_VENDOR_SCREEN_ON, 0, 0, 0);
            usleep(60000);
            boost(POWER_HINT_VENDOR_SCREEN_OFF, 0, 1, 0);
        }
        pthread_mutex_unlock(&pm->lock);
    }
    EXIT("%d", on);
}

static void handle_power_hint_vendor(int value)
{
    int subtype = 0;
    int enable = 0;

    enable = value>0? 1: 0;
    subtype = abs(value) & 0xff;

    if (subtype == 0) {
        ALOGE("%s: subtype=0 is invalid value", __func__);
        return;
    }

    ALOGD("%s: hint id subtype: 0x%08x", __func__, subtype);
    boost(POWER_HINT_VENDOR, subtype, enable, 0);
}

static void handle_power_mode_switch(int mode, int enable, int value)
{
    int ret = -1;

    if (CC_UNLIKELY((power_mode == mode && enable == 1) || (power_mode != mode && enable == 0)))
        return;

    if (enable) {
        ALOGD("switch mode: 0x%08x -> 0x%08x", power_mode, mode);
    } else {
        ALOGD("switch mode: 0x%08x -> 0x%08x", power_mode, POWER_HINT_VENDOR_MODE_NORMAL);
    }

    if (update_mode(mode, enable)) {
        if (power_mode == POWER_HINT_VENDOR_MODE_NORMAL)
            sceenoff_configed = false;

        if (is_in_interactive) {
            boost(POWER_HINT_VENDOR_SCREEN_ON, 0, 1, 0);
        } else {
            if (power_mode == POWER_HINT_VENDOR_MODE_NORMAL) {
                int is_powered = property_get_int32("sys.sprd.power.ispowered", 0);
                if (is_powered == 0) {
                    boost(POWER_HINT_VENDOR_SCREEN_OFF, 0, 1, 0);
                    sceenoff_configed = true;
                }
            } else {
                boost(POWER_HINT_VENDOR_SCREEN_OFF, 0, 1, 0);
            }
        }
    }
}

static void sprd_power_hint(struct power_module *module, power_hint_t hint,
                             void *data)
{
    struct sprd_power_module *pm = (struct sprd_power_module *)module;
    static bool is_launching = false;

    if (CC_UNLIKELY(power_hint_enable == 0)) return;

    pthread_mutex_lock(&pm->lock);
    if (CC_UNLIKELY(!pm->init_done)) {
        pthread_mutex_unlock(&pm->lock);
        ALOGE("%s: power hint is not inited", __func__);
        return;
    }

    // Do not support boost int non normal mode
    if ((power_mode != POWER_HINT_VENDOR_MODE_NORMAL)
        && (hint < POWER_HINT_VENDOR_MODE_NORMAL || hint > POWER_HINT_VENDOR_SCREEN_ON)) {
        pthread_mutex_unlock(&pm->lock);
        return;
    }

    switch (hint) {
        case POWER_HINT_INTERACTION:
        {
            /*
             * 23       15          0
             * +---------+----------+
             * |  subtype|  duration|
             * +---------+----------+
             */
            int duration = 0;
            int action_subtype = 0;

            if (data != NULL) {
                duration = *((int*)data) & 0xffff;
                action_subtype = (*((int*)data) & 0xff0000) >> 16;
            }

            if (duration < BOOST_DURATION_DEFAULT || duration > BOOST_DURATION_MAX)
                duration = BOOST_DURATION_DEFAULT;

            if (data != NULL) {
                ALOGD("POWER_HINT_INTERACTION: data=0x%x, subtype=%d, duration=%d",
                    *((int*)data), action_subtype, duration);
            } else {
                ALOGD("POWER_HINT_INTERACTION: data=0x00, subtype=%d, duration=%d",
                    action_subtype, duration);
            }

            if (!is_in_interactive && action_subtype == INTERACTION_SUBTYPE_LAUNCH) {
                ALOGE("interactive=false, don't do LAUNCH boost!!!");
                break;
            }

            boost(POWER_HINT_INTERACTION, action_subtype, 1, duration);
            break;
        }
        case POWER_HINT_LAUNCH:
        {
            bool launch = (data != NULL)? true: false;

            if (!is_in_interactive) {
                ALOGD("Screenoff don't do LAUNCH boost!!!");
                break;
            }

            if (is_launching == launch) break;

            is_launching = launch;
            boost(POWER_HINT_LAUNCH, 0, (is_launching? 1: 0), 0);
            break;
        }
        case POWER_HINT_VSYNC:
            break;
        case POWER_HINT_SUSTAINED_PERFORMANCE:
            break;
        case POWER_HINT_VR_MODE:
            break;
        case POWER_HINT_VIDEO_DECODE:
            break;
        case POWER_HINT_VIDEO_ENCODE:
            if (data != NULL) {
                int state =  *((int*)data) & 0xffff;
                ALOGE("POWER_HINT_VIDEO_ENCODE: %d", state);

                if (state == 1) {
                    /* Video encode started */
                    boost(hint, 0, 1, 0);
                } else if (state == 0) {
                    /* Video encode stopped */
                    boost(hint, 0, 0, 0);
                }
            }
            break;
        case POWER_HINT_VENDOR:
            if (data == NULL) break;

            handle_power_hint_vendor(*((int*)data));
            break;
        case POWER_HINT_LOW_POWER:
        case POWER_HINT_VENDOR_MODE_NORMAL:
        case POWER_HINT_VENDOR_MODE_LOW_POWER:
        case POWER_HINT_VENDOR_MODE_POWER_SAVE:
        case POWER_HINT_VENDOR_MODE_ULTRA_POWER_SAVE:
        case POWER_HINT_VENDOR_MODE_PERFORMANCE:
            if (((power_mode == hint) && (data != NULL)) || ((power_mode != hint) && (data == NULL)))
                break;

            handle_power_mode_switch(hint, (data != NULL)? 1: 0, 0);
            break;
        default:
            ALOGE("%s: Power hint type 0x%08x is not supported!!!", __func__, hint);
            break;
    }

    pthread_mutex_unlock(&pm->lock);
}

static void power_init(struct power_module __unused *module) {

    struct sprd_power_module *pm = (struct sprd_power_module *)module;

    power_hint_enable = property_get_int32(POWER_HINT_ENABLE_PROP, 1);
    if (CC_UNLIKELY(power_hint_enable == 0)) return;

    if (CC_UNLIKELY(pm == NULL || pm->init_done)) return;

    pthread_mutex_lock(&pm->lock);
    // Read config file
    if (config_read() == 0) {
        pthread_mutex_unlock(&pm->lock);
        return;
    }

    // Must at the bottom
    start_thread_for_timing_request(module);
    pm->init_done = true;
    pthread_mutex_unlock(&pm->lock);
}

struct hw_module_methods_t power_module_methods = {
    .open = NULL,
};

struct sprd_power_module HAL_MODULE_INFO_SYM = {
    .base= {
        .common = {
            .tag = HARDWARE_MODULE_TAG,
            .module_api_version = POWER_MODULE_API_VERSION_0_2,
            .hal_api_version = HARDWARE_HAL_API_VERSION,
            .id = POWER_HARDWARE_MODULE_ID,
            .name = "Spreadtrum Power HAL",
            .author = "The Android Open Source Project",
            .methods = &power_module_methods,
        },

        .init = power_init,
        .setInteractive = power_set_interactive,
        .powerHint = sprd_power_hint,
    },


    .init_done = false,
    .lock = PTHREAD_MUTEX_INITIALIZER,
};
