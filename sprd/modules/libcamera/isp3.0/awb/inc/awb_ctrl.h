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
#ifndef _AWB_CTRL_H_
#define _AWB_CTRL_H_
/*----------------------------------------------------------------------------*
**				Dependencies					*
**---------------------------------------------------------------------------*/

#include <sys/types.h>
#include "isp_common_types.h"
#include "isp_3a_adpt.h"


/**********************************ENUM*************************************/
enum awb_ctrl_cmd {
	AWB_CTRL_CMD_SET_BASE = 0,
	AWB_CTRL_CMD_SET_SCENE_MODE,
	AWB_CTRL_CMD_SET_WB_MODE,
	AWB_CTRL_CMD_SET_FLASH_MODE,
	AWB_CTRL_CMD_SET_STAT_IMG_SIZE,
	AWB_CTRL_CMD_SET_STAT_IMG_FORMAT,
	AWB_CTRL_CMD_SET_WORK_MODE,
	AWB_CTRL_CMD_SET_FACE_INFO,
	AWB_CTRL_CMD_SET_DZOOM_FACTOR,
	AWB_CTRL_CMD_SET_BYPASS,
	AWB_CTRL_CMD_SET_SOF_FRAME_IDX,
	AWB_CTRL_CMD_GET_PARAM_WIN_START,
	AWB_CTRL_CMD_GET_PARAM_WIN_SIZE,
	AWB_CTRL_CMD_GET_GAIN,
	AWB_CTRL_CMD_FLASH_OPEN_P,
	AWB_CTRL_CMD_FLASH_OPEN_M,
	AWB_CTRL_CMD_FLASHING,
	AWB_CTRL_CMD_FLASH_CLOSE,
	AWB_CTRL_CMD_LOCK,
	AWB_CTRL_CMD_UNLOCK,
	AWB_CTRL_CMD_GET_STAT_SIZE,
	AWB_CTRL_CMD_GET_WIN_SIZE,
	AWB_CTRL_CMD_GET_CT,
	AWB_CTRL_CMD_FLASH_BEFORE_P,
	AWB_CTRL_CMD_SET_FLASH_STATUS,
	AWB_CTRL_CMD_FLASH_BEFORE_M,
	AWB_CTRL_CMD_SET_AE_REPORT,
	AWB_CTRL_CMD_SET_AF_REPORT,
	AWB_CTRL_CMD_GET_EXIF_DEBUG_INFO,//to jpeg exif
	AWB_CTRL_CMD_GET_DEBUG_INFO,//to jpeg tail
	AWB_CTRL_CMD_SET_SLAVE_ISO_SPEED,
	AWB_CTRL_CMD_SET_MASTER,
	AWB_CTRL_CMD_SET_TUNING_MODE,
	AWB_CTRL_CMD_MAX
};

enum awb_ctrl_response_type {
	AWB_CTRL_RESPONSE_STABLE,
	AWB_CTRL_RESPONSE_QUICK_ACT,
	AWB_CTRL_RESPONSE_DIRECT,
	AWB_CTRL_RESPONSE_TYPE_MAX
};

enum awb_ctrl_response_level {
	AWB_CTRL_RESPONSE_NORMAL,
	AWB_CTRL_RESPONSE_SLOW,
	AWB_CTRL_RESPONSE_QUICK,
	AWB_CTRL_RESPONSE_LEVEL_MAX
};

enum awb_ctrl_wb_mode {
	AWB_CTRL_WB_MODE_AUTO,
	AWB_CTRL_MWB_MODE_SUNNY,
	AWB_CTRL_MWB_MODE_CLOUDY,
	AWB_CTRL_MWB_MODE_FLUORESCENT,
	AWB_CTRL_MWB_MODE_INCANDESCENT,
	AWB_CTRL_MWB_MODE_DAYLIGHT,
	AWB_CTRL_MWB_MODE_WHITE_FLUORESCENT,
	AWB_CTRL_MWB_MODE_TUNGSTEN,
	AWB_CTRL_MWB_MODE_SHADE,
	AWB_CTRL_MWB_MODE_TWILIGHT,
	AWB_CTRL_MWB_MODE_FACE,
	AWB_CTRL_MWB_MODE_MANUAL_GAIN,
	AWB_CTRL_MWB_MODE_MANUAL_CT,
	AWB_CTRL_MWB_MODE_MAX
};

enum awb_ctrl_scene_mode {
	//AWB_CTRL_SCENEMODE_AUTO,
	//AWB_CTRL_SCENEMODE_DUSK,
	//AWB_CTRL_SCENEMODE_USER_0,
	//AWB_CTRL_SCENEMODE_USER_1,
	//AWB_CTRL_SCENEMODE_MAX
	AWB_CTRL_SCENE_NORMAL,
	AWB_CTRL_SCENE_NIGHT,
	AWB_CTRL_SCENE_SPORT,
	AWB_CTRL_SCENE_PORTRAIT,
	AWB_CTRL_SCENE_LANDSCAPE,
	AWB_CTRL_SCENE_MAX
};

