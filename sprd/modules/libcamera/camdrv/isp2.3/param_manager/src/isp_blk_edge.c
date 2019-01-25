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
#define LOG_TAG "isp_blk_edge"
#include "isp_blocks_cfg.h"

cmr_u32 _pm_edge_convert_param(void *dst_edge_param, cmr_u32 strength_level, cmr_u32 mode_flag, cmr_u32 scene_flag)
{
	cmr_s32 rtn = ISP_SUCCESS;
	cmr_s32 i = 0;
	cmr_u32 total_offset_units = 0;
	struct isp_edge_param *dst_ptr = (struct isp_edge_param *)dst_edge_param;
	struct sensor_ee_level *edge_param = PNULL;

	if (SENSOR_MULTI_MODE_FLAG != dst_ptr->nr_mode_setting) {
		edge_param = (struct sensor_ee_level *)(dst_ptr->param_ptr);
	} else {
		cmr_u32 *multi_nr_map_ptr = PNULL;
		multi_nr_map_ptr = (cmr_u32 *) dst_ptr->scene_ptr;
		total_offset_units = _pm_calc_nr_addr_offset(mode_flag, scene_flag, multi_nr_map_ptr);
		edge_param = (struct sensor_ee_level *)((cmr_u8 *) dst_ptr->param_ptr + total_offset_units * dst_ptr->level_num * sizeof(struct sensor_ee_level));

	}
	strength_level = PM_CLIP(strength_level, 0, dst_ptr->level_num - 1);

	if (edge_param != NULL) {
		dst_ptr->cur.bypass = edge_param[strength_level].bypass;

		dst_ptr->cur.flat_smooth_mode = edge_param[strength_level].flat_smooth_mode;
		dst_ptr->cur.edge_smooth_mode = edge_param[strength_level].edge_smooth_mode;
		dst_ptr->cur.mode = edge_param[strength_level].ee_mode;

		dst_ptr->cur.ee_str_d.p = edge_param[strength_level].str_d_p.positive;
		dst_ptr->cur.ee_str_d.n = edge_param[strength_level].str_d_p.negative;
		dst_ptr->cur.ee_incr_d.p = edge_param[strength_level].ee_incr_d.positive;
		dst_ptr->cur.ee_incr_d.n = edge_param[strength_level].ee_incr_d.negative;
		dst_ptr->cur.ee_edge_thr_d.p = edge_param[strength_level].ee_thr_d.positive;
		dst_ptr->cur.ee_edge_thr_d.n = edge_param[strength_level].ee_thr_d.negative;
		dst_ptr->cur.ee_cv_clip.p = edge_param[strength_level].ee_cv_clip.positive;
		dst_ptr->cur.ee_cv_clip.n = edge_param[strength_level].ee_cv_clip.negative;

		dst_ptr->cur.ee_corner_sm.p = edge_param[strength_level].ee_corner.ee_corner_sm.positive;
		dst_ptr->cur.ee_corner_sm.n = edge_param[strength_level].ee_corner.ee_corner_sm.negative;
		dst_ptr->cur.ee_corner_gain.p = edge_param[strength_level].ee_corner.ee_corner_gain.positive;
		dst_ptr->cur.ee_corner_gain.n = edge_param[strength_level].ee_corner.ee_corner_gain.negative;
		dst_ptr->cur.ee_corner_th.p = edge_param[strength_level].ee_corner.ee_corner_th.positive;
		dst_ptr->cur.ee_corner_th.n = edge_param[strength_level].ee_corner.ee_corner_th.negative;
		dst_ptr->cur.ee_corner_cor = edge_param[strength_level].ee_corner.ee_corner_cor;

		dst_ptr->cur.ipd_bypass = edge_param[strength_level].ee_ipd.ipd_bypass;
		dst_ptr->cur.ipd_mask_mode = edge_param[strength_level].ee_ipd.ipd_mask_mode;
		dst_ptr->cur.ipd_less_thr.p = edge_param[strength_level].ee_ipd.ipd_less_thr.positive;
		dst_ptr->cur.ipd_less_thr.n = edge_param[strength_level].ee_ipd.ipd_less_thr.negative;
		dst_ptr->cur.ipd_flat_thr.p = edge_param[strength_level].ee_ipd.ipd_flat_thr.positive;
		dst_ptr->cur.ipd_flat_thr.n = edge_param[strength_level].ee_ipd.ipd_flat_thr.negative;
		dst_ptr->cur.ipd_eq_thr.p = edge_param[strength_level].ee_ipd.ipd_eq_thr.positive;
		dst_ptr->cur.ipd_eq_thr.n = edge_param[strength_level].ee_ipd.ipd_eq_thr.negative;
		dst_ptr->cur.ipd_more_thr.p = edge_param[strength_level].ee_ipd.ipd_more_thr.positive;
		dst_ptr->cur.ipd_more_thr.n = edge_param[strength_level].ee_ipd.ipd_more_thr.negative;
		dst_ptr->cur.ipd_smooth_en = edge_param[strength_level].ee_ipd.ipd_smooth.smooth_en;
		dst_ptr->cur.ipd_smooth_mode.p = edge_param[strength_level].ee_ipd.ipd_smooth.smooth_mode.positive;
		dst_ptr->cur.ipd_smooth_mode.n = edge_param[strength_level].ee_ipd.ipd_smooth.smooth_mode.negative;
		dst_ptr->cur.ipd_smooth_edge_thr.p = edge_param[strength_level].ee_ipd.ipd_smooth.smooth_ee_thr.positive;
		dst_ptr->cur.ipd_smooth_edge_thr.n = edge_param[strength_level].ee_ipd.ipd_smooth.smooth_ee_thr.negative;
		dst_ptr->cur.ipd_smooth_edge_diff.p = edge_param[strength_level].ee_ipd.ipd_smooth.smooth_ee_diff.positive;
		dst_ptr->cur.ipd_smooth_edge_diff.n = edge_param[strength_level].ee_ipd.ipd_smooth.smooth_ee_diff.negative;

		dst_ptr->cur.ee_ratio_hv_3 = edge_param[strength_level].ee_gradient.ratio.ratio_hv_3;
		dst_ptr->cur.ee_ratio_hv_5 = edge_param[strength_level].ee_gradient.ratio.ratio_hv_5;
		dst_ptr->cur.ee_ratio_diag_3 = edge_param[strength_level].ee_gradient.ratio.ratio_dg_3;
		dst_ptr->cur.ee_ratio_diag_5 = edge_param[strength_level].ee_gradient.ratio.ratio_dg_5;
		dst_ptr->cur.ee_weight_hv2diag = edge_param[strength_level].ee_gradient.wgt_hv2diag;
		dst_ptr->cur.ee_weight_diag2hv = edge_param[strength_level].ee_gradient.wgt_diag2hv;

		dst_ptr->cur.ee_gradient_computation_type = edge_param[strength_level].ee_gradient.grd_cmpt_type;

		dst_ptr->cur.ee_gain_hv_t[0][0] = edge_param[strength_level].ee_gradient.ee_gain_hv1.t_cfg.ee_t1_cfg;
		dst_ptr->cur.ee_gain_hv_t[0][1] = edge_param[strength_level].ee_gradient.ee_gain_hv1.t_cfg.ee_t2_cfg;
		dst_ptr->cur.ee_gain_hv_t[0][2] = edge_param[strength_level].ee_gradient.ee_gain_hv1.t_cfg.ee_t3_cfg;
		dst_ptr->cur.ee_gain_hv_t[0][3] = edge_param[strength_level].ee_gradient.ee_gain_hv1.t_cfg.ee_t4_cfg;
		dst_ptr->cur.ee_gain_hv_t[1][0] = edge_param[strength_level].ee_gradient.ee_gain_hv2.t_cfg.ee_t1_cfg;
		dst_ptr->cur.ee_gain_hv_t[1][1] = edge_param[strength_level].ee_gradient.ee_gain_hv2.t_cfg.ee_t2_cfg;
		dst_ptr->cur.ee_gain_hv_t[1][2] = edge_param[strength_level].ee_gradient.ee_gain_hv2.t_cfg.ee_t3_cfg;
		dst_ptr->cur.ee_gain_hv_t[1][3] = edge_param[strength_level].ee_gradient.ee_gain_hv2.t_cfg.ee_t4_cfg;
		dst_ptr->cur.ee_gain_hv_r[0][0] = edge_param[strength_level].ee_gradient.ee_gain_hv1.r_cfg.ee_r1_cfg;
		dst_ptr->cur.ee_gain_hv_r[0][1] = edge_param[strength_level].ee_gradient.ee_gain_hv1.r_cfg.ee_r2_cfg;
		dst_ptr->cur.ee_gain_hv_r[0][2] = edge_param[strength_level].ee_gradient.ee_gain_hv1.r_cfg.ee_r3_cfg;
		dst_ptr->cur.ee_gain_hv_r[1][0] = edge_param[strength_level].ee_gradient.ee_gain_hv2.r_cfg.ee_r1_cfg;
		dst_ptr->cur.ee_gain_hv_r[1][1] = edge_param[strength_level].ee_gradient.ee_gain_hv2.r_cfg.ee_r2_cfg;
		dst_ptr->cur.ee_gain_hv_r[1][2] = edge_param[strength_level].ee_gradient.ee_gain_hv2.r_cfg.ee_r3_cfg;

		dst_ptr->cur.ee_gain_diag_t[0][0] = edge_param[strength_level].ee_gradient.ee_gain_dg1.t_cfg.ee_t1_cfg;
		dst_ptr->cur.ee_gain_diag_t[0][1] = edge_param[strength_level].ee_gradient.ee_gain_dg1.t_cfg.ee_t2_cfg;
		dst_ptr->cur.ee_gain_diag_t[0][2] = edge_param[strength_level].ee_gradient.ee_gain_dg1.t_cfg.ee_t3_cfg;
		dst_ptr->cur.ee_gain_diag_t[0][3] = edge_param[strength_level].ee_gradient.ee_gain_dg1.t_cfg.ee_t4_cfg;
		dst_ptr->cur.ee_gain_diag_t[1][0] = edge_param[strength_level].ee_gradient.ee_gain_dg2.t_cfg.ee_t1_cfg;
		dst_ptr->cur.ee_gain_diag_t[1][1] = edge_param[strength_level].ee_gradient.ee_gain_dg2.t_cfg.ee_t2_cfg;
		dst_ptr->cur.ee_gain_diag_t[1][2] = edge_param[strength_level].ee_gradient.ee_gain_dg2.t_cfg.ee_t3_cfg;
		dst_ptr->cur.ee_gain_diag_t[1][3] = edge_param[strength_level].ee_gradient.ee_gain_dg2.t_cfg.ee_t4_cfg;
		dst_ptr->cur.ee_gain_diag_r[0][0] = edge_param[strength_level].ee_gradient.ee_gain_dg1.r_cfg.ee_r1_cfg;
		dst_ptr->cur.ee_gain_diag_r[0][1] = edge_param[strength_level].ee_gradient.ee_gain_dg1.r_cfg.ee_r2_cfg;
		dst_ptr->cur.ee_gain_diag_r[0][2] = edge_param[strength_level].ee_gradient.ee_gain_dg1.r_cfg.ee_r3_cfg;
		dst_ptr->cur.ee_gain_diag_r[1][0] = edge_param[strength_level].ee_gradient.ee_gain_dg2.r_cfg.ee_r1_cfg;
		dst_ptr->cur.ee_gain_diag_r[1][1] = edge_param[strength_level].ee_gradient.ee_gain_dg2.r_cfg.ee_r2_cfg;
		dst_ptr->cur.ee_gain_diag_r[1][2] = edge_param[strength_level].ee_gradient.ee_gain_dg2.r_cfg.ee_r3_cfg;

		dst_ptr->cur.ee_cv_t[0] = edge_param[strength_level].ee_cv.t_cfg.ee_t1_cfg;
		dst_ptr->cur.ee_cv_t[1] = edge_param[strength_level].ee_cv.t_cfg.ee_t2_cfg;
		dst_ptr->cur.ee_cv_t[2] = edge_param[strength_level].ee_cv.t_cfg.ee_t3_cfg;
		dst_ptr->cur.ee_cv_t[3] = edge_param[strength_level].ee_cv.t_cfg.ee_t4_cfg;
		dst_ptr->cur.ee_cv_r[0] = edge_param[strength_level].ee_cv.r_cfg.ee_r1_cfg;
		dst_ptr->cur.ee_cv_r[1] = edge_param[strength_level].ee_cv.r_cfg.ee_r2_cfg;
		dst_ptr->cur.ee_cv_r[2] = edge_param[strength_level].ee_cv.r_cfg.ee_r3_cfg;

		dst_ptr->cur.ee_lum_t[0] = edge_param[strength_level].ee_lum.t_cfg.ee_t1_cfg;
		dst_ptr->cur.ee_lum_t[1] = edge_param[strength_level].ee_lum.t_cfg.ee_t2_cfg;
		dst_ptr->cur.ee_lum_t[2] = edge_param[strength_level].ee_lum.t_cfg.ee_t3_cfg;
		dst_ptr->cur.ee_lum_t[3] = edge_param[strength_level].ee_lum.t_cfg.ee_t4_cfg;
		dst_ptr->cur.ee_lum_r[0] = edge_param[strength_level].ee_lum.r_cfg.ee_r1_cfg;
		dst_ptr->cur.ee_lum_r[1] = edge_param[strength_level].ee_lum.r_cfg.ee_r2_cfg;
		dst_ptr->cur.ee_lum_r[2] = edge_param[strength_level].ee_lum.r_cfg.ee_r3_cfg;

		dst_ptr->cur.ee_freq_t[0] = edge_param[strength_level].ee_freq.t_cfg.ee_t1_cfg;
		dst_ptr->cur.ee_freq_t[1] = edge_param[strength_level].ee_freq.t_cfg.ee_t2_cfg;
		dst_ptr->cur.ee_freq_t[2] = edge_param[strength_level].ee_freq.t_cfg.ee_t3_cfg;
		dst_ptr->cur.ee_freq_t[3] = edge_param[strength_level].ee_freq.t_cfg.ee_t4_cfg;
		dst_ptr->cur.ee_freq_r[0] = edge_param[strength_level].ee_freq.r_cfg.ee_r1_cfg;
		dst_ptr->cur.ee_freq_r[1] = edge_param[strength_level].ee_freq.r_cfg.ee_r2_cfg;
		dst_ptr->cur.ee_freq_r[2] = edge_param[strength_level].ee_freq.r_cfg.ee_r3_cfg;

		dst_ptr->cur.ee_pos_t[0] = edge_param[strength_level].ee_clip.ee_pos.t_cfg.ee_t1_cfg;
		dst_ptr->cur.ee_pos_t[1] = edge_param[strength_level].ee_clip.ee_pos.t_cfg.ee_t2_cfg;
		dst_ptr->cur.ee_pos_t[2] = edge_param[strength_level].ee_clip.ee_pos.t_cfg.ee_t3_cfg;
		dst_ptr->cur.ee_pos_t[3] = edge_param[strength_level].ee_clip.ee_pos.t_cfg.ee_t4_cfg;
		dst_ptr->cur.ee_pos_r[0] = edge_param[strength_level].ee_clip.ee_pos.r_cfg.ee_r1_cfg;
		dst_ptr->cur.ee_pos_r[1] = edge_param[strength_level].ee_clip.ee_pos.r_cfg.ee_r2_cfg;
		dst_ptr->cur.ee_pos_r[2] = edge_param[strength_level].ee_clip.ee_pos.r_cfg.ee_r3_cfg;
		dst_ptr->cur.ee_pos_c[0] = edge_param[strength_level].ee_clip.ee_pos_c.ee_c1_cfg;
		dst_ptr->cur.ee_pos_c[1] = edge_param[strength_level].ee_clip.ee_pos_c.ee_c2_cfg;
		dst_ptr->cur.ee_pos_c[2] = edge_param[strength_level].ee_clip.ee_pos_c.ee_c3_cfg;
		dst_ptr->cur.ee_neg_t[0] = edge_param[strength_level].ee_clip.ee_neg.t_cfg.ee_t1_cfg;
		dst_ptr->cur.ee_neg_t[1] = edge_param[strength_level].ee_clip.ee_neg.t_cfg.ee_t2_cfg;
		dst_ptr->cur.ee_neg_t[2] = edge_param[strength_level].ee_clip.ee_neg.t_cfg.ee_t3_cfg;
		dst_ptr->cur.ee_neg_t[3] = edge_param[strength_level].ee_clip.ee_neg.t_cfg.ee_t4_cfg;
		dst_ptr->cur.ee_neg_r[0] = edge_param[strength_level].ee_clip.ee_neg.r_cfg.ee_r1_cfg;
		dst_ptr->cur.ee_neg_r[1] = edge_param[strength_level].ee_clip.ee_neg.r_cfg.ee_r2_cfg;
		dst_ptr->cur.ee_neg_r[2] = edge_param[strength_level].ee_clip.ee_neg.r_cfg.ee_r3_cfg;
		dst_ptr->cur.ee_neg_c[0] = edge_param[strength_level].ee_clip.ee_neg_c.ee_c1_cfg;
		dst_ptr->cur.ee_neg_c[1] = edge_param[strength_level].ee_clip.ee_neg_c.ee_c2_cfg;
		dst_ptr->cur.ee_neg_c[2] = edge_param[strength_level].ee_clip.ee_neg_c.ee_c3_cfg;
	}
	return rtn;
}

