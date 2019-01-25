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
#define LOG_TAG "isp_com_alg"
#include "isp_com_alg.h"
#ifdef WIN32
#include "memory.h"
#include "malloc.h"
#endif
#include "cmr_types.h"
#include "isp_type.h"

#define INTERP_WEIGHT_UNIT 256

cmr_s32 isp_gamma_adjust(struct isp_gamma_curve_info * src_ptr0, struct isp_gamma_curve_info * src_ptr1, struct isp_gamma_curve_info * dst_ptr, struct isp_weight_value * point_ptr)
{

	cmr_s32 rtn = ISP_SUCCESS;
	cmr_u32 i = 0, tmp = 0;
	cmr_u32 scale_coef = 0;

	if (point_ptr->value[0] == point_ptr->value[1]) {
		memcpy((void *)dst_ptr, src_ptr0, sizeof(struct isp_gamma_curve_info));
	} else {
		scale_coef = point_ptr->weight[0] + point_ptr->weight[1];
		if (0 == scale_coef) {
			ISP_LOGE("fail to get weight[2] (%d, %d)\n", point_ptr->weight[0], point_ptr->weight[1]);
			return ISP_ERROR;
		}
		for (i = ISP_ZERO; i < ISP_GAMMA_SAMPLE_NUM; i++) {
			dst_ptr->axis[0][i] = src_ptr0->axis[0][i];
			tmp = (src_ptr0->axis[0][i] * point_ptr->weight[0] + src_ptr1->axis[0][i] * point_ptr->weight[1]) / scale_coef;
			dst_ptr->axis[1][i] = tmp;
		}
	}

	return rtn;
}

cmr_s32 isp_cmc_adjust(cmr_u16 src0[9], cmr_u16 src1[9], struct isp_sample_point_info * point_ptr, cmr_u16 dst[9])
{
	cmr_u32 i, scale_coeff;

	if ((NULL == src0) || (NULL == src1) || (NULL == dst)) {
		ISP_LOGE("fail to get vailed pointer src0;%p /src1:%p /dst:%p\n", src0, src1, dst);
		return ISP_ERROR;
	}

	if ((0 == point_ptr->weight0) && (0 == point_ptr->weight1)) {
		ISP_LOGE("fail to get validate all weight \n");
		return ISP_ERROR;
	}

	if (point_ptr->x0 == point_ptr->x1) {
		memcpy((void *)dst, (void *)src0, sizeof(cmr_u16) * 9);
	} else {
		if (0 == point_ptr->weight0) {
			memcpy((void *)dst, (void *)src1, sizeof(cmr_u16) * 9);
		} else if (0 == point_ptr->weight1) {
			memcpy((void *)dst, (void *)src0, sizeof(cmr_u16) * 9);
		} else {
			scale_coeff = point_ptr->weight0 + point_ptr->weight1;
			for (i = 0; i < 9; i++) {
				cmr_s32 x0 = (cmr_s16) (src0[i] << 2);
				cmr_s32 x1 = (cmr_s16) (src1[i] << 2);
				cmr_s32 x = (x0 * point_ptr->weight0 + x1 * point_ptr->weight1) / scale_coeff;
				dst[i] = (cmr_u16) ((x >> 2) & 0x3fff);
			}
		}
	}

	return ISP_SUCCESS;
}

