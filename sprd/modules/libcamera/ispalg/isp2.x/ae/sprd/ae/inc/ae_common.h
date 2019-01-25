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

#ifndef _AE_COMMON_H_
#define _AE_COMMON_H_

#ifdef CONFIG_FOR_TIZEN
#include "stdint.h"
#elif WIN32
#include "cmr_types.h"
#else
#include "cmr_types.h"
#endif

#define AEC_LINETIME_PRECESION 1000000000.0
#define AE_EXP_GAIN_TABLE_SIZE 512
#define AE_WEIGHT_TABLE_SIZE	1024
#define AE_ISO_NUM	6
#define AE_ISO_NUM_NEW 8
#define AE_SCENE_NUM   8
#define AE_FLICKER_NUM 2
#define AE_WEIGHT_TABLE_NUM 3
#define AE_EV_LEVEL_NUM 16
#define AE_PARAM_VERIFY	0x61656165
#define AE_OFFSET_NUM 20
#define AE_CVGN_NUM  4
#define AE_TABLE_32
#define AE_BAYER_CHNL_NUM 4
#define AE_PIECEWISE_MAX_NUM 16
#define AE_WEIGHT_UNIT 256
#define AE_FIX_PCT 1024
#define AE_PIECEWISE_SAMPLE_NUM 0x10
#define AE_CFG_NUM 8
#define AE_FD_NUM 20

enum ae_environ_mod {
	ae_environ_night,
	ae_environ_lowlux,
	ae_environ_normal,
	ae_environ_hightlight,
	ae_environ_num,
};

enum ae_state {
	AE_STATE_NORMAL,
	AE_STATE_LOCKED,
	//AE_STATE_PAUSE,
	AE_STATE_SEARCHING,
	AE_STATE_CONVERGED,
	AE_STATE_FLASH_REQUIRED,
	AE_STATE_PRECAPTURE,
	AE_STATE_INACTIVE,
	AE_STATE_MAX
};

enum ae_return_value {
	AE_SUCCESS = 0x00,
	AE_ERROR,
	AE_PARAM_ERROR,
	AE_PARAM_NULL,
	AE_FUN_NULL,
	AE_HANDLER_NULL,
	AE_HANDLER_ID_ERROR,
	AE_ALLOC_ERROR,
	AE_FREE_ERROR,
	AE_DO_NOT_WRITE_SENSOR,
	AE_SKIP_FRAME,
	AE_RTN_MAX
};

enum ae_calc_func_y_type {
	AE_CALC_FUNC_Y_TYPE_VALUE = 0,
	AE_CALC_FUNC_Y_TYPE_WEIGHT_VALUE = 1,
};

enum ae_iso_mode {
	AE_ISO_AUTO = 0x00,
	AE_ISO_100,
	AE_ISO_200,
	AE_ISO_400,
	AE_ISO_800,
	AE_ISO_1600,
	AE_ISO_MAX
};

enum ae_scene_mode {
	AE_SCENE_NORMAL = 0x00,
	AE_SCENE_NIGHT,
	AE_SCENE_SPORT,
	AE_SCENE_PORTRAIT,
	AE_SCENE_LANDSPACE,
	AE_SCENE_PANORAMA,
	AE_SCENE_MOD_MAX
};
enum alg_flash_type {
	FLASH_NONE,
	FLASH_PRE_BEFORE,
	FLASH_PRE_BEFORE_RECEIVE,
	FLASH_PRE,
	FLASH_PRE_RECEIVE,
	FLASH_PRE_AFTER,
	FLASH_PRE_AFTER_RECEIVE,
	FLASH_MAIN_BEFORE,
	FLASH_MAIN_BEFORE_RECEIVE,
	FLASH_MAIN,
	FLASH_MAIN_RECEIVE,
	FLASH_MAIN_AFTER,
	FLASH_MAIN_AFTER_RECEIVE,
	FLASH_LED_ON,
	FLASH_LED_OFF,
	FLASH_LED_AUTO,
	FLASH_MAX
};

enum ae_flicker_mode {
	AE_FLICKER_50HZ = 0x00,
	AE_FLICKER_60HZ,
	AE_FLICKER_OFF,
	AE_FLICKER_AUTO,
	AE_FLICKER_MAX
};

typedef cmr_handle ae_handle_t;

struct ae_ct_table {
	float ct[20];
	float rg[20];
};

struct ae_weight_value {
	cmr_s16 value[2];
	cmr_s16 weight[2];
};

struct ae_sample {
	cmr_s16 x;
	cmr_s16 y;
};

