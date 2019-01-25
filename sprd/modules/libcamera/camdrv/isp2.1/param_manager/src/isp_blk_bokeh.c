/*
 *opyright (C) 2012 The Android Open Source Project
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
#define LOG_TAG "isp_blk_bokeh"
#include "isp_blocks_cfg.h"

cmr_s32 _pm_bokeh_init(void *dst_bokeh_param, void *src_bokeh_param, void *param1, void *param2)
{
	cmr_s32 rtn = ISP_SUCCESS;
	cmr_s32 i = 0;
	struct isp_bokeh_micro_depth_param *dst_bokeh_ptr = (struct isp_bokeh_micro_depth_param*)dst_bokeh_param;
	struct bokeh_micro_depth_tune_param *src_bokeh_ptr = (struct bokeh_micro_depth_tune_param *)src_bokeh_param;
	struct isp_pm_block_header *bokeh_header_ptr = (struct isp_pm_block_header *)param1;
	UNUSED(param2);

	dst_bokeh_ptr->cur.tuning_exist = src_bokeh_ptr->tuning_exist;
	dst_bokeh_ptr->cur.fir_mode = src_bokeh_ptr->fir_mode;
	dst_bokeh_ptr->cur.fir_len = src_bokeh_ptr->fir_len;
	dst_bokeh_ptr->cur.fir_channel = src_bokeh_ptr->fir_channel;
	dst_bokeh_ptr->cur.fir_cal_mode = src_bokeh_ptr->fir_cal_mode;
	dst_bokeh_ptr->cur.fir_edge_factor = src_bokeh_ptr->fir_edge_factor;
	dst_bokeh_ptr->cur.depth_mode = src_bokeh_ptr->depth_mode;
	dst_bokeh_ptr->cur.smooth_thr = src_bokeh_ptr->smooth_thr;
	dst_bokeh_ptr->cur.touch_factor = src_bokeh_ptr->touch_factor;
	dst_bokeh_ptr->cur.scale_factor = src_bokeh_ptr->scale_factor;
	dst_bokeh_ptr->cur.refer_len = src_bokeh_ptr->refer_len;
	dst_bokeh_ptr->cur.merge_factor = src_bokeh_ptr->merge_factor;
	dst_bokeh_ptr->cur.similar_factor = src_bokeh_ptr->similar_factor;
	dst_bokeh_ptr->cur.tmp_mode = src_bokeh_ptr->tmp_mode;
	dst_bokeh_ptr->cur.tmp_thr = src_bokeh_ptr->tmp_thr;
	dst_bokeh_ptr->cur.enable = src_bokeh_ptr->enable;

	for (i = 0; i < 7; i++) {
		dst_bokeh_ptr->cur.hfir_coeff[i] = src_bokeh_ptr->hfir_coeff[i];
		dst_bokeh_ptr->cur.vfir_coeff[i] = src_bokeh_ptr->vfir_coeff[i];
	}

	for (i = 0; i < 3; i++)
		dst_bokeh_ptr->cur.similar_coeff[i] = src_bokeh_ptr->similar_coeff[i];

	for (i = 0; i < 8; i++)
		dst_bokeh_ptr->cur.tmp_coeff[i] = src_bokeh_ptr->tmp_coeff[i];

	bokeh_header_ptr->is_update = ISP_ONE;

	return rtn;
}

cmr_s32 _pm_bokeh_set_param(void *bokeh_param, cmr_u32 cmd, void *param_ptr0, void *param_ptr1)
{
	cmr_s32 rtn = ISP_SUCCESS;
	struct bokeh_micro_depth_tune_param *src_bokeh_ptr = PNULL;
	struct isp_bokeh_micro_depth_param *dst_bokeh_ptr = (struct isp_bokeh_micro_depth_param *)bokeh_param;
	struct isp_pm_block_header *bokeh_header_ptr = (struct isp_pm_block_header *)param_ptr1;
	UNUSED(param_ptr0);
	UNUSED(src_bokeh_ptr);
	bokeh_header_ptr->is_update = ISP_ONE;
	switch (cmd) {
	case ISP_PM_BLK_BOKEH_MICRO_DEPTH_BYPASS:
		dst_bokeh_ptr->cur.enable = *((cmr_u32 *) param_ptr0);
		bokeh_header_ptr->is_update = ISP_ONE;
		break;
	case ISP_PM_BLK_BOKEH_MICRO_DEPTH:
		break;
	default:
		bokeh_header_ptr->is_update = ISP_ZERO;
		break;
	}
	return rtn;
}

cmr_s32 _pm_bokeh_get_param(void *bokeh_param, cmr_u32 cmd, void *rtn_param0, void *rtn_param1)
{
	cmr_s32 rtn = ISP_SUCCESS;
	struct isp_pm_param_data *param_data_ptr = (struct isp_pm_param_data *)rtn_param0;
	struct isp_dual_flash_param *bokeh_ptr = (struct isp_dual_flash_param *)bokeh_param;
	cmr_u32 *update_flag = (cmr_u32 *) rtn_param1;

	param_data_ptr->id = ISP_BLK_BOKEH_MICRO_DEPTH;
	param_data_ptr->cmd = cmd;

	switch (cmd) {
	case ISP_PM_BLK_ISP_SETTING:
		param_data_ptr->data_ptr = &bokeh_ptr->cur;
		param_data_ptr->data_size = sizeof(bokeh_ptr->cur);
		*update_flag = 0;
		break;

	default:
		break;
	}

	return rtn;
}

