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
#define LOG_TAG "isp_blk_yuv_nf"
#include "isp_blocks_cfg.h"

cmr_u32 _pm_yuv_noisefilter_convert_param(void *dst_yuv_noisefilter_param, cmr_u32 strength_level, cmr_u32 mode_flag, cmr_u32 scene_flag)
{
	cmr_s32 rtn = ISP_SUCCESS;
	cmr_s32 i = 0;
	cmr_u32 total_offset_units = 0;
	struct isp_dev_noise_filter_param *dst_ptr = (struct isp_dev_noise_filter_param *)dst_yuv_noisefilter_param;
	struct sensor_yuv_noisefilter_level *yuv_noisefilter_param = PNULL;

	if (SENSOR_MULTI_MODE_FLAG != dst_ptr->nr_mode_setting) {
		yuv_noisefilter_param = (struct sensor_yuv_noisefilter_level *)(dst_ptr->param_ptr);
	} else {
		cmr_u32 *multi_nr_map_ptr = PNULL;
		multi_nr_map_ptr = (cmr_u32 *) dst_ptr->scene_ptr;
		total_offset_units = _pm_calc_nr_addr_offset(mode_flag, scene_flag, multi_nr_map_ptr);
		yuv_noisefilter_param = (struct sensor_yuv_noisefilter_level *)((cmr_u8 *) dst_ptr->param_ptr +
										total_offset_units * dst_ptr->level_num * sizeof(struct sensor_yuv_noisefilter_level));
	}
	strength_level = PM_CLIP(strength_level, 0, dst_ptr->level_num - 1);

	if (yuv_noisefilter_param != NULL) {
		dst_ptr->cur.yrandom_bypass = yuv_noisefilter_param[strength_level].bypass;

		dst_ptr->cur.shape_mode = yuv_noisefilter_param[strength_level].noisefilter_shape_mode;

		for (i = 0; i < 4; i++) {
			dst_ptr->cur.yrandom_seed[i] = yuv_noisefilter_param[strength_level].yuv_noisefilter_gaussian.random_seed[i];
		}
		for (i = 0; i < 8; i++) {
			dst_ptr->cur.takebit[i] = yuv_noisefilter_param[strength_level].yuv_noisefilter_gaussian.random_takebit[i];
		}
		dst_ptr->cur.r_offset = yuv_noisefilter_param[strength_level].yuv_noisefilter_gaussian.random_r_offset;
		dst_ptr->cur.r_shift = yuv_noisefilter_param[strength_level].yuv_noisefilter_gaussian.random_r_shift;

		dst_ptr->cur.filter_thr = yuv_noisefilter_param[strength_level].yuv_noisefilter_adv.filter_thr;
		dst_ptr->cur.filter_thr_mode = yuv_noisefilter_param[strength_level].yuv_noisefilter_adv.filter_thr_mode;
		for (i = 0; i < 4; i++) {
			dst_ptr->cur.cv_t[i] = yuv_noisefilter_param[strength_level].yuv_noisefilter_adv.filter_cv_t[i];
		}
		for (i = 0; i < 3; i++) {
			dst_ptr->cur.cv_r[i] = yuv_noisefilter_param[strength_level].yuv_noisefilter_adv.filter_cv_r[i];
		}
		dst_ptr->cur.noise_clip.p = yuv_noisefilter_param[strength_level].yuv_noisefilter_adv.filter_clip_p;
		dst_ptr->cur.noise_clip.n = yuv_noisefilter_param[strength_level].yuv_noisefilter_adv.filter_clip_n;
	}
	return rtn;

}

