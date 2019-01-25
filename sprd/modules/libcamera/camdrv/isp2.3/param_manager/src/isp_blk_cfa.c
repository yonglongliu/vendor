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
#define LOG_TAG "isp_blk_cfa"
#include "isp_blocks_cfg.h"

cmr_u32 _pm_cfa_convert_param(void *dst_cfae_param, cmr_u32 strength_level, cmr_u32 mode_flag, cmr_u32 scene_flag)
{
	cmr_s32 rtn = ISP_SUCCESS;
	cmr_s32 i = 0;
	cmr_u32 total_offset_units = 0;
	struct isp_cfa_param *dst_ptr = (struct isp_cfa_param *)dst_cfae_param;
	struct sensor_cfa_param_level *cfae_param = PNULL;

	if (SENSOR_MULTI_MODE_FLAG != dst_ptr->nr_mode_setting) {
		cfae_param = (struct sensor_cfa_param_level *)(dst_ptr->param_ptr);
	} else {
		cmr_u32 *multi_nr_map_ptr = PNULL;
		multi_nr_map_ptr = (cmr_u32 *) dst_ptr->scene_ptr;
		total_offset_units = _pm_calc_nr_addr_offset(mode_flag, scene_flag, multi_nr_map_ptr);
		cfae_param = (struct sensor_cfa_param_level *)((cmr_u8 *) dst_ptr->param_ptr + total_offset_units * dst_ptr->level_num * sizeof(struct sensor_cfa_param_level));

	}
	strength_level = PM_CLIP(strength_level, 0, dst_ptr->level_num - 1);

	if (cfae_param != NULL) {
		dst_ptr->cur.min_grid_new = cfae_param[strength_level].cfai_intplt_level.min_grid_new;
		dst_ptr->cur.grid_gain_new = cfae_param[strength_level].cfai_intplt_level.grid_gain_new;
		dst_ptr->cur.strong_edge_thr = cfae_param[strength_level].cfai_intplt_level.str_edge_tr;
		dst_ptr->cur.cdcr_adj_factor = cfae_param[strength_level].cfai_intplt_level.cdcr_adj_factor;
		dst_ptr->cur.grid_thr = cfae_param[strength_level].cfai_intplt_level.grid_tr;
		dst_ptr->cur.smooth_area_thr = cfae_param[strength_level].cfai_intplt_level.smooth_tr;
		dst_ptr->cur.uni_dir_intplt_thr_new = cfae_param[strength_level].cfai_intplt_level.uni_dir_intplt_tr;
		dst_ptr->cur.readblue_high_sat_thr = cfae_param[strength_level].cfai_intplt_level.rb_high_sat_thr;

		dst_ptr->cur.round_diff_03_thr = cfae_param[strength_level].cfai_intplt_level.gref_thr.round_diff_03_thr;
		dst_ptr->cur.low_lux_03_thr = cfae_param[strength_level].cfai_intplt_level.gref_thr.lowlux_03_thr;
		dst_ptr->cur.round_diff_12_thr = cfae_param[strength_level].cfai_intplt_level.gref_thr.round_diff_12_thr;
		dst_ptr->cur.low_lux_12_thr = cfae_param[strength_level].cfai_intplt_level.gref_thr.lowlux_12_thr;

		dst_ptr->cur.css_weak_edge_thr = cfae_param[strength_level].cfai_css_level.css_comm_thr.css_weak_egde_thr;
		dst_ptr->cur.css_edge_thr = cfae_param[strength_level].cfai_css_level.css_comm_thr.css_edge_thr;
		dst_ptr->cur.css_texture1_thr = cfae_param[strength_level].cfai_css_level.css_comm_thr.css_txt1_thr;
		dst_ptr->cur.css_texture2_thr = cfae_param[strength_level].cfai_css_level.css_comm_thr.css_txt2_thr;
		dst_ptr->cur.css_uv_val_thr = cfae_param[strength_level].cfai_css_level.css_comm_thr.uv_val_thr;
		dst_ptr->cur.css_uv_diff_thr = cfae_param[strength_level].cfai_css_level.css_comm_thr.uv_diff_thr;
		dst_ptr->cur.css_gray_thr = cfae_param[strength_level].cfai_css_level.css_comm_thr.gray_thr;
		dst_ptr->cur.css_pix_similar_thr = cfae_param[strength_level].cfai_css_level.css_comm_thr.pix_similar_thr;

		dst_ptr->cur.css_green_edge_thr = cfae_param[strength_level].cfai_css_level.css_green_thr.green_edge_thr;
		dst_ptr->cur.css_green_weak_edge_thr = cfae_param[strength_level].cfai_css_level.css_green_thr.green_weak_edge_thr;
		dst_ptr->cur.css_green_tex1_thr = cfae_param[strength_level].cfai_css_level.css_green_thr.green_txt1_thr;
		dst_ptr->cur.css_green_tex2_thr = cfae_param[strength_level].cfai_css_level.css_green_thr.green_txt2_thr;
		dst_ptr->cur.css_green_flat_thr = cfae_param[strength_level].cfai_css_level.css_green_thr.green_flat_thr;

		dst_ptr->cur.css_edge_corr_ratio_b = cfae_param[strength_level].cfai_css_level.b_ratio.edge_ratio;
		dst_ptr->cur.css_text1_corr_ratio_b = cfae_param[strength_level].cfai_css_level.b_ratio.txt1_ratio;
		dst_ptr->cur.css_text2_corr_ratio_b = cfae_param[strength_level].cfai_css_level.b_ratio.txt2_ratio;
		dst_ptr->cur.css_flat_corr_ratio_b = cfae_param[strength_level].cfai_css_level.b_ratio.flat_ratio;
		dst_ptr->cur.css_wedge_corr_ratio_b = cfae_param[strength_level].cfai_css_level.b_ratio.weak_edge_ratio;

		dst_ptr->cur.css_edge_corr_ratio_r = cfae_param[strength_level].cfai_css_level.r_ratio.edge_ratio;
		dst_ptr->cur.css_text1_corr_ratio_r = cfae_param[strength_level].cfai_css_level.r_ratio.txt1_ratio;
		dst_ptr->cur.css_text2_corr_ratio_r = cfae_param[strength_level].cfai_css_level.r_ratio.txt2_ratio;
		dst_ptr->cur.css_flat_corr_ratio_r = cfae_param[strength_level].cfai_css_level.r_ratio.flat_ratio;
		dst_ptr->cur.css_wedge_corr_ratio_r = cfae_param[strength_level].cfai_css_level.r_ratio.weak_edge_ratio;

		dst_ptr->cur.css_alpha_for_tex2 = cfae_param[strength_level].cfai_css_level.css_alpha_for_txt2;

		dst_ptr->cur.css_skin_u_top[0] = cfae_param[strength_level].cfai_css_level.css_skin_uv.skin_u1.skin_top;
		dst_ptr->cur.css_skin_u_top[1] = cfae_param[strength_level].cfai_css_level.css_skin_uv.skin_u2.skin_top;
		dst_ptr->cur.css_skin_u_down[0] = cfae_param[strength_level].cfai_css_level.css_skin_uv.skin_u1.skin_down;
		dst_ptr->cur.css_skin_u_down[1] = cfae_param[strength_level].cfai_css_level.css_skin_uv.skin_u2.skin_down;
		dst_ptr->cur.css_skin_v_top[0] = cfae_param[strength_level].cfai_css_level.css_skin_uv.skin_v1.skin_top;
		dst_ptr->cur.css_skin_v_top[1] = cfae_param[strength_level].cfai_css_level.css_skin_uv.skin_v2.skin_top;
		dst_ptr->cur.css_skin_v_down[0] = cfae_param[strength_level].cfai_css_level.css_skin_uv.skin_v1.skin_down;
		dst_ptr->cur.css_skin_v_down[1] = cfae_param[strength_level].cfai_css_level.css_skin_uv.skin_v2.skin_down;

		dst_ptr->cur.weight_control_bypass = cfae_param[strength_level].cfai_intplt_level.cfai_dir_intplt.wgtctrl_bypass;
		dst_ptr->cur.grid_dir_weight_t1 = cfae_param[strength_level].cfai_intplt_level.cfai_dir_intplt.grid_dir_wgt1;
		dst_ptr->cur.grid_dir_weight_t2 = cfae_param[strength_level].cfai_intplt_level.cfai_dir_intplt.grid_dir_wgt2;
		dst_ptr->cur.css_bypass = cfae_param[strength_level].cfai_css_level.css_bypass;
	}

	return rtn;
}

