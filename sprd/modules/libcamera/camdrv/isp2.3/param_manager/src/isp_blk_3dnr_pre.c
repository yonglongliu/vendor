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
#define LOG_TAG "isp_blk_3dnr_pre"
#include "isp_blocks_cfg.h"

cmr_u32 _pm_3d_nr_pre_convert_param(void *dst_3d_nr_param, cmr_u32 strength_level, cmr_u32 mode_flag, cmr_u32 scene_flag)
{
	cmr_s32 rtn = ISP_SUCCESS;
	cmr_s32 i = 0;
	cmr_u32 total_offset_units = 0;
	struct isp_3d_nr_pre_param *dst_ptr = (struct isp_3d_nr_pre_param *)dst_3d_nr_param;
	struct sensor_3dnr_level *nr_3d_param = PNULL;

	if (SENSOR_MULTI_MODE_FLAG != dst_ptr->nr_mode_setting) {
		nr_3d_param = (struct sensor_3dnr_level *)(dst_ptr->param_ptr);
	} else {
		cmr_u32 *multi_nr_map_ptr = PNULL;
		multi_nr_map_ptr = (cmr_u32 *) dst_ptr->scene_ptr;
		total_offset_units = _pm_calc_nr_addr_offset(mode_flag, scene_flag, multi_nr_map_ptr);
		nr_3d_param = (struct sensor_3dnr_level *)((cmr_u8 *) dst_ptr->param_ptr + total_offset_units * dst_ptr->level_num * sizeof(struct sensor_3dnr_level));
	}
	strength_level = PM_CLIP(strength_level, 0, dst_ptr->level_num - 1);
	if (nr_3d_param != NULL) {
		//dst_ptr->cur.blend_bypass = nr_3d_param[strength_level].bypass;
#if 0
		dst_ptr->cur.fusion_mode = nr_3d_param[strength_level].fusion_mode;
		dst_ptr->cur.filter_switch = nr_3d_param[strength_level].filter_swt_en;
		for (i = 0; i < 4; i++) {
			dst_ptr->cur.y_pixel_src_weight[i] = nr_3d_param[strength_level].yuv_cfg.y_cfg.src_wgt[i];
			dst_ptr->cur.u_pixel_src_weight[i] = nr_3d_param[strength_level].yuv_cfg.u_cfg.src_wgt[i];
			dst_ptr->cur.v_pixel_src_weight[i] = nr_3d_param[strength_level].yuv_cfg.v_cfg.src_wgt[i];
		}

		dst_ptr->cur.y_pixel_noise_threshold = nr_3d_param[strength_level].yuv_cfg.y_cfg.nr_thr;
		dst_ptr->cur.u_pixel_noise_threshold = nr_3d_param[strength_level].yuv_cfg.u_cfg.nr_thr;
		dst_ptr->cur.v_pixel_noise_threshold = nr_3d_param[strength_level].yuv_cfg.v_cfg.nr_thr;
		dst_ptr->cur.y_pixel_noise_weight = nr_3d_param[strength_level].yuv_cfg.y_cfg.nr_wgt;
		dst_ptr->cur.u_pixel_noise_weight = nr_3d_param[strength_level].yuv_cfg.u_cfg.nr_wgt;
		dst_ptr->cur.v_pixel_noise_weight = nr_3d_param[strength_level].yuv_cfg.v_cfg.nr_wgt;
		dst_ptr->cur.threshold_radial_variation_u_range_min = nr_3d_param[strength_level].sensor_3dnr_cor.u_range.min;
		dst_ptr->cur.threshold_radial_variation_u_range_max = nr_3d_param[strength_level].sensor_3dnr_cor.u_range.max;
		dst_ptr->cur.threshold_radial_variation_v_range_min = nr_3d_param[strength_level].sensor_3dnr_cor.v_range.min;
		dst_ptr->cur.threshold_radial_variation_v_range_max = nr_3d_param[strength_level].sensor_3dnr_cor.v_range.max;

		dst_ptr->cur.y_threshold_polyline_0 = nr_3d_param[strength_level].yuv_cfg.y_cfg.thr_polyline[0];
		dst_ptr->cur.y_threshold_polyline_1 = nr_3d_param[strength_level].yuv_cfg.y_cfg.thr_polyline[1];
		dst_ptr->cur.y_threshold_polyline_2 = nr_3d_param[strength_level].yuv_cfg.y_cfg.thr_polyline[2];
		dst_ptr->cur.y_threshold_polyline_3 = nr_3d_param[strength_level].yuv_cfg.y_cfg.thr_polyline[3];
		dst_ptr->cur.y_threshold_polyline_4 = nr_3d_param[strength_level].yuv_cfg.y_cfg.thr_polyline[4];
		dst_ptr->cur.y_threshold_polyline_5 = nr_3d_param[strength_level].yuv_cfg.y_cfg.thr_polyline[5];
		dst_ptr->cur.y_threshold_polyline_6 = nr_3d_param[strength_level].yuv_cfg.y_cfg.thr_polyline[6];
		dst_ptr->cur.y_threshold_polyline_7 = nr_3d_param[strength_level].yuv_cfg.y_cfg.thr_polyline[7];
		dst_ptr->cur.y_threshold_polyline_8 = nr_3d_param[strength_level].yuv_cfg.y_cfg.thr_polyline[8];

		dst_ptr->cur.u_threshold_polyline_0 = nr_3d_param[strength_level].yuv_cfg.u_cfg.thr_polyline[0];
		dst_ptr->cur.u_threshold_polyline_1 = nr_3d_param[strength_level].yuv_cfg.u_cfg.thr_polyline[1];
		dst_ptr->cur.u_threshold_polyline_2 = nr_3d_param[strength_level].yuv_cfg.u_cfg.thr_polyline[2];
		dst_ptr->cur.u_threshold_polyline_3 = nr_3d_param[strength_level].yuv_cfg.u_cfg.thr_polyline[3];
		dst_ptr->cur.u_threshold_polyline_4 = nr_3d_param[strength_level].yuv_cfg.u_cfg.thr_polyline[4];
		dst_ptr->cur.u_threshold_polyline_5 = nr_3d_param[strength_level].yuv_cfg.u_cfg.thr_polyline[5];
		dst_ptr->cur.u_threshold_polyline_6 = nr_3d_param[strength_level].yuv_cfg.u_cfg.thr_polyline[6];
		dst_ptr->cur.u_threshold_polyline_7 = nr_3d_param[strength_level].yuv_cfg.u_cfg.thr_polyline[7];
		dst_ptr->cur.u_threshold_polyline_8 = nr_3d_param[strength_level].yuv_cfg.u_cfg.thr_polyline[8];

		dst_ptr->cur.v_threshold_polyline_0 = nr_3d_param[strength_level].yuv_cfg.v_cfg.thr_polyline[0];
		dst_ptr->cur.v_threshold_polyline_1 = nr_3d_param[strength_level].yuv_cfg.v_cfg.thr_polyline[1];
		dst_ptr->cur.v_threshold_polyline_2 = nr_3d_param[strength_level].yuv_cfg.v_cfg.thr_polyline[2];
		dst_ptr->cur.v_threshold_polyline_3 = nr_3d_param[strength_level].yuv_cfg.v_cfg.thr_polyline[3];
		dst_ptr->cur.v_threshold_polyline_4 = nr_3d_param[strength_level].yuv_cfg.v_cfg.thr_polyline[4];
		dst_ptr->cur.v_threshold_polyline_5 = nr_3d_param[strength_level].yuv_cfg.v_cfg.thr_polyline[5];
		dst_ptr->cur.v_threshold_polyline_6 = nr_3d_param[strength_level].yuv_cfg.v_cfg.thr_polyline[6];
		dst_ptr->cur.v_threshold_polyline_7 = nr_3d_param[strength_level].yuv_cfg.v_cfg.thr_polyline[7];
		dst_ptr->cur.v_threshold_polyline_8 = nr_3d_param[strength_level].yuv_cfg.v_cfg.thr_polyline[8];

		dst_ptr->cur.y_intensity_gain_polyline_0 = nr_3d_param[strength_level].yuv_cfg.y_cfg.gain_polyline[0];
		dst_ptr->cur.y_intensity_gain_polyline_1 = nr_3d_param[strength_level].yuv_cfg.y_cfg.gain_polyline[1];
		dst_ptr->cur.y_intensity_gain_polyline_2 = nr_3d_param[strength_level].yuv_cfg.y_cfg.gain_polyline[2];
		dst_ptr->cur.y_intensity_gain_polyline_3 = nr_3d_param[strength_level].yuv_cfg.y_cfg.gain_polyline[3];
		dst_ptr->cur.y_intensity_gain_polyline_4 = nr_3d_param[strength_level].yuv_cfg.y_cfg.gain_polyline[4];
		dst_ptr->cur.y_intensity_gain_polyline_5 = nr_3d_param[strength_level].yuv_cfg.y_cfg.gain_polyline[5];
		dst_ptr->cur.y_intensity_gain_polyline_6 = nr_3d_param[strength_level].yuv_cfg.y_cfg.gain_polyline[6];
		dst_ptr->cur.y_intensity_gain_polyline_7 = nr_3d_param[strength_level].yuv_cfg.y_cfg.gain_polyline[7];
		dst_ptr->cur.y_intensity_gain_polyline_8 = nr_3d_param[strength_level].yuv_cfg.y_cfg.gain_polyline[8];

		dst_ptr->cur.u_intensity_gain_polyline_0 = nr_3d_param[strength_level].yuv_cfg.u_cfg.gain_polyline[0];
		dst_ptr->cur.u_intensity_gain_polyline_1 = nr_3d_param[strength_level].yuv_cfg.u_cfg.gain_polyline[1];
		dst_ptr->cur.u_intensity_gain_polyline_2 = nr_3d_param[strength_level].yuv_cfg.u_cfg.gain_polyline[2];
		dst_ptr->cur.u_intensity_gain_polyline_3 = nr_3d_param[strength_level].yuv_cfg.u_cfg.gain_polyline[3];
		dst_ptr->cur.u_intensity_gain_polyline_4 = nr_3d_param[strength_level].yuv_cfg.u_cfg.gain_polyline[4];
		dst_ptr->cur.u_intensity_gain_polyline_5 = nr_3d_param[strength_level].yuv_cfg.u_cfg.gain_polyline[5];
		dst_ptr->cur.u_intensity_gain_polyline_6 = nr_3d_param[strength_level].yuv_cfg.u_cfg.gain_polyline[6];
		dst_ptr->cur.u_intensity_gain_polyline_7 = nr_3d_param[strength_level].yuv_cfg.u_cfg.gain_polyline[7];
		dst_ptr->cur.u_intensity_gain_polyline_8 = nr_3d_param[strength_level].yuv_cfg.u_cfg.gain_polyline[8];

		dst_ptr->cur.v_intensity_gain_polyline_0 = nr_3d_param[strength_level].yuv_cfg.v_cfg.gain_polyline[0];
		dst_ptr->cur.v_intensity_gain_polyline_1 = nr_3d_param[strength_level].yuv_cfg.v_cfg.gain_polyline[1];
		dst_ptr->cur.v_intensity_gain_polyline_2 = nr_3d_param[strength_level].yuv_cfg.v_cfg.gain_polyline[2];
		dst_ptr->cur.v_intensity_gain_polyline_3 = nr_3d_param[strength_level].yuv_cfg.v_cfg.gain_polyline[3];
		dst_ptr->cur.v_intensity_gain_polyline_4 = nr_3d_param[strength_level].yuv_cfg.v_cfg.gain_polyline[4];
		dst_ptr->cur.v_intensity_gain_polyline_5 = nr_3d_param[strength_level].yuv_cfg.v_cfg.gain_polyline[5];
		dst_ptr->cur.v_intensity_gain_polyline_6 = nr_3d_param[strength_level].yuv_cfg.v_cfg.gain_polyline[6];
		dst_ptr->cur.v_intensity_gain_polyline_7 = nr_3d_param[strength_level].yuv_cfg.v_cfg.gain_polyline[7];
		dst_ptr->cur.v_intensity_gain_polyline_8 = nr_3d_param[strength_level].yuv_cfg.v_cfg.gain_polyline[8];

		dst_ptr->cur.gradient_weight_polyline_0 = nr_3d_param[strength_level].yuv_cfg.grad_wgt_polyline[0];
		dst_ptr->cur.gradient_weight_polyline_1 = nr_3d_param[strength_level].yuv_cfg.grad_wgt_polyline[1];
		dst_ptr->cur.gradient_weight_polyline_2 = nr_3d_param[strength_level].yuv_cfg.grad_wgt_polyline[2];
		dst_ptr->cur.gradient_weight_polyline_3 = nr_3d_param[strength_level].yuv_cfg.grad_wgt_polyline[3];
		dst_ptr->cur.gradient_weight_polyline_4 = nr_3d_param[strength_level].yuv_cfg.grad_wgt_polyline[4];
		dst_ptr->cur.gradient_weight_polyline_5 = nr_3d_param[strength_level].yuv_cfg.grad_wgt_polyline[5];
		dst_ptr->cur.gradient_weight_polyline_6 = nr_3d_param[strength_level].yuv_cfg.grad_wgt_polyline[6];
		dst_ptr->cur.gradient_weight_polyline_7 = nr_3d_param[strength_level].yuv_cfg.grad_wgt_polyline[7];
		dst_ptr->cur.gradient_weight_polyline_8 = nr_3d_param[strength_level].yuv_cfg.grad_wgt_polyline[8];
		dst_ptr->cur.gradient_weight_polyline_9 = nr_3d_param[strength_level].yuv_cfg.grad_wgt_polyline[9];
		dst_ptr->cur.gradient_weight_polyline_10 = nr_3d_param[strength_level].yuv_cfg.grad_wgt_polyline[10];

		for (i = 0; i < 4; i++) {
			dst_ptr->cur.u_threshold_factor[i] = nr_3d_param[strength_level].sensor_3dnr_cor.uv_factor.u_thr[i];
			dst_ptr->cur.v_threshold_factor[i] = nr_3d_param[strength_level].sensor_3dnr_cor.uv_factor.v_thr[i];
			dst_ptr->cur.u_divisor_factor[i] = nr_3d_param[strength_level].sensor_3dnr_cor.uv_factor.u_div[i];
			dst_ptr->cur.v_divisor_factor[i] = nr_3d_param[strength_level].sensor_3dnr_cor.uv_factor.v_div[i];
		}

		dst_ptr->cur.r1_circle = nr_3d_param[strength_level].sensor_3dnr_cor.r_circle[0];
		dst_ptr->cur.r2_circle = nr_3d_param[strength_level].sensor_3dnr_cor.r_circle[1];
		dst_ptr->cur.r3_circle = nr_3d_param[strength_level].sensor_3dnr_cor.r_circle[2];
#endif
	}
	return rtn;
}

