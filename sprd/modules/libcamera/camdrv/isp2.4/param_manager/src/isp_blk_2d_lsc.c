/*
 * Copyright (C) 2012 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *		http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#define LOG_TAG "isp_blk_2d_lsc"
#include "isp_blocks_cfg.h"

cmr_s32 _pm_2d_lsc_init(void *dst_lnc_param, void *src_lnc_param, void *param1, void *param2)
{
	cmr_s32 rtn = ISP_SUCCESS;
	cmr_u32 i = 0;
	intptr_t addr = 0, index = 0;
	struct isp_size *img_size_ptr = (struct isp_size *)param2;
	struct isp_2d_lsc_param *dst_ptr = (struct isp_2d_lsc_param *)dst_lnc_param;
	struct sensor_2d_lsc_param *src_ptr = (struct sensor_2d_lsc_param *)src_lnc_param;
	struct isp_pm_block_header *header_ptr = (struct isp_pm_block_header *)param1;

	dst_ptr->tab_num = src_ptr->tab_num;
	for (i = 0; i < ISP_COLOR_TEMPRATURE_NUM; ++i) {
		addr = (intptr_t) & (src_ptr->tab_info.lsc_2d_map) + src_ptr->tab_info.lsc_2d_info[i].lsc_2d_offset;
		dst_ptr->map_tab[i].param_addr = (void *)addr;
		dst_ptr->map_tab[i].len = src_ptr->tab_info.lsc_2d_info[i].lsc_2d_len;
		dst_ptr->map_tab[i].grid = src_ptr->tab_info.lsc_2d_info[i].lsc_2d_map_info.grid;
		dst_ptr->map_tab[i].grid_mode = src_ptr->tab_info.lsc_2d_info[i].lsc_2d_map_info.grid;
		dst_ptr->map_tab[i].grid_pitch = _pm_get_lens_grid_pitch(src_ptr->tab_info.lsc_2d_info[i].lsc_2d_map_info.grid, img_size_ptr->w, ISP_ONE);

		dst_ptr->map_tab[i].gain_w = dst_ptr->map_tab[i].grid_pitch;
		dst_ptr->map_tab[i].gain_h = _pm_get_lens_grid_pitch(src_ptr->tab_info.lsc_2d_info[i].lsc_2d_map_info.grid, img_size_ptr->h, ISP_ONE);

	}
	if (dst_ptr->final_lsc_param.size < src_ptr->tab_info.lsc_2d_info[0].lsc_2d_len) {
		if (NULL != dst_ptr->final_lsc_param.data_ptr) {
			free(dst_ptr->final_lsc_param.data_ptr);
			dst_ptr->final_lsc_param.data_ptr = NULL;
		}
		if (NULL != dst_ptr->final_lsc_param.param_ptr) {
			free(dst_ptr->final_lsc_param.param_ptr);
			dst_ptr->final_lsc_param.param_ptr = NULL;
		}
		if (NULL != dst_ptr->tmp_ptr_a) {
			free(dst_ptr->tmp_ptr_a);
			dst_ptr->tmp_ptr_a = NULL;
		}
		if (NULL != dst_ptr->tmp_ptr_b) {
			free(dst_ptr->tmp_ptr_b);
			dst_ptr->tmp_ptr_b = NULL;
		}
		dst_ptr->final_lsc_param.size = 0;
	}

	if (NULL == dst_ptr->final_lsc_param.data_ptr) {
		dst_ptr->final_lsc_param.data_ptr = (void *)malloc(src_ptr->tab_info.lsc_2d_info[0].lsc_2d_len);
		if (NULL == dst_ptr->final_lsc_param.data_ptr) {
			rtn = ISP_ERROR;
			ISP_LOGE("fail to malloc\n");
			return rtn;
		}
	}

	if (NULL == dst_ptr->final_lsc_param.param_ptr) {
		dst_ptr->final_lsc_param.param_ptr = (void *)malloc(src_ptr->tab_info.lsc_2d_info[0].lsc_2d_len);
		if (NULL == dst_ptr->final_lsc_param.param_ptr) {
			rtn = ISP_ERROR;
			ISP_LOGE("fail to malloc\n");
			return rtn;
		}
	}

	if (NULL == dst_ptr->tmp_ptr_a) {
		dst_ptr->tmp_ptr_a = (void *)malloc(src_ptr->tab_info.lsc_2d_info[0].lsc_2d_len);
		if (NULL == dst_ptr->tmp_ptr_a) {
			rtn = ISP_ERROR;
			ISP_LOGE("fail to malloc\n");
			return rtn;
		}
	}

	if (NULL == dst_ptr->tmp_ptr_b) {
		dst_ptr->tmp_ptr_b = (void *)malloc(src_ptr->tab_info.lsc_2d_info[0].lsc_2d_len);
		if (NULL == dst_ptr->tmp_ptr_b) {
			rtn = ISP_ERROR;
			ISP_LOGE("fail to malloc\n");
			return rtn;
		}
	}

	dst_ptr->tab_num = src_ptr->tab_num;
	dst_ptr->cur_index_info = src_ptr->cur_idx;
	index = src_ptr->cur_idx.x0;
	dst_ptr->cur_index_info.weight0 = 0;
	dst_ptr->cur_index_info.x0 = 0;
	dst_ptr->final_lsc_param.size = src_ptr->tab_info.lsc_2d_info[index].lsc_2d_len;
	memcpy((void *)dst_ptr->final_lsc_param.data_ptr, (void *)dst_ptr->map_tab[index].param_addr, dst_ptr->map_tab[index].len);
	dst_ptr->cur.buf_len = dst_ptr->final_lsc_param.size;
#if __WORDSIZE == 64
	dst_ptr->cur.buf_addr[0] = (cmr_uint) (dst_ptr->final_lsc_param.data_ptr) & 0xffffffff;
	dst_ptr->cur.buf_addr[1] = (cmr_uint) (dst_ptr->final_lsc_param.data_ptr) >> 32;
#else
	dst_ptr->cur.buf_addr[0] = (cmr_uint) (dst_ptr->final_lsc_param.data_ptr);
	dst_ptr->cur.buf_addr[1] = 0;
#endif
	dst_ptr->cur.slice_size.width = img_size_ptr->w;
	dst_ptr->cur.slice_size.height = img_size_ptr->h;
	dst_ptr->cur.grid_width = dst_ptr->map_tab[index].grid_mode;
	dst_ptr->cur.grid_x_num = _pm_get_lens_grid_pitch(dst_ptr->cur.grid_width, dst_ptr->cur.slice_size.width, ISP_ZERO);
	dst_ptr->cur.grid_y_num = _pm_get_lens_grid_pitch(dst_ptr->cur.grid_width, dst_ptr->cur.slice_size.height, ISP_ZERO);
	dst_ptr->cur.grid_num_t = (dst_ptr->cur.grid_x_num + 2) * (dst_ptr->cur.grid_y_num + 2);
	dst_ptr->cur.grid_pitch = dst_ptr->cur.grid_x_num + 2;
	dst_ptr->cur.endian = ISP_ONE;
	dst_ptr->resolution = *img_size_ptr;
	dst_ptr->is_init = ISP_ONE;

	dst_ptr->cur.bypass = header_ptr->bypass;
	header_ptr->is_update = ISP_PM_BLK_LSC_UPDATE_MASK_PARAM;
	return rtn;
}

cmr_s32 _pm_2d_lsc_set_param(void *lnc_param, cmr_u32 cmd, void *param_ptr0, void *param_ptr1)
{
	cmr_s32 rtn = ISP_SUCCESS;
	struct isp_2d_lsc_param *dst_lnc_ptr = (struct isp_2d_lsc_param *)lnc_param;
	struct isp_pm_block_header *lnc_header_ptr = (struct isp_pm_block_header *)param_ptr1;

	switch (cmd) {
	case ISP_PM_BLK_LSC_MEM_ADDR:
		{
			memcpy((void *)dst_lnc_ptr->final_lsc_param.data_ptr, param_ptr0, dst_lnc_ptr->final_lsc_param.size);

#if __WORDSIZE == 64
			dst_lnc_ptr->cur.buf_addr[0] = (cmr_uint) (dst_lnc_ptr->final_lsc_param.data_ptr) & 0xffffffff;
			dst_lnc_ptr->cur.buf_addr[1] = (cmr_uint) (dst_lnc_ptr->final_lsc_param.data_ptr) >> 32;
#else
			dst_lnc_ptr->cur.buf_addr[0] = (cmr_uint) (dst_lnc_ptr->final_lsc_param.data_ptr);
			dst_lnc_ptr->cur.buf_addr[1] = 0;
#endif
			lnc_header_ptr->is_update |= ISP_PM_BLK_LSC_UPDATE_MASK_VALIDATE;
			dst_lnc_ptr->update_flag = lnc_header_ptr->is_update;
		}
		break;

	case ISP_PM_BLK_LSC_INFO:
		{
			lnc_header_ptr->is_update |= ISP_PM_BLK_LSC_UPDATE_MASK_PARAM;
			dst_lnc_ptr->update_flag = lnc_header_ptr->is_update;
			ISP_LOGV("ISP_PM_BLK_LSC_INFO");

			{
				cmr_u16 *ptr = NULL;
#if __WORDSIZE == 64
				ptr = (void *)((cmr_uint) dst_lnc_ptr->cur.buf_addr[1] << 32 | dst_lnc_ptr->cur.buf_addr[0]);
#else
				ptr = (void *)(dst_lnc_ptr->cur.buf_addr[0]);
#endif
				ISP_LOGV("lsc[0]: 0x%0x, 0x%0x, 0x%0x, 0x%0x", *ptr, *(ptr + 1), *(ptr + 2), *(ptr + 3));
				ISP_LOGV("lsc[1]: 0x%0x, 0x%0x, 0x%0x, 0x%0x", *(ptr + 4), *(ptr + 5), *(ptr + 6), *(ptr + 7));
			}
		}
		break;

	default:
		break;
	}

	ISP_LOGV("ISP_SMART: cmd=%d, update=%d, value=(%d, %d), weight=(%d, %d)\n", cmd, lnc_header_ptr->is_update,
		 dst_lnc_ptr->cur_index_info.x0, dst_lnc_ptr->cur_index_info.x1, dst_lnc_ptr->cur_index_info.weight0, dst_lnc_ptr->cur_index_info.weight1);

	return rtn;
}

cmr_s32 _pm_2d_lsc_get_param(void *lnc_param, cmr_u32 cmd, void *rtn_param0, void *rtn_param1)
{
	cmr_s32 rtn = ISP_SUCCESS;
	struct isp_pm_param_data *param_data_ptr = (struct isp_pm_param_data *)rtn_param0;
	struct isp_2d_lsc_param *lnc_ptr = (struct isp_2d_lsc_param *)lnc_param;
	cmr_u32 *update_flag = (cmr_u32 *) rtn_param1;

	param_data_ptr->id = ISP_BLK_2D_LSC;
	param_data_ptr->cmd = cmd;

	switch (cmd) {
	case ISP_PM_BLK_ISP_SETTING:
		param_data_ptr->data_ptr = (void *)&lnc_ptr->cur;
		param_data_ptr->data_size = sizeof(lnc_ptr->cur);
		*update_flag &= (~(ISP_PM_BLK_LSC_UPDATE_MASK_PARAM | ISP_PM_BLK_LSC_UPDATE_MASK_VALIDATE));
		lnc_ptr->update_flag = *update_flag;
		break;

	case ISP_PM_BLK_LSC_INFO:
		lnc_ptr->lsc_info.cur_idx = lnc_ptr->cur_index_info;
		lnc_ptr->lsc_info.gain_w = lnc_ptr->map_tab[lnc_ptr->lsc_info.cur_idx.x0].gain_w;
		lnc_ptr->lsc_info.gain_h = lnc_ptr->map_tab[lnc_ptr->lsc_info.cur_idx.x0].gain_h;
		lnc_ptr->lsc_info.grid = lnc_ptr->map_tab[lnc_ptr->lsc_info.cur_idx.x0].grid;
		lnc_ptr->lsc_info.data_ptr = lnc_ptr->final_lsc_param.data_ptr;
		lnc_ptr->lsc_info.len = lnc_ptr->final_lsc_param.size;
		lnc_ptr->lsc_info.param_ptr = lnc_ptr->final_lsc_param.param_ptr;
		param_data_ptr->data_ptr = (void *)&lnc_ptr->lsc_info;
		param_data_ptr->data_size = sizeof(lnc_ptr->lsc_info);
		break;

	case ISP_PM_BLK_LSC_GET_LSCTAB:
		param_data_ptr->data_ptr = (void *)lnc_ptr;
		param_data_ptr->data_size = lnc_ptr->map_tab[lnc_ptr->lsc_info.cur_idx.x0].gain_w * lnc_ptr->map_tab[lnc_ptr->lsc_info.cur_idx.x0].gain_h * 4;
		break;

	default:
		break;
	}

	return rtn;
}

cmr_s32 _pm_2d_lsc_deinit(void *lnc_param)
{
	cmr_u32 rtn = ISP_SUCCESS;
	struct isp_2d_lsc_param *lsc_param_ptr = (struct isp_2d_lsc_param *)lnc_param;

	if (lsc_param_ptr->final_lsc_param.data_ptr) {
		free(lsc_param_ptr->final_lsc_param.data_ptr);
		lsc_param_ptr->final_lsc_param.data_ptr = PNULL;
		lsc_param_ptr->final_lsc_param.size = 0;
	}

	if (lsc_param_ptr->final_lsc_param.param_ptr) {
		free(lsc_param_ptr->final_lsc_param.param_ptr);
		lsc_param_ptr->final_lsc_param.param_ptr = PNULL;
	}

	if (lsc_param_ptr->tmp_ptr_a) {
		free(lsc_param_ptr->tmp_ptr_a);
		lsc_param_ptr->tmp_ptr_a = PNULL;
	}

	if (lsc_param_ptr->tmp_ptr_b) {
		free(lsc_param_ptr->tmp_ptr_b);
		lsc_param_ptr->tmp_ptr_b = PNULL;
	}

	return rtn;
}