cmr_s32 _pm_cfa_init(void *dst_cfae_param, void *src_cfae_param, void *param1, void *param_ptr2)
{
	cmr_s32 rtn = ISP_SUCCESS;

	struct isp_cfa_param *dst_ptr = (struct isp_cfa_param *)dst_cfae_param;
	struct isp_pm_nr_header_param *src_ptr = (struct isp_pm_nr_header_param *)src_cfae_param;
	struct isp_pm_block_header *header_ptr = (struct isp_pm_block_header *)param1;
	struct isp_size *img_size_ptr = (struct isp_size *)param_ptr2;

	dst_ptr->cur.bypass = header_ptr->bypass;
	dst_ptr->cur_level = src_ptr->default_strength_level;
	dst_ptr->level_num = src_ptr->level_number;
	dst_ptr->param_ptr = src_ptr->param_ptr;
	dst_ptr->scene_ptr = src_ptr->multi_nr_map_ptr;
	dst_ptr->nr_mode_setting = src_ptr->nr_mode_setting;

	dst_ptr->cur.gbuf_addr_max = (img_size_ptr->w / 2) + 1;
	rtn = _pm_cfa_convert_param(dst_ptr, dst_ptr->cur_level, ISP_MODE_ID_COMMON, ISP_SCENEMODE_AUTO);
	dst_ptr->cur.bypass |= header_ptr->bypass;
	if (ISP_SUCCESS != rtn) {
		ISP_LOGE("fail to convert pm cfa param !");
		return rtn;
	}

	header_ptr->is_update = ISP_ONE;
	return rtn;
}

