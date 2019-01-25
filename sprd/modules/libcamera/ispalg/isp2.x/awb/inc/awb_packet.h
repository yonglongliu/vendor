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
#ifndef _AWB_PACKET_H_
#define _AWB_PACKET_H_

#ifndef WIN32
#include <linux/types.h>
#include <sys/types.h>
#else
#include "sci_types.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define AWB_PACKET_ERROR	255
#define AWB_PACKET_SUCCESS	0

#define AWB_ALG_RESOLUTION_NUM 	8
#define AWB_ALG_MWB_NUM		20
#define AWB_CTRL_SCENEMODE_NUM 10

	struct awb_alg_size {
		cmr_u16 w;
		cmr_u16 h;
	};

	struct awb_alg_pos {
		cmr_s16 x;
		cmr_s16 y;
	};

	struct awb_param_ctrl {
		/*window size of statistic image */
		struct awb_alg_size stat_win_size;
		/*start position of statistic area */
		struct awb_alg_pos stat_start_pos;
		/*compensate gain for each resolution */
		struct awb_alg_gain compensate_gain[AWB_ALG_RESOLUTION_NUM];
		/*gain for each manual white balance */
		struct awb_alg_gain mwb_gain[AWB_ALG_MWB_NUM];
		/*gain for each scenemode gain */
		struct awb_alg_gain scene_gain[AWB_CTRL_SCENEMODE_NUM];
		/*bv value range for awb */
		struct awb_alg_bv bv_range;
		/*init gain and ct */
		struct awb_alg_gain init_gain;
		cmr_u32 init_ct;
	};

	struct awb_param_tuning {
		struct awb_param_ctrl common;
		/*algorithm param */
		void *alg_param;
		/*algorithm param size */
		cmr_u32 alg_param_size;
	};

//cmr_s32 awb_param_pack(struct awb_param *awb_param, cmr_u32 pack_buf_size, void *pack_buf, cmr_u32 *pack_data_size);
	cmr_s32 awb_param_unpack(void *pack_data, cmr_u32 data_size, struct awb_param_tuning *tuning_param);

#ifdef __cplusplus
}
#endif
#endif
