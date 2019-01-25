/*
 * Copyright (C) 2012 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#define LOG_TAG "lsc_ctrl"
#define ATRACE_TAG (ATRACE_TAG_CAMERA | ATRACE_TAG_HAL)
#include <cutils/trace.h>
#include "lsc_adv.h"
#include "isp_adpt.h"
#include <dlfcn.h>
#include "isp_mw.h"
#include <utils/Timers.h>

#define SMART_LSC_VERSION 1

struct lsc_ctrl_work_lib {
	cmr_handle lib_handle;
	struct adpt_ops_type *adpt_ops;
};

struct lsc_ctrl_cxt {
	cmr_handle thr_handle;
	cmr_handle caller_handle;
	struct lsc_ctrl_work_lib work_lib;
	struct lsc_adv_calc_result proc_out;
};

char liblsc_path[][20] = {
	"liblsc.so",
	"liblsc_v1.so",
	"liblsc_v2.so",
	"liblsc_v3.so",
	"liblsc_v4.so",
	"liblsc_v5.so",
};

typedef struct {
	int grid_size;
	int lpf_mode;
	int lpf_radius;
	int lpf_border;
	int border_patch;
	int border_expand;
	int shading_mode;
	int shading_pct;
} lsc2d_calib_param_t;

struct lsc_wrapper_ops {
	void (*lsc2d_grid_samples) (int w, int h, int gridx, int gridy, int *nx, int *ny);
	void (*lsc2d_calib_param_default) (lsc2d_calib_param_t * calib_param, int grid_size, int lpf_radius, int shading_pct);
	int (*lsc2d_table_preproc) (uint16_t * otp_chn[4], uint16_t * tbl_chn[4], int w, int h, int sx, int sy, lsc2d_calib_param_t * calib_param);
	int (*lsc2d_table_postproc) (uint16_t * tbl_chn[4], int w, int h, int sx, int sy, lsc2d_calib_param_t * calib_param);
};

static int _lsc_gain_14bits_to_16bits(unsigned short *src_14bits, unsigned short *dst_16bits, unsigned int size_bytes)
{
	unsigned int gain_compressed_bits = 14;
	unsigned int gain_origin_bits = 16;
	unsigned int i = 0;
	unsigned int j = 0;
	unsigned int bit_left = 0;
	unsigned int bit_buf = 0;
	unsigned int offset = 0;
	unsigned int dst_gain_num = 0;
	unsigned int src_uncompensate_bytes = size_bytes * gain_compressed_bits % gain_origin_bits;
	unsigned int cmp_bits = size_bytes * gain_compressed_bits;
	unsigned int src_bytes = (cmp_bits + gain_origin_bits - 1) / gain_origin_bits * (gain_origin_bits / 8);

	if (0 == src_bytes || 0 != (src_bytes & 1)) {
		return 0;
	}

	for (i = 0; i < src_bytes / 2; i++) {
		bit_buf |= src_14bits[i] << bit_left;
		bit_left += 16;

		if (bit_left > gain_compressed_bits) {
			offset = 0;
			while (bit_left >= gain_compressed_bits) {
				dst_16bits[j] = (unsigned short)(bit_buf & 0x3fff);
				j++;
				bit_left -= gain_compressed_bits;
				bit_buf = (bit_buf >> gain_compressed_bits);
			}
		}
	}

	if (gain_compressed_bits == src_uncompensate_bytes) {
		dst_gain_num = j - 1;
	} else {
		dst_gain_num = j;
	}

	return dst_gain_num;
}

static uint16_t *_lsc_table_wrapper(uint16_t * lsc_otp_tbl, int grid, int image_width, int image_height, int *tbl_w, int *tbl_h)
{
	int ret = ISP_SUCCESS;
	lsc2d_calib_param_t calib_param;
	int lpf_radius = 16;
	int shading_pct = 100;
	int nx, ny, sx, sy;
	uint16_t *otp_chn[4], *tbl_chn[4];
	int w = image_width / 2;
	int h = image_height / 2;
	uint16_t *lsc_table = NULL;
	int i;
	void *lsc_handle = dlopen("libsprdlsc.so", RTLD_NOW);
	if (!lsc_handle) {
		ISP_LOGE("init_lsc_otp, fail to dlopen libsprdlsc lib");
		ret = ISP_ERROR;
		return lsc_table;
	}

	struct lsc_wrapper_ops lsc_ops;

	lsc_ops.lsc2d_grid_samples = dlsym(lsc_handle, "lsc2d_grid_samples");
	if (!lsc_ops.lsc2d_grid_samples) {
		ISP_LOGE("init_lsc_otp, fail to dlsym lsc2d_grid_samples");
		ret = ISP_ERROR;
		goto error_dlsym;
	}

	lsc_ops.lsc2d_calib_param_default = dlsym(lsc_handle, "lsc2d_calib_param_default");
	if (!lsc_ops.lsc2d_calib_param_default) {
		ISP_LOGE("init_lsc_otp, fail to dlsym lsc2d_calib_param_default");
		ret = ISP_ERROR;
		goto error_dlsym;
	}

	lsc_ops.lsc2d_table_preproc = dlsym(lsc_handle, "lsc2d_table_preproc");
	if (!lsc_ops.lsc2d_table_preproc) {
		ISP_LOGE("init_lsc_otp, fail to dlsym lsc2d_table_preproc");
		ret = ISP_ERROR;
		goto error_dlsym;
	}

	lsc_ops.lsc2d_table_postproc = dlsym(lsc_handle, "lsc2d_table_postproc");
	if (!lsc_ops.lsc2d_table_postproc) {
		ISP_LOGE("init_lsc_otp, fail to dlsym lsc2d_table_postproc");
		ret = ISP_ERROR;
		goto error_dlsym;
	}

	lsc_ops.lsc2d_grid_samples(w, h, grid, grid, &nx, &ny);
	sx = nx + 2;
	sy = ny + 2;

	lsc_table = (uint16_t *) malloc(4 * sx * sy * sizeof(uint16_t));

	*tbl_w = sx;
	*tbl_h = sy;

	for (i = 0; i < 4; i++) {
		otp_chn[i] = lsc_otp_tbl + i * nx * ny;
		tbl_chn[i] = lsc_table + i * sx * sy;
	}

	lsc_ops.lsc2d_calib_param_default(&calib_param, grid, lpf_radius, shading_pct);

	lsc_ops.lsc2d_table_preproc(otp_chn, tbl_chn, w, h, sx, sy, &calib_param);

	lsc_ops.lsc2d_table_postproc(tbl_chn, w, h, sx, sy, &calib_param);

  error_dlsym:
	dlclose(lsc_handle);
	lsc_handle = NULL;

	return lsc_table;
}

void _lsc_get_otp_size_info(cmr_s32 full_img_width, cmr_s32 full_img_height, cmr_s32 * lsc_otp_width, cmr_s32 * lsc_otp_height, cmr_s32 lsc_otp_grid)
{
	*lsc_otp_width = 0;
	*lsc_otp_height = 0;

	*lsc_otp_width = (int)(full_img_width / (2 * lsc_otp_grid)) + 1;
	*lsc_otp_height = (int)(full_img_height / (2 * lsc_otp_grid)) + 1;

	if (full_img_width % (2 * lsc_otp_grid) != 0) {
		*lsc_otp_width += 1;
	}

	if (full_img_height % (2 * lsc_otp_grid) != 0) {
		*lsc_otp_height += 1;
	}
}

cmr_int _lsc_parser_otp(struct lsc_adv_init_param *lsc_param)
{
	struct sensor_otp_data_info *lsc_otp_info;
	struct sensor_otp_data_info *oc_otp_info;
	cmr_u8 *module_info;
	cmr_u32 full_img_width = lsc_param->img_width;
	cmr_u32 full_img_height = lsc_param->img_height;
	cmr_u32 lsc_otp_grid = lsc_param->grid;
	cmr_u8 *lsc_otp_addr;
	cmr_u16 lsc_otp_len;
	cmr_s32 compressed_lens_bits = 14;
	cmr_s32 lsc_otp_width, lsc_otp_height;
	cmr_s32 lsc_otp_len_chn;
	cmr_s32 lsc_otp_chn_gain_num;
	cmr_s32 gain_w, gain_h;
	uint16_t *lsc_table = NULL;
	cmr_u8 *oc_otp_data;
	cmr_u16 oc_otp_len;
	cmr_u8 *otp_data_ptr;
	cmr_u32 otp_data_len;
	cmr_u32 resolution = 0;
	struct sensor_otp_section_info *lsc_otp_info_ptr = NULL;
	struct sensor_otp_section_info *oc_otp_info_ptr = NULL;
	struct sensor_otp_section_info *module_info_ptr = NULL;

	_lsc_get_otp_size_info(full_img_width, full_img_height, &lsc_otp_width, &lsc_otp_height, lsc_otp_grid);

	if (NULL != lsc_param->otp_info_ptr) {
		struct sensor_otp_cust_info *otp_info_ptr = (struct sensor_otp_cust_info *)lsc_param->otp_info_ptr;
		if (otp_info_ptr->otp_vendor == OTP_VENDOR_SINGLE) {
			lsc_otp_info_ptr = otp_info_ptr->single_otp.lsc_info;
			oc_otp_info_ptr = otp_info_ptr->single_otp.optical_center_info;
			module_info_ptr = otp_info_ptr->single_otp.module_info;
			ISP_LOGV("init_lsc_otp, single cam");
		} else if (otp_info_ptr->otp_vendor == OTP_VENDOR_SINGLE_CAM_DUAL || otp_info_ptr->otp_vendor == OTP_VENDOR_DUAL_CAM_DUAL) {
			if (lsc_param->is_master == 1) {
				lsc_otp_info_ptr = otp_info_ptr->dual_otp.master_lsc_info;
				oc_otp_info_ptr = otp_info_ptr->dual_otp.master_optical_center_info;
				module_info_ptr = otp_info_ptr->dual_otp.master_module_info;
				ISP_LOGV("init_lsc_otp, dual cam master");
			} else {
				lsc_otp_info_ptr = otp_info_ptr->dual_otp.slave_lsc_info;
				oc_otp_info_ptr = otp_info_ptr->dual_otp.slave_optical_center_info;
				module_info_ptr = otp_info_ptr->dual_otp.slave_module_info;
				ISP_LOGV("init_lsc_otp, dual cam slave");
			}
		}
	} else {
		lsc_otp_info_ptr = NULL;
		oc_otp_info_ptr = NULL;
		module_info_ptr = NULL;
		ISP_LOGE("lsc otp_info_ptr is NULL");
	}

	if (NULL != module_info_ptr) {
		module_info = (cmr_u8 *) module_info_ptr->rdm_info.data_addr;

		if (NULL == module_info) {
			ISP_LOGE("lsc module_info is NULL");
			goto EXIT;
		}

		if ((module_info[4] == 0 && module_info[5] == 1)
			|| (module_info[4] == 0 && module_info[5] == 2)
			|| (module_info[4] == 0 && module_info[5] == 3)
			|| (module_info[4] == 0 && module_info[5] == 4)
			|| (module_info[4] == 1 && module_info[5] == 0 && (module_info[0] != 0x53 || module_info[1] != 0x50 || module_info[2] != 0x52 || module_info[3] != 0x44))
			|| (module_info[4] == 2 && module_info[5] == 0)
			|| (module_info[4] == 3 && module_info[5] == 0)
			|| (module_info[4] == 4 && module_info[5] == 0)) {
			ISP_LOGV("lsc otp map v0.4");
			if (NULL != lsc_otp_info_ptr && NULL != oc_otp_info_ptr) {
				lsc_otp_info = &lsc_otp_info_ptr->rdm_info;
				oc_otp_info = &oc_otp_info_ptr->rdm_info;
				lsc_otp_addr = (cmr_u8 *) lsc_otp_info->data_addr;
				lsc_otp_len = lsc_otp_info->data_size;
				lsc_otp_len_chn = lsc_otp_len / 4;
				lsc_otp_chn_gain_num = lsc_otp_len_chn * 8 / compressed_lens_bits;
				oc_otp_data = (cmr_u8 *) oc_otp_info->data_addr;
				oc_otp_len = oc_otp_info->data_size;
			} else {
				ISP_LOGE("lsc otp_info_lsc_ptr = %p, otp_info_optical_center_ptr = %p. Parser fail !", lsc_otp_info_ptr, oc_otp_info_ptr);
				goto EXIT;
			}
		} else if (module_info[4] == 1 && module_info[5] == 0 && module_info[0] == 0x53 && module_info[1] == 0x50 && module_info[2] == 0x52 && module_info[3] == 0x44) {
			ISP_LOGV("lsc otp map v1.0");
			if (NULL != lsc_otp_info_ptr) {
				otp_data_ptr = lsc_otp_info_ptr->rdm_info.data_addr;
				otp_data_len = lsc_otp_info_ptr->rdm_info.data_size;
				lsc_otp_addr = otp_data_ptr + 1 + 16 + 5;
				lsc_otp_len = otp_data_len - 1 - 16 - 5;

				resolution = (full_img_width * full_img_height + 500000) / 1000000;
				switch (resolution) {
				case 16:
					lsc_otp_len_chn = 526;
					break;
				case 13:
					lsc_otp_len_chn = 726;
					break;
				case 12:
					lsc_otp_len_chn = 656;
					break;
				case 8:
					lsc_otp_len_chn = 442;
					break;
				case 5:
					lsc_otp_len_chn = 656;
					break;
				case 2:
					lsc_otp_len_chn = 270;
					break;
				default:
					lsc_otp_len_chn = 0;
					break;
				}
				lsc_otp_chn_gain_num = lsc_otp_len_chn * 8 / compressed_lens_bits;

				oc_otp_data = otp_data_ptr + 1;
				oc_otp_len = 16;
			} else {
				ISP_LOGE("lsc lsc_otp_info_ptr = %p. Parser fail !", lsc_otp_info_ptr);
				goto EXIT;
			}
		} else {
			ISP_LOGE("lsc otp map version error");
			goto EXIT;
		}
	} else {
		ISP_LOGE("lsc module_info_ptr = %p. Parser fail !", module_info_ptr);
		goto EXIT;
	}

	ISP_LOGV("init_lsc_otp, full_img_width=%d, full_img_height=%d, lsc_otp_grid=%d", full_img_width, full_img_height, lsc_otp_grid);
	ISP_LOGV("init_lsc_otp, before, lsc_otp_chn_gain_num=%d", lsc_otp_chn_gain_num);

	if (lsc_otp_chn_gain_num < 100 || lsc_otp_grid < 32 || lsc_otp_grid > 256 || full_img_width < 800 || full_img_height < 600) {
		ISP_LOGE("init_lsc_otp, sensor setting error, lsc_otp_len=%d, full_img_width=%d, full_img_height=%d, lsc_otp_grid=%d", lsc_otp_len, full_img_width, full_img_height, lsc_otp_grid);
		goto EXIT;
	}

	if (lsc_otp_chn_gain_num != lsc_otp_width * lsc_otp_height) {
		ISP_LOGE("init_lsc_otp, sensor setting error, lsc_otp_len=%d, lsc_otp_chn_gain_num=%d, lsc_otp_width=%d, lsc_otp_height=%d, lsc_otp_grid=%d", lsc_otp_len, lsc_otp_chn_gain_num, lsc_otp_width,
				 lsc_otp_height, lsc_otp_grid);
		goto EXIT;
	}

	cmr_s32 lsc_ori_chn_len = lsc_otp_chn_gain_num * sizeof(uint16_t);

	if ((lsc_otp_addr != NULL) && (lsc_otp_len != 0)) {

		uint16_t *lsc_16_bits = (uint16_t *) malloc(lsc_ori_chn_len * 4);
		_lsc_gain_14bits_to_16bits((unsigned short *)(lsc_otp_addr + lsc_otp_len_chn * 0), lsc_16_bits + lsc_otp_chn_gain_num * 0, lsc_otp_chn_gain_num);
		_lsc_gain_14bits_to_16bits((unsigned short *)(lsc_otp_addr + lsc_otp_len_chn * 1), lsc_16_bits + lsc_otp_chn_gain_num * 1, lsc_otp_chn_gain_num);
		_lsc_gain_14bits_to_16bits((unsigned short *)(lsc_otp_addr + lsc_otp_len_chn * 2), lsc_16_bits + lsc_otp_chn_gain_num * 2, lsc_otp_chn_gain_num);
		_lsc_gain_14bits_to_16bits((unsigned short *)(lsc_otp_addr + lsc_otp_len_chn * 3), lsc_16_bits + lsc_otp_chn_gain_num * 3, lsc_otp_chn_gain_num);

		lsc_table = _lsc_table_wrapper(lsc_16_bits, lsc_otp_grid, full_img_width, full_img_height, &gain_w, &gain_h);	//  wrapper otp table

		free(lsc_16_bits);
		if (lsc_table == NULL) {
			ISP_LOGE("init_lsc_otp, sensor setting error, lsc_otp_len=%d, lsc_otp_chn_gain_num=%d, lsc_otp_width=%d, lsc_otp_height=%d, lsc_otp_grid=%d", lsc_otp_len, lsc_otp_chn_gain_num,
					 lsc_otp_width, lsc_otp_height, lsc_otp_grid);
			goto EXIT;
		}
		lsc_param->lsc_otp_table_width = gain_w;
		lsc_param->lsc_otp_table_height = gain_h;
		lsc_param->lsc_otp_grid = lsc_otp_grid;
		lsc_param->lsc_otp_table_addr = lsc_table;
		lsc_param->lsc_otp_table_en = 1;

		ISP_LOGV("init_lsc_otp, lsc_otp_width=%d, lsc_otp_height=%d, gain_w=%d, gain_h=%d, lsc_otp_grid=%d", lsc_otp_width, lsc_otp_height, gain_w, gain_h, lsc_otp_grid);
		ISP_LOGV("init_lsc_otp, lsc_table0_RGGB=[%d,%d,%d,%d]", lsc_table[0], lsc_table[gain_w * gain_h], lsc_table[gain_w * gain_h * 2], lsc_table[gain_w * gain_h * 3]);
		ISP_LOGV("init_lsc_otp, lsc_table1_RGGB=[%d,%d,%d,%d]", lsc_table[gain_w + 1], lsc_table[gain_w * gain_h + gain_w + 1], lsc_table[gain_w * gain_h * 2 + gain_w + 1],
				 lsc_table[gain_w * gain_h * 3 + gain_w + 1]);
	} else {
		ISP_LOGE("lsc_otp_addr = %p, lsc_otp_len = %d. Parser lsc otp fail", lsc_otp_addr, lsc_otp_len);
		ISP_LOGE("init_lsc_otp, sensor setting error, lsc_otp_len=%d, lsc_otp_chn_gain_num=%d, lsc_otp_width=%d, lsc_otp_height=%d, lsc_otp_grid=%d", lsc_otp_len, lsc_otp_chn_gain_num, lsc_otp_width,
				 lsc_otp_height, lsc_otp_grid);
		goto EXIT;
	}

	if (NULL != oc_otp_data && 0 != oc_otp_len) {
		lsc_param->lsc_otp_oc_r_x = (oc_otp_data[1] << 8) | oc_otp_data[0];
		lsc_param->lsc_otp_oc_r_y = (oc_otp_data[3] << 8) | oc_otp_data[2];
		lsc_param->lsc_otp_oc_gr_x = (oc_otp_data[5] << 8) | oc_otp_data[4];
		lsc_param->lsc_otp_oc_gr_y = (oc_otp_data[7] << 8) | oc_otp_data[6];
		lsc_param->lsc_otp_oc_gb_x = (oc_otp_data[9] << 8) | oc_otp_data[8];
		lsc_param->lsc_otp_oc_gb_y = (oc_otp_data[11] << 8) | oc_otp_data[10];
		lsc_param->lsc_otp_oc_b_x = (oc_otp_data[13] << 8) | oc_otp_data[12];
		lsc_param->lsc_otp_oc_b_y = (oc_otp_data[15] << 8) | oc_otp_data[14];
		lsc_param->lsc_otp_oc_en = 1;

		ISP_LOGV("init_lsc_otp, lsc_otp_oc_r=[%d,%d], lsc_otp_oc_gr=[%d,%d], lsc_otp_oc_gb=[%d,%d], lsc_otp_oc_b=[%d,%d] ",
				 lsc_param->lsc_otp_oc_r_x,
				 lsc_param->lsc_otp_oc_r_y,
				 lsc_param->lsc_otp_oc_gr_x, lsc_param->lsc_otp_oc_gr_y, lsc_param->lsc_otp_oc_gb_x, lsc_param->lsc_otp_oc_gb_y, lsc_param->lsc_otp_oc_b_x, lsc_param->lsc_otp_oc_b_y);
	} else {
		ISP_LOGE("oc_otp_data = %p, oc_otp_len = %d, Parser OC otp fail", oc_otp_data, oc_otp_len);
		goto EXIT;
	}

	return LSC_SUCCESS;

  EXIT:
	lsc_param->lsc_otp_table_addr = NULL;
	lsc_param->lsc_otp_table_en = 0;
	lsc_param->lsc_otp_oc_en = 0;
	lsc_param->lsc_otp_oc_r_x = 0;
	lsc_param->lsc_otp_oc_r_y = 0;
	lsc_param->lsc_otp_oc_gr_x = 0;
	lsc_param->lsc_otp_oc_gr_y = 0;
	lsc_param->lsc_otp_oc_gb_x = 0;
	lsc_param->lsc_otp_oc_gb_y = 0;
	lsc_param->lsc_otp_oc_b_x = 0;
	lsc_param->lsc_otp_oc_b_y = 0;
	lsc_param->lsc_otp_oc_en = 0;
	return LSC_ERROR;
}

static cmr_s32 _lscctrl_deinit_adpt(struct lsc_ctrl_cxt *cxt_ptr)
{
	cmr_int rtn = LSC_SUCCESS;
	struct lsc_ctrl_work_lib *lib_ptr = NULL;

	if (!cxt_ptr) {
		ISP_LOGE("fail to check param, param is NULL!");
		goto exit;
	}

	lib_ptr = &cxt_ptr->work_lib;
	if (lib_ptr->adpt_ops->adpt_deinit) {
		rtn = lib_ptr->adpt_ops->adpt_deinit(lib_ptr->lib_handle, NULL, NULL);
		lib_ptr->lib_handle = NULL;
	} else {
		ISP_LOGI("adpt_deinit fun is NULL");
	}

  exit:
	ISP_LOGI("done %ld", rtn);
	return rtn;
}

static cmr_s32 _lscctrl_destroy_thread(struct lsc_ctrl_cxt *cxt_ptr)
{
	cmr_int rtn = LSC_SUCCESS;

	if (!cxt_ptr) {
		ISP_LOGE("fail to check param , param is NULL");
		rtn = LSC_ERROR;
		goto exit;
	}

	if (cxt_ptr->thr_handle) {
		rtn = cmr_thread_destroy(cxt_ptr->thr_handle);
		if (!rtn) {
			cxt_ptr->thr_handle = NULL;
		} else {
			ISP_LOGE("fail to destroy ctrl thread %ld", rtn);
		}
	}
  exit:
	ISP_LOGI("done %ld", rtn);
	return rtn;
}

static cmr_s32 _lscctrl_process(struct lsc_ctrl_cxt *cxt_ptr, struct lsc_adv_calc_param *in_ptr, struct lsc_adv_calc_result *out_ptr)
{
	cmr_int rtn = LSC_SUCCESS;
	struct lsc_ctrl_work_lib *lib_ptr = NULL;

	if (!cxt_ptr) {
		ISP_LOGE("fail to check param,param is NULL!");
		goto exit;
	}
	lib_ptr = &cxt_ptr->work_lib;
	if (lib_ptr->adpt_ops->adpt_process) {
		rtn = lib_ptr->adpt_ops->adpt_process(lib_ptr->lib_handle, in_ptr, out_ptr);
	} else {
		ISP_LOGI("process fun is NULL");
	}
  exit:
	ISP_LOGV("done %ld", rtn);
	return rtn;
}

static cmr_int _lscctrl_ctrl_thr_proc(struct cmr_msg *message, void *p_data)
{
	cmr_int rtn = LSC_SUCCESS;
	struct lsc_ctrl_cxt *handle = (struct lsc_ctrl_cxt *)p_data;

	if (NULL == handle) {
		ISP_LOGE("Error: handle is NULL");
		return rtn;
	}

	if (!message || !p_data) {
		ISP_LOGE("fail to chcek param");
		goto exit;
	}
	ISP_LOGV("message.msg_type 0x%x, data %p", message->msg_type, message->data);

	switch (message->msg_type) {
	case LSCCTRL_EVT_INIT:
		break;
	case LSCCTRL_EVT_DEINIT:
		rtn = _lscctrl_deinit_adpt(handle);
		break;
	case LSCCTRL_EVT_IOCTRL:
		break;
	case LSCCTRL_EVT_PROCESS:	// ISP_PROC_LSC_CALC
		rtn = _lscctrl_process(handle, (struct lsc_adv_calc_param *)message->data, &handle->proc_out);
		break;
	default:
		ISP_LOGE("fail to proc,don't support msg");
		break;
	}

  exit:
	ISP_LOGV("done %ld", rtn);
	return rtn;
}

static cmr_s32 _lscctrl_init_lib(struct lsc_ctrl_cxt *cxt_ptr, struct lsc_adv_init_param *in_ptr)
{
	cmr_int rtn = LSC_SUCCESS;
	struct lsc_ctrl_work_lib *lib_ptr = NULL;

	if (!cxt_ptr) {
		ISP_LOGE("fail to check param ,param is NULL!");
		goto exit;
	}

	lib_ptr = &cxt_ptr->work_lib;
	if (lib_ptr->adpt_ops->adpt_init) {
		lib_ptr->lib_handle = lib_ptr->adpt_ops->adpt_init(in_ptr, NULL);
	} else {
		ISP_LOGI("adpt_init fun is NULL");
	}
  exit:
	ISP_LOGI("done %ld", rtn);
	return rtn;
}

static cmr_s32 _lscctrl_init_adpt(struct lsc_ctrl_cxt *cxt_ptr, struct lsc_adv_init_param *in_ptr)
{
	cmr_int rtn = LSC_SUCCESS;

	if (!cxt_ptr) {
		ISP_LOGE("fail to check param, param is NULL!");
		goto exit;
	}

	/* find vendor adpter */
	rtn = adpt_get_ops(ADPT_LIB_LSC, &in_ptr->lib_param, &cxt_ptr->work_lib.adpt_ops);
	if (rtn) {
		ISP_LOGE("fail to get adapter layer ret = %ld", rtn);
		goto exit;
	}

	rtn = _lscctrl_init_lib(cxt_ptr, in_ptr);
  exit:
	ISP_LOGI("done %ld", rtn);
	return rtn;
}

