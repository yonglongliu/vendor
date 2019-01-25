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
#define LOG_TAG "isp_blk_antiflicker"
#include "isp_blocks_cfg.h"

cmr_s32 _pm_antiflicker_init(void *dst_antiflicker_param, void *src_antiflicker_param, void *param1, void *param2)
{
	cmr_s32 rtn = ISP_SUCCESS;
	struct isp_anti_flicker_param *dst_antiflicker_ptr = (struct isp_anti_flicker_param *)dst_antiflicker_param;
	struct anti_flicker_tune_param *src_antiflicker_ptr = (struct anti_flicker_tune_param *)src_antiflicker_param;
	struct isp_pm_block_header *antiflicker_header_ptr = (struct isp_pm_block_header *)param1;
	UNUSED(param2);

	dst_antiflicker_ptr->cur.normal_50hz_thrd = src_antiflicker_ptr->normal_50hz_thrd;
	dst_antiflicker_ptr->cur.lowlight_50hz_thrd = src_antiflicker_ptr->lowlight_50hz_thrd;
	dst_antiflicker_ptr->cur.normal_60hz_thrd = src_antiflicker_ptr->normal_60hz_thrd;
	dst_antiflicker_ptr->cur.lowlight_60hz_thrd = src_antiflicker_ptr->lowlight_60hz_thrd;

	antiflicker_header_ptr->is_update = ISP_ONE;

	return rtn;
}

cmr_s32 _pm_antiflicker_set_param(void *antiflicker_param, cmr_u32 cmd, void *param_ptr0, void *param_ptr1)
{
	cmr_s32 rtn = ISP_SUCCESS;
	struct isp_pm_block_header *antiflicker_header_ptr = (struct isp_pm_block_header *)param_ptr1;

	UNUSED(antiflicker_param);
	UNUSED(param_ptr0);

	antiflicker_header_ptr->is_update = ISP_ONE;

	switch (cmd) {
	case ISP_PM_BLK_ANTI_FLICKER:
		break;
	default:
		antiflicker_header_ptr->is_update = ISP_ZERO;
		break;
	}

	return rtn;
}

cmr_s32 _pm_antiflicker_get_param(void *antiflicker_param, cmr_u32 cmd, void *rtn_param0, void *rtn_param1)
{
	cmr_s32 rtn = ISP_SUCCESS;
	struct isp_pm_param_data *param_data_ptr = (struct isp_pm_param_data *)rtn_param0;
	struct isp_anti_flicker_param *antiflicker_ptr = (struct isp_anti_flicker_param *)antiflicker_param;
	cmr_u32 *update_flag = (cmr_u32 *)rtn_param1;

	UNUSED(rtn_param1);
	param_data_ptr->id = ISP_BLK_ANTI_FLICKER;
	param_data_ptr->cmd = cmd;

	switch (cmd) {
	case ISP_PM_BLK_ISP_SETTING:
		param_data_ptr->data_ptr = &antiflicker_ptr->cur;
		param_data_ptr->data_size = sizeof(antiflicker_ptr->cur);
		*update_flag = 0;
		break;

	default:
		break;
	}

	return rtn;
}
