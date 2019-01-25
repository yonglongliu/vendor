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

/******************************************************************************
 **                   Edit    History                                         *
 **---------------------------------------------------------------------------*
 ** DATE          Module              DESCRIPTION                             *
 ** 22/09/2013    Hardware Composer   Responsible for processing some         *
 **                                   Hardware layers. These layers comply    *
 **                                   with display controller specification,  *
 **                                   can be displayed directly, bypass       *
 **                                   SurfaceFligner composition. It will     *
 **                                   improve system performance.             *
 ******************************************************************************
 ** File: SprdPrimaryPlane.h          DESCRIPTION                             *
 **                                   display RGBA format Hardware layer      *
 ******************************************************************************
 ******************************************************************************
 ** Author:         zhongjun.chen@spreadtrum.com                              *
 *****************************************************************************/

#include "SprdPrimaryPlane.h"
#include "dump.h"
#include "../AndroidFence.h"

using namespace android;

SprdPrimaryPlane::SprdPrimaryPlane()
    : SprdDisplayPlane(),
      mFBInfo(NULL),
      mHWLayer(NULL),
      mPrimaryPlaneCount(1),
      mFreePlaneCount(1),
      mContext(NULL),
      mBuffer(NULL),
      mBufferIndex(-1),
      mDefaultDisplayFormat(-1),
      mDisplayFormat(-1),
      mPlaneDisable(false),
      mThreadID(-1),
      mDebugFlag(0),
      mDumpFlag(0) {
#ifdef PRIMARYPLANE_USE_RGB565
  mDefaultDisplayFormat = HAL_PIXEL_FORMAT_RGB_565;
#else
  mDefaultDisplayFormat = HAL_PIXEL_FORMAT_RGBA_8888;
#endif

  SprdDisplayPlane::setPlaneRunThreshold(100);

  mContext = SprdDisplayPlane::getPlaneBaseContext();

  mDisplayFormat = mDefaultDisplayFormat;

  mThreadID = gettid();
}

SprdPrimaryPlane::~SprdPrimaryPlane() { close(); }

void SprdPrimaryPlane::updateFBInfo(FrameBufferInfo *fbInfo) {
  if (fbInfo == NULL) {
    ALOGE("SprdPrimaryPlane:: updateFBInfo input para is NULL");
    return;
  }

  mFBInfo = fbInfo;

  SprdDisplayPlane::setGeometry(mFBInfo->fb_width, mFBInfo->fb_height,
                                mDefaultDisplayFormat,
                                GRALLOC_USAGE_OVERLAY_BUFFER);

#ifndef FORCE_DISABLE_HWC_OVERLAY
#ifndef DYNAMIC_RELEASE_PLANEBUFFER
  open();
#endif
#endif
}

int SprdPrimaryPlane::setPlaneContext(void *context) {
  if (mContext == NULL) {
    ALOGE("SprdOverlayPlane::setPlaneContext mContext is NULL");
    return -1;
  }

  if (context == NULL) {
    ALOGE("SprdOverlayPlane::setPlaneContext input contxt is NULL");
    return -1;
  }

  mContext->BaseContext = context;

  return 0;
}

native_handle_t* SprdPrimaryPlane::dequeueBuffer(int *fenceFd) {
  bool ret = false;
  int localThreadID = -1;

  queryDebugFlag(&mDebugFlag);
  queryDumpFlag(&mDumpFlag);

  mFreePlaneCount = 1;

  enable();

  mBuffer = SprdDisplayPlane::dequeueBuffer(fenceFd);
  if (mBuffer == NULL) {
    ALOGE("SprdPrimaryPlane cannot get ION buffer");
    return NULL;
  }

  mBufferIndex = SprdDisplayPlane::getPlaneBufferIndex();

  ALOGI_IF(mDebugFlag, "SprdPrimaryPlane::dequeueBuffer handle:%p, index: %d",
           (void *)mBuffer, mBufferIndex);

  return mBuffer;
}

int SprdPrimaryPlane::queueBuffer(int fenceFd) {
  if (mContext->DisplayFBTarget || mContext->DirectDisplay) {
  }

  SprdDisplayPlane::queueBuffer(fenceFd);

  mFreePlaneCount = 0;

  return 0;
}

void SprdPrimaryPlane::AttachPrimaryLayer(SprdHWLayer *l,
                                          bool DirectDisplayFlag) {
  int ret = checkHWLayer(l);

  if (ret != 0) {
    ALOGE("Check Sprd PrimaryLayer failed");
    return;
  }

  mHWLayer = l;

  if (DirectDisplayFlag) {
    hwc_layer_1_t *AndroidLayer = mHWLayer->getAndroidLayer();
    SetDisplayParameters(AndroidLayer);
  }
}

void SprdPrimaryPlane::AttachFramebufferTargetLayer(
    hwc_layer_1_t *FBTargetLayer) {
  native_handle_t *privateH = (native_handle_t *)FBTargetLayer->handle;

  mDisplayFormat = ADP_FORMAT(privateH);

  mContext->DisplayFBTarget = true;

  ALOGI_IF(mDebugFlag, "FBTargetLayer FB handle:%p", (void *)privateH);
}

