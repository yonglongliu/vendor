/*
 * Copyright (C) 2012 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define LOG_TAG "isp_u_raw_aem"

#include "isp_drv.h"

cmr_s32 isp_u_raw_aem_block(cmr_handle handle, void *param_ptr)
{
	cmr_s32 ret = 0;
	struct isp_u_blocks_info *raw_aem_ptr = NULL;
	struct isp_file *file = NULL;
	struct isp_io_param param;
	struct isp_dev_raw_aem_info aem_info;
	struct sprd_aem_info dcam_aem_info;

	if (!handle || !param_ptr) {
		ISP_LOGE("failed to get ptr: %p, %p", handle, param_ptr);
		return -1;
	}
	raw_aem_ptr = (struct isp_u_blocks_info *)param_ptr;

	memset(&dcam_aem_info, 0x00, sizeof(dcam_aem_info));
	dcam_aem_info.skip_num = raw_aem_ptr->stats_info.skip_num;
	dcam_aem_info.mode = raw_aem_ptr->stats_info.mode;
	dcam_aem_info.offset.x = raw_aem_ptr->stats_info.offset.x;
	dcam_aem_info.offset.y = raw_aem_ptr->stats_info.offset.y;
	dcam_aem_info.blk_size.width = raw_aem_ptr->stats_info.size.width;
	dcam_aem_info.blk_size.height = raw_aem_ptr->stats_info.size.height;
	dcam_aem_info.aem_avgshf.aem_h_avgshf = 0;
	dcam_aem_info.aem_avgshf.aem_l_avgshf = 0;
	dcam_aem_info.aem_avgshf.aem_m_avgshf = 0;
	dcam_aem_info.red_thr.low = 1;
	dcam_aem_info.red_thr.high = 1022;
	dcam_aem_info.blue_thr.low = 1;
	dcam_aem_info.blue_thr.high = 1022;
	dcam_aem_info.green_thr.low = 1;
	dcam_aem_info.green_thr.high = 1022;

	ret = dcam_u_raw_aem_block(handle, &dcam_aem_info);

	file = (struct isp_file *)(handle);
	aem_info.bypass = 1;
	param.isp_id = file->isp_id;
	param.scene_id = 0;
	param.sub_block = ISP_BLOCK_RAW_AEM;
	param.property = ISP_PRO_RAW_AEM_BLOCK;
	param.property_param = &aem_info;

	ret = ioctl(file->fd, SPRD_ISP_IO_CFG_PARAM, &param);

	param.scene_id = 1;
	ret = ioctl(file->fd, SPRD_ISP_IO_CFG_PARAM, &param);

	return ret;
}

cmr_s32 isp_u_raw_aem_bypass(cmr_handle handle, void *param_ptr)
{
	cmr_s32 ret = 0;

	if (!handle || !param_ptr) {
		ISP_LOGE("failed to get ptr: %p, %p", handle, param_ptr);
		return -1;
	}

	return ret;
}

cmr_s32 isp_u_raw_aem_mode(cmr_handle handle, void *param_ptr)
{
	struct isp_u_blocks_info *raw_aem_ptr = NULL;
	raw_aem_ptr = (struct isp_u_blocks_info *)param_ptr;

	return dcam_u_raw_aem_mode(handle, raw_aem_ptr->stats_info.mode);
}

cmr_s32 isp_u_raw_aem_skip_num(cmr_handle handle, void *param_ptr)
{
	struct isp_u_blocks_info *raw_aem_ptr = NULL;
	raw_aem_ptr = (struct isp_u_blocks_info *)param_ptr;

	return dcam_u_raw_aem_skip_num(handle, raw_aem_ptr->stats_info.skip_num);
}

cmr_s32 isp_u_raw_aem_shift(cmr_handle handle, void *param_ptr)
{
	struct isp_u_blocks_info *raw_aem_ptr = NULL;
	raw_aem_ptr = (struct isp_u_blocks_info *)param_ptr;

	return dcam_u_raw_aem_shift(handle, &raw_aem_ptr->shift);
}

cmr_s32 isp_u_raw_aem_offset(cmr_handle handle, void *param_ptr)
{
	struct isp_u_blocks_info *raw_aem_ptr = NULL;
	raw_aem_ptr = (struct isp_u_blocks_info *)param_ptr;

	return dcam_u_raw_aem_offset(handle,
			raw_aem_ptr->stats_info.offset.x,
			raw_aem_ptr->stats_info.offset.y);
}

cmr_s32 isp_u_raw_aem_blk_size(cmr_handle handle, void *param_ptr)
{
	struct isp_u_blocks_info *raw_aem_ptr = NULL;
	raw_aem_ptr = (struct isp_u_blocks_info *)param_ptr;

	return dcam_u_raw_aem_blk_size(handle,
			raw_aem_ptr->stats_info.size.width,
			raw_aem_ptr->stats_info.size.height);
}

cmr_s32 isp_u_raw_aem_slice_size(cmr_handle handle, void *param_ptr)
{
	cmr_s32 ret = 0;
	struct isp_file *file = NULL;
	struct isp_u_blocks_info *raw_aem_ptr = NULL;
	struct isp_io_param param;
	struct isp_img_size size;

	if (!handle || !param_ptr) {
		ISP_LOGE("failed to get ptr: %p, %p", handle, param_ptr);
		return -1;
	}

	file = (struct isp_file *)(handle);
	raw_aem_ptr = (struct isp_u_blocks_info *)param_ptr;

	size.width = raw_aem_ptr->size.width;
	size.height = raw_aem_ptr->size.height;

	param.isp_id = file->isp_id;
	param.scene_id = raw_aem_ptr->scene_id;
	param.sub_block = ISP_BLOCK_RAW_AEM;
	param.property = ISP_PRO_RAW_AEM_SLICE_SIZE;
	param.property_param = &size;

	ret = ioctl(file->fd, SPRD_ISP_IO_CFG_PARAM, &param);

	return ret;
}
