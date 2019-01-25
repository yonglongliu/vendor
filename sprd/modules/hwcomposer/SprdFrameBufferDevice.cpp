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
 *  SprdFrameBufferDevice.cpp:
 *  The traditional Frame buffer device,
 *  It pass display data from HWC to Display driver.
 *  Author: zhongjun.chen@spreadtrum.com
 */

#include "SprdFrameBufferDevice.h"
#include "MemoryHeapIon.h"
#include "SprdDisplayDevice.h"

using namespace android;

/*
 *  Public interface
 */
SprdFrameBufferDevice::SprdFrameBufferDevice()
    : mPrimaryPlane(false),
      mOverlayPlane(false),
      mFBTDisplay(false),
      mDebugFlag(0),
      mFBDInfo(NULL),
      mVsyncEvent(0),
      mFBTLayer(NULL) {}

SprdFrameBufferDevice::~SprdFrameBufferDevice() {
  frameBufferDeviceClose();

  if (mVsyncEvent != NULL) {
    mVsyncEvent->setEnabled(0);
    mVsyncEvent->requestExitAndWait();
  }
}

bool SprdFrameBufferDevice::Init() {
  if (SprdDisplayCore::Init() == false) {
    ALOGE("SprdFrameBufferDevice:: Init SprdDisplayCore::Init failed");
  }

  /*
   *  Open Frame Buffer device
   */
  if (frameBufferDeviceOpen() != 0) {
    ALOGE("SprdFrameBufferDevice:: Init frameBufferDeviceOpen error");
    mInitFlag = false;
    return false;
  }

  mVsyncEvent = new SprdVsyncEvent(this, mFBDInfo->fd);
  if (mVsyncEvent == NULL) {
    ALOGE("SprdFrameBufferDevice:: Init new SprdVsyncEvent failed");
    mInitFlag = false;
    frameBufferDeviceClose();
    return false;
  }

  return mInitFlag;
}

int SprdFrameBufferDevice::AddFlushData(int DisplayType, SprdHWLayer **list,
                                        int LayerCount) {
  FlushContext_t *ctx = NULL;
  queryDebugFlag(&mDebugFlag);

  if ((DisplayType < 0) || (DisplayType > DEFAULT_DISPLAY_TYPE_NUM)) {
    ALOGE("SprdFrameBufferDevice:: AddFlushData DisplayType is invalidate");
    return -1;
  }

  if (list == NULL || LayerCount <= 0) {
    ALOGE("SprdFrameBufferDevice:: AddFlushData context para error");
    return -1;
  }

  ctx = &(mFlushContext[DisplayType]);
  ctx->LayerCount = LayerCount;
  ctx->LayerList = list;
  ctx->DisplayType = DisplayType;
  ctx->user = NULL;
  ctx->Active = true;

  mLayerCount += ctx->LayerCount;
  mActiveContextCount++;

  ALOGI_IF(mDebugFlag,
           "SprdFrameBufferDevice:: AddFlushData disp: %d, LayerCount: %d",
           DisplayType, LayerCount);

  return 0;
}

