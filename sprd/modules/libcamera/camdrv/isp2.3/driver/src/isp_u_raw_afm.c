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

#define LOG_TAG "isp_u_raw_afm"

#include "isp_drv.h"

cmr_s32 isp_u_raw_afm_block(cmr_handle handle, void *param_ptr)
{
	cmr_s32 ret = 0;
	struct isp_file *file = NULL;
	struct isp_u_blocks_info *raw_afm_ptr = NULL;
	struct isp_io_param param;

	if (!handle || !param_ptr) {
		ISP_LOGE("failed to get ptr: %p, %p", handle, param_ptr);
		return -1;
	}

	file = (struct isp_file *)(handle);
	raw_afm_ptr = (struct isp_u_blocks_info *)param_ptr;

	param.isp_id = file->isp_id;
	param.scene_id = raw_afm_ptr->scene_id;
	param.sub_block = ISP_BLOCK_RAW_AFM;
	param.property = ISP_PRO_RGB_AFM_BLOCK;
	param.property_param = raw_afm_ptr->block_info;

	ret = ioctl(file->fd, SPRD_ISP_IO_CFG_PARAM, &param);
	return ret;
}

cmr_s32 isp_u_raw_afm_slice_size(cmr_handle handle, void *param_ptr)
{
	cmr_s32 ret = 0;
	struct isp_file *file = NULL;
	struct isp_u_blocks_info *raw_afm_ptr = NULL;
	struct isp_io_param param;
	struct isp_img_size size;

	if (!handle || !param_ptr) {
		ISP_LOGE("failed to get ptr: %p, %p", handle, param_ptr);
		return -1;
	}

	file = (struct isp_file *)(handle);
	raw_afm_ptr = (struct isp_u_blocks_info *)param_ptr;

	size.width = raw_afm_ptr->size.width;
	size.height = raw_afm_ptr->size.height;

	param.isp_id = file->isp_id;
	param.scene_id = raw_afm_ptr->scene_id;
	param.sub_block = ISP_BLOCK_RAW_AFM;
	param.property = ISP_PRO_RGB_AFM_FRAME_SIZE;
	param.property_param = &size;

	ret = ioctl(file->fd, SPRD_ISP_IO_CFG_PARAM, &param);
	return ret;
}

cmr_s32 isp_u_raw_afm_iir_nr_cfg(cmr_handle handle, void *param_ptr)
{
	cmr_s32 ret = 0;

	struct isp_file *file = NULL;
	struct isp_u_blocks_info *raw_afm_ptr = NULL;
	struct isp_io_param param;

	if (!handle || !param_ptr) {
		ISP_LOGE("failed to get ptr: %p, %p", handle, param_ptr);
		return -1;
	}

	file = (struct isp_file *)(handle);
	raw_afm_ptr = (struct isp_u_blocks_info *)param_ptr;

	param.isp_id = file->isp_id;
	param.scene_id = raw_afm_ptr->scene_id;
	param.sub_block = ISP_BLOCK_RAW_AFM;
	param.property = ISP_PRO_RGB_AFM_IIR_NR_CFG;
	param.property_param = raw_afm_ptr->block_info;

	ret = ioctl(file->fd, SPRD_ISP_IO_CFG_PARAM, &param);

	return ret;
}

cmr_s32 isp_u_raw_afm_modules_cfg(cmr_handle handle, void *param_ptr)
{
	cmr_s32 ret = 0;

	struct isp_file *file = NULL;
	struct isp_u_blocks_info *raw_afm_ptr = NULL;
	struct isp_io_param param;

	if (!handle || !param_ptr) {
		ISP_LOGE("failed to get ptr: %p, %p", handle, param_ptr);
		return -1;
	}

	file = (struct isp_file *)(handle);
	raw_afm_ptr = (struct isp_u_blocks_info *)param_ptr;

	param.isp_id = file->isp_id;
	param.scene_id = raw_afm_ptr->scene_id;
	param.sub_block = ISP_BLOCK_RAW_AFM;
	param.property = ISP_PRO_RGB_AFM_MODULE_CFG;
	param.property_param = raw_afm_ptr->block_info;

	ret = ioctl(file->fd, SPRD_ISP_IO_CFG_PARAM, &param);

	return ret;
}

cmr_s32 isp_u_raw_afm_bypass(cmr_handle handle, void *param_ptr)
{
	cmr_s32 ret = 0;
	struct isp_file *file = NULL;
	struct isp_u_blocks_info *raw_afm_ptr = NULL;
	struct isp_io_param param;

	if (!handle || !param_ptr) {
		ISP_LOGE("failed to get ptr: %p, %p", handle, param_ptr);
		return -1;
	}

	file = (struct isp_file *)(handle);
	raw_afm_ptr = (struct isp_u_blocks_info *)param_ptr;

	param.isp_id = file->isp_id;
	param.scene_id = raw_afm_ptr->scene_id;
	param.sub_block = ISP_BLOCK_RAW_AFM;
	param.property = ISP_PRO_RGB_AFM_BYPASS;
	param.property_param = &raw_afm_ptr->afm_info.bypass;

	ret = ioctl(file->fd, SPRD_ISP_IO_CFG_PARAM, &param);
	return ret;
}

