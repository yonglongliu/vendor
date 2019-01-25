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

#ifndef _AE_SPRD_ADPT_INTERNAL_H_
#define _AE_SPRD_ADPT_INTERNAL_H_
#include "flash.h"
#include "ae_tuning_type.h"
#include "ae_ctrl_types.h"
#include "ae_ctrl.h"

#if defined(CONFIG_ISP_2_2)
#include "isp_match.h"
#elif defined(CONFIG_ISP_2_3)
#include "isp_bridge.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

	struct ae_exposure_param {
		cmr_u32 cur_index;
		cmr_u32 line_time;
		cmr_u32 exp_line;
		cmr_u32 exp_time;
		cmr_s32 dummy;
		cmr_u32 gain;			/*gain = sensor_gain * isp_gain */
		cmr_u32 sensor_gain;
		cmr_u32 isp_gain;
		cmr_s32 target_offset;
		cmr_s32 bv;
	};

	struct ae_sensor_exp_data {
		struct ae_exposure_param lib_data;	/*AE lib output data */
		struct ae_exposure_param actual_data;	/*the actual effect data */
		struct ae_exposure_param write_data;	/*write to sensor data in current time */
	};

	struct touch_zone_param {
		cmr_u32 enable;
		struct ae_weight_table weight_table;
		struct touch_zone zone_param;
	};

	enum initialization {
		AE_PARAM_NON_INIT,
		AE_PARAM_INIT
	};

	struct flash_cali_data {
		cmr_u16 ydata;			//1024
		cmr_u16 rdata;			// base on gdata1024
		cmr_u16 bdata;			// base on gdata1024
		cmr_s8 used;
	};

/*struct ae_monitor_unit {
	cmr_u32 mode;
	struct ae_size win_num;
	struct ae_size win_size;
	struct ae_trim trim;
	struct ae_monitor_cfg cfg;
	cmr_u32 is_stop_monitor;
};*/

	struct flash_swith_param {
		cmr_s16 led_thr_up;
		cmr_s16 led_thr_down;
	};

/**************************************************************************/
/*
* BEGIN: FDAE related definitions
*/
	struct ae_fd_info {
		cmr_s8 enable;
		cmr_s8 pause;
		cmr_s8 allow_to_work;	/* When allow_to_work==0, FDAE
								 * will never work */
		struct ae_fd_param face_info;	/* The current face information */
	};
/*
* END: FDAE related definitions
*/
	struct ae_update_list {
		cmr_u32 is_scene:1;
		cmr_u32 is_miso:1;
		cmr_u32 is_mev:1;
	};