cmr_s32 _pm_edge_init(void *dst_edge_param, void *src_edge_param, void *param1, void *param2)
{
	cmr_s32 rtn = ISP_SUCCESS;
	cmr_u32 i = 0;
	struct isp_pm_block_header *header_ptr = (struct isp_pm_block_header *)param1;
	struct isp_pm_nr_header_param *src_ptr = (struct isp_pm_nr_header_param *)src_edge_param;
	struct isp_edge_param *dst_ptr = (struct isp_edge_param *)dst_edge_param;
	UNUSED(param2);

	dst_ptr->cur.bypass = header_ptr->bypass;

	dst_ptr->param_ptr = src_ptr->param_ptr;
	dst_ptr->cur_level = src_ptr->default_strength_level;
	dst_ptr->level_num = src_ptr->level_number;
	dst_ptr->scene_ptr = src_ptr->multi_nr_map_ptr;
	dst_ptr->nr_mode_setting = src_ptr->nr_mode_setting;

	rtn = _pm_edge_convert_param(dst_ptr, dst_ptr->cur_level, ISP_MODE_ID_COMMON, ISP_SCENEMODE_AUTO);
	dst_ptr->cur.bypass |= header_ptr->bypass;
	if (ISP_SUCCESS != rtn) {
		ISP_LOGE("fail to convert pm edge param !");
		return rtn;
	}

	header_ptr->is_update = ISP_ONE;

	return rtn;
}

