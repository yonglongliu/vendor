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

#define LOG_TAG "isp_u_brightness"

#include "isp_drv.h"

cmr_s32 isp_u_brightness_block(cmr_handle handle, void *param_ptr)
{
	cmr_s32 ret = 0;
	struct isp_file *file = NULL;
	struct isp_u_blocks_info *brightness_ptr = NULL;
	struct isp_io_param param;

	if (!handle || !param_ptr) {
		ISP_LOGE("failed to get ptr: %p, %p", handle, param_ptr);
		return -1;
	}

	file = (struct isp_file *)(handle);
	brightness_ptr = (struct isp_u_blocks_info *)param_ptr;

	param.isp_id = file->isp_id;
	param.scene_id = brightness_ptr->scene_id;
	param.sub_block = ISP_BLOCK_BRIGHTNESS;
	param.property = ISP_PRO_BRIGHT_BLOCK;
	param.property_param = brightness_ptr->block_info;

	ret = ioctl(file->fd, SPRD_ISP_IO_CFG_PARAM, &param);

	return ret;
}
