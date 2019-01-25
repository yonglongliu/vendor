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
#ifndef _AF_CTRL_H_
#define _AF_CTRL_H_
#include "isp_common_types.h"

enum af_ctrl_cmd_type {
	AF_CTRL_CMD_SET_BASE,
	AF_CTRL_CMD_SET_AF_MODE,
	AF_CTRL_CMD_SET_AF_POS,
	AF_CTRL_CMD_SET_AF_POS_DONE,
	AF_CTRL_CMD_SET_AF_SCENE_MODE,
	AF_CTRL_CMD_SET_AF_START,
	AF_CTRL_CMD_SET_AF_STOP,
	AF_CTRL_CMD_SET_AF_RESTART,
	AF_CTRL_CMR_SET_AF_LOCK,
	AF_CTRL_CMR_SET_AF_UNLOCK,
	AF_CTRL_CMD_SET_TRIGGER_STATS,
	AF_CTRL_CMD_SET_CAF_RESET,
	AF_CTRL_CMD_SET_CAF_STOP,
	AF_CTRL_CMD_SET_AF_DONE,
	AF_CTRL_CMD_SET_AF_BYPASS,
	AF_CTRL_CMD_SET_ROI,
	AF_CTRL_CMD_SET_GYRO_INFO,
	AF_CTRL_CMD_SET_GSENSOR_INFO,
	AF_CTRL_CMD_SET_FLASH_NOTICE,
	AF_CTRL_CMD_SET_TUNING_MODE,
	AF_CTRL_CMD_SET_SOF_FRAME_IDX,
	AF_CTRL_CMD_SET_UPDATE_AE,
	AF_CTRL_CMD_SET_UPDATE_AWB,
	AF_CTRL_CMD_SET_PROC_START,
	AF_CTRL_CMD_SET_DEBUG,
	AF_CTRL_CMD_SET_UPDATE_AUX_SENSOR,
	AF_CTRL_CMD_SET_YIMG_INFO,
	AF_CTRL_CMD_SET_LIVE_VIEW_SIZE,
	AF_CTRL_CMD_SET_PRV_IMG_SIZE,
	AF_CTRL_CMD_SET_PD_INFO,
	AF_CTRL_CMD_SET_PD_ENABLE,
	AF_CTRL_CMD_SET_AF_TUNING_MODE,
	AF_CTRL_CMD_SET_MAX,

	AF_CTRL_CMD_GET_BASE = AF_CTRL_CMD_SET_MAX,
	AF_CTRL_CMD_GET_AF_MODE,
	AF_CTRL_CMD_GET_PD_STATUS,
	AF_CTRL_CMD_GET_DEFAULT_LENS_POS,
	AF_CTRL_CMD_GET_AF_CUR_LENS_POS,
	AF_CTRL_CMD_GET_DEBUG,
	AF_CTRL_CMD_GET_EXIF_DEBUG_INFO,
	AF_CTRL_CMD_GET_DEBUG_INFO,
	AF_CTRL_CMD_GET_YIMG,
	AF_CTRL_CMD_GET_MAX,
};

enum af_ctrl_mode_type {
	AF_CTRL_MODE_MACRO = 0,
	AF_CTRL_MODE_AUTO,
	AF_CTRL_MODE_CAF,
	AF_CTRL_MODE_CAF_MACRO,
	AF_CTRL_MODE_INFINITY,
	AF_CTRL_MODE_MANUAL,
	AF_CTRL_MODE_AUTO_VIDEO,
	AF_CTRL_MODE_CONTINUOUS_VIDEO,
	AF_CTRL_MODE_CONTINUOUS_PICTURE_HYBRID,
	AF_CTRL_MODE_CONTINUOUS_VIDEO_HYBRID,
	AF_CTRL_MODE_AUTO_HYBRID,
	AF_CTRL_MODE_AUTO_INSTANT_HYBRID,
	AF_CTRL_MODE_BYPASS,
	AF_CTRL_MODE_MACRO_FIXED,
	AF_CTRL_MODE_MAX,
};

enum af_ctrl_status_type {
	AF_CTRL_STATUS_INIT,
	AF_CTRL_STATUS_FOCUSED,
	AF_CTRL_STATUS_UNKNOWN,
	AF_CTRL_STATUS_FOCUSING,
	AF_CTRL_STATUS_MAX,
};