cmr_s32 _pm_yuv_noisefilter_init(void *dst_yuv_noisefilter_param, void *src_yuv_noisefilter_param, void *param1, void *param_ptr2)
{
	cmr_s32 rtn = ISP_SUCCESS;
	struct isp_pm_nr_header_param *src_ptr = (struct isp_pm_nr_header_param *)src_yuv_noisefilter_param;
	struct isp_dev_noise_filter_param *dst_ptr = (struct isp_dev_noise_filter_param *)dst_yuv_noisefilter_param;
	struct isp_pm_block_header *header_ptr = (struct isp_pm_block_header *)param1;
	UNUSED(param_ptr2);

	dst_ptr->cur.yrandom_bypass = header_ptr->bypass;

	dst_ptr->param_ptr = src_ptr->param_ptr;
	dst_ptr->cur_level = src_ptr->default_strength_level;
	dst_ptr->level_num = src_ptr->level_number;
	dst_ptr->scene_ptr = src_ptr->multi_nr_map_ptr;
	dst_ptr->nr_mode_setting = src_ptr->nr_mode_setting;

	rtn = _pm_yuv_noisefilter_convert_param(dst_ptr, dst_ptr->cur_level, ISP_MODE_ID_COMMON, ISP_SCENEMODE_AUTO);
	dst_ptr->cur.yrandom_bypass |= header_ptr->bypass;
	if (ISP_SUCCESS != rtn) {
		ISP_LOGE("fail to convert pm yuv noisefilter param!");
		return rtn;
	}

	header_ptr->is_update = ISP_ONE;

	return rtn;
}

cmr_s32 _pm_yuv_noisefilter_set_param(void *yuv_noisefilter_param, cmr_u32 cmd, void *param_ptr0, void *param_ptr1)
{
	cmr_s32 rtn = ISP_SUCCESS;
	struct isp_pm_block_header *header_ptr = (struct isp_pm_block_header *)param_ptr1;
	struct isp_dev_noise_filter_param *dst_ptr = (struct isp_dev_noise_filter_param *)yuv_noisefilter_param;

	switch (cmd) {
	case ISP_PM_BLK_YUV_NOISEFILTER_BYPASS:
		dst_ptr->cur.yrandom_bypass = *((cmr_u32 *) param_ptr0);
		header_ptr->is_update = ISP_ONE;
		break;

	case ISP_PM_BLK_YUV_NOISEFILTER_STRENGTH_LEVEL:
		dst_ptr->cur_level = *((cmr_u32 *) param_ptr0);
		header_ptr->is_update = ISP_ONE;
		break;

	case ISP_PM_BLK_SMART_SETTING:
		{
			struct smart_block_result *block_result = (struct smart_block_result *)param_ptr0;
			struct isp_range val_range = { 0, 0 };
			cmr_u32 level = 0;

			val_range.min = 0;
			val_range.max = 255;

			rtn = _pm_check_smart_param(block_result, &val_range, 1, ISP_SMART_Y_TYPE_VALUE);
			if (ISP_SUCCESS != rtn) {
				ISP_LOGE("fail to check pm smart param !");
				return rtn;
			}

			level = (cmr_u32) block_result->component[0].fix_data[0];

			if (level != dst_ptr->cur_level || nr_tool_flag[15] || block_result->mode_flag_changed) {
				dst_ptr->cur_level = level;
				header_ptr->is_update = ISP_ONE;
				nr_tool_flag[15] = 0;
				block_result->mode_flag_changed = 0;

				rtn = _pm_yuv_noisefilter_convert_param(dst_ptr, dst_ptr->cur_level, block_result->mode_flag, block_result->scene_flag);
				dst_ptr->cur.yrandom_bypass |= header_ptr->bypass;
				if (ISP_SUCCESS != rtn) {
					ISP_LOGE("fail to convert pm yuv noisefilter param!");
					return rtn;
				}
			}
		}
		break;

	default:

		break;
	}
	return rtn;
}

cmr_s32 _pm_yuv_noisefilter_get_param(void *yuv_noisefilter_param, cmr_u32 cmd, void *rtn_param0, void *rtn_param1)
{
	cmr_s32 rtn = ISP_SUCCESS;
	struct isp_dev_noise_filter_param *yuv_noisefilter_ptr = (struct isp_dev_noise_filter_param *)yuv_noisefilter_param;
	struct isp_pm_param_data *param_data_ptr = (struct isp_pm_param_data *)rtn_param0;
	cmr_u32 *update_flag = (cmr_u32 *) rtn_param1;

	param_data_ptr->id = ISP_BLK_YUV_NOISEFILTER;
	param_data_ptr->cmd = cmd;

	switch (cmd) {
	case ISP_PM_BLK_ISP_SETTING:
		param_data_ptr->data_ptr = &yuv_noisefilter_ptr->cur;
		param_data_ptr->data_size = sizeof(yuv_noisefilter_ptr->cur);
		*update_flag = ISP_ZERO;
		break;

	default:
		break;
	}

	return rtn;
}
