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

#define LOG_TAG "isp_u_pdaf"

#include "isp_drv.h"

cmr_s32 isp_u_pdaf_bypass(cmr_handle handle, cmr_u32 *bypass)
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
	param.sub_block = ISP_BLOCK_PDAF;
	param.property = ISP_PRO_PDAF_BYPASS;
	param.property_param = bypass;

	ret = ioctl(file->fd, SPRD_ISP_IO_CFG_PARAM, &param);
	return ret;
}

cmr_s32 isp_u_pdaf_work_mode(cmr_handle handle, cmr_u32 *work_mode)
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
	param.sub_block = ISP_BLOCK_PDAF;
	param.property = ISP_PRO_PDAF_SET_MODE;
	param.property_param = work_mode;

	ret = ioctl(file->fd, SPRD_ISP_IO_CFG_PARAM, &param);
	return ret;
}

cmr_s32 isp_u_pdaf_extractor_bypass(cmr_handle handle, cmr_u32 *bypass)
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
	param.sub_block = ISP_BLOCK_PDAF;
	param.property = ISP_PRO_PDAF_SET_EXTRACTOR_BYPASS;
	param.property_param = bypass;

	ret = ioctl(file->fd, SPRD_ISP_IO_CFG_PARAM, &param);
	return ret;
}

cmr_s32 isp_u_pdaf_skip_num(cmr_handle handle, cmr_u32 *skip_num)
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
	param.sub_block = ISP_BLOCK_PDAF;
	param.property = ISP_PRO_PDAF_SET_SKIP_NUM;
	param.property_param = skip_num;

	ret = ioctl(file->fd, SPRD_ISP_IO_CFG_PARAM, &param);
	return ret;
}

cmr_s32 isp_u_pdaf_roi(cmr_handle handle, void *roi_info)
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
	param.sub_block = ISP_BLOCK_PDAF;
	param.property = ISP_PRO_PDAF_SET_ROI;
	param.property_param = roi_info;

	ret = ioctl(file->fd, SPRD_ISP_IO_CFG_PARAM, &param);
	return ret;
}

cmr_s32 isp_u_pdaf_ppi_info(cmr_handle handle, void *ppi_info)
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
	param.sub_block = ISP_BLOCK_PDAF;
	param.property = ISP_PRO_PDAF_SET_PPI_INFO;
	param.property_param = ppi_info;

	ret = ioctl(file->fd, SPRD_ISP_IO_CFG_PARAM, &param);
	return ret;
}

cmr_s32 isp_u_pdaf_block(cmr_handle handle, void *block_info)
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
	param.sub_block = ISP_BLOCK_PDAF;
	param.property = ISP_PRO_PDAF_BLOCK;
	param.property_param = block_info;

	ret = ioctl(file->fd, SPRD_ISP_IO_CFG_PARAM, &param);

	return ret;
}
