/*
 * Copyright (C) 2010 The Android Open Source Project
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

/*
 *  SprdDisplayCore: surpport several display framework, such as
 *                    traditional Frame buffer device, Android ADF,
 *                    and the DRM in the future.
 *  It pass display data from HWC to Display driver.
 *  Author: zhongjun.chen@spreadtrum.com
*/

#ifndef _SPRD_DISPLAY_CORE_H_
#define _SPRD_DISPLAY_CORE_H_

#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <hardware/hwcomposer.h>
#include <cutils/log.h>

#include "SprdHWLayer.h"

using namespace android;

#define DEFAULT_DISPLAY_TYPE_NUM 3

typedef struct {
  int releaseFenceFd;
  int retiredFenceFd;
} DisplayTrack;

class SprdDisplayCore {
 public:
  SprdDisplayCore()
      : mInitFlag(false),
        mActiveContextCount(0),
        mLayerCount(0),
        mEventProcs(NULL) {}

  virtual ~SprdDisplayCore() {
    if (mEventProcs) {
      free(mEventProcs);
      mEventProcs = NULL;
    }

    mInitFlag = false;
    mActiveContextCount = 0;
    mLayerCount = 0;
  }

  /*
   *  Init SprdDisplayCore, open interface, and get some configs.
   */
  virtual bool Init() {
    mEventProcs = (hwc_procs_t const **)malloc(sizeof(hwc_procs_t));
    if (mEventProcs == NULL) {
      mInitFlag = false;
    } else {
      mInitFlag = true;
    }

    return mInitFlag;
  }

  void SetEventProcs(hwc_procs_t const *procs) { *mEventProcs = procs; }

  inline hwc_procs_t const **getEventProcs() { return mEventProcs; }

  virtual int AddFlushData(int DisplayType, SprdHWLayer **list,
                           int LayerCount) = 0;

  virtual int PostDisplay(DisplayTrack *tracker) = 0;

  virtual int QueryDisplayInfo(uint32_t *DisplayNum) = 0;

  virtual int GetConfigs(int DisplayType, uint32_t *Configs,
                         size_t *NumConfigs) = 0;

  virtual int GetConfigAttributes(int DisplayType, uint32_t Config,
                                  const uint32_t *attributes,
                                  int32_t *values) = 0;

  virtual int EventControl(int DisplayType, int event, bool enabled) = 0;

  virtual int Blank(int DisplayType, bool enabled) = 0;

  virtual int Dump(char *buffer) = 0;

 protected:
  bool mInitFlag;
  int32_t mActiveContextCount;
  int32_t mLayerCount;
  hwc_procs_t const **mEventProcs;
};

class SprdEventHandle {
 public:
  SprdEventHandle() {}
  ~SprdEventHandle() {}

  static void SprdHandleVsyncReport(void *data, int disp, uint64_t timestamp) {
    SprdDisplayCore *core = static_cast<SprdDisplayCore *>(data);
    if (core == NULL) {
      ALOGE("SprdHandleVsyncReport cannot get the SprdDisplayCore reference");
      return;
    }

    const hwc_procs_t *EventProcs = *(core->getEventProcs());
    if (EventProcs && EventProcs->vsync) {
      static int count = 0;
      EventProcs->vsync(EventProcs, disp, timestamp);
    }
  }

  static void SprdHandleHotPlugReport(void *data, int disp, bool connected) {
    SprdDisplayCore *core = static_cast<SprdDisplayCore *>(data);
    if (core == NULL) {
      ALOGE("SprdHandleHotPlugReport cannot get the SprdDisplayCore reference");
      return;
    }

    const hwc_procs_t *EventProcs = *(core->getEventProcs());
    if (EventProcs && EventProcs->hotplug) {
      EventProcs->hotplug(EventProcs, disp, connected);
    }
  }

  static void SprdHandleCustomReport(void *data, int disp,
                                     struct adf_event *event) {
    PARAM_IGNORE(data);
    PARAM_IGNORE(disp);
    PARAM_IGNORE(event);
  }
};

#endif  // #ifndef _SPRD_DISPLAY_CORE_H_
