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
#define LOG_TAG "isp_blk_ynr"
#include "isp_blocks_cfg.h"

cmr_u32 _pm_ynr_convert_param(void *dst_param, cmr_u32 strength_level, cmr_u32 mode_flag, cmr_u32 scene_flag)
{
	cmr_s32 rtn = ISP_SUCCESS;
	cmr_s32 i = 0;
	cmr_u32 total_offset_units = 0;
	struct isp_ynr_param *dst_ptr = (struct isp_ynr_param *)dst_param;
	struct sensor_ynr_level *ynr_param = PNULL;

	if (SENSOR_MULTI_MODE_FLAG != dst_ptr->nr_mode_setting) {
		ynr_param = (struct sensor_ynr_level *)(dst_ptr->param_ptr);
	} else {
		cmr_u32 *multi_nr_map_ptr = PNULL;
		multi_nr_map_ptr = (cmr_u32 *) dst_ptr->scene_ptr;
		total_offset_units = _pm_calc_nr_addr_offset(mode_flag, scene_flag, multi_nr_map_ptr);
		ynr_param = (struct sensor_ynr_level *)((cmr_u8 *) dst_ptr->param_ptr + total_offset_units * dst_ptr->level_num * sizeof(struct sensor_ynr_level));

	}
	strength_level = PM_CLIP(strength_level, 0, dst_ptr->level_num - 1);

	if (ynr_param != NULL) {
		dst_ptr->cur.lowlux_bypass = ynr_param[strength_level].ynr_txt_calc.ynr_lowlux_bypass;
		for (i = 0; i < 7; i++) {
			dst_ptr->cur.flat_th[i] = ynr_param[strength_level].ynr_txt_calc.flat_thr[i];
		}
		for (i = 0; i < 8; i++) {
			dst_ptr->cur.edgeStep[i] = ynr_param[strength_level].ynr_txt_calc.edge_step[i];
		}
		dst_ptr->cur.edge_th = ynr_param[strength_level].ynr_txt_calc.sedge_thr;
		dst_ptr->cur.txt_th = ynr_param[strength_level].ynr_txt_calc.txt_thr;
		dst_ptr->cur.l1_txt_th1 = ynr_param[strength_level].ynr_txt_calc.l1_txt_thr2;
		dst_ptr->cur.l1_txt_th0 = ynr_param[strength_level].ynr_txt_calc.l1_txt_thr1;
		dst_ptr->cur.l0_lut_th1 = ynr_param[strength_level].ynr_txt_calc.l0_lut_thr1;
		dst_ptr->cur.l0_lut_th0 = ynr_param[strength_level].ynr_txt_calc.l0_lut_thr0;

		dst_ptr->cur.nr_enable = ynr_param[strength_level].ynr_den_str.wv_nr_en;
		for (i = 0; i < 3; i++) {
			dst_ptr->cur.wlt_T[i] = ynr_param[strength_level].ynr_den_str.wlt_thr[i];
			dst_ptr->cur.ratio[i] = ynr_param[strength_level].ynr_den_str.wlt_ratio[i];
			dst_ptr->cur.ad_para[i] = ynr_param[strength_level].ynr_den_str.ad_para[i];
		}
		for (i = 0; i < 8; i++) {
			dst_ptr->cur.sal_nr_str[i] = ynr_param[strength_level].ynr_den_str.sal_nr_str[i];
			dst_ptr->cur.sal_offset[i] = ynr_param[strength_level].ynr_den_str.sal_offset[i];
		}
		for (i = 0; i < 7; i++) {
			dst_ptr->cur.lut_th[i] = ynr_param[strength_level].ynr_den_str.lut_thresh[i];
		}
		for (i = 0; i < 9; i++) {
			dst_ptr->cur.sub_th[i] = ynr_param[strength_level].ynr_den_str.subthresh[i];
			dst_ptr->cur.addback[i] = ynr_param[strength_level].ynr_den_str.addback[i];
		}

		dst_ptr->cur.l_blf_en[0] = ynr_param[strength_level].ynr_den_str.layer1.blf_en;
		dst_ptr->cur.l_wf_index[0] = ynr_param[strength_level].ynr_den_str.layer1.wfindex;
		dst_ptr->cur.l_blf_en[1] = ynr_param[strength_level].ynr_den_str.layer2.blf_en;
		dst_ptr->cur.l_wf_index[1] = ynr_param[strength_level].ynr_den_str.layer2.wfindex;
		dst_ptr->cur.l_blf_en[2] = ynr_param[strength_level].ynr_den_str.layer3.blf_en;
		dst_ptr->cur.l_wf_index[2] = ynr_param[strength_level].ynr_den_str.layer3.wfindex;
		for (i = 0; i < 3; i++) {
			dst_ptr->cur.l_euroweight[0][i] = ynr_param[strength_level].ynr_den_str.layer1.eurodist[i];
			dst_ptr->cur.l_euroweight[1][i] = ynr_param[strength_level].ynr_den_str.layer2.eurodist[i];
			dst_ptr->cur.l_euroweight[2][i] = ynr_param[strength_level].ynr_den_str.layer3.eurodist[i];
		}

		dst_ptr->cur.maxRadius = ynr_param[strength_level].ynr_region.max_radius;
		dst_ptr->cur.radius = ynr_param[strength_level].ynr_region.radius;
		dst_ptr->cur.center.x = ynr_param[strength_level].ynr_region.imgcetx;
		dst_ptr->cur.center.y = ynr_param[strength_level].ynr_region.imgcety;
		for (i = 0; i < 24; i++) {
			dst_ptr->cur.freq_ratio[i] = ynr_param[strength_level].freqratio[i];
			dst_ptr->cur.wlt_th[i] = ynr_param[strength_level].wltT[i];
		}
		dst_ptr->cur.dist_interval = ynr_param[strength_level].ynr_region.dist_interval;
	}
	return rtn;
}

