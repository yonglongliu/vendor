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
#define LOG_TAG "isp_blk_css"
#include "isp_blocks_cfg.h"

cmr_s32 _pm_css_init(void *dst_css_param, void *src_css_param, void *param1, void *param_ptr2)
{
	cmr_s32 rtn = ISP_SUCCESS;
	cmr_u32 i = 0;
	struct isp_css_param *dst_css_ptr = (struct isp_css_param *)dst_css_param;
	struct sensor_css_v1_param *src_css_ptr = (struct sensor_css_v1_param *)src_css_param;
	struct isp_pm_block_header *css_header_ptr = (struct isp_pm_block_header *)param1;
	UNUSED(param_ptr2);

	dst_css_ptr->cur.bypass = css_header_ptr->bypass;
	dst_css_ptr->cur.lh_chrom_th = src_css_ptr->lh.css_lh_chrom_th;
	for (i = 0; i < 7 ; i++) {
		dst_css_ptr->cur.chrom_lower_th[i] = src_css_ptr->chrom_thre.css_chrom_lower_th[i];
		dst_css_ptr->cur.chrom_high_th[i] = src_css_ptr->chrom_thre.css_chrom_high_th[i];
	}
	dst_css_ptr->cur.lum_low_shift = src_css_ptr->lum_thre.css_lum_low_shift;
	dst_css_ptr->cur.lum_hig_shift = src_css_ptr->lum_thre.css_lum_hig_shift;
	for (i = 0; i < 8 ; i++) {
		dst_css_ptr->cur.lh_ratio[i] = src_css_ptr->lh.css_lh_ratio[i];
		dst_css_ptr->cur.ratio[i] = src_css_ptr->ratio[i];
	}
	dst_css_ptr->cur.lum_low_th = src_css_ptr->lum_thre.css_lum_low_th;
	dst_css_ptr->cur.lum_ll_th = src_css_ptr->lum_thre.css_lum_ll_th;
	dst_css_ptr->cur.lum_hig_th = src_css_ptr->lum_thre.css_lum_hig_th;
	dst_css_ptr->cur.lum_hh_th = src_css_ptr->lum_thre.css_lum_hh_th;
	dst_css_ptr->cur.u_th_0_l = src_css_ptr->exclude[0].exclude_u_th_l;
	dst_css_ptr->cur.u_th_0_h = src_css_ptr->exclude[0].exclude_u_th_h;
	dst_css_ptr->cur.v_th_0_l = src_css_ptr->exclude[0].exclude_v_th_l;
	dst_css_ptr->cur.v_th_0_h = src_css_ptr->exclude[0].exclude_v_th_h;
	dst_css_ptr->cur.u_th_1_l = src_css_ptr->exclude[1].exclude_u_th_l;
	dst_css_ptr->cur.u_th_1_h = src_css_ptr->exclude[1].exclude_u_th_h;
	dst_css_ptr->cur.v_th_1_l = src_css_ptr->exclude[1].exclude_v_th_l;
	dst_css_ptr->cur.v_th_1_h = src_css_ptr->exclude[1].exclude_v_th_h;
	dst_css_ptr->cur.cutoff_th = src_css_ptr->chrom_thre.css_chrom_cutoff_th;
	css_header_ptr->is_update = ISP_ONE;

	return rtn;
}

cmr_s32 _pm_css_set_param(void *css_param, cmr_u32 cmd, void *param_ptr0, void *param_ptr1)
{
	cmr_s32 rtn = ISP_SUCCESS;
	struct isp_css_param *css_ptr = (struct isp_css_param *)css_param;
	struct isp_pm_block_header *css_header_ptr = PNULL;
	UNUSED(cmd);

	css_header_ptr = (struct isp_pm_block_header *)param_ptr1;
	css_header_ptr->is_update = 1;
	css_ptr->cur.bypass = *((cmr_u32 *)param_ptr0);

	return rtn;

}

cmr_s32 _pm_css_get_param(void *css_param, cmr_u32 cmd, void *rtn_param0, void *rtn_param1)
{
	cmr_s32 rtn = ISP_SUCCESS;
	struct isp_css_param *css_ptr = (struct isp_css_param *)css_param;
	struct isp_pm_param_data *param_data_ptr = (struct isp_pm_param_data *)rtn_param0;
	cmr_u32 *update_flag = (cmr_u32*)rtn_param1;

	param_data_ptr->id = ISP_BLK_CSS;
	param_data_ptr->cmd = cmd;

	switch (cmd) {
	case ISP_PM_BLK_ISP_SETTING:
		param_data_ptr->data_ptr = (void *)&css_ptr->cur;
		param_data_ptr->data_size = sizeof(css_ptr->cur);
		*update_flag = 0;
		break;

	default:
		break;
	}

	return rtn;
}