static cmr_s32 _lscctrl_create_thread(struct lsc_ctrl_cxt *cxt_ptr)
{
	cmr_int rtn = LSC_SUCCESS;

	rtn = cmr_thread_create(&cxt_ptr->thr_handle, ISP_THREAD_QUEUE_NUM, _lscctrl_ctrl_thr_proc, (void *)cxt_ptr);
	if (rtn) {
		ISP_LOGE("fail to create ctrl thread");
		rtn = LSC_ERROR;
		goto exit;
	}
	rtn = cmr_thread_set_name(cxt_ptr->thr_handle, "lscctrl");
	if (CMR_MSG_SUCCESS != rtn) {
		ISP_LOGE("fail to set lscctrl name");
		rtn = CMR_MSG_SUCCESS;
	}
  exit:
	ISP_LOGI("lsc_ctrl thread rtn %ld", rtn);
	return rtn;
}

static cmr_s32 _lscsprd_unload_lib(struct lsc_ctrl_context *cxt)
{
	cmr_s32 rtn = LSC_SUCCESS;

	if (NULL == cxt) {
		ISP_LOGE("fail to check param, Param is NULL");
		rtn = LSC_PARAM_NULL;
		goto exit;
	}

	if (cxt->lib_handle) {
		dlclose(cxt->lib_handle);
		cxt->lib_handle = NULL;
	}

  exit:
	return rtn;
}

