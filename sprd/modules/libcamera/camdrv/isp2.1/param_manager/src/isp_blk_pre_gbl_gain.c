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
#define LOG_TAG "isp_blk_pre_gbl_gain"
#include "isp_blocks_cfg.h"

cmr_s32 _pm_pre_gbl_gain_init(void *dst_pre_gbl_gain, void *src_pre_gbl_gain, void *param1, void *param2)
{
	cmr_s32 rtn = ISP_SUCCESS;
	struct sensor_pre_global_gain_param *src_ptr = (struct sensor_pre_global_gain_param *)src_pre_gbl_gain;
	struct isp_pre_global_gain_param *dst_ptr = (struct isp_pre_global_gain_param *)dst_pre_gbl_gain;
	struct isp_pm_block_header *pre_gbl_gain_header_ptr = (struct isp_pm_block_header *)param1;
	UNUSED(param2);

	dst_ptr->cur.bypass = pre_gbl_gain_header_ptr->bypass;

	dst_ptr->cur.gain = src_ptr->gain;

	pre_gbl_gain_header_ptr->is_update = ISP_ONE;

	return rtn;
}

cmr_s32 _pm_pre_gbl_gain_set_param(void *pre_gbl_gain_param, cmr_u32 cmd, void *param_ptr0, void *param_ptr1)
{
	cmr_s32 rtn = ISP_SUCCESS;
	struct isp_pre_global_gain_param *pre_gbl_gain_ptr = (struct isp_pre_global_gain_param *)pre_gbl_gain_param;
	struct isp_pm_block_header *pre_gbl_gain_header_ptr = (struct isp_pm_block_header *)param_ptr1;

	pre_gbl_gain_header_ptr->is_update = ISP_ONE;

	switch (cmd) {
	case ISP_PM_BLK_PRE_GBL_GAIN:
		{
			struct sensor_pre_global_gain_param *pre_gbl_gain_cfg_ptr = (struct sensor_pre_global_gain_param *)param_ptr0;
			pre_gbl_gain_ptr->cur.gain = pre_gbl_gain_cfg_ptr->gain;
		}
		break;

	case ISP_PM_BLK_PRE_GBL_GIAN_BYPASS:
		pre_gbl_gain_ptr->cur.bypass = *((cmr_u32 *) param_ptr0);
		break;

	default:
		pre_gbl_gain_header_ptr->is_update = ISP_ZERO;
		break;
	}

	return rtn;
}

cmr_s32 _pm_pre_gbl_gain_get_param(void *pre_gbl_gain_param, cmr_u32 cmd, void *rtn_param0, void *rtn_param1)
{
	cmr_s32 rtn = ISP_SUCCESS;
	struct isp_pre_global_gain_param *pre_gbl_gain = (struct isp_pre_global_gain_param *)pre_gbl_gain_param;
	struct isp_pm_param_data *param_data_ptr = (struct isp_pm_param_data *)rtn_param0;
	cmr_u32 *update_flag = (cmr_u32 *) rtn_param1;

	param_data_ptr->id = ISP_BLK_PRE_GBL_GAIN;
	param_data_ptr->cmd = cmd;

	switch (cmd) {
	case ISP_PM_BLK_ISP_SETTING:
		param_data_ptr->data_ptr = (void *)&pre_gbl_gain->cur;
		param_data_ptr->data_size = sizeof(pre_gbl_gain->cur);
		*update_flag = 0;
		break;

	case ISP_PM_BLK_PRE_GBL_GIAN_BYPASS:
		param_data_ptr->data_ptr = (void *)&pre_gbl_gain->cur.bypass;
		param_data_ptr->data_size = sizeof(pre_gbl_gain->cur.bypass);
		break;

	default:
		break;
	}

	return rtn;
}
