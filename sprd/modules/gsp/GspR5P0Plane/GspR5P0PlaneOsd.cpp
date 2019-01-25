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

#include "GspR5P0PlaneOsd.h"
#include "gralloc_public.h"
#include "SprdHWLayer.h"
#include "gsp_r5p0_cfg.h"

namespace android {

GspR5P0PlaneOsd::GspR5P0PlaneOsd(bool async, const GspRangeSize &range) {
  mIndex = -1;
  mAttached = false;
  mAsync = async;
  mRangeSize = range;
  mRangeSize.print();
  ALOGI("create GspR5P0PlaneOsd");
}

void GspR5P0PlaneOsd::reset(int flag) {
  mAttached = false;
  mIndex = -1;
  mDebugFlag = flag;
  memset(&mConfig, 0, sizeof(struct gsp_r5p0_osd_layer_user));
}

bool GspR5P0PlaneOsd::checkFBC(uint32_t pitch, uint32_t height,
                               enum gsp_r5p0_osd_layer_format format,
                               bool inFBC) {
  bool isIFBC = false;
  uint32_t horAlign, verAlign;

  if (inFBC) {
    if ((GSP_R5P0_OSD_FMT_ARGB888 == format) ||
        (GSP_R5P0_OSD_FMT_RGB888 == format)) {
      isIFBC = true;
      horAlign = 16;
      verAlign = 4;
    } else {
      horAlign = 1;
      verAlign = 1;
      ALOGI_IF(mDebugFlag, "osd plane format(%d) unsuitable for FBC\n", format);
      return false;
    }

    if ((0 != pitch % horAlign) || (0 != height % verAlign)) {
      ALOGI_IF(mDebugFlag, "pitch/height un aligned %dx%d\n", horAlign,
               verAlign);
      return false;
    }
  }

  return true;
}

bool GspR5P0PlaneOsd::adapt(SprdHWLayer *layer, int LayerIndex) {
  struct sprdRect *srcRect = layer->getSprdSRCRect();
  struct sprdRect *dstRect = layer->getSprdFBRect();
  hwc_layer_1_t *hwcLayer = layer->getAndroidLayer();
  native_handle_t *handle = (native_handle_t *)(hwcLayer->handle);
  // mali_gralloc_yuv_info yuv_info = ADP_YINFO(handle);
  // enum gsp_addr_type bufType = bufTypeConvert(handle->flags);
  enum gsp_r5p0_osd_layer_format format = osdFormatConvert(ADP_FORMAT(handle));
  enum gsp_rot_angle rot = rotationTypeConvert(hwcLayer->transform);
  bool inFBC = ADP_COMPRESSED(handle);
  uint32_t pitch = ADP_STRIDE(handle);
  // uint32_t height = (inFBC ? ADP_VSTRIDE(handle):ADP_HEIGHT(handle));
  uint32_t height = ADP_HEIGHT(handle);
  uint32_t fbc_mode;

  ALOGI_IF(mDebugFlag, "osd plane start to adapt, index: %d ", LayerIndex);
  ALOGI_IF(mDebugFlag, "osd inFBC(%d)\n", inFBC);

  if (isAttached() == true) return false;

  if (isVideoLayer(format) == true) return false;

  if (checkRangeSize(srcRect, dstRect, mRangeSize) == false) return false;

  if (checkFBC(pitch, height, format, inFBC) == false) return false;
  if (needScale(srcRect, dstRect, rot) == true) return false;

  if (checkBlending(hwcLayer) == false) return false;

  attached(LayerIndex);
  ALOGI_IF(mDebugFlag, "osd plane attached layer index: %d", LayerIndex);

  return true;
}

void GspR5P0PlaneOsd::configCommon(int wait_fd, int share_fd, int enable) {
  struct gsp_layer_user *common = &mConfig.common;

  common->type = GSP_OSD_LAYER;
  common->enable = enable;
  common->wait_fd = mAsync == true ? wait_fd : -1;
  common->share_fd = share_fd;
  ALOGI_IF(mDebugFlag, "config osd plane enable: %d, wait_fd: %d, share_fd: %d",
           common->enable, common->wait_fd, common->share_fd);
}

void GspR5P0PlaneOsd::configSize(struct sprdRect *srcRect,
                                 struct sprdRect *dstRect, uint32_t pitch,
                                 uint32_t height) {
  struct gsp_r5p0_osd_layer_params *params = &mConfig.params;

  // configure source clip size
  params->clip_rect.st_x = srcRect->x;
  params->clip_rect.st_y = srcRect->y;
  params->clip_rect.rect_w = srcRect->w;
  params->clip_rect.rect_h = srcRect->h;

  params->des_pos.pt_x = dstRect->x;
  params->des_pos.pt_y = dstRect->y;

  params->pitch = pitch;
  params->height = height;

  ALOGI_IF(mDebugFlag, "config osd plane pitch: %u, height: %u ", pitch,
           height);
  ALOGI_IF(mDebugFlag, "config osd plane clip rect[%d %d %d %d] dst pos[%d %d]",
           params->clip_rect.st_x, params->clip_rect.st_y,
           params->clip_rect.rect_w, params->clip_rect.rect_h,
           params->des_pos.pt_x, params->des_pos.pt_y);
}

void GspR5P0PlaneOsd::configSize(uint32_t w, uint32_t h) {
  struct gsp_r5p0_osd_layer_params *params = &mConfig.params;

  params->clip_rect.st_x = 0;
  params->clip_rect.st_y = 0;
  params->clip_rect.rect_w = w;
  params->clip_rect.rect_h = h;

  params->des_pos.pt_x = 0;
  params->des_pos.pt_y = 0;

  params->pitch = w;

  ALOGI_IF(mDebugFlag, "config osd plane pitch: %u", w);
  ALOGI_IF(mDebugFlag, "config osd plane clip rect[%d %d %d %d] dst pos[%d %d]",
           params->clip_rect.st_x, params->clip_rect.st_y,
           params->clip_rect.rect_w, params->clip_rect.rect_h,
           params->des_pos.pt_x, params->des_pos.pt_y);
}

void GspR5P0PlaneOsd::configFormat(enum gsp_r5p0_osd_layer_format format) {
  struct gsp_r5p0_osd_layer_params *params = &mConfig.params;

  params->osd_format = format;
  ALOGI_IF(mDebugFlag, "config osd plane format: %d", format);
}

void GspR5P0PlaneOsd::configFBC(bool inFBC, int format) {
  struct gsp_r5p0_osd_layer_params *params = &mConfig.params;
  uint32_t fbc_mode;

  if (inFBC && ((GSP_R5P0_OSD_FMT_ARGB888 == format) ||
                (GSP_R5P0_OSD_FMT_RGB888 == format)))
    fbc_mode = 1;
  else
    fbc_mode = 0;
  params->vfbc_mode = fbc_mode;
  ALOGI_IF(mDebugFlag, "config osd plane vfbc_mode: %d", fbc_mode);
}

void GspR5P0PlaneOsd::configPmargbMode(hwc_layer_1_t *layer) {
  struct gsp_r5p0_osd_layer_params *params = &mConfig.params;

  if (layer->blending == HWC_BLENDING_PREMULT ||
      layer->blending == HWC_BLENDING_COVERAGE)
    params->pmargb_mod = 1;
  else
    params->pmargb_mod = 0;

  ALOGI_IF(mDebugFlag, "config osd plane pmargb mode: %d", params->pmargb_mod);
}

void GspR5P0PlaneOsd::configAlpha(uint8_t alpha) {
  struct gsp_r5p0_osd_layer_params *params = &mConfig.params;

  params->alpha = alpha;
  ALOGI_IF(mDebugFlag, "config osd plane apha: 0x%x", alpha);
}

void GspR5P0PlaneOsd::configZorder(uint8_t zorder) {
  struct gsp_r5p0_osd_layer_params *params = &mConfig.params;

  params->zorder = zorder;
  ALOGI_IF(mDebugFlag, "config osd zorder: %d", zorder);
}

void GspR5P0PlaneOsd::configEndian(native_handle_t *handle) {
  struct gsp_r5p0_osd_layer_params *params = &mConfig.params;
  struct gsp_layer_user *common = &mConfig.common;
  int format = ADP_FORMAT(handle);
  common->offset.v_offset = common->offset.uv_offset = 0;

  switch (format) {
    case HAL_PIXEL_FORMAT_RGBA_8888:
    case HAL_PIXEL_FORMAT_RGBX_8888:
      params->endian.y_rgb_word_endn = GSP_R5P0_WORD_ENDN_1;
      params->endian.y_rgb_dword_endn = GSP_R5P0_DWORD_ENDN_0;
      params->endian.y_rgb_qword_endn = GSP_R5P0_QWORD_ENDN_0;
      params->endian.a_swap_mode = GSP_R5P0_A_SWAP_RGBA;
      break;
    case HAL_PIXEL_FORMAT_RGB_565:
      params->endian.rgb_swap_mode = GSP_R5P0_RGB_SWP_RGB;
      break;
    default:
      ALOGE("osd configEndian, unsupport format=0x%x.", format);
      break;
  }

  ALOGI_IF(mDebugFlag, "osd plane y_rgb_word_endn: %d, uv_word_endn: %d",
           params->endian.y_rgb_word_endn, params->endian.uv_word_endn);

  ALOGI_IF(mDebugFlag, "osd plane rgb_swap_mode: %d, a_swap_mode: %d",
           params->endian.rgb_swap_mode, params->endian.a_swap_mode);
}

void GspR5P0PlaneOsd::configPallet(int enable) {
  struct gsp_r5p0_osd_layer_params *params = &mConfig.params;

  params->pallet_en = enable;
}

struct gsp_r5p0_osd_layer_user &GspR5P0PlaneOsd::getConfig() {
  return mConfig;
}

bool GspR5P0PlaneOsd::parcel(SprdHWLayer *layer) {
  if (layer == NULL) {
    ALOGE("osd plane parcel params layer=NULL.");
    return false;
  }

  ALOGI_IF(mDebugFlag, "osd plane start to parcel");
  hwc_layer_1_t *hwcLayer = layer->getAndroidLayer();
  enum gsp_rot_angle rot = rotationTypeConvert(hwcLayer->transform);
  struct sprdRect *srcRect = layer->getSprdSRCRect();
  struct sprdRect *dstRect = layer->getSprdFBRect();
  native_handle_t *handle = (native_handle_t *)(hwcLayer->handle);
  bool inFBC = ADP_COMPRESSED(handle);
  // uint32_t height = (inFBC ? ADP_VSTRIDE(handle):ADP_HEIGHT(handle));
  uint32_t height = ADP_HEIGHT(handle);
  uint32_t fbc_mode;
  enum gsp_r5p0_osd_layer_format format = osdFormatConvert(ADP_FORMAT(handle));

  // configure format
  configFormat(format);
  configFBC(inFBC, format);

  // configure clip size and dst size
  configSize(srcRect, dstRect, ADP_STRIDE(handle), height);
  // configure pmargb mode
  configPmargbMode(hwcLayer);

  int acquireFenceFd = hwcLayer->acquireFenceFd;
  int share_fd = ADP_BUFFD(handle);
  // configure acquire fence fd and dma buffer share fd
  configCommon(acquireFenceFd, share_fd, true);

  uint8_t alpha = hwcLayer->planeAlpha;
  // configure alpha
  configAlpha(alpha);

  configZorder(layer->getLayerIndex());

  configEndian(handle);

  configPallet(0);

  return true;
}

bool GspR5P0PlaneOsd::parcel(uint32_t w, uint32_t h) {
  configPallet(1);

  configCommon(-1, -1, false);

  configSize(w, h);

  return true;
}

}  // namespace android
