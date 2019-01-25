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

#define LOG_TAG "isp_u_dev"

#include "isp_drv.h"

#define ISP_REGISTER_MAX_NUM 20

cmr_s32 isp_dev_open(cmr_s32 fd, isp_handle * handle)
{
	cmr_s32 ret = 0;
	cmr_u32 chip_id = 0;
	struct isp_file *file = NULL;

	file = malloc(sizeof(struct isp_file));
	if (!file) {
		ret = -1;
		ISP_LOGE("alloc memory error.");
		return ret;
	}

	if (fd < 0) {
		ret = -1;
		ISP_LOGE("there is something wrong about opening device.");
		goto isp_free;
	}

	file->fd = fd;
	file->isp_id = 0;
	*handle = (isp_handle) file;
#if 0
	ret = isp_u_capability_chip_id(*handle, &chip_id);
	if (ret) {
		ret = -1;
		ISP_LOGE("get chip id error.");
		goto isp_free;
	} else {
		file->chip_id = chip_id;
		*handle = (isp_handle) file;
	}
#endif
	return ret;

isp_free:
	if (file)
		free((void *)file);
	file = NULL;

	return ret;
}

cmr_s32 isp_dev_close(isp_handle handle)
{
	cmr_s32 ret = 0;
	struct isp_file *file = NULL;

	if (!handle) {
		ISP_LOGE("file hand is null error.");
		ret = -1;
		return ret;
	}

	file = (struct isp_file *)handle;

	if (file) {
		free((void *)file);
		file = NULL;
	}

	return ret;
}

cmr_u32 isp_dev_get_chip_id(isp_handle handle)
{
	struct isp_file *file = NULL;

	if (!handle) {
		ISP_LOGE("handle is null error.");
		return ISP_CHIP_ID_INVALID;
	}

	file = (struct isp_file *)(handle);

	return file->chip_id;
}

cmr_s32 isp_dev_reset(isp_handle handle)
{
	cmr_s32 ret = 0;
	cmr_u32 isp_id = 0;
	struct isp_file *file = NULL;

	if (!handle) {
		ISP_LOGE("handle is null error.");
		return -1;
	}

	file = (struct isp_file *)(handle);
	isp_id = file->isp_id;

	ret = ioctl(file->fd, SPRD_ISP_IO_RST, &isp_id);
	if (ret) {
		ISP_LOGE("isp hardawre reset error.");
	}

	return ret;
}

cmr_s32 isp_dev_stop(isp_handle handle)
{
	cmr_s32 ret = 0;
	cmr_u32 isp_id = 0;
	struct isp_file *file = NULL;

	if (!handle) {
		ISP_LOGE("handle is null error.");
		return -1;
	}

	file = (struct isp_file *)(handle);
	isp_id = file->isp_id;

	ret = ioctl(file->fd, SPRD_ISP_IO_STOP, &isp_id);
	if (ret) {
		ISP_LOGE("isp hardawre stop error.");
	}

	return ret;
}

cmr_s32 isp_dev_enable_irq(isp_handle handle, cmr_u32 irq_mode)
{
	cmr_s32 ret = 0;
	struct isp_file *file = NULL;
	struct isp_interrupt int_param;

	if (!handle) {
		ISP_LOGE("handle is null error.");
		return -1;
	}

	file = (struct isp_file *)(handle);
	int_param.isp_id = file->isp_id;
	int_param.int_mode = irq_mode;

	ret = ioctl(file->fd, SPRD_ISP_IO_INT, (void *)&int_param);
	if (ret) {
		ISP_LOGE("isp enable interrupt error.");
	}

	return ret;
}