int SprdFrameBufferDevice::PostDisplay(DisplayTrack *tracker) {
  int i = 0;
  int ret = -1;
  FlushContext_t *ctx = NULL;
  mFBTDisplay = false;

  if (tracker == NULL) {
    ALOGE("SprdFrameBufferDevice:: PostDisplay tracker is NULL");
    ret = -1;
    goto EXT0;
  }

  if (mLayerCount <= 0) {
    ALOGE("SprdFrameBufferDevice:: PostDisplay No Layer should be displayed");
    ret = -1;
    goto EXT0;
  }

  mActiveContextCount = (mActiveContextCount > DEFAULT_DISPLAY_TYPE_NUM)
                            ? DEFAULT_DISPLAY_TYPE_NUM
                            : mActiveContextCount;

  for (i = 0; i < mActiveContextCount; i++) {
    ctx = getFlushContext(i);

    if (ctx == NULL) {
      ALOGI_IF(mDebugFlag,
               "SprdFrameBufferDevice:: PostDisplay get NULL flush context");
      continue;
    }

    if (ctx->Active == false) {
      continue;
    }

    /*
     *  SPRD chip do not support EXTERNAL display device at present.
     */
    if (i == DISPLAY_EXTERNAL) {
      ALOGI_IF(
          mDebugFlag,
          "SprdFrameBufferDevice:: PostDisplay not support EXTERNAL DISPLAY");
      continue;
    }

    if (i == DISPLAY_PRIMARY) {
      int j;
      bool FBTDisplay = false;
      SprdHWLayer *l = NULL;
      for (j = 0; j < ctx->LayerCount; j++) {
        struct private_handle_t *privateH = NULL;
        l = ctx->LayerList[j];
        if (l == NULL) {
          continue;
        }

        privateH = l->getBufferHandle();
        if (privateH == NULL) {
          ALOGE(
              "SprdFrameBufferDevice:: configPrimaryPlane buffer handle is "
              "NULL");
          return -1;
        }

        if (privateH->usage & GRALLOC_USAGE_HW_FB) {
          mFBTDisplay = true;
          mFBTLayer = l;
          ret = 0;
          continue;
        }

        if (l->getLayerType() == LAYER_OSD) {
          ret = configPrimaryPlane(ctx, l);
        } else {
          ret = configOverlayPlane(ctx, l);
        }
      }

      if (mFBTDisplay == false) {
        // TODO:
        // we should calculate the dirty region later.
        ret = configDisplayPara(ctx, l);
      }
    }

    if (ret != 0) {
      ALOGI_IF(mDebugFlag,
               "SprdFrameBufferDevice:: PostDisplay config Plane error");
      goto EXT0;
    }

    ret = frameBufferDevicePost();
    if (ret != 0) {
      ALOGE("SprdFrameBufferDevice:: PostDisplay frameBufferDevicePost error");
    }
  }

EXT0:
  GenerateSyncFenceForFBDevice(DISPLAY_PRIMARY, &(tracker->releaseFenceFd),
                               &(tracker->retiredFenceFd));

  invalidateFlushContext();

  return ret;
}

int SprdFrameBufferDevice::QueryDisplayInfo(uint32_t *DisplayNum) {
  if (mInitFlag == false) {
    ALOGE("func: %s line: %d SprdADFWrapper Need Init first", __func__,
          __LINE__);
    return -1;
  }

  *DisplayNum = 1;

  return 0;
}

int SprdFrameBufferDevice::GetConfigs(int DisplayType, uint32_t *Configs,
                                      size_t *NumConfigs) {
  int ret = -1;

  switch (DisplayType) {
    case DISPLAY_PRIMARY:
      if (*NumConfigs > 0) {
        Configs[0] = 0;
        *NumConfigs = 1;
      }
      ret = 0;
      break;
    case DISPLAY_EXTERNAL:
      ret = -1;
      if (*NumConfigs > 0) {
        Configs[0] = 0;
        *NumConfigs = 1;
      }
      ret = 0;
      break;
    default:
      return ret;
  }

  return ret;
}

int SprdFrameBufferDevice::GetConfigAttributes(int DisplayType, uint32_t Config,
                                               const uint32_t *attributes,
                                               int32_t *values) {
  float refreshRate = 60.0;
  framebuffer_device_t *fbDev = mFBDInfo->fbDev;

  if (DISPLAY_EXTERNAL == DisplayType) {
    // ALOGD("External Display Device is not connected");
    return -1;
  }

  if (DISPLAY_VIRTUAL == DisplayType) {
    ALOGD("VIRTUAL Display Device is not connected");
    return -1;
  }

  if (fbDev->fps > 0) {
    refreshRate = fbDev->fps;
  }

  static const uint32_t DISPLAY_ATTRIBUTES[] = {
      HWC_DISPLAY_VSYNC_PERIOD, HWC_DISPLAY_WIDTH, HWC_DISPLAY_HEIGHT,
      HWC_DISPLAY_DPI_X,        HWC_DISPLAY_DPI_Y, HWC_DISPLAY_NO_ATTRIBUTE,
  };

  for (int i = 0; i < NUM_DISPLAY_ATTRIBUTES - 1; i++) {
    switch (attributes[i]) {
      case HWC_DISPLAY_VSYNC_PERIOD:
        values[i] = 1000000000l / refreshRate;
        break;
      case HWC_DISPLAY_WIDTH:
        values[i] = mFBDInfo->width;
        break;
      case HWC_DISPLAY_HEIGHT:
        values[i] = mFBDInfo->height;
        break;
      case HWC_DISPLAY_DPI_X:
        values[i] = (int32_t)(mFBDInfo->xdpi);
        break;
      case HWC_DISPLAY_DPI_Y:
        values[i] = (int32_t)(mFBDInfo->ydpi);
        break;
      default:
        ALOGE("Unknown Display Attributes:%d", attributes[i]);
        return -EINVAL;
    }
  }

  return 0;
}

