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
#define LOG_TAG "isp_blk_awbc"
#include "isp_blocks_cfg.h"

cmr_s32 _pm_awbc_init(void *dst_awb, void *src_awb, void *param1, void *param2)
{
	cmr_s32 rtn = ISP_SUCCESS;
	struct isp_awb_param *dst_ptr = (struct isp_awb_param *)dst_awb;
	struct sensor_awbc_param *src_ptr = (struct sensor_awbc_param *)src_awb;
	struct isp_pm_block_header *header_ptr = (struct isp_pm_block_header *)param1;
	UNUSED(param2);

	dst_ptr->ct_value = 5000;
	memset((void *)&dst_ptr->cur, 0x00, sizeof(dst_ptr->cur));
		dst_ptr->cur.awbc_bypass = header_ptr->bypass;

	dst_ptr->cur.gain.r = src_ptr->awbc_gain.r_gain;
	dst_ptr->cur.gain.gr = src_ptr->awbc_gain.gr_gain;
	dst_ptr->cur.gain.gb = src_ptr->awbc_gain.gb_gain;
	dst_ptr->cur.gain.b = src_ptr->awbc_gain.b_gain;
	dst_ptr->cur.thrd.r = src_ptr->awbc_thr.r_thr;
	dst_ptr->cur.thrd.g = src_ptr->awbc_thr.g_thr;
	dst_ptr->cur.thrd.b = src_ptr->awbc_thr.b_thr;
	dst_ptr->cur.gain_offset.r = src_ptr->awbc_offset.r_offset;
	dst_ptr->cur.gain_offset.gr = src_ptr->awbc_offset.gr_offset;
	dst_ptr->cur.gain_offset.gb = src_ptr->awbc_offset.gb_offset;
	dst_ptr->cur.gain_offset.b = src_ptr->awbc_offset.b_offset;

	header_ptr->is_update = ISP_ONE;

	return rtn;
}

cmr_s32 _pm_awbc_set_param(void *awb_param, cmr_u32 cmd, void *param_ptr0, void *param_ptr1)
{
	cmr_s32 rtn = ISP_SUCCESS;
	struct isp_awb_param *dst_ptr = (struct isp_awb_param *)awb_param;
	struct isp_pm_block_header *awb_header_ptr = (struct isp_pm_block_header *)param_ptr1;

	awb_header_ptr->is_update = ISP_ONE;

	switch (cmd) {
	case ISP_PM_BLK_AWBC:
		{
			struct isp_awbc_cfg *cfg_ptr = (struct isp_awbc_cfg *)param_ptr0;
			dst_ptr->cur.gain.r = cfg_ptr->r_gain;
			dst_ptr->cur.gain.gr = cfg_ptr->g_gain;
			dst_ptr->cur.gain.gb = cfg_ptr->g_gain;
			dst_ptr->cur.gain.b = cfg_ptr->b_gain;
			dst_ptr->cur.gain_offset.r = cfg_ptr->r_offset;
			dst_ptr->cur.gain_offset.gr = cfg_ptr->g_offset;
			dst_ptr->cur.gain_offset.gb = cfg_ptr->g_offset;
			dst_ptr->cur.gain_offset.b = cfg_ptr->b_offset;
		}
		break;

	case ISP_PM_BLK_AWBM:

		break;

	case ISP_PM_BLK_AWB_CT:
		dst_ptr->ct_value = *((cmr_u32 *) param_ptr0);
		break;

	case ISP_PM_BLK_AWBC_BYPASS:
		dst_ptr->cur.awbc_bypass = *((cmr_u32 *) param_ptr0);
		break;

	case ISP_PM_BLK_AWBM_BYPASS:
		break;

	case ISP_PM_BLK_MEMORY_INIT:
		{
			cmr_u32 i = 0;
			struct isp_pm_memory_init_param *memory_ptr = (struct isp_pm_memory_init_param *)param_ptr0;
			for (i = 0; i < memory_ptr->size_info.count_lines; ++i) {
				dst_ptr->awb_statistics[i].data_ptr = ((cmr_u8 *) memory_ptr->buffer.data_ptr + memory_ptr->size_info.pitch * i);
				dst_ptr->awb_statistics[i].size = memory_ptr->size_info.pitch;
			}
		}
		break;
	default:
		awb_header_ptr->is_update = ISP_ZERO;
		break;
	}

	return rtn;
}

cmr_s32 _pm_awbc_get_param(void *awb_param, cmr_u32 cmd, void *rtn_param0, void *rtn_param1)
{
	cmr_s32 rtn = ISP_SUCCESS;
	struct isp_awb_param *awb_param_ptr = (struct isp_awb_param *)awb_param;
	struct isp_pm_param_data *param_data_ptr = (struct isp_pm_param_data *)rtn_param0;
	cmr_u32 *update_flag = (cmr_u32 *) rtn_param1;

	param_data_ptr->id = ISP_BLK_AWBC;
	param_data_ptr->cmd = cmd;

	switch (cmd) {
	case ISP_PM_BLK_ISP_SETTING:
		param_data_ptr->data_ptr = &awb_param_ptr->cur;
		param_data_ptr->data_size = sizeof(awb_param_ptr->cur);
		*update_flag = 0;
		break;

	case ISP_PM_BLK_AWBM:
		break;

	case ISP_PM_BLK_AWBC_BYPASS:
		break;

	case ISP_PM_BLK_AWB_CT:
		param_data_ptr->data_ptr = &awb_param_ptr->ct_value;
		param_data_ptr->data_size = sizeof(awb_param_ptr->ct_value);
		break;

	case ISP_PM_BLK_AWBM_STATISTIC:
		param_data_ptr->data_ptr = (void *)&awb_param_ptr->stat;
		param_data_ptr->data_size = sizeof(awb_param_ptr->stat);
		break;

	default:
		break;
	}

	return rtn;
}