struct ae_piecewise_func {
	cmr_s32 num;
	struct ae_sample samples[AE_PIECEWISE_SAMPLE_NUM];
};

struct ae_range {
	cmr_s32 min;
	cmr_s32 max;
};

struct ae_size {
	cmr_u32 w;
	cmr_u32 h;
};

struct ae_trim {
	cmr_u32 x;
	cmr_u32 y;
	cmr_u32 w;
	cmr_u32 h;
};

struct ae_rect {
	cmr_u32 start_x;
	cmr_u32 start_y;
	cmr_u32 end_x;
	cmr_u32 end_y;
};

struct ae1_face {
	cmr_u32 start_x;
	cmr_u32 start_y;
	cmr_u32 end_x;
	cmr_u32 end_y;				/*4 x 4bytes */
	cmr_s32 pose;				/* face pose: frontal, half-profile, full-profile */
};

struct ae1_face_info {
	cmr_u16 face_num;
	cmr_u16 reserved;			/*1 x 4bytes */
	cmr_u32 rect[1024];			/*1024 x 4bytes */
	struct ae1_face face_area[20];	/*20 x 5 * 4bytes */
};								/*1125 x 4bytes */

struct ae1_fd_param {
	struct ae1_face_info cur_info;	/*1125 x 4bytes */
	cmr_u8 update_flag;
	cmr_u8 enable_flag;
	cmr_u16 reserved;			/*1 x 4bytes */
	cmr_u16 img_width;
	cmr_u16 img_height;			/*1 x 4bytes */
};								/*1127 x 4bytes */

struct ae_param {
	cmr_handle param;
	cmr_u32 size;
};

struct ae_exp_gain_delay_info {
	cmr_u8 group_hold_flag;
	cmr_u8 valid_exp_num;
	cmr_u8 valid_gain_num;
};

struct ae_set_fps {
	cmr_u32 min_fps;			// min fps
	cmr_u32 max_fps;			// fix fps flag
};

struct ae_exp_gain_table {
	cmr_s32 min_index;
	cmr_s32 max_index;
	cmr_u32 exposure[AE_EXP_GAIN_TABLE_SIZE];
	cmr_u32 dummy[AE_EXP_GAIN_TABLE_SIZE];
	cmr_u16 again[AE_EXP_GAIN_TABLE_SIZE];
	cmr_u16 dgain[AE_EXP_GAIN_TABLE_SIZE];
};

struct ae_weight_table {
	cmr_u8 weight[AE_WEIGHT_TABLE_SIZE];
};

struct ae_ev_table {
	cmr_s32 lum_diff[AE_EV_LEVEL_NUM];
	/* number of level */
	cmr_u32 diff_num;
	/* index of default */
	cmr_u32 default_level;
};

struct ae_flash_ctrl {
	cmr_u32 enable;
	cmr_u32 main_flash_lum;
	cmr_u32 convergence_speed;
};

struct touch_zone {
	cmr_u32 level_0_weight;
	cmr_u32 level_1_weight;
	cmr_u32 level_1_percent;	//x64
	cmr_u32 level_2_weight;
	cmr_u32 level_2_percent;	//x64
};

struct ae_flash_tuning {
	cmr_u32 exposure_index;
};

struct ae_stat_req {
	cmr_u32 mode;				//0:normal, 1:G(center area)
	cmr_u32 G_width;			//100:G mode(100x100)
};

struct ae_auto_iso_tab {
	cmr_u16 tbl[AE_FLICKER_NUM][AE_EXP_GAIN_TABLE_SIZE];
};

struct ae_ev_cali_param {
	cmr_u32 index;
	cmr_u32 lux;
	cmr_u32 lv;
};

struct ae_ev_cali {
	cmr_u32 num;
	cmr_u32 min_lum;			// close all the module of after awb module
	struct ae_ev_cali_param tab[16];	// cali EV sequence is low to high
};

struct ae_rgb_l {
	cmr_u32 r;
	cmr_u32 g;
	cmr_u32 b;
};

struct ae_opt_info {
	struct ae_rgb_l gldn_stat_info;
	struct ae_rgb_l rdm_stat_info;
};

struct ae_exp_anti {
	cmr_u32 enable;
	cmr_u8 hist_thr[40];
	cmr_u8 hist_weight[40];
	cmr_u8 pos_lut[256];
	cmr_u8 hist_thr_num;
	cmr_u8 adjust_thr;
	cmr_u8 stab_conter;
	cmr_u8 reserved1;

	cmr_u32 reserved[175];
};