cmr_s32 isp_cmc_adjust_4_reduce_saturation(cmr_u16 cmc_in[9], cmr_u16 cmc_out[9], cmr_u32 percent)
{
	static const cmr_s64 r2y[9] = { 299, 587, 114, -168, -331, 500, 500, -419, -81 };
	static const cmr_s64 y2r[9] = { 1000, 0, 1402, 1000, -344, -714, 1000, 1772, 0 };
	cmr_s64 cmc_matrix[9];
	cmr_s64 matrix_0[9];
	cmr_s64 matrix_1[9];
	cmr_u8 i = 0x00;
	cmr_s16 calc_matrix[9];
	cmr_u16 *matrix_ptr = PNULL;

	matrix_ptr = (cmr_u16 *) cmc_out;

	percent = (0 == percent) ? 1 : percent;

	if (255 == percent) {
		matrix_ptr[0] = cmc_in[0];
		matrix_ptr[1] = cmc_in[1];
		matrix_ptr[2] = cmc_in[2];
		matrix_ptr[3] = cmc_in[3];
		matrix_ptr[4] = cmc_in[4];
		matrix_ptr[5] = cmc_in[5];
		matrix_ptr[6] = cmc_in[6];
		matrix_ptr[7] = cmc_in[7];
		matrix_ptr[8] = cmc_in[8];

	} else if (0 < percent) {
		matrix_0[0] = y2r[0] * 255;
		matrix_0[1] = y2r[1] * percent;
		matrix_0[2] = y2r[2] * percent;
		matrix_0[3] = y2r[3] * 255;
		matrix_0[4] = y2r[4] * percent;
		matrix_0[5] = y2r[5] * percent;
		matrix_0[6] = y2r[6] * 255;
		matrix_0[7] = y2r[7] * percent;
		matrix_0[8] = y2r[8] * percent;

		matrix_1[0] = matrix_0[0] * r2y[0] + matrix_0[1] * r2y[3] + matrix_0[2] * r2y[6];
		matrix_1[1] = matrix_0[0] * r2y[1] + matrix_0[1] * r2y[4] + matrix_0[2] * r2y[7];
		matrix_1[2] = matrix_0[0] * r2y[2] + matrix_0[1] * r2y[5] + matrix_0[2] * r2y[8];
		matrix_1[3] = matrix_0[3] * r2y[0] + matrix_0[4] * r2y[3] + matrix_0[5] * r2y[6];
		matrix_1[4] = matrix_0[3] * r2y[1] + matrix_0[4] * r2y[4] + matrix_0[5] * r2y[7];
		matrix_1[5] = matrix_0[3] * r2y[2] + matrix_0[4] * r2y[5] + matrix_0[5] * r2y[8];
		matrix_1[6] = matrix_0[6] * r2y[0] + matrix_0[7] * r2y[3] + matrix_0[8] * r2y[6];
		matrix_1[7] = matrix_0[6] * r2y[1] + matrix_0[7] * r2y[4] + matrix_0[8] * r2y[7];
		matrix_1[8] = matrix_0[6] * r2y[2] + matrix_0[7] * r2y[5] + matrix_0[8] * r2y[8];

		matrix_0[0] = cmc_in[0];
		matrix_0[1] = cmc_in[1];
		matrix_0[2] = cmc_in[2];
		matrix_0[3] = cmc_in[3];
		matrix_0[4] = cmc_in[4];
		matrix_0[5] = cmc_in[5];
		matrix_0[6] = cmc_in[6];
		matrix_0[7] = cmc_in[7];
		matrix_0[8] = cmc_in[8];

		for (i = 0x00; i < 9; i++) {
			if (0 != (matrix_0[i] & 0x2000)) {
				matrix_0[i] |= 0xffffffffffffc000;
			}
		}

		cmc_matrix[0] = matrix_1[0] * matrix_0[0] + matrix_1[1] * matrix_0[3] + matrix_1[2] * matrix_0[6];
		cmc_matrix[1] = matrix_1[0] * matrix_0[1] + matrix_1[1] * matrix_0[4] + matrix_1[2] * matrix_0[7];
		cmc_matrix[2] = matrix_1[0] * matrix_0[2] + matrix_1[1] * matrix_0[5] + matrix_1[2] * matrix_0[8];
		cmc_matrix[3] = matrix_1[3] * matrix_0[0] + matrix_1[4] * matrix_0[3] + matrix_1[5] * matrix_0[6];
		cmc_matrix[4] = matrix_1[3] * matrix_0[1] + matrix_1[4] * matrix_0[4] + matrix_1[5] * matrix_0[7];
		cmc_matrix[5] = matrix_1[3] * matrix_0[2] + matrix_1[4] * matrix_0[5] + matrix_1[5] * matrix_0[8];
		cmc_matrix[6] = matrix_1[6] * matrix_0[0] + matrix_1[7] * matrix_0[3] + matrix_1[8] * matrix_0[6];
		cmc_matrix[7] = matrix_1[6] * matrix_0[1] + matrix_1[7] * matrix_0[4] + matrix_1[8] * matrix_0[7];
		cmc_matrix[8] = matrix_1[6] * matrix_0[2] + matrix_1[7] * matrix_0[5] + matrix_1[8] * matrix_0[8];

		cmc_matrix[0] = cmc_matrix[0] / 1000 / 1000 / 255;
		cmc_matrix[1] = cmc_matrix[1] / 1000 / 1000 / 255;
		cmc_matrix[2] = cmc_matrix[2] / 1000 / 1000 / 255;
		cmc_matrix[3] = cmc_matrix[3] / 1000 / 1000 / 255;
		cmc_matrix[4] = cmc_matrix[4] / 1000 / 1000 / 255;
		cmc_matrix[5] = cmc_matrix[5] / 1000 / 1000 / 255;
		cmc_matrix[6] = cmc_matrix[6] / 1000 / 1000 / 255;
		cmc_matrix[7] = cmc_matrix[7] / 1000 / 1000 / 255;
		cmc_matrix[8] = cmc_matrix[8] / 1000 / 1000 / 255;

		calc_matrix[0] = (cmr_s16) (cmc_matrix[0] & 0x3fff);
		calc_matrix[1] = (cmr_s16) (cmc_matrix[1] & 0x3fff);
		calc_matrix[2] = (cmr_s16) (cmc_matrix[2] & 0x3fff);
		calc_matrix[3] = (cmr_s16) (cmc_matrix[3] & 0x3fff);
		calc_matrix[4] = (cmr_s16) (cmc_matrix[4] & 0x3fff);
		calc_matrix[5] = (cmr_s16) (cmc_matrix[5] & 0x3fff);
		calc_matrix[6] = (cmr_s16) (cmc_matrix[6] & 0x3fff);
		calc_matrix[7] = (cmr_s16) (cmc_matrix[7] & 0x3fff);
		calc_matrix[8] = (cmr_s16) (cmc_matrix[8] & 0x3fff);

		matrix_ptr[0] = calc_matrix[0];
		matrix_ptr[1] = calc_matrix[1];
		matrix_ptr[2] = calc_matrix[2];
		matrix_ptr[3] = calc_matrix[3];
		matrix_ptr[4] = calc_matrix[4];
		matrix_ptr[5] = calc_matrix[5];
		matrix_ptr[6] = calc_matrix[6];
		matrix_ptr[7] = calc_matrix[7];
		matrix_ptr[8] = calc_matrix[8];
	}

	return 0;
}

