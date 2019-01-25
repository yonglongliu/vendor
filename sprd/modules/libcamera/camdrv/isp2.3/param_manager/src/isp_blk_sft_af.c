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
#define LOG_TAG "isp_blk_aft_af"
#include "isp_blocks_cfg.h"

cmr_s32 _pm_sft_af_init(void *dst_sft_af_param, void *src_sft_af_param, void *param1, void *param_ptr2)
{
	cmr_s32 rtn = ISP_SUCCESS;
	UNUSED(dst_sft_af_param);
	UNUSED(src_sft_af_param);
	UNUSED(param1);
	UNUSED(param_ptr2);
#if 0
	struct isp_sft_af_param *src_sft_af_ptr = (struct isp_sft_af_param *)src_sft_af_param;
	struct isp_sft_af_param *dst_sft_af_ptr = (struct isp_sft_af_param *)dst_sft_af_param;
	struct isp_pm_block_header *sft_af_header_ptr = (struct isp_pm_block_header *)param1;
	UNUSED(param_ptr2);

	memcpy(dst_sft_af_ptr, src_sft_af_ptr, sizeof(struct isp_sft_af_param));

	sft_af_header_ptr->is_update = ISP_ONE;
#endif
	return rtn;
}

cmr_s32 _pm_sft_af_set_param(void *sft_af_param, cmr_u32 cmd, void *param_ptr0, void *param_ptr1)
{
	cmr_s32 rtn = ISP_SUCCESS;

	UNUSED(sft_af_param);
	UNUSED(cmd);
	UNUSED(param_ptr0);
	UNUSED(param_ptr1);

	return rtn;
}

cmr_s32 _pm_sft_af_get_param(void *sft_af_param, cmr_u32 cmd, void *rtn_param0, void *rtn_param1)
{
	cmr_s32 rtn = ISP_SUCCESS;
	UNUSED(sft_af_param);
	UNUSED(cmd);
	UNUSED(rtn_param0);
	UNUSED(rtn_param1);
#if 0
	struct isp_sft_af_param *sft_af_ptr = (struct isp_sft_af_param *)sft_af_param;
	struct isp_pm_param_data *param_data_ptr = (struct isp_pm_param_data *)rtn_param0;
	cmr_u32 *update_flag = (cmr_u32 *) rtn_param1;
	param_data_ptr->cmd = cmd;
	param_data_ptr->id = ISP_BLK_SMART;

	switch (cmd) {
	case ISP_PM_BLK_SMART_SETTING:
		{
			param_data_ptr->data_ptr = (void *)sft_af_ptr;
			param_data_ptr->data_size = sizeof(struct isp_sft_af_param);
			*update_flag = ISP_ZERO;
		}
		break;
	default:
		break;
	}
#endif
	return rtn;
}