bool SprdPrimaryPlane::SetDisplayParameters(hwc_layer_1_t *AndroidLayer) {
  if (AndroidLayer == NULL) {
    ALOGI_IF(mDebugFlag, "SprdHWLayer is NULL");
    mContext->DirectDisplay = false;
    return false;
  }

  native_handle_t *privateH = (native_handle_t *)AndroidLayer->handle;

  if (privateH == NULL) {
    ALOGE("SetDisplayParameters privateH is NULL");
    mContext->DirectDisplay = false;
    return false;
  }

  if ((ADP_FORMAT(privateH) == HAL_PIXEL_FORMAT_YCbCr_420_SP) ||
      (ADP_FORMAT(privateH) == HAL_PIXEL_FORMAT_YCrCb_420_SP) ||
      (ADP_FORMAT(privateH) == HAL_PIXEL_FORMAT_YV12)) {
    ALOGI(
        "Current transform device and display device cannot support virtual "
        "adress");
    mContext->DirectDisplay = false;
    return false;
  }

//#if defined(GPU_TYPE_MIDGARD) || defined(GPU_TYPE_UTGARD)
#if (TARGET_GPU_PLATFORM != rogue)
  if (!(ADP_FLAGS(privateH) & (private_handle_t::PRIV_FLAGS_USES_PHY))) {
    ALOGI_IF(mDebugFlag, "Current device cannot support virtual adress");
    mContext->DirectDisplay = false;
    return false;
  }
#endif

  if (AndroidLayer->transform != 0) {
    ALOGI_IF(mDebugFlag, "This layer need to be transformed");
    mContext->DirectDisplay = false;
    return false;
  }

  mDisplayFormat = ADP_FORMAT(privateH);

  mContext->DirectDisplay = true;

  return mContext->DirectDisplay;
}

void SprdPrimaryPlane::disable() { mPlaneDisable = true; }

void SprdPrimaryPlane::enable() { mPlaneDisable = false; }

bool SprdPrimaryPlane::online() {
  if (mPlaneDisable) {
    ALOGI_IF(mDebugFlag, "SprdPrimaryPlane has been disabled");
    return false;
  }

  if (mFreePlaneCount == 0) {
    ALOGI_IF(mDebugFlag, "SprdPrimaryPlanens is not avaiable");
    return false;
  }

  return true;
}

int SprdPrimaryPlane::checkHWLayer(SprdHWLayer *l) {
  if (l == NULL) {
    ALOGE("SprdPrimaryPlane Failed to check the list, SprdHWLayer is NULL");
    return -1;
  }

  return 0;
}

int SprdPrimaryPlane::getPlaneFormat() const { return mDisplayFormat; }

native_handle_t* SprdPrimaryPlane::flush(int *fenceFd) {
  if (mContext == NULL /* || mContext->BaseContext == NULL*/) {
    ALOGE("SprdPrimaryPlane::flush get display context failed");
    return NULL;
  }

  native_handle_t* flushingBuffer = NULL;

  queryDebugFlag(&mDebugFlag);
  queryDumpFlag(&mDumpFlag);

  // TODO: Need add DirectDisplay path.
  // if ((mContext->DisplayFBTarget || mContext->DirectDisplay) == false)
  { flushingBuffer = SprdDisplayPlane::flush(fenceFd); }

  if ((HWCOMPOSER_DUMP_OSD_OVERLAY_FLAG & mDumpFlag) &&
      (flushingBuffer != NULL)) {
    const char *name = "OverlayOSD";

    dumpOverlayImage(flushingBuffer, name, (*fenceFd));
  }

  return flushingBuffer;
}

bool SprdPrimaryPlane::open() {
  if (SprdDisplayPlane::open() == false) {
    ALOGE("SprdPrimaryPlane::open failed");
    return false;
  }

  mContext->DirectDisplay = false;
  mContext->DisplayFBTarget = false;
  mPrimaryPlaneCount = 1;
  mFreePlaneCount = 1;

  return true;
}

bool SprdPrimaryPlane::close() {
  SprdDisplayPlane::close();

  mFreePlaneCount = 0;
  mContext->DirectDisplay = false;
  mContext->DisplayFBTarget = false;

  return true;
}

void SprdPrimaryPlane::InvalidatePlane() {
  int fenceFd = -1;

  if (mContext == NULL /* || mContext->BaseContext == NULL*/) {
    ALOGE("SprdPrimaryPlane::InvalidatePlane display context is NULL");
    return;
  }

  /*
   *  Restore some status.
   * */
  mContext->DisplayFBTarget = false;

  mContext->DirectDisplay = false;

  mDisplayFormat = mDefaultDisplayFormat;
}

SprdHWLayer *SprdPrimaryPlane::getPrimaryLayer() const { return mHWLayer; }

int SprdPrimaryPlane::getPlaneBufferIndex() const { return mBufferIndex; }

native_handle_t *SprdPrimaryPlane::getPlaneBuffer() const {
  if (mBuffer == NULL) {
    ALOGE("Failed to get hte SprdPrimaryPlane buffer");
    return NULL;
  }

  return mBuffer;
}

void SprdPrimaryPlane::getPlaneGeometry(unsigned int *width,
                                        unsigned int *height,
                                        int *format) const {
  if (width == NULL || height == NULL || format == NULL) {
    ALOGE("getPlaneGeometry, input parameters are NULL");
    return;
  }

  *width = mFBInfo->fb_width;
  *height = mFBInfo->fb_height;
  *format = mDefaultDisplayFormat;
}

PlaneContext *SprdPrimaryPlane::getPlaneContext() const { return mContext; }

#ifdef BORROW_PRIMARYPLANE_BUFFER
native_handle_t* SprdPrimaryPlane::dequeueFriendBuffer(int *fenceFd) {
  return SprdDisplayPlane::dequeueBuffer(fenceFd);
}

int SprdPrimaryPlane::queueFriendBuffer(int fenceFd) {
  return SprdDisplayPlane::queueBuffer(fenceFd);
}

native_handle_t* SprdPrimaryPlane::flushFriend(int *fenceFd) {
  return SprdDisplayPlane::flush(fenceFd);
}

int SprdPrimaryPlane::addFriendFlushReleaseFence(int fenceFd) {
  return SprdDisplayPlane::addFlushReleaseFence(fenceFd);
}
#endif
