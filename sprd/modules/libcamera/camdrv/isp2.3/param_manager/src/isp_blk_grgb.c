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
#define LOG_TAG "isp_blk_grgb"
#include "isp_blocks_cfg.h"

cmr_u32 _pm_grgb_convert_param(void *dst_param, cmr_u32 strength_level, cmr_u32 mode_flag, cmr_u32 scene_flag)
{
	cmr_s32 rtn = ISP_SUCCESS;
	cmr_s32 i = 0;
	cmr_u32 total_offset_units = 0;
	struct isp_grgb_param *dst_ptr = (struct isp_grgb_param *)dst_param;
	struct sensor_grgb_level *grgb_param = PNULL;

	if (SENSOR_MULTI_MODE_FLAG != dst_ptr->nr_mode_setting) {
		grgb_param = (struct sensor_grgb_level *)(dst_ptr->param_ptr);
	} else {
		cmr_u32 *multi_nr_map_ptr = PNULL;
		multi_nr_map_ptr = (cmr_u32 *) dst_ptr->scene_ptr;
		total_offset_units = _pm_calc_nr_addr_offset(mode_flag, scene_flag, multi_nr_map_ptr);
		grgb_param = (struct sensor_grgb_level *)((cmr_u8 *) dst_ptr->param_ptr + total_offset_units * dst_ptr->level_num * sizeof(struct sensor_grgb_level));
	}

	strength_level = PM_CLIP(strength_level, 0, dst_ptr->level_num - 1);

	if (grgb_param != NULL) {
		dst_ptr->cur.diff_thd = grgb_param[strength_level].diff_th;
		dst_ptr->cur.hv_edge_thr = grgb_param[strength_level].hv_edge_thr;
		dst_ptr->cur.hv_flat_thr = grgb_param[strength_level].hv_flat_thr;
		dst_ptr->cur.slash_edge_thr = grgb_param[strength_level].slash_edge_thr;
		dst_ptr->cur.slash_flat_thr = grgb_param[strength_level].slash_flat_thr;
		dst_ptr->cur.gr_ratio = grgb_param[strength_level].grgb_ratio.gr_ratio;
		dst_ptr->cur.gb_ratio = grgb_param[strength_level].grgb_ratio.gb_ratio;
		//lum
		dst_ptr->cur.lum.curve_t[0][0] = grgb_param[strength_level].lum_curve_flat.t_cfg.grgb_t1_cfg;
		dst_ptr->cur.lum.curve_t[0][1] = grgb_param[strength_level].lum_curve_flat.t_cfg.grgb_t2_cfg;
		dst_ptr->cur.lum.curve_t[0][2] = grgb_param[strength_level].lum_curve_flat.t_cfg.grgb_t3_cfg;
		dst_ptr->cur.lum.curve_t[0][3] = grgb_param[strength_level].lum_curve_flat.t_cfg.grgb_t4_cfg;
		dst_ptr->cur.lum.curve_r[0][0] = grgb_param[strength_level].lum_curve_flat.r_cfg.grgb_r1_cfg;
		dst_ptr->cur.lum.curve_r[0][1] = grgb_param[strength_level].lum_curve_flat.r_cfg.grgb_r2_cfg;
		dst_ptr->cur.lum.curve_r[0][2] = grgb_param[strength_level].lum_curve_flat.r_cfg.grgb_r3_cfg;

		dst_ptr->cur.lum.curve_t[1][0] = grgb_param[strength_level].lum_curve_edge.t_cfg.grgb_t1_cfg;
		dst_ptr->cur.lum.curve_t[1][1] = grgb_param[strength_level].lum_curve_edge.t_cfg.grgb_t2_cfg;
		dst_ptr->cur.lum.curve_t[1][2] = grgb_param[strength_level].lum_curve_edge.t_cfg.grgb_t3_cfg;
		dst_ptr->cur.lum.curve_t[1][3] = grgb_param[strength_level].lum_curve_edge.t_cfg.grgb_t4_cfg;
		dst_ptr->cur.lum.curve_r[1][0] = grgb_param[strength_level].lum_curve_edge.r_cfg.grgb_r1_cfg;
		dst_ptr->cur.lum.curve_r[1][1] = grgb_param[strength_level].lum_curve_edge.r_cfg.grgb_r2_cfg;
		dst_ptr->cur.lum.curve_r[1][2] = grgb_param[strength_level].lum_curve_edge.r_cfg.grgb_r3_cfg;

		dst_ptr->cur.lum.curve_t[2][0] = grgb_param[strength_level].lum_curve_tex.t_cfg.grgb_t1_cfg;
		dst_ptr->cur.lum.curve_t[2][1] = grgb_param[strength_level].lum_curve_tex.t_cfg.grgb_t2_cfg;
		dst_ptr->cur.lum.curve_t[2][2] = grgb_param[strength_level].lum_curve_tex.t_cfg.grgb_t3_cfg;
		dst_ptr->cur.lum.curve_t[2][3] = grgb_param[strength_level].lum_curve_tex.t_cfg.grgb_t4_cfg;
		dst_ptr->cur.lum.curve_r[2][0] = grgb_param[strength_level].lum_curve_tex.r_cfg.grgb_r1_cfg;
		dst_ptr->cur.lum.curve_r[2][1] = grgb_param[strength_level].lum_curve_tex.r_cfg.grgb_r2_cfg;
		dst_ptr->cur.lum.curve_r[2][2] = grgb_param[strength_level].lum_curve_tex.r_cfg.grgb_r3_cfg;
		//frez
		dst_ptr->cur.frez.curve_t[0][0] = grgb_param[strength_level].frez_curve_flat.t_cfg.grgb_t1_cfg;
		dst_ptr->cur.frez.curve_t[0][1] = grgb_param[strength_level].frez_curve_flat.t_cfg.grgb_t2_cfg;
		dst_ptr->cur.frez.curve_t[0][2] = grgb_param[strength_level].frez_curve_flat.t_cfg.grgb_t3_cfg;
		dst_ptr->cur.frez.curve_t[0][3] = grgb_param[strength_level].frez_curve_flat.t_cfg.grgb_t4_cfg;
		dst_ptr->cur.frez.curve_r[0][0] = grgb_param[strength_level].frez_curve_flat.r_cfg.grgb_r1_cfg;
		dst_ptr->cur.frez.curve_r[0][1] = grgb_param[strength_level].frez_curve_flat.r_cfg.grgb_r2_cfg;
		dst_ptr->cur.frez.curve_r[0][2] = grgb_param[strength_level].frez_curve_flat.r_cfg.grgb_r3_cfg;

		dst_ptr->cur.frez.curve_t[1][0] = grgb_param[strength_level].frez_curve_edge.t_cfg.grgb_t1_cfg;
		dst_ptr->cur.frez.curve_t[1][1] = grgb_param[strength_level].frez_curve_edge.t_cfg.grgb_t2_cfg;
		dst_ptr->cur.frez.curve_t[1][2] = grgb_param[strength_level].frez_curve_edge.t_cfg.grgb_t3_cfg;
		dst_ptr->cur.frez.curve_t[1][3] = grgb_param[strength_level].frez_curve_edge.t_cfg.grgb_t4_cfg;
		dst_ptr->cur.frez.curve_r[1][0] = grgb_param[strength_level].frez_curve_edge.r_cfg.grgb_r1_cfg;
		dst_ptr->cur.frez.curve_r[1][1] = grgb_param[strength_level].frez_curve_edge.r_cfg.grgb_r2_cfg;
		dst_ptr->cur.frez.curve_r[1][2] = grgb_param[strength_level].frez_curve_edge.r_cfg.grgb_r3_cfg;

		dst_ptr->cur.frez.curve_t[2][0] = grgb_param[strength_level].frez_curve_tex.t_cfg.grgb_t1_cfg;
		dst_ptr->cur.frez.curve_t[2][1] = grgb_param[strength_level].frez_curve_tex.t_cfg.grgb_t2_cfg;
		dst_ptr->cur.frez.curve_t[2][2] = grgb_param[strength_level].frez_curve_tex.t_cfg.grgb_t3_cfg;
		dst_ptr->cur.frez.curve_t[2][3] = grgb_param[strength_level].frez_curve_tex.t_cfg.grgb_t4_cfg;
		dst_ptr->cur.frez.curve_r[2][0] = grgb_param[strength_level].frez_curve_tex.r_cfg.grgb_r1_cfg;
		dst_ptr->cur.frez.curve_r[2][1] = grgb_param[strength_level].frez_curve_tex.r_cfg.grgb_r2_cfg;
		dst_ptr->cur.frez.curve_r[2][2] = grgb_param[strength_level].frez_curve_tex.r_cfg.grgb_r3_cfg;
	}

	return rtn;
}