static cmr_s32 _lscsprd_load_lib(struct lsc_ctrl_context *cxt)
{
	cmr_s32 rtn = LSC_SUCCESS;
	cmr_u32 v_count = 0;
	cmr_u32 version_id = 0;

	if (NULL == cxt) {
		ISP_LOGE("fail to check param,Param is NULL");
		rtn = LSC_PARAM_NULL;
		goto exit;
	}

	version_id = cxt->lib_info->version_id;
	v_count = sizeof(liblsc_path) / sizeof(liblsc_path[0]);
	if (version_id >= v_count) {
		ISP_LOGE("fail to get lsc lib version , version_id :%d", version_id);
		rtn = LSC_ERROR;
		goto exit;
	}

	ISP_LOGI("lib lsc v_count : %d, version id: %d, liblsc path :%s", v_count, version_id, liblsc_path[version_id]);

	cxt->lib_handle = dlopen(liblsc_path[version_id], RTLD_NOW);
	if (!cxt->lib_handle) {
		ISP_LOGE("fail to dlopen lsc lib");
		rtn = LSC_ERROR;
		goto exit;
	}

	cxt->lib_ops.alsc_init = dlsym(cxt->lib_handle, "lsc_adv_init");
	if (!cxt->lib_ops.alsc_init) {
		ISP_LOGE("fail to dlsym lsc_sprd_init");
		goto error_dlsym;
	}

	cxt->lib_ops.alsc_calc = dlsym(cxt->lib_handle, "lsc_adv_calculation");
	if (!cxt->lib_ops.alsc_calc) {
		ISP_LOGE("fail to dlsym lsc_sprd_calculation");
		goto error_dlsym;
	}

	cxt->lib_ops.alsc_io_ctrl = dlsym(cxt->lib_handle, "lsc_adv_ioctrl");
	if (!cxt->lib_ops.alsc_io_ctrl) {
		ISP_LOGE("fail to dlsym lsc_sprd_io_ctrl");
		goto error_dlsym;
	}

	cxt->lib_ops.alsc_deinit = dlsym(cxt->lib_handle, "lsc_adv_deinit");
	if (!cxt->lib_ops.alsc_deinit) {
		ISP_LOGE("fail to dlsym lsc_sprd_deinit");
		goto error_dlsym;
	}
	ISP_LOGI("load lsc lib success");

	return LSC_SUCCESS;

  error_dlsym:
	rtn = _lscsprd_unload_lib(cxt);

  exit:
	ISP_LOGE("fail to load lsc lib ret = %d", rtn);
	return rtn;
}

