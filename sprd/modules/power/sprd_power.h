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

#ifndef INCLUDE_SPRD_POWER_H
#define INCLUDE_SPRD_POWER_H

#include <dirent.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <linux/time.h>
#include <stdbool.h>
#include <cutils/properties.h>
#include <cutils/compiler.h>
#include <math.h>
#include <sys/prctl.h>
#include <sys/resource.h>

#define LOG_TAG "PowerHAL"
#include <utils/Log.h>

#include <hardware/hardware.h>
#include <hardware/power.h>

// POWER_HINT_INTERACTION subtype
#define INTERACTION_SUBTYPE_OTHER            0
#define INTERACTION_SUBTYPE_TOUCH            1
#define INTERACTION_SUBTYPE_LAUNCH           2
#define INTERACTION_SUBTYPE_FLING            3
#define INTERACTION_SUBTYPE_BUTTON           4

#define BOOST_DURATION_DEFAULT               500
#define BOOST_DURATION_MAX                   5000

#define POWER_HINT_ENABLE_PROP               "persist.sys.power.hint"

// For POWER_HINT_VIDEO_ENCODE
#define STATE_ON                             "state=1"
#define STATE_OFF                            "state=0"
#define STATE_HDR_ON                         "state=2"
#define STATE_HDR_OFF                        "state=3"

struct sprd_power_module {
    struct power_module base;
    pthread_mutex_t lock;
    bool init_done;
};

#endif