cmr_s32 isp_cce_adjust(cmr_u16 src[9], cmr_u16 coef[3], cmr_u16 dst[9], cmr_u16 base_gain)
{
	cmr_s32 matrix[3][3];
	cmr_s32 *matrix_ptr = NULL;
	cmr_u16 *src_ptr = NULL;
	cmr_u16 *dst_ptr = NULL;
	cmr_s32 tmp = 0;
	cmr_u32 i = 0, j = 0;

	if ((NULL == src) || (NULL == dst) || (NULL == coef)) {
		ISP_LOGE("fail to get vailed pointer src;%p /dst:%p /coef:%p\n", src, dst, coef);
		return ISP_ERROR;
	}

	src_ptr = (cmr_u16 *) src;
	matrix_ptr = (cmr_s32 *) & matrix[0][0];
	for (i = 0x00; i < 9; i++) {
		if (ISP_ZERO != (src_ptr[i] & 0x8000)) {
			*(matrix_ptr + i) = (src_ptr[i]) | 0xffff0000;
		} else {
			*(matrix_ptr + i) = src_ptr[i];
		}
	}

	dst_ptr = (cmr_u16 *) dst;
	for (i = 0; i < 3; ++i) {
		for (j = 0; j < 3; ++j) {

			if (coef[j] == base_gain)
				tmp = matrix[i][j];
			else
				tmp = (((cmr_s32) coef[j]) * matrix[i][j] + base_gain / 2) / base_gain;

			*dst_ptr = (cmr_u16) (((cmr_u32) tmp) & 0xffff);
			dst_ptr++;
		}
	}

	return ISP_SUCCESS;
}

cmr_s32 isp_lsc_adjust(void *lnc0_ptr, void *lnc1_ptr, cmr_u32 lnc_len, struct isp_weight_value * point_ptr, void *dst_lnc_ptr)
{
	cmr_s32 rtn = ISP_SUCCESS;
	cmr_u32 i = 0x00;

	cmr_u16 *src0_ptr = (cmr_u16 *) lnc0_ptr;
	cmr_u16 *src1_ptr = (cmr_u16 *) lnc1_ptr;
	cmr_u16 *dst_ptr = (cmr_u16 *) dst_lnc_ptr;

	if ((NULL == lnc0_ptr)
	    || (NULL == dst_lnc_ptr)) {
		ISP_LOGE("fail to get lnc buf  %p, %p, %p \n", lnc0_ptr, lnc1_ptr, dst_lnc_ptr);
		return ISP_ERROR;
	}

	if ((0 == point_ptr->weight[0]) && (0 == point_ptr->weight[1])) {
		ISP_LOGE("fail to get weight[2]\n");
		return ISP_ERROR;
	}

	if (point_ptr->value[0] == point_ptr->value[1]) {
		memcpy(dst_lnc_ptr, lnc0_ptr, lnc_len);
	} else {
		if (0 == point_ptr->weight[0]) {
			memcpy(dst_lnc_ptr, lnc1_ptr, lnc_len);
		} else if (0 == point_ptr->weight[1]) {
			memcpy(dst_lnc_ptr, lnc0_ptr, lnc_len);
		} else {
			cmr_u32 lnc_num = lnc_len / sizeof(cmr_u16);
			cmr_u32 scale_coeff = 0;

			ISP_LOGV("lnc interpolation: index(%d, %d); weight: (%d, %d), num=%d, src0 = %p, src1 = %p",
				 point_ptr->value[0], point_ptr->value[1], point_ptr->weight[0], point_ptr->weight[1], lnc_num, src0_ptr, src1_ptr);

			scale_coeff = point_ptr->weight[0] + point_ptr->weight[1];

			for (i = 0x00; i < lnc_num; i++) {
				dst_ptr[i] = (src0_ptr[i] * point_ptr->weight[0] + src1_ptr[i] * point_ptr->weight[1]) / scale_coeff;
			}
		}
	}

	return rtn;
}

