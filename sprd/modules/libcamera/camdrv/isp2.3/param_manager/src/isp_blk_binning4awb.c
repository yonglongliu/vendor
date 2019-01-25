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
#define LOG_TAG "isp_blk_binning4awb"
#include "isp_blocks_cfg.h"

cmr_s32 _pm_binning4awb_init(void *dst_binning4awb, void *src_binning4awb, void *param1, void *param2)
{
	cmr_s32 rtn = ISP_SUCCESS;

	struct isp_binning4awb_param *dst_ptr = (struct isp_binning4awb_param *)dst_binning4awb;
	struct isp_bin_param *src_ptr = (struct isp_bin_param *)src_binning4awb;
	struct isp_pm_block_header *header_ptr = (struct isp_pm_block_header *)param1;
	struct isp_size *img_size_ptr = (struct isp_size *)param2;
	UNUSED(param2);

	memset((void *)&dst_ptr->cur, 0x00, sizeof(dst_ptr->cur));
	/*modify to debug binning */
	dst_ptr->cur.bypass = header_ptr->bypass;
	dst_ptr->cur.hx = src_ptr->hx;
	dst_ptr->cur.vx = src_ptr->vx;
	dst_ptr->cur.img_size.width = img_size_ptr->w;
	dst_ptr->cur.img_size.height = img_size_ptr->h;
	header_ptr->is_update = ISP_ONE;

	return rtn;
}

cmr_s32 _pm_binning4awb_set_param(void *binning4awb_param, cmr_u32 cmd, void *param_ptr0, void *param_ptr1)
{
	cmr_s32 rtn = ISP_SUCCESS;
	struct isp_binning4awb_param *dst_ptr = (struct isp_binning4awb_param *)binning4awb_param;
	struct isp_pm_block_header *header_ptr = (struct isp_pm_block_header *)param_ptr1;

	header_ptr->is_update = ISP_ONE;

	switch (cmd) {
	case ISP_PM_BLK_BINNING4AWB_BYPASS:
		dst_ptr->cur.bypass = *((cmr_u32 *) param_ptr0);
		break;

	default:
		header_ptr->is_update = ISP_ZERO;
		break;
	}

	return rtn;
}

cmr_s32 _pm_binning4awb_get_param(void *binning4awb_param, cmr_u32 cmd, void *rtn_param0, void *rtn_param1)
{
	cmr_s32 rtn = ISP_SUCCESS;
	struct isp_binning4awb_param *binning4awb_ptr = (struct isp_binning4awb_param *)binning4awb_param;
	struct isp_pm_param_data *param_data_ptr = (struct isp_pm_param_data *)rtn_param0;
	cmr_u32 *update_flag = (cmr_u32 *) rtn_param1;

	param_data_ptr->id = ISP_BLK_BINNING4AWB;
	param_data_ptr->cmd = cmd;

	switch (cmd) {
	case ISP_PM_BLK_ISP_SETTING:
		param_data_ptr->data_ptr = (void *)&binning4awb_ptr->cur;
		param_data_ptr->data_size = sizeof(binning4awb_ptr->cur);
		*update_flag = 0;
		break;

	case ISP_PM_BLK_BINNING4AWB_BYPASS:
		param_data_ptr->data_ptr = (void *)&binning4awb_ptr->cur.bypass;
		param_data_ptr->data_size = sizeof(binning4awb_ptr->cur.bypass);
		break;

	default:
		break;
	}

	return rtn;
}
