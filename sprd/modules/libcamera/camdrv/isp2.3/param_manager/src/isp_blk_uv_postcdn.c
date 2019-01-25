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
#define LOG_TAG "isp_blk_uv_postcdn"
#include "isp_blocks_cfg.h"

cmr_u32 _pm_uv_postcdn_convert_param(void *dst_postcdn_param, cmr_u32 strength_level, cmr_u32 mode_flag, cmr_u32 scene_flag)
{
	cmr_s32 rtn = ISP_SUCCESS;
	cmr_s32 i = 0, j = 0;
	cmr_u32 total_offset_units = 0;
	struct isp_uv_postcdn_param *dst_ptr = (struct isp_uv_postcdn_param *)dst_postcdn_param;
	struct sensor_uv_postcdn_level *postcdn_param = PNULL;

	if (SENSOR_MULTI_MODE_FLAG != dst_ptr->nr_mode_setting) {
		postcdn_param = (struct sensor_uv_postcdn_level *)(dst_ptr->param_ptr);
	} else {
		cmr_u32 *multi_nr_map_ptr = PNULL;
		multi_nr_map_ptr = (cmr_u32 *) dst_ptr->scene_ptr;
		total_offset_units = _pm_calc_nr_addr_offset(mode_flag, scene_flag, multi_nr_map_ptr);
		postcdn_param = (struct sensor_uv_postcdn_level *)((cmr_u8 *) dst_ptr->param_ptr +
								   total_offset_units * dst_ptr->level_num * sizeof(struct sensor_uv_postcdn_level));
	}
	strength_level = PM_CLIP(strength_level, 0, dst_ptr->level_num - 1);

	if (postcdn_param != NULL) {
		for (i = 0; i < 7; i++) {
			dst_ptr->cur.r_segu[0][i] = postcdn_param[strength_level].r_segu.r_seg0[i];
			dst_ptr->cur.r_segu[1][i] = postcdn_param[strength_level].r_segu.r_seg1[i];
			dst_ptr->cur.r_segv[0][i] = postcdn_param[strength_level].r_segv.r_seg0[i];
			dst_ptr->cur.r_segv[1][i] = postcdn_param[strength_level].r_segv.r_seg1[i];
		}

		for (i = 0; i < 15; i++) {
			for (j = 0; j < 5; j++)
				dst_ptr->cur.r_distw[i][j] = postcdn_param[strength_level].r_distw.distw[i][j];
		}

		dst_ptr->cur.downsample_bypass = postcdn_param[strength_level].downsample_bypass;
		dst_ptr->cur.mode = postcdn_param[strength_level].postcdn_mode;
		dst_ptr->cur.writeback_en = postcdn_param[strength_level].median_res_wb_en;
		dst_ptr->cur.uvjoint = postcdn_param[strength_level].uv_joint;
		dst_ptr->cur.median_mode = postcdn_param[strength_level].median_mode;
		dst_ptr->cur.adapt_med_thr = postcdn_param[strength_level].adpt_med_thr;

		dst_ptr->cur.uvthr0 = postcdn_param[strength_level].uv_thr.thr0;
		dst_ptr->cur.uvthr1 = postcdn_param[strength_level].uv_thr.thr1;
		dst_ptr->cur.thr_uv.thru0 = postcdn_param[strength_level].u_thr.thr0;
		dst_ptr->cur.thr_uv.thru1 = postcdn_param[strength_level].u_thr.thr1;
		dst_ptr->cur.thr_uv.thrv0 = postcdn_param[strength_level].v_thr.thr0;
		dst_ptr->cur.thr_uv.thrv1 = postcdn_param[strength_level].v_thr.thr1;
		dst_ptr->cur.bypass = postcdn_param[strength_level].bypass;
	}
	return rtn;
}