cmr_s32 isp_hue_saturation_2_gain(cmr_s32 hue, cmr_s32 saturation, struct isp_rgb_gains * gain)
{
	cmr_s32 rtn = ISP_SUCCESS;

	gain->gain_r = ISP_FIX_10BIT_UNIT;
	gain->gain_g = ISP_FIX_10BIT_UNIT;
	gain->gain_b = ISP_FIX_10BIT_UNIT;

	if (hue < 1 || hue > 60 || saturation > 100 || saturation < 1) {
		return rtn;
	}

	gain->gain_b = (cmr_u32) ((6000 - 60 * saturation) * ISP_FIX_10BIT_UNIT / (6000 + hue * saturation - 60 * saturation));
	gain->gain_g = (cmr_u32) ISP_FIX_10BIT_UNIT;
	gain->gain_r = (cmr_u32) (((hue - 60) * gain->gain_b + 60 * ISP_FIX_10BIT_UNIT) / hue);

	ISP_LOGV("gain = (%d, %d, %d)", gain->gain_r, gain->gain_g, gain->gain_b);

	return rtn;
}

static void _interp_uint8(cmr_u8 * dst, cmr_u8 * src[2], cmr_u16 weight[2], cmr_u32 data_num)
{
	cmr_u32 data_bytes = 0;
	cmr_u32 i = 0;

	data_bytes = data_num * sizeof(cmr_u8);

	if (INTERP_WEIGHT_UNIT == weight[0]) {
		memcpy(dst, src[0], data_bytes);
	} else if (INTERP_WEIGHT_UNIT == weight[1]) {
		memcpy(dst, src[1], data_bytes);
	} else {

		for (i = 0; i < data_num; i++) {

			cmr_u32 dst_val = 0;
			cmr_u32 src0_val = *src[0]++;
			cmr_u32 src1_val = *src[1]++;

			dst_val = (src0_val * weight[0] + src1_val * weight[1]) / INTERP_WEIGHT_UNIT;

			*dst++ = dst_val;
		}
	}
}

static void _interp_uint16(cmr_u16 * dst, cmr_u16 * src[2], cmr_u16 weight[2], cmr_u32 data_num)
{
	cmr_u32 data_bytes = 0;
	cmr_u32 i = 0;

	data_bytes = data_num * sizeof(cmr_u16);

	if (INTERP_WEIGHT_UNIT == weight[0]) {
		memcpy(dst, src[0], data_bytes);
	} else if (INTERP_WEIGHT_UNIT == weight[1]) {
		memcpy(dst, src[1], data_bytes);
	} else {

		for (i = 0; i < data_num; i++) {

			cmr_u32 dst_val = 0;
			cmr_u32 src0_val = *src[0]++;
			cmr_u32 src1_val = *src[1]++;

			dst_val = (src0_val * weight[0] + src1_val * weight[1]) / INTERP_WEIGHT_UNIT;

			*dst++ = dst_val;
		}
	}
}

static void _interp_uint32(cmr_u32 * dst, cmr_u32 * src[2], cmr_u16 weight[2], cmr_u32 data_num)
{
	cmr_u32 data_bytes = 0;
	cmr_u32 i = 0;

	data_bytes = data_num * sizeof(cmr_u32);

	if (INTERP_WEIGHT_UNIT == weight[0]) {
		memcpy(dst, src[0], data_bytes);
	} else if (INTERP_WEIGHT_UNIT == weight[1]) {
		memcpy(dst, src[1], data_bytes);
	} else {

		for (i = 0; i < data_num; i++) {

			cmr_u32 dst_val = 0;
			cmr_u32 src0_val = *src[0]++;
			cmr_u32 src1_val = *src[1]++;

			dst_val = (src0_val * weight[0] + src1_val * weight[1]) / INTERP_WEIGHT_UNIT;

			*dst++ = dst_val;
		}
	}
}

