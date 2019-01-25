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
#define LOG_TAG "isp_blk_pdaf_tune"
#include "isp_blocks_cfg.h"

cmr_s32 _pm_pdaf_tune_init(void *dst_pdaf_tune_param, void *src_pdaf_tune_param, void *param1, void *param2)
{
	cmr_s32 rtn = ISP_SUCCESS;
	cmr_s32 i = 0;
	struct isp_haf_tune_param *dst_pdaf_tune_ptr = (struct isp_haf_tune_param *)dst_pdaf_tune_param;
	struct haf_tune_param *src_pdaf_tune_ptr = (struct haf_tune_param *)src_pdaf_tune_param;
	struct isp_pm_block_header *pdaf_tune_header_ptr = (struct isp_pm_block_header *)param1;
	UNUSED(param2);

	for (i = 0; i < 3; i++) {
		dst_pdaf_tune_ptr->isp_pdaf_tune_data[i].min_pd_vcm_steps = src_pdaf_tune_ptr->pdaf_tune_data[i].min_pd_vcm_steps;
		dst_pdaf_tune_ptr->isp_pdaf_tune_data[i].max_pd_vcm_steps = src_pdaf_tune_ptr->pdaf_tune_data[i].max_pd_vcm_steps;
		dst_pdaf_tune_ptr->isp_pdaf_tune_data[i].coc_range = src_pdaf_tune_ptr->pdaf_tune_data[i].coc_range;
		dst_pdaf_tune_ptr->isp_pdaf_tune_data[i].far_tolerance = src_pdaf_tune_ptr->pdaf_tune_data[i].far_tolerance;
		dst_pdaf_tune_ptr->isp_pdaf_tune_data[i].near_tolerance = src_pdaf_tune_ptr->pdaf_tune_data[i].near_tolerance;
		dst_pdaf_tune_ptr->isp_pdaf_tune_data[i].err_limit = src_pdaf_tune_ptr->pdaf_tune_data[i].err_limit;
		dst_pdaf_tune_ptr->isp_pdaf_tune_data[i].pd_converge_thr = src_pdaf_tune_ptr->pdaf_tune_data[i].pd_converge_thr;
		dst_pdaf_tune_ptr->isp_pdaf_tune_data[i].pd_converge_thr_2nd = src_pdaf_tune_ptr->pdaf_tune_data[i].pd_converge_thr_2nd;
		dst_pdaf_tune_ptr->isp_pdaf_tune_data[i].pd_focus_times_thr = src_pdaf_tune_ptr->pdaf_tune_data[i].pd_focus_times_thr;
		dst_pdaf_tune_ptr->isp_pdaf_tune_data[i].pd_thread_sync_frm = src_pdaf_tune_ptr->pdaf_tune_data[i].pd_thread_sync_frm;
		dst_pdaf_tune_ptr->isp_pdaf_tune_data[i].pd_thread_sync_frm_init = src_pdaf_tune_ptr->pdaf_tune_data[i].pd_thread_sync_frm_init;
		dst_pdaf_tune_ptr->isp_pdaf_tune_data[i].min_process_frm = src_pdaf_tune_ptr->pdaf_tune_data[i].min_process_frm;
		dst_pdaf_tune_ptr->isp_pdaf_tune_data[i].max_process_frm = src_pdaf_tune_ptr->pdaf_tune_data[i].max_process_frm;
		dst_pdaf_tune_ptr->isp_pdaf_tune_data[i].pd_conf_thr = src_pdaf_tune_ptr->pdaf_tune_data[i].pd_conf_thr;
		dst_pdaf_tune_ptr->isp_pdaf_tune_data[i].pd_conf_thr_2nd = src_pdaf_tune_ptr->pdaf_tune_data[i].pd_conf_thr_2nd;
	}

	pdaf_tune_header_ptr->is_update = ISP_ONE;

	return rtn;
}

cmr_s32 _pm_pdaf_tune_set_param(void *pdaf_tune_param, cmr_u32 cmd, void *param_ptr0, void *param_ptr1)
{
	cmr_s32 rtn = ISP_SUCCESS;
	struct isp_pm_block_header *pdaf_tune_header_ptr = (struct isp_pm_block_header *)param_ptr1;

	UNUSED(pdaf_tune_param);
	UNUSED(param_ptr0);

	pdaf_tune_header_ptr->is_update = ISP_ONE;
	switch (cmd) {
	case ISP_PM_BLK_PDAF_TUNE:
		break;
	default:
		pdaf_tune_header_ptr->is_update = ISP_ZERO;
		break;
	}

	return rtn;
}

cmr_s32 _pm_pdaf_tune_get_param(void *pdaf_tune_param, cmr_u32 cmd, void *rtn_param0, void *rtn_param1)
{
	cmr_s32 rtn = ISP_SUCCESS;
	struct isp_pm_param_data *param_data_ptr = (struct isp_pm_param_data *)rtn_param0;
	struct isp_haf_tune_param *pdaf_tune_ptr = (struct isp_haf_tune_param *)pdaf_tune_param;

	UNUSED(rtn_param1);
	param_data_ptr->id = ISP_BLK_PDAF_TUNE;
	param_data_ptr->cmd = cmd;

	switch (cmd) {
	case ISP_PM_BLK_ISP_SETTING:
		param_data_ptr->data_ptr = pdaf_tune_ptr;
		param_data_ptr->data_size = sizeof(struct isp_haf_tune_param);
		break;

	default:
		break;
	}

	return rtn;
}