static void *lsc_sprd_init(void *in, void *out)
{
	cmr_s32 rtn = LSC_SUCCESS;
	struct lsc_ctrl_context *cxt = NULL;
	struct lsc_adv_init_param *init_param = (struct lsc_adv_init_param *)in;
	void *alsc_handle = NULL;
	UNUSED(out);

	//parser lsc otp info
	_lsc_parser_otp(init_param);

	cxt = (struct lsc_ctrl_context *)malloc(sizeof(struct lsc_ctrl_context));
	if (NULL == cxt) {
		rtn = LSC_ALLOC_ERROR;
		ISP_LOGE("fail to alloc!");
		goto EXIT;
	}

	memset(cxt, 0, sizeof(*cxt));

	cxt->dst_gain = (cmr_u16 *) malloc(32 * 32 * 4 * sizeof(cmr_u16));
	if (NULL == cxt->dst_gain) {
		rtn = LSC_ALLOC_ERROR;
		ISP_LOGE("fail to alloc dst_gain!");
		goto EXIT;
	}

	cxt->lsc_buffer = (cmr_u16 *) malloc(32 * 32 * 4 * sizeof(cmr_u16));
	if (NULL == cxt->lsc_buffer) {
		rtn = LSC_ALLOC_ERROR;
		ISP_LOGE("fail to alloc lsc_buffer!");
		goto EXIT;
	}
	init_param->lsc_buffer_addr = cxt->lsc_buffer;
	cxt->gain_pattern = init_param->gain_pattern;
	cxt->lib_info = &init_param->lib_param;

	rtn = _lscsprd_load_lib(cxt);
	if (LSC_SUCCESS != rtn) {
		ISP_LOGE("fail to load lsc lib");
		goto EXIT;
	}

	alsc_handle = cxt->lib_ops.alsc_init(init_param);
	if (NULL == alsc_handle) {
		ISP_LOGE("fail to do alsc init!");
		rtn = LSC_ALLOC_ERROR;
		goto EXIT;
	}

	cxt->alsc_handle = alsc_handle;

	pthread_mutex_init(&cxt->status_lock, NULL);

	ISP_LOGI("lsc init success rtn = %d", rtn);
	return (void *)cxt;

  EXIT:

	if (NULL != cxt) {
		rtn = _lscsprd_unload_lib(cxt);
		free(cxt);
		cxt = NULL;
	}

	ISP_LOGI("done rtn = %d", rtn);

	return NULL;
}