cmr_s32 _pm_3d_nr_pre_init(void *dst_3d_nr_param, void *src_3d_nr_param, void *param1, void *param_ptr2)
{
	cmr_s32 rtn = ISP_SUCCESS;

	struct isp_pm_nr_header_param *src_ptr = (struct isp_pm_nr_header_param *)src_3d_nr_param;
	struct isp_3d_nr_pre_param *dst_ptr = (struct isp_3d_nr_pre_param *)dst_3d_nr_param;
	struct isp_pm_block_header *nr_3d_header_ptr = (struct isp_pm_block_header *)param1;
	UNUSED(param_ptr2);

	//dst_ptr->cur.blend_bypass = nr_3d_header_ptr->bypass;

	dst_ptr->cur_level = src_ptr->default_strength_level;
	dst_ptr->level_num = src_ptr->level_number;
	dst_ptr->param_ptr = src_ptr->param_ptr;
	dst_ptr->scene_ptr = src_ptr->multi_nr_map_ptr;
	dst_ptr->nr_mode_setting = src_ptr->nr_mode_setting;

	rtn = _pm_3d_nr_pre_convert_param(dst_ptr, dst_ptr->cur_level, ISP_MODE_ID_COMMON, ISP_SCENEMODE_AUTO);
	//dst_ptr->cur.blend_bypass |= nr_3d_header_ptr->bypass;
	if (ISP_SUCCESS != rtn) {
		ISP_LOGE("fail to convert pm 3d nr pre param !");
		return rtn;
	}

	nr_3d_header_ptr->is_update = ISP_ONE;

	return rtn;
}

