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
#define LOG_TAG "isp_blk_rgb_gain"
#include "isp_blocks_cfg.h"

cmr_s32 _pm_rgb_gain_init(void *dst_gbl_gain, void *src_gbl_gain, void *param1, void *param2)
{
	cmr_s32 rtn = ISP_SUCCESS;
	struct sensor_rgb_gain_param *src_ptr = (struct sensor_rgb_gain_param *)src_gbl_gain;
	struct isp_rgb_gain_param *dst_ptr = (struct isp_rgb_gain_param *)dst_gbl_gain;
	struct isp_pm_block_header *header_ptr = (struct isp_pm_block_header *)param1;
	UNUSED(param2);

	dst_ptr->cur.bypass = header_ptr->bypass;
	dst_ptr->cur.global_gain = src_ptr->glb_gain;
	dst_ptr->cur.r_gain = src_ptr->r_gain;
	dst_ptr->cur.b_gain = src_ptr->b_gain;
	dst_ptr->cur.g_gain = src_ptr->g_gain;
	header_ptr->is_update = ISP_ONE;
	return rtn;
}

cmr_s32 _pm_rgb_gain_set_param(void *gbl_gain_param, cmr_u32 cmd, void *param_ptr0, void *param_ptr1)
{
	cmr_s32 rtn = ISP_SUCCESS;
	struct isp_rgb_gain_param *rgb_gain_ptr = (struct isp_rgb_gain_param *)gbl_gain_param;
	struct isp_pm_block_header *rgb_gain_header_ptr = (struct isp_pm_block_header *)param_ptr1;

	rgb_gain_header_ptr->is_update = ISP_ONE;

	switch (cmd) {
	case ISP_PM_BLK_GBL_GAIN_BYPASS:
		rgb_gain_ptr->cur.bypass = *((cmr_u32 *) param_ptr0);
		break;

	case ISP_PM_BLK_GBL_GAIN:
		rgb_gain_ptr->cur.global_gain = *((cmr_u32 *) param_ptr0);
		break;

	default:
		rgb_gain_header_ptr->is_update = ISP_ZERO;
		break;
	}

	return rtn;
}

cmr_s32 _pm_rgb_gain_get_param(void *gbl_gain_param, cmr_u32 cmd, void *rtn_param0, void *rtn_param1)
{
	cmr_s32 rtn = ISP_SUCCESS;
	struct isp_rgb_gain_param *gbl_gain = (struct isp_rgb_gain_param *)gbl_gain_param;
	struct isp_pm_param_data *param_data_ptr = (struct isp_pm_param_data *)rtn_param0;
	cmr_u32 *update_flag = (cmr_u32 *) rtn_param1;

	param_data_ptr->id = ISP_BLK_RGB_GAIN;
	param_data_ptr->cmd = cmd;

	switch (cmd) {
	case ISP_PM_BLK_ISP_SETTING:
		param_data_ptr->data_ptr = (void *)&gbl_gain->cur;
		param_data_ptr->data_size = sizeof(gbl_gain->cur);
		*update_flag = 0;
		break;

	case ISP_PM_BLK_GBL_GAIN_BYPASS:
		param_data_ptr->data_ptr = (void *)&(gbl_gain->cur.bypass);
		param_data_ptr->data_size = sizeof(gbl_gain->cur.bypass);
		break;

	default:
		break;
	}

	return rtn;
}