cmr_s32 _pm_edge_set_param(void *edge_param, cmr_u32 cmd, void *param_ptr0, void *param_ptr1)
{
	cmr_s32 rtn = ISP_SUCCESS;
	struct isp_pm_block_header *header_ptr = (struct isp_pm_block_header *)param_ptr1;
	struct isp_edge_param *dst_ptr = (struct isp_edge_param *)edge_param;

	switch (cmd) {
	case ISP_PM_BLK_EDGE_BYPASS:
		dst_ptr->cur.bypass = *((cmr_u32 *) param_ptr0);
		header_ptr->is_update = ISP_ONE;
		break;

	case ISP_PM_BLK_EDGE_STRENGTH:
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

			if (level != dst_ptr->cur_level || nr_tool_flag[4] || block_result->mode_flag_changed) {
				dst_ptr->cur_level = level;
				header_ptr->is_update = ISP_ONE;
				nr_tool_flag[4] = 0;
				block_result->mode_flag_changed = 0;

				rtn = _pm_edge_convert_param(dst_ptr, dst_ptr->cur_level, block_result->mode_flag, block_result->scene_flag);
				dst_ptr->cur.bypass |= header_ptr->bypass;
				if (ISP_SUCCESS != rtn) {
					ISP_LOGE("fail to convert pm edge param !");
					return rtn;
				}
			}
		}
		break;
#if 0
	case ISP_PM_BLK_SCENE_MODE:
		{
			cmr_u32 idx = *((cmr_u32 *) param_ptr0);
			if (0 == idx) {
				//      edge_ptr->cur.ee_level = edge_ptr->tab[edge_ptr->cur_idx] + edge_ptr->level;
			} else {
				//      edge_ptr->cur.ee_level = edge_ptr->scene_mode_tab[idx] + edge_ptr->level;
			}
			edge_header_ptr->is_update = ISP_ZERO;
		}
		break;
#endif
	default:
		break;
	}

	ISP_LOGV("ISP_SMART_NR: cmd=%d, update=%d, ee_level=%d", cmd, header_ptr->is_update, dst_ptr->cur_level);

	return rtn;
}

cmr_s32 _pm_edge_get_param(void *edge_param, cmr_u32 cmd, void *rtn_param0, void *rtn_param1)
{
	cmr_s32 rtn = ISP_SUCCESS;
	struct isp_edge_param *edge_ptr = (struct isp_edge_param *)edge_param;
	struct isp_pm_param_data *param_data_ptr = (struct isp_pm_param_data *)rtn_param0;
	cmr_u32 *update_flag = (cmr_u32 *) rtn_param1;

	param_data_ptr->id = ISP_BLK_EDGE;
	param_data_ptr->cmd = cmd;

	switch (cmd) {
	case ISP_PM_BLK_ISP_SETTING:
		param_data_ptr->data_ptr = &edge_ptr->cur;
		param_data_ptr->data_size = sizeof(edge_ptr->cur);
		*update_flag = ISP_ZERO;
		break;

	default:
		break;
	}

	return rtn;
}