enum af_ctrl_lens_status {
	AF_CTRL_LENS_STOP = 1,
	AF_CTRL_LENS_MOVING,
	AF_CTRL_LENS_MOVE_DONE,
	AF_CTRL_LENS_MAX,
};

enum af_ctrl_scene_mode {
	AF_CTRL_SCENE_OFF,
	AF_CTRL_SCENE_AUTO,
	AF_CTRL_SCENE_LANDSCAPE,
	AF_CTRL_SCENE_SNOW,
	AF_CTRL_SCENE_BEACH,
	AF_CTRL_SCENE_SUNSET,
	AF_CTRL_SCENE_NIGHT,
	AF_CTRL_SCENE_PORTRAIT,
	AF_CTRL_SCENE_BACKLIGHT,
	AF_CTRL_SCENE_SPORTS,
	AF_CTRL_SCENE_ANTISHAKE,
	AF_CTRL_SCENE_FLOWERS,
	AF_CTRL_SCENE_CANDLELIGHT,
	AF_CTRL_SCENE_FIREWORKS,
	AF_CTRL_SCENE_PARTY,
	AF_CTRL_SCENE_NIGHT_PORTRAIT,
	AF_CTRL_SCENE_THEATRE,
	AF_CTRL_SCENE_ACTION,
	AF_CTRL_SCENE_AR,
	AF_CTRL_SCENE_FACE_PRIORITY,
	AF_CTRL_SCENE_BARCODE,
	AF_CTRL_SCENE_HDR,
	AF_CTRL_SCENE_MAX
};

enum af_ctrl_roi_type_t {
	AF_CTRL_ROI_TYPE_NORMAL,
	AF_CTRL_ROI_TYPE_FACE,
	AF_CTRL_ROI_TYPE_TOUCH,
	AF_CTRL_ROI_TYPE_MAX,
};

struct af_result_param {
	cmr_u32 motor_pos;
	cmr_u32 suc_win;
};

struct af_ctrl_roi_type {
	cmr_u16 x;
	cmr_u16 y;
	cmr_u16 dx;
	cmr_u16 dy;
};

struct af_ctrl_roi_info_type {
	cmr_int roi_type;
	cmr_u32 num_roi;
	struct af_ctrl_roi_type roi[5];
	cmr_u32 multi_roi_weight[5];
};

#if 0
enum af_lib_fun_type {
	AF_CTRL_LIB_ALTEC,
	AF_CTRL_LIB_SPRD,
	AF_CTRL_LIB_MAX,
};
#endif

struct af_ctrl_motor_pos {
	cmr_u32 motor_pos;
	cmr_s32 motor_offset;
	cmr_u32 skip_frame;
	cmr_u32 vcm_wait_ms;
};

struct af_ctrl_cb_ops_type {
	cmr_int (*set_pos)(cmr_handle caller_handle,
			   struct af_ctrl_motor_pos *in);
	cmr_int (*start_notify)(cmr_handle caller_handle, void *data);
	cmr_int (*end_notify)(cmr_handle caller_handle,
			      struct af_result_param *data);
	cmr_int (*lock_ae_awb)(cmr_handle caller_handle, void *lock);
	cmr_int (*cfg_af_stats)(cmr_handle caller_handle, void *data);
	cmr_int (*cfg_pdaf_enable)(cmr_handle caller_handle, void *data);
	cmr_int (*cfg_pdaf_config)(cmr_handle caller_handle, void *data);
	cmr_int (*get_system_time)(cmr_handle caller_handler, cmr_u32 *sec_ptr,
				   cmr_u32 *usec_ptr);
};

struct af_ctrl_input_pd_info_t {
	cmr_u16 token_id;
	cmr_u32 frame_id;
	cmr_u8 enable;
	struct isp3a_timestamp timestamp;
	void* extend_data_ptr;
	cmr_u32 extend_data_size;
};

enum af_ctrl_lib_product_id {
	ALTEC_AF_LIB,
	SPRD_AF_LIB,
	SFT_AF_LIB,
	ALC_AF_LIB,
};

struct af_ctrl_sensor_crop_info_t {
	/* top x position */
	cmr_u16 x;
	/* top y position */
	cmr_u16 y;
	/* crop width */
	cmr_u16 width;
	/* crop height */
	cmr_u16 height;
};

struct af_ctrl_sensor_info_type {
	cmr_u32 sensor_res_height;
	cmr_u32 sensor_res_width;
	struct af_ctrl_sensor_crop_info_t crop_info;
	struct af_pose_dis pose_dis;
};

