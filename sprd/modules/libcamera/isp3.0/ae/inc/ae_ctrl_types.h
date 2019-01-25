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

#ifndef _AE_CTRL_TYPES_H_
#define _AE_CTRL_TYPES_H_
/*----------------------------------------------------------------------------*
 **				 Dependencies				*
 **---------------------------------------------------------------------------*/
#include "isp_common_types.h"
#include "isp_type.h"
#include "isp_3a_adpt.h"
/**---------------------------------------------------------------------------*
 **				 Compiler Flag				*
 **---------------------------------------------------------------------------*/
#ifdef __cplusplus
extern "C"
{
#endif

/**---------------------------------------------------------------------------*
**				 Macro Define				*
**----------------------------------------------------------------------------*/

/**---------------------------------------------------------------------------*
**				Data Prototype				*
**----------------------------------------------------------------------------*/
enum ae_ctrl_cmd {
	AE_CTRL_GET_LUM,
	AE_CTRL_GET_ISO,
	AE_CTRL_GET_EV_TABLE,
	AE_CTRL_SET_BYPASS,
	AE_CTRL_SET_FLICKER,
	AE_CTRL_SET_SCENE_MODE,
	AE_CTRL_SET_ISO,
	AE_CTRL_SET_FPS,
	AE_CTRL_SET_EXP_COMP,
	AE_CTRL_SET_MEASURE_LUM,
	AE_CTRL_SET_STAT_TRIM,
	AE_CTRL_SET_TOUCH_ZONE,
	AE_CTRL_SET_WORK_MODE,
	AE_CTRL_SET_PAUSE,
	AE_CTRL_SET_RESTORE,
	AE_CTRL_SET_FLASH_NOTICE,
	AE_CTRL_GET_FLASH_EFFECT,
	AE_CTRL_GET_AE_STATE,
	AE_CTRL_GET_FLASH_EB,
	AE_CTRL_GET_BV_BY_LUM,
	AE_CTRL_GET_BV_BY_GAIN,
	AE_CTRL_SET_FORCE_PAUSE,
	AE_CTRL_SET_FORCE_RESTORE,
	AE_CTRL_GET_MONITOR_INFO,
	AE_CTRL_GET_FLICKER_MODE,
	AE_CTRL_SET_FD_ENABLE,
	AE_CTRL_SET_FD_PARAM,
	AE_CTRL_GET_SKIP_FRAME_NUM,
	AE_CTRL_SET_QUICK_MODE,
	AE_CTRL_SET_GYRO_PARAM,
	AE_CTRL_SET_HDR_EV,
	AE_CTRL_SET_HDR_ENABLE,
	AE_CTRL_GET_HWISP_CONFIG,
	AE_CTRL_SET_AWB_REPORT,
	AE_CTRL_SET_AF_REPORT,
	AE_CTRL_SET_CONVERGENCE_REQ,
	AE_CTRL_SET_SNAPSHOT_FINISHED,
	AE_CTRL_SET_MASTER,
	AE_CTRL_GET_DEBUG_DATA,
	AE_CTRL_GET_EXIF_DATA,
	AE_CTRL_GET_EXT_DEBUG_INFO,
	AE_CTRL_GET_EXP_GAIN,
	AE_CTRL_SET_FLASH_MODE,
	AE_CTRL_SET_Y_HIST_STATS,
	AE_CTRL_SET_FIX_EXPOSURE_TIME,
	AE_CTRL_SET_FIX_SENSITIVITY,
	AE_CTRL_SET_FIX_FRAME_DURATION,
	AE_CTRL_SET_MANUAL_EXPTIME,
	AE_CTRL_SET_MANUAL_GAIN,
	AE_CTRL_SET_MANUAL_ISO,
	AE_CTRL_SET_ENGINEER_MODE,
	AE_CTRL_SET_TUNING_MODE,
	AE_CTRL_SYNC_MSG_END,

	/*
	 * warning if you wanna send async msg
	 * please add msg id below here
	 * */
	AE_CTRL_SYNC_NONE_MSG_BEGIN,
	AE_CTRL_SET_SOF,
	AE_CTRL_SYNC_NONE_MSG_END,

	/*
	 * warning if you wanna set ioctrl directly
	 * please add msg id below here
	 * */
	AE_CTRL_DIRECT_MSG_BEGIN,
	AE_CTRL_GET_HW_ISO_SPEED,
	AE_CTRL_DIRECT_MSG_END,

	AE_CTRL_CMD_MAX,
};

enum ae_ctrl_proc_type {
	AE_CTRL_PROC_STAT_RGB,
	AE_CTRL_PROC_STAT_Y,
	AE_CTRL_PROC_TYPE_MAX
};

enum ae_ctrl_measure_lum_mode {
	AE_CTRL_MEASURE_LUM_AVG,
	AE_CTRL_MEASURE_LUM_CENTER,
	AE_CTRL_MEASURE_LUM_SPOT,
	AE_CTRL_MEASURE_LUM_INTELLIWT,
	AE_CTRL_MEASURE_LUM_TOUCH,
	AE_CTRL_MEASURE_LUM_USERDEF,
	AE_CTRL_METERING_MAX
};

enum ae_ctrl_scene_mode {
	AE_CTRL_SCENE_NORMAL,
	AE_CTRL_SCENE_NIGHT,
	AE_CTRL_SCENE_SPORT,
	AE_CTRL_SCENE_PORTRAIT,
	AE_CTRL_SCENE_LANDSCAPE,
	AE_CTRL_SCENE_MAX
};

enum ae_ctrl_iso_mode {
	AE_CTRL_ISO_AUTO,
	AE_CTRL_ISO_100,
	AE_CTRL_ISO_200,
	AE_CTRL_ISO_400,
	AE_CTRL_ISO_800,
	AE_CTRL_ISO_1600,
	AE_CTRL_ISO_3200,
	AE_CTRL_ISO_6400,
	AE_CTRL_ISO_12800,
	AE_CTRL_ISO_USRDEF,
	AE_CTRL_ISO_MAX
};

enum ae_ctrl_flicker_mode {
	AE_CTRL_FLICKER_50HZ,
	AE_CTRL_FLICKER_60HZ,
	AE_CTRL_FLICKER_AUTO,
	AE_CTRL_FLICKER_OFF,
	AE_CTRL_FLICKER_MAX
};
#if 0
enum ae_ctrl_capture_mode {
	AE_CTRL_CAP_MODE_AUTO,
	AE_CTRL_CAP_MODE_ZSL,
	AE_CTRL_CAP_MODE_HDR,
	AE_CTRL_CAP_MODE_VIDEO,
	AE_CTRL_CAP_MODE_VIDEO_HDR,
	AE_CTRL_CAP_MODE_BRACKET,
	AE_CTRL_CAP_MODE_DUALCAM_SYNC,
	AE_CTRL_CAP_MODE_DUALCAM_ASYNC,   // Fusion
	AE_CTRL_CAP_MODE_FASTSHOT,
	AE_CTRL_CAP_MODE_FASTSHOT_DUALCAM,
	AE_CTRL_CAP_MODE_MAX
};
#endif

enum ae_ctrl_cb_type {
	AE_CTRL_CB_CONVERGED, //normal converged
	AE_CTRL_CB_FLASHING_CONVERGED,
	AE_CTRL_CB_QUICKMODE_DOWN,
	AE_CTRL_CB_STAB_NOTIFY,
	AE_CTRL_CB_AE_LOCK_NOTIFY,
	AE_CTRL_CB_AE_UNLOCK_NOTIFY,
	AE_CTRL_CB_CLOSE_PREFLASH,
	AE_CTRL_CB_PREFLASH_PERIOD_END,
	AE_CTRL_CB_CLOSE_MAIN_FLASH,
	AE_CTRL_CB_PROC_OUT,
	AE_CTRL_CB_SOF,
	AE_CTRl_CB_TOUCH_CONVERGED,
	AE_CTRL_CB_SYNC_INFO,
	AE_CTRL_CB_MAX
};

enum ae_ctrl_hdr_ev_level {
	AE_CTRL_HDR_EV_UNDEREXPOSURE,
	AE_CTRL_HDR_EV_NORMAL,
	AE_CTRL_HDR_EV_OVEREXPOSURE,
	AE_CTRL_HDR_EV_MAX
};

enum ae_ctrl_attr_level {
	AE_CTRL_ATTR_LEVEL_0,
	AE_CTRL_ATTR_LEVEL_1,
	AE_CTRL_ATTR_LEVEL_2,
	AE_CTRL_ATTR_LEVEL_3,
	AE_CTRL_ATTR_LEVEL_4,
	AE_CTRL_ATTR_LEVEL_5,
	AE_CTRL_ATTR_LEVEL_6,
	AE_CTRL_ATTR_LEVEL_7,
	AE_CTRL_ATTR_LEVEL_8,
	AE_CTRL_ATTR_LEVEL_9,
	AE_CTRL_ATTR_LEVEL_10,
	AE_CTRL_ATTR_LEVEL_11,
	AE_CTRL_ATTR_LEVEL_12,
	AE_CTRL_ATTR_LEVEL_13,
	AE_CTRL_ATTR_LEVEL_14,
	AE_CTRL_ATTR_LEVEL_15,
	AE_CTRL_ATTR_LEVEL_16,
	AE_CTRL_ATTR_LEVEL_17,
	AE_CTRL_ATTR_LEVEL_18,
	AE_CTRL_ATTR_LEVEL_19,
	AE_CTRL_ATTR_LEVEL_20,
	AE_CTRL_ATTR_LEVEL_MAX
};

struct ae_ctrl_param_exp_comp {
	cmr_int level;
};

struct ae_ctrl_param_iso {
	cmr_int iso_mode;
};

struct ae_ctrl_param_flash {
	cmr_int flash_mode;
};

struct ae_ctrl_param_flicker {
	cmr_int flicker_mode;
};


struct ae_ctrl_param_touch_zone {
	struct isp_trim_size zone;
};

struct ae_ctrl_param_flash_notice {
	cmr_u32 flash_mode;
	cmr_u32 will_capture;
	cmr_u32 flash_ratio;
	cmr_u32 capture_skip_num;
	cmr_u32 ui_flash_status;
	struct isp_flash_led_info led_info;
};

struct ae_ctrl_param_ae_lum {
	cmr_u32 ae_lum;
};

struct ae_ctrl_param_measure_highflash {
	cmr_u32 highflash_flag;
	cmr_u32 capture_skip_num;
};

struct ae_ctrl_param_resolution {
	struct isp_size frame_size;
	cmr_u32 line_time;
	cmr_u32 frame_line;
	cmr_u32 sensor_size_index;
	cmr_u32 max_gain;
	cmr_u32 max_fps;
	cmr_u32 min_fps;
};

struct ae_ctrl_param_work {
		cmr_int work_mode;
		cmr_int capture_mode;
		cmr_u16 is_refocus;
		struct ae_ctrl_param_resolution resolution;
		struct ae_ctrl_param_measure_highflash highflash;
		struct isp_sensor_fps_info sensor_fps;
		void *tuning_param;
};

struct ae_ctrl_param_scene {
	cmr_int scene_mode;
	struct scene_setting_info  scene_info;
};

struct ae_ctrl_param_measure_lum {
	cmr_int lum_mode;
};

struct ae_ctrl_param_range_fps {
	float min_fps;
	float max_fps;
};

struct ae_ctrl_param_sensor_exposure {
	cmr_u32 exp_line;
	cmr_u32 dummy;
	cmr_u32 size_index;
};

struct ae_ctrl_param_sensor_gain {
	cmr_u32 gain;
};

struct ae_ctrl_param_sensor_static_info {
	cmr_s32 f_num;
	cmr_s32 exposure_valid_num;
	cmr_s32 gain_valid_num;
	cmr_s32 preview_skip_num;
	cmr_s32 capture_skip_num;
	cmr_s32 max_fps;
	cmr_s32 max_gain;
	cmr_s32 ois_supported;
};

struct ae_ctrl_param_hdr_ev {
	cmr_s32 enable;
};

struct ae_ctrl_param_ae_frame {
	cmr_u32 is_skip_cur_frame;
	void *stats_buff_ptr;
	void *awb_stats_buff_ptr;
};

struct ae_ctrl_param_raw_cell {
	cmr_u32 r;
	cmr_u32 g;
	cmr_u32 b;
};

struct ae_ctrl_param_awb_gain {
	cmr_u32 color_temp;
	struct ae_ctrl_param_raw_cell normal_gain;
	struct ae_ctrl_param_raw_cell balance_gain;
	struct ae_ctrl_param_raw_cell flash_gain;
	cmr_u32 awb_state;
};

struct ae_ctrl_param_af {
	cmr_u32 state;
	cmr_u32 distance;
};

struct ae_ctrl_param_sof {
	cmr_uint frame_index;
	struct isp3a_timestamp timestamp;
};

struct ae_ctrl_param_gyro {
	cmr_u32 param_type;   // 1: interger, 0: float
	cmr_u32 uinfo[3];
	cmr_u32 finfo[3];
};

struct ae_ctrl_debug_param {
	cmr_u32  size;
	void   *data;
};

struct ae_ctrl_ext_debug_info {
	cmr_u32 flash_flag;
	cmr_u32 fn_value;
	cmr_u32 valid_ad_gain;
	cmr_u32 valid_exposure_line;
	cmr_u32 valid_exposure_time;
};

struct ae_ctrl_otp_data {
	cmr_u32 r;
	cmr_u32 g;
	cmr_u32 b;
	cmr_u32 iso;
};

struct ae_ctrl_exp_gain_param {
	cmr_u32 exposure_line;
	cmr_u32 exposure_time;
	cmr_u32 dummy;
	cmr_u32 gain;
};

struct ae_ctrl_iq_info {
	cmr_u8 valid_flg;  /* 0:AE Not Process yet,1:AE already process */
	cmr_u8 reserved[16];  /*reserved 16 bytes*/
};

struct ae_ctrl_sof_cfg_info {
	cmr_u32 hw_iso_speed;
	cmr_u32 curmean;
	cmr_s32 bv_val;
	cmr_s32 bg_bvresult;
	cmr_u32 ae_cur_iso;
	struct ae_ctrl_iq_info ae_iq_info;
};

struct ae_ctrl_proc_out {
	struct isp3a_ae_info ae_info;
	struct ae_ctrl_param_ae_frame ae_frame;
	struct isp3a_ae_hw_cfg hw_cfg;

	cmr_u32 priv_size;
	void *priv_data;
};

struct ae_ctrl_proc_in {
	struct isp3a_statistics_data *stat_data_ptr;
	struct ae_ctrl_param_awb_gain awb_gain;
	struct ae_ctrl_param_af af_info;
	struct isp3a_statistics_data *awb_stat_data_ptr;
};

struct ae_ctrl_param_out {
	union{
	cmr_s32 flash_eb;
	cmr_u32 iso_val;
	struct ae_ctrl_proc_out proc_out;
	struct isp3a_ae_hw_cfg hw_cfg;
	cmr_u32 isp_d_gain;
	cmr_u32 hw_iso_speed;
	cmr_s32 bv;
	cmr_u32 flicker_mode;
	cmr_u32 ae_state;
	struct ae_ctrl_debug_param debug_param;
	struct ae_ctrl_debug_param exif_param;
	struct ae_ctrl_ext_debug_info debug_info;
	struct ae_ctrl_exp_gain_param exp_gain;
	};
};

struct ae_ctrl_param_in {
	union {
	cmr_u32 value;
	cmr_u32 idx_num;
	struct ae_ctrl_param_scene scene;
	struct ae_ctrl_param_measure_lum measure_lum;
	struct ae_ctrl_param_touch_zone touch_zone;
	struct ae_ctrl_param_flicker flicker;
	struct ae_ctrl_param_iso iso;
	struct ae_ctrl_param_exp_comp exp_comp;
	struct ae_ctrl_param_flash_notice flash_notice;
	struct ae_ctrl_param_range_fps range_fps;
	struct isp_face_area face_area;
	struct ae_ctrl_param_hdr_ev soft_hdr_ev;
	struct ae_ctrl_param_work work_param;
	struct ae_ctrl_param_sof sof_param;
	struct ae_ctrl_param_awb_gain awb_report;
	struct ae_ctrl_param_af af_report;
	struct ae_ctrl_param_gyro gyro;
	struct ae_ctrl_param_flash flash;
	struct isp3a_statistics_data *y_hist_stat;

	};
};

struct ae_ctrl_callback_in {
	union {
	struct ae_ctrl_proc_out proc_out;
	struct ae_ctrl_sof_cfg_info ae_cfg_info;
	struct ispae_sync_info_output *ae_sync_info;
	};
};

struct ae_ctrl_ops_in {
	cmr_int (*set_exposure)(cmr_handle handler, struct ae_ctrl_param_sensor_exposure *in_ptr);
	cmr_int (*set_again)(cmr_handle handler, struct ae_ctrl_param_sensor_gain *in_ptr);
	cmr_int (*set_iso_slv)(cmr_handle handler, cmr_u32 iso);
	cmr_int (*read_aec_info)(cmr_handle handler, void *aec_info); /* TBD master & slave will merge by bridge */
	cmr_int (*read_aec_info_slv)(cmr_handle handler, void *aec_info);
	cmr_int (*write_aec_info)(cmr_handle handler, void *aec_i2c_info);
	cmr_int (*ae_callback)(cmr_handle handler, enum ae_ctrl_cb_type, struct ae_ctrl_callback_in *in_ptr);

	cmr_int (*get_system_time)(cmr_handle handler, cmr_u32 *sec_ptr, cmr_u32 *usec_ptr);

	cmr_int (*flash_get_charge)(cmr_handle handler, struct isp_flash_cfg *cfg_ptr, struct isp_flash_cell *cell_ptr);
	cmr_int (*flash_get_time)(cmr_handle handler, struct isp_flash_cfg *cfg_ptr, struct isp_flash_cell *cell_ptr);
	cmr_int (*flash_set_charge)(cmr_handle handler, struct isp_flash_cfg *cfg_ptr, struct isp_flash_element *element_ptr);
	cmr_int (*flash_set_time)(cmr_handle handler, struct isp_flash_cfg *cfg_ptr, struct isp_flash_element *element_ptr);
	cmr_int (*flash_ctrl)(cmr_handle handler, struct isp_flash_cfg *cfg_ptr, struct isp_flash_element *element_ptr);

	cmr_int (*release_stat_buffer)(cmr_handle handler, cmr_int type, struct isp3a_statistics_data *buf_ptr);
	cmr_int (*flash_set_skip)(cmr_handle handler, cmr_u32 skip_num);
};

struct ae_ctrl_init_out {
	struct isp3a_ae_hw_cfg hw_cfg;
	cmr_u32 hw_iso_speed;
};

struct ae_ctrl_init_in {
	cmr_u32 camera_id;
	cmr_u8 is_master;
	void *tuning_param[ISP_INDEX_MAX];
	struct ae_ctrl_otp_data otp_data;
	struct ae_ctrl_otp_data otp_data_slv;
	cmr_handle caller_handle;
	struct ae_ctrl_ops_in ops_in;
	struct isp_lib_config lib_param;
	struct ae_ctrl_param_sensor_static_info sensor_static_info;
	struct ae_ctrl_param_work preview_work;
	struct ae_ctrl_param_sensor_static_info sensor_static_info_slv; // slave sensor
	struct ae_ctrl_param_work preview_work_slv; // slave sensor
};

/**----------------------------------------------------------------------------*
**					Compiler Flag				**
**----------------------------------------------------------------------------*/
#ifdef __cplusplus
}
#endif
/**---------------------------------------------------------------------------*/
#endif

