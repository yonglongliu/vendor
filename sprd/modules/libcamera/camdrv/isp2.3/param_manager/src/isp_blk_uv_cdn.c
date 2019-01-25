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
#define LOG_TAG "isp_blk_uv_cdn"
#include "isp_blocks_cfg.h"

cmr_u32 _pm_uv_cdn_convert_param(void *dst_cdn_param, cmr_u32 strength_level, cmr_u32 mode_flag, cmr_u32 scene_flag)
{
	cmr_s32 rtn = ISP_SUCCESS;
	cmr_s32 i = 0;
	cmr_u32 total_offset_units = 0;
	struct isp_uv_cdn_param *dst_ptr = (struct isp_uv_cdn_param *)dst_cdn_param;
	struct sensor_uv_cdn_level *uv_cdn_param = PNULL;

	if (SENSOR_MULTI_MODE_FLAG != dst_ptr->nr_mode_setting) {
		uv_cdn_param = (struct sensor_uv_cdn_level *)(dst_ptr->param_ptr);
	} else {
		cmr_u32 *multi_nr_map_ptr = PNULL;
		multi_nr_map_ptr = (cmr_u32 *) dst_ptr->scene_ptr;
		total_offset_units = _pm_calc_nr_addr_offset(mode_flag, scene_flag, multi_nr_map_ptr);
		uv_cdn_param = (struct sensor_uv_cdn_level *)((cmr_u8 *) dst_ptr->param_ptr + total_offset_units * dst_ptr->level_num * sizeof(struct sensor_uv_cdn_level));

	}
	strength_level = PM_CLIP(strength_level, 0, dst_ptr->level_num - 1);
	if (uv_cdn_param != NULL) {
		dst_ptr->cur.median_thru0 = uv_cdn_param[strength_level].median_thru0;
		dst_ptr->cur.median_thru1 = uv_cdn_param[strength_level].median_thru1;
		dst_ptr->cur.median_thrv0 = uv_cdn_param[strength_level].median_thrv0;
		dst_ptr->cur.median_thrv1 = uv_cdn_param[strength_level].median_thrv1;

		for (i = 0; i < 31; i++) {
			dst_ptr->cur.rangewu[i] = uv_cdn_param[strength_level].u_ranweight[i];
			dst_ptr->cur.rangewv[i] = uv_cdn_param[strength_level].v_ranweight[i];
		}

		dst_ptr->cur.gaussian_mode = uv_cdn_param[strength_level].cdn_gaussian_mode;
		dst_ptr->cur.median_mode = uv_cdn_param[strength_level].median_mode;
		dst_ptr->cur.median_writeback_en = uv_cdn_param[strength_level].median_writeback_en;
		dst_ptr->cur.filter_bypass = uv_cdn_param[strength_level].filter_bypass;
		dst_ptr->cur.median_thr = uv_cdn_param[strength_level].median_thr;
		dst_ptr->cur.bypass = uv_cdn_param[strength_level].bypass;
	}
	return rtn;
}

cmr_s32 _pm_uv_cdn_init(void *dst_cdn_param, void *src_cdn_param, void *param1, void *param2)
{
	cmr_s32 rtn = ISP_SUCCESS;
	cmr_u32 i = 0;
	struct isp_pm_block_header *header_ptr = (struct isp_pm_block_header *)param1;
	struct isp_pm_nr_header_param *src_ptr = (struct isp_pm_nr_header_param *)src_cdn_param;
	struct isp_uv_cdn_param *dst_ptr = (struct isp_uv_cdn_param *)dst_cdn_param;
	UNUSED(param2);

	dst_ptr->cur.bypass = header_ptr->bypass;

	dst_ptr->param_ptr = src_ptr->param_ptr;
	dst_ptr->cur.level = src_ptr->default_strength_level;
	dst_ptr->cur_level = src_ptr->default_strength_level;
	dst_ptr->level_num = src_ptr->level_number;
	dst_ptr->scene_ptr = src_ptr->multi_nr_map_ptr;
	dst_ptr->nr_mode_setting = src_ptr->nr_mode_setting;

	rtn = _pm_uv_cdn_convert_param(dst_ptr, dst_ptr->cur_level, ISP_MODE_ID_COMMON, ISP_SCENEMODE_AUTO);
	dst_ptr->cur.bypass |= header_ptr->bypass;
	if (ISP_SUCCESS != rtn) {
		ISP_LOGE("fail to  convert pm uv cdn param!");
		return rtn;
	}

	header_ptr->is_update = ISP_ONE;
	return rtn;
}

cmr_s32 _pm_uv_cdn_set_param(void *cdn_param, cmr_u32 cmd, void *param_ptr0, void *param_ptr1)
{
	cmr_s32 rtn = ISP_SUCCESS;
	struct isp_pm_block_header *header_ptr = (struct isp_pm_block_header *)param_ptr1;
	struct isp_uv_cdn_param *dst_ptr = (struct isp_uv_cdn_param *)cdn_param;

	switch (cmd) {
	case ISP_PM_BLK_UV_CDN_BYPASS_V1:
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

			if (cur_level != dst_ptr->cur_level || nr_tool_flag[11] || block_result->mode_flag_changed) {
				dst_ptr->cur_level = cur_level;
				dst_ptr->cur.level = cur_level;
				header_ptr->is_update = ISP_ONE;
				nr_tool_flag[11] = 0;
				block_result->mode_flag_changed = 0;
				rtn = _pm_uv_cdn_convert_param(dst_ptr, dst_ptr->cur_level, block_result->mode_flag, block_result->scene_flag);
				dst_ptr->cur.bypass |= header_ptr->bypass;
				if (ISP_SUCCESS != rtn) {
					ISP_LOGE("fail to  convert pm uv cdn param!");
					return rtn;
				}
			}
		}
		break;

	default:
		break;
	}

	ISP_LOGV("ISP_SMART_NR: cmd=%d, update=%d, cdn_level=%d", cmd, header_ptr->is_update, dst_ptr->cur_level);

	return rtn;
}

cmr_s32 _pm_uv_cdn_get_param(void *cdn_param, cmr_u32 cmd, void *rtn_param0, void *rtn_param1)
{
	cmr_s32 rtn = ISP_SUCCESS;
	struct isp_uv_cdn_param *cdn_ptr = (struct isp_uv_cdn_param *)cdn_param;
	struct isp_pm_param_data *param_data_ptr = (struct isp_pm_param_data *)rtn_param0;
	cmr_u32 *update_flag = (cmr_u32 *) rtn_param1;

	param_data_ptr->id = ISP_BLK_UV_CDN;
	param_data_ptr->cmd = ISP_PM_BLK_ISP_SETTING;

	switch (cmd) {
	case ISP_PM_BLK_ISP_SETTING:
		param_data_ptr->data_ptr = &cdn_ptr->cur;
		param_data_ptr->data_size = sizeof(cdn_ptr->cur);
		*update_flag = ISP_ZERO;
		break;

	default:
		break;
	}

	return rtn;
}
