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
#ifndef _SPRD_AF_CTRL_V1_H
#define _SPRD_AF_CTRL_V1_H

#include <utils/Timers.h>
#include <cutils/properties.h>

#include "AFv1_Common.h"
#include "AFv1_Interface.h"
// #include "AFv1_Tune.h"

#include "aft_interface.h"

#define AF_SYS_VERSION "-20171130-00"
#define AF_SAVE_MLOG_STR "persist.sys.isp.af.mlog"	/*save/no */
#define AF_WAIT_CAF_SEC 3		//1s == (1000 * 1000 * 1000)ns
#define AF_WAIT_CAF_NSEC 0		//this macro should be less than 1000 * 1000 * 1000

enum afv1_bool {
	AFV1_FALSE = 0,
	AFV1_TRUE,
};

enum afv1_err_type {
	AFV1_SUCCESS = 0x00,
	AFV1_ERROR,
	AFV1_ERR_MAX
};

enum _lock_block {
	LOCK_AE = 0x01,
	LOCK_LSC = 0x02,
	LOCK_NLM = 0x04,
	LOCK_AWB = 0x08,
};

enum af_state {
	STATE_MANUAL = 0,
	STATE_NORMAL_AF,			// single af or touch af
	STATE_CAF,
	STATE_RECORD_CAF,
	STATE_FAF,
	STATE_FULLSCAN,
	STATE_PICTURE,
};

enum focus_state {
	AF_IDLE,
	AF_SEARCHING,
};

enum dcam_after_vcm {
	DCAM_AFTER_VCM_NO = 0,
	DCAM_AFTER_VCM_YES
};

typedef struct _isp_info {
	cmr_u32 width;
	cmr_u32 height;
	cmr_u32 win_num;
} isp_info_t;

#define MAX_ROI_NUM     32		// arbitrary, more than hardware wins

typedef struct _roi_rgb_y {
	cmr_s32 num;
	cmr_u32 Y_sum[MAX_ROI_NUM];
	cmr_u32 R_sum[MAX_ROI_NUM];
	cmr_u32 G_sum[MAX_ROI_NUM];
	cmr_u32 B_sum[MAX_ROI_NUM];
} roi_rgb_y_t;

typedef struct _lens_info {
	cmr_u16 pos;
} lens_info_t;

typedef struct _ae_info {
	struct af_ae_calc_out ae_report;
	cmr_u32 win_size;
} ae_info_t;

typedef struct _awb_info {
	cmr_u32 r_gain;
	cmr_u32 g_gain;
	cmr_u32 b_gain;
} awb_info_t;

typedef struct _isp_awb_statistic_hist_info {
	cmr_u32 r_info[1024];
	cmr_u32 g_info[1024];
	cmr_u32 b_info[1024];
	cmr_u32 r_hist[256];
	cmr_u32 g_hist[256];
	cmr_u32 b_hist[256];
	cmr_u32 y_hist[256];
} isp_awb_statistic_hist_info_t;

typedef struct _win_coord {
	cmr_u32 start_x;
	cmr_u32 start_y;
	cmr_u32 end_x;
	cmr_u32 end_y;
} win_coord_t;

typedef struct _roi_info {
	cmr_u32 num;
	win_coord_t win[MAX_ROI_NUM];
} roi_info_t;

enum filter_type {
	FILTER_SOBEL5 = 0,
	FILTER_SOBEL9,
	FILTER_SPSMD,
	FILTER_ENHANCED,
	FILTER_NUM
};

#define SOBEL5_BIT  (1 << FILTER_SOBEL5)
#define SOBEL9_BIT  (1 << FILTER_SOBEL9)
#define SPSMD_BIT   (1 << FILTER_SPSMD)
#define ENHANCED_BIT   (1 << FILTER_ENHANCED)

// af lib
typedef struct _af_lib_ops {
	void *(*init) (AF_Ctrl_Ops * AF_Ops, af_tuning_block_param * af_tuning_data, haf_tuning_param_t * haf_tune_data, cmr_u32 * dump_info_len, char *sys_version);
	 cmr_u8(*deinit) (void *handle);
	 cmr_u8(*calc) (void *handle);

	 cmr_u8(*ioctrl) (void *handle, cmr_u32 cmd, void *param);
} af_lib_ops_t;

#define AF_LIB "libspafv1.so"

// caf trigger
typedef struct _caf_trigger_ops {
	aft_proc_handle_t handle;
	struct aft_ae_skip_info ae_skip_info;

	 cmr_s32(*init) (struct aft_tuning_block_param * init_param, aft_proc_handle_t * handle);
	 cmr_s32(*deinit) (aft_proc_handle_t * handle);

	 cmr_s32(*calc) (aft_proc_handle_t * handle, struct aft_proc_calc_param * alg_calc_in, struct aft_proc_result * alg_calc_result);
	 cmr_s32(*ioctrl) (aft_proc_handle_t * handle, enum aft_cmd cmd, void *param0, void *param1);
} caf_trigger_ops_t;

#define CAF_TRIGGER_LIB "libspcaftrigger.so"

typedef struct _ae_cali {
	cmr_u32 r_avg[9];
	cmr_u32 g_avg[9];
	cmr_u32 b_avg[9];
	cmr_u32 r_avg_all;
	cmr_u32 g_avg_all;
	cmr_u32 b_avg_all;
} ae_cali_t;

