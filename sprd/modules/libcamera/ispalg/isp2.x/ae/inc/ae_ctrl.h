/*
 * Copyright (C) 2015 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *		http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _AE_CTRL_H_
#define _AE_CTRL_H_

#include "isp_common_types.h"
#include "isp_pm.h"
#include "isp_adpt.h"
#include "sensor_drv_u.h"
#ifndef CONFIG_ISP_2_2
#include "isp_bridge.h"
#else
#include "isp_match.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifdef CONFIG_ISP_2_2
	typedef cmr_int(*func_isp_br_ioctrl) (cmr_u32 camera_id, cmr_int cmd, void *in, void *out);
#endif

	struct ae_init_in {
		cmr_u32 param_num;
		cmr_u32 dflash_num;
		struct ae_param param[AE_MAX_PARAM_NUM];
		struct ae_param flash_tuning[AE_MAX_PARAM_NUM];
		struct ae_size monitor_win_num;
		struct ae_isp_ctrl_ops isp_ops;
		struct ae_resolution_info resolution_info;
		struct third_lib_info lib_param;
		cmr_u32 camera_id;
		cmr_handle caller_handle;
		isp_ae_cb ae_set_cb;
		cmr_u32 has_force_bypass;
		struct ae_opt_info otp_info;
		cmr_handle lsc_otp_random;
		cmr_handle lsc_otp_golden;
		cmr_u32 lsc_otp_width;
		cmr_u32 lsc_otp_height;
		cmr_u8 sensor_role;		//1:master 0: slave
		cmr_u32 is_multi_mode;
		func_isp_br_ioctrl ptr_isp_br_ioctrl;
		struct sensor_otp_cust_info *otp_info_ptr;
		cmr_u8 is_master;
		cmr_u32 bakup_rgb_gain;
	};

	struct ae_init_out {
		cmr_u32 cur_index;
		cmr_u32 cur_exposure;
		cmr_u32 cur_again;
		cmr_u32 cur_dgain;
		cmr_u32 cur_dummy;
		cmr_u32 flash_ver;
	};

	struct ae_calc_in {
		cmr_u32 stat_fmt;
		union {
			cmr_u32 *stat_img;
			cmr_u32 *rgb_stat_img;
		};
		cmr_u32 *yiq_stat_img;
		struct ae_binning_stats_info binning_stat_info;
		cmr_u32 awb_gain_r;
		cmr_u32 awb_gain_g;
		cmr_u32 awb_gain_b;
		struct ae_stat_img_info info;
		cmr_u32 sec;
		cmr_u32 usec;
		struct isp_sensor_fps_info sensor_fps;
		cmr_u32 awb_mode;
		cmr_u32 awb_cur_gain_r;
		cmr_u32 awb_cur_gain_g;
		cmr_u32 awb_cur_gain_b;
		cmr_u32 is_update;
	};

	struct ae_ctrl_param_out {
		union {
			cmr_u32 real_iso;
			cmr_u32 ae_effect;
			cmr_u32 ae_state;
			cmr_u32 flash_eb;
			cmr_u32 lum;
			cmr_u32 mode;
			cmr_s32 bv_gain;
			cmr_s32 bv_lum;
			float gain;
			float expoture;
			struct ae_calc_out ae_result;
			struct ae_get_ev ae_ev;
			struct ae_monitor_info info;
		};
	};

	cmr_s32 ae_ctrl_init(struct ae_init_in *input_ptr, cmr_handle * handle_ae, cmr_handle result);
	cmr_int ae_ctrl_deinit(cmr_handle * handle_ae);
	cmr_int ae_ctrl_ioctrl(cmr_handle handle, enum ae_io_ctrl_cmd cmd, cmr_handle in_ptr, cmr_handle out_ptr);
	cmr_int ae_ctrl_process(cmr_handle handle, struct ae_calc_in *in_param, struct ae_calc_out *result);

#ifdef __cplusplus
}
#endif
#endif
