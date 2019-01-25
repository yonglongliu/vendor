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
#define LOG_TAG "isp_blk_bpc"
#include "isp_blocks_cfg.h"

cmr_u32 _pm_bpc_convert_param(void *dst_param, cmr_u32 strength_level, cmr_u32 mode_flag, cmr_u32 scene_flag)
{
	cmr_s32 rtn = ISP_SUCCESS;
	cmr_s32 i = 0;
	cmr_u32 total_offset_units = 0;
	struct isp_bpc_param *dst_ptr = (struct isp_bpc_param *)dst_param;
	struct sensor_bpc_level *bpc_param;

	if (SENSOR_MULTI_MODE_FLAG != dst_ptr->nr_mode_setting) {
		bpc_param = (struct sensor_bpc_level *)(dst_ptr->param_ptr);
	} else {
		cmr_u32 *multi_nr_map_ptr = PNULL;
		multi_nr_map_ptr = (cmr_u32 *) dst_ptr->scene_ptr;
		total_offset_units = _pm_calc_nr_addr_offset(mode_flag, scene_flag, multi_nr_map_ptr);
		bpc_param = (struct sensor_bpc_level *)((cmr_u8 *) dst_ptr->param_ptr + total_offset_units * dst_ptr->level_num * sizeof(struct sensor_bpc_level));
	}
	strength_level = PM_CLIP(strength_level, 0, dst_ptr->level_num - 1);

	if (bpc_param != NULL) {
		dst_ptr->cur.bad_pixel_pos_out_en = bpc_param[strength_level].bpc_pos.pos_out_en;
		dst_ptr->cur.bpc_mode = bpc_param[strength_level].bpc_comm.bpc_mode;
		dst_ptr->cur.edge_hv_mode = bpc_param[strength_level].bpc_comm.hv_mode;
		dst_ptr->cur.edge_rd_mode = bpc_param[strength_level].bpc_comm.rd_mode;
		for (i = 0; i < 8; i++) {
			dst_ptr->cur.lut_level[i] = bpc_param[strength_level].bpc_comm.lut_level[i];
			dst_ptr->cur.slope_k[i] = bpc_param[strength_level].bpc_comm.slope_k[i];
			dst_ptr->cur.intercept_b[i] = bpc_param[strength_level].bpc_comm.intercept_b[i];
		}
		for (i = 0; i < 4; i++) {
			dst_ptr->cur.double_badpixel_th[i] = bpc_param[strength_level].bpc_thr.double_th[i];
			dst_ptr->cur.three_badpixel_th[i] = bpc_param[strength_level].bpc_thr.three_th[i];
			dst_ptr->cur.four_badpixel_th[i] = bpc_param[strength_level].bpc_thr.four_th[i];
		}
		dst_ptr->cur.texture_th = bpc_param[strength_level].bpc_thr.texture_th;
		dst_ptr->cur.flat_th = bpc_param[strength_level].bpc_thr.flat_th;
		for (i = 0; i < 3; i++) {
			dst_ptr->cur.shift[i] = bpc_param[strength_level].bpc_thr.shift[i];
		}
		dst_ptr->cur.edge_ratio_hv = bpc_param[strength_level].bpc_rules.hv_ratio;
		dst_ptr->cur.edge_ratio_rd = bpc_param[strength_level].bpc_rules.rd_ration;
		dst_ptr->cur.high_offset = bpc_param[strength_level].bpc_rules.highoffset;
		dst_ptr->cur.low_offset = bpc_param[strength_level].bpc_rules.lowoffset;
		dst_ptr->cur.high_coeff = bpc_param[strength_level].bpc_rules.highcoeff;
		dst_ptr->cur.low_coeff = bpc_param[strength_level].bpc_rules.lowcoeff;
		dst_ptr->cur.min_coeff = bpc_param[strength_level].bpc_rules.k_val.min;
		dst_ptr->cur.max_coeff = bpc_param[strength_level].bpc_rules.k_val.max;

		dst_ptr->cur.map_addr = 0x00;
		dst_ptr->cur.bad_pixel_pos_out_addr = 0x00;
		dst_ptr->cur.bad_pixel_pos_fifo_clr = 0x00;
		dst_ptr->cur.bad_map_hw_fifo_clr_en = 0x00;
		dst_ptr->cur.bpc_map_fifo_clr = 0x00;
		dst_ptr->cur.bad_pixel_num = 0x00;
	}
	return rtn;
}

