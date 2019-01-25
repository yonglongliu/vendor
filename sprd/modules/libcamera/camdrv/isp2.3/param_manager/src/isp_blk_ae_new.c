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
#define LOG_TAG "isp_blk_ae_new"
#include "isp_blocks_cfg.h"

cmr_s32 _pm_ae_new_init(void *dst_ae_new, void *src_ae_new, void *param1, void *param2)
{
	cmr_s32 rtn = ISP_SUCCESS;
	UNUSED(dst_ae_new);
	UNUSED(src_ae_new);
	UNUSED(param1);
	UNUSED(param2);

	return rtn;
}

cmr_s32 _pm_ae_new_set_param(void *ae_new_param, cmr_u32 cmd, void *param_ptr0, void *param_ptr1)
{
	cmr_s32 rtn = ISP_SUCCESS;
	UNUSED(ae_new_param);
	UNUSED(cmd);
	UNUSED(param_ptr0);
	UNUSED(param_ptr1);

	return rtn;
}

cmr_s32 _pm_ae_new_get_param(void *ae_new_param, cmr_u32 cmd, void *rtn_param0, void *rtn_param1)
{
	cmr_s32 rtn = ISP_SUCCESS;
	UNUSED(ae_new_param);
	UNUSED(rtn_param0);
	UNUSED(rtn_param1);

	switch (cmd) {
	case ISP_PM_BLK_ISP_SETTING:
		break;

	default:
		break;
	}

	return rtn;
}
