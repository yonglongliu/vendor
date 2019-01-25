/*
 * Copyright (C) 2016 The Android Open Source Project
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

#define LOG_TAG "dcam_u_pdaf"

#include "isp_drv.h"

cmr_s32 isp_u_pdaf_bypass(cmr_handle handle, void *param_ptr)
{
	cmr_s32 ret = 0;
	struct isp_file *file = NULL;
	struct isp_u_blocks_info *pdaf_ptr = NULL;
	struct isp_io_param param;

	if (!handle || !param_ptr) {
		ISP_LOGE("failed to get prt: %p, %p", handle, param_ptr);
		return -1;
	}

	file = (struct isp_file *)(handle);
	pdaf_ptr = (struct isp_u_blocks_info *)param_ptr;

	param.isp_id = file->isp_id;
	param.scene_id = pdaf_ptr->scene_id;
	param.sub_block = DCAM_BLOCK_PDAF;
	param.property = DCAM_PRO_PDAF_BYPASS;
	param.property_param = &pdaf_ptr->bypass;

	ret = ioctl(file->fd, SPRD_STATIS_IO_CFG_PARAM, &param);
	return ret;
}

cmr_s32 isp_u_pdaf_work_mode(cmr_handle handle, void *param_ptr)
{
	cmr_s32 ret = 0;
	struct isp_file *file = NULL;
	struct isp_u_blocks_info *pdaf_ptr = NULL;
	struct isp_io_param param;

	if (!handle || !param_ptr) {
		ISP_LOGE("failed to get prt: %p, %p", handle, param_ptr);
		return -1;
	}

	file = (struct isp_file *)(handle);
	pdaf_ptr = (struct isp_u_blocks_info *)param_ptr;

	param.isp_id = file->isp_id;
	param.scene_id = pdaf_ptr->scene_id;
	param.sub_block = DCAM_BLOCK_PDAF;
	param.property = DCAM_PRO_PDAF_SET_MODE;
	param.property_param = &pdaf_ptr->mode;

	ret = ioctl(file->fd, SPRD_STATIS_IO_CFG_PARAM, &param);
	return ret;
}

cmr_s32 isp_u_pdaf_extractor_bypass(cmr_handle handle, void *param_ptr)
{
	cmr_s32 ret = 0;
	struct isp_file *file = NULL;
	struct isp_u_blocks_info *pdaf_ptr = NULL;
	struct isp_io_param param;

	if (!handle || !param_ptr) {
		ISP_LOGE("failed to get prt: %p, %p", handle, param_ptr);
		return -1;
	}

	file = (struct isp_file *)(handle);
	pdaf_ptr = (struct isp_u_blocks_info *)param_ptr;

	param.isp_id = file->isp_id;
	param.scene_id = pdaf_ptr->scene_id;
	param.sub_block = DCAM_BLOCK_PDAF;
	param.property = DCAM_PRO_PDAF_SET_EXTRACTOR_BYPASS;
	param.property_param = &pdaf_ptr->bypass;

	ret = ioctl(file->fd, SPRD_STATIS_IO_CFG_PARAM, &param);
	return ret;
}

cmr_s32 isp_u_pdaf_skip_num(cmr_handle handle, void *param_ptr)
{
	cmr_s32 ret = 0;
	struct isp_file *file = NULL;
	struct isp_u_blocks_info *pdaf_ptr = NULL;
	struct isp_io_param param;

	if (!handle || !param_ptr) {
		ISP_LOGE("failed to get prt: %p, %p", handle, param_ptr);
		return -1;
	}

	file = (struct isp_file *)(handle);
	pdaf_ptr = (struct isp_u_blocks_info *)param_ptr;

	param.isp_id = file->isp_id;
	param.scene_id = pdaf_ptr->scene_id;
	param.sub_block = DCAM_BLOCK_PDAF;
	param.property = DCAM_PRO_PDAF_SET_SKIP_NUM;
	param.property_param = &pdaf_ptr->skip_num;

	ret = ioctl(file->fd, SPRD_STATIS_IO_CFG_PARAM, &param);
	return ret;
}

cmr_s32 isp_u_pdaf_roi(cmr_handle handle, void *param_ptr)
{
	cmr_s32 ret = 0;
	struct isp_file *file = NULL;
	struct isp_u_blocks_info *pdaf_ptr = NULL;
	struct isp_io_param param;

	if (!handle || !param_ptr) {
		ISP_LOGE("failed to get prt: %p, %p", handle, param_ptr);
		return -1;
	}

	file = (struct isp_file *)(handle);
	pdaf_ptr = (struct isp_u_blocks_info *)param_ptr;

	param.isp_id = file->isp_id;
	param.scene_id = pdaf_ptr->scene_id;
	param.sub_block = DCAM_BLOCK_PDAF;
	param.property = DCAM_PRO_PDAF_SET_ROI;
	param.property_param = pdaf_ptr->roi_info;

	ret = ioctl(file->fd, SPRD_STATIS_IO_CFG_PARAM, &param);
	return ret;
}

cmr_s32 isp_u_pdaf_ppi_info(cmr_handle handle, void *param_ptr)
{
	cmr_s32 ret = 0;
	struct isp_file *file = NULL;
	struct isp_u_blocks_info *pdaf_ptr = NULL;
	struct isp_io_param param;

	if (!handle || !param_ptr) {
		ISP_LOGE("failed to get prt: %p, %p", handle, param_ptr);
		return -1;
	}

	file = (struct isp_file *)(handle);
	pdaf_ptr = (struct isp_u_blocks_info *)param_ptr;

	param.isp_id = file->isp_id;
	param.scene_id = pdaf_ptr->scene_id;
	param.sub_block = DCAM_BLOCK_PDAF;
	param.property = DCAM_PRO_PDAF_SET_PPI_INFO;
	param.property_param = pdaf_ptr->ppi_info;

	ret = ioctl(file->fd, SPRD_STATIS_IO_CFG_PARAM, &param);
	return ret;
}

cmr_s32 isp_u_pdaf_block(cmr_handle handle, void *param_ptr)
{
	cmr_s32 ret = 0;
	struct isp_file *file = NULL;
	struct isp_u_blocks_info *pdaf_ptr = NULL;
	struct isp_io_param param;

	if (!handle || !param_ptr) {
		ISP_LOGE("failed to get prt: %p, %p", handle, param_ptr);
		return -1;
	}

	file = (struct isp_file *)(handle);
	pdaf_ptr = (struct isp_u_blocks_info *)param_ptr;

	param.isp_id = file->isp_id;
	param.scene_id = pdaf_ptr->scene_id;
	param.sub_block = DCAM_BLOCK_PDAF;
	param.property = DCAM_PRO_PDAF_BLOCK;
	param.property_param = pdaf_ptr->block_info;

	ret = ioctl(file->fd, SPRD_STATIS_IO_CFG_PARAM, &param);

	return ret;
}
