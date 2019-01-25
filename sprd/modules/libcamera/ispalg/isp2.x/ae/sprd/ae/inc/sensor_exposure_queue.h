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

#ifndef _SENSOR_EXPOSURE_QUEUE_H_
#define _SENSOR_EXPOSURE_QUEUE_H_

#include "cmr_types.h"

#ifdef __cplusplus
extern "C" {
#endif

	struct q_item {
		cmr_u32 exp_time;
		cmr_u32 exp_line;
		cmr_u32 dumy_line;
		cmr_u32 sensor_gain;
		cmr_u32 isp_gain;
	};

	struct s_q_open_param {
		cmr_s32 exp_valid_num;
		cmr_s32 sensor_gain_valid_num;
		cmr_s32 isp_gain_valid_num;
	};

	struct s_q_init_in {
		cmr_u32 exp_line;
		cmr_u32 exp_time;
		cmr_u32 dmy_line;
		cmr_u32 sensor_gain;
		cmr_u32 isp_gain;
#ifdef CONFIG_CAMERA_DUAL_SYNC
		cmr_u32 slave_exp_line;	//it is invalid value while 0
		cmr_u32 slave_gain;		//it is invalid value while 0
		cmr_u32 slave_exp_time;
		cmr_u32 slave_dummy;
#endif
	};

	struct s_q_init_out {
		struct q_item actual_item;
		struct q_item write_item;
	};

	cmr_handle s_q_open(struct s_q_open_param *param);
	cmr_s32 s_q_init(cmr_handle q_handle, struct s_q_init_in *in, struct s_q_init_out *out);
	cmr_s32 s_q_put(cmr_handle q_handle, struct q_item *input_item, struct q_item *write_2_sensor, struct q_item *actual_item);
	cmr_s32 s_q_close(cmr_handle q_handle);

#ifdef __cplusplus
}
#endif
#endif