struct ae_convergence_parm {
	cmr_u32 highcount;
	cmr_u32 lowcount;
	cmr_u32 highlum_offset_default[AE_OFFSET_NUM];
	cmr_u32 lowlum_offset_default[AE_OFFSET_NUM];
	cmr_u32 highlum_index[AE_OFFSET_NUM];
	cmr_u32 lowlum_index[AE_OFFSET_NUM];
};

struct ae_flash_tuning_param {
	cmr_u8 skip_num;
	cmr_u8 target_lum;
	cmr_u8 adjust_ratio;		/* 1x --> 32 */
	cmr_u8 reserved;
};

struct ae_sensor_cfg {
	cmr_u16 max_gain;			/*sensor max gain */
	cmr_u16 min_gain;			/*sensor min gain */
	cmr_u8 gain_precision;
	cmr_u8 exp_skip_num;
	cmr_u8 gain_skip_num;
	cmr_u8 min_exp_line;
};

struct ae_lv_calibration {
	cmr_u16 lux_value;
	cmr_s16 bv_value;
};

struct ae_scene_info {
	cmr_u32 enable;
	cmr_u32 scene_mode;
	cmr_u32 target_lum;
	cmr_u32 iso_index;
	cmr_u32 ev_offset;
	cmr_u32 max_fps;
	cmr_u32 min_fps;
	cmr_u32 weight_mode;
	cmr_u8 table_enable;
	cmr_u8 exp_tbl_mode;
	cmr_u16 reserved0;
	cmr_u32 reserved1;
	struct ae_exp_gain_table ae_table[AE_FLICKER_NUM];
};

struct ae_alg_init_in {
	cmr_u32 flash_version;
	cmr_u32 start_index;
	cmr_handle param_ptr;
	cmr_u32 size;
};

struct ae_alg_init_out {
	cmr_u32 start_index;
};

struct ae_alg_rgb_gain {
	cmr_u32 r;
	cmr_u32 g;
	cmr_u32 b;
};

struct ae_stats_gyro_info {
	/* Gyro data in float */
	cmr_u32 validate;
	cmr_s64 timestamp;
	float x;
	float y;
	float z;
};

struct ae_stats_accelerator_info {
	cmr_u32 validate;
	cmr_s64 timestamp;
	float x;
	float y;
	float z;
};

struct ae_stats_sensor_info {
	cmr_u32 aux_sensor_support;
	struct ae_stats_gyro_info gyro;
	struct ae_stats_accelerator_info accelerator;
};

struct ae_settings {
	cmr_u16 ver;
	cmr_u8 force_lock_ae;
	cmr_s8 lock_ae;				/* 0:unlock 1:lock 2:pause 3:wait-lock */
	cmr_s32 pause_cnt;
	cmr_s8 manual_mode;			/* 0:exp&gain       1:table-index */
	cmr_u32 exp_line;			/* set exposure lines */
	cmr_u16 gain;				/* set gain: 128 means 1 time gain , user can set any value to it */
	cmr_u16 table_idx;			/* set ae-table-index */
	cmr_s16 min_fps;			/* e.g. 2000 means 20.00fps , 0 means Auto */
	cmr_s16 max_fps;
	cmr_u8 sensor_max_fps;		/*the fps of sensor setting: it always is 30fps in normal setting */
	cmr_s8 flash;				/*flash */
	cmr_s16 flash_ration;		/* mainflash : preflash -> 1x = 32 */
	cmr_s16 flash_target;
	cmr_s8 iso;
	cmr_s8 touch_scrn_status;	//touch screen,1: touch;0:no touch
	cmr_s8 touch_tuning_enable;	//for touch ae
	cmr_s8 ev_index;			/* not real value , just index !! */
	cmr_s8 flicker;				/* 50hz 0 60hz 1 */
	cmr_s8 flicker_mode;		/* auto 0 manual 1,2 */
	cmr_s8 FD_AE;				/* 0:off; 1:on */
	cmr_s8 metering_mode;
	cmr_s8 work_mode;			/* DC DV */
	cmr_s8 scene_mode;			/* pano sports night */
	cmr_s16 intelligent_module;	/* pano sports night */
	cmr_s8 af_info;				/*AF trigger info */
	cmr_s8 reserve_case;
	cmr_u32 iso_special_mode;
	cmr_u32 iso_manual_status;	/*iso manual setting */
	cmr_u32 ev_manual_status;	/*ev manual setting */
	cmr_u8 *reserve_info;		/* reserve for future */
	cmr_s16 reserve_len;		/*len for reserve */
};

