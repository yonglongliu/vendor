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
#define LOG_TAG "isp_blk_iircnr"
#include "isp_blocks_cfg.h"

cmr_u32 _pm_iircnr_iir_convert_param(void *dst_param, cmr_u32 strength_level, cmr_u32 mode_flag, cmr_u32 scene_flag)
{
	cmr_s32 rtn = ISP_SUCCESS;
	cmr_s32 i = 0;
	cmr_u32 total_offset_units = 0;
	struct isp_iircnr_iir_param *dst_ptr = (struct isp_iircnr_iir_param *)dst_param;
	struct sensor_iircnr_level *iir_cnr_param = PNULL;

	if (SENSOR_MULTI_MODE_FLAG != dst_ptr->nr_mode_setting) {
		iir_cnr_param = (struct sensor_iircnr_level *)(dst_ptr->param_ptr);
	} else {
		cmr_u32 *multi_nr_map_ptr = PNULL;
		multi_nr_map_ptr = (cmr_u32 *) dst_ptr->scene_ptr;
		total_offset_units = _pm_calc_nr_addr_offset(mode_flag, scene_flag, multi_nr_map_ptr);
		iir_cnr_param = (struct sensor_iircnr_level *)((cmr_u8 *) dst_ptr->param_ptr + total_offset_units * dst_ptr->level_num * sizeof(struct sensor_iircnr_level));
	}
	strength_level = PM_CLIP(strength_level, 0, dst_ptr->level_num - 1);
	if (iir_cnr_param != NULL) {
		dst_ptr->cur.bypass = iir_cnr_param[strength_level].bypass;

		dst_ptr->cur.pre_uv_th = iir_cnr_param[strength_level].pre_uv_th.iircnr_pre_uv_th;
		dst_ptr->cur.sat_ratio = iir_cnr_param[strength_level].pre_uv_th.iircnr_sat_ratio;
		dst_ptr->cur.uv_th = iir_cnr_param[strength_level].iircnr_ee.iircnr_uv_th;
		dst_ptr->cur.uv_dist = iir_cnr_param[strength_level].iircnr_ee.iircnr_uv_dist;
		dst_ptr->cur.uv_s_th = iir_cnr_param[strength_level].iircnr_ee.iircnr_uv_s_th;

		for (i = 0; i < 8; i++) {
			dst_ptr->cur.y_edge_thr_max[i] = iir_cnr_param[strength_level].iircnr_ee.y_edge_thr_max[i];
			dst_ptr->cur.y_edge_thr_min[i] = iir_cnr_param[strength_level].iircnr_str.y_edge_thr_min[i];
		}

		dst_ptr->cur.y_max_th = iir_cnr_param[strength_level].iircnr_str.iircnr_y_max_th;
		dst_ptr->cur.y_min_th = iir_cnr_param[strength_level].iircnr_str.iircnr_y_min_th;
		dst_ptr->cur.y_th = iir_cnr_param[strength_level].iircnr_str.iircnr_y_th;
		dst_ptr->cur.uv_diff_thr = iir_cnr_param[strength_level].iircnr_str.iircnr_uv_diff_thr;
		dst_ptr->cur.alpha_low_u = iir_cnr_param[strength_level].iircnr_str.iircnr_alpha_low_u;
		dst_ptr->cur.alpha_low_v = iir_cnr_param[strength_level].iircnr_str.iircnr_alpha_low_v;

		for (i = 0; i < 8; i++) {
			dst_ptr->cur.slope_uv[i] = iir_cnr_param[strength_level].slop_uv[i];
			dst_ptr->cur.middle_factor_uv[i] = iir_cnr_param[strength_level].middle_factor_uv[i];
		}
		dst_ptr->cur.uv_low_thr1 = iir_cnr_param[strength_level].iircnr_ee.uv_low_thr1[0];
		dst_ptr->cur.uv_low_thr2 = iir_cnr_param[strength_level].iircnr_str.cnr_uv_thr2[0].uv_low_thr2;
		dst_ptr->cur.slope_y_0 = iir_cnr_param[strength_level].slop_y[0];
		dst_ptr->cur.middle_factor_y_0 = iir_cnr_param[strength_level].middle_factor_y[0];
		dst_ptr->cur.uv_high_thr2_0 = iir_cnr_param[strength_level].iircnr_str.cnr_uv_thr2[0].uv_high_thr2;
		for (i = 0; i < 7; i++) {
			dst_ptr->cur.uv_low_thr[i][1]= iir_cnr_param[strength_level].iircnr_ee.uv_low_thr1[i + 1];
			dst_ptr->cur.uv_low_thr[i][0] = iir_cnr_param[strength_level].iircnr_str.cnr_uv_thr2[i + 1].uv_low_thr2;
			dst_ptr->cur.uv_high_thr2[i] = iir_cnr_param[strength_level].iircnr_str.cnr_uv_thr2[i + 1].uv_high_thr2;
			dst_ptr->cur.slope_y[i] = iir_cnr_param[strength_level].slop_y[i + 1];
			dst_ptr->cur.middle_factor_y[i] = iir_cnr_param[strength_level].middle_factor_y[i + 1];
		}

		dst_ptr->cur.css_lum_thr = iir_cnr_param[strength_level].css_lum_thr.iircnr_css_lum_thr;
		dst_ptr->cur.mode = iir_cnr_param[strength_level].cnr_iir_mode;
		dst_ptr->cur.uv_pg_th = iir_cnr_param[strength_level].cnr_uv_pg_th;
		dst_ptr->cur.ymd_u = iir_cnr_param[strength_level].ymd_u;
		dst_ptr->cur.ymd_v = iir_cnr_param[strength_level].ymd_v;
		dst_ptr->cur.ymd_min_u = iir_cnr_param[strength_level].ymd_min_u;
		dst_ptr->cur.ymd_min_v = iir_cnr_param[strength_level].ymd_min_v;
	}
	return rtn;
}

