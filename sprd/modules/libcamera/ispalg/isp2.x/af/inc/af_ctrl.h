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

#ifndef _ISP_AF_H_
#define _ISP_AF_H_

#include "isp_adpt.h"
#include "sensor_raw.h"

#ifdef WIN32
#include "sci_type.h"
#endif

#ifdef  __cplusplus
extern "C" {
#endif

#define MAX_AF_WINS 32

	enum af_err_type {
		AF_SUCCESS = 0x00,
		AF_ERROR,
		AF_ERR_MAX
	};

	enum scene {
		OUT_SCENE = 0,
		INDOOR_SCENE,			// INDOOR_SCENE,
		DARK_SCENE,				// DARK_SCENE,
		SCENE_NUM,
	};

	enum af_mode {
		AF_MODE_NORMAL = 0x00,
		AF_MODE_MACRO,
		AF_MODE_CONTINUE,
		AF_MODE_VIDEO,
		AF_MODE_MANUAL,
		AF_MODE_PICTURE,
		AF_MODE_FULLSCAN,
		AF_MODE_MAX
	};

	enum af_cmd {
		AF_CMD_SET_BASE = 0x1000,
		AF_CMD_SET_AF_MODE = 0x1001,
		AF_CMD_SET_AF_POS = 0x1002,
		AF_CMD_SET_TUNING_MODE = 0x1003,
		AF_CMD_SET_SCENE_MODE = 0x1004,
		AF_CMD_SET_AF_START = 0x1005,
		AF_CMD_SET_AF_STOP = 0x1006,
		AF_CMD_SET_AF_RESTART = 0x1007,
		AF_CMD_SET_CAF_RESET = 0x1008,
		AF_CMD_SET_CAF_STOP = 0x1009,
		AF_CMD_SET_AF_FINISH = 0x100A,
		AF_CMD_SET_AF_BYPASS = 0x100B,
		AF_CMD_SET_DEFAULT_AF_WIN = 0x100C,
		AF_CMD_SET_FLASH_NOTICE = 0x100D,
		AF_CMD_SET_ISP_START_INFO = 0x100E,
		AF_CMD_SET_ISP_STOP_INFO = 0x100F,
		AF_CMD_SET_ISP_TOOL_AF_TEST = 0x1010,
		AF_CMD_SET_CAF_TRIG_START = 0x1011,
		AF_CMD_SET_AE_INFO = 0x1012,
		AF_CMD_SET_AWB_INFO = 0x1013,
		AF_CMD_SET_FACE_DETECT = 0x1014,
		AF_CMD_SET_DCAM_TIMESTAMP = 0x1015,
		AF_CMD_SET_PD_INFO = 0x1016,
		AF_CMD_SET_UPDATE_AUX_SENSOR = 0x1017,
		// SharkLE Only ++
		AF_CMD_SET_DAC_INFO = 0x1018,
		// SharkLE Only --
		AF_CMD_SET_MAX,

		AF_CMD_GET_BASE = 0x2000,
		AF_CMD_GET_AF_MODE = 0X2001,
		AF_CMD_GET_AF_CUR_POS = 0x2002,
		AF_CMD_GET_AF_INIT_POS = 0x2003,
		AF_CMD_GET_MULTI_WIN_CFG = 0x2004,
		AF_CMD_GET_AF_FULLSCAN_INFO = 0x2005,
		AF_CMD_GET_AF_LOG_INFO = 0x2006,
		AF_CMD_GET_AFT_LOG_INFO = 0x2007,
		AF_CMD_GET_MAX,
	};

	enum af_calc_data_type {
		AF_DATA_AFM_STAT = 0,
		AF_DATA_AF = (1 << 0),	// 0x1
		AF_DATA_IMG_BLK = (1 << 1),	// 0x2
		AF_DATA_AE = (1 << 2),	// 0x4
		AF_DATA_FD = (1 << 3),	// 0x8
		AF_DATA_PD = (1 << 4),	// 0x10
		AF_DATA_G = (1 << 5),	// 0x20
		AF_DATA_MAX
	};

	enum af_sensor_type {
		AF_SENSOR_ACCELEROMETER,
		AF_SENSOR_MAGNETIC_FIELD,
		AF_SENSOR_GYROSCOPE,
		AF_SENSOR_LIGHT,
		AF_SENSOR_PROXIMITY,
	};

	enum af_flash_status {
		AF_FLASH_PRE_BEFORE,
		AF_FLASH_PRE_LIGHTING,
		AF_FLASH_PRE_AFTER,
		AF_FLASH_MAIN_BEFORE,
		AF_FLASH_MAIN_LIGHTING,
		AF_FLASH_MAIN_AE_MEASURE,
		AF_FLASH_MAIN_AFTER,
		AF_FLASH_AF_DONE,
		AF_FLASH_SLAVE_FLASH_OFF,
		AF_FLASH_SLAVE_FLASH_TORCH,
		AF_FLASH_SLAVE_FLASH_AUTO,
		AF_FLASH_MODE_MAX
	};

	enum af_locker_type {
		AF_LOCKER_AE,
		AF_LOCKER_AE_CAF,
		AF_LOCKER_AWB,
		AF_LOCKER_LSC,
		AF_LOCKER_NLM,
		AF_LOCKER_MAX
	};

	enum af_focus_type {
		AF_FOCUS_SAF,
		AF_FOCUS_CAF,
		AF_FOCUS_FAF,
		AF_FOCUS_MAX
	};

	enum af_cb_cmd {
		AF_CB_CMD_SET_START_NOTICE = 0x1000,
		AF_CB_CMD_SET_END_NOTICE,
		AF_CB_CMD_SET_AE_LOCK,
		AF_CB_CMD_SET_AE_UNLOCK,
		AF_CB_CMD_SET_AE_CAF_LOCK,
		AF_CB_CMD_SET_AE_CAF_UNLOCK,
		AF_CB_CMD_SET_AWB_LOCK,
		AF_CB_CMD_SET_AWB_UNLOCK,
		AF_CB_CMD_SET_LSC_LOCK,
		AF_CB_CMD_SET_LSC_UNLOCK,
		AF_CB_CMD_SET_NLM_LOCK,
		AF_CB_CMD_SET_NLM_UNLOCK,
		AF_CB_CMD_SET_MOTOR_POS,
		AF_CB_CMD_SET_PULSE_LINE,
		AF_CB_CMD_SET_NEXT_VCM_POS,
		AF_CB_CMD_SET_CLEAR_NEXT_VCM_POS,
		AF_CB_CMD_SET_PULSE_LOG,
		AF_CB_CMD_SET_MOTOR_BESTMODE,
		AF_CB_CMD_SET_VCM_TEST_MODE,
		AF_CB_CMD_SET_MONITOR,
		AF_CB_CMD_SET_MONITOR_WIN,
		AF_CB_CMD_SET_AFM_BYPASS,
		AF_CB_CMD_SET_AFM_SKIP_NUM,
		AF_CB_CMD_SET_AFM_MODE,
		AF_CB_CMD_SET_AFM_NR_CFG,
		AF_CB_CMD_SET_AFM_MODULES_CFG,
		AF_CB_CMD_SET_MAX,

		AF_CB_CMD_GET_MONITOR_WIN_NUM = 0x2000,
		AF_CB_CMD_GET_LENS_OTP,
		AF_CB_CMD_GET_MOTOR_POS,
		AF_CB_CMD_GET_VCM_TEST_MODE,
		AF_CB_CMD_GET_SYSTEM_TIME,
		AF_CB_CMD_GET_MAX,
	};

	enum af_move_mode {
		AF_MOVE_START = 0x00,
		AF_MOVE_END,
		AF_MOVE_MAX
	};

	struct af_img_blk_info {
		cmr_u32 block_w;
		cmr_u32 block_h;
		cmr_u32 pix_per_blk;
		cmr_u32 chn_num;
		cmr_u32 *data;
	};

	struct af_img_blk_statistic {
		cmr_u32 r_info[1024];
		cmr_u32 g_info[1024];
		cmr_u32 b_info[1024];
	};

	struct af_ae_calc_out {
		cmr_s32 bv;
		cmr_u32 is_stab;
		cmr_u32 cur_exp_line;
		cmr_u32 cur_dummy;
		cmr_u32 frame_line;
		cmr_u32 line_time;
		cmr_u32 cur_again;
		cmr_u32 cur_dgain;
		cmr_u32 cur_lum;
		cmr_u32 target_lum;
		cmr_u32 target_lum_ori;
		cmr_u32 flag4idx;
		cmr_u32 cur_ev;
		cmr_u32 cur_index;
		cmr_u32 cur_iso;
	};

	struct afctrl_ae_info {
		cmr_u32 is_update;
		struct af_img_blk_info img_blk_info;
		struct af_ae_calc_out ae_rlt_info;
	};

	struct afctrl_awb_info {
		cmr_u32 is_update;
		cmr_u32 r_gain;
		cmr_u32 g_gain;
		cmr_u32 b_gain;
	};

	struct afctrl_gyro_info {
		cmr_s64 timestamp;
		float x;
		float y;
		float z;
	};

	struct afctrl_gsensor_info {
		cmr_s64 timestamp;
		float vertical_up;
		float vertical_down;
		float horizontal;
		cmr_u32 valid;
	};

	struct afctrl_sensor_info_t {
		enum af_sensor_type type;
		union {
			struct afctrl_gyro_info gyro_info;
			struct afctrl_gsensor_info gsensor_info;
		};
	};

	struct afctrl_face_area {
		cmr_u32 sx;
		cmr_u32 sy;
		cmr_u32 ex;
		cmr_u32 ey;
		cmr_u32 brightness;
		cmr_s32 pose;
		cmr_s32 angle;
	};

	struct afctrl_face_info {
		cmr_u16 type;			// focus or ae,
		cmr_u16 face_num;
		cmr_u16 frame_width;
		cmr_u16 frame_height;
		struct afctrl_face_area face_info[10];
	};

	struct afctrl_fwstart_info {
		struct isp_size size;
		cmr_u32 reserved[10];
	};

	struct afctrl_ts_info {
		cmr_u64 timestamp;
		cmr_u32 capture;
	};

	struct afctrl_fps_info {
		cmr_u32 is_high_fps;
		cmr_u32 high_fps_skip_num;
	};

	struct af_motor_pos {
		cmr_u32 motor_pos;
		cmr_u32 skip_frame;
		cmr_u32 wait_time;
	};

	struct af_result_param {
		cmr_u32 motor_pos;
		cmr_u32 suc_win;
		cmr_u32 focus_type;
		cmr_u32 reserved[10];
	};

	struct af_notice {
		cmr_u32 mode;
		cmr_u32 valid_win;
		cmr_u32 focus_type;
		cmr_u32 reserved[10];
	};

	struct af_monitor_set {
		cmr_u32 type;
		cmr_u32 bypass;
		cmr_u32 int_mode;
		cmr_u32 skip_num;
		cmr_u32 need_denoise;
	};

	struct af_win_rect {
		cmr_u32 sx;
		cmr_u32 sy;
		cmr_u32 ex;
		cmr_u32 ey;
	};

	struct af_monitor_win {
		cmr_u32 type;
		struct af_win_rect *win_pos;
	};

	struct af_trig_info {
		cmr_u32 win_num;
		cmr_u32 mode;
		struct af_win_rect win_pos[MAX_AF_WINS];
	};

	struct af_otp_data {
		cmr_u16 infinite_cali;
		cmr_u16 macro_cali;
	};

	struct afctrl_otp_info {
		struct af_otp_data gldn_data;
		struct af_otp_data rdm_data;
	};

	struct afctrl_cb_ops {
		cmr_s32(*start_notice) (void *handle, struct af_result_param * in_param);
		cmr_s32(*end_notice) (void *handle, struct af_result_param * in_param);
		cmr_s32(*set_monitor) (void *handle, struct af_monitor_set * in_param, cmr_u32 cur_envi);
		cmr_s32(*set_monitor_win) (void *handle, struct af_monitor_win * in_param);
		cmr_s32(*get_monitor_win_num) (void *handle, cmr_u32 * win_num);
		cmr_s32(*lock_module) (void *handle, cmr_int af_locker_type);
		cmr_s32(*unlock_module) (void *handle, cmr_int af_locker_type);
		cmr_u32(*af_get_otp) (void *handle, uint16_t * inf, uint16_t * macro);
		cmr_u32(*af_set_motor_pos) (void *handle, cmr_u16 pos);
		cmr_u32(*af_get_motor_pos) (void *handle, cmr_u16 * motor_pos);
		cmr_u32(*af_set_motor_bestmode) (void *handle);
		cmr_u32(*af_get_test_vcm_mode) (void *handle);
		cmr_u32(*af_set_test_vcm_mode) (void *handle, char *vcm_mode);
		cmr_s32(*af_monitor_bypass) (void *handle, cmr_u32 * bypass);
		cmr_s32(*af_monitor_skip_num) (void *handle, cmr_u32 * afm_skip_num);
		cmr_s32(*af_monitor_mode) (void *handle, cmr_u32 * afm_mode);
		cmr_s32(*af_monitor_iir_nr_cfg) (void *handle, void *af_iir_nr);
		cmr_s32(*af_monitor_module_cfg) (void *handle, void *af_enhanced_module);
		cmr_s32(*af_get_system_time) (void *handle, cmr_u32 * sec, cmr_u32 * usec);
		// SharkLE Only ++
		cmr_s32(*af_set_pulse_line) (void *handle, cmr_u32 line);
		cmr_s32(*af_set_next_vcm_pos) (void *handle, cmr_u32 pos);
		cmr_s32(*af_set_pulse_log) (void *handle, cmr_u32 flag);
		cmr_s32(*af_set_clear_next_vcm_pos) (void *handle);
		// SharkLE Only --
	};

	struct af_log_info {
		void *log_cxt;
		cmr_u32 log_len;
	};

	typedef cmr_int(*af_ctrl_cb) (cmr_handle handle, cmr_int type, void *param0, void *param1);

	struct afctrl_init_in {
		cmr_handle caller_handle;	// struct isp_alg_fw_context *cxt
		af_ctrl_cb af_set_cb;
		cmr_u32 camera_id;
		struct third_lib_info lib_param;
		struct isp_size src;
		cmr_handle caller;		// struct afctrl_cxt *cxt_ptr
		struct afctrl_otp_info otp_info;
		cmr_u32 is_multi_mode;
		cmr_u32 is_supoprt;
		cmr_u8 *aftuning_data;
		cmr_u32 aftuning_data_len;
		cmr_u8 *pdaftuning_data;
		cmr_u32 pdaftuning_data_len;
		cmr_u8 *afttuning_data;
		cmr_u32 afttuning_data_len;
		struct sensor_otp_cust_info *otp_info_ptr;
		cmr_u8 is_master;
		struct afctrl_cb_ops cb_ops;
	};

	struct afctrl_init_out {
		struct af_log_info log_info;
		cmr_u32 init_motor_pos;
	};

	struct afctrl_calc_in {
		cmr_u32 data_type;
		void *data;
		struct afctrl_fps_info sensor_fps;
	};

	struct afctrl_calc_out {
		cmr_u32 motor_pos;
		cmr_u32 suc_win;
	};

#define AREA_LOOP 4

	struct pd_result {
		/*TBD get reset from */
		cmr_s32 pdConf[AREA_LOOP + 1];
		double pdPhaseDiff[AREA_LOOP + 1];
		cmr_s32 pdGetFrameID;
		cmr_s32 pdDCCGain[AREA_LOOP + 1];
		cmr_u32 pd_roi_num;
	};

	cmr_int af_ctrl_init(struct afctrl_init_in *input_ptr, cmr_handle * handle_af);
	cmr_int af_ctrl_deinit(cmr_handle * handle_af);
	cmr_int af_ctrl_process(cmr_handle handle_af, void *in_ptr, struct afctrl_calc_out *result);
	cmr_int af_ctrl_ioctrl(cmr_handle handle_af, cmr_int cmd, void *in_ptr, void *out_ptr);

#ifdef	 __cplusplus
}
#endif
#endif
