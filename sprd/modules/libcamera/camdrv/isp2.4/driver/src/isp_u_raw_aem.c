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

cmr_s32 isp_u_raw_aem_block(cmr_handle handle, void *block_info)
{
	cmr_s32 ret = 0;
	struct isp_file *file = NULL;
	struct isp_io_param param;

	if (!handle) {
		ISP_LOGE("fail to get handle.");
		return -1;
	}

	if (!block_info) {
		ISP_LOGE("fail to get handle.");
		return ret;
	}

	if (((struct isp_dev_raw_aem_info *)block_info)->bypass > 1) {
		return ret;

	}
	file = (struct isp_file *)(handle);
	param.isp_id = file->isp_id;
	param.sub_block = ISP_BLOCK_RAW_AEM;
	param.property = ISP_PRO_RAW_AEM_BLOCK;
	param.property_param = block_info;

	ret = ioctl(file->fd, SPRD_ISP_IO_CFG_PARAM, &param);

	return ret;
}

cmr_s32 isp_u_raw_aem_bypass(cmr_handle handle, void *block_info)
{
	cmr_s32 ret = 0;
	struct isp_file *file = NULL;
	struct isp_io_param param;

	if (!handle || !block_info) {
		ISP_LOGE("fail to get handle: handle = %p, block_info = %p.", handle, block_info);
		return -1;
	}

	file = (struct isp_file *)(handle);
	param.isp_id = file->isp_id;
	param.sub_block = ISP_BLOCK_RAW_AEM;
	param.property = ISP_PRO_RAW_AEM_BYPASS;
	param.property_param = block_info;

	ret = ioctl(file->fd, SPRD_ISP_IO_CFG_PARAM, &param);

	return ret;
}

cmr_s32 isp_u_raw_aem_mode(cmr_handle handle, cmr_u32 mode)
{
	cmr_s32 ret = 0;
	struct isp_file *file = NULL;
	struct isp_io_param param;

	if (!handle) {
		ISP_LOGE("fail to get handle.");
		return -1;
	}

	file = (struct isp_file *)(handle);
	param.isp_id = file->isp_id;
	param.sub_block = ISP_BLOCK_RAW_AEM;
	param.property = ISP_PRO_RAW_AEM_MODE;
	param.property_param = &mode;

	ret = ioctl(file->fd, SPRD_ISP_IO_CFG_PARAM, &param);

	return ret;
}

cmr_s32 isp_u_raw_aem_skip_num(cmr_handle handle, cmr_u32 skip_num)
{
	cmr_s32 ret = 0;
	struct isp_file *file = NULL;
	struct isp_io_param param;

	if (!handle) {
		ISP_LOGE("fail to get handle.");
		return -1;
	}

	file = (struct isp_file *)(handle);
	param.isp_id = file->isp_id;
	param.sub_block = ISP_BLOCK_RAW_AEM;
	param.property = ISP_PRO_RAW_AEM_SKIP_NUM;
	param.property_param = &skip_num;

	ret = ioctl(file->fd, SPRD_ISP_IO_CFG_PARAM, &param);

	return ret;
}

cmr_s32 isp_u_raw_aem_shift(cmr_handle handle, void *shift)
{
	cmr_s32 ret = 0;
	struct isp_file *file = NULL;
	struct isp_io_param param;

	if (!handle) {
		ISP_LOGE("fail to get handle.");
		return -1;
	}
	file = (struct isp_file *)(handle);
	param.isp_id = file->isp_id;
	param.sub_block = ISP_BLOCK_RAW_AEM;
	param.property = ISP_PRO_RAW_AEM_SHIFT;
	param.property_param = shift;

	ret = ioctl(file->fd, SPRD_ISP_IO_CFG_PARAM, &param);

	return ret;
}

cmr_s32 isp_u_raw_aem_offset(cmr_handle handle, cmr_u32 x, cmr_u32 y)
{
	cmr_s32 ret = 0;
	struct isp_file *file = NULL;
	struct isp_io_param param;
	struct img_offset offset;

	if (!handle) {
		ISP_LOGE("fail to get handle.");
		return -1;
	}

	file = (struct isp_file *)(handle);
	param.isp_id = file->isp_id;
	param.sub_block = ISP_BLOCK_RAW_AEM;
	param.property = ISP_PRO_RAW_AEM_OFFSET;
	offset.x = x;
	offset.y = y;
	param.property_param = &offset;

	ret = ioctl(file->fd, SPRD_ISP_IO_CFG_PARAM, &param);

	return ret;
}

cmr_s32 isp_u_raw_aem_blk_size(cmr_handle handle, cmr_u32 width, cmr_u32 height)
{
	cmr_s32 ret = 0;
	struct isp_file *file = NULL;
	struct isp_io_param param;
	struct isp_img_size size;

	if (!handle) {
		ISP_LOGE("fail to get handle.");
		return -1;
	}

	file = (struct isp_file *)(handle);
	param.isp_id = file->isp_id;
	param.sub_block = ISP_BLOCK_RAW_AEM;
	param.property = ISP_PRO_RAW_AEM_BLK_SIZE;
	size.width = width;
	size.height = height;
	param.property_param = &size;

	ret = ioctl(file->fd, SPRD_ISP_IO_CFG_PARAM, &param);

	return ret;
}

cmr_s32 isp_u_raw_aem_slice_size(cmr_handle handle, cmr_u32 width, cmr_u32 height)
{
	cmr_s32 ret = 0;
	struct isp_file *file = NULL;
	struct isp_io_param param;
	struct isp_img_size size;

	if (!handle) {
		ISP_LOGE("fail to get handle.");
		return -1;
	}

	file = (struct isp_file *)(handle);
	param.isp_id = file->isp_id;
	param.sub_block = ISP_BLOCK_RAW_AEM;
	param.property = ISP_PRO_RAW_AEM_SLICE_SIZE;
	size.width = width;
	size.height = height;
	param.property_param = &size;

	ret = ioctl(file->fd, SPRD_ISP_IO_CFG_PARAM, &param);

	return ret;
}