cmr_s32 _pm_iircnr_iir_init(void *dst_iircnr_param, void *src_iircnr_param, void *param1, void *param_ptr2)
{
	cmr_s32 rtn = ISP_SUCCESS;
	cmr_s32 i = 0;
	struct isp_iircnr_iir_param *dst_ptr = (struct isp_iircnr_iir_param *)dst_iircnr_param;
	struct isp_pm_nr_header_param *src_ptr = (struct isp_pm_nr_header_param *)src_iircnr_param;
	struct isp_pm_block_header *header_ptr = (struct isp_pm_block_header *)param1;
	UNUSED(param_ptr2);

	dst_ptr->cur.bypass = header_ptr->bypass;

	dst_ptr->param_ptr = src_ptr->param_ptr;
	dst_ptr->cur_level = src_ptr->default_strength_level;
	dst_ptr->level_num = src_ptr->level_number;
	dst_ptr->scene_ptr = src_ptr->multi_nr_map_ptr;
	dst_ptr->nr_mode_setting = src_ptr->nr_mode_setting;

	rtn = _pm_iircnr_iir_convert_param(dst_ptr, dst_ptr->cur_level, ISP_MODE_ID_COMMON, ISP_SCENEMODE_AUTO);
	dst_ptr->cur.bypass |= header_ptr->bypass;
	if (ISP_SUCCESS != rtn) {
		ISP_LOGE("fail to convert pm iircnr iir param !");
		return rtn;
	}

	header_ptr->is_update = ISP_ONE;

	return rtn;
}

cmr_s32 _pm_iircnr_iir_set_param(void *iircnr_param, cmr_u32 cmd, void *param_ptr0, void *param_ptr1)
{
	cmr_s32 rtn = ISP_SUCCESS;
	cmr_u32 index = 0;
	struct isp_iircnr_iir_param *dst_ptr = (struct isp_iircnr_iir_param *)iircnr_param;
	struct isp_pm_block_header *header_ptr = (struct isp_pm_block_header *)param_ptr1;

	switch (cmd) {
	case ISP_PM_BLK_IIR_NR_STRENGTH_LEVEL:
		dst_ptr->cur_level = *((cmr_u32 *) param_ptr0);
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

			if (cur_level != dst_ptr->cur_level || nr_tool_flag[6] || block_result->mode_flag_changed) {
				dst_ptr->cur_level = cur_level;
				header_ptr->is_update = ISP_ONE;
				nr_tool_flag[6] = 0;
				block_result->mode_flag_changed = 0;
				rtn = _pm_iircnr_iir_convert_param(dst_ptr, dst_ptr->cur_level, block_result->mode_flag, block_result->scene_flag);
				dst_ptr->cur.bypass |= header_ptr->bypass;
				if (ISP_SUCCESS != rtn) {
					ISP_LOGE("fail to convert pm iircnr iir param !");
					return rtn;
				}
			}
		}
		break;

	default:
		break;
	}

	ISP_LOGV("ISP_SMART_NR: cmd=%d, update=%d, ccnr_level=%d", cmd, header_ptr->is_update, dst_ptr->cur_level);

	return rtn;

}

cmr_s32 _pm_iircnr_iir_get_param(void *iircnr_param, cmr_u32 cmd, void *rtn_param0, void *rtn_param1)
{
	cmr_s32 rtn = ISP_SUCCESS;
	struct isp_iircnr_iir_param *iircnr_ptr = (struct isp_iircnr_iir_param *)iircnr_param;
	struct isp_pm_param_data *param_data_ptr = (struct isp_pm_param_data *)rtn_param0;
	cmr_u32 *update_flag = (cmr_u32 *) rtn_param1;

	param_data_ptr->id = ISP_BLK_IIRCNR_IIR;
	param_data_ptr->cmd = cmd;

	switch (cmd) {
	case ISP_PM_BLK_ISP_SETTING:
		param_data_ptr->data_ptr = (void *)&iircnr_ptr->cur;
		param_data_ptr->data_size = sizeof(iircnr_ptr->cur);
		*update_flag = 0;
		break;

	default:
		break;
	}

	return rtn;
}
