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
#ifndef _ISP_TYPE_H_
#define _ISP_TYPE_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/types.h>
#include "cmr_types.h"

#define ISP_GRAB_BUF_MAX 16
#define VIDEO_USE     1

struct isp_sample_point_info {
	cmr_s32 x0;
	cmr_s32 x1;
	cmr_u32 weight0;
	cmr_u32 weight1;
};

struct isp_data_info {
	cmr_u32 size;
	void *data_ptr;
	void *param_ptr;
};
struct isp_data_bin_info {
	cmr_u32 size;
	cmr_u32 offset;
};

struct isp_pos {
	cmr_s32 x;
	cmr_s32 y;
};
struct isp_point {
	cmr_s16 x;
	cmr_s16 y;
};
struct isp_size {
	cmr_u32 w;
	cmr_u32 h;
};

struct isp_buffer_size_info {
	cmr_u32 pitch;
	cmr_u32 count_lines;
};

struct isp_trim_size {
	cmr_s32 x;
	cmr_s32 y;
	cmr_s32 w;
	cmr_s32 h;
};

struct isp_rect {
	cmr_s32 st_x;
	cmr_s32 st_y;
	cmr_u32 width;
	cmr_u32 height;
};

struct isp_pos_rect {
	cmr_s32 start_x;
	cmr_s32 start_y;
	cmr_u32 end_x;
	cmr_u32 end_y;
};

struct isp_rgb_gains {
	cmr_u32 gain_r;
	cmr_u32 gain_g;
	cmr_u32 gain_b;
	cmr_u32 reserved;
};

struct isp_rgb_offset {
	cmr_s32 offset_r;
	cmr_s32 offset_g;
	cmr_s32 offset_b;
	cmr_s32 reserved;
};

struct isp_range {
	cmr_s32 min;
	cmr_s32 max;
};

struct isp_weight_value {
	cmr_s16 value[2];
	cmr_s16 weight[2];
};

struct isp_yuv {
	cmr_u16 y;
	cmr_u16 u;
	cmr_u16 v;
	cmr_u16 reserved;
};
struct isp_curve_coeff {
	cmr_u32 p1;
	cmr_u32 p2;
};
struct isp_bin_param {
	cmr_u16 hx;
	cmr_u16 vx;
};

struct isp_sample {
	cmr_s16 x;
	cmr_s16 y;
};

struct isp_calibration_awb_gain {
	cmr_int r;
	cmr_int g;
	cmr_int b;
};

struct isp_awb_gain {
	cmr_u16 r;
	cmr_u16 g;
	cmr_u16 b;
};

#ifdef __cplusplus
}
#endif
#endif
