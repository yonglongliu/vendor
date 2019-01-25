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
#define LOG_TAG "isp_blk_pdaf_extraction"
#include "isp_blocks_cfg.h"

cmr_s32 _pm_pdaf_extraction_init(void *dst_pdaf_extraction_param, void *src_pdaf_extraction_param, void *param1, void *param2)
{
	cmr_s32 rtn = ISP_SUCCESS;
	struct isp_pdaf_extraction_param *dst_ptr = (struct isp_pdaf_extraction_param *)dst_pdaf_extraction_param;
	struct sensor_pdaf_extraction *src_ptr = (struct sensor_pdaf_extraction *)src_pdaf_extraction_param;
	struct isp_pm_block_header *header_ptr = (struct isp_pm_block_header *)param1;
	cmr_s32 i = 0;
	UNUSED(param2);

	dst_ptr->cur.ppi_extractor_bypass = src_ptr->extractor_bypass;
	dst_ptr->cur.ppi_skip_num = src_ptr->skip_num;
	dst_ptr->cur.skip_mode = src_ptr->continuous_mode;

	dst_ptr->cur.ppi_af_win_sy0 = src_ptr->pdaf_af_win.af_win_sy0;
	dst_ptr->cur.ppi_af_win_sx0 = src_ptr->pdaf_af_win.af_win_sx0;
	dst_ptr->cur.ppi_af_win_ey0 = src_ptr->pdaf_af_win.af_win_ey0;
	dst_ptr->cur.ppi_af_win_ex0 = src_ptr->pdaf_af_win.af_win_ex0;
	dst_ptr->cur.ppi_block_start_col = src_ptr->pdaf_region.start_col;
	dst_ptr->cur.ppi_block_start_row = src_ptr->pdaf_region.start_row;
	dst_ptr->cur.ppi_block_end_col = src_ptr->pdaf_region.end_col;
	dst_ptr->cur.ppi_block_end_row = src_ptr->pdaf_region.end_row;
	dst_ptr->cur.ppi_block_width = src_ptr->pdaf_region.width;
	dst_ptr->cur.ppi_block_height = src_ptr->pdaf_region.height;
	for (i = 0; i < 64; i++) {
		dst_ptr->cur.pattern_pos[i] = src_ptr->pdaf_region.pdaf_pattern[i].is_right;
		dst_ptr->cur.pattern_row[i] = src_ptr->pdaf_region.pdaf_pattern[i].pattern_pixel.x;
		dst_ptr->cur.pattern_col[i] = src_ptr->pdaf_region.pdaf_pattern[i].pattern_pixel.y;
	}

	header_ptr->is_update = ISP_ONE;

	return rtn;
}

cmr_s32 _pm_pdaf_extraction_set_param(void *pdaf_extraction_param, cmr_u32 cmd, void *param_ptr0, void *param_ptr1)
{
	cmr_s32 rtn = ISP_SUCCESS;
	struct isp_pdaf_extraction_param *dst_ptr = (struct isp_pdaf_extraction_param *)pdaf_extraction_param;
	struct isp_pm_block_header *header_ptr = (struct isp_pm_block_header *)param_ptr1;

	switch (cmd) {
	case ISP_PM_BLK_PDAF_BYPASS:
		dst_ptr->cur.ppi_extractor_bypass = *((cmr_u32 *) param_ptr0);
		header_ptr->is_update = ISP_ONE;
		break;
	default:
		header_ptr->is_update = ISP_ZERO;
		break;
	}

	return rtn;
}

cmr_s32 _pm_pdaf_extraction_get_param(void *pdaf_extraction_param, cmr_u32 cmd, void *rtn_param0, void *rtn_param1)
{
	cmr_s32 rtn = ISP_SUCCESS;
	struct isp_pdaf_extraction_param *pdaf_extraction_ptr = (struct isp_pdaf_extraction_param *)pdaf_extraction_param;
	struct isp_pm_param_data *param_data_ptr = (struct isp_pm_param_data *)rtn_param0;
	cmr_u32 *update_flag = (cmr_u32 *) rtn_param1;

	param_data_ptr->id = ISP_BLK_PDAF_EXTRACT;
	param_data_ptr->cmd = cmd;

	switch (cmd) {
	case ISP_PM_BLK_ISP_SETTING:
		param_data_ptr->data_ptr = (void *)&pdaf_extraction_ptr->cur;
		param_data_ptr->data_size = sizeof(pdaf_extraction_ptr->cur);
		*update_flag = 0;
		break;

	case ISP_PM_BLK_PDAF_BYPASS:
		param_data_ptr->data_ptr = (void *)&pdaf_extraction_ptr->cur.ppi_extractor_bypass;
		param_data_ptr->data_size = sizeof(pdaf_extraction_ptr->cur.ppi_extractor_bypass);
		break;

	default:
		break;
	}

	return rtn;
}
