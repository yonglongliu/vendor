/* Copyright (c) 2016, The Linux Foundataion. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *     * Neither the name of The Linux Foundation nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */
#define LOG_TAG "Cam3Wrapper"
#include "SprdCamera3Wrapper.h"

using namespace android;
namespace sprdcamera {

SprdCamera3Wrapper::SprdCamera3Wrapper() {
#ifdef CONFIG_RANGEFINDER_SUPPORT
    SprdCamera3RangeFinder::getCameraRangeFinder(&mRangeFinder);
#endif
#ifdef CONFIG_STEREOVIDEO_SUPPORT
    SprdCamera3StereoVideo::getCameraMuxer(&mStereoVideo);
#endif
#ifdef CONFIG_STEREOPREVIEW_SUPPORT
    SprdCamera3StereoPreview::getCameraMuxer(&mStereoPreview);
#endif
#ifdef CONFIG_STEREOCAPUTRE_SUPPORT
    SprdCamera3Capture::getCameraCapture(&mCapture);
#endif
#ifdef CONFIG_BLUR_SUPPORT
    SprdCamera3Blur::getCameraBlur(&mBlur);
#endif
#ifdef CONFIG_SIDEBYSIDE_SUPPORT
    SprdCamera3SideBySideCamera::getCameraSidebyside(&mSidebyside);
#endif
#ifdef CONFIG_COVERED_SENSOR
    SprdCamera3SelfShot::getCameraMuxer(&mSelfShot);
    SprdCamera3PageTurn::getCameraMuxer(&mPageturn);
#endif
}

SprdCamera3Wrapper::~SprdCamera3Wrapper() {}

void SprdCamera3Wrapper::getCameraWrapper(SprdCamera3Wrapper **pFinder) {
    *pFinder = NULL;
    *pFinder = new SprdCamera3Wrapper();
    HAL_LOGV("gWrapper: %p ", *pFinder);
    return;
}

int SprdCamera3Wrapper::cameraDeviceOpen(
    __unused const struct hw_module_t *module, const char *id,
    struct hw_device_t **hw_device) {

    int rc = NO_ERROR;

    HAL_LOGI("id=%d", atoi(id));

    switch (atoi(id)) {
#ifdef CONFIG_STEREOVIDEO_SUPPORT
    case MODE_3D_VIDEO:
        rc = mStereoVideo->camera_device_open(module, id, hw_device);
        return rc;
#endif
#ifdef CONFIG_RANGEFINDER_SUPPORT
    case MODE_RANGE_FINDER:
        rc = mRangeFinder->camera_device_open(module, id, hw_device);
        return rc;
#endif
#ifdef CONFIG_STEREOCAPUTRE_SUPPORT
    case MODE_3D_CAPTURE:
        rc = mCapture->camera_device_open(module, id, hw_device);
        return rc;
#endif
#ifdef CONFIG_STEREOPREVIEW_SUPPORT
    case MODE_3D_PREVIEW:
        rc = mStereoPreview->camera_device_open(module, id, hw_device);
        return rc;
#endif
#ifdef CONFIG_SIDEBYSIDE_SUPPORT
    case MODE_BLUR:
    case MODE_BLUR_FRONT:
        rc = mSidebyside->camera_device_open(module, id, hw_device);
        return rc;
#else
#ifdef CONFIG_BLUR_SUPPORT
    case MODE_BLUR:
    case MODE_BLUR_FRONT:
        rc = mBlur->camera_device_open(module, id, hw_device);
        return rc;
#endif
#endif
#ifdef CONFIG_COVERED_SENSOR
    case MODE_SELF_SHOT:
        rc = mSelfShot->camera_device_open(module, id, hw_device);
        return rc;
    case MODE_PAGE_TURN:
        rc = mPageturn->camera_device_open(module, id, hw_device);
        return rc;
#endif
    default:

        HAL_LOGE("cameraId:%d not supported yet!", atoi(id));
        return -EINVAL;
    }

    return rc;
}

int SprdCamera3Wrapper::getCameraInfo(__unused int camera_id,
                                      struct camera_info *info) {

    int rc = NO_ERROR;

    HAL_LOGI("id= %d", camera_id);

    switch (camera_id) {
#ifdef CONFIG_STEREOVIDEO_SUPPORT
    case MODE_3D_VIDEO:
        rc = mStereoVideo->get_camera_info(camera_id, info);
        return rc;
#endif
#ifdef CONFIG_RANGEFINDER_SUPPORT
    case MODE_RANGE_FINDER:
        rc = mRangeFinder->get_camera_info(camera_id, info);
        return rc;
#endif
#ifdef CONFIG_STEREOCAPUTRE_SUPPORT
    case MODE_3D_CAPTURE:
        rc = mCapture->get_camera_info(camera_id, info);
        return rc;
#endif
#ifdef CONFIG_STEREOPREVIEW_SUPPORT
    case MODE_3D_PREVIEW:
        rc = mStereoPreview->get_camera_info(camera_id, info);
        return rc;
#endif
#ifdef CONFIG_SIDEBYSIDE_SUPPORT
    case MODE_BLUR:
    case MODE_BLUR_FRONT:
        rc = mSidebyside->get_camera_info(camera_id, info);
        return rc;
#else
#ifdef CONFIG_BLUR_SUPPORT
    case MODE_BLUR:
    case MODE_BLUR_FRONT:
        rc = mBlur->get_camera_info(camera_id, info);
        return rc;
#endif
#endif
#ifdef CONFIG_COVERED_SENSOR
    case MODE_SELF_SHOT:
        rc = mSelfShot->get_camera_info(camera_id, info);
        return rc;
    case MODE_PAGE_TURN:
        rc = mPageturn->get_camera_info(camera_id, info);
        return rc;
#endif
    default:

        HAL_LOGE("cameraId:%d not supported yet!", camera_id);
        return -EINVAL;
    }

    return rc;
}
};