cmr_s32 _pm_uv_postcdn_init(void *dst_postcdn_param, void *src_postcdn_param, void *param1, void *param_ptr2)
{
	cmr_s32 rtn = ISP_SUCCESS;
	struct isp_uv_postcdn_param *dst_ptr = (struct isp_uv_postcdn_param *)dst_postcdn_param;
	struct isp_pm_nr_header_param *src_ptr = (struct isp_pm_nr_header_param *)src_postcdn_param;
	struct isp_pm_block_header *header_ptr = (struct isp_pm_block_header *)param1;
	cmr_s32 i = 0, j = 0;
	UNUSED(param_ptr2);

	dst_ptr->cur.bypass = header_ptr->bypass;

	dst_ptr->param_ptr = src_ptr->param_ptr;
	dst_ptr->cur_level = src_ptr->default_strength_level;
	dst_ptr->level_num = src_ptr->level_number;
	dst_ptr->scene_ptr = src_ptr->multi_nr_map_ptr;
	dst_ptr->nr_mode_setting = src_ptr->nr_mode_setting;

	rtn = _pm_uv_postcdn_convert_param(dst_ptr, dst_ptr->cur_level, ISP_MODE_ID_COMMON, ISP_SCENEMODE_AUTO);
	dst_ptr->cur.bypass |= header_ptr->bypass;
	if (ISP_SUCCESS != rtn) {
		ISP_LOGE("fail to  convert pm uv postcdn param!");
		return rtn;
	}

	header_ptr->is_update = ISP_ONE;

	return rtn;
}

cmr_s32 _pm_uv_postcdn_set_param(void *postcdn_param, cmr_u32 cmd, void *param_ptr0, void *param_ptr1)
{
	cmr_s32 rtn = ISP_SUCCESS;
	struct isp_uv_postcdn_param *dst_ptr = (struct isp_uv_postcdn_param *)postcdn_param;
	struct isp_pm_block_header *header_ptr = (struct isp_pm_block_header *)param_ptr1;

	switch (cmd) {
	case ISP_PM_BLK_UV_POST_CDN_BYPASS:
		dst_ptr->cur.bypass = *((cmr_u32 *) param_ptr0);
		header_ptr->is_update = ISP_ONE;
		break;

	case ISP_PM_BLK_UV_POST_CDN_STRENGTH_LEVEL:
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

			if (cur_level != dst_ptr->cur_level || nr_tool_flag[13] || block_result->mode_flag_changed) {
				dst_ptr->cur_level = cur_level;
				header_ptr->is_update = ISP_ONE;
				nr_tool_flag[13] = 0;
				block_result->mode_flag_changed = 0;
				rtn = _pm_uv_postcdn_convert_param(dst_ptr, dst_ptr->cur_level, block_result->mode_flag, block_result->scene_flag);
				dst_ptr->cur.bypass |= header_ptr->bypass;
				if (ISP_SUCCESS != rtn) {
					ISP_LOGE("fail to  convert pm uv postcdn param!");
					return rtn;
				}
			}
		}
		break;

	default:

		break;
	}

	ISP_LOGV("ISP_SMART_NR: cmd=%d, update=%d, postcdn_level=%d", cmd, header_ptr->is_update, dst_ptr->cur_level);

	return rtn;

}

cmr_s32 _pm_uv_postcdn_get_param(void *postcdn_param, cmr_u32 cmd, void *rtn_param0, void *rtn_param1)
{
	cmr_s32 rtn = ISP_SUCCESS;
	struct isp_uv_postcdn_param *postcdn_ptr = (struct isp_uv_postcdn_param *)postcdn_param;
	struct isp_pm_param_data *param_data_ptr = (struct isp_pm_param_data *)rtn_param0;
	cmr_u32 *update_flag = (cmr_u32 *) rtn_param1;

	param_data_ptr->id = ISP_BLK_UV_POSTCDN;
	param_data_ptr->cmd = cmd;

	switch (cmd) {
	case ISP_PM_BLK_ISP_SETTING:
		param_data_ptr->data_ptr = (void *)&postcdn_ptr->cur;
		param_data_ptr->data_size = sizeof(postcdn_ptr->cur);
		*update_flag = 0;
		break;

	default:
		break;
	}

	return rtn;
}