/**************************************************************************/
/*
* ae handler for isp_app
*/
	struct ae_ctrl_cxt {
		cmr_u32 start_id;
		char alg_id[32];
		cmr_u32 checksum;
		cmr_u32 bypass;
		cmr_u32 capture_skip_num;
		cmr_u32 zsl_flag;
		cmr_u32 skip_update_param_flag;
		cmr_u32 skip_update_param_cnt;
		cmr_u32 delay_cnt;
		cmr_u32 backup_rgb_gain;
		double ob_rgb_gain;
		pthread_mutex_t data_sync_lock;
		/*
		 * camera id: front camera or rear camera
		 */
		cmr_s8 camera_id;
		cmr_s8 is_snapshot;
		cmr_u8 is_first;
		/*
		 * ae control operation infaces
		 */
		struct ae_isp_ctrl_ops isp_ops;
		/*
		 * ae stat monitor config
		 */
		//struct ae_monitor_unit monitor_unit;
		/*
		 * ae slow motion info
		 */
		struct ae_sensor_fps_info high_fps_info;
		/*
		 * ae fps range
		 */
		struct ae_range fps_range;
		/*
		 * current flicker flag
		 */
		cmr_u32 cur_flicker;
		/*
		 * for ae tuning parameters
		 */
		struct ae_stats_monitor_cfg monitor_cfg;
		struct ae_tuning_param tuning_param[AE_MAX_PARAM_NUM];
		cmr_s8 tuning_param_enable[AE_MAX_PARAM_NUM];
		struct ae_tuning_param *cur_param;
		struct ae_exp_gain_table back_scene_mode_ae_table[AE_SCENE_NUM][AE_FLICKER_NUM];
		struct flash_tune_param dflash_param[AE_MAX_PARAM_NUM];
		/*
		 * sensor related information
		 */
		struct ae_resolution_info snr_info;
		/*
		 * force update list
		 */
		struct ae_update_list mod_update_list;
		/*
		 * ae current status: include some tuning
		 * param/calculatioin result and so on
		 */
		struct ae_alg_calc_param prv_status;	/*just backup the alg status of normal scene,
												   as switch from special scene mode to normal,
												   and we use is to recover the algorithm status
												 */
		struct ae_alg_calc_param cur_status;
		struct ae_alg_calc_param sync_cur_status;
		cmr_s32 target_lum_zone_bak;
		/*
		 * convergence & stable zone
		 */
		cmr_s8 stable_zone_ev[16];
		cmr_u8 cnvg_stride_ev_num;
		cmr_s16 cnvg_stride_ev[32];
		/*
		 * Touch ae param
		 */
		struct touch_zone_param touch_zone_param;
		/*
		 * flash ae param
		 */
		/*ST: for dual flash algorithm */
		struct flash_swith_param flash_swith;
		cmr_u8 flash_ver;
		cmr_s32 pre_flash_skip;
		cmr_s32 aem_effect_delay;
		struct ae_leds_ctrl ae_leds_ctrl;
		cmr_handle flash_alg_handle;
		cmr_u16 aem_stat_rgb[3 * 1024];	/*save the average data of AEM Stats data */
		cmr_u32 sync_aem[3 * 1024 + 4];	/*aem statistics data, 0: frame id;1: exposure time, 2: dummy line, 3: gain; */
		cmr_u16 sync_binning_stats_data[3 * 640 * 480];	/*binning statistics data */
		struct ae_size binning_stat_size;
		cmr_u8 bakup_ae_status_for_flash;	/* 0:unlock 1:lock 2:pause 3:wait-lock */
		cmr_s16 pre_flash_level1;
		cmr_s16 pre_flash_level2;
		struct Flash_pfOneIterationInput flash_esti_input;
		struct Flash_pfOneIterationOutput flash_esti_result;
		struct Flash_mfCalcInput flash_main_esti_input;
		struct Flash_mfCalcOutput flash_main_esti_result;
		cmr_s32 flash_last_exp_line;
		cmr_s32 flash_last_gain;
		float ctTabRg[20];
		float ctTab[20];
		/*ED: for dual flash algorithm */
		cmr_s16 flash_on_off_thr;
		cmr_u32 flash_effect;
		struct ae_exposure_param flash_backup;
		struct flash_cali_data flash_cali[32][32];
		cmr_u8 flash_debug_buf[256 * 1024];
		cmr_u32 flash_buf_len;
		/*
		 * fd-ae param
		 */
		struct ae_fd_info fdae;
		/*
		 * control information for sensor update
		 */
		struct ae_alg_calc_result cur_result;
		struct ae_alg_calc_result sync_cur_result;
		struct ae_calc_results calc_results;	/*ae current calculation results, and it is just for other algorithm block */
		/*
		 * AE write/effective E&G queue
		 */
		cmr_handle seq_handle;
		cmr_s8 exp_skip_num;
		cmr_s8 gain_skip_num;
		cmr_s16 sensor_gain_precision;
		cmr_u16 sensor_max_gain;	/*the max gain that sensor can be used */
		cmr_u16 sensor_min_gain;	/*the mini gain that sensor can be used */
		cmr_u16 min_exp_line;	/*the mini exposure line that sensor can be used */
		struct ae_sensor_exp_data exp_data;
		/*
		 * recording when video stop
		 */
		struct ae_exposure_param last_exp_param;
		cmr_s32 last_index;
		cmr_s32 last_enable;
		/*
		 * just for debug information
		 */
		cmr_u8 debug_enable;
		char debug_file_name[256];
		cmr_handle debug_info_handle;
		cmr_u32 debug_str[512];
		cmr_u8 debug_info_buf[512 * 1024];
		//struct debug_ae_param debug_buf;
		/*
		 * for manual ae stat
		 */
		cmr_u8 manual_ae_on;
		/*
		 *Save exposure & iso value on manual ae mode
		 */
		cmr_u32 manual_exp_time;
		cmr_u32 manual_iso_value;
		/*
		 * flash_callback control
		 */
		cmr_s8 send_once[5];
		/*
		 * HDR control
		 */
		cmr_s8 hdr_enable;
		cmr_s8 hdr_frame_cnt;
		cmr_s8 hdr_cb_cnt;
		cmr_s8 hdr_flag;
		cmr_s16 hdr_up;
		cmr_s16 hdr_down;
		cmr_s16 hdr_base_ae_idx;
		/*
		 *dual flash simulation
		 */
		cmr_s8 led_record[2];
		/*
		 * ae misc layer handle
		 */
		cmr_handle misc_handle;
		/*
		 * for dual camera sync
		 */
		cmr_u8 sensor_role;
		cmr_u32 is_multi_mode;
		func_isp_br_ioctrl ptr_isp_br_ioctrl;

#ifdef  CONFIG_ISP_2_2
//  struct ae_calc_result pre_write_exp_data_slv;
		struct ae_exposure_param pre_write_exp_data_slv;
//  struct ae_calc_result pre_write_exp_data;
		struct ae_exposure_param pre_write_exp_data;
		struct match_data_param dualcam_aesync_param;
#endif
		cmr_u32 end_id;
		/*
		 * for binning facter = 2 
		 */
		cmr_s32 binning_factor_before;
		cmr_s32 binning_factor_after;
	};

#endif
