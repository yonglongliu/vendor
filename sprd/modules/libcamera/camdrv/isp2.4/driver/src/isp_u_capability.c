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

#define LOG_TAG "isp_u_capability"

#include "isp_drv.h"

cmr_s32 isp_u_capability_continue_size(cmr_handle handle, cmr_u32 * width, cmr_u32 * height)
{
	cmr_s32 ret = 0;
	struct isp_file *file = NULL;
	struct isp_img_size size;
	struct isp_capability param;

	if (!handle) {
		ISP_LOGE("fail to get handle.");
		return -1;
	}

	file = (struct isp_file *)(handle);
	param.isp_id = file->isp_id;
	param.index = ISP_CAPABILITY_CONTINE_SIZE;
	param.property_param = &size;
	ret = ioctl(file->fd, SPRD_ISP_IO_CAPABILITY, &param);

	if (0 == ret) {
		*width = size.width;
		*height = size.height;
	} else {
		*width = 0;
		*height = 0;
		ISP_LOGE("fail to get continue size.");
	}

	return ret;
}

cmr_s32 isp_u_capability_time(cmr_handle handle, cmr_u32 * sec, cmr_u32 * usec)
{
	cmr_s32 ret = 0;
	struct isp_file *file = NULL;
	struct isp_capability param;
	struct isp_time time = { 0, 0 };

	if (!handle || !sec || !usec) {
		ISP_LOGE("fail to get handle: handle = %p sec = %p usec = %p.", handle, sec, usec);
		return -1;
	}

	file = (struct isp_file *)(handle);
	param.isp_id = file->isp_id;
	param.index = ISP_CAPABILITY_TIME;
	param.property_param = &time;
	ret = ioctl(file->fd, SPRD_ISP_IO_CAPABILITY, &param);

	if (ret) {
		*sec = 0;
		*usec = 0;
		ISP_LOGE("fail to get time.");
	} else {
		*sec = time.sec;
		*usec = time.usec;
	}

	return ret;
}
