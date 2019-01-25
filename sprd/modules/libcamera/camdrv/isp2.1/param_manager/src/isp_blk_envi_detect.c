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
#define LOG_TAG "isp_blk_envi_detect"
#include "isp_blocks_cfg.h"

cmr_s32 _pm_envi_detect_init(void *dst_envi_detect_param, void *src_envi_detect_param, void *param1, void *param2)
{
	cmr_s32 rtn = ISP_SUCCESS;
	cmr_s32 i = 0;
	struct isp_envi_detect_param *dst_ptr = (struct isp_envi_detect_param *)dst_envi_detect_param;
	struct sensor_envi_detect_param *src_ptr = (struct sensor_envi_detect_param *)src_envi_detect_param;
	struct isp_pm_block_header *header_ptr = (struct isp_pm_block_header *)param1;
	UNUSED(param2);

	dst_ptr->enable = src_ptr->enable;
	for (i = 0; i < SENSOR_ENVI_NUM; i++) {
		dst_ptr->envi_range[i].min = src_ptr->envi_range[i].min;
		dst_ptr->envi_range[i].max = src_ptr->envi_range[i].max;
	}

	header_ptr->is_update = ISP_ONE;

	return rtn;
}

cmr_s32 _pm_envi_detect_set_param(void *envi_detect_param, cmr_u32 cmd, void *param_ptr0, void *param_ptr1)
{
	cmr_s32 rtn = ISP_SUCCESS;
	cmr_s32 i = 0;
	struct sensor_envi_detect_param *src_envi_detect_ptr = PNULL;
	struct isp_envi_detect_param *dst_envi_detect_ptr = (struct isp_envi_detect_param *)envi_detect_param;
	struct isp_pm_block_header *header_ptr = (struct isp_pm_block_header *)param_ptr1;

	header_ptr->is_update = ISP_ONE;

	switch (cmd) {
	case ISP_PM_BLK_ENVI_DETECT_BYPASS:
		dst_envi_detect_ptr->enable = *((cmr_u32 *) param_ptr0);
		header_ptr->is_update = ISP_ONE;
		break;

	case ISP_PM_BLK_ENVI_DETECT:
		src_envi_detect_ptr = (struct sensor_envi_detect_param *)param_ptr0;
		dst_envi_detect_ptr->enable = src_envi_detect_ptr->enable;
		for (i = 0; i < SENSOR_ENVI_NUM; i++) {
			dst_envi_detect_ptr->envi_range[i].min = src_envi_detect_ptr->envi_range[i].min;
			dst_envi_detect_ptr->envi_range[i].max = src_envi_detect_ptr->envi_range[i].max;
		}
		break;

	default:
		header_ptr->is_update = ISP_ZERO;
		break;
	}

	return rtn;
}

cmr_s32 _pm_envi_detect_get_param(void *envi_detect_param, cmr_u32 cmd, void *rtn_param0, void *rtn_param1)
{
	cmr_s32 rtn = ISP_SUCCESS;
	struct isp_pm_param_data *param_data_ptr = (struct isp_pm_param_data *)rtn_param0;
	struct isp_envi_detect_param *envi_detect_ptr = (struct isp_envi_detect_param *)envi_detect_param;

	UNUSED(rtn_param1);
	param_data_ptr->id = ISP_BLK_ENVI_DETECT;
	param_data_ptr->cmd = cmd;

	switch (cmd) {
	case ISP_PM_BLK_ISP_SETTING:
		param_data_ptr->data_ptr = &envi_detect_ptr;
		param_data_ptr->data_size = sizeof(envi_detect_ptr);
		break;

	default:
		break;
	}

	return rtn;
}