int SprdFrameBufferDevice::EventControl(int DisplayType, int event,
                                        bool enabled) {
  int status = -1;

  sp<SprdVsyncEvent> VE = getVsyncEventHandle();
  if (VE == NULL) {
    ALOGE("getVsyncEventHandle failed");
    return -1;
  }

  if (DisplayType == DISPLAY_PRIMARY) {
    switch (event) {
      case HWC_EVENT_VSYNC:
        VE->setEnabled(enabled);
        break;

      default:
        ALOGE("unsupported event");
        status = -EPERM;
        return status;
    }
  }

  return 0;
}

int SprdFrameBufferDevice::Blank(int DisplayType, bool enabled) {
  int ret = -1;
  int sprdMode = -1;

  return ret;
}

int SprdFrameBufferDevice::Dump(char *buffer) { return 0; }

/*
 *  Private interface
 */
int SprdFrameBufferDevice::configPrimaryPlane(FlushContext_t *ctx,
                                              SprdHWLayer *l) {
  int format = -1;
  size_t size;
  struct private_handle_t *privateH = NULL;
  struct sprdRect *fb;

  if (ctx == NULL || l == NULL) {
    ALOGE("SprdFrameBufferDevice:: configPrimaryPlane input para is NULL");
    return -1;
  }

  privateH = l->getBufferHandle();
  if (privateH == NULL) {
    ALOGE("SprdFrameBufferDevice:: configPrimaryPlane buffer handle is NULL");
    return -1;
  }

  fb = l->getSprdFBRect();
  if (fb == NULL) {
    ALOGE("SprdFrameBufferDevice:: configPrimaryPlane fb region is NULL");
    return -1;
  }

  format = l->getLayerFormat();

  fb = l->getSprdFBRect();

  ctx->base.layer_index = SPRD_LAYERS_OSD;
  ctx->base.rect.x = fb->x;
  ctx->base.rect.y = fb->y;
  ctx->base.rect.w = fb->w;
  ctx->base.rect.h = fb->h;

  switch (format) {
    case HAL_PIXEL_FORMAT_RGBA_8888:
    case HAL_PIXEL_FORMAT_RGBX_8888:
      ctx->base.data_type = SPRD_DATA_FORMAT_RGB888;
      ctx->base.y_endian = SPRD_DATA_ENDIAN_B0B1B2B3;
      ctx->base.uv_endian = SPRD_DATA_ENDIAN_B0B1B2B3;
      ctx->base.rb_switch = 1;
      break;
    case HAL_PIXEL_FORMAT_BGRA_8888:
#if 0  // should verify lator
            ctx->base.data_type = SPRD_DATA_FORMAT_RGB888;
            ctx->base.y_endian = SPRD_DATA_ENDIAN_B0B1B2B3;
            ctx->base.uv_endian = SPRD_DATA_ENDIAN_B0B1B2B3;
            ctx->base.rb_switch = 0;
#endif
    case HAL_PIXEL_FORMAT_RGB_565:
      ctx->base.data_type = SPRD_DATA_FORMAT_RGB565;
      ctx->base.y_endian = SPRD_DATA_ENDIAN_B2B3B0B1;
      ctx->base.uv_endian = SPRD_DATA_ENDIAN_B0B1B2B3;
      ctx->base.rb_switch = 0;
      break;
    default:
      ALOGE(
          "SprdFrameBufferDevice:: configPrimaryPlane not support format: 0x%x",
          format);
  }

  MemoryHeapIon::Get_phy_addr_from_ion(privateH->share_fd, &(privateH->phyaddr),
                                       &size);

  ctx->base.buffer = (unsigned char *)(privateH->phyaddr);

  ALOGI_IF(mDebugFlag,
           "SprdFrameBufferDevice:: configPrimaryPlane parameter datatype = "
           "%d, x = %d, y = %d, w = %d, h = %d, buffer = 0x%p",
           ctx->base.data_type, ctx->base.rect.x, ctx->base.rect.y,
           ctx->base.rect.w, ctx->base.rect.h, ctx->base.buffer);

  if (mFBDInfo->fd < 0) {
    ALOGE("SprdFrameBufferDevice:: configPrimaryPlane mFBD fd: %d < 0");
    return -1;
  }

  if (ioctl(mFBDInfo->fd, SPRD_FB_SET_OVERLAY, &(ctx->base)) == -1) {
    ALOGE("fail osd SPRD_FB_SET_OVERLAY");
    ioctl(mFBDInfo->fd, SPRD_FB_SET_OVERLAY, &(ctx->base));  // Fix ME later
  }

  mPrimaryPlane = true;

  return 0;
}

