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

#define LOG_TAG "isp_u_comm"

#include "isp_drv.h"

cmr_s32 isp_u_comm_block(cmr_handle handle, void *block_info)
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
	param.sub_block = ISP_BLOCK_COMMON;
	param.property = ISP_PRO_COMMON_BLOCK;
	param.property_param = block_info;

	ret = ioctl(file->fd, SPRD_ISP_IO_CFG_PARAM, &param);

	return ret;
}

cmr_s32 isp_u_comm_shadow_ctrl(cmr_handle handle, cmr_u32 shadow_done)
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
	param.sub_block = ISP_BLOCK_COMMON;
	param.property = ISP_PRO_COMMON_COMM_SHADOW;
	param.property_param = &shadow_done;

	ret = ioctl(file->fd, SPRD_ISP_IO_CFG_PARAM, &param);

	return ret;
}

cmr_s32 isp_u_3a_ctrl(cmr_handle handle, cmr_u32 enable)
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
	param.sub_block = ISP_BLOCK_COMMON;
	param.property = ISP_PRO_COMMON_3A_SINGLE_FRAME_CTRL;
	param.property_param = &enable;

	ret = ioctl(file->fd, SPRD_ISP_IO_CFG_PARAM, &param);

	return ret;
}

cmr_s32 isp_u_afl_ctrl(cmr_handle handle, cmr_u32 afl_sel)
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
	param.sub_block = ISP_BLOCK_COMMON;
	param.property = ISP_PRO_COMMON_AFL_SEL;
	param.property_param = &afl_sel;

	ret = ioctl(file->fd, SPRD_ISP_IO_CFG_PARAM, &param);

	return ret;
}