cmr_s32 _pm_grgb_init(void *dst_grgb_param, void *src_grgb_param, void *param1, void *param2)
{
	cmr_s32 rtn = ISP_SUCCESS;
	struct isp_grgb_param *dst_ptr = (struct isp_grgb_param *)dst_grgb_param;
	struct isp_pm_nr_header_param *src_ptr = (struct isp_pm_nr_header_param *)src_grgb_param;
	struct isp_pm_block_header *grgb_header_ptr = (struct isp_pm_block_header *)param1;
	UNUSED(param2);

	dst_ptr->cur.bypass = grgb_header_ptr->bypass;

	dst_ptr->param_ptr = src_ptr->param_ptr;
	dst_ptr->cur_level = src_ptr->default_strength_level;
	dst_ptr->level_num = src_ptr->level_number;
	dst_ptr->scene_ptr = src_ptr->multi_nr_map_ptr;
	dst_ptr->nr_mode_setting = src_ptr->nr_mode_setting;

	rtn = _pm_grgb_convert_param(dst_ptr, dst_ptr->cur_level, ISP_MODE_ID_COMMON, ISP_SCENEMODE_AUTO);
	dst_ptr->cur.bypass |= grgb_header_ptr->bypass;

	if (ISP_SUCCESS != rtn) {
		ISP_LOGE("fail to convert pm grgb param !");
		return rtn;
	}

	grgb_header_ptr->is_update = ISP_ONE;

	return rtn;
}