int SprdFrameBufferDevice::configOverlayPlane(FlushContext_t *ctx,
                                              SprdHWLayer *l) {
  int format = -1;
  size_t size;
  struct private_handle_t *privateH = NULL;
  struct sprdRect *fb;

  if (ctx == NULL || l == NULL) {
    ALOGE("SprdFrameBufferDevice:: configOverlayPlane input para is NULL");
    return -1;
  }

  privateH = l->getBufferHandle();
  if (privateH == NULL) {
    ALOGE("SprdFrameBufferDevice:: configOverlayPlane buffer handle is NULL");
    return -1;
  }

  if (fb == NULL) {
    ALOGE("SprdFrameBufferDevice:: configOverlayPlane fb region is NULL");
    return -1;
  }

  format = l->getLayerFormat();

  fb = l->getSprdFBRect();

  ctx->base.layer_index = SPRD_LAYERS_IMG;
  ctx->base.rect.x = fb->x;
  ctx->base.rect.y = fb->y;
  ctx->base.rect.w = fb->w;
  ctx->base.rect.h = fb->h;

  switch (format) {
    case HAL_PIXEL_FORMAT_RGBA_8888:
    case HAL_PIXEL_FORMAT_RGBX_8888:
      ctx->base.data_type = SPRD_DATA_FORMAT_RGB888;
      ctx->base.y_endian = SPRD_DATA_ENDIAN_B0B1B2B3;
      ctx->base.uv_endian = SPRD_DATA_ENDIAN_B0B1B2B3;
      ctx->base.rb_switch = 1;
      break;
    case HAL_PIXEL_FORMAT_BGRA_8888:
#if 0  // should verify lator
            ctx->base.data_type = SPRD_DATA_FORMAT_RGB888;
            ctx->base.y_endian = SPRD_DATA_ENDIAN_B0B1B2B3;
            ctx->base.uv_endian = SPRD_DATA_ENDIAN_B0B1B2B3;
            ctx->base.rb_switch = 0;
#endif
    case HAL_PIXEL_FORMAT_RGB_565:
      ctx->base.data_type = SPRD_DATA_FORMAT_RGB565;
      ctx->base.y_endian = SPRD_DATA_ENDIAN_B2B3B0B1;
      ctx->base.uv_endian = SPRD_DATA_ENDIAN_B0B1B2B3;
      ctx->base.rb_switch = 0;
      break;
    case HAL_PIXEL_FORMAT_YCbCr_422_SP:
      ctx->base.data_type = SPRD_DATA_FORMAT_YUV422;
      ctx->base.y_endian = SPRD_DATA_ENDIAN_B3B2B1B0;
      // ctx->base.uv_endian = SPRD_DATA_ENDIAN_B3B2B1B0;
      ctx->base.uv_endian = SPRD_DATA_ENDIAN_B0B1B2B3;
      ctx->base.rb_switch = 0;
      break;
    case HAL_PIXEL_FORMAT_YCbCr_420_SP:
      ctx->base.data_type = SPRD_DATA_FORMAT_YUV420;
      ctx->base.y_endian = SPRD_DATA_ENDIAN_B3B2B1B0;
      ctx->base.rb_switch = 0;
      ctx->base.uv_endian = SPRD_DATA_ENDIAN_B3B2B1B0;
      break;
    default:
      ALOGE(
          "SprdFrameBufferDevice:: configOverlayPlane not support format: 0x%x",
          format);
  }

  MemoryHeapIon::Get_phy_addr_from_ion(privateH->share_fd, &(privateH->phyaddr),
                                       &size);

  ctx->base.buffer = (unsigned char *)privateH->phyaddr;

  ALOGI_IF(mDebugFlag,
           "SprdFrameBufferDevice::configOverlayPlane(video) parameter "
           "datatype = %d, x = %d, y = %d, w = %d, h = %d, buffer = 0x%p",
           ctx->base.data_type, ctx->base.rect.x, ctx->base.rect.y,
           ctx->base.rect.w, ctx->base.rect.h, ctx->base.buffer);

  if (mFBDInfo->fd < 0) {
    ALOGE("SprdFrameBufferDevice:: configOverlayPlane mFBDeviceFd: %d < 0");
    return -1;
  }

  if (ioctl(mFBDInfo->fd, SPRD_FB_SET_OVERLAY, &(ctx->base)) == -1) {
    ALOGE("fail video SPRD_FB_SET_OVERLAY");
    ioctl(mFBDInfo->fd, SPRD_FB_SET_OVERLAY, &(ctx->base));  // Fix ME later
  }

  mOverlayPlane = true;

  return 0;
}

