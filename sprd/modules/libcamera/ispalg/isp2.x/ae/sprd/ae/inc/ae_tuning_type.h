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

#ifndef _AE_TUNING_TYPE_H_
#define _AE_TUNING_TYPE_H_
#include "ae_common.h"

#define MULAES_CFG_NUM AE_CFG_NUM
#define REGION_CFG_NUM AE_CFG_NUM
#define FLAT_CFG_NUM AE_CFG_NUM

struct ae_param_tmp_001 {
	cmr_u32 version;
	cmr_u32 verify;
	cmr_u32 alg_id;
	cmr_u32 target_lum;
	cmr_u32 target_lum_zone;	// x16
	cmr_u32 convergence_speed;	// x16
	cmr_u32 flicker_index;
	cmr_u32 min_line;
	cmr_u32 start_index;
	cmr_u32 exp_skip_num;
	cmr_u32 gain_skip_num;
	struct ae_stat_req stat_req;
	struct ae_flash_tuning flash_tuning;
	struct touch_zone touch_param;
	struct ae_ev_table ev_table;
};

struct ae_param_tmp_002 {
	struct ae_exp_anti exp_anti;
	struct ae_ev_cali ev_cali;
	struct ae_convergence_parm cvgn_param[AE_CVGN_NUM];
};

struct mulaes_cfg {
	cmr_s16 x_idx;
	cmr_s16 y_lum;
};

struct mulaes_tuning_param {
	cmr_u8 enable;
	cmr_u8 num;
	cmr_u16 reserved;			/*1 * 4bytes */
	struct mulaes_cfg cfg[MULAES_CFG_NUM];	/*8 * 4bytes */
};								/*9 * 4bytes */

typedef struct {
	struct ae_range region_thrd[6];	/*u d l r */
	cmr_s16 up_max;
	cmr_s16 dwn_max;
	cmr_s16 vote_region[6];		/*u d l r */
} region_cfg;					/*16 * 4bytes */

struct region_tuning_param {
	cmr_u8 enable;
	cmr_u8 num;
	cmr_u16 reserved;			/*1 * 4bytes */
	region_cfg cfg_info[REGION_CFG_NUM];	/*total 8 group: 128 * 4bytes */
	struct ae_piecewise_func input_piecewise;	/*17 * 4bytes */
	struct ae_piecewise_func u_out_piecewise;	/*17 * 4bytes */
	struct ae_piecewise_func d_out_piecewise;	/*17 * 4bytes */
};								/*180 * 4bytes */

typedef struct {
	cmr_s16 thrd[2];
	cmr_s16 offset[2];
} flat_cfg;						/*2 * 4bytes */

struct flat_tuning_param {
	/*1 * 4bytes */
	cmr_u8 enable;
	cmr_u8 num;
	cmr_u16 reserved;
	/*flat tune param; total 8 group */
	flat_cfg cfg_info[FLAT_CFG_NUM];	/*16 * 4bytes */
	struct ae_piecewise_func out_piecewise;	/*17 * 4bytes */
	struct ae_piecewise_func in_piecewise;	/*17 * 4bytes */
};								/*51 * 4bytes */

struct face_tuning_param {
	cmr_u8 face_tuning_enable;
	cmr_u8 face_target;			//except to get the face lum
	cmr_u8 face_tuning_lum1;	// scope is [0,256]
	cmr_u8 face_tuning_lum2;	//if face lum > this value, offset will set to be 0
	cmr_u16 cur_offset_weight;	//10~100 will trans 0~1
	cmr_u16 reserved[41];		//?
};

struct ae_touch_param {
	cmr_u8 win2_weight;			//for touch ae
	cmr_u8 enable;				//for touch ae
	cmr_u8 win1_weight;			//for touch ae
	cmr_u8 reserved;			//for touch ae
	struct ae_size touch_tuning_win;	//for touch ae
};