enum awb_ctrl_flash_status {
	AWB_CTRL_FLASH_PRE_BEFORE,
	AWB_CTRL_FLASH_PRE_LIGHTING,
	AWB_CTRL_FLASH_PRE_AFTER,
	AWB_CTRL_FLASH_MAIN_BEFORE,
	AWB_CTRL_FLASH_MAIN_LIGHTING,
	AWB_CTRL_FLASH_MAIN_MEASURE,
	AWB_CTRL_FLASH_MAIN_AFTER,
	AWB_CTRL_FLASH_STATUS_MAX
};

enum awb_ctrl_cb_type {
	AWB_CTRL_CB_NORMAL = 0,
	AWB_CTRL_CB_CONVERGE,
	AWB_CTRL_CB_MAX,
};

enum awb_ctrl_status {
	AWB_CTRL_STATUS_NORMAL = 0,
	AWB_CTRL_STATUS_CONVERGE,
	AWB_CTRL_STATUS_MAX
};
/**********************************STRUCT***********************************/
struct awb_ctrl_wbmode_param {
	cmr_u32 wb_mode;
	cmr_u32 manual_ct;
};

struct awb_ctrl_work_param {
	cmr_u32 work_mode;
	cmr_u32 capture_mode;
	cmr_u16 is_refocus;
	struct isp_size sensor_size;
};
struct awb_ctrl_callback_in {

};

typedef cmr_int (*awb_callback)(cmr_handle handle, cmr_u32 cb_type, struct awb_ctrl_callback_in *input);

struct awb_ctrl_init_in {
	cmr_handle caller_handle;
	cmr_u32 camera_id;
	cmr_u16 is_refocus;
	cmr_u32 base_gain;
	cmr_u32 awb_enable;
	cmr_int wb_mode;
	cmr_u8 is_master;
//	enum awb_ctrl_stat_img_format stat_img_format;
//	struct awb_ctrl_size stat_img_size;
//	struct awb_ctrl_size stat_win_size;
//	struct awb_ctrl_opt_info otp_info;
	struct isp_lib_config lib_config;
	struct isp_calibration_awb_gain calibration_gain;
	struct isp_calibration_awb_gain calibration_gain_slv;
	cmr_u32 awb_process_type;
	cmr_u32 awb_process_level;
	void *tuning_param[ISP_INDEX_MAX];
	void *tuning_param_slv[ISP_INDEX_MAX];
	cmr_u32 param_size;
	void *lsc_otp_random;
	void *lsc_otp_golden;
	cmr_u32 lsc_otp_width;
	cmr_u32 lsc_otp_height;
	awb_callback awb_cb;
	void *priv_handle;
};

struct awb_ctrl_init_out {
	struct isp_awb_gain gain;
	struct isp_awb_gain gain_balanced;
	cmr_u32 ct;
	struct isp3a_awb_hw_cfg hw_cfg;
};

struct awb_ctrl_process_in {
	cmr_u32 response_level;
	cmr_u32 awb_process_type;
	void *report;
	cmr_u32 report_size;
	struct isp3a_ae_info ae_info;
	struct isp3a_statistics_data *statistics_data;
};

struct awb_ctrl_process_out {
	cmr_u32 hw3a_frame_id;
	cmr_u32 awb_mode;
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
struct awb_ctrl_manual_lock {
	cmr_u16 lock_flag;
	cmr_u16 ct;
	struct isp_awb_gain   wbgain;
};

struct awb_ctrl_af_report {
	void *data;
	cmr_u32 data_size;
};

/**********************************UNION*************************************/
union awb_ctrl_cmd_in {
	float dzoom_factor;
	cmr_u32 manual_ct;
	cmr_u32 lock_flag;
	cmr_u32 scene_mode;
	cmr_u32 flash_status;
	cmr_u32 bypass;
	cmr_u32 sof_frame_idx;
	cmr_u32 idx_num;
	cmr_u16 iso_speed;
	cmr_u8 is_master;
	struct awb_ctrl_wbmode_param wb_mode;
	struct isp_face_area face_info;
	struct awb_ctrl_work_param work_param;
	struct isp3a_ae_info ae_info;
	struct awb_ctrl_manual_lock lock_param;
	struct awb_ctrl_af_report af_report;
	struct scene_setting_info  scene_info;
};

union awb_ctrl_cmd_out {
	struct isp_awb_gain gain;
	struct isp3a_awb_hw_cfg hw_cfg;
	struct isp_info debug_info;
	cmr_u32 ct;
};
/**********************************END***********************************/


/**********************************INTERFACES********************************/
cmr_int awb_ctrl_init(struct awb_ctrl_init_in *input_ptr, struct awb_ctrl_init_out *output_ptr, cmr_handle *awb_handle);
cmr_int awb_ctrl_deinit(cmr_handle awb_handle);
cmr_int awb_ctrl_ioctrl(cmr_handle awb_handle, enum awb_ctrl_cmd cmd, union awb_ctrl_cmd_in *input_ptr, union awb_ctrl_cmd_out *output_ptr);
cmr_int awb_ctrl_process(cmr_handle awb_handle, struct awb_ctrl_process_in *input_ptr, struct awb_ctrl_process_out *output_ptr);

/**********************************STRUCT***********************************/
#endif