static cmr_s32 lsc_sprd_deinit(void *handle, void *in, void *out)
{
	cmr_s32 rtn = LSC_SUCCESS;
	struct lsc_ctrl_context *cxt = NULL;
	UNUSED(in);
	UNUSED(out);

	if (!handle) {
		ISP_LOGE("fail to check param!");
		return LSC_ERROR;
	}

	cxt = (struct lsc_ctrl_context *)handle;

	rtn = cxt->lib_ops.alsc_deinit(cxt->alsc_handle);
	if (LSC_SUCCESS != rtn) {
		ISP_LOGE("fail to do alsc deinit!");
	}

	rtn = _lscsprd_unload_lib(cxt);
	if (LSC_SUCCESS != rtn) {
		ISP_LOGE("fail to unload lsc lib");
	}

	pthread_mutex_destroy(&cxt->status_lock);

	free(cxt->dst_gain);
	cxt->dst_gain = NULL;
	free(cxt->lsc_buffer);
	cxt->lsc_buffer = NULL;

	memset(cxt, 0, sizeof(*cxt));
	free(cxt);
	cxt = NULL;
	ISP_LOGI("done rtn = %d", rtn);

	return rtn;
}

static cmr_s32 lsc_sprd_calculation(void *handle, void *in, void *out)
{
	cmr_int rtn = LSC_SUCCESS;
	struct lsc_ctrl_context *cxt = NULL;
	struct lsc_adv_calc_param *param = (struct lsc_adv_calc_param *)in;
	struct lsc_adv_calc_result *result = (struct lsc_adv_calc_result *)out;
	struct alsc_update_info update_info = { 0, 0, NULL };
	cmr_s32 dst_gain_size = param->gain_width * param->gain_height * 4 * sizeof(cmr_u16);

	if (!handle) {
		ISP_LOGE("fail to check param is NULL!");
		return LSC_ERROR;
	}

	cxt = (struct lsc_ctrl_context *)handle;

	result->dst_gain = cxt->dst_gain;
	ATRACE_BEGIN(__FUNCTION__);
	cmr_u64 ae_time0 = systemTime(CLOCK_MONOTONIC);
	rtn = cxt->lib_ops.alsc_calc(cxt->alsc_handle, param, result);
	cmr_u64 ae_time1 = systemTime(CLOCK_MONOTONIC);
	ATRACE_END();
	ISP_LOGV("SYSTEM_TEST -lsc_test  %dus ", (cmr_s32) ((ae_time1 - ae_time0) / 1000));
	rtn = cxt->lib_ops.alsc_io_ctrl(cxt->alsc_handle, ALSC_GET_UPDATE_INFO, NULL, (void *)&update_info);
	if (update_info.can_update_dest == 1) {
#ifdef CONFIG_ISP_2_3
		cmr_s32 i = 0;
		switch (cxt->gain_pattern) {
		case LSC_GAIN_PATTERN_GRBG:
		{
			for (i = 0; i < dst_gain_size / 8; i++) {
				*((cmr_u16 *)update_info.lsc_buffer_addr + i * 4+ 0) = *((cmr_u16 *)result->dst_gain + i * 4+ 3);
				*((cmr_u16 *)update_info.lsc_buffer_addr + i * 4+ 1) = *((cmr_u16 *)result->dst_gain + i * 4+ 2);
				*((cmr_u16 *)update_info.lsc_buffer_addr + i * 4+ 2) = *((cmr_u16 *)result->dst_gain + i * 4+ 1);
				*((cmr_u16 *)update_info.lsc_buffer_addr + i * 4+ 3) = *((cmr_u16 *)result->dst_gain + i * 4+ 0);
			}
			break;
		}
		case LSC_GAIN_PATTERN_RGGB:
		{
			for (i = 0; i < dst_gain_size / 8; i++) {
				*((cmr_u16 *)update_info.lsc_buffer_addr + i * 4+ 0) = *((cmr_u16 *)result->dst_gain + i * 4+ 2);
				*((cmr_u16 *)update_info.lsc_buffer_addr + i * 4+ 1) = *((cmr_u16 *)result->dst_gain + i * 4+ 3);
				*((cmr_u16 *)update_info.lsc_buffer_addr + i * 4+ 2) = *((cmr_u16 *)result->dst_gain + i * 4+ 0);
				*((cmr_u16 *)update_info.lsc_buffer_addr + i * 4+ 3) = *((cmr_u16 *)result->dst_gain + i * 4+ 1);
			}
			break;
		}
		case LSC_GAIN_PATTERN_BGGR:
		{
			for (i = 0; i < dst_gain_size / 8; i++) {
				*((cmr_u16 *)update_info.lsc_buffer_addr + i * 4+ 0) = *((cmr_u16 *)result->dst_gain + i * 4+ 1);
				*((cmr_u16 *)update_info.lsc_buffer_addr + i * 4+ 1) = *((cmr_u16 *)result->dst_gain + i * 4+ 0);
				*((cmr_u16 *)update_info.lsc_buffer_addr + i * 4+ 2) = *((cmr_u16 *)result->dst_gain + i * 4+ 3);
				*((cmr_u16 *)update_info.lsc_buffer_addr + i * 4+ 3) = *((cmr_u16 *)result->dst_gain + i * 4+ 2);
			}
			break;
		}
		default:
			memcpy(update_info.lsc_buffer_addr, result->dst_gain, dst_gain_size);
			break;
		}

#else
		memcpy(update_info.lsc_buffer_addr, result->dst_gain, dst_gain_size);
#endif
		rtn = cxt->lib_ops.alsc_io_ctrl(cxt->alsc_handle, ALSC_UNLOCK_UPDATE_FLAG, NULL, NULL);
	}

	return rtn;
}

