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

#define LOG_TAG "isp_u_2d_lsc"

#include "isp_drv.h"
#include "math.h"

#define ISP_LSC_BUF_SIZE                             (32 * 1024)

#define CLIP1(input,top, bottom) {if (input>top) input = top; if (input < bottom) input = bottom;}
#define MAX_TABLE_SIZE 129

typedef struct lnc_bicubic_weight_t_64_tag {
	cmr_s16 w0;
	cmr_s16 w1;
	cmr_s16 w2;
} LNC_BICUBIC_WEIGHT_TABLE_T;

LNC_BICUBIC_WEIGHT_TABLE_T lnc_bicubic_weight_simple[129];

void generate_bicubic_weight_table(LNC_BICUBIC_WEIGHT_TABLE_T lnc_bicubic_weight_t_simple[129],cmr_u16 LNC_GRID)
{
	double PRECISION = 1024;
	double param[4][4] =
	{
		{0, 2, 0, 0},
		{-1,0, 1, 0},
		{2, -5, 4, -1},
		{-1, 3, -3, 1},
	};

	double t,matrix_result;
	cmr_u16 relative;

	//init
	for (relative = 0; relative < MAX_TABLE_SIZE; relative++) {
		lnc_bicubic_weight_t_simple[relative].w0 = 0;
		lnc_bicubic_weight_t_simple[relative].w1 = 0;
		lnc_bicubic_weight_t_simple[relative].w2 = 0;
	}

	//generate weight table based on current LNC_GRID
	for (relative = 0; relative < ((LNC_GRID/2)+1); relative++) {
		t = relative * 1.0 / LNC_GRID;

		matrix_result = 1*param[0][0] + t*param[1][0] + t*t*param[2][0] + t*t*t*param[3][0];
		lnc_bicubic_weight_t_simple[relative].w0 = (cmr_s16)(floor((0.5 * matrix_result * PRECISION) +0.5));

		matrix_result = 1*param[0][1] + t*param[1][1] + t*t*param[2][1] + t*t*t*param[3][1];
		lnc_bicubic_weight_t_simple[relative].w1 = (cmr_s16)(floor((0.5 * matrix_result * PRECISION) +0.5));

		matrix_result = 1*param[0][2] + t*param[1][2] + t*t*param[2][2] + t*t*t*param[3][2];
		lnc_bicubic_weight_t_simple[relative].w2 = (cmr_s16)(floor((0.5 * matrix_result * PRECISION) +0.5));
	}
}

cmr_u16 cubic_1D( cmr_u16 a, cmr_u16 b, cmr_u16 c, cmr_u16 d, cmr_u16 u, cmr_u16 box, LNC_BICUBIC_WEIGHT_TABLE_T *lnc_bicubic_weight_t_simple)
{
	cmr_s32 out_value;
	cmr_u16 out_value_uint16;
	cmr_s16 w0, w1, w2, w3;

	cmr_u32 out_value_tmp;
	cmr_s16 sub_tmp0;
	cmr_s16 sub_tmp1;
	cmr_s16 sub_tmp2;

	//use simple table
	if (u < (box/2 + 1)) {
		w0 = lnc_bicubic_weight_t_simple[u].w0;
		w1 = lnc_bicubic_weight_t_simple[u].w1;
		w2 = lnc_bicubic_weight_t_simple[u].w2;

		sub_tmp0 = a-d;
		sub_tmp1 = b-d;
		sub_tmp2 = c-d;
		out_value_tmp = ((cmr_u32)d)<<10;
		out_value = out_value_tmp + sub_tmp0 * w0  + sub_tmp1 * w1 + sub_tmp2 * w2;
	} else {
		w1 = lnc_bicubic_weight_t_simple[box - u].w2;
		w2 = lnc_bicubic_weight_t_simple[box - u].w1;
		w3 = lnc_bicubic_weight_t_simple[box - u].w0;

		sub_tmp0 = b-a;
		sub_tmp1 = c-a;
		sub_tmp2 = d-a;
		out_value_tmp = ((cmr_u32)a)<<10;
		out_value = out_value_tmp + sub_tmp0 * w1  + sub_tmp1 * w2 + sub_tmp2 * w3;
	}

	CLIP1(out_value, 16383*1024, 1024*1024);	// for LSC gain, 1024 = 1.0, 16383 = 16.0 ; 16383 is for boundary extension, 14 bit parameter is used.

	out_value_uint16 = (cmr_u16)((out_value + 512) >> 10);

	return out_value_uint16;
}

