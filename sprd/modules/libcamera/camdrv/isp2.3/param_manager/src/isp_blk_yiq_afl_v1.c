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
#define LOG_TAG "isp_blk_yiq_afl_v1"
#include "isp_blocks_cfg.h"

cmr_s32 _pm_yiq_afl_init_v1(void *dst_afl_param, void *src_afl_param, void *param1, void *param_ptr2)
{
	cmr_s32 rtn = ISP_SUCCESS;
	struct isp_yiq_afl_param_v1 *dst_ptr = (struct isp_yiq_afl_param_v1 *)dst_afl_param;
	struct sensor_y_afl_param_v1 *src_ptr = (struct sensor_y_afl_param_v1 *)src_afl_param;
	struct isp_pm_block_header *header_ptr = (struct isp_pm_block_header *)param1;
	UNUSED(param_ptr2);

	dst_ptr->cur.bypass = header_ptr->bypass;

	dst_ptr->cur.mode = 0;
	dst_ptr->cur.skip_frame_num = src_ptr->skip_num;
	dst_ptr->cur.frame_num = src_ptr->frm_num;
	dst_ptr->cur.start_col = src_ptr->col_st;
	dst_ptr->cur.end_col = src_ptr->col_ed;
	dst_ptr->cur.line_step = src_ptr->line_step;
	dst_ptr->cur.vheight = src_ptr->v_height;

	ISP_LOGV("bypass %d", header_ptr->bypass);

	header_ptr->is_update = ISP_ONE;

	return rtn;
}

cmr_s32 _pm_yiq_afl_set_param_v1(void *afl_param, cmr_u32 cmd, void *param_ptr0, void *param_ptr1)
{
	cmr_s32 rtn = ISP_SUCCESS;
	struct isp_yiq_afl_param_v1 *dst_ptr = (struct isp_yiq_afl_param_v1 *)afl_param;
	struct isp_pm_block_header *header_ptr = (struct isp_pm_block_header *)param_ptr1;
	struct isp_anti_flicker_cfg *cfl_cfg_ptr = (struct isp_anti_flicker_cfg *)param_ptr0;

	header_ptr->is_update = ISP_ONE;

	switch (cmd) {
	case ISP_PM_BLK_YIQ_AFL_BYPASS:
		dst_ptr->cur.bypass = *((cmr_u32 *) param_ptr0);
		break;

	case ISP_PM_BLK_YIQ_AFL_CFG:
		ISP_LOGV("skip-num %d", cfl_cfg_ptr->skip_frame_num);
		dst_ptr->cur.bypass = cfl_cfg_ptr->bypass;
		dst_ptr->cur.skip_frame_num = cfl_cfg_ptr->skip_frame_num;
		dst_ptr->cur.frame_num = cfl_cfg_ptr->frame_num;
		dst_ptr->cur.line_step = cfl_cfg_ptr->line_step;
		dst_ptr->cur.mode = cfl_cfg_ptr->mode;
		dst_ptr->cur.vheight = cfl_cfg_ptr->vheight;
		dst_ptr->cur.start_col = cfl_cfg_ptr->start_col;
		dst_ptr->cur.end_col = cfl_cfg_ptr->end_col;
		break;

	default:
		header_ptr->is_update = ISP_ZERO;
		break;
	}

	return rtn;
}

cmr_s32 _pm_yiq_afl_get_param_v1(void *afl_param, cmr_u32 cmd, void *rtn_param0, void *rtn_param1)
{
	cmr_s32 rtn = ISP_SUCCESS;
	struct isp_yiq_afl_param_v1 *afl_ptr = (struct isp_yiq_afl_param_v1 *)afl_param;
	struct isp_pm_param_data *param_data_ptr = (struct isp_pm_param_data *)rtn_param0;
	cmr_u32 *update_flag = (cmr_u32 *) rtn_param1;

	param_data_ptr->id = ISP_BLK_YIQ_AFL_V1;
	param_data_ptr->cmd = cmd;

	switch (cmd) {
	case ISP_PM_BLK_ISP_SETTING:
		param_data_ptr->data_ptr = &afl_ptr->cur;
		param_data_ptr->data_size = sizeof(afl_ptr->cur);
		*update_flag = 0;
		break;

	default:
		break;
	}

	return rtn;
}
