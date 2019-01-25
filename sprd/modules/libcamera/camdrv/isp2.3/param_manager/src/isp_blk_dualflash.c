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
#define LOG_TAG "isp_blk_dualflash"
#include "isp_blocks_cfg.h"

cmr_s32 _pm_dualflash_init(void *dst_flash_param, void *src_flash_param, void *param1, void *param2)
{
	cmr_s32 rtn = ISP_SUCCESS;
	cmr_s32 i = 0;
	struct isp_dual_flash_param *dst_flash_ptr = (struct isp_dual_flash_param *)dst_flash_param;
	struct dual_flash_tune_param *src_flash_ptr = (struct dual_flash_tune_param *)src_flash_param;
	struct isp_pm_block_header *flashlight_header_ptr = (struct isp_pm_block_header *)param1;
	UNUSED(param2);

	dst_flash_ptr->cur.version = src_flash_ptr->version;
	dst_flash_ptr->cur.alg_id = src_flash_ptr->alg_id;
	dst_flash_ptr->cur.flashLevelNum1 = src_flash_ptr->flashLevelNum1;
	dst_flash_ptr->cur.flashLevelNum2 = src_flash_ptr->flashLevelNum2;
	dst_flash_ptr->cur.preflahLevel1 = src_flash_ptr->preflahLevel1;
	dst_flash_ptr->cur.preflahLevel2 = src_flash_ptr->preflahLevel2;
	dst_flash_ptr->cur.preflashBrightness = src_flash_ptr->preflashBrightness;
	dst_flash_ptr->cur.brightnessTargetMax = src_flash_ptr->brightnessTargetMax;
	dst_flash_ptr->cur.foregroundRatioHigh = src_flash_ptr->foregroundRatioHigh;
	dst_flash_ptr->cur.foregroundRatioLow = src_flash_ptr->foregroundRatioLow;

	for (i = 0; i < 1024; i++) {
		dst_flash_ptr->cur.flashMask[i] = src_flash_ptr->flashMask[i];
		dst_flash_ptr->cur.brightnessTable[i] = src_flash_ptr->brightnessTable[i];
		dst_flash_ptr->cur.rTable[i] = src_flash_ptr->rTable[i];
		dst_flash_ptr->cur.bTable[i] = src_flash_ptr->bTable[i];
	}

	flashlight_header_ptr->is_update = ISP_ONE;

	return rtn;
}

cmr_s32 _pm_dualflash_set_param(void *flash_param, cmr_u32 cmd, void *param_ptr0, void *param_ptr1)
{
	cmr_s32 rtn = ISP_SUCCESS;
	struct dual_flash_tune_param *src_flash_ptr = PNULL;
	struct isp_dual_flash_param *dst_flash_ptr = (struct isp_dual_flash_param *)flash_param;
	struct isp_pm_block_header *flash_header_ptr = (struct isp_pm_block_header *)param_ptr1;
	UNUSED(param_ptr0);
	flash_header_ptr->is_update = ISP_ONE;
	switch (cmd) {
	case ISP_PM_BLK_DUAL_FLASH:
		break;
	default:
		flash_header_ptr->is_update = ISP_ZERO;
		break;
	}

	return rtn;
}

cmr_s32 _pm_dualflash_get_param(void *flash_param, cmr_u32 cmd, void *rtn_param0, void *rtn_param1)
{
	cmr_s32 rtn = ISP_SUCCESS;
	struct isp_pm_param_data *param_data_ptr = (struct isp_pm_param_data *)rtn_param0;
	struct isp_dual_flash_param *flash_ptr = (struct isp_dual_flash_param *)flash_param;
	cmr_u32 *update_flag = (cmr_u32 *) rtn_param1;

	param_data_ptr->id = ISP_BLK_DUAL_FLASH;
	param_data_ptr->cmd = cmd;

	switch (cmd) {
	case ISP_PM_BLK_ISP_SETTING:
		param_data_ptr->data_ptr = &flash_ptr->cur;
		param_data_ptr->data_size = sizeof(flash_ptr->cur);
		break;

	default:
		break;
	}

	return rtn;
}
