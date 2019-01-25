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
#ifndef GSPR5P0PLANE_GSPR5P0PLANEOSD_H_
#define GSPR5P0PLANE_GSPR5P0PLANEOSD_H_

#include "GspR5P0Plane.h"
#include "gralloc_public.h"
#include "SprdHWLayer.h"
#include "gsp_r5p0_cfg.h"

namespace android {

class GspR5P0PlaneOsd : public GspR5P0Plane {
 public:
  GspR5P0PlaneOsd(bool async, const GspRangeSize &range);
  virtual ~GspR5P0PlaneOsd() {}

  void reset(int flag);

  bool adapt(SprdHWLayer *layer, int index);

  bool parcel(SprdHWLayer *layer);

  bool parcel(uint32_t w, uint32_t h);

  struct gsp_r5p0_osd_layer_user &getConfig();

 private:
  void configCommon(int wait_fd, int share_fd, int enable);

  void configSize(struct sprdRect *srcRect, struct sprdRect *dstRect,
                  uint32_t pitch, uint32_t height);

  void configSize(uint32_t w, uint32_t h);

  void configFormat(enum gsp_r5p0_osd_layer_format format);

  void configAlpha(uint8_t alpha);

  void configZorder(uint8_t zorder);

  void configPmargbMode(hwc_layer_1_t *layer);

  void configEndian(native_handle_t *handle);

  void configPallet(int enable);

  void configFBC(bool inFBC, int format);

  bool checkFBC(uint32_t pitch, uint32_t height,
                enum gsp_r5p0_osd_layer_format format, bool inFBC);

  GspRangeSize mRangeSize;

  struct gsp_r5p0_osd_layer_user mConfig;
};
}  // namespace android

#endif  // GSPR5P0PLANE_GSPR5P0PLANEOSD_H_
