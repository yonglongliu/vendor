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

#include "isp_awb_types.h"
#include "isp_com.h"
#include "isp_common_types.h"
#ifndef CONFIG_ISP_2_2
#include "isp_bridge.h"
#else
#include "isp_match.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define AWB_CTRL_INVALID_HANDLE NULL
#define AWB_CTRL_ENVI_NUM 8

	typedef void *awb_ctrl_handle_t;

	enum awb_ctrl_rtn {
		AWB_CTRL_SUCCESS = 0,
		AWB_CTRL_ERROR = 255
	};

	enum awb_ctrl_cmd {
		/*
		 * warning if you wanna send async msg
		 * please add msg id below here
		 */
		AWB_SYNC_MSG_BEGIN = 0x00,
		AWB_CTRL_CMD_SET_BASE = 0x100,
		AWB_CTRL_CMD_SET_WB_MODE = 0x102,
		AWB_CTRL_CMD_SET_FLASH_MODE = 0x103,
		AWB_CTRL_CMD_SET_STAT_IMG_SIZE = 0x104,
		AWB_CTRL_CMD_SET_STAT_IMG_FORMAT = 0x105,
		AWB_CTRL_CMD_SET_WORK_MODE = 0x106,
		AWB_CTRL_CMD_SET_UPDATE_TUNING_PARAM = 0X107,
		AWB_CTRL_CMD_GET_BASE = 0x200,
		AWB_CTRL_CMD_GET_PARAM_WIN_START = 0X201,
		AWB_CTRL_CMD_GET_PARAM_WIN_SIZE = 0x202,
		AWB_CTRL_CMD_FLASH_OPEN_P = 0x300,
		AWB_CTRL_CMD_FLASH_OPEN_M = 0x301,
		AWB_CTRL_CMD_FLASHING = 0x302,
		AWB_CTRL_CMD_FLASH_CLOSE = 0x303,
		AWB_CTRL_CMD_LOCK = 0x304,
		AWB_CTRL_CMD_UNLOCK = 0x305,
		AWB_CTRL_CMD_GET_STAT_SIZE = 0x306,
		AWB_CTRL_CMD_GET_WIN_SIZE = 0x307,
		AWB_CTRL_CMD_GET_CT = 0x308,
		AWB_CTRL_CMD_FLASH_BEFORE_P = 0x309,
		AWB_CTRL_CMD_SET_FLASH_STATUS = 0x30A,
		AWB_CTRL_CMD_GET_DEBUG_INFO = 0x30B,
		AWB_CTRL_CMD_GET_PIX_CNT = 0x30C,
		AWB_CTRL_CMD_VIDEO_STOP_NOTIFY = 0x30D,
		AWB_CTRL_CMD_FLASH_SNOP = 0X30f,
		AWB_CTRL_CMD_EM_GET_PARAM = 0x400,
		AWB_CTRL_CMD_GET_CT_TABLE20 = 0x500,
		AWB_CTRL_CMD_GET_DATA_TYPE = 0x600,
		AWB_SYNC_MSG_END,
		/*
		 * warning if you wanna set ioctrl directly
		 * please add msg id below here
		 */
		AWB_DIRECT_MSG_BEGIN,
		AWB_CTRL_CMD_GET_GAIN,
		AWB_CTRL_CMD_GET_CUR_GAIN,
		AWB_CTRL_CMD_GET_WB_MODE,
		AWB_CTRL_CMD_RESULT_INFO,
		AWB_DIRECT_MSG_END
	};

	enum awb_ctrl_wb_mode {
		AWB_CTRL_WB_MODE_AUTO = 0x0,
		AWB_CTRL_MWB_MODE_SUNNY = 0x1,
		AWB_CTRL_MWB_MODE_CLOUDY = 0x2,
		AWB_CTRL_MWB_MODE_FLUORESCENT = 0x3,
		AWB_CTRL_MWB_MODE_INCANDESCENT = 0x4,
		AWB_CTRL_MWB_MODE_USER_0 = 0x5,
		AWB_CTRL_MWB_MODE_USER_1 = 0x6,
		AWB_CTRL_AWB_MODE_OFF = 0x7
	};

	enum awb_ctrl_scene_mode {
		AWB_CTRL_SCENEMODE_AUTO = 0x0,
		AWB_CTRL_SCENEMODE_DUSK = 0x1,
		AWB_CTRL_SCENEMODE_USER_0 = 0x2,
		AWB_CTRL_SCENEMODE_USER_1 = 0x3
	};
	enum awb_ctrl_stat_img_format {
		AWB_CTRL_STAT_IMG_CHN = 0x0,
		AWB_CTRL_STAT_IMG_RAW_8 = 0x1,
		AWB_CTRL_STAT_IMG_RAW_16 = 0x2
	};

	enum awb_ctrl_flash_mode {
		AWB_CTRL_FLASH_END = 0x0,
		AWB_CTRL_FLASH_PRE = 0x1,
		AWB_CTRL_FLASH_MAIN = 0x2,
	};

	enum awb_ctrl_flash_status {
		AWB_FLASH_PRE_BEFORE,
		AWB_FLASH_PRE_LIGHTING,
		AWB_FLASH_PRE_AFTER,
		AWB_FLASH_MAIN_BEFORE,
		AWB_FLASH_MAIN_LIGHTING,
		AWB_FLASH_MAIN_MEASURE,
		AWB_FLASH_MAIN_AFTER,
		AWB_FLASH_MODE_MAX
	};

	struct awb_ctrl_chn_img {
		cmr_u32 *r;
		cmr_u32 *g;
		cmr_u32 *b;
	};

	struct awb_ctrl_weight {
		cmr_u32 value[2];
		cmr_u32 weight[2];
	};
	struct awb_ctrl_size {
		cmr_u16 w;
		cmr_u16 h;
	};

	struct awb_ctrl_pos {
		cmr_s16 x;
		cmr_s16 y;
	};

	struct awb_flash_info {
		enum awb_ctrl_flash_mode flash_mode;
		struct awb_ctrl_gain flash_ratio;
		cmr_u32 effect;
		cmr_u32 patten;
		cmr_u32 flash_enable;
		cmr_u32 main_flash_enable;
		enum awb_ctrl_flash_status flash_status;
	};
	union awb_ctrl_stat_img {
		struct awb_ctrl_chn_img chn_img;
		cmr_u8 *raw_img_8;
		cmr_u16 *raw_img_16;
	};

	struct awb_ctrl_rgb_l {
		cmr_u32 r;
		cmr_u32 g;
		cmr_u32 b;
	};

	struct awb_ctrl_opt_info {
		struct awb_ctrl_rgb_l gldn_stat_info;
		struct awb_ctrl_rgb_l rdm_stat_info;
	};
	struct awb_ctrl_init_param {
		cmr_u32 base_gain;
		cmr_u32 awb_enable;
		enum awb_ctrl_wb_mode wb_mode;
		enum awb_ctrl_stat_img_format stat_img_format;
		struct awb_ctrl_size stat_img_size;
		struct awb_ctrl_size stat_win_size;
		struct awb_ctrl_opt_info otp_info;
		struct third_lib_info lib_param;
		void *tuning_param;
		cmr_u32 param_size;
		cmr_u32 camera_id;
		void *priv_handle;

		/*
		 * for dual camera sync
		 */
		cmr_u8 sensor_role;
		cmr_u32 is_multi_mode;
		func_isp_br_ioctrl ptr_isp_br_ioctrl;

		struct sensor_otp_cust_info *otp_info_ptr;
		cmr_u8 is_master;
	};

	struct awb_ctrl_init_result {
		struct awb_ctrl_gain gain;
		cmr_u32 ct;
//ALC_S 20150517
		cmr_u32 use_ccm;
		cmr_u16 ccm[9];
//ALC_S 20150517

//ALC_S 20150519
		cmr_u32 use_lsc;
		cmr_u16 *lsc;
//ALC_S 20150519
		cmr_u32 lsc_size;
	};