struct af_fullscan_info {
	/* Register Parameters */
	/* These params will depend on the AF setting */
	cmr_u8 row_num;				/* The number of AF windows with row (i.e. vertical) *//* depend on the AF Scanning */
	cmr_u8 column_num;			/* The number of AF windows with row (i.e. horizontal) *//* depend on the AF Scanning */
	cmr_u32 *win_peak_pos;		/* The seqence of peak position which be provided via struct isp_af_fullscan_info *//* depend on the AF Scanning */
	cmr_u16 vcm_dac_up_bound;
	cmr_u16 vcm_dac_low_bound;
	cmr_u16 boundary_ratio;		/* (Unit : Percentage) *//* depend on the AF Scanning */
	cmr_u32 af_peak_pos;
	cmr_u32 near_peak_pos;
	cmr_u32 far_peak_pos;
	cmr_u32 distance_reminder;
	cmr_u32 reserved[16];
};

typedef struct _af_fv_info {
	cmr_u64 af_fv0[10];			// [10]:10 ROI, sum of FV0
	cmr_u64 af_fv1[10];			// [10]:10 ROI, sum of FV1
} af_fv;

typedef struct _afm_tuning_param_sharkl2 {
	cmr_u8 iir_level;
	cmr_u8 nr_mode;
	cmr_u8 cw_mode;
	cmr_u8 fv0_e;
	cmr_u8 fv1_e;
	cmr_u8 dummy[3];			// 4 bytes align
} afm_tuning_sharkl2;

struct af_enhanced_module_info_u {
	cmr_u8 chl_sel;
	cmr_u8 nr_mode;
	cmr_u8 center_weight;
	cmr_u8 fv_enhanced_mode[2];
	cmr_u8 clip_en[2];
	cmr_u32 max_th[2];
	cmr_u32 min_th[2];
	cmr_u8 fv_shift[2];
	cmr_s8 fv1_coeff[36];
};

struct af_iir_nr_info_u {
	cmr_u8 iir_nr_en;
	cmr_s16 iir_g0;
	cmr_s16 iir_c1;
	cmr_s16 iir_c2;
	cmr_s16 iir_c3;
	cmr_s16 iir_c4;
	cmr_s16 iir_c5;
	cmr_s16 iir_g1;
	cmr_s16 iir_c6;
	cmr_s16 iir_c7;
	cmr_s16 iir_c8;
	cmr_s16 iir_c9;
	cmr_s16 iir_c10;
};

typedef struct _af_ctrl {
	void *af_alg_cxt;			// AF_Data fv;
	cmr_u32 af_dump_info_len;
	cmr_u32 state;				// enum af_state state;
	cmr_u32 pre_state;			// enum af_state pre_state;
	cmr_u32 focus_state;		// enum caf_state caf_state;
	cmr_u32 algo_mode;			// eAF_MODE algo_mode;
	cmr_u32 takePicture_timeout;
	cmr_u32 request_mode;
	cmr_u64 vcm_timestamp;
	cmr_u64 dcam_timestamp;
	cmr_u64 takepic_timestamp;
	cmr_u32 Y_sum_trigger;
	cmr_u32 Y_sum_normalize;
	cmr_u64 fv_combine[T_TOTAL_FILTER_TYPE];
	af_fv af_fv_val;
	struct afctrl_gsensor_info gsensor_info;
	struct afctrl_face_info face_info;
	isp_info_t isp_info;
	lens_info_t lens;
	cmr_s32 flash_on;
	roi_info_t roi;
	roi_rgb_y_t roi_RGBY;
	ae_info_t ae;
	awb_info_t awb;
	pd_algo_result_t pd;
	cmr_s32 ae_lock_num;
	cmr_s32 awb_lock_num;
	cmr_s32 lsc_lock_num;
	cmr_s32 nlm_lock_num;
	cmr_s32 ae_partial_lock_num;
	void *trig_lib;
	caf_trigger_ops_t trig_ops;
	void *af_lib;
	af_lib_ops_t af_ops;
	ae_cali_t ae_cali_data;
	cmr_u32 vcm_stable;
	cmr_u32 defocus;
	cmr_u8 bypass;
	cmr_u32 force_trigger;
	cmr_u32 cb_trigger;
	cmr_u32 ts_counter;
	// non-zsl,easy for motor moving and capturing
	cmr_u8 test_loop_quit;
	pthread_t test_loop_handle;
	cmr_handle caller;
	cmr_u32 win_peak_pos[MULTI_STATIC_TOTAL];
	cmr_u32 is_high_fps;
	cmr_u32 afm_skip_num;
	afm_tuning_sharkl2 afm_tuning;
	struct aft_proc_calc_param prm_trigger;
	isp_awb_statistic_hist_info_t rgb_stat;
	cmr_u32 trigger_source_type;
	char AF_MODE[PROPERTY_VALUE_MAX];
	struct afctrl_otp_info otp_info;
	cmr_u32 is_multi_mode;
	cmr_u8 *aftuning_data;
	cmr_u32 aftuning_data_len;
	cmr_u8 *pdaftuning_data;
	cmr_u32 pdaftuning_data_len;
	cmr_u8 *afttuning_data;
	cmr_u32 afttuning_data_len;
	struct afctrl_cb_ops cb_ops;
} af_ctrl_t;

typedef struct _test_mode_command {
	char *command;
	cmr_u64 key;
	void (*command_func) (af_ctrl_t * af, char *test_param);
} test_mode_command_t;

cmr_handle sprd_afv1_init(void *in, void *out);
cmr_s32 sprd_afv1_deinit(cmr_handle handle, void *param, void *result);
cmr_s32 sprd_afv1_process(void *handle, void *in, void *out);
cmr_s32 sprd_afv1_ioctrl(void *handle, cmr_s32 cmd, void *param0, void *param1);

#endif