cmr_s32 _pm_ynr_init(void *dst_ynr_param, void *src_ynr_param, void *param1, void *param2)
{
	cmr_s32 rtn = ISP_SUCCESS;
	cmr_s32 i = 0;
	struct isp_ynr_param *dst_ptr = (struct isp_ynr_param *)dst_ynr_param;
	struct isp_pm_nr_header_param *src_ptr = (struct isp_pm_nr_header_param *)src_ynr_param;
	struct isp_pm_block_header *ynr_header_ptr = (struct isp_pm_block_header *)param1;
	UNUSED(param2);

	dst_ptr->cur.bypass = ynr_header_ptr->bypass;

	dst_ptr->cur_level = src_ptr->default_strength_level;
	dst_ptr->level_num = src_ptr->level_number;
	dst_ptr->param_ptr = src_ptr->param_ptr;
	dst_ptr->scene_ptr = src_ptr->multi_nr_map_ptr;
	dst_ptr->nr_mode_setting = src_ptr->nr_mode_setting;

	rtn = _pm_ynr_convert_param(dst_ptr, dst_ptr->cur_level, ISP_MODE_ID_COMMON, ISP_SCENEMODE_AUTO);
	dst_ptr->cur.bypass |= ynr_header_ptr->bypass;

	if (ISP_SUCCESS != rtn) {
		ISP_LOGE("fail to  convert pm ynr param!");
		return rtn;
	}

	ynr_header_ptr->is_update = ISP_ONE;

	return rtn;
}

cmr_s32 _pm_ynr_set_param(void *ynr_param, cmr_u32 cmd, void *param_ptr0, void *param_ptr1)
{
	cmr_s32 rtn = ISP_SUCCESS;
	struct isp_ynr_param *dst_ptr = (struct isp_ynr_param *)ynr_param;
	struct isp_pm_block_header *header_ptr = (struct isp_pm_block_header *)param_ptr1;

	switch (cmd) {
	case ISP_PM_BLK_YNR_BYPASS:
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
			if (cur_level != dst_ptr->cur_level || nr_tool_flag[14] || block_result->mode_flag_changed) {
				dst_ptr->cur_level = cur_level;
				header_ptr->is_update = ISP_ONE;
				nr_tool_flag[14] = 0;
				block_result->mode_flag_changed = 0;
				rtn = _pm_ynr_convert_param(dst_ptr, cur_level, block_result->mode_flag, block_result->scene_flag);
				dst_ptr->cur.bypass |= header_ptr->bypass;
				if (ISP_SUCCESS != rtn) {
					ISP_LOGE("fail to  convert pm ynr param!");
					return rtn;
				}
			}
		}
		break;

	default:

		break;
	}

	ISP_LOGV("ISP_SMART_NR: cmd = %d, is_update = %d, ynr_level=%d", cmd, header_ptr->is_update, dst_ptr->cur_level);

	return rtn;
}

cmr_s32 _pm_ynr_get_param(void *ynr_param, cmr_u32 cmd, void *rtn_param0, void *rtn_param1)
{
	cmr_s32 rtn = ISP_SUCCESS;
	struct isp_ynr_param *ynr_ptr = (struct isp_ynr_param *)ynr_param;
	struct isp_pm_param_data *param_data_ptr = (struct isp_pm_param_data *)rtn_param0;
	cmr_u32 *update_flag = (cmr_u32 *) rtn_param1;

	param_data_ptr->id = ISP_BLK_YNR;
	param_data_ptr->cmd = cmd;

	switch (cmd) {
	case ISP_PM_BLK_ISP_SETTING:
		param_data_ptr->data_ptr = (void *)&ynr_ptr->cur;
		param_data_ptr->data_size = sizeof(ynr_ptr->cur);
		*update_flag = 0;
		break;

	case ISP_PM_BLK_YNR_BYPASS:
		param_data_ptr->data_ptr = (void *)&ynr_ptr->cur.bypass;
		param_data_ptr->data_size = sizeof(ynr_ptr->cur.bypass);
		break;

	default:
		break;
	}

	return rtn;
}
