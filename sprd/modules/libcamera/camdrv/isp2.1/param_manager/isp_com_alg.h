/*
 * Copyright (C) 2012 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef _ISP_COM_ALG_H_
#define _ISP_COM_ALG_H_

#include "cmr_types.h"
#include "isp_com.h"

enum isp_interp_type {
	ISP_INTERP_UINT8 = 0,
	ISP_INTERP_UINT16 = 1,
	ISP_INTERP_UINT32 = 2,
	ISP_INTERP_INT14 = 3,
	ISP_INTERP_UINT20 = 4,
};

struct isp_gamma_curve_info {
	cmr_u32 axis[2][ISP_GAMMA_SAMPLE_NUM];
};

cmr_s32 isp_gamma_adjust(struct isp_gamma_curve_info *src_ptr0, struct isp_gamma_curve_info *src_ptr1, struct isp_gamma_curve_info *dst_ptr, struct isp_weight_value *point_ptr);

cmr_s32 isp_cmc_adjust(cmr_u16 src0[9], cmr_u16 src1[9], struct isp_sample_point_info *point_ptr, cmr_u16 dst[9]);

cmr_s32 isp_cmc_adjust_4_reduce_saturation(cmr_u16 src_cmc[9], cmr_u16 dst_cmc[9], cmr_u32 percent);

cmr_s32 isp_cce_adjust(cmr_u16 src[9], cmr_u16 coef[3], cmr_u16 dst[9], cmr_u16 base_gain);

cmr_s32 isp_lsc_adjust(void *lnc0_ptr, void *lnc1_ptr, cmr_u32 lnc_len, struct isp_weight_value *point_ptr, void *dst_lnc_ptr);

cmr_s32 isp_hue_saturation_2_gain(cmr_s32 hue, cmr_s32 saturation, struct isp_rgb_gains *gain);

cmr_s32 isp_interp_data(void *dst, void *src[2], cmr_u16 weight[2], cmr_u32 data_num, cmr_u32 data_type);

#endif