cmr_s32 _pm_grgb_set_param(void *grgb_param, cmr_u32 cmd, void *param_ptr0, void *param_ptr1)
{
	cmr_s32 rtn = ISP_SUCCESS;
	struct isp_grgb_param *dst_ptr = (struct isp_grgb_param *)grgb_param;
	struct isp_pm_block_header *header_ptr = (struct isp_pm_block_header *)param_ptr1;

	switch (cmd) {
	case ISP_PM_BLK_GRGB_BYPASS:
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
			if (cur_level != dst_ptr->cur_level || nr_tool_flag[5] || block_result->mode_flag_changed) {
				dst_ptr->cur_level = cur_level;
				header_ptr->is_update = ISP_ONE;
				nr_tool_flag[5] = 0;
				block_result->mode_flag_changed = 0;
				rtn = _pm_grgb_convert_param(dst_ptr, cur_level, block_result->mode_flag, block_result->scene_flag);
				dst_ptr->cur.bypass |= header_ptr->bypass;
				if (ISP_SUCCESS != rtn) {
					ISP_LOGE("fail to convert pm grgb param !");
					return rtn;
				}
			}
		}
		break;

	default:
		break;
	}

	ISP_LOGV("ISP_SMART: cmd = %d, is_update = %d, cur_level=%d", cmd, header_ptr->is_update, dst_ptr->cur_level);

	return rtn;
}

cmr_s32 _pm_grgb_get_param(void *grgb_param, cmr_u32 cmd, void *rtn_param0, void *rtn_param1)
{
	cmr_s32 rtn = ISP_SUCCESS;
	struct isp_grgb_param *grgb_ptr = (struct isp_grgb_param *)grgb_param;
	struct isp_pm_param_data *param_data_ptr = (struct isp_pm_param_data *)rtn_param0;
	cmr_u32 *update_flag = (cmr_u32 *) rtn_param1;

	param_data_ptr->id = ISP_BLK_GRGB;
	param_data_ptr->cmd = cmd;

	switch (cmd) {
	case ISP_PM_BLK_ISP_SETTING:
		param_data_ptr->data_ptr = (void *)&grgb_ptr->cur;
		param_data_ptr->data_size = sizeof(grgb_ptr->cur);
		*update_flag = 0;
		break;

	case ISP_PM_BLK_GRGB_BYPASS:
		param_data_ptr->data_ptr = (void *)&grgb_ptr->cur.bypass;
		param_data_ptr->data_size = sizeof(grgb_ptr->cur.bypass);
		break;

	default:
		break;
	}

	return rtn;
}
