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
#define LOG_TAG "isp_blk_rgb_afm"
#include "isp_blocks_cfg.h"

cmr_u32 _pm_rgb_afm_convert_param(void *dst_param, cmr_u32 strength_level, cmr_u32 mode_flag, cmr_u32 scene_flag)
{
	cmr_s32 rtn = ISP_SUCCESS;
	cmr_s32 i = 0;
	cmr_u32 total_offset_units = 0;
	struct isp_rgb_afm_param *dst_ptr = (struct isp_rgb_afm_param *)dst_param;
	struct sensor_rgb_afm_level *rgb_afm_param = PNULL;

	if (SENSOR_MULTI_MODE_FLAG != dst_ptr->nr_mode_setting) {
		rgb_afm_param = (struct sensor_rgb_afm_level *)(dst_ptr->param_ptr);
	} else {
		cmr_u32 *multi_nr_map_ptr = PNULL;
		multi_nr_map_ptr = (cmr_u32 *) dst_ptr->scene_ptr;
		total_offset_units = _pm_calc_nr_addr_offset(mode_flag, scene_flag, multi_nr_map_ptr);
		rgb_afm_param = (struct sensor_rgb_afm_level *)((cmr_u8 *) dst_ptr->param_ptr + total_offset_units * dst_ptr->level_num * sizeof(struct sensor_rgb_afm_level));

	}
	strength_level = PM_CLIP(strength_level, 0, dst_ptr->level_num - 1);

	if (rgb_afm_param != NULL) {
		dst_ptr->cur.mode = rgb_afm_param[strength_level].afm_mode;
		dst_ptr->cur.skip_num = rgb_afm_param[strength_level].afm_skip_num;
		dst_ptr->cur.skip_num_clear = rgb_afm_param[strength_level].afm_skip_num_clear;
		dst_ptr->cur.overflow_protect_en = rgb_afm_param[strength_level].oflow_protect_en;
		for (i = 0; i < 10; i++) {
			dst_ptr->cur.coord[i].start_x = rgb_afm_param[strength_level].coord[i].start_x;
			dst_ptr->cur.coord[i].start_y = rgb_afm_param[strength_level].coord[i].start_y;
			dst_ptr->cur.coord[i].end_x = rgb_afm_param[strength_level].coord[i].end_x;
			dst_ptr->cur.coord[i].end_y = rgb_afm_param[strength_level].coord[i].end_y;
		}

		dst_ptr->cur.spsmd_rtgbot_enable = rgb_afm_param[strength_level].spsmd_rtgbot_enable;
		dst_ptr->cur.spsmd_diagonal_enable = rgb_afm_param[strength_level].spsmd_diagonal_enable;
		dst_ptr->cur.spsmd_square_en = rgb_afm_param[strength_level].spsmd_square_en;
		dst_ptr->cur.spsmd_touch_mode = rgb_afm_param[strength_level].spsmd_touch_mode;
		dst_ptr->cur.shift.shift_spsmd = rgb_afm_param[strength_level].spsmd_shift;
		dst_ptr->cur.subfilter.average = rgb_afm_param[strength_level].spsmd_subfilter_average;
		dst_ptr->cur.subfilter.median = rgb_afm_param[strength_level].spsmd_subfilter_median;
		dst_ptr->cur.thrd.spsmd_thr_min_red = rgb_afm_param[strength_level].spsmd_thrd.r.min;
		dst_ptr->cur.thrd.spsmd_thr_max_red = rgb_afm_param[strength_level].spsmd_thrd.r.max;
		dst_ptr->cur.thrd.spsmd_thr_min_green = rgb_afm_param[strength_level].spsmd_thrd.g.min;
		dst_ptr->cur.thrd.spsmd_thr_max_green = rgb_afm_param[strength_level].spsmd_thrd.g.max;
		dst_ptr->cur.thrd.spsmd_thr_min_blue = rgb_afm_param[strength_level].spsmd_thrd.b.min;
		dst_ptr->cur.thrd.spsmd_thr_max_blue = rgb_afm_param[strength_level].spsmd_thrd.b.max;

		dst_ptr->cur.shift.shift_sobel5 = rgb_afm_param[strength_level].sobel5_shift;
		dst_ptr->cur.thrd.sobel5_thr_min_red = rgb_afm_param[strength_level].sobel5_thrd.r.min;
		dst_ptr->cur.thrd.sobel5_thr_max_red = rgb_afm_param[strength_level].sobel5_thrd.r.max;
		dst_ptr->cur.thrd.sobel5_thr_min_green = rgb_afm_param[strength_level].sobel5_thrd.g.min;
		dst_ptr->cur.thrd.sobel5_thr_max_green = rgb_afm_param[strength_level].sobel5_thrd.g.max;
		dst_ptr->cur.thrd.sobel5_thr_min_blue = rgb_afm_param[strength_level].sobel5_thrd.b.min;
		dst_ptr->cur.thrd.sobel5_thr_max_blue = rgb_afm_param[strength_level].sobel5_thrd.b.max;

		dst_ptr->cur.shift.shift_sobel9 = rgb_afm_param[strength_level].sobel9_shift;
		dst_ptr->cur.thrd.sobel9_thr_min_red = rgb_afm_param[strength_level].sobel9_thrd.r.min;
		dst_ptr->cur.thrd.sobel9_thr_max_red = rgb_afm_param[strength_level].sobel9_thrd.r.max;
		dst_ptr->cur.thrd.sobel9_thr_min_green = rgb_afm_param[strength_level].sobel9_thrd.g.min;
		dst_ptr->cur.thrd.sobel9_thr_max_green = rgb_afm_param[strength_level].sobel9_thrd.g.max;
		dst_ptr->cur.thrd.sobel9_thr_min_blue = rgb_afm_param[strength_level].sobel9_thrd.b.min;
		dst_ptr->cur.thrd.sobel9_thr_max_blue = rgb_afm_param[strength_level].sobel9_thrd.b.max;
	}
	return rtn;
}