static cmr_s32 lsc_sprd_ioctrl(void *handle, cmr_s32 cmd, void *in, void *out)
{
	cmr_int rtn = LSC_SUCCESS;
	struct lsc_ctrl_context *cxt = NULL;

	if (!handle) {
		ISP_LOGE("fail to check param, param is NULL!");
		return LSC_ERROR;
	}

	cxt = (struct lsc_ctrl_context *)handle;

	rtn = cxt->lib_ops.alsc_io_ctrl(cxt->alsc_handle, (enum alsc_io_ctrl_cmd)cmd, in, out);

	return rtn;
}

/*
void alsc_set_param(struct lsc_adv_init_param *lsc_param)
{
	//	alg_open
	//	0: front_camera close, back_camera close;
	//	1: front_camera open, back_camera open;
	//	2: front_camera close, back_camera open;
	lsc_param->alg_open = 1;
       	lsc_param->tune_param.enable = 1;
#if SMART_LSC_VERSION
	lsc_param->tune_param.alg_id = 2;
#else
	lsc_param->tune_param.alg_id = 0;
#endif
	//common
	lsc_param->tune_param.strength_level = 5;
	lsc_param->tune_param.pa = 1;
	lsc_param->tune_param.pb = 1;
	lsc_param->tune_param.fft_core_id = 0;
	lsc_param->tune_param.con_weight =	5;		//double avg weight:[1~16]; weight =100 for 10 tables avg
	lsc_param->tune_param.restore_open = 0;
	lsc_param->tune_param.freq = 3;

	//Smart LSC Setting
	lsc_param->SLSC_setting.num_seg_queue = 10;
	lsc_param->SLSC_setting.num_seg_vote_th = 8;
	lsc_param->SLSC_setting.proc_inter = 3;
	lsc_param->SLSC_setting.seg_count = 4;
	lsc_param->SLSC_setting.smooth_ratio = 0.9;
	lsc_param->SLSC_setting.segs[0].table_pair[0]=LSC_TAB_A;
	lsc_param->SLSC_setting.segs[0].table_pair[1]=LSC_TAB_TL84;
	lsc_param->SLSC_setting.segs[0].max_CT = 3999;
	lsc_param->SLSC_setting.segs[0].min_CT = 0;
	lsc_param->SLSC_setting.segs[1].table_pair[0]=LSC_TAB_A;
	lsc_param->SLSC_setting.segs[1].table_pair[1]=LSC_TAB_CWF;
	lsc_param->SLSC_setting.segs[1].max_CT = 3999;
	lsc_param->SLSC_setting.segs[1].min_CT = 0;
	lsc_param->SLSC_setting.segs[2].table_pair[0]=LSC_TAB_D65;
	lsc_param->SLSC_setting.segs[2].table_pair[1]=LSC_TAB_TL84;
	lsc_param->SLSC_setting.segs[2].max_CT = 10000;
	lsc_param->SLSC_setting.segs[2].min_CT = 0;
	lsc_param->SLSC_setting.segs[3].table_pair[0]=LSC_TAB_D65;
	lsc_param->SLSC_setting.segs[3].table_pair[1]=LSC_TAB_CWF;
	lsc_param->SLSC_setting.segs[3].max_CT = 10000;
	lsc_param->SLSC_setting.segs[3].min_CT = 0;

	lsc_param->SLSC_setting.segs[4].table_pair[0]=LSC_TAB_D65;
	lsc_param->SLSC_setting.segs[4].table_pair[1]=LSC_TAB_DNP;
	lsc_param->SLSC_setting.segs[4].max_CT = 10000;
	lsc_param->SLSC_setting.segs[4].min_CT = 0;
}
*/

