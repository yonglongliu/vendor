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
#define LOG_TAG "dcam_blk_aem"
#include "isp_blocks_cfg.h"

cmr_s32 _pm_dcam_aem_init(void *dst_rgb_aem, void *src_rgb_aem, void *param1, void *param2)
{
	cmr_s32 rtn = ISP_SUCCESS;
	struct dcam_rgb_aem_param *dst_ptr = (struct dcam_rgb_aem_param *)dst_rgb_aem;
	struct sensor_rgb_aem_param *src_ptr = (struct sensor_rgb_aem_param *)src_rgb_aem;
	struct isp_pm_block_header *header_ptr = (struct isp_pm_block_header *)param1;
	UNUSED(param2);

	memset((void *)&dst_ptr->cur, 0x00, sizeof(dst_ptr->cur));
	dst_ptr->cur.skip_num = src_ptr->aem_skip_num;
	dst_ptr->cur.blk_size.width = src_ptr->win_size.w;
	dst_ptr->cur.blk_size.height = src_ptr->win_size.h;
	dst_ptr->cur.offset.x = src_ptr->win_start.x;
	dst_ptr->cur.offset.y = src_ptr->win_start.y;
	header_ptr->is_update = ISP_ONE;

	return rtn;
}

cmr_s32 _pm_dcam_aem_set_param(void *rgb_aem_param, cmr_u32 cmd, void *param_ptr0, void *param_ptr1)
{
	cmr_s32 rtn = ISP_SUCCESS;
	struct dcam_rgb_aem_param *dst_ptr = (struct dcam_rgb_aem_param *)rgb_aem_param;
	struct isp_pm_block_header *header_ptr = (struct isp_pm_block_header *)param_ptr1;

	header_ptr->is_update = ISP_ONE;

	switch (cmd) {
	case ISP_PM_BLK_AEM_MODE:
		dst_ptr->cur.mode = *(cmr_u32 *)param_ptr0;
		break;

	case ISP_PM_BLK_AEM_STRENGTH_LEVEL:
		{
			cmr_u32 val = *(cmr_u32 *)param_ptr0;
			dst_ptr->cur.aem_avgshf.aem_h_avgshf = val;
			dst_ptr->cur.aem_avgshf.aem_l_avgshf = val;
			dst_ptr->cur.aem_avgshf.aem_m_avgshf = val;
		}
		break;

	case ISP_PM_BLK_AEM_STAT_THR:
		{
			struct dcam_ae_stat_threshold *thr_ptr = (struct dcam_ae_stat_threshold *)param_ptr0;
			dst_ptr->cur.red_thr.high = thr_ptr->r_thr_high;
			dst_ptr->cur.red_thr.low = thr_ptr->r_thr_low;
			dst_ptr->cur.blue_thr.high = thr_ptr->b_thr_high;
			dst_ptr->cur.blue_thr.low = thr_ptr->b_thr_low;
			dst_ptr->cur.green_thr.high = thr_ptr->g_thr_high;
			dst_ptr->cur.green_thr.low = thr_ptr->g_thr_low;
		}
		break;

	default:
		header_ptr->is_update = ISP_ZERO;
		break;
	}

	return rtn;
}

cmr_s32 _pm_dcam_aem_get_param(void *rgb_aem_param, cmr_u32 cmd, void *rtn_param0, void *rtn_param1)
{
	cmr_s32 rtn = ISP_SUCCESS;
	struct dcam_rgb_aem_param *rgb_aem_ptr = (struct dcam_rgb_aem_param *)rgb_aem_param;
	struct isp_pm_param_data *param_data_ptr = (struct isp_pm_param_data *)rtn_param0;
	cmr_u32 *update_flag = (cmr_u32 *) rtn_param1;

	param_data_ptr->id = ISP_BLK_AE_NEW;
	param_data_ptr->cmd = cmd;

	switch (cmd) {
	case ISP_PM_BLK_ISP_SETTING:
		param_data_ptr->data_ptr = (void *)&rgb_aem_ptr->cur;
		param_data_ptr->data_size = sizeof(rgb_aem_ptr->cur);
		*update_flag = 0;
		break;

	case ISP_PM_BLK_AEM_MODE:
		param_data_ptr->data_ptr = (void *)&rgb_aem_ptr->cur.mode;
		param_data_ptr->data_size = sizeof(rgb_aem_ptr->cur.mode);
		break;

	case ISP_PM_BLK_AEM_STATISTIC:
		param_data_ptr->data_ptr = (void *)&rgb_aem_ptr->stat;
		param_data_ptr->data_size = sizeof(rgb_aem_ptr->stat);
		break;

	default:
		break;
	}

	return rtn;
}
