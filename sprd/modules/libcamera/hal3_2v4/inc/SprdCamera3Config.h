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
#ifndef _SPRD_CAMERA3_CONFIG_H_
#define _SPRD_CAMERA3_CONFIG_H_
#include "cmr_oem.h"

/* Effect type, used for CAMERA_PARM_EFFECT */
enum {
    CAMERA_EFFECT_NONE = 0,
    CAMERA_EFFECT_MONO,
    CAMERA_EFFECT_RED,
    CAMERA_EFFECT_GREEN,
    CAMERA_EFFECT_BLUE,
    CAMERA_EFFECT_YELLOW,
    CAMERA_EFFECT_NEGATIVE,
    CAMERA_EFFECT_SEPIA,
    CAMERA_EFFECT_MAX
};

#endif //_SPRD_CAMERA_HARDWARE_CONFIG_H_