struct ae_alg_calc_param {
	struct ae_size frame_size;
	struct ae_size win_size;
	struct ae_size win_num;
	struct ae_alg_rgb_gain awb_gain;
	struct ae_exp_gain_table *ae_table;
	struct ae_size touch_tuning_win;	//for touch ae
	struct ae_trim touch_scrn_win;	//for touch ae
	cmr_u8 *weight_table;
	struct ae_stats_sensor_info aux_sensor_data;
	cmr_u32 *stat_img;
	cmr_u16 *binning_stat_data;
	struct ae_size binnig_stat_size;
	cmr_u8 monitor_shift;		//for ae monitor data overflow
	cmr_u8 win1_weight;			//for touch ae
	cmr_u8 win2_weight;			//for touch ae
	cmr_s8 target_offset;
	cmr_s16 target_lum;
	cmr_s16 target_lum_zone;
	cmr_s16 start_index;
	cmr_u32 line_time;
	cmr_s16 snr_max_fps;
	cmr_s16 snr_min_fps;
	cmr_u32 frame_id;
	cmr_u32 *r;
	cmr_u32 *g;
	cmr_u32 *b;
	cmr_s8 ae_initial;
	cmr_s8 alg_id;
	cmr_s32 effect_expline;
	cmr_s32 effect_gain;
	cmr_s32 effect_dummy;
	cmr_u8 led_state;			//0:off, 1:on
	cmr_u8 flash_fired;			//just notify APP in flash auto
//caliberation for bv match with lv
	float lv_cali_lv;
	float lv_cali_bv;
/*for mlog function*/
	cmr_u8 mlog_en;
/*modify the lib log level, if necissary*/
	cmr_u8 log_level;
//refer to convergence
	cmr_u8 ae_start_delay;
	cmr_s16 stride_config[2];
	cmr_s16 under_satu;
	cmr_s16 ae_satur;
	//for touch AE
	cmr_s8 to_ae_state;
	//for face AE
	struct ae1_fd_param ae1_finfo;
//adv_alg module init
	cmr_handle adv[8];
	/*
	   0:region
	   1:flat
	   2: mulaes
	   3: touch ae
	   4: face ae
	   5:flash ae????
	 */
	struct ae_settings settings;
	cmr_u32 awb_mode;
	struct ae_alg_rgb_gain awb_cur_gain;
};

struct ae1_senseor_out {
	cmr_s8 stable;
	cmr_s8 f_stable;
	cmr_s16 cur_index;			/*the current index of ae table in ae now: 1~1024 */
	cmr_u32 exposure_time;		/*exposure time, unit: 0.1us */
	float cur_fps;				/*current fps:1~120 */
	cmr_u16 cur_exp_line;		/*current exposure line: the value is related to the resolution */
	cmr_u16 cur_dummy;			/*dummy line: the value is related to the resolution & fps */
	cmr_s16 cur_again;			/*current analog gain */
	cmr_s16 cur_dgain;			/*current digital gain */
	cmr_s32 cur_bv;
};

struct ae_alg_calc_result {
	cmr_u32 version;			/*version No. for this structure */
	cmr_s16 cur_lum;			/*the average of image:0 ~255 */
	cmr_s16 target_lum;			/*the ae target lum: 0 ~255 */
	cmr_s16 target_zone;		/*ae target lum stable zone: 0~255 */
	cmr_s16 target_lum_ori;		/*the ae target lum(original): 0 ~255 */
	cmr_s16 target_zone_ori;	/*the ae target lum stable zone(original):0~255 */
	cmr_u32 frame_id;
	cmr_s16 cur_bv;				/*bv parameter */
	cmr_s16 cur_bv_nonmatch;
	cmr_s16 *histogram;			/*luma histogram of current frame */
	//for flash
	cmr_s32 flash_effect;
	cmr_s8 flash_status;
	cmr_s16 mflash_exp_line;
	cmr_s16 mflash_dummy;
	cmr_s16 mflash_gain;
	//for touch
	cmr_s8 tcAE_status;
	cmr_s8 tcRls_flag;
	//for face debug
	cmr_u32 face_lum;
	void *pmulaes;
	void *pflat;
	void *pregion;
	void *ptc;					/*Bethany add touch info to debug info */
	void *pface_ae;
	struct ae1_senseor_out wts;
	cmr_handle log;
	cmr_u32 flag4idx;
	cmr_u32 *reserved;			/*resurve for future */
};

struct ae_alg_fun_tab {
	cmr_handle(*init) (cmr_handle, cmr_handle);
	cmr_s32(*deinit) (cmr_handle, cmr_handle, cmr_handle);
	cmr_s32(*calc) (cmr_handle, cmr_handle, cmr_handle);
	cmr_s32(*ioctrl) (cmr_handle, cmr_u32, cmr_handle, cmr_handle);
};
#endif
