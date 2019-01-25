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

#define LOG_TAG "SprdCamera2"
#include <utils/Log.h>
#include "system/camera_metadata.h"
#include "SprdCameraHardwareConfig2.h"
#include "SprdOEMCamera.h"

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

int androidSceneModeToDrvMode(
    camera_metadata_enum_android_control_scene_mode_t androidScreneMode,
    int8_t *convertDrvMode) {
    int ret = 0;

    ALOGD("%s sceneMode=%d", __FUNCTION__, androidScreneMode);
    switch (androidScreneMode) {
    case ANDROID_CONTROL_SCENE_MODE_UNSUPPORTED:
        *convertDrvMode = CAMERA_SCENE_MODE_AUTO;
        break;

    case ANDROID_CONTROL_SCENE_MODE_ACTION:
        *convertDrvMode = CAMERA_SCENE_MODE_ACTION;
        break;

    case ANDROID_CONTROL_SCENE_MODE_NIGHT:
        *convertDrvMode = CAMERA_SCENE_MODE_NIGHT;
        break;

    case ANDROID_CONTROL_SCENE_MODE_PORTRAIT:
        *convertDrvMode = CAMERA_SCENE_MODE_PORTRAIT;
        break;

    case ANDROID_CONTROL_SCENE_MODE_LANDSCAPE:
        *convertDrvMode = CAMERA_SCENE_MODE_LANDSCAPE;
        break;

    case ANDROID_CONTROL_SCENE_MODE_HDR:
        *convertDrvMode = CAMERA_SCENE_MODE_HDR;
        break;

    default:
        *convertDrvMode = CAMERA_SCENE_MODE_AUTO;
        break;
    }
    return ret;
}

int androidAfModeToDrvAfMode(
    camera_metadata_enum_android_control_af_mode_t androidAfMode,
    int8_t *convertDrvMode) {
    int ret = 0;

    ALOGD("%s afMode=%d", __FUNCTION__, androidAfMode);

    switch (androidAfMode) {
    case ANDROID_CONTROL_AF_MODE_AUTO:
        *convertDrvMode = CAMERA_FOCUS_MODE_AUTO;
        break;

    case ANDROID_CONTROL_AF_MODE_MACRO:
        *convertDrvMode = CAMERA_FOCUS_MODE_MACRO;
        break;

    case ANDROID_CONTROL_AF_MODE_EDOF:
    case ANDROID_CONTROL_AF_MODE_OFF:
        *convertDrvMode = CAMERA_FOCUS_MODE_INFINITY; // OFF
        break;

    case ANDROID_CONTROL_AF_MODE_CONTINUOUS_VIDEO:
        *convertDrvMode = CAMERA_FOCUS_MODE_AUTO;
        break;

    case ANDROID_CONTROL_AF_MODE_CONTINUOUS_PICTURE:
        *convertDrvMode = CAMERA_FOCUS_MODE_CAF;
        break;

    default:
        *convertDrvMode = CAMERA_FOCUS_MODE_INFINITY;
        break;
    }
    return ret;
}

int androidFlashModeToDrvFlashMode(
    camera_metadata_enum_android_flash_mode_t androidFlashMode,
    int8_t *convertDrvMode) {
    int ret = 0;

    ALOGD("%s flashMode=%d", __FUNCTION__, androidFlashMode);

    switch (androidFlashMode) {
    case ANDROID_FLASH_MODE_TORCH:
        *convertDrvMode = CAMERA_FLASH_MODE_TORCH;
        break;

    case ANDROID_FLASH_MODE_OFF:
    case ANDROID_FLASH_MODE_SINGLE:
    default:
        break;
    }
    return ret;
}

int androidAeModeToDrvAeMode(
    camera_metadata_enum_android_control_ae_mode_t androidAeMode,
    int8_t *convertDrvMode) {
    int ret = 0;

    ALOGD("%s aeMode=%d", __FUNCTION__, androidAeMode);
    switch (androidAeMode) {
    case ANDROID_CONTROL_AE_MODE_OFF:
        *convertDrvMode = -1;
        ret = -1;
        break;

    case ANDROID_CONTROL_AE_MODE_ON:
        *convertDrvMode = CAMERA_FLASH_MODE_OFF;
        break;

    case ANDROID_CONTROL_AE_MODE_ON_AUTO_FLASH:
        *convertDrvMode = CAMERA_FLASH_MODE_AUTO;
        break;

    case ANDROID_CONTROL_AE_MODE_ON_ALWAYS_FLASH:
        *convertDrvMode = CAMERA_FLASH_MODE_ON;
        break;

    default:
        *convertDrvMode = CAMERA_FLASH_MODE_OFF;
        break;
    }
    return ret;
}

int androidAwbModeToDrvAwbMode(
    camera_metadata_enum_android_control_awb_mode_t androidAwbMode,
    int8_t *convertDrvMode) {
    int ret = 0;

    ALOGD("%s awbMode=%d", __FUNCTION__, androidAwbMode);

    switch (androidAwbMode) {
    case ANDROID_CONTROL_AWB_MODE_OFF:
        *convertDrvMode = CAMERA_WB_MAX;
        break;

    case ANDROID_CONTROL_AWB_MODE_AUTO:
        *convertDrvMode = CAMERA_WB_AUTO;
        break;

    case ANDROID_CONTROL_AWB_MODE_INCANDESCENT:
        *convertDrvMode = CAMERA_WB_INCANDESCENT;
        break;

    case ANDROID_CONTROL_AWB_MODE_FLUORESCENT:
        *convertDrvMode = CAMERA_WB_FLUORESCENT;
        break;

    case ANDROID_CONTROL_AWB_MODE_DAYLIGHT:
        *convertDrvMode = CAMERA_WB_DAYLIGHT;
        break;

    case ANDROID_CONTROL_AWB_MODE_CLOUDY_DAYLIGHT:
        *convertDrvMode = CAMERA_WB_CLOUDY_DAYLIGHT;
        break;
    default:
        *convertDrvMode = CAMERA_WB_AUTO;
        break;
    }
    return ret;
}

int androidParametTagToDrvParaTag(uint32_t androidParaTag,
                                  enum camera_param_type *convertDrvTag) {
    int ret = 0;

    switch (androidParaTag) {
    case ANDROID_CONTROL_SCENE_MODE:
        *convertDrvTag = CAMERA_PARAM_SCENE_MODE;
        break;

    case ANDROID_CONTROL_AWB_MODE:
        *convertDrvTag = CAMERA_PARAM_WB;
        break;

    case ANDROID_SCALER_CROP_REGION:
        *convertDrvTag = CAMERA_PARAM_ZOOM;
        break;

    case ANDROID_CONTROL_AE_EXPOSURE_COMPENSATION:
        *convertDrvTag = CAMERA_PARAM_EXPOSURE_COMPENSATION;
        break;

    case ANDROID_CONTROL_AF_MODE:
        *convertDrvTag = CAMERA_PARAM_AF_MODE;
        break;

    case ANDROID_SPRD_BRIGHTNESS:
        *convertDrvTag = CAMERA_PARAM_BRIGHTNESS;
        break;

    case ANDROID_CONTROL_AE_MODE:
    case ANDROID_FLASH_MODE:
        *convertDrvTag = CAMERA_PARAM_FLASH;
        break;

    default:
        break;
    }
    return ret;
}