/*ALC_S*/
	struct awb_ctrl_ae_info {
		cmr_s32 bv;
		cmr_s32 iso;
		float gain;
		float exposure;
		float f_value;
		cmr_u32 stable;

		cmr_s32 ev_index;		/* 0 ~ 15, such as 0(ev-2.0), 1(ev-1.5), 2(ev-1.0), 3(ev-0.5), 4(ev0), 5(ev+0.5), 6(ev+1.0), 7(ev+1.5), 8(ev+2.0) */
		cmr_s32 ev_table[16];
	};

	struct awb_ctrl_calc_param {
		cmr_u32 quick_mode;
		cmr_s32 bv;
		union awb_ctrl_stat_img stat_img;	// stat from aem

		union awb_ctrl_stat_img stat_img_awb;	// stat from binning
		cmr_u32 stat_width_awb;
		cmr_u32 stat_height_awb;

		struct awb_ctrl_ae_info ae_info;
		cmr_u32 scalar_factor;

		// just for simulation
		cmr_s32 matrix[9];
		cmr_u8 gamma[256];
	};

	struct awb_ctrl_lock_info {
		struct awb_ctrl_gain lock_gain;
		cmr_u32 lock_mode;
		cmr_u32 lock_ct;
		cmr_u32 lock_num;
		cmr_u32 lock_flash_frame;	// recovery flash awb continus frames
		cmr_u32 unlock_num;
	};

	struct awb_data_info {
		void *data_ptr;
		cmr_u32 data_size;
	};

	struct awb_ctrl_msg_ctrl {
		cmr_int cmd;
		cmr_handle in;
		cmr_handle out;
	};

	cmr_int awb_ctrl_init(struct awb_ctrl_init_param *input_ptr, cmr_handle * handle_awb);
	cmr_int awb_ctrl_process(cmr_handle handle_awb, struct awb_ctrl_calc_param *param, struct awb_ctrl_calc_result *result);
	cmr_int awb_ctrl_deinit(cmr_handle * handle_awb);
	cmr_int awb_ctrl_ioctrl(cmr_handle handle_awb, enum awb_ctrl_cmd cmd, void *in_ptr, void *out_ptr);

#ifdef __cplusplus
}
#endif
#endif