cmr_s32 isp_dev_get_irq(isp_handle handle, cmr_u32 * evt_ptr)
{
	cmr_s32 ret = 0;
	struct isp_file *file = NULL;
	struct isp_irq *ptr = (struct isp_irq *)evt_ptr;

	if (!handle) {
		ISP_LOGE("handle is null error.");
		return -1;
	}

	file = (struct isp_file *)(handle);

	while (1) {
		ret = ioctl(file->fd, SPRD_ISP_IO_IRQ, ptr);
		if (0 == ret) {
			break;
		} else {
			if (-EINTR == ptr->ret_val) {
				usleep(5000);
				ISP_LOGE("continue.");
				continue;
			}
			ISP_LOGE("ret_val=%d", ptr->ret_val);
			break;
		}
	}

	return ret;
}

static cmr_s32 isp_register_write(isp_handle handle, cmr_u32 * param)
{
	cmr_s32 ret = 0;
	struct isp_file *file = NULL;

	if (!handle) {
		ISP_LOGE("handle is null error.");
		return -1;
	}

	file = (struct isp_file *)(handle);

	ret = ioctl(file->fd, SPRD_ISP_IO_WRITE, param);
	if (ret) {
		ISP_LOGE("register write error.");
	}

	return ret;
}

static cmr_s32 isp_register_read(isp_handle handle, cmr_u32 * param)
{
	cmr_s32 ret = 0;
	struct isp_file *file = NULL;

	if (!handle) {
		ISP_LOGE("handle is null error.");
		return -1;
	}

	file = (struct isp_file *)(handle);

	ret = ioctl(file->fd, SPRD_ISP_IO_READ, param);
	if (ret) {
		ISP_LOGE("register read error.");
	}

	return ret;
}

cmr_s32 isp_dev_reg_write(isp_handle handle, cmr_u32 num, void *param_ptr)
{
	cmr_s32 ret = 0;
	cmr_u32 reg_num = num, i = 0;
	cmr_u32 *reg_ptr = (cmr_u32 *) param_ptr;
	struct isp_reg_bits reg_config[ISP_REGISTER_MAX_NUM];
	struct isp_reg_param write_param;

	if (!handle) {
		ISP_LOGE("handle is null error.");
		return -1;
	}

	if (ISP_REGISTER_MAX_NUM < reg_num) {
		reg_num = ISP_REGISTER_MAX_NUM;
	}

	memset(&reg_config, 0x00, sizeof(reg_config));

	for (i = 0x00; i < reg_num; i++) {
		reg_config[i].reg_addr = (*reg_ptr++) & 0xFFFF;
		reg_config[i].reg_value = *reg_ptr++;
	}

	write_param.reg_param = (cmr_uint) & reg_config[0];
	write_param.counts = reg_num;

	ret = isp_register_write(handle, (cmr_u32 *) & write_param);
	if (ret) {
		ISP_LOGE("dev reg write error.");
	}

	return ret;
}

cmr_s32 isp_dev_reg_read(isp_handle handle, cmr_u32 num, void *param_ptr)
{
	cmr_s32 ret = 0;
	cmr_u32 reg_num = num, i = 0;
	cmr_u32 *reg_ptr = (cmr_u32 *) param_ptr;
	cmr_u32 reg_addr = reg_ptr[0] & 0xFFFF;
	struct isp_reg_bits reg_config[ISP_REGISTER_MAX_NUM];
	struct isp_reg_param read_param;

	if (!handle) {
		ISP_LOGE("handle is null error.");
		return -1;
	}

	if (ISP_REGISTER_MAX_NUM < reg_num) {
		reg_num = ISP_REGISTER_MAX_NUM;
	}

	memset(&reg_config, 0x00, sizeof(reg_config));

	for (i = 0x00; i < reg_num; i++) {
		reg_config[i].reg_addr = reg_addr + i * 0x04;
	}

	read_param.reg_param = (cmr_uint) & reg_config[0];
	read_param.counts = reg_num;

	ret = isp_register_read(handle, (cmr_u32 *) & read_param);
	if (0 == ret) {
		reg_addr = reg_ptr[0];
		for (i = 0; i < reg_num; i++) {
			*reg_ptr++ = reg_addr + i * 0x04;
			*reg_ptr++ = reg_config[i].reg_value;
		}
	} else {
		ISP_LOGE("dev reg read error.");
	}

	return ret;
}