cmr_s32 _pm_cfa_set_param(void *cfae_param, cmr_u32 cmd, void *param_ptr0, void *param_ptr1)
{
	cmr_s32 rtn = ISP_SUCCESS;
	cmr_s32 frame_width = 0;
	struct isp_cfa_param *cfae_ptr = (struct isp_cfa_param *)cfae_param;
	struct isp_pm_block_header *cfae_header_ptr = (struct isp_pm_block_header *)param_ptr1;

	switch (cmd) {
	case ISP_PM_BLK_CFA_BYPASS:
		cfae_ptr->cur.bypass = *((cmr_u32 *) param_ptr0);
		cfae_header_ptr->is_update = ISP_ONE;
		break;

	case ISP_PM_BLK_CFA_CFG:
		frame_width = *((cmr_u32 *) param_ptr0);
		cfae_ptr->cur.gbuf_addr_max = (frame_width / 2) + 1;
		cfae_header_ptr->is_update = ISP_ONE;
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

			if (cur_level != cfae_ptr->cur_level || nr_tool_flag[3] || block_result->mode_flag_changed) {
				cfae_ptr->cur_level = cur_level;
				cfae_header_ptr->is_update = 1;
				nr_tool_flag[3] = 0;
				block_result->mode_flag_changed = 0;
				rtn = _pm_cfa_convert_param(cfae_ptr, cfae_ptr->cur_level, block_result->mode_flag, block_result->scene_flag);
				if (ISP_SUCCESS != rtn) {
					ISP_LOGE("fail to convert pm cfa param");
					return rtn;
				}
			}
		}
		break;

	default:

		break;
	}

	ISP_LOGV("ISP_SMART: cmd = %d, is_update = %d, cur_level=%d", cmd, cfae_header_ptr->is_update, cfae_ptr->cur_level);

	return rtn;
}

cmr_s32 _pm_cfa_get_param(void *cfa_param, cmr_u32 cmd, void *rtn_param0, void *rtn_param1)
{
	cmr_s32 rtn = ISP_SUCCESS;
	struct isp_cfa_param *cfa_ptr = (struct isp_cfa_param *)cfa_param;
	struct isp_pm_param_data *param_data_ptr = (struct isp_pm_param_data *)rtn_param0;
	cmr_u32 *update_flag = (cmr_u32 *) rtn_param1;

	param_data_ptr->id = ISP_BLK_CFA;
	param_data_ptr->cmd = cmd;

	switch (cmd) {
	case ISP_PM_BLK_ISP_SETTING:
		param_data_ptr->data_ptr = &cfa_ptr->cur;
		param_data_ptr->data_size = sizeof(cfa_ptr->cur);
		*update_flag = 0;
		break;
	default:
		break;
	}

	return rtn;
}