static cmr_s32 ISP_GenerateQValues(cmr_u32 word_endian, cmr_u32 q_val[][5], cmr_uint param_address, cmr_u16 grid_w, cmr_u16 grid_num, cmr_u16 u)
{
	cmr_u8 i;
	cmr_u16 a0 = 0, b0 = 0, c0 = 0, d0 = 0;
	cmr_u16 a1 = 0, b1 = 0, c1 = 0, d1 = 0;
	cmr_u32 *addr = (cmr_u32 *) param_address;

	if (param_address == 0x0 || grid_num == 0x0) {
		ISP_LOGE("fail to get ISP_GenerateQValues param_address : addr=0x%lx grid_num=%d \n", param_address, grid_num);
		return -1;
	}

	generate_bicubic_weight_table(lnc_bicubic_weight_simple, grid_num);
	for (i = 0; i < 5; i++) {
		if (1 == word_endian) {	// ABCD = 1 word
			a0 = *(addr + i * 2) >> 16;	// AB
			b0 = *(addr + i * 2 + grid_w * 2) >> 16;	// AB
			c0 = *(addr + i * 2 + grid_w * 2 * 2) >> 16;	// AB
			d0 = *(addr + i * 2 + grid_w * 2 * 3) >> 16;	// AB
			a1 = *(addr + i * 2) & 0xFFFF;	// CD
			b1 = *(addr + i * 2 + grid_w * 2) & 0xFFFF;	// CD
			c1 = *(addr + i * 2 + grid_w * 2 * 2) & 0xFFFF;	// CD
			d1 = *(addr + i * 2 + grid_w * 2 * 3) & 0xFFFF;	// CD
		}
		q_val[0][i] = cubic_1D(a0, b0, c0, d0, u, grid_num, lnc_bicubic_weight_simple);
		q_val[1][i] = cubic_1D(a1, b1, c1, d1, u, grid_num, lnc_bicubic_weight_simple);
	}

	return 0;
}

/*end cal Q value*/

cmr_s32 isp_u_2d_lsc_block(cmr_handle handle, void *block_info)
{
	cmr_s32 ret = 0;
	struct isp_file *file = NULL;
	struct isp_io_param param;

	if (!handle || !block_info) {
		ISP_LOGE("fail to get handle: handle = %p, block_info = %p.", handle, block_info);
		return -1;
	}

	file = (struct isp_file *)(handle);

	struct isp_dev_2d_lsc_info *lens_info = (struct isp_dev_2d_lsc_info *)(block_info);
	cmr_uint buf_addr;

#if __WORDSIZE == 64
	buf_addr = ((cmr_uint) lens_info->buf_addr[1] << 32) | lens_info->buf_addr[0];
#else
	buf_addr = lens_info->buf_addr[0];
#endif

	if (0 == file->reserved || 0 == buf_addr || ISP_LSC_BUF_SIZE < lens_info->buf_len) {
		ISP_LOGE("fail to get lsc memory: file->reserved = %p buf_addr = %lx lens_info->buf_len = %d.",
			file->reserved, buf_addr, lens_info->buf_len);
		return ret;
	} else {
		memcpy((void *)file->reserved, (void *)buf_addr, lens_info->buf_len);
	}

	ret = ISP_GenerateQValues(1, lens_info->q_value, buf_addr,
				  ((cmr_u16) lens_info->grid_x_num & 0xFF) + 2, (lens_info->grid_width & 0xFF), (lens_info->relative_y & 0xFF) >> 1);
	if (0 != ret) {
		ISP_LOGE("fail to get ISP_GenerateQValues.");
		return ret;
	}

	param.isp_id = file->isp_id;
	param.sub_block = ISP_BLOCK_2D_LSC;
	param.property = ISP_PRO_2D_LSC_BLOCK;
	param.property_param = lens_info;

	ret = ioctl(file->fd, SPRD_ISP_IO_CFG_PARAM, &param);

	return ret;
}

cmr_s32 isp_u_2d_lsc_slice_size(cmr_handle handle, cmr_u32 w, cmr_u32 h)
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
	param.sub_block = ISP_BLOCK_2D_LSC;
	param.property = ISP_PRO_2D_LSC_SLICE_SIZE;
	size.width = w;
	size.height = h;
	param.property_param = &size;

	ret = ioctl(file->fd, SPRD_ISP_IO_CFG_PARAM, &param);

	return ret;
}

cmr_s32 isp_u_2d_lsc_transaddr(cmr_handle handle, struct isp_statis_buf_input * buf)
{
	cmr_s32 ret = 0;
	struct isp_file *file = NULL;
	struct isp_io_param param;
	struct isp_lsc_addr lsc_addr;

	if (!handle) {
		ISP_LOGE("fail to get handle.");
		return -1;
	}

	memset(&lsc_addr, 0, sizeof(struct isp_lsc_addr));
	file = (struct isp_file *)(handle);
	param.isp_id = file->isp_id;
	param.sub_block = ISP_BLOCK_2D_LSC;
	param.property = ISP_PRO_2D_LSC_TRANSADDR;
	lsc_addr.fd = buf->mfd;
	param.property_param = &lsc_addr;

	ret = ioctl(file->fd, SPRD_ISP_IO_CFG_PARAM, &param);

	return ret;
}
