/*
 * Copyright (C) 2012 The Android Open Source Project
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

#ifndef _ISP_COM_H_
#define _ISP_COM_H_

#ifndef WIN32
#include <sys/types.h>
#include <utils/Log.h>
#endif

#include "sprd_img.h"
#include "isp_type.h"
#include "isp_app.h"
#include "sensor_raw.h"
#include <string.h>
#include <stdlib.h>

#ifdef	 __cplusplus
extern "C" {
#endif

#ifdef CONFIG_USE_CAMERASERVER_PROC
#define CAMERA_DUMP_PATH  "/data/misc/cameraserver/"
#else
#define CAMERA_DUMP_PATH  "/data/misc/media/"
#endif
#define ISP_SLICE_WIN_NUM 0x0b
#define ISP_SLICE_WIN_NUM_V1 0x18
#define ISP_CMC_NUM 0x09
#define ISP_COLOR_TEMPRATURE_NUM 0x09
#define ISP_RAWAEM_BUF_SIZE   (4*3*1024)
#define ISP_BQ_AEM_CNT                      3
#define ISP_INPUT_SIZE_NUM_MAX 0x09
#define ISP_GAMMA_SAMPLE_NUM 26
#define ISP_CCE_COEF_COLOR_CAST 0
#define ISP_CCE_COEF_GAIN_OFFSET 1
#define CLIP(in, bottom, top) {if(in<bottom) in=bottom; if(in>top) in=top;}
	typedef cmr_int(*io_fun) (cmr_handle isp_alg_handle, void *param_ptr, cmr_s32(*call_back) ());

	enum {
		ISP_ALG_SINGLE = 0,
		ISP_ALG_DUAL_NORMAL,
		ISP_ALG_DUAL_SBS,
		ISP_ALG_CAMERA_MAX
	};

	struct isp_awb_statistic_info {
		cmr_u32 r_info[1024];
		cmr_u32 g_info[1024];
		cmr_u32 b_info[1024];
		cmr_u32 sec;
		cmr_u32 usec;
	};

	struct isp_system {
		isp_handle caller_id;
		proc_callback callback;
		cmr_u32 isp_callback_bypass;
		pthread_t monitor_thread;
		isp_handle monitor_queue;
		cmr_u32 monitor_status;

		isp_handle thread_ctrl;
		isp_handle thread_afl_proc;
		isp_handle thread_af_proc;
		isp_handle thread_awb_proc;
		struct isp_ops ops;
	};

	struct isp_otp_info {
		struct isp_data_info lsc;
		struct isp_data_info awb;

		void *lsc_random;
		void *lsc_golden;
		cmr_u32 width;
		cmr_u32 height;
	};

	struct isp_ae_info {
		cmr_s32 bv;
		float gain;
		float exposure;
		float f_value;
		cmr_u32 stable;
	};

	typedef struct {
		/* isp_ctrl private */
#ifndef WIN32
		struct isp_system system;
#endif
		cmr_u32 camera_id;
		uint isp_mode;

		//new param
		void *dev_access_handle;
		void *alg_fw_handle;
		//void *isp_otp_handle;

		/* isp_driver */
		void *handle_device;

		/* 4A algorithm */
		void *handle_ae;
		void *handle_af;
		void *handle_awb;
		void *handle_smart;
		void *handle_lsc_adv;

		/* isp param manager */
		void *handle_pm;

		/* sensor param */
		cmr_u32 param_index;
		struct sensor_raw_resolution_info
		 input_size_trim[ISP_INPUT_SIZE_NUM_MAX];
		struct sensor_raw_ioctrl *ioctrl_ptr;

		cmr_u32 alc_awb;
		cmr_s32 awb_pg_flag;
		cmr_u8 *log_alc_awb;
		cmr_u32 log_alc_awb_size;
		cmr_u8 *log_alc_lsc;
		cmr_u32 log_alc_lsc_size;
		cmr_u8 *log_alc;
		cmr_u32 log_alc_size;
		cmr_u8 *log_alc_ae;
		cmr_u32 log_alc_ae_size;

		struct awb_lib_fun *awb_lib_fun;
		struct ae_lib_fun *ae_lib_fun;
		struct af_lib_fun *af_lib_fun;

		struct isp_ops ops;

		struct sensor_raw_info *sn_raw_info;

		/*for new param struct */
		struct isp_data_info isp_init_data[MAX_MODE_NUM];
		struct isp_data_info isp_update_data[MAX_MODE_NUM];	/*for isp_tool */

		cmr_u32 gamma_sof_cnt;
		cmr_u32 gamma_sof_cnt_eb;
		cmr_u32 update_gamma_eb;
		cmr_u32 mode_flag;
		cmr_u32 scene_flag;
		cmr_u32 multi_nr_flag;
		cmr_s8 *sensor_name;
	} isp_ctrl_context;

	struct afl_ctrl_proc_out {
		cmr_int flag;
		cmr_int cur_flicker;
	};

	struct isp_anti_flicker_cfg {
		cmr_u32 bypass;
		pthread_mutex_t status_lock;
		cmr_u32 mode;
		cmr_u32 skip_frame_num;
		cmr_u32 line_step;
		cmr_u32 frame_num;
		cmr_u32 vheight;
		cmr_u32 start_col;
		cmr_u32 end_col;
		void *addr;
		cmr_handle thr_handle;
		cmr_handle caller_handle;
		cmr_uint vir_addr;
		cmr_uint height;
		cmr_uint width;
		cmr_uint skip_num_clr;
		cmr_uint afl_glb_total_num;
		cmr_uint afl_region_total_num;
		struct afl_ctrl_proc_out proc_out;
		isp_afl_cb afl_set_cb;
		cmr_int flag;
		cmr_int cur_flicker;
		cmr_s8 version;
	};

	struct isp_antiflicker_param {
		cmr_u32 normal_50hz_thrd;
		cmr_u32 lowlight_50hz_thrd;
		cmr_u32 normal_60hz_thrd;
		cmr_u32 lowlight_60hz_thrd;
	};

#ifdef	 __cplusplus
}
#endif
#endif
