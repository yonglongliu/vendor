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
#ifndef _PDAF_CTRL_H_
#define _PDAF_CTRL_H_
#include "isp_common_types.h"


enum pdaf_ctrl_data_type {
	PDAF_DATA_TYPE_RAW = 0,
	PDAF_DATA_TYPE_LEFT,
	PDAF_DATA_TYPE_RIGHT,
	PDAF_DATA_TYPE_OUT,
	PDAF_DATA_TYPE_RAW_OUT,
	PDAF_DATA_TYPE_MAX
};
enum pdaf_ctrl_cmd_type {
	PDAF_CTRL_CMD_SET_OPEN,
	PDAF_CTRL_CMD_SET_CLOSE,

	/*
	 * warning if you wanna set ioctrl directly
	 * please add msg id below here
	  * */
	PDAF_CTRL_CMD_DIRECT_BEGIN,
	PDAF_CTRL_CMD_GET_BUSY,
	PDAF_CTRL_CMD_SET_CONFIG,
	PDAF_CTRL_CMD_SET_ENABLE,
	PDAF_CTRL_CMD_DIRECT_END,
};

enum pdaf_ctrl_lib_product_id {
	ALTEC_PDAF_LIB,
	SPRD_PDAF_LIB,
	SFT_PDAF_LIB,
	ALC_PDAF_LIB,
};

enum pdaf_ctrl_data_bit {
	PD_DATA_BIT_8 = 8,
	PD_DATA_BIT_10 = 10,
	PD_DATA_BIT_12 = 12,
};

struct pdaf_ctrl_otp_info_t {
	void *otp_data;
	cmr_int size;
};

struct pdaf_ctrl_process_in {
	cmr_u8 bit;
	cmr_u16 dcurrentVCM;
	cmr_s32 dBv;
	struct pd_raw_info pd_raw;
};

struct pdaf_ctrl_process_out {
	struct isp3a_pdaf_altek_report_t pd_report_data;
};

struct pdaf_ctrl_callback_in {
	union {
	struct pdaf_ctrl_process_out proc_out;
	};
};

struct pdaf_ctrl_cb_ops_type {
	cmr_int (*call_back)(cmr_handle caller_handle,
			   struct pdaf_ctrl_callback_in *in);
};

struct pdaf_ctrl_param_in {
	union {
		cmr_u8 pd_enable;
		struct isp3a_pd_config_t *pd_config;
		cmr_int (*pd_set_buffer) (struct pd_frame_in *cb_param);
	};
};

struct pdaf_ctrl_param_out {
	union {
		cmr_u8 is_busy;
	};
};

struct pdaf_ctrl_init_in {
	cmr_u32 camera_id;
	cmr_handle caller_handle;
	cmr_u8 pdaf_support;
	cmr_s8 *name;
	void *tuning_param[ISP_INDEX_MAX];
	struct isp_lib_config pdaf_lib_info;
	struct pdaf_ctrl_otp_info_t af_otp;
	struct pdaf_ctrl_otp_info_t pdaf_otp;
	struct sensor_pdaf_info *pd_info;
	struct pdaf_ctrl_cb_ops_type pdaf_ctrl_cb_ops;
};

struct pdaf_ctrl_init_out {
	union {
		cmr_u8 enable;
		cmr_u8 init_success;
	};
};

cmr_int pdaf_ctrl_init(struct pdaf_ctrl_init_in *in,
		     struct pdaf_ctrl_init_out *out, cmr_handle *handle);

cmr_int pdaf_ctrl_deinit(cmr_handle handle);

cmr_int pdaf_ctrl_process(cmr_handle handle, struct pdaf_ctrl_process_in *in,
			struct pdaf_ctrl_process_out *out);

cmr_int pdaf_ctrl_ioctrl(cmr_handle handle, cmr_int cmd,
		       struct pdaf_ctrl_param_in *in,
		       struct pdaf_ctrl_param_out *out);
#endif