static void _interp_int14(cmr_u16 * dst, cmr_u16 * src[2], cmr_u16 weight[2], cmr_u32 data_num)
{
	cmr_u32 data_bytes = 0;
	cmr_u32 i = 0;

	data_bytes = data_num * sizeof(cmr_u16);

	if (INTERP_WEIGHT_UNIT == weight[0]) {
		memcpy(dst, src[0], data_bytes);
	} else if (INTERP_WEIGHT_UNIT == weight[1]) {
		memcpy(dst, src[1], data_bytes);
	} else {

		for (i = 0; i < data_num; i++) {

			cmr_u16 dst_val = 0;
			cmr_s16 src0_val = (cmr_s16) (*src[0] << 2);
			cmr_s16 src1_val = (cmr_s16) (*src[1] << 2);

			dst_val = (src0_val * weight[0] + src1_val * weight[1]) / INTERP_WEIGHT_UNIT;

			*dst++ = (cmr_u16) ((dst_val >> 2) & 0x3fff);
			src[0]++;
			src[1]++;
		}
	}
}

static void _interp_uint20(cmr_u32 * dst, cmr_u32 * src[2], cmr_u16 weight[2], cmr_u32 data_num)
{
	cmr_u32 data_bytes = 0;
	cmr_u32 i = 0;

	data_bytes = data_num * sizeof(cmr_u32);

	if (INTERP_WEIGHT_UNIT == weight[0]) {
		memcpy(dst, src[0], data_bytes);
	} else if (INTERP_WEIGHT_UNIT == weight[1]) {
		memcpy(dst, src[1], data_bytes);
	} else {
		for (i = 0; i < data_num; i++) {
			cmr_u32 src0_val = (cmr_u32) (*src[0]);
			cmr_u32 src1_val = (cmr_u32) (*src[1]);

			cmr_u32 src0_val_h = src0_val & 0x1FF;
			cmr_u32 src0_val_s = (src0_val >> 9) & 0x7FF;
			cmr_u32 src1_val_h = src1_val & 0x1FF;
			cmr_u32 src1_val_s = (src1_val >> 9) & 0x7FF;

			cmr_u32 dst_val_h = (src0_val_h * weight[0] + src1_val_h * weight[1]) / INTERP_WEIGHT_UNIT;
			cmr_u32 dst_val_s = (src0_val_s * weight[0] + src1_val_s * weight[1]) / INTERP_WEIGHT_UNIT;

			cmr_u32 dst_val = (dst_val_h & 0x1FF) | ((dst_val_s & 0x7FF) << 9);

			*dst++ = (cmr_u32) (dst_val);
			src[0]++;
			src[1]++;
		}
	}
}

cmr_s32 isp_interp_data(void *dst, void *src[2], cmr_u16 weight[2], cmr_u32 data_num, cmr_u32 data_type)
{
	cmr_s32 rtn = ISP_SUCCESS;

	ISP_LOGV("src[0]=%p, src[1]=%p, dst=%p, weight=(%d, %d), data_num=%d, type=%d", src[0], src[1], dst, weight[0], weight[1], data_num, data_type);

	if (NULL == dst || NULL == src[0] || NULL == src[1] || 0 == data_num) {
		ISP_LOGE("fail to get valid param: dst=%p, src=(%p, %p), data_num=%d\n", dst, src[0], src[1], data_num);
		return ISP_ERROR;
	}

	if (256 != weight[0] + weight[1]) {
		ISP_LOGE("fail to get valid weight[2] (%d, %d)\n", weight[0], weight[1]);
		return ISP_ERROR;
	}

	switch (data_type) {
	case ISP_INTERP_UINT8:
		_interp_uint8((cmr_u8 *) dst, (cmr_u8 **) src, weight, data_num);
		break;

	case ISP_INTERP_UINT16:
		_interp_uint16((cmr_u16 *) dst, (cmr_u16 **) src, weight, data_num);
		break;

	case ISP_INTERP_UINT32:
		_interp_uint32((cmr_u32 *) dst, (cmr_u32 **) src, weight, data_num);
		break;

	case ISP_INTERP_INT14:
		_interp_int14((cmr_u16 *) dst, (cmr_u16 **) src, weight, data_num);
		break;

	case ISP_INTERP_UINT20:
		_interp_uint20((cmr_u32 *) dst, (cmr_u32 **) src, weight, data_num);
		break;

	default:
		ISP_LOGE("fail to get a valid data type %d\n", data_type);
		rtn = ISP_ERROR;
		break;
	}

	return rtn;
}
