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
#define LOG_TAG "isp_blk_yrandom"
#include "isp_blocks_cfg.h"

cmr_s32 _pm_iircnr_yrandom_init(void *dst_iircnr_param, void *src_iircnr_param, void *param1, void *param_ptr2)
{
	cmr_s32 rtn = ISP_SUCCESS;
	cmr_u32 i = 0;
	struct isp_iircnr_yrandom_param *dst_ptr = (struct isp_iircnr_yrandom_param *)dst_iircnr_param;
	struct sensor_iircnr_yrandom_param *src_ptr = (struct sensor_iircnr_yrandom_param *)src_iircnr_param;
	struct isp_pm_block_header *header_ptr = (struct isp_pm_block_header *)param1;
	UNUSED(param_ptr2);

	dst_ptr->cur.bypass = header_ptr->bypass;

	dst_ptr->cur.seed = src_ptr->iircnr_yrandom_level.yrandom_seed;
	dst_ptr->cur.offset = src_ptr->iircnr_yrandom_level.yrandom_offset;
	dst_ptr->cur.shift = src_ptr->iircnr_yrandom_level.yrandom_shift;
	for (i = 0; i < 8; i++) {
		dst_ptr->cur.takeBit[i] = src_ptr->iircnr_yrandom_level.yrandom_takebit[i];
	}
	header_ptr->is_update = ISP_ONE;

	return rtn;
}

cmr_s32 _pm_iircnr_yrandom_set_param(void *iircnr_param, cmr_u32 cmd, void *param_ptr0, void *param_ptr1)
{
	cmr_s32 rtn = ISP_SUCCESS;

	struct isp_iircnr_yrandom_param *dst_ptr = (struct isp_iircnr_yrandom_param *)iircnr_param;
	struct isp_pm_block_header *header_ptr = (struct isp_pm_block_header *)param_ptr1;
	UNUSED(cmd);

	header_ptr->is_update = ISP_ONE;
	dst_ptr->cur.bypass = *((cmr_u32 *) param_ptr0);

	return rtn;
}

cmr_s32 _pm_iircnr_yrandom_get_param(void *iircnr_param, cmr_u32 cmd, void *rtn_param0, void *rtn_param1)
{
	cmr_s32 rtn = ISP_SUCCESS;
	struct isp_iircnr_yrandom_param *iircnr_ptr = (struct isp_iircnr_yrandom_param *)iircnr_param;
	struct isp_pm_param_data *param_data_ptr = (struct isp_pm_param_data *)rtn_param0;
	cmr_u32 *update_flag = (cmr_u32 *) rtn_param1;

	param_data_ptr->id = ISP_BLK_IIRCNR_YRANDOM;
	param_data_ptr->cmd = cmd;

	switch (cmd) {
	case ISP_PM_BLK_ISP_SETTING:
		param_data_ptr->data_ptr = (void *)&iircnr_ptr->cur;
		param_data_ptr->data_size = sizeof(iircnr_ptr->cur);
		*update_flag = 0;
		break;
	case ISP_PM_BLK_GRGB_BYPASS:
		param_data_ptr->data_ptr = (void *)&iircnr_ptr->cur.bypass;
		param_data_ptr->data_size = sizeof(iircnr_ptr->cur.bypass);
		break;
	default:
		break;
	}

	return rtn;
}
