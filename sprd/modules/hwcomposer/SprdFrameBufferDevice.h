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
 *  SprdFrameBufferDevice.h:
 *  The traditional Frame buffer device,
 *  It pass display data from HWC to Display driver.
 *  Author: zhongjun.chen@spreadtrum.com
 */

#ifndef SPRD_FRAME_BUFFER_DEVICE_H_
#define SPRD_FRAME_BUFFER_DEVICE_H_

#include <fcntl.h>
#include <errno.h>

#include <hardware/hardware.h>
#include <hardware/hwcomposer.h>
#include <hardware/gralloc.h>
#include <utils/RefBase.h>
#include <cutils/log.h>


#include "SprdVsyncEvent.h"
#include "SprdDisplayDevice.h"
#include "SprdHWLayer.h"
#include "AndroidFence.h"

using namespace android;

class SprdFrameBufferDevice : public SprdDisplayCore {
 public:
  SprdFrameBufferDevice();
  ~SprdFrameBufferDevice();

  /*
   *  Init func: open framebuffer device, and get some configs.
   * */
  virtual bool Init();

  virtual int AddFlushData(int DisplayType, SprdHWLayer **list, int LayerCount);

  virtual int PostDisplay(DisplayTrack *tracker);

  virtual int QueryDisplayInfo(uint32_t *DisplayNum);

  virtual int GetConfigs(int DisplayType, uint32_t *Configs,
                         size_t *NumConfigs);

  virtual int GetConfigAttributes(int DisplayType, uint32_t Config,
                                  const uint32_t *attributes, int32_t *values);

  virtual int EventControl(int DisplayType, int event, bool enabled);

  virtual int Blank(int DisplayType, bool enabled);

  virtual int Dump(char *buffer);

 private:
  typedef struct _FlushContext_t {
    int LayerCount;
    SprdHWLayer **LayerList;
    int32_t Active;
    int32_t DisplayType;
    // struct overlay_setting         base;
    // struct overlay_display_setting display;
    void *user;
  } FlushContext_t;

  typedef struct _FramebufferDeviceInfo {
    int fd;
    int width;
    int height;
    float xdpi;
    float ydpi;
    int stride;
    int format;
    framebuffer_device_t *fbDev;
  } FramebufferDeviceInfo_t;

  bool mPrimaryPlane;
  bool mOverlayPlane;
  bool mFBTDisplay;
  int mDebugFlag;
  FramebufferDeviceInfo_t *mFBDInfo;
  sp<SprdVsyncEvent> mVsyncEvent;
  FlushContext_t mFlushContext[DEFAULT_DISPLAY_TYPE_NUM];
  SprdHWLayer *mFBTLayer;

  inline FlushContext_t *getFlushContext(int displayType) {
    if (displayType >= 0) {
      return &(mFlushContext[displayType]);
    } else {
      return NULL;
    }
  }

  inline sp<SprdVsyncEvent> getVsyncEventHandle() { return mVsyncEvent; }

  int configPrimaryPlane(FlushContext_t *ctx, SprdHWLayer *l);
  int configOverlayPlane(FlushContext_t *ctx, SprdHWLayer *l);
  int configDisplayPara(FlushContext_t *ctx, SprdHWLayer *l);

  int frameBufferDeviceOpen();
  void frameBufferDeviceClose();
  int frameBufferDevicePost();

  void invalidateFlushContext();
};

#endif  // #ifndef SPRD_FRAME_BUFFER_DEVICE_H_
