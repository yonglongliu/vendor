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
#ifndef _ISP_BRIDGE_H_
#define _ISP_BRIDGE_H_
/*----------------------------------------------------------------------------*
**				Dependencies					*
**---------------------------------------------------------------------------*/

#include "isp_common_types.h"

enum isp_br_ctrl_cmd_type {
	GET_MATCH_AE_DATA,
	SET_MATCH_AE_DATA,
	GET_MATCH_AWB_DATA,
	SET_MATCH_AWB_DATA,
};

struct awb_match_data {
	struct isp_awb_gain gain;
	struct isp_awb_gain gain_balanced;
	struct isp_awb_gain gain_flash_off;
	struct isp_awb_gain gain_capture;
	cmr_u32 ct;
	cmr_u32 ct_flash_off;
	cmr_u32 ct_capture;
	cmr_u32 is_update;
	cmr_u16 light_source;
	cmr_u32 awb_states;
	cmr_u16 awb_decision;
};

struct ae_match_data {
	cmr_u32 iso;
	cmr_u32 exif_iso;
	cmr_u32 exposure_time;
	cmr_u32 exposure_line;
	cmr_u32 sensor_ad_gain;
	cmr_u32 isp_d_gain;
	cmr_u16 uw_cur_fps;
	cmr_s32 bv_val;
	cmr_u8 uc_sensor_mode;
	cmr_s32 master_bgbv;
	cmr_u32 master_curmean;
};

struct match_data_param {
	struct awb_match_data awb_data;
	struct ae_match_data ae_data;
};

cmr_handle isp_br_get_3a_handle(cmr_u32 camera_id);
cmr_int isp_br_init(cmr_u32 camera_id, cmr_handle isp_3a_handle);
cmr_int isp_br_deinit(cmr_u32 camera_id);
cmr_int isp_br_ioctrl(cmr_u32 camera_id, cmr_int cmd, void *in, void *out);
cmr_int isp_br_save_dual_otp(cmr_u32 camera_id, struct sensor_dual_otp_info *dual_otp);
cmr_int isp_br_get_dual_otp(cmr_u32 camera_id, struct sensor_dual_otp_info **dual_otp);

#endif