cmr_int lsc_ctrl_init(struct lsc_adv_init_param * input_ptr, cmr_handle * handle_lsc)
{
	cmr_int rtn = ISP_SUCCESS;
	struct lsc_ctrl_cxt *cxt_ptr = NULL;

	cxt_ptr = (struct lsc_ctrl_cxt *)malloc(sizeof(struct lsc_ctrl_cxt));
	if (NULL == cxt_ptr) {
		ISP_LOGE("fail to create lsc ctrl context!");
		rtn = LSC_ALLOC_ERROR;
		goto exit;
	}
	memset(cxt_ptr, 0, sizeof(struct lsc_ctrl_cxt));

	rtn = _lscctrl_create_thread(cxt_ptr);
	if (rtn) {
		goto exit;
	}

	rtn = _lscctrl_init_adpt(cxt_ptr, input_ptr);
	if (rtn) {
		goto exit;
	}

  exit:
	if (rtn) {
		if (cxt_ptr) {
			free(cxt_ptr);
		}
	} else {
		*handle_lsc = (cmr_handle) cxt_ptr;
	}
	ISP_LOGI("done %ld", rtn);

	return rtn;
}

cmr_int lsc_ctrl_deinit(cmr_handle * handle_lsc)
{
	cmr_int rtn = LSC_SUCCESS;
	struct lsc_ctrl_cxt *cxt_ptr = *handle_lsc;
	CMR_MSG_INIT(message);

	if (!cxt_ptr) {
		ISP_LOGE("fail to check param, param is NULL!");
		rtn = LSC_HANDLER_NULL;
		goto exit;
	}

	message.msg_type = LSCCTRL_EVT_DEINIT;
	message.sync_flag = CMR_MSG_SYNC_PROCESSED;
	message.alloc_flag = 0;
	message.data = NULL;
	rtn = cmr_thread_msg_send(cxt_ptr->thr_handle, &message);
	if (rtn) {
		ISP_LOGE("failed to send msg to lsc thr %ld", rtn);
		goto exit;
	}

	rtn = _lscctrl_destroy_thread(cxt_ptr);
	if (rtn) {
		ISP_LOGE("fail to destroy lscctrl thread %ld", rtn);
		goto exit;
	}

  exit:
	if (cxt_ptr) {
		free((void *)cxt_ptr);
		*handle_lsc = NULL;
	}

	ISP_LOGI("done %ld", rtn);
	return rtn;
}