cmr_s32 _pm_rgb_afm_init(void *dst_rgb_afm, void *src_rgb_afm, void *param1, void *param2)
{
	cmr_s32 rtn = ISP_SUCCESS;
	struct isp_rgb_afm_param *dst_ptr = (struct isp_rgb_afm_param *)dst_rgb_afm;
	struct isp_pm_nr_header_param *src_ptr = (struct isp_pm_nr_header_param *)src_rgb_afm;
	struct isp_pm_block_header *header_ptr = (struct isp_pm_block_header *)param1;
	UNUSED(param2);

	dst_ptr->cur.bypass = 0;//header_ptr->bypass;

	dst_ptr->cur_level = src_ptr->default_strength_level;
	dst_ptr->level_num = src_ptr->level_number;
	dst_ptr->param_ptr = src_ptr->param_ptr;
	dst_ptr->scene_ptr = src_ptr->multi_nr_map_ptr;
	dst_ptr->nr_mode_setting = src_ptr->nr_mode_setting;

	rtn = _pm_rgb_afm_convert_param(dst_ptr, dst_ptr->cur_level, ISP_MODE_ID_COMMON, ISP_SCENEMODE_AUTO);
	//dst_ptr->cur.bypass |= header_ptr->bypass;

	if (ISP_SUCCESS != rtn) {
		ISP_LOGE("fail to  convert pm rgb afm param!");
		return rtn;
	}
	header_ptr->is_update = ISP_ONE;

	return rtn;
}

cmr_s32 _pm_rgb_afm_set_param(void *rgb_aem_param, cmr_u32 cmd, void *param_ptr0, void *param_ptr1)
{
	cmr_s32 rtn = ISP_SUCCESS;
	struct isp_rgb_afm_param *dst_ptr = (struct isp_rgb_afm_param *)rgb_aem_param;
	struct isp_pm_block_header *header_ptr = (struct isp_pm_block_header *)param_ptr1;
	UNUSED(cmd);

	switch (cmd) {
	case ISP_PM_BLK_RGB_AFM_BYPASS:
		dst_ptr->cur.bypass = *((cmr_u32 *) param_ptr0);
		header_ptr->is_update = ISP_ONE;
		break;
	case ISP_PM_BLK_SMART_SETTING:
		{
			struct smart_block_result *block_result = (struct smart_block_result *)param_ptr0;
			struct isp_range val_range = { 0, 0 };
			cmr_u32 cur_level = 0;

			val_range.min = 0;
			val_range.max = 255;

			rtn = _pm_check_smart_param(block_result, &val_range, 1, ISP_SMART_Y_TYPE_VALUE);
			if (ISP_SUCCESS != rtn) {
				ISP_LOGE("fail to check pm smart param !");
				return rtn;
			}

			cur_level = (cmr_u32) block_result->component[0].fix_data[0];

			if (0 && (cur_level != dst_ptr->cur_level || nr_tool_flag[9] || block_result->mode_flag_changed)) {
				dst_ptr->cur_level = cur_level;
				header_ptr->is_update = ISP_ONE;
				nr_tool_flag[9] = 0;
				block_result->mode_flag_changed = 0;
				rtn = _pm_rgb_afm_convert_param(dst_ptr, dst_ptr->cur_level, block_result->mode_flag, block_result->scene_flag);
				dst_ptr->cur.bypass |= header_ptr->bypass;
				if (ISP_SUCCESS != rtn) {
					ISP_LOGE("fail to  convert pm rgb afm param!");
					return rtn;
				}
			}
		}
		break;
	default:

		break;
	}
	ISP_LOGV("ISP_SMART: cmd=%d, update=%d, rgb_afm_level=%d", cmd, header_ptr->is_update, dst_ptr->cur_level);
	return rtn;
}

cmr_s32 _pm_rgb_afm_get_param(void *rgb_aem_param, cmr_u32 cmd, void *rtn_param0, void *rtn_param1)
{
	cmr_s32 rtn = ISP_SUCCESS;
	struct isp_rgb_afm_param *dst_ptr = (struct isp_rgb_afm_param *)rgb_aem_param;
	struct isp_pm_param_data *param_data_ptr = (struct isp_pm_param_data *)rtn_param0;
	cmr_u32 *update_flag = (cmr_u32 *) rtn_param1;

	param_data_ptr->id = ISP_BLK_RGB_AFM;
	param_data_ptr->cmd = cmd;

	switch (cmd) {
	case ISP_PM_BLK_ISP_SETTING:
		param_data_ptr->data_ptr = &dst_ptr->cur;
		param_data_ptr->data_size = sizeof(dst_ptr->cur);
		*update_flag = 0;
		break;

	case ISP_PM_BLK_RGB_AFM_BYPASS:
		param_data_ptr->data_ptr = (void *)&dst_ptr->cur.bypass;
		param_data_ptr->data_size = sizeof(dst_ptr->cur.bypass);
		break;

	default:
		break;
	}

	return rtn;
}
