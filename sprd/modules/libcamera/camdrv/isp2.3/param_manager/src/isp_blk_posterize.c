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
#define LOG_TAG "isp_blk_posterize"
#include "isp_blocks_cfg.h"

cmr_s32 _pm_posterize_init(void *dst_pstrz_param, void *src_pstrz_param, void *param1, void *param_ptr2)
{
	cmr_u32 i = 0;
	cmr_u32 j = 0;
	cmr_s32 rtn = ISP_SUCCESS;
	struct sensor_posterize_param *src_pstrz_ptr = (struct sensor_posterize_param *)src_pstrz_param;
	struct isp_posterize_param *dst_pstrz_ptr = (struct isp_posterize_param *)dst_pstrz_param;
	struct isp_pm_block_header *pstrz_header_ptr = (struct isp_pm_block_header *)param1;
	UNUSED(param_ptr2);

	dst_pstrz_ptr->cur.bypass = pstrz_header_ptr->bypass;

	for (i = 0; i < 8; i++) {
		dst_pstrz_ptr->cur.posterize_level_bottom[i] = src_pstrz_ptr->pstrz_bot[i];
		dst_pstrz_ptr->cur.posterize_level_top[i] = src_pstrz_ptr->pstrz_top[i];
		dst_pstrz_ptr->cur.posterize_level_out[i] = src_pstrz_ptr->pstrz_out[i];
	}
	for (j = 0; j < MAX_SPECIALEFFECT_NUM; ++j) {
		dst_pstrz_ptr->specialeffect_tab[j].bypass = 1;
		for (i = 0; i < 8; i++) {
			dst_pstrz_ptr->specialeffect_tab[j].posterize_level_bottom[i] = src_pstrz_ptr->specialeffect_bot[j][i];
			dst_pstrz_ptr->specialeffect_tab[j].posterize_level_top[i] = src_pstrz_ptr->specialeffect_top[j][i];
			dst_pstrz_ptr->specialeffect_tab[j].posterize_level_out[i] = src_pstrz_ptr->specialeffect_out[j][i];
		}
	}

	pstrz_header_ptr->is_update = ISP_ONE;

	return rtn;
}

cmr_s32 _pm_posterize_set_param(void *pstrz_param, cmr_u32 cmd, void *param_ptr0, void *param_ptr1)
{
	cmr_s32 rtn = ISP_SUCCESS;
	struct isp_posterize_param *pstrz_ptr = (struct isp_posterize_param *)pstrz_param;
	struct isp_pm_block_header *pstrz_header_ptr = PNULL;

	pstrz_header_ptr = (struct isp_pm_block_header *)param_ptr1;
	pstrz_header_ptr->is_update = ISP_ONE;

	switch (cmd) {
	case ISP_PM_BLK_PSTRZ_BYPASS:
		pstrz_ptr->cur.bypass = *((cmr_u32 *) param_ptr0);
		break;

	case ISP_PM_BLK_SPECIAL_EFFECT:
		{
			cmr_u32 idx = *((cmr_u32 *) param_ptr0);
			if (idx == 0) {
				pstrz_ptr->cur.bypass = 1;
			} else {
				pstrz_ptr->cur.bypass = 0;
				pstrz_ptr->cur = pstrz_ptr->specialeffect_tab[idx];
			}
		}
		break;

	default:
		pstrz_header_ptr->is_update = ISP_ZERO;
		break;
	}

	return rtn;
}

cmr_s32 _pm_posterize_get_param(void *pstrz_param, cmr_u32 cmd, void *rtn_param0, void *rtn_param1)
{
	cmr_s32 rtn = ISP_SUCCESS;
	struct isp_posterize_param *pstrz_ptr = (struct isp_posterize_param *)pstrz_param;
	struct isp_pm_param_data *param_data_ptr = (struct isp_pm_param_data *)rtn_param0;
	cmr_u32 *update_flag = (cmr_u32 *) rtn_param1;

	param_data_ptr->id = ISP_BLK_POSTERIZE;
	param_data_ptr->cmd = cmd;

	switch (cmd) {
	case ISP_PM_BLK_ISP_SETTING:
		param_data_ptr->data_ptr = &pstrz_ptr->cur;
		param_data_ptr->data_size = sizeof(pstrz_ptr->cur);
		*update_flag = 0;
		break;

	default:
		break;
	}

	return rtn;
}
