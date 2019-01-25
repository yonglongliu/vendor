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

#define LOG_TAG "dcam_u_raw_aem"

#include "isp_drv.h"

cmr_s32 dcam_u_raw_aem_block(cmr_handle handle, void *block_info)
{
	cmr_s32 ret = 0;
	struct isp_file *file = NULL;
	struct isp_io_param param;

	if (!handle) {
		ISP_LOGE("handle is null error: 0x%lx 0x%lx", (cmr_uint) handle, (cmr_uint) block_info);
		return -1;
	}

	if (!block_info) {
		ISP_LOGE("block info is null");
		return ret;
	}

	file = (struct isp_file *)(handle);
	param.isp_id = file->isp_id;
	param.sub_block = DCAM_BLOCK_RAW_AEM;
	param.property = DCAM_PRO_RAW_AEM_BLOCK;
	param.property_param = block_info;

	ret = ioctl(file->fd, SPRD_STATIS_IO_CFG_PARAM, &param);

	return ret;
}

cmr_s32 dcam_u_raw_aem_mode(cmr_handle handle, cmr_u32 mode)
{
	cmr_s32 ret = 0;
	struct isp_file *file = NULL;
	struct isp_io_param param;

	if (!handle) {
		ISP_LOGE("handle is null error: 0x%lx", (cmr_uint) handle);
		return -1;
	}

	file = (struct isp_file *)(handle);
	param.isp_id = file->isp_id;
	param.sub_block = DCAM_BLOCK_RAW_AEM;
	param.property = DCAM_PRO_RAW_AEM_MODE;
	param.property_param = &mode;

	ret = ioctl(file->fd, SPRD_STATIS_IO_CFG_PARAM, &param);

	return ret;
}

cmr_s32 dcam_u_raw_aem_skip_num(cmr_handle handle, cmr_u32 skip_num)
{
	cmr_s32 ret = 0;
	struct isp_file *file = NULL;
	struct isp_io_param param;

	if (!handle) {
		ISP_LOGE("handle is null error.");
		return -1;
	}

	file = (struct isp_file *)(handle);
	param.isp_id = file->isp_id;
	param.sub_block = DCAM_BLOCK_RAW_AEM;
	param.property = DCAM_PRO_RAW_AEM_SKIP_NUM;
	param.property_param = &skip_num;

	ret = ioctl(file->fd, SPRD_STATIS_IO_CFG_PARAM, &param);

	return ret;
}

cmr_s32 dcam_u_raw_aem_shift(cmr_handle handle, void *shift)
{
	cmr_s32 ret = 0;
	struct isp_file *file = NULL;
	struct isp_io_param param;

	if (!handle) {
		ISP_LOGE("handle is null error.");
		return -1;
	}

	file = (struct isp_file *)(handle);
	param.isp_id = file->isp_id;
	param.sub_block = DCAM_BLOCK_RAW_AEM;
	param.property = DCAM_PRO_RAW_AEM_SHIFT;
	param.property_param = shift;

	ret = ioctl(file->fd, SPRD_STATIS_IO_CFG_PARAM, &param);

	return ret;
}

cmr_s32 dcam_u_raw_aem_offset(cmr_handle handle, cmr_u32 x, cmr_u32 y)
{
	cmr_s32 ret = 0;
	struct isp_file *file = NULL;
	struct isp_io_param param;
	struct sprd_aem_offset offset;

	if (!handle) {
		ISP_LOGE("handle is null error.");
		return -1;
	}

	file = (struct isp_file *)(handle);
	param.isp_id = file->isp_id;
	param.sub_block = DCAM_BLOCK_RAW_AEM;
	param.property = DCAM_PRO_RAW_AEM_OFFSET;
	offset.x = x;
	offset.y = y;
	param.property_param = &offset;

	ret = ioctl(file->fd, SPRD_STATIS_IO_CFG_PARAM, &param);

	return ret;
}

cmr_s32 dcam_u_raw_aem_blk_size(cmr_handle handle, cmr_u32 width, cmr_u32 height)
{
	cmr_s32 ret = 0;
	struct isp_file *file = NULL;
	struct isp_io_param param;
	struct sprd_aem_blk_size size;

	if (!handle) {
		ISP_LOGE("handle is null error.");
		return -1;
	}

	file = (struct isp_file *)(handle);
	param.isp_id = file->isp_id;
	param.sub_block = DCAM_BLOCK_RAW_AEM;
	param.property = DCAM_PRO_RAW_AEM_BLK_SIZE;
	size.width = width;
	size.height = height;
	param.property_param = &size;

	ret = ioctl(file->fd, SPRD_STATIS_IO_CFG_PARAM, &param);

	return ret;
}