int SprdFrameBufferDevice::configDisplayPara(FlushContext_t *ctx,
                                             SprdHWLayer *l) {
  int PlaneType = 0;
  struct sprdRect *fb;

  if (ctx == NULL || l == NULL) {
    ALOGE("SprdFrameBufferDevice:: configDisplayPara input para is NULL");
  }

  fb = l->getSprdFBRect();
  if (fb == NULL) {
    ALOGE("SprdFrameBufferDevice:: configDisplayPara fb region is NULL");
    return -1;
  }

  if (mPrimaryPlane) {
    PlaneType |= SPRD_LAYERS_OSD;
  }

  if (mOverlayPlane) {
    PlaneType |= SPRD_LAYERS_IMG;
  }

  ctx->display.display_mode = SPRD_DISPLAY_OVERLAY_ASYNC;
  ctx->display.layer_index = PlaneType;
  // TODO:
  // we should calculate the dirty region later.
  ctx->display.rect.x = fb->x;
  ctx->display.rect.y = fb->y;
  ctx->display.rect.w = fb->w;
  ctx->display.rect.h = fb->h;

  ALOGI_IF(mDebugFlag, "SPRD_FB_DISPLAY_OVERLAY %d", PlaneType);

  return 0;
}

int SprdFrameBufferDevice::frameBufferDeviceOpen() {
  char const *const deviceTemplate[] = {"/dev/graphics/fb%u", "/dev/fb%u",
                                        NULL};

  int ret = -1;
  int fd = -1;
  int i = 0;
  int bytespp;
  char name[64];
  void *vaddr;

  struct fb_fix_screeninfo finfo;
  struct fb_var_screeninfo vinfo;

  hw_module_t const *module;
  framebuffer_device_t *fbDev;

  ret = hw_get_module(GRALLOC_HARDWARE_MODULE_ID, &module);
  if (ret != 0) {
    ALOGE("%s module not found", GRALLOC_HARDWARE_MODULE_ID);
    return ret;
  }

  ret = framebuffer_open(module, &fbDev);
  if (ret != 0) {
    ALOGE("framebuffer_open failed");
    return ret;
  }

  while ((fd == -1) && deviceTemplate[i]) {
    snprintf(name, 64, deviceTemplate[i], 0);
    fd = open(name, O_RDWR, 0);
    i++;
  }

  if (fd < 0) {
    ALOGE("fail to open fb");
    ret = -1;
    return ret;
  }

  if (ioctl(fd, FBIOGET_FSCREENINFO, &finfo) == -1) {
    ALOGE("fail to get FBIOGET_FSCREENINFO");
    close(fd);
    ret = -1;
    return ret;
  }

  if (ioctl(fd, FBIOGET_VSCREENINFO, &vinfo) == -1) {
    ALOGE("fail to get FBIOGET_VSCREENINFO");
    close(fd);
    ret = -1;
    return ret;
  }

  /*
   * Store the FrameBuffer info
   * */
  FramebufferDeviceInfo_t *FBInfo =
      (FramebufferDeviceInfo_t *)malloc(sizeof(FramebufferDeviceInfo_t));
  if (FBInfo == NULL) {
    ALOGE("Cannot malloc the FrameBufferInfo, no MEM");
    close(fd);
    ret = -1;
    return ret;
  }

  FBInfo->fd = fd;
  FBInfo->width = vinfo.xres;
  FBInfo->height = vinfo.yres;
  FBInfo->stride = finfo.line_length / (vinfo.xres / 8);
  FBInfo->xdpi = (vinfo.xres * 25.4f) / vinfo.width;
  FBInfo->ydpi = (vinfo.yres * 25.4f) / vinfo.height;

  switch (vinfo.bits_per_pixel) {
    case 16:
      bytespp = 2;
      FBInfo->format = HAL_PIXEL_FORMAT_RGB_565;
      break;
    case 24:
      bytespp = 3;
      FBInfo->format = HAL_PIXEL_FORMAT_RGB_888;
      break;
    case 32:
      bytespp = 4;
      FBInfo->format = HAL_PIXEL_FORMAT_RGBA_8888;
      break;
    default:
      ALOGE("fail to getFrameBufferInfo not support bits per pixel:%d",
            vinfo.bits_per_pixel);
      free(FBInfo);
      ret = -1;
      return ret;
  }

  FBInfo->fbDev = fbDev;
  mFBDInfo = FBInfo;

  return 0;
}

