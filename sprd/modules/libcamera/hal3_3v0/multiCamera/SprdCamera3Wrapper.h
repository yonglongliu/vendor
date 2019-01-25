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
#ifndef SPRDCAMERAWRAPPER_H_HEADER
#define SPRDCAMERAWRAPPER_H_HEADER

#include <stdlib.h>
#include <dlfcn.h>
#include <utils/Log.h>
#include <utils/Errors.h>
#include <utils/List.h>
#include <utils/Mutex.h>
#include <utils/Thread.h>
#include <cutils/properties.h>
#include <hardware/camera3.h>
#include <hardware/camera.h>
#include <system/camera.h>
#include <sys/mman.h>
#include "../SprdCamera3HWI.h"
#ifdef CONFIG_STEREOVIDEO_SUPPORT
#include "SprdCamera3StereoVideo.h"
#endif
#ifdef CONFIG_RANGEFINDER_SUPPORT
#include "SprdCamera3RangeFinder.h"
#endif
#ifdef CONFIG_STEREOCAPUTRE_SUPPORT
#include "SprdCamera3Capture.h"
#endif
#include "SprdDualCamera3Tuning.h"

namespace sprdcamera {

class SprdCamera3Wrapper {
  public:
    SprdCamera3Wrapper();
    virtual ~SprdCamera3Wrapper();
    static void getCameraWrapper(SprdCamera3Wrapper **pWrapper);
    int cameraDeviceOpen(__unused const struct hw_module_t *module,
                         const char *id, struct hw_device_t **hw_device);
    int getCameraInfo(int camera_id, struct camera_info *info);

  private:
#ifdef CONFIG_STEREOVIDEO_SUPPORT
    SprdCamera3StereoVideo *mStereoVideo;
#endif
#ifdef CONFIG_RANGEFINDER_SUPPORT
    SprdCamera3RangeFinder *mRangeFinder;
#endif
#ifdef CONFIG_STEREOCAPUTRE_SUPPORT
    SprdCamera3Capture *mCapture;
#endif
    SprdDualCamera3Tuning *mTCam;
};
};
#endif
