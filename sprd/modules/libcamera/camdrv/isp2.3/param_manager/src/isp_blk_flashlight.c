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
#define LOG_TAG "isp_blk_flashlight"
#include "isp_blocks_cfg.h"

cmr_s32 _pm_flashlight_init(void *dst_flash_param, void *src_flash_param, void *param1, void *param2)
{
	cmr_s32 rtn = ISP_SUCCESS;
	struct isp_flash_param *dst_flash_ptr = (struct isp_flash_param *)dst_flash_param;
	struct sensor_flash_cali_param *src_flash_ptr = (struct sensor_flash_cali_param *)src_flash_param;
	struct isp_pm_block_header *flashlight_header_ptr = (struct isp_pm_block_header *)param1;
	UNUSED(param2);

	dst_flash_ptr->cur.r_ratio = src_flash_ptr->r_ratio;
	dst_flash_ptr->cur.g_ratio = src_flash_ptr->g_ratio;
	dst_flash_ptr->cur.b_ratio = src_flash_ptr->b_ratio;
	dst_flash_ptr->cur.lum_ratio = src_flash_ptr->lum_ratio;
	dst_flash_ptr->cur.auto_flash_thr = src_flash_ptr->auto_threshold;
	dst_flash_ptr->attrib.global.r_sum = src_flash_ptr->attrib.global.r_sum;
	dst_flash_ptr->attrib.global.gr_sum = src_flash_ptr->attrib.global.gr_sum;
	dst_flash_ptr->attrib.global.gb_sum = src_flash_ptr->attrib.global.gb_sum;
	dst_flash_ptr->attrib.global.b_sum = src_flash_ptr->attrib.global.b_sum;
	dst_flash_ptr->attrib.random.r_sum = src_flash_ptr->attrib.random.r_sum;
	dst_flash_ptr->attrib.random.gr_sum = src_flash_ptr->attrib.random.gr_sum;
	dst_flash_ptr->attrib.random.gb_sum = src_flash_ptr->attrib.random.gb_sum;
	dst_flash_ptr->attrib.random.b_sum = src_flash_ptr->attrib.random.b_sum;
	flashlight_header_ptr->is_update = ISP_ONE;

	return rtn;
}

cmr_s32 _pm_flashlight_set_param(void *flash_param, cmr_u32 cmd, void *param_ptr0, void *param_ptr1)
{
	cmr_s32 rtn = ISP_SUCCESS;
	struct sensor_flash_cali_param *src_flash_ptr = PNULL;
	struct isp_flash_param *dst_flash_ptr = (struct isp_flash_param *)flash_param;
	struct isp_pm_block_header *flash_header_ptr = (struct isp_pm_block_header *)param_ptr1;

	flash_header_ptr->is_update = ISP_ONE;
	switch (cmd) {
	case ISP_PM_BLK_FLASH:
		src_flash_ptr = (struct sensor_flash_cali_param *)param_ptr0;
		dst_flash_ptr->cur.r_ratio = src_flash_ptr->r_ratio;
		dst_flash_ptr->cur.g_ratio = src_flash_ptr->g_ratio;
		dst_flash_ptr->cur.b_ratio = src_flash_ptr->b_ratio;
		dst_flash_ptr->cur.lum_ratio = src_flash_ptr->lum_ratio;

		dst_flash_ptr->attrib.global.r_sum = src_flash_ptr->attrib.global.r_sum;
		dst_flash_ptr->attrib.global.gr_sum = src_flash_ptr->attrib.global.gr_sum;
		dst_flash_ptr->attrib.global.gb_sum = src_flash_ptr->attrib.global.gb_sum;
		dst_flash_ptr->attrib.global.b_sum = src_flash_ptr->attrib.global.b_sum;

		dst_flash_ptr->attrib.random.r_sum = src_flash_ptr->attrib.random.r_sum;
		dst_flash_ptr->attrib.random.gr_sum = src_flash_ptr->attrib.random.gr_sum;
		dst_flash_ptr->attrib.random.gb_sum = src_flash_ptr->attrib.random.gb_sum;
		dst_flash_ptr->attrib.random.b_sum = src_flash_ptr->attrib.random.b_sum;
		break;

	case ISP_PM_BLK_FLASH_RATION_LUM:
		{
			cmr_u32 lum_ratio = *((cmr_u32 *) param_ptr0);
			dst_flash_ptr->cur.lum_ratio = lum_ratio;
		}
		break;

	case ISP_PM_BLK_FLASH_RATION_RGB:
		{
			struct isp_rgb_gains *rgb_ratio = (struct isp_rgb_gains *)param_ptr0;
			dst_flash_ptr->cur.r_ratio = rgb_ratio->gain_r;
			dst_flash_ptr->cur.g_ratio = rgb_ratio->gain_g;
			dst_flash_ptr->cur.b_ratio = rgb_ratio->gain_b;
		}
		break;

	default:
		flash_header_ptr->is_update = ISP_ZERO;
		break;
	}

	return rtn;
}

cmr_s32 _pm_flashlight_get_param(void *flash_param, cmr_u32 cmd, void *rtn_param0, void *rtn_param1)
{
	cmr_s32 rtn = ISP_SUCCESS;
	struct isp_pm_param_data *param_data_ptr = (struct isp_pm_param_data *)rtn_param0;
	struct isp_flash_param *flash_ptr = (struct isp_flash_param *)flash_param;
	cmr_u32 *update_flag = (cmr_u32 *) rtn_param1;

	param_data_ptr->id = ISP_BLK_FLASH_CALI;
	param_data_ptr->cmd = cmd;

	switch (cmd) {
	case ISP_PM_BLK_ISP_SETTING:
		param_data_ptr->data_ptr = &flash_ptr->cur;
		param_data_ptr->data_size = sizeof(flash_ptr->cur);
		*update_flag = ISP_ZERO;
		break;

	default:
		break;
	}

	return rtn;
}