struct ae_face_tune_param {
	cmr_s32 param_face_weight;	/* The ratio of face area weight (in percent) */
	cmr_s32 param_convergence_speed;	/* AE convergence speed */
	cmr_s32 param_lock_ae;		/* frames to lock AE */
	cmr_s32 param_lock_weight_has_face;	/* frames to lock the weight table, when has faces */
	cmr_s32 param_lock_weight_no_face;	/* frames to lock the weight table, when no faces */
	cmr_s32 param_shrink_face_ratio;	/* The ratio to shrink face area. In percent */
};

struct ae_hdr_tuning_param {
	cmr_s32 ev_minus_offset;
	cmr_s32 ev_plus_offset;
};

struct ae_flash_swith_param {
	cmr_s32 flash_open_thr;
	cmr_s32 flash_close_thr;
};

struct ae_flash_control_param {
	cmr_u8 pre_flash_skip;
	cmr_u8 aem_effect_delay;
	cmr_u8 pre_open_count;
	cmr_u8 pre_close_count;
	cmr_u8 main_flash_set_count;
	cmr_u8 main_capture_count;
	cmr_u8 main_flash_notify_delay;
	cmr_u8 reserved;
};

struct ae_video_set_fps_param {
	cmr_s32 ae_video_fps_thr_low;
	cmr_s32 ae_video_fps_thr_high;
};

struct ae_tuning_param {		//total bytes must be 263480
	cmr_u32 version;
	cmr_u32 verify;
	cmr_u32 alg_id;
	cmr_u32 target_lum;
	cmr_u32 target_lum_zone;	// x16
	cmr_u8 convergence_speed;
	cmr_u8 iso_special_mode;
	cmr_u16 reserved1;
	cmr_u32 flicker_index;
	cmr_u32 min_line;
	cmr_u32 start_index;
	cmr_u32 exp_skip_num;
	cmr_u32 gain_skip_num;
	struct ae_stat_req stat_req;
	struct ae_flash_tuning flash_tuning;
	struct touch_zone touch_param;
	struct ae_ev_table ev_table;
	struct ae_exp_gain_table ae_table[AE_FLICKER_NUM][AE_ISO_NUM_NEW];
	struct ae_exp_gain_table backup_ae_table[AE_FLICKER_NUM][AE_ISO_NUM_NEW];
	struct ae_weight_table weight_table[AE_WEIGHT_TABLE_NUM];
	struct ae_scene_info scene_info[AE_SCENE_NUM];
	struct ae_auto_iso_tab auto_iso_tab;
	struct ae_exp_anti exp_anti;
	struct ae_ev_cali ev_cali;
	struct ae_convergence_parm cvgn_param[AE_CVGN_NUM];
	struct ae_touch_param touch_info;	/*it is in here,just for compatible; 3 * 4bytes */
	struct ae_face_tune_param face_info;

	/*13 * 4bytes */
	cmr_u8 monitor_mode;		/*0: single, 1: continue */
	cmr_u8 ae_tbl_exp_mode;		/*0: ae table exposure is exposure time; 1: ae table exposure is exposure line */
	cmr_u8 enter_skip_num;		/*AE alg skip frame as entering camera */
	cmr_u8 cnvg_stride_ev_num;
	cmr_s8 cnvg_stride_ev[32];
	cmr_s8 stable_zone_ev[16];

	struct ae_sensor_cfg sensor_cfg;	/*sensor cfg information: 2 * 4bytes */

	struct ae_lv_calibration lv_cali;	/*1 * 4bytes */
	/*scene detect and others alg */
	/*for touch info */

	struct flat_tuning_param flat_param;	/*51 * 4bytes */
	struct ae_flash_tuning_param flash_param;	/*1 * 4bytes */
	struct region_tuning_param region_param;	/*180 * 4bytes */
	struct mulaes_tuning_param mulaes_param;	/*9 * 4bytes */
	struct face_tuning_param face_param;
	struct ae_hdr_tuning_param hdr_param;
	struct ae_flash_swith_param flash_swith_param;
	struct ae_flash_control_param flash_control_param;
	struct ae_video_set_fps_param ae_video_fps;
	cmr_u32 reserved[2018];
};

#endif
