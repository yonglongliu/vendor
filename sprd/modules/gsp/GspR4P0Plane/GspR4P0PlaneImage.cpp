/*
 * Copyright (C) 2010 The Android Open Source Project
 * Copyright (C) 2012-2015, The Linux Foundation. All rights reserved.
 *
 * Not a Contribution, Apache license notifications and license are retained
 * for attribution purposes only.
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

#include <hardware/hwcomposer_defs.h>
#include "GspR4P0Plane.h"
#include "GspR4P0PlaneArray.h"
#include "gsp_cfg.h"
#include "gsp_r4p0_cfg.h"

namespace android {

GspR4P0PlaneImage::GspR4P0PlaneImage(bool async, bool is_need_even,
                                     bool support_up_down, bool videoSize,
                                     uint16_t up, uint16_t down,
                                     const GspRangeSize &range) {
  mAttached = false;
  mIndex = -1;
  mAsync = async;
  mOddSupport = is_need_even ? false : true;
  mScale_Updown_Sametime = support_up_down ? true : false;
  mVideoMaxSize = videoSize;
  mRangeSize = range;
  mScaleRangeUp = up;
  mScaleRangeDown = down;
  ALOGI("create GspR4P0PlaneImage");
  mRangeSize.print();
  ALOGI("scale range up: %d, scale range down: %d", mScaleRangeUp,
        mScaleRangeDown);
  ALOGI("odd support: %d", mOddSupport);
  ALOGI("up_down_sametime support: %d", mScale_Updown_Sametime);
  ALOGI("vidoe max size: %d", mVideoMaxSize);
}

#if 0
bool GspR4P0PlaneImage::checkYuvInfo(mali_gralloc_yuv_info info) {
  bool result = true;

  // source layer YUV range check.
  if (info == MALI_YUV_BT709_NARROW || info == MALI_YUV_BT709_WIDE) {
    ALOGI_IF(mDebugFlag, "only support BT609 convert.");
    result = false;
  }

  return result;
}
#endif

bool GspR4P0PlaneImage::checkVideoSize(struct sprdRect *srcRect) {
  bool result = true;

  // source video size check.
  if (mVideoMaxSize == true) {
    if (srcRect->w * srcRect->h > 1280 * 720) {
      ALOGI_IF(mDebugFlag, "not support > 720P video.");
      result = false;
    }
  } else if (srcRect->w * srcRect->h > 1920 * 1080) {
    ALOGI_IF(mDebugFlag, "not support > 1080P video.");
    result = false;
  }

  return result;
}

bool GspR4P0PlaneImage::checkOddBoundary(
    struct sprdRect *srcRect, enum gsp_r4p0_img_layer_format format) {
  bool result = true;

  // if yuv_xywh_even == 1, gsp do not support odd source layer.
  if (isVideoLayer(format) == true && mOddSupport == false) {
    if ((srcRect->x & 0x1) || (srcRect->y & 0x1) || (srcRect->w & 0x1) ||
        (srcRect->h & 0x1)) {
      ALOGD_IF(mDebugFlag, "do not support odd source layer xywh.");
      result = false;
    }
  }

  return result;
}

bool GspR4P0PlaneImage::needScaleUpDown(struct sprdRect *srcRect,
                                        struct sprdRect *dstRect,
                                        enum gsp_rot_angle rot) {
  bool result = false;
  uint32_t srcw = 0;
  uint32_t srch = 0;
  uint32_t dstw = dstRect->w;
  uint32_t dsth = dstRect->h;

  if (isLandScapeTransform(rot) == true) {
    srcw = srcRect->h;
    srch = srcRect->w;
  } else {
    srcw = srcRect->w;
    srch = srcRect->h;
  }

  if ((mScale_Updown_Sametime == false) &&
      ((srcw < dstw && srch > dsth) || (srcw > dstw && srch < dsth))) {
    ALOGI_IF(mDebugFlag,
             "need scale up and down at same time, which not support");
    result = true;
  }

  return result;
}

bool GspR4P0PlaneImage::checkScaleSize(struct sprdRect *srcRect,
                                struct sprdRect *dstRect,
                                enum gsp_rot_angle rot, bool inFBC) {
  bool result = true;

  uint16_t scaleUpLimit = (inFBC ? 4 : (mScaleRangeUp / 16));
  uint16_t scaleDownLimit = (inFBC ? 4 : (16 / mScaleRangeDown));

  uint32_t srcw = 0;
  uint32_t srch = 0;
  uint32_t dstw = dstRect->w;
  uint32_t dsth = dstRect->h;

  if (isLandScapeTransform(rot) == true) {
    srcw = srcRect->h;
    srch = srcRect->w;
  } else {
    srcw = srcRect->w;
    srch = srcRect->h;
  }

  if (scaleUpLimit * srcw < dstw || scaleUpLimit * srch < dsth ||
      scaleDownLimit * dstw < srcw || scaleDownLimit * dsth < srch) {
    // gsp support [1/16-gsp_scaling_up_limit] scaling
    ALOGI_IF(mDebugFlag, "GSP only support %d-%d scaling!",
        scaleDownLimit, scaleUpLimit);
    result = false;
  }

  return result;
}

bool GspR4P0PlaneImage::checkScale(struct sprdRect *srcRect,
                                   struct sprdRect *dstRect,
                                   enum gsp_rot_angle rot, bool inFBC) {
  if (needScaleUpDown(srcRect, dstRect, rot) == true) return false;

  if (checkScaleSize(srcRect, dstRect, rot, inFBC) == false) return false;

  return true;
}

#if 0
bool GspR4P0PlaneImage::checkYuvType(mali_gralloc_yuv_info yuv_info) {
  bool result = true;

  if (yuv_info == MALI_YUV_BT709_NARROW || yuv_info == MALI_YUV_BT709_WIDE) {
    ALOGI_IF(mDebugFlag, "gsp not support BT609 convert");
    result = false;
  }

  return result;
}
#endif

void GspR4P0PlaneImage::reset(int flag) {
  mAttached = false;
  mIndex = -1;
  mDebugFlag = flag;
  memset(&mConfig, 0, sizeof(struct gsp_r4p0_img_layer_user));
}

bool GspR4P0PlaneImage::checkFBC(uint32_t pitch, uint32_t height,
                                 enum gsp_r4p0_img_layer_format format,
                                 bool inFBC) {
  bool isIFBC = false;
  bool isVFBC = false;
  uint32_t horAlign, verAlign;

  if (inFBC) {
    if ((GSP_R4P0_IMG_FMT_ARGB888 == format)||
      (GSP_R4P0_IMG_FMT_RGB888 == format)) {
      isIFBC = true;
      horAlign = 16;
      verAlign = 4;
    } else if (GSP_R4P0_IMG_FMT_YUV420_2P == format) {
      isVFBC = true;
      horAlign = 32;
      verAlign = 8;
    } else {
      horAlign = 1;
      verAlign = 1;
      ALOGI_IF(mDebugFlag, "image plane format(%d) unsuitable for FBC\n",
	  	format);
      return false;
    }

    if ((0 != pitch % horAlign)||(0 != height % verAlign)) {
      ALOGI_IF(mDebugFlag, "pitch/height un aligned %dx%d\n",
               horAlign, verAlign);
      return false;
    }
  }

  return true;
}

bool GspR4P0PlaneImage::adapt(SprdHWLayer *layer, int LayerIndex) {
  struct sprdRect *srcRect = layer->getSprdSRCRect();
  struct sprdRect *dstRect = layer->getSprdFBRect();
  hwc_layer_1_t *hwcLayer = layer->getAndroidLayer();
  native_handle_t *handle = (native_handle_t *)hwcLayer->handle;
  //  mali_gralloc_yuv_info yuv_info = ADP_YINFO(handle);
  //  enum gsp_addr_type bufType = bufTypeConvert(handle->flags);
  enum gsp_r4p0_img_layer_format format = imgFormatConvert(ADP_FORMAT(handle));
  enum gsp_rot_angle rot = rotationTypeConvert(hwcLayer->transform);
  bool inFBC = ADP_COMPRESSED(handle);
  uint32_t pitch = ADP_STRIDE(handle);
  uint32_t height = (inFBC ? ADP_VSTRIDE(handle):ADP_HEIGHT(handle));
  uint32_t fbc_mode;

  ALOGI_IF(mDebugFlag, "image plane start to adapt");
  ALOGI_IF(mDebugFlag, "img inFBC(%d)\n", inFBC);

  if (isAttached() == true) return false;

  if (isProtectedVideo(handle) == true) return false;

  if (checkRangeSize(srcRect, dstRect, mRangeSize) == false) return false;

  // if (checkYuvType(yuv_info) == false) return false;

  // if (checkVideoSize(srcRect) == false) return false;

  if (checkOddBoundary(srcRect, format) == false) return false;

  if (checkFBC(pitch, height, format, inFBC) == false) return false;

  if (checkScale(srcRect, dstRect, rot, inFBC) == false) return false;

  if (checkBlending(hwcLayer) == false) return false;

  attached(LayerIndex);
  ALOGI_IF(mDebugFlag, "image plane attached, layer index: %d", LayerIndex);

  return true;
}

void GspR4P0PlaneImage::configCommon(int wait_fd, int share_fd, int enable) {
  struct gsp_layer_user *common = &mConfig.common;

  common->type = GSP_IMG_LAYER;
  common->enable = enable;
  common->wait_fd = mAsync == true ? wait_fd : -1;
  common->share_fd = share_fd;
  ALOGI_IF(mDebugFlag,
           "conifg image plane enable: %d, wait_fd: %d, share_fd: %d",
           common->enable, common->wait_fd, common->share_fd);
}

void GspR4P0PlaneImage::configScaleEnable(struct sprdRect *srcRect,
                                          struct sprdRect *dstRect,
                                          enum gsp_rot_angle rot) {
  struct gsp_r4p0_img_layer_params *params = &mConfig.params;

  if (needScale(srcRect, dstRect, rot) == true) {
    params->scaling_en = 1;
  }
}

void GspR4P0PlaneImage::configSize(struct sprdRect *srcRect,
                                   struct sprdRect *dstRect, uint32_t pitch,
                                   uint32_t height,
                                   enum gsp_r4p0_img_layer_format format,
                                   enum gsp_rot_angle rot) {
  struct gsp_r4p0_img_layer_params *params = &mConfig.params;

  // configure source clip size
  params->clip_rect.st_x = srcRect->x;
  params->clip_rect.st_y = srcRect->y;
  params->clip_rect.rect_w = srcRect->w;
  params->clip_rect.rect_h = srcRect->h;

  params->des_rect.st_x = dstRect->x;
  params->des_rect.st_y = dstRect->y;
  params->des_rect.rect_w = dstRect->w;
  params->des_rect.rect_h = dstRect->h;

  params->pitch = pitch;
  params->height = height;

  if (isVideoLayer(format) == true && mOddSupport == false) {
    // image plane not support yuv format osdd size
    if (needScale(srcRect, dstRect, rot) == true) {
      params->clip_rect.st_x &= 0xfffe;
      params->clip_rect.st_y &= 0xfffe;
      params->clip_rect.rect_w &= 0xfffe;
      params->clip_rect.rect_h &= 0xfffe;
    }
  }

  ALOGI_IF(mDebugFlag, "config image plane pitch: %u, height: %d ", pitch,
           height);
  ALOGI_IF(mDebugFlag,
           "config image plane clip rect[%d %d %d %d] dst rect[%d %d %d %d]",
           params->clip_rect.st_x, params->clip_rect.st_y,
           params->clip_rect.rect_w, params->clip_rect.rect_h,
           params->des_rect.st_x, params->des_rect.st_y,
           params->des_rect.rect_w, params->des_rect.rect_h);
}

void GspR4P0PlaneImage::configFormat(enum gsp_r4p0_img_layer_format format) {
  struct gsp_r4p0_img_layer_params *params = &mConfig.params;

  params->img_format = format;
  ALOGI_IF(mDebugFlag, "config image plane format: %d", format);
}

void GspR4P0PlaneImage::configFBC(bool inFBC, int format) {
  struct gsp_r4p0_img_layer_params *params = &mConfig.params;
  uint32_t fbc_mode;

  if (inFBC && ((GSP_R4P0_IMG_FMT_ARGB888 == format)||
  	(GSP_R4P0_IMG_FMT_RGB888 == format)))
    fbc_mode = 1;
  else if (inFBC && (GSP_R4P0_IMG_FMT_YUV420_2P == format))
    fbc_mode = 2;
  else
    fbc_mode = 0;
  params->vfbc_mode = fbc_mode;
  ALOGI_IF(mDebugFlag, "config image plane vfbc_mode: %d", fbc_mode);
}

void GspR4P0PlaneImage::configPmargbMode(hwc_layer_1_t *layer) {
  struct gsp_r4p0_img_layer_params *params = &mConfig.params;

  if (layer->blending == HWC_BLENDING_PREMULT ||
      layer->blending == HWC_BLENDING_COVERAGE)
    params->pmargb_mod = 1;
  else
    params->pmargb_mod = 0;

  ALOGI_IF(mDebugFlag, "config image plane pmargb mode: %d",
           params->pmargb_mod);
}

void GspR4P0PlaneImage::configAlpha(uint8_t alpha) {
  struct gsp_r4p0_img_layer_params *params = &mConfig.params;

  params->alpha = alpha;
  ALOGI_IF(mDebugFlag, "config image plane apha: 0x%x", alpha);
}

void GspR4P0PlaneImage::configCSCMatrix(uint8_t y2r_mod) {
  struct gsp_r4p0_img_layer_params *params = &mConfig.params;

  params->y2r_mod = y2r_mod;
  params->y2y_mod = 1;
  ALOGI_IF(mDebugFlag, "config image plane y2r_mod: %d", y2r_mod);
}

void GspR4P0PlaneImage::configZorder(uint8_t zorder) {
  struct gsp_r4p0_img_layer_params *params = &mConfig.params;

  params->zorder = zorder;
  ALOGI_IF(mDebugFlag, "config image zorder: %d", zorder);
}

void GspR4P0PlaneImage::configEndian(native_handle_t *handle) {
  struct gsp_r4p0_img_layer_params *params = &mConfig.params;
  struct gsp_layer_user *common = &mConfig.common;
  int format = ADP_FORMAT(handle);
  uint32_t width = ADP_STRIDE(handle);
  uint32_t height = ADP_HEIGHT(handle);
  uint32_t pixel_cnt = ADP_STRIDE(handle) * ADP_HEIGHT(handle);
  common->offset.v_offset = common->offset.uv_offset = pixel_cnt;

  params->endian.uv_dword_endn = GSP_R4P0_DWORD_ENDN_0;
  params->endian.uv_word_endn = GSP_R4P0_WORD_ENDN_0;
  params->endian.y_rgb_dword_endn = GSP_R4P0_DWORD_ENDN_0;
  params->endian.y_rgb_word_endn = GSP_R4P0_WORD_ENDN_0;
  params->endian.a_swap_mode = GSP_R4P0_A_SWAP_ARGB;
  switch (format) {
    case HAL_PIXEL_FORMAT_RGBA_8888:
    case HAL_PIXEL_FORMAT_RGBX_8888:
      params->endian.rgb_swap_mode = GSP_R4P0_RGB_SWP_BGR;
      break;
    case HAL_PIXEL_FORMAT_YV12:  // YUV420_3P, Y V U
      common->offset.uv_offset += (height+1)/2 * ALIGN((width+1)/2, 16);
      params->endian.uv_word_endn = GSP_R4P0_WORD_ENDN_0;
      params->endian.y_rgb_word_endn = GSP_R4P0_WORD_ENDN_0;
      params->endian.a_swap_mode = GSP_R4P0_A_SWAP_ARGB;
      break;
    case HAL_PIXEL_FORMAT_YCrCb_420_SP:
      params->endian.uv_word_endn = GSP_R4P0_WORD_ENDN_2;
      break;
    case HAL_PIXEL_FORMAT_YCbCr_420_SP:
      params->endian.uv_word_endn = GSP_R4P0_WORD_ENDN_0;
      params->endian.y_rgb_word_endn = GSP_R4P0_WORD_ENDN_0;
      params->endian.a_swap_mode = GSP_R4P0_A_SWAP_ARGB;
      break;
    case HAL_PIXEL_FORMAT_RGB_565:
      params->endian.rgb_swap_mode = GSP_R4P0_RGB_SWP_RGB;
      break;
    case HAL_PIXEL_FORMAT_BGRA_8888:
    case HAL_PIXEL_FORMAT_BGRX_8888:
      params->endian.rgb_swap_mode = GSP_R4P0_RGB_SWP_RGB;
      break;
    default:
      ALOGE("image configEndian, unsupport format=0x%x.", format);
      break;
  }

  ALOGI_IF(mDebugFlag, "image plane y_rgb_word_endn: %d, uv_word_endn: %d",
           params->endian.y_rgb_word_endn, params->endian.uv_word_endn);

  ALOGI_IF(mDebugFlag, "image plane rgb_swap_mode: %d, a_swap_mode: %d",
           params->endian.rgb_swap_mode, params->endian.a_swap_mode);
}

struct gsp_r4p0_img_layer_user &GspR4P0PlaneImage::getConfig() {
  return mConfig;
}

bool GspR4P0PlaneImage::parcel(SprdHWLayer *layer) {
  if (layer == NULL) {
    ALOGE("image plane parcel params layer=NULL.");
    return false;
  }

  ALOGI_IF(mDebugFlag, "image plane start to parcel");
  hwc_layer_1_t *hwcLayer = layer->getAndroidLayer();
  enum gsp_rot_angle rot = rotationTypeConvert(hwcLayer->transform);

  struct sprdRect *srcRect = layer->getSprdSRCRect();
  struct sprdRect *dstRect = layer->getSprdFBRect();
  native_handle_t *handle = (native_handle_t *)hwcLayer->handle;
  enum gsp_r4p0_img_layer_format format = imgFormatConvert(ADP_FORMAT(handle));
  bool inFBC = ADP_COMPRESSED(handle);
  uint32_t pitch = ADP_STRIDE(handle);
  uint32_t height = (inFBC ? ADP_VSTRIDE(handle):ADP_HEIGHT(handle));

  // configure format
  configFormat(format);
  configFBC(inFBC, format);

  // configure clip size and dst size
  configSize(srcRect, dstRect, pitch, height, format, rot);

  // configure pmargb mode
  configPmargbMode(hwcLayer);

  int acquireFenceFd = hwcLayer->acquireFenceFd;
  int share_fd = ADP_BUFFD(handle);
  // configure acquire fence fd and dma buffer share fd
  configCommon(acquireFenceFd, share_fd, true);

  configScaleEnable(srcRect, dstRect, rot);

  uint8_t alpha = hwcLayer->planeAlpha;
  // configure alpha
  configAlpha(alpha);

  configZorder(layer->getLayerIndex());

  // configure csc matrix
  uint8_t y2r_mod = 3;
#if 0
  if (ADP_YINFO(handle) == YUV_BT601_NARROW ||
      ADP_YINFO(handle) == YUV_BT709_NARROW
      /*||private_h->yuv_info == MALI_YUV_BT2020_NARROW*/) {
    y2r_mod = 3;
  } else if (ADP_YINFO(handle) == YUV_BT601_WIDE ||
             ADP_YINFO(handle) == YUV_BT709_WIDE
             /*||private_h->yuv_info == YUV_BT2020_WIDE*/) {
    y2r_mod = 2;
  } else {
    y2r_mod = 3;
  }
#endif
  configCSCMatrix(y2r_mod);

  configEndian(handle);
  return true;
}

bool GspR4P0PlaneImage::parcel(uint32_t w, uint32_t h) {
  configCommon(-1, -1, false);

  return true;
}

}  // namespace android
