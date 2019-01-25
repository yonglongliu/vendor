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
#define LOG_TAG "isp_blk_brightness"
#include "isp_blocks_cfg.h"

cmr_s32 _pm_brightness_init(void *dst_brightness, void *src_brightness, void *param1, void *param2)
{
	cmr_s32 rtn = ISP_SUCCESS;
	struct sensor_bright_param *src_ptr = (struct sensor_bright_param *)src_brightness;
	struct isp_bright_param *dst_ptr = (struct isp_bright_param *)dst_brightness;
	struct isp_pm_block_header *bright_header_ptr = (struct isp_pm_block_header *)param1;
	UNUSED(param2);

	dst_ptr->cur_index = src_ptr->cur_index;
	dst_ptr->cur.bypass = bright_header_ptr->bypass;

	dst_ptr->cur.factor = src_ptr->factor[src_ptr->cur_index];
	memcpy((void *)dst_ptr->bright_tab, (void *)src_ptr->factor, sizeof(dst_ptr->bright_tab));
	memcpy((void *)dst_ptr->scene_mode_tab, (void *)src_ptr->scenemode, sizeof(dst_ptr->scene_mode_tab));
	bright_header_ptr->is_update = 1;

	return rtn;
}

cmr_s32 _pm_brightness_set_param(void *bright_param, cmr_u32 cmd, void *param_ptr0, void *param_ptr1)
{
	cmr_s32 rtn = ISP_SUCCESS;
	struct isp_bright_param *bright_ptr = (struct isp_bright_param *)bright_param;
	struct isp_pm_block_header *bright_header_ptr = (struct isp_pm_block_header *)param_ptr1;

	bright_header_ptr->is_update = ISP_ONE;

	switch (cmd) {
	case ISP_PM_BLK_BRIGHT_BYPASS:
		bright_ptr->cur.bypass = *((cmr_u32 *) param_ptr0);
		break;

	case ISP_PM_BLK_BRIGHT:
		bright_ptr->cur_index = *((cmr_u32 *) param_ptr0);
		bright_ptr->cur.factor = bright_ptr->bright_tab[bright_ptr->cur_index];
		break;

	case ISP_PM_BLK_SCENE_MODE:
		{
			cmr_u32 idx = *((cmr_u32 *) param_ptr0);
			if (0 == idx) {
				bright_ptr->cur.factor = bright_ptr->bright_tab[bright_ptr->cur_index];
			} else {
				bright_ptr->cur.factor = bright_ptr->scene_mode_tab[idx];
			}
		}
		break;

	default:
		bright_header_ptr->is_update = ISP_ZERO;
		break;
	}

	return rtn;
}

cmr_s32 _pm_brightness_get_param(void *bright_param, cmr_u32 cmd, void *rtn_param0, void *rtn_param1)
{
	cmr_s32 rtn = ISP_SUCCESS;
	struct isp_bright_param *bright_ptr = (struct isp_bright_param *)bright_param;
	struct isp_pm_param_data *param_data_ptr = (struct isp_pm_param_data *)rtn_param0;
	cmr_u32 *update_flag = (cmr_u32 *) rtn_param1;

	param_data_ptr->cmd = cmd;
	param_data_ptr->id = ISP_BLK_BRIGHT;

	switch (cmd) {
	case ISP_PM_BLK_ISP_SETTING:
		param_data_ptr->data_ptr = (void *)&bright_ptr->cur;
		param_data_ptr->data_size = sizeof(bright_ptr->cur);
		*update_flag = 0;
		break;

	case ISP_PM_BLK_BRIGHT_BYPASS:
		param_data_ptr->data_ptr = (void *)&bright_ptr->cur.bypass;
		param_data_ptr->data_size = sizeof(bright_ptr->cur.bypass);
		break;

	default:
		break;
	}

	return rtn;
}