cmr_s32 _pm_bpc_init(void *dst_bpc_param, void *src_bpc_param, void *param1, void *param2)
{
	cmr_s32 rtn = ISP_SUCCESS;
	cmr_u32 i = 0x00;
	struct isp_pm_nr_header_param *src_ptr = (struct isp_pm_nr_header_param *)src_bpc_param;
	struct isp_bpc_param *dst_ptr = (struct isp_bpc_param *)dst_bpc_param;
	struct isp_pm_block_header *bpc_header_ptr = (struct isp_pm_block_header *)param1;
	UNUSED(param2);

	dst_ptr->cur.bypass = bpc_header_ptr->bypass;
	dst_ptr->cur_level = src_ptr->default_strength_level;
	dst_ptr->level_num = src_ptr->level_number;
	dst_ptr->param_ptr = src_ptr->param_ptr;
	dst_ptr->scene_ptr = src_ptr->multi_nr_map_ptr;
	dst_ptr->nr_mode_setting = src_ptr->nr_mode_setting;

	rtn = _pm_bpc_convert_param(dst_ptr, dst_ptr->cur_level, ISP_MODE_ID_COMMON, ISP_SCENEMODE_AUTO);
	dst_ptr->cur.bypass |= bpc_header_ptr->bypass;
	if (ISP_SUCCESS != rtn) {
		ISP_LOGE("fail to convert pm bpc param !");
		return rtn;
	}

	bpc_header_ptr->is_update = ISP_ONE;

	return rtn;
}

cmr_s32 _pm_bpc_set_param(void *bpc_param, cmr_u32 cmd, void *param_ptr0, void *param_ptr1)
{
	cmr_s32 rtn = ISP_SUCCESS;
	struct isp_bpc_param *dst_ptr = (struct isp_bpc_param *)bpc_param;
	struct isp_pm_block_header *header_ptr = (struct isp_pm_block_header *)param_ptr1;

	switch (cmd) {
	case ISP_PM_BLK_BPC:
		//TODO:
		break;

	case ISP_PM_BLK_BPC_BYPASS:
		dst_ptr->cur.bypass = *((cmr_u32 *) param_ptr0);
		header_ptr->is_update = ISP_ONE;
		break;

	case ISP_PM_BLK_BPC_MODE:
		{
			cmr_u32 mode = *((cmr_u32 *) param_ptr0);
			dst_ptr->cur.bpc_mode = mode;
			header_ptr->is_update = ISP_ONE;
		}
		break;

	case ISP_PM_BLK_BPC_THRD:
		//TODO:
		break;

	case ISP_PM_BLK_BPC_MAP_ADDR:
		{
			/*need caller this enum to set this value. */
			//      intptr_t map_addr = *(intptr_t*)param_ptr0;
			//      bpc_ptr->cur.bpc_map_addr_new = map_addr;
			header_ptr->is_update = ISP_ONE;
		}
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
				ISP_LOGE("fail to check pm  smart param!");
				return rtn;
			}

			cur_level = (cmr_u32) block_result->component[0].fix_data[0];

			if (cur_level != dst_ptr->cur_level || nr_tool_flag[2] || block_result->mode_flag_changed) {
				dst_ptr->cur_level = cur_level;
				header_ptr->is_update = ISP_ONE;
				nr_tool_flag[2] = 0;
				block_result->mode_flag_changed = 0;
				rtn = _pm_bpc_convert_param(dst_ptr, dst_ptr->cur_level, block_result->mode_flag, block_result->scene_flag);
				dst_ptr->cur.bypass |= header_ptr->bypass;

				if (ISP_SUCCESS != rtn) {
					ISP_LOGE("fail to convert pm bpc param !");
					return rtn;
				}
			}
		}
		break;

	default:

		break;
	}

	ISP_LOGV("ISP_SMART: cmd=%d, update=%d, cur_level=%d", cmd, header_ptr->is_update, dst_ptr->cur_level);

	return rtn;
}

cmr_s32 _pm_bpc_get_param(void *bpc_param, cmr_u32 cmd, void *rtn_param0, void *rtn_param1)
{
	cmr_s32 rtn = ISP_SUCCESS;
	struct isp_bpc_param *bpc_ptr = (struct isp_bpc_param *)bpc_param;
	struct isp_pm_param_data *param_data_ptr = (struct isp_pm_param_data *)rtn_param0;
	cmr_u32 *update_flag = (cmr_u32 *) rtn_param1;

	param_data_ptr->id = ISP_BLK_BPC;
	param_data_ptr->cmd = cmd;

	switch (cmd) {
	case ISP_PM_BLK_ISP_SETTING:
		param_data_ptr->data_ptr = (void *)&bpc_ptr->cur;
		param_data_ptr->data_size = sizeof(bpc_ptr->cur);
		*update_flag = 0;
		break;

	case ISP_PM_BLK_BPC_BYPASS:
		param_data_ptr->data_ptr = (void *)&bpc_ptr->cur.bypass;
		param_data_ptr->data_size = sizeof(bpc_ptr->cur.bypass);
		break;

	default:
		break;
	}

	return rtn;
}