cmr_s32 _pm_3d_nr_pre_set_param(void *nr_3d_param, cmr_u32 cmd, void *param_ptr0, void *param_ptr1)
{
	cmr_s32 rtn = ISP_SUCCESS;
	struct isp_pm_block_header *header_ptr = (struct isp_pm_block_header *)param_ptr1;
	struct isp_3d_nr_pre_param *dst_ptr = (struct isp_3d_nr_pre_param *)nr_3d_param;

	switch (cmd) {
	case ISP_PM_BLK_3D_NR_BYPASS:
		//dst_ptr->cur.blend_bypass = *((cmr_u32 *) param_ptr0);
		header_ptr->is_update = ISP_ONE;
		break;

	case ISP_PM_BLK_3D_NR_STRENGTH_LEVEL:
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

			if (level != dst_ptr->cur_level || nr_tool_flag[1] || block_result->mode_flag_changed) {
				dst_ptr->cur_level = level;
				header_ptr->is_update = ISP_ONE;
				nr_tool_flag[1] = 0;
				block_result->mode_flag_changed = 0;

				rtn = _pm_3d_nr_pre_convert_param(dst_ptr, dst_ptr->cur_level, block_result->mode_flag, block_result->scene_flag);
				//dst_ptr->cur.blend_bypass |= header_ptr->bypass;
				if (ISP_SUCCESS != rtn) {
					ISP_LOGE("fail to convert pm 3d nr pre param !");
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

cmr_s32 _pm_3d_nr_pre_get_param(void *nr_3d_param, cmr_u32 cmd, void *rtn_param0, void *rtn_param1)
{
	cmr_s32 rtn = ISP_SUCCESS;
	struct isp_3d_nr_pre_param *nr_3d_ptr = (struct isp_3d_nr_pre_param *)nr_3d_param;
	struct isp_pm_param_data *param_data_ptr = (struct isp_pm_param_data *)rtn_param0;
	cmr_u32 *update_flag = (cmr_u32 *) rtn_param1;

	param_data_ptr->id = ISP_BLK_3DNR_PRE;
	param_data_ptr->cmd = cmd;

	switch (cmd) {
	case ISP_PM_BLK_ISP_SETTING:
		param_data_ptr->data_ptr = &nr_3d_ptr->cur;
		param_data_ptr->data_size = sizeof(nr_3d_ptr->cur);
		*update_flag = ISP_ZERO;
		break;

	default:
		break;
	}

	return rtn;
}
