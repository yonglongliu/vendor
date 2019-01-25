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

#define LOG_TAG "dcam_u_2d_lsc"

#include "isp_drv.h"

#define ISP_LSC_BUF_SIZE  (32 * 1024)


cmr_s32 dcam_u_2d_lsc_block(cmr_handle handle, void *param_ptr)
{
	cmr_s32 ret = 0;
	cmr_uint buf_addr;
	struct isp_io_param param;
	struct isp_file *file = NULL;
	struct isp_dev_2d_lsc_info *lens_info = NULL;
	struct isp_u_blocks_info *lsc_2d_ptr = NULL;

	if (!handle || !param_ptr) {
		ISP_LOGE("failed to get ptr: %p, %p", handle, param_ptr);
		return -1;
	}

	file = (struct isp_file *)(handle);
	lsc_2d_ptr = (struct isp_u_blocks_info *)param_ptr;

	lens_info = (struct isp_dev_2d_lsc_info *)(lsc_2d_ptr->block_info);
	if (!lens_info) {
		ISP_LOGE("failed to get lens_info ptr!");
		return -1;
	}

#if __WORDSIZE == 64
	buf_addr = ((cmr_uint) lens_info->buf_addr[1] << 32) | lens_info->buf_addr[0];
#else
	buf_addr = lens_info->buf_addr[0];
#endif

	if (0 == file->dcam_lsc_vaddr || 0 == buf_addr || ISP_LSC_BUF_SIZE < lens_info->buf_len) {
		ISP_LOGE("lsc memory error: 0x%p %lx %x", file->dcam_lsc_vaddr, buf_addr, lens_info->buf_len);
		return -1;
	} else {
		memcpy((void *)file->dcam_lsc_vaddr, (void *)buf_addr, lens_info->buf_len);
	}

	param.isp_id = file->isp_id;
	param.sub_block = DCAM_BLOCK_2D_LSC;
	param.property = DCAM_PRO_2D_LSC_BLOCK;
	param.property_param = lens_info;

	ret = ioctl(file->fd, SPRD_STATIS_IO_CFG_PARAM, &param);

	return ret;
}

cmr_s32 dcam_u_2d_lsc_transaddr(cmr_handle handle, struct isp_statis_buf_input * buf)
{
	cmr_s32 ret = 0;
	struct isp_file *file = NULL;
	struct isp_io_param param;
	struct isp_dev_block_addr lsc_buf;

	if (!handle) {
		ISP_LOGE("handle is null error.");
		return -1;
	}

	memset(&lsc_buf, 0, sizeof(lsc_buf));
	file = (struct isp_file *)(handle);
	param.isp_id = file->isp_id;
	param.sub_block = DCAM_BLOCK_2D_LSC;
	param.property = DCAM_PRO_2D_LSC_TRANSADDR;
	lsc_buf.img_fd = buf->mfd;
	param.property_param = &lsc_buf;

	ret = ioctl(file->fd, SPRD_STATIS_IO_CFG_PARAM, &param);

	return ret;
}