cmr_s32 isp_dev_reg_fetch(isp_handle handle, cmr_u32 base_offset, cmr_u32 * buf, cmr_u32 len)
{
	cmr_s32 ret = 0;
	cmr_u32 offset_addr = base_offset, i = 0;
	struct isp_reg_bits *reg_config = (struct isp_reg_bits *)buf;
	struct isp_reg_param read_param;

	if (!handle) {
		ISP_LOGE("handle is null error.");
		return -1;
	}

	for (i = 0; i < len; i++) {
		reg_config[i].reg_addr = offset_addr;
		offset_addr += 4;
	}

	read_param.reg_param = (cmr_uint) reg_config;
	read_param.counts = len;

	ret = isp_register_read(handle, (cmr_u32 *) & read_param);
	if (ret) {
		ISP_LOGE("dev reg fetch error.");
	}

	return ret;
}

cmr_s32 isp_dev_set_statis_buf(isp_handle handle, struct isp_statis_buf_input * param)
{
	cmr_s32 ret = 0;
	struct isp_file *file = NULL;

	if (!handle) {
		ISP_LOGE("handle is null error.");
		return -1;
	}
	if (!param) {
		ISP_LOGE("Param is null error.");
		return -1;
	}

	file = (struct isp_file *)(handle);

	ret = ioctl(file->fd, SPRD_ISP_IO_SET_STATIS_BUF, param);
	if (ret) {
		ISP_LOGE("isp_dev_set_statis_buf error. 0x%x", ret);
	}

	return ret;
}

cmr_s32 isp_dev_3dnr(isp_handle handle, struct isp_3dnr_info *param)
{
	cmr_s32 ret = 0;
	struct isp_file *file = NULL;
	struct isp_3dnr_param isp_3dnr;

	if (!handle) {
		ISP_LOGE("handle is null error.");
		return -1;
	}
	if (!param) {
		ISP_LOGE("Param is null error.");
		return -1;
	}

	file = (struct isp_file *)(handle);

	isp_3dnr.mv_x = param->mv_x;
	isp_3dnr.mv_y = param->mv_y;
	isp_3dnr.fetch_ref_addr = (unsigned int)param->image[0].buffer;
	isp_3dnr.fetch_cur_addr = (unsigned int)param->image[1].buffer;
	isp_3dnr.store_ref_addr = (unsigned int)param->image[2].buffer;
	isp_3dnr.fetch_ref_addr_fd = (signed int)param->image[0].fd;
	isp_3dnr.fetch_cur_addr_fd = (signed int)param->image[1].fd;
	isp_3dnr.store_ref_addr_fd = (signed int)param->image[2].fd;
	isp_3dnr.fetch_cur_endian = 0;
	isp_3dnr.fetch_ref_endian = 0;
	isp_3dnr.store_ref_endian = 0;
	isp_3dnr.image_width = param->width;
	isp_3dnr.image_height = param->height;
	isp_3dnr.blending_no = param->blending_no;

	ISP_LOGI("ref buf: 0x%x, 0x%x, cur buf: 0x%x, 0x%x, store buf: 0x%x, 0x%x,",
		isp_3dnr.fetch_ref_addr,isp_3dnr.fetch_ref_addr_fd,
		isp_3dnr.fetch_cur_addr,isp_3dnr.fetch_cur_addr_fd,
		isp_3dnr.store_ref_addr,isp_3dnr.store_ref_addr_fd);

	ISP_LOGI("w,h, mv, no: %d, %d, %d, %d, %d.\n",
		isp_3dnr.image_width,isp_3dnr.image_height,isp_3dnr.mv_x, isp_3dnr.mv_y, isp_3dnr.blending_no);

	ret = ioctl(file->fd, SPRD_ISP_IO_POST_3DNR, &isp_3dnr);
	if (ret) {
		ISP_LOGE("isp_dev_3dnr error. 0x%x", ret);
	}

	return ret;
}