void SprdFrameBufferDevice::frameBufferDeviceClose() {
  if (mFBDInfo) {
    free(mFBDInfo);
    mFBDInfo = NULL;
  }
}

int SprdFrameBufferDevice::frameBufferDevicePost() {
  /*
   *  This is temporarily method, call gralloc fbPost.
   *  will be replaced to other methods lator
   *  If this is framebuffer_target, call fbPost and then
   *  return, suppose FBT is Not compatible with Overlay
   */
  if (mFBTDisplay) {
    hwc_layer_1_t *FBTargetLayer = NULL;
    if (mFBTLayer == NULL) {
      ALOGE("SprdFBTargetLayer is NULL");
      return -1;
    }

    FBTargetLayer = mFBTLayer->getAndroidLayer();

    if (FBTargetLayer == NULL) {
      ALOGE("Android FBT is NULL");
      return -1;
    }

    struct private_handle_t *privateH = mFBTLayer->getBufferHandle();

    ALOGI_IF(mDebugFlag, "Start Displaying FramebufferTarget layer");

    if (FBTargetLayer->acquireFenceFd >= 0) {
      String8 name("HWCFBT::Post");

      FenceWaitForever(name, FBTargetLayer->acquireFenceFd);
      if (FBTargetLayer->acquireFenceFd >= 0) {
        close(FBTargetLayer->acquireFenceFd);
        FBTargetLayer->acquireFenceFd = -1;
      }
    }

    mFBDInfo->fbDev->post(mFBDInfo->fbDev, privateH);

    goto FBTDone;
  }

  ioctl(mFBDInfo->fd, SPRD_FB_DISPLAY_OVERLAY,
        &(mFlushContext[DISPLAY_PRIMARY].display));

FBTDone:
  return 0;
}

void SprdFrameBufferDevice::invalidateFlushContext() {
  int i;

  for (i = 0; i < mActiveContextCount; i++) {
    FlushContext_t *ctx = getFlushContext(i);
    ctx->LayerCount = 0;
    ctx->LayerList = NULL;
    ctx->Active = false;
    ctx->user = NULL;
  }

  mActiveContextCount = 0;

  mPrimaryPlane = false;
  mOverlayPlane = false;
  mFBTDisplay = false;
  mFBTLayer = NULL;
}