cmr_int lsc_ctrl_process(cmr_handle handle_lsc, struct lsc_adv_calc_param * in_ptr, struct lsc_adv_calc_result * result)
{
	cmr_int rtn = LSC_SUCCESS;
	struct lsc_ctrl_cxt *cxt_ptr = (struct lsc_ctrl_cxt *)handle_lsc;

	if (!handle_lsc || !in_ptr || !result) {
		ISP_LOGE("fail to check param, param is NULL!");
		rtn = LSC_HANDLER_NULL;
		goto exit;
	}
	//cxt_ptr->proc_out.dst_gain = result->dst_gain;

	CMR_MSG_INIT(message);
	message.data = malloc(sizeof(struct lsc_adv_calc_param));
	if (!message.data) {
		ISP_LOGE("fail to malloc msg");
		rtn = LSC_ALLOC_ERROR;
		goto exit;
	}

	memcpy(message.data, (void *)in_ptr, sizeof(struct lsc_adv_calc_param));
	message.alloc_flag = 1;
	message.msg_type = LSCCTRL_EVT_PROCESS;
	message.sync_flag = CMR_MSG_SYNC_NONE;
	rtn = cmr_thread_msg_send(cxt_ptr->thr_handle, &message);

	if (rtn) {
		ISP_LOGE("fail to send msg to lsc thr %ld", rtn);
		if (message.data)
			free(message.data);
		goto exit;
	}

  exit:
	ISP_LOGV("done %ld", rtn);
	return rtn;
}

cmr_int lsc_ctrl_ioctrl(cmr_handle handle_lsc, cmr_s32 cmd, void *in_ptr, void *out_ptr)
{
	cmr_int rtn = LSC_SUCCESS;
	struct lsc_ctrl_cxt *cxt_ptr = (struct lsc_ctrl_cxt *)handle_lsc;
	struct lsc_ctrl_work_lib *lib_ptr = NULL;

	if (!handle_lsc) {
		ISP_LOGE("fail to check param, param is NULL!");
		rtn = LSC_HANDLER_NULL;
		goto exit;
	}

	lib_ptr = &cxt_ptr->work_lib;
	if (lib_ptr->adpt_ops->adpt_ioctrl) {
		rtn = lib_ptr->adpt_ops->adpt_ioctrl(lib_ptr->lib_handle, cmd, in_ptr, out_ptr);
	} else {
		ISP_LOGI("ioctrl fun is NULL");
	}
  exit:
	ISP_LOGV("cmd = %d,done %ld", cmd, rtn);
	return rtn;
}

struct adpt_ops_type lsc_sprd_adpt_ops_ver0 = {
	.adpt_init = lsc_sprd_init,
	.adpt_deinit = lsc_sprd_deinit,
	.adpt_process = lsc_sprd_calculation,
	.adpt_ioctrl = lsc_sprd_ioctrl,
};
