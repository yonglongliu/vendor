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

#ifndef _AFL_CTRL_TYPES_H_
#define _AFL_CTRL_TYPES_H_
/*----------------------------------------------------------------------------*
 **				 Dependencies				*
 **---------------------------------------------------------------------------*/
#include "isp_common_types.h"
#include "isp_type.h"
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
enum afl_ctrl_cmd {
	AFL_CTRL_SET_WORK_MODE,
	AFL_CTRL_SET_FLICKER,
	AFL_CTRL_SET_ENABLE,
	AFL_CTRL_SET_PREVIOUS_DATA_INTERVAL,
	AFL_CTRL_SET_SHIFT_INFO,
	AFL_CTRL_GET_SUCCESS_NUM,
	AFL_CTRL_SET_STAT_QUEUE_RELEASE,
	AFL_CTRL_SET_UI_FLICKER_MODE,
	AFL_CTRL_GET_DEBUG_DATA,
	AFL_CTRL_GET_EXIF_DATA,
	AFL_CTRL_CMD_MAX
};

enum afl_ctrl_flicker_mode {
	AFL_CTRL_FLICKER_50HZ,
	AFL_CTRL_FLICKER_60HZ,
	AFL_CTRL_FLICKER_AUTO,
	AFL_CTRL_FLICKER_OFF,
	AFL_CTRL_FLICKER_MAX
};

enum afl_ctrl_cb_type {
	AFL_CTRL_CB_FLICKER_MODE,
	AFL_CTRL_CB_STAT_DATA,
	AFL_CTRL_CB_MAX
};

struct afl_ctrl_callback_in {
	union {
	cmr_s32 flicker_mode;
	struct isp3a_statistics_data *stat_data;
	};
};

struct afl_ctrl_param_mode {
	cmr_s32 flicker_mode;
};

struct afl_ctrl_param_enable {
	cmr_s32 flicker_enable;
};

struct afl_ctrl_param_resolution {
	struct isp_size frame_size;
	cmr_u32 line_time;
};

struct afl_ctrl_param_ref_data {
	cmr_s32 data_interval;
};

struct afl_ctrl_param_shift_info {
	cmr_u32 avgmean;
	cmr_u32 center_mean2x2;
	cmr_s32 bv;
	cmr_u32 exposure_time;
	cmr_u16 adgain;
	cmr_u16 iso;
};

struct afl_ctrl_param_init {
	struct afl_ctrl_param_resolution resolution;
};

struct afl_ctrl_work_param {
	cmr_u32 work_mode;
	cmr_u32 capture_mode;
	struct afl_ctrl_param_resolution resolution;
};

struct afl_ctrl_debug_param {
	cmr_u32  size;
	void    *data;
};


struct afl_ctrl_ops_in {
	cmr_int (*afl_callback)(cmr_handle handler, enum afl_ctrl_cb_type cmd, struct afl_ctrl_callback_in *in_ptr);
};

struct afl_ctrl_proc_out {
};

struct afl_ctrl_proc_in {
	void *prev_frame;
	void *cur_frame;
	cmr_u32 total_queue;
	cmr_u32 ref_queue;
	struct isp3a_statistics_data *stat_data_ptr;
};


struct afl_ctrl_param_out {
	union {
	cmr_u32 flicker_mode;
	struct isp3a_afl_hw_cfg hw_cfg;
	struct afl_ctrl_debug_param debug_param;
	struct afl_ctrl_debug_param exif_param;
	};
};

struct afl_ctrl_param_in {
	union {
	struct afl_ctrl_param_init init;
	struct afl_ctrl_work_param work_param;
	struct afl_ctrl_param_mode mode;
	struct afl_ctrl_param_enable enable;
	struct afl_ctrl_param_ref_data ref_data;
	struct afl_ctrl_param_shift_info shift_info;
	};
};

struct afl_ctrl_init_out {
	cmr_u32 value;
	struct isp3a_afl_hw_cfg hw_cfg;
};


struct afl_ctrl_init_in {
	cmr_u32 camera_id;
	cmr_handle caller_handle;
	struct afl_ctrl_param_init init_param;
	struct afl_ctrl_ops_in  ops_in;
	struct isp_lib_config  lib_param;
};

/**----------------------------------------------------------------------------*
**					Compiler Flag				**
**----------------------------------------------------------------------------*/
#ifdef __cplusplus
}
#endif
/**---------------------------------------------------------------------------*/
#endif

