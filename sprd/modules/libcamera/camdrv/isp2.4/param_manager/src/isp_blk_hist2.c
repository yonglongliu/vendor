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
#define LOG_TAG "isp_blk_hist2"
#include "isp_blocks_cfg.h"

cmr_s32 _pm_hist2_init(void *dst_hist2_param, void *src_hist2_param, void *param1, void *param2)
{
	cmr_s32 rtn = ISP_SUCCESS;
	cmr_u32 i = 0;
	struct isp_pm_block_header *header_ptr = (struct isp_pm_block_header *)param1;
	struct sensor_yuv_hists2_param *src_ptr = (struct sensor_yuv_hists2_param *)src_hist2_param;
	struct isp_hist2_param *dst_ptr = (struct isp_hist2_param *)dst_hist2_param;
	UNUSED(param2);

	dst_ptr->cur.bypass = header_ptr->bypass;
	dst_ptr->cur.skip_num = src_ptr->skip_num;
	dst_ptr->cur.en = 0x0;
	dst_ptr->cur.skip_num_clr = 0x0;
	dst_ptr->cur.mode = 0x1;
	for (i = 0; i < SENSOR_HIST2_ROI_NUM; i++) {
		dst_ptr->cur.hist_roi_x_s[i] = src_ptr->roi_point[i].start_x;
		dst_ptr->cur.hist_roi_x_e[i] = src_ptr->roi_point[i].end_x;
		dst_ptr->cur.hist_roi_y_s[i] = src_ptr->roi_point[i].start_y;
		dst_ptr->cur.hist_roi_y_e[i] = src_ptr->roi_point[i].end_y;
	}
	header_ptr->is_update = ISP_ONE;

	return rtn;
}

cmr_s32 _pm_hist2_set_param(void *hist2_param, cmr_u32 cmd, void *param_ptr0, void *param_ptr1)
{
	cmr_s32 rtn = ISP_SUCCESS;
	struct isp_pm_block_header *hist2_header_ptr = (struct isp_pm_block_header *)param_ptr1;
	struct isp_hist2_param *hist2_ptr = (struct isp_hist2_param *)hist2_param;

	hist2_header_ptr->is_update = ISP_ONE;

	switch (cmd) {
	case ISP_PM_BLK_HIST2_BYPASS:
		hist2_ptr->cur.bypass = *((cmr_u32 *) param_ptr0);
		break;

	default:
		hist2_header_ptr->is_update = ISP_ZERO;
		break;
	}

	return rtn;
}

cmr_s32 _pm_hist2_get_param(void *hist2_param, cmr_u32 cmd, void *rtn_param0, void *rtn_param1)
{
	cmr_s32 rtn = ISP_SUCCESS;
	struct isp_hist2_param *hist2_ptr = (struct isp_hist2_param *)hist2_param;
	struct isp_pm_param_data *param_data_ptr = (struct isp_pm_param_data *)rtn_param0;
	cmr_u32 *update_flag = (cmr_u32 *) rtn_param1;

	param_data_ptr->id = ISP_BLK_HIST2;
	param_data_ptr->cmd = cmd;

	switch (cmd) {
	case ISP_PM_BLK_ISP_SETTING:
		param_data_ptr->data_ptr = &hist2_ptr->cur;
		param_data_ptr->data_size = sizeof(hist2_ptr->cur);
		*update_flag = ISP_ZERO;
		break;

	default:
		break;
	}

	return rtn;
}
