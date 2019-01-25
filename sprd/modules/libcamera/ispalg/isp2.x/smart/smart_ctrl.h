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

#ifndef _ISP_SMART_CTL_H_
#define _ISP_SMART_CTL_H_

#include "isp_type.h"
#include "sensor_raw.h"
#include "isp_com.h"
#include "isp_awb_types.h"

#ifdef	 __cplusplus
extern "C" {
#endif

#define SMART_CMD_MASK        0x0F00
#define SMART_WEIGHT_UNIT	  256
#define SMART_MAX_WORK_MODE		16

	typedef void *isp_smart_handle_t;
	typedef void *smart_handle_t;
	typedef cmr_int(*isp_smart_cb) (cmr_handle handle, cmr_int type, void *param0, void *param1);

/* smart callback command */
	enum ispalg_smart_set_cmd {
		ISP_SMART_SET_COMMON,
		ISP_SMART_SET_GAMMA_CUR,
	};

	enum {
		ISP_SMART_IOCTL_GET_BASE = 0x100,
		ISP_SMART_IOCTL_GET_PARAM,
		ISP_SMART_IOCTL_GET_UPDATE_PARAM,
		ISP_SMART_IOCTL_SET_BASE = 0x200,
		ISP_SMART_IOCTL_SET_PARAM,
		ISP_SMART_IOCTL_SET_WORK_MODE,
		ISP_SMART_IOCTL_SET_FLASH_MODE,
		ISP_SMART_IOCTL_CMD_MAX,
	};

	enum smart_ctrl_flash_mode {
		SMART_CTRL_FLASH_CLOSE = 0x0,
		SMART_CTRL_FLASH_PRE = 0x1,
		SMART_CTRL_FLASH_MAIN = 0x2,
		SMART_CTRL_FLASH_END = 0x3
	};

	struct smart_component_result {
		cmr_u32 y_type;			//0: normal data(directly to driver); 1: index (few block support);
		// 2: index and weight (few block support); 3 gain: gain (new lens shading support)
		cmr_u32 x_type;
		cmr_u32 flash;
		cmr_u32 offset;
		cmr_u32 size;			//if data is not NULL, use the data as the output buffer
		cmr_s32 fix_data[20];
		void *data;
	};

	struct smart_block_result {
		cmr_u32 block_id;
		cmr_u32 smart_id;
		cmr_u32 update;
		cmr_u32 component_num;
		cmr_u32 mode_flag;
		cmr_u32 scene_flag;
		cmr_u32 mode_flag_changed;
		struct smart_component_result component[4];
	};

	struct smart_tuning_param {
		cmr_u32 version;
		cmr_u32 bypass;
		struct isp_data_info data;
	};

	struct smart_init_param {
		cmr_handle caller_handle;
		isp_smart_cb smart_set_cb;
		struct smart_tuning_param tuning_param[SMART_MAX_WORK_MODE];
		cmr_u32 camera_id;
	};

	struct smart_calc_param {
		cmr_s32 bv;
		cmr_s32 bv_gain;
		cmr_u32 stable;
		cmr_u32 ct;
		cmr_u32 alc_awb;
		cmr_handle handle_pm;
		cmr_s32 flash_ratio;
		cmr_s32 flash_ratio1;
		void *gamma_tab;
	};

	struct smart_proc_input {
		cmr_handle handle_pm;
		cmr_u32 alc_awb;
		struct smart_calc_param cal_para;
		cmr_u32 mode_flag;
		cmr_u32 scene_flag;
		cmr_u32 lsc_sprd_version;	// LSC version of Spreadtrum
		cmr_u32 *r_info;
		cmr_u32 *g_info;
		cmr_u32 *b_info;
		cmr_u32 win_num_w;
		cmr_u32 win_num_h;
		cmr_u32 win_size_w;
		cmr_u32 win_size_h;
		cmr_u32 aem_shift;
		cmr_u8 *log;
		cmr_u32 size;
		cmr_u8 lock_nlm;
		cmr_u8 lock_ee;
		cmr_u8 lock_precdn;
		cmr_u8 lock_cdn;
		cmr_u8 lock_postcdn;
		cmr_u8 lock_ccnr;
		cmr_u8 lock_ynr;
	};

	struct smart_calc_result {
		struct smart_block_result *block_result;
		cmr_u32 counts;
	};

	struct nr_data {
		cmr_u32 ppi[3];
		cmr_u32 bayer_nr[3];
		cmr_u32 rgb_dither[3];
		cmr_u32 bpc[3];
		cmr_u32 grgb[3];
		cmr_u32 cfae[3];
		cmr_u32 rgb_afm[3];
		cmr_u32 uvdiv[3];
		cmr_u32 dnr3_pre[3];
		cmr_u32 dnr3_cap[3];
		cmr_u32 edge[3];
		cmr_u32 precdn[3];
		cmr_u32 ynr[3];
		cmr_u32 cdn[3];
		cmr_u32 postcdn[3];
		cmr_u32 ccnr[3];
		cmr_u32 iir_yrandom[3];
		cmr_u32 noisefilter[3];
	};

	typedef struct {
		cmr_u64 u8Hist[256];
		cmr_u32 u4RespCurve[256];
		cmr_u8 uOutputGamma[3][256];
		cmr_u8 uLowPT;
		cmr_u8 uHighPT;
		cmr_u8 uFinalLowBin;
		cmr_u8 uFinalHighBin;
	} smart_gamma_debuginfo;

	typedef struct {
		struct nr_data nr_param;
		smart_gamma_debuginfo smt_gma;
	} smart_debuginfo;

	cmr_s32 smart_ctl_ioctl(smart_handle_t handle, cmr_u32 cmd, void *param, void *result);
	cmr_s32 smart_ctl_block_eb(smart_handle_t handle, void *block_eb, cmr_u32 is_eb);
	cmr_s32 smart_ctl_block_enable_recover(smart_handle_t handle, cmr_u32 smart_id);
	cmr_s32 smart_ctl_block_disable(smart_handle_t handle, cmr_u32 smart_id);
	cmr_s32 smart_ctl_NR_block_disable(smart_handle_t handle, cmr_u32 is_diseb);

	smart_handle_t smart_ctl_init(struct smart_init_param *param, void *result);
	cmr_s32 smart_ctl_deinit(smart_handle_t * handle, void *param, void *result);
	cmr_int _smart_calc(cmr_handle handle_smart, struct smart_proc_input *in_ptr);

#ifdef	 __cplusplus
}
#endif
#endif
