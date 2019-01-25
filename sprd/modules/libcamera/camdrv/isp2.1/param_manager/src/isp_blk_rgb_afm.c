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
	cmr_s32 i = 0, j = 0;
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
		dst_ptr->cur.skip_num = rgb_afm_param[strength_level].afm_skip_num;
		dst_ptr->cur.touch_mode = rgb_afm_param[strength_level].touch_mode;
		dst_ptr->cur.overflow_protect_en = rgb_afm_param[strength_level].oflow_protect_en;
		dst_ptr->cur.data_update_sel = rgb_afm_param[strength_level].data_update_sel;
		dst_ptr->cur.mode = rgb_afm_param[strength_level].afm_mode;
		for (i = 0; i < 10; i++) {
			dst_ptr->cur.win[i].start_x = rgb_afm_param[strength_level].win_range[i].start_x;
			dst_ptr->cur.win[i].start_y = rgb_afm_param[strength_level].win_range[i].start_y;
			dst_ptr->cur.win[i].end_x = rgb_afm_param[strength_level].win_range[i].end_x;
			dst_ptr->cur.win[i].end_y = rgb_afm_param[strength_level].win_range[i].end_y;
			dst_ptr->cur.iir_c[i] = rgb_afm_param[strength_level].afm_iir_denoise.iir_c[i];
		}
		dst_ptr->cur.iir_eb = rgb_afm_param[strength_level].afm_iir_denoise.afm_iir_en;
		dst_ptr->cur.iir_g0 = rgb_afm_param[strength_level].afm_iir_denoise.iir_g[0];
		dst_ptr->cur.iir_g1 = rgb_afm_param[strength_level].afm_iir_denoise.iir_g[1];

		dst_ptr->cur.channel_sel = rgb_afm_param[strength_level].afm_enhanced.afm_enhanced_pre.channel_sel;
		dst_ptr->cur.denoise_mode = rgb_afm_param[strength_level].afm_enhanced.afm_enhanced_pre.denoise_mode;
		dst_ptr->cur.center_weight = rgb_afm_param[strength_level].afm_enhanced.afm_enhanced_pre.center_wgt;
		for (i = 0; i < 4; i++) {
			for (j = 0; j < 9; j++) {
				dst_ptr->cur.fv1_coeff[i][j] = rgb_afm_param[strength_level].afm_enhanced.afm_enhanced_process.fv1_coeff[i][j];
			}
		}
		dst_ptr->cur.fv0_enhance_mode = rgb_afm_param[strength_level].afm_enhanced.afm_enhanced_post.enhanced_fv0.fv_enhanced_mode;
		dst_ptr->cur.clip_en0 = rgb_afm_param[strength_level].afm_enhanced.afm_enhanced_post.enhanced_fv0.clip_en;
		dst_ptr->cur.fv0_shift = rgb_afm_param[strength_level].afm_enhanced.afm_enhanced_post.enhanced_fv0.fv_shift;
		dst_ptr->cur.fv0_th.min = rgb_afm_param[strength_level].afm_enhanced.afm_enhanced_post.enhanced_fv0.fv_min_th;
		dst_ptr->cur.fv0_th.max = rgb_afm_param[strength_level].afm_enhanced.afm_enhanced_post.enhanced_fv0.fv_max_th;

		dst_ptr->cur.fv1_enhance_mode = rgb_afm_param[strength_level].afm_enhanced.afm_enhanced_post.enhanced_fv1.fv_enhanced_mode;
		dst_ptr->cur.clip_en1 = rgb_afm_param[strength_level].afm_enhanced.afm_enhanced_post.enhanced_fv1.clip_en;
		dst_ptr->cur.fv1_shift = rgb_afm_param[strength_level].afm_enhanced.afm_enhanced_post.enhanced_fv1.fv_shift;
		dst_ptr->cur.fv1_th.min = rgb_afm_param[strength_level].afm_enhanced.afm_enhanced_post.enhanced_fv1.fv_min_th;
		dst_ptr->cur.fv1_th.max = rgb_afm_param[strength_level].afm_enhanced.afm_enhanced_post.enhanced_fv1.fv_max_th;
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

	dst_ptr->cur.bypass = header_ptr->bypass;

	dst_ptr->cur_level = src_ptr->default_strength_level;
	dst_ptr->level_num = src_ptr->level_number;
	dst_ptr->param_ptr = src_ptr->param_ptr;
	dst_ptr->scene_ptr = src_ptr->multi_nr_map_ptr;
	dst_ptr->nr_mode_setting = src_ptr->nr_mode_setting;

	rtn = _pm_rgb_afm_convert_param(dst_ptr, dst_ptr->cur_level, ISP_MODE_ID_COMMON, ISP_SCENEMODE_AUTO);
	dst_ptr->cur.bypass |= header_ptr->bypass;

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

			if (cur_level != dst_ptr->cur_level || nr_tool_flag[9] || block_result->mode_flag_changed) {
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