struct af_ctrl_module_info_type {
	cmr_u32 f_num;
	cmr_u32 focal_length;
};

struct af_ctrl_otp_info_t {
	void *otp_data;
	cmr_int size;
};

struct af_ctrl_tuning_file_t {
	void *tuning_file;
	cmr_int size;
};

struct af_ctrl_out_status_t {
	cmr_u8 focus_done;
	enum af_ctrl_status_type status;
	float f_distance;
};

struct af_ctrl_stats_t {
	void *stats_config;
	cmr_int stats_size;
};

struct af_ctrl_output_report_t {
	cmr_s32 sof_frame_id;
	cmr_s32 result;
	cmr_s32 type;
	struct af_ctrl_out_status_t focus_status;
	struct af_ctrl_stats_t stats;
	cmr_s32 wrap_result;
};

struct af_ctrl_isp_info_t {
	cmr_u16 img_width;
	cmr_u16 img_height;
};

struct af_ctrl_init_in {
	cmr_u32 camera_id;
	cmr_handle caller_handle;
	cmr_u8 af_support;
	cmr_u8 pdaf_support;
	struct isp_lib_config af_lib_info;
	struct af_ctrl_otp_info_t otp_info;
	struct af_ctrl_tuning_file_t tuning_info[ISP_INDEX_MAX];
	struct af_ctrl_isp_info_t isp_info;
	struct af_ctrl_sensor_info_type sensor_info;
	struct af_ctrl_module_info_type module_info;
	struct af_ctrl_cb_ops_type af_ctrl_cb_ops;
	struct af_ctrl_tuning_file_t caf_tuning_info;
};

struct af_ctrl_init_out {
	struct isp3a_af_hw_cfg hw_cfg;
};

struct af_ctrl_sof_info {
	cmr_u32 sof_frame_idx;
	struct isp3a_timestamp timestamp;
};

struct af_ctrl_debug_info_t {
	void *addr;
	cmr_u32 size;
};

struct af_ctrl_y_info_t {
	cmr_uint yaddr[2];
	cmr_u32 in_proc[2];
};
struct af_ctrl_pd_status_t {
	enum af_ctrl_lens_status lens_status;
	cmr_u8 need_skip_frame;
};
struct af_ctrl_param_in {
	union {
		enum af_ctrl_mode_type af_mode;
		cmr_u16 motor_pos;
		enum af_ctrl_scene_mode af_scene_mode;
		cmr_u16 bypass;
		struct af_ctrl_sof_info sof_info;
		struct isp_face_area face_info;
		struct isp_af_win af_ctrl_roi_info;
		enum isp_flash_mode flash_mode;
		struct af_ctrl_isp_info_t live_view_sz;
		struct af_ctrl_sensor_info_type sensor_info;
		struct af_ctrl_motor_pos pos_info;
		struct isp3a_ae_info ae_info;
		struct af_aux_sensor_info_t aux_info;
		struct af_ctrl_input_pd_info_t pd_info;
		cmr_u8 haf_enable;
		void *y_img;
		cmr_u32 idx_num;
	};
};

struct af_ctrl_param_out {
	union {
		enum af_ctrl_mode_type af_mode;
		cmr_u16 motor_pos;
		enum af_ctrl_scene_mode af_scene_mode;
		cmr_u16 bypass;
		struct af_ctrl_roi_info_type af_ctrl_roi_info;
		enum af_ctrl_status_type af_ctrl_status;
		struct af_ctrl_debug_info_t debug_info;
		struct af_ctrl_debug_info_t exif_info;
		struct af_ctrl_y_info_t y_info;
		struct af_ctrl_pd_status_t pd_status;
	};
};

struct af_ctrl_process_in {
	struct isp3a_statistics_data *statistics_data;
};

struct af_ctrl_process_out {
	void *data;
	cmr_u32 size;
};

cmr_int af_ctrl_init(struct af_ctrl_init_in *in,
		     struct af_ctrl_init_out *out, cmr_handle *handle);

cmr_int af_ctrl_deinit(cmr_handle handle);

cmr_int af_ctrl_process(cmr_handle handle, struct af_ctrl_process_in *in,
			struct af_ctrl_process_out *out);

cmr_int af_ctrl_ioctrl(cmr_handle handle, cmr_int cmd,
		       struct af_ctrl_param_in *in,
		       struct af_ctrl_param_out *out);
#endif