cmr_s32 isp_u_raw_afm_mode(cmr_handle handle, void *param_ptr)
{
	cmr_s32 ret = 0;

	struct isp_file *file = NULL;
	struct isp_u_blocks_info *raw_afm_ptr = NULL;
	struct isp_io_param param;

	if (!handle || !param_ptr) {
		ISP_LOGE("failed to get ptr: %p, %p", handle, param_ptr);
		return -1;
	}

	file = (struct isp_file *)(handle);
	raw_afm_ptr = (struct isp_u_blocks_info *)param_ptr;

	param.isp_id = file->isp_id;
	param.scene_id = raw_afm_ptr->scene_id;
	param.sub_block = ISP_BLOCK_RAW_AFM;
	param.property = ISP_PRO_RGB_AFM_MODE;
	param.property_param = &raw_afm_ptr->mode;

	ret = ioctl(file->fd, SPRD_ISP_IO_CFG_PARAM, &param);

	return ret;
}

cmr_s32 isp_u_raw_afm_skip_num(cmr_handle handle, void *param_ptr)
{
	cmr_s32 ret = 0;

	struct isp_file *file = NULL;
	struct isp_u_blocks_info *raw_afm_ptr = NULL;
	struct isp_io_param param;

	if (!handle || !param_ptr) {
		ISP_LOGE("failed to get ptr: %p, %p", handle, param_ptr);
		return -1;
	}

	file = (struct isp_file *)(handle);
	raw_afm_ptr = (struct isp_u_blocks_info *)param_ptr;

	param.isp_id = file->isp_id;
	param.scene_id = raw_afm_ptr->scene_id;
	param.sub_block = ISP_BLOCK_RAW_AFM;
	param.property = ISP_PRO_RGB_AFM_SKIP_NUM;
	param.property_param = &raw_afm_ptr->afm_info.skip_num;

	ret = ioctl(file->fd, SPRD_ISP_IO_CFG_PARAM, &param);

	return ret;
}

cmr_s32 isp_u_raw_afm_skip_num_clr(cmr_handle handle, void *param_ptr)
{
	cmr_s32 ret = 0;
	struct isp_file *file = NULL;
	struct isp_u_blocks_info *raw_afm_ptr = NULL;
	struct isp_io_param param;

	if (!handle || !param_ptr) {
		ISP_LOGE("failed to get ptr: %p, %p", handle, param_ptr);
		return -1;
	}

	file = (struct isp_file *)(handle);
	raw_afm_ptr = (struct isp_u_blocks_info *)param_ptr;

	param.isp_id = file->isp_id;
	param.scene_id = raw_afm_ptr->scene_id;
	param.sub_block = ISP_BLOCK_RAW_AFM;
	param.property = ISP_PRO_RGB_AFM_SKIP_NUM_CLR;
	param.property_param = &raw_afm_ptr->clear;

	ret = ioctl(file->fd, SPRD_ISP_IO_CFG_PARAM, &param);

	return ret;
}

cmr_s32 isp_u_raw_afm_win(cmr_handle handle, void *param_ptr)
{
	cmr_s32 ret = 0;
	struct isp_file *file = NULL;
	struct isp_u_blocks_info *raw_afm_ptr = NULL;
	struct isp_io_param param;

	if (!handle || !param_ptr) {
		ISP_LOGE("failed to get ptr: %p, %p", handle, param_ptr);
		return -1;
	}

	file = (struct isp_file *)(handle);
	raw_afm_ptr = (struct isp_u_blocks_info *)param_ptr;

	param.isp_id = file->isp_id;
	param.scene_id = raw_afm_ptr->scene_id;
	param.sub_block = ISP_BLOCK_RAW_AFM;
	param.property = ISP_PRO_RGB_AFM_WIN;
	param.property_param = raw_afm_ptr->win_range;

	ret = ioctl(file->fd, SPRD_ISP_IO_CFG_PARAM, &param);

	return ret;
}

cmr_s32 isp_u_raw_afm_win_num(cmr_handle handle, void *param_ptr)
{
	cmr_s32 ret = 0;
	struct isp_file *file = NULL;
	struct isp_u_blocks_info *raw_afm_ptr = NULL;
	struct isp_io_param param;

	if (!handle || !param_ptr) {
		ISP_LOGE("failed to get ptr: %p, %p", handle, param_ptr);
		return -1;
	}

	file = (struct isp_file *)(handle);
	raw_afm_ptr = (struct isp_u_blocks_info *)param_ptr;

	param.isp_id = file->isp_id;
	param.scene_id = BLOCK_SCENE_DEF;
	param.sub_block = ISP_BLOCK_RAW_AFM;
	param.property = ISP_PRO_RGB_AFM_WIN_NUM;
	param.property_param = (void *)raw_afm_ptr->win_num;

	ret = ioctl(file->fd, SPRD_ISP_IO_CFG_PARAM, &param);

	return ret;
}